//
//   ____                ___ _   _____ 
//  / __ \___  ___ ___  / _ \ | / / _ \
// / /_/ / _ \/ -_) _ \/ ___/ |/ / // /
// \____/ .__/\__/_//_/_/   |___/____/ 
//     /_/  https://github.com/ishani/OpenPVD
// 
// capture tool accepts a standard PVD connection from a client game and streams the data to a PXD2 file on disk
//

#include "pch.h"
#include "common/OpFoundation.h"
#include "common/OpEventUnpacker.h"

#include "PsFileBuffer.h"
#include "PsSocket.h"

// ---------------------------------------------------------------------------------------------------------------------
namespace cmdline
{
    static std::string PxDOutput    = "captured.pxd2";
    static uint16_t PvPort          = 5425;
    static uint32_t BufferSizeKb    = 768;              // need something large enough to read and process the largest single data packet (which can be big for trimeshes etc)

    int parse( int argc, char** argv )
    {
        CLI::App app{ "opvd-capture" };

        app.add_option( "-o,--out",     cmdline::PxDOutput,     "filename to write captured data to" );
        app.add_option( "-p,--port",    PvPort,                 "port to listen on" );
        app.add_option( "-b,--buf",     BufferSizeKb,           "transmission buffer, in KB" );

        CLI11_PARSE( app, argc, argv );

        return 0;
    }
}

// ---------------------------------------------------------------------------------------------------------------------
enum class ProcessingState
{
    WaitingOnInit,
    WalkingEventGroups
};

// ---------------------------------------------------------------------------------------------------------------------
// trivial buffer walker for use with PvdEventSerializer
struct MemoryReader
{
    MemoryReader() = delete;
    MemoryReader( const uint8_t* buffer, const uint32_t bufferLength )
        : m_buffer( buffer )
        , m_bufferLength( bufferLength )
        , m_bufferRead( 0 )
    {}

    uint32_t read( void* buffer, uint32_t size )
    {
        if ( m_bufferRead + size > m_bufferLength )
            return 0;

        memcpy( buffer, &m_buffer[m_bufferRead], size );
        m_bufferRead += size;

        return size;
    }

    constexpr bool seekForward( uint32_t size )
    {
        if ( m_bufferRead + size > m_bufferLength )
            return false;

        m_bufferRead += size;
        return true;
    }

    constexpr uint32_t bytesRemaining() const
    {
        return m_bufferLength - m_bufferRead;
    }

    const uint8_t* m_buffer;
    const uint32_t m_bufferLength;
          uint32_t m_bufferRead;
};

// ---------------------------------------------------------------------------------------------------------------------
int main( int argc, char** argv )
{
    spdlog::set_pattern( "[%^%L%$] %v" );

    if ( int cmdr = cmdline::parse( argc, argv ) )
        return cmdr;

    Op::Foundation opFoundation;
    {
        spdlog::info( "Waiting for PVD connection from client ..." );

        physx::shdfnd::Socket mSocket;
        mSocket.listen( cmdline::PvPort );
        mSocket.accept( true );

        spdlog::info( "Connection established, streaming data to [{}] ...", cmdline::PxDOutput );

        auto PxDFileOut = physx::PsFileBuffer( cmdline::PxDOutput.c_str(), physx::general_PxIOStream2::PxFileBuf::OPEN_WRITE_ONLY );

        const uint32_t recvBufferSize = cmdline::BufferSizeKb * 1024;
        uint8_t* recvBuffer = (uint8_t*)_aligned_malloc( recvBufferSize, 16 );
        uint8_t* offloadBuffer = (uint8_t*)_aligned_malloc( recvBufferSize, 16 );

        uint32_t recvBufferOffset = 0;
        uint32_t eventGroupsRead = 0;
        uint32_t eventLargestData = 0;
        ProcessingState processingState = ProcessingState::WaitingOnInit;

        bool bStreamingData = true;
        while ( bStreamingData )
        {
            using namespace physx::pvdsdk;

            bStreamingData = mSocket.isConnected();
            if ( !bStreamingData )
                break;

            const uint32_t bytesRead = mSocket.read( recvBuffer + recvBufferOffset, recvBufferSize - recvBufferOffset ) + recvBufferOffset;
            recvBufferOffset = 0;

            if ( bytesRead > 0 )
            {
                MemoryReader recvReader( recvBuffer, bytesRead );
                auto eventUnpacker = Op::EventUnpacker< MemoryReader >( recvReader );

                // initial state grabs the initialisation block, then onto the events
                if ( processingState == ProcessingState::WaitingOnInit )
                {
                    if ( recvReader.bytesRemaining() > sizeof( StreamInitialization ) )
                    {
                        StreamInitialization init;
                        init.serialize( eventUnpacker );

                        if ( init.mStreamId != StreamInitialization::getStreamId() )
                        {
                            spdlog::error( "stream ID invalid; got {}, expected {}", init.mStreamId, StreamInitialization::getStreamId() );
                            exit( 1 );
                        }
                        if ( init.mStreamVersion != StreamInitialization::getStreamVersion() )
                        {
                            spdlog::error( "stream version invalid; got {}, expected {}", init.mStreamVersion, StreamInitialization::getStreamVersion() );
                            exit( 1 );
                        }

                        spdlog::info( "Stream initialised successfully" );
                        processingState = ProcessingState::WalkingEventGroups;
                    }
                }
                // event processing means we're iterating the stream of event groups
                if ( processingState == ProcessingState::WalkingEventGroups )
                {
                    // loop while there is enough data to start reading the event group
                    while ( recvReader.bytesRemaining() > sizeof( EventGroup ) )
                    {
                        // save current read point in case the event group cannot be fully read, we'll rewind to this point
                        const uint32_t preGroupBytesRead = recvReader.m_bufferRead;
                        EventGroup eg;
                        eg.serialize( eventUnpacker );

                        // signals end of the stream
                        if ( eg.mNumEvents == 0 )
                        {
                            bStreamingData = false;
                            break;
                        }
                        eventLargestData = std::max( eventLargestData, eg.mDataSize );

                        // is there enough data left to fully read the group?
                        if ( recvReader.bytesRemaining() >= eg.mDataSize )
                        {
                            // fetch the type of the first event (there are usually only ever 1 in a group)
                            uint8_t u8EventType;
                            recvReader.read( &u8EventType, 1 );
                            const Op::PvdEventType eventType = (Op::PvdEventType)u8EventType;

                            if ( !Op::eventTypeValid( eventType ) )
                            {
                                spdlog::error( "unknown or invalid event encountered" );
                                exit( 1 );
                            }

                            recvReader.seekForward( eg.mDataSize - 1 );

                            eventGroupsRead++;
                            if ( eventGroupsRead % 1024 == 0 )
                            {
                                spdlog::info( "events : {:>16} | largest data : {}", eventGroupsRead, eventLargestData );
                            }
                        }
                        else
                        {
                            // rewind event group parsing, we don't have enough bytes to read it completely
                            recvReader.m_bufferRead = preGroupBytesRead;
                            break;
                        }
                    }
                }

                // any unprocessed bytes will be appended to in the next loop around
                recvBufferOffset = recvReader.bytesRemaining();

                // write out what we got to the PXD file
                const auto bytesToWrite = bytesRead - recvBufferOffset;
                if ( bytesToWrite > 0 )
                    PxDFileOut.write( recvBuffer, bytesToWrite );

                // move the remaining bytes to the front of the buffer
                memcpy( offloadBuffer, &recvBuffer[bytesToWrite], recvBufferOffset );
                memcpy( recvBuffer, offloadBuffer, recvBufferOffset );
            }
        }
        _aligned_free( offloadBuffer );
        _aligned_free( recvBuffer );

        spdlog::info( "Closing ..." );
    }
}
