//
//   ____                ___ _   _____ 
//  / __ \___  ___ ___  / _ \ | / / _ \
// / /_/ / _ \/ -_) _ \/ ___/ |/ / // /
// \____/ .__/\__/_//_/_/   |___/____/ 
//     /_/  https://github.com/ishani/OpenPVD
// 
// 
//

#include "pch.h"
#include "common/OpFoundation.h"
#include "common/OpEventBreaker.h"
#include "common/OpEventUnpacker.h"

#include "PxPvdCommStreamEvents.h"
#include "PxPvdDefaultFileTransport.h"
#include "PxPvdDefaultSocketTransport.h"

// ---------------------------------------------------------------------------------------------------------------------
namespace cmdline
{
    enum class OutputMode
    {
        None,
        File,
        Network
    };

    static std::string PxDInput     = "testdata/basic.pxd2";
    static std::string PxDOutput    = "testdata/filtered.pxd2";
    static std::string PxDAddress   = "127.0.0.1";
    static uint16_t PvPort          = 5425;

    static int32_t TriMeshLimit     = -1;

    static OutputMode AppOutputMode = OutputMode::None;

    int parse( int argc, char** argv )
    {
        CLI::App app{ "OpenPVD" };

        app.add_option( "-p,--pxd", PxDInput, "path to a PXD2 capture file to parse" )->check( CLI::ExistingFile );
        app.add_option( "--meshlimit", TriMeshLimit, "limit of trimesh instances to allow")->check( CLI::PositiveNumber );

        // optional output mode selection
        CLI::App* outToFile = app.add_subcommand( "to_file", "" );
        CLI::App* outToNet  = app.add_subcommand( "to_net", "" );
        app.require_subcommand(-1); // require 1 subcommand at most

        outToFile->add_option( "-o,--out", PxDOutput, "where to write a filtered PXD2 output" );

        outToNet->add_option( "-o,--out", PxDAddress, "address to connect to" );
        outToNet->add_option( "-p,--port", PvPort, "port to connect to" );

        CLI11_PARSE( app, argc, argv );

        if ( *outToFile )
            AppOutputMode = OutputMode::File;
        if ( *outToNet )
            AppOutputMode = OutputMode::Network;

        return 0;
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// a transport that does nothing, used as a default output
//
class NullTransport : public physx::PxPvdTransport
{
public:
    bool connect() override { return true; }
    void disconnect() override {}
    bool isConnected() override { return true; }
    bool write( const uint8_t* inBytes, uint32_t inLength ) override { return true; }
    PxPvdTransport& lock() override { return *this; }
    void unlock() override { }
    void flush() override { }
    uint64_t getWrittenDataSize() override { return 0; }
    void release() override { }

    static NullTransport Instance;
};
NullTransport NullTransport::Instance;

// ---------------------------------------------------------------------------------------------------------------------
int main( int argc, char** argv )
{
    spdlog::set_pattern( "[%^%L%$] %v" );

    if ( int cmdr = cmdline::parse( argc, argv ) )
        return cmdr;

    if ( !fs::exists( cmdline::PxDInput ) )
    {
        spdlog::error( "Cannot find PXD file [{}]", cmdline::PxDInput );
        exit( 1 );
    }

    Op::Foundation opFoundation;
    {
        spdlog::info( "Loading : {}", cmdline::PxDInput );
        auto PxDFile = physx::PsFileBuffer( cmdline::PxDInput.c_str(), physx::general_PxIOStream2::PxFileBuf::OPEN_READ_ONLY );

        auto eventUnpacker = Op::EventUnpacker< physx::PsFileBuffer >( PxDFile );

        Op::EventBreaker eventBreaker;

        // default to not emitting the stream with or without filtering to a file/network connection
        physx::PxPvdTransport* outboundTransport = &NullTransport::Instance;
        bool bSerialize = false;

        if ( cmdline::AppOutputMode == cmdline::OutputMode::File )
        {
            if ( !cmdline::PxDOutput.empty() )
            {
                spdlog::info( "Writing to file : {}", cmdline::PxDOutput );

                outboundTransport = physx::PxDefaultPvdFileTransportCreate( cmdline::PxDOutput.c_str() );
                bSerialize = true;
            }
        }
        if ( cmdline::AppOutputMode == cmdline::OutputMode::Network )
        {
            if ( !cmdline::PxDAddress.empty() )
            {
                spdlog::info( "Writing to network : {}", cmdline::PxDAddress );

                outboundTransport = physx::PxDefaultPvdSocketTransportCreate( cmdline::PxDAddress.c_str(), cmdline::PvPort, 250 );
                bSerialize = true;
            }
        }

        // read the stream input block
        physx::pvdsdk::StreamInitialization init;
        init.serialize( eventUnpacker );

        // check the id/version is what we expect
        if ( init.mStreamId != physx::pvdsdk::StreamInitialization::getStreamId() )
        {
            spdlog::error( "stream ID invalid; got {}, expected {}", init.mStreamId, physx::pvdsdk::StreamInitialization::getStreamId() );
            exit( 1 );
        }
        if ( init.mStreamVersion != physx::pvdsdk::StreamInitialization::getStreamVersion() )
        {
            spdlog::error( "stream version invalid; got {}, expected {}", init.mStreamVersion, physx::pvdsdk::StreamInitialization::getStreamVersion() );
            exit( 1 );
        }

        // if there is an output mode chosen, connect and prepare to send events to it
        if ( bSerialize )
        {
            outboundTransport->connect();

            physx::pvdsdk::EventStreamifier<physx::PxPvdTransport> streamOut( outboundTransport->lock() );
            init.serialize( streamOut );
            outboundTransport->unlock();
        }

        // create the data dump log file next to the input file
        auto pxdLogFile = fs::path( cmdline::PxDInput ).replace_extension( ".stream.log" );
        eventBreaker.m_verboseLog = spdlog::basic_logger_mt( "stream_logger", pxdLogFile.string(), true );

        // setup any filtering required
        Op::FilterState opFilterState;
        if ( cmdline::TriMeshLimit >= 0 )
        {
            spdlog::info( "Limiting [PxTriangleMesh] instances to {}", cmdline::TriMeshLimit );
            opFilterState.m_instanceLimits["PxTriangleMesh"] = cmdline::TriMeshLimit;
        }

        uint32_t numEventsProcessed = 0;
        physx::pvdsdk::EventStreamifier<physx::PxPvdTransport> streamOut( outboundTransport->lock() );
        for ( ;; )
        {
            physx::pvdsdk::EventGroup eg;
            eg.serialize( eventUnpacker );

            // no events seems to signify the end of a stream
            if ( eg.mNumEvents == 0 )
                break;

            // for each event in the group (which is usually 1), decode and pass over to the event breaker logic
            for ( auto eventIndex = 0U; eventIndex < eg.mNumEvents; eventIndex++, numEventsProcessed++ )
            {
                Op::PvdEventType eventType;
                eventUnpacker.read( eventType );

                switch ( eventType )
                {
                    // if the event breaker returns true to indicate the event should be kept, and we're 
                    // actively serializing, write the type + data back out into the outbound transport
#define DECLARE_PVD_COMM_STREAM_EVENT(x)            case Op::PvdEventType::x: {             \
                    physx::pvdsdk::x _ev;                                                   \
                    _ev.serialize( eventUnpacker );                                         \
                    eventBreaker.logStartEvent( #x );                                       \
                    if ( eventBreaker.handleEvent( opFilterState, eg, _ev ) && bSerialize ) \
                    {                                                                       \
                        eg.serialize( streamOut );                                          \
                        const auto u8EventType = (uint8_t)eventType;                        \
                        streamOut.write( u8EventType );                                     \
                        _ev.serialize( streamOut );                                         \
                    }                                                                       \
                } break;

#define DECLARE_PVD_COMM_STREAM_EVENT_NO_COMMA(x)   DECLARE_PVD_COMM_STREAM_EVENT(x)
                    DECLARE_COMM_STREAM_EVENTS
#undef DECLARE_PVD_COMM_STREAM_EVENT_NO_COMMA
#undef DECLARE_PVD_COMM_STREAM_EVENT

                default:
                    spdlog::error( "Unhandled Event : {}", (int32_t)eventType );
                    __debugbreak();
                    break;
                }
            }

            if ( numEventsProcessed % 5000 == 0 )
            {
                spdlog::info( " ... {:>8} events", numEventsProcessed );
            }
        }
        
        spdlog::info( "- - - - - - - - - - - - - - - -" );
        eventBreaker.logSummary();

        outboundTransport->unlock();
        outboundTransport->flush();
    }
}
