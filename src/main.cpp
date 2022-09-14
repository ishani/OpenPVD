//
//   ____                ___ _   _____ 
//  / __ \___  ___ ___  / _ \ | / / _ \
// / /_/ / _ \/ -_) _ \/ ___/ |/ / // /
// \____/ .__/\__/_//_/_/   |___/____/ 
//     /_/  https://github.com/ishani/OpenPVD
// 
// an experimental PhysX visual debugger
//

#include "pch.h"

#include "PsFileBuffer.h"

#include "EventUnpacker.h"


struct ResolvedStreamNamespacedName
{
    std::string mNamespace;
    std::string mName;
};

struct PxDatabase
{
    const std::string m_invalidString = "<invalid>";
    std::unordered_map< uint32_t, std::string > m_stringTable;


    inline const std::string& lookupStringByHandle( const uint32_t handle ) const
    {
        const auto it = m_stringTable.find( handle );
        if ( it == m_stringTable.end() )
        {
            spdlog::error( "invalid string handle found : {}", handle );
            return m_invalidString;
        }
        return it->second;
    }

    // this is here to avoid accidentally trying to resolve a StringHandle as a namespace; StreamNamespacedName
    // will silently get constructed and null out one of the entries otherwise
    inline ResolvedStreamNamespacedName lookupNamespace( const physx::pvdsdk::StringHandle& ) const;

    inline ResolvedStreamNamespacedName lookupNamespace( const physx::pvdsdk::StreamNamespacedName& snn ) const
    {
        return {
            lookupStringByHandle( snn.mNamespace ),
            lookupStringByHandle( snn.mName )
        };
    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::StringHandleEvent& _event )
    {
        m_stringTable.emplace( _event.mHandle, _event.mString );
        flog.info( "[{}] <= \"{}\"", _event.mHandle, _event.mString );
    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::CreateClass& _event )
    {
        const auto nsName       = lookupNamespace( _event.mName );
        flog.info( "mName   : [{}.{}]", nsName.mNamespace, nsName.mName );
    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::DeriveClass& _event )
    {
        const auto nsParent     = lookupNamespace( _event.mParent );
        const auto nsChild      = lookupNamespace( _event.mChild );
        flog.info( "mParent : [{}.{}]", nsParent.mNamespace, nsParent.mName );
        flog.info( "mChild  : [{}.{}]", nsChild.mNamespace, nsChild.mName );
    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::CreateProperty& _event )
    {
        const auto propClass    = lookupNamespace( _event.mClass );
        const auto propName     = lookupStringByHandle( _event.mName );
        const auto propSemantic = lookupStringByHandle( _event.mSemantic );
        const auto propDatatype = lookupNamespace( _event.mDatatypeName );

        flog.info( "mClass  : [{}.{}]", propClass.mNamespace, propClass.mName );
        flog.info( "mName   : {}", propName );

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::CreatePropertyMessage& _event )
    {
        const auto propClass = lookupNamespace( _event.mClass );
        const auto propMsg = lookupNamespace( _event.mMessageName );

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::CreateInstance& _event )
    {
        const auto instClass = lookupNamespace( _event.mClass );

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::SetPropertyValue& _event )
    {
        const auto propName = lookupStringByHandle( _event.mPropertyName );
        const auto propTypeName = lookupNamespace( _event.mIncomingTypeName );

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::BeginSetPropertyValue& _event )
    {

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::AppendPropertyValueData& _event )
    {

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::EndSetPropertyValue& _event )
    {

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::SetPropertyMessage& _event )
    {
        const auto nsMessage = lookupNamespace( _event.mMessageName );
        flog.info( "mMsgName  : [{}.{}]", nsMessage.mNamespace, nsMessage.mName );

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::BeginPropertyMessageGroup& _event )
    {

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::SendPropertyMessageFromGroup& _event )
    {

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::EndPropertyMessageGroup& _event )
    {

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::DestroyInstance& _event )
    {

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::PushBackObjectRef& _event )
    {

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::RemoveObjectRef& _event )
    {

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::BeginSection& _event )
    {

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::EndSection& _event )
    {

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::SetPickable& _event )
    {

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::SetColor& _event )
    {

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::SetIsTopLevel& _event )
    {

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::SetCamera& _event )
    {

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::AddProfileZone& _event )
    {

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::AddProfileZoneEvent& _event )
    {

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::StreamEndEvent& _event )
    {

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::ErrorMessage& _event )
    {

    }

    void handleEvent( spdlog::logger& flog, const physx::pvdsdk::OriginShift& _event )
    {

    }
};

// ---------------------------------------------------------------------------------------------------------------------
namespace cmdline
{
    static std::string PxDFilename = "testdata/basic.pxd2";

    int parse( int argc, char** argv )
    {
        CLI::App app{ "OpenPVD" };

        app.add_option( "-p,--pxd", cmdline::PxDFilename, "path to a PXD2 capture file to parse" );

        CLI11_PARSE( app, argc, argv );

        return 0;
    }
}

// ---------------------------------------------------------------------------------------------------------------------
int main( int argc, char** argv )
{
    spdlog::set_pattern( "[%^%L%$] %v" );

    if ( int cmdr = cmdline::parse( argc, argv ) )
        return cmdr;

    if ( !fs::exists( cmdline::PxDFilename ) )
    {
        spdlog::error( "Cannot find PXD file [{}]", cmdline::PxDFilename );
        exit( 1 );
    }


    using namespace physx::pvdsdk;

    spdlog::info( "Loading : {}", cmdline::PxDFilename );
    auto PxDFile = physx::PsFileBuffer( cmdline::PxDFilename.c_str(), physx::general_PxIOStream2::PxFileBuf::OPEN_READ_ONLY );

    auto eventUnpacker = decoder::EventUnpacker< physx::PsFileBuffer >( PxDFile );


    PxDatabase PxDb;

    StreamInitialization init;
    init.serialize( eventUnpacker );

    auto pxdLogFile = fs::path( cmdline::PxDFilename ).replace_extension( ".stream.log" );
    auto streamLogger = spdlog::basic_logger_mt( "stream_logger", pxdLogFile.string(), true );

    for ( ;; )
    {
        EventGroup eg;
        eg.serialize( eventUnpacker );

        if ( eg.mNumEvents == 0 )
            break;

        const auto eventReadPoint = PxDFile.tellRead();
        for ( auto eventIndex = 0U; eventIndex < eg.mNumEvents; eventIndex++ )
        {
            decoder::PvdEventType eventType;
            eventUnpacker.read( eventType );

            switch ( eventType )
            {
#define DECLARE_PVD_COMM_STREAM_EVENT(x)            case decoder::PvdEventType::x: {    \
                x _ev;                                                                  \
                _ev.serialize( eventUnpacker );                                         \
                streamLogger->set_pattern( "%v" );                                      \
                streamLogger->info("\n"#x);                                             \
                streamLogger->set_pattern( "    %v" );                                  \
                PxDb.handleEvent(*streamLogger, _ev);                                   \
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

    }

    return 0;
}