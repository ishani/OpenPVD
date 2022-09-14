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

struct PxProperty
{
    std::string m_name;

};

struct PxClass
{
    std::string m_name;

};

struct PxDatabase
{
    using PxStringTable = ankerl::unordered_dense::map< uint32_t, std::string >;

    bool m_verboseDataLog = false;

    PxStringTable m_stringTable;                        // table of handles:string build as events are parsed
    const std::string m_invalidString = "<invalid>";    // return value for any unresolved string table lookups


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

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::StringHandleEvent& _event )
    {
        m_stringTable.emplace( _event.mHandle, _event.mString );

        if ( m_verboseDataLog )
        {
            elog.info( "[{}]           <= \"{}\"", _event.mHandle, _event.mString );
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::CreateClass& _event )
    {
        const auto nsName       = lookupNamespace( _event.mName );

        if ( m_verboseDataLog )
        {
            elog.info( "mName           : [{}.{}]", nsName.mNamespace, nsName.mName );
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::DeriveClass& _event )
    {
        const auto nsParent     = lookupNamespace( _event.mParent );
        const auto nsChild      = lookupNamespace( _event.mChild );

        if ( m_verboseDataLog )
        {
            elog.info( "mParent         : [{}.{}]", nsParent.mNamespace, nsParent.mName );
            elog.info( "mChild          : [{}.{}]", nsChild.mNamespace, nsChild.mName );
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::CreateProperty& _event )
    {
        const auto propClass    = lookupNamespace( _event.mClass );
        const auto propName     = lookupStringByHandle( _event.mName );
        const auto propSemantic = ( _event.mSemantic == 0 ) ? "n/a" : lookupStringByHandle( _event.mSemantic );
        const auto propDatatype = lookupNamespace( _event.mDatatypeName );

        if ( m_verboseDataLog )
        {
            elog.info( "mClass          : [{}.{}]", propClass.mNamespace, propClass.mName );
            elog.info( "mName           : {}", propName );
            elog.info( "mSemantic       : {}", propSemantic );
            elog.info( "mDatatypeName   : [{}.{}]", propDatatype.mNamespace, propDatatype.mName );
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::CreatePropertyMessage& _event )
    {
        const auto propClass = lookupNamespace( _event.mClass );
        const auto propMsg = lookupNamespace( _event.mMessageName );

        if ( m_verboseDataLog )
        {
            elog.info( "mClass          : [{}.{}]", propClass.mNamespace, propClass.mName );
            elog.info( "mMessageName    : [{}.{}]", propMsg.mNamespace, propMsg.mName );
            elog.info( "mMessageByteSz  : {}", _event.mMessageByteSize );
        }

        const auto messageCount = _event.mMessageEntries.size();
        for ( uint32_t idx = 0; idx < messageCount; ++idx )
        {
            const auto& dtype( const_cast<const physx::pvdsdk::StreamPropMessageArg&>(_event.mMessageEntries[idx]) );

            const auto msgDatatype = lookupNamespace( dtype.mDatatypeName );
            const auto msgName = lookupStringByHandle( dtype.mPropertyName );

            if ( m_verboseDataLog )
            {
                elog.info( " {:>2} -> [{}.{}] {}", idx, msgDatatype.mNamespace, msgDatatype.mName, msgName );
                elog.info( "       {} bytes, {} offset", idx, dtype.mByteSize, dtype.mMessageOffset );
            }
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::CreateInstance& _event )
    {
        const auto instClass = lookupNamespace( _event.mClass );

        if ( m_verboseDataLog )
        {
            elog.info( "mClass          : [{}.{}]", instClass.mNamespace, instClass.mName );
            elog.info( "mInstanceId     : {:#x}", _event.mInstanceId );
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::SetPropertyValue& _event )
    {
        const auto propName = lookupStringByHandle( _event.mPropertyName );
        const auto propTypeName = lookupNamespace( _event.mIncomingTypeName );

        if ( m_verboseDataLog )
        {
            elog.info( "mInstanceId     : {:#x}", _event.mInstanceId );
            elog.info( "mPropertyName   : {}", propName );
            elog.info( "mIncTypeName    : [{}.{}]", propTypeName.mNamespace, propTypeName.mName );
            elog.info( "mNumItems       : {}", _event.mNumItems );
            elog.info( "mData.size()    : {}", _event.mData.size() );
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::BeginSetPropertyValue& _event )
    {
        const auto propName = lookupStringByHandle( _event.mPropertyName );
        const auto propTypeName = lookupNamespace( _event.mIncomingTypeName );

        if ( m_verboseDataLog )
        {
            elog.info( "mInstanceId     : {:#x}", _event.mInstanceId );
            elog.info( "mPropertyName   : {}", propName );
            elog.info( "mIncTypeName    : [{}.{}]", propTypeName.mNamespace, propTypeName.mName );
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::AppendPropertyValueData& _event )
    {
        if ( m_verboseDataLog )
        {
            elog.info( "mData.size()    : {}", _event.mData.size() );
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::EndSetPropertyValue& _event )
    {

    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::SetPropertyMessage& _event )
    {
        const auto nsMessage = lookupNamespace( _event.mMessageName );

        if ( m_verboseDataLog )
        {
            elog.info( "mMsgName        : [{}.{}]", nsMessage.mNamespace, nsMessage.mName );
            elog.info( "mInstanceId     : {:#x}", _event.mInstanceId );
            elog.info( "mData.size()    : {}", _event.mData.size() );
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::BeginPropertyMessageGroup& _event )
    {
        const auto nsMessage = lookupNamespace( _event.mMsgName );

        if ( m_verboseDataLog )
        {
            elog.info( "mMsgName        : [{}.{}]", nsMessage.mNamespace, nsMessage.mName );
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::SendPropertyMessageFromGroup& _event )
    {
        if ( m_verboseDataLog )
        {
            elog.info( "mInstance       : {:#x}", _event.mInstance );
            elog.info( "mData.size()    : {}", _event.mData.size() );
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::EndPropertyMessageGroup& _event )
    {

    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::DestroyInstance& _event )
    {
        if ( m_verboseDataLog )
        {
            elog.info( "mInstanceId     : {:#x}", _event.mInstanceId );
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::PushBackObjectRef& _event )
    {
        const auto propName = lookupStringByHandle( _event.mProperty );

        if ( m_verboseDataLog )
        {
            elog.info( "mInstanceId     : {:#x}", _event.mInstanceId );
            elog.info( "mPropertyName   : {}", propName );
            elog.info( "mObjectRef      : {:#x}", _event.mObjectRef );
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::RemoveObjectRef& _event )
    {
        const auto propName = lookupStringByHandle( _event.mProperty );

        if ( m_verboseDataLog )
        {
            elog.info( "mInstanceId     : {:#x}", _event.mInstanceId );
            elog.info( "mPropertyName   : {}", propName );
            elog.info( "mObjectRef      : {:#x}", _event.mObjectRef );
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::BeginSection& _event )
    {
        const auto sectionName = lookupStringByHandle( _event.mName );

        if ( m_verboseDataLog )
        {
            elog.info( "mSectionId      : {}", _event.mSectionId );
            elog.info( "mName           : {}", sectionName );
            elog.info( "mTimestamp      : {}", _event.mTimestamp );
            elog.info( "{:-^120}", "\\/" );
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::EndSection& _event )
    {
        const auto sectionName = lookupStringByHandle( _event.mName );

        if ( m_verboseDataLog )
        {
            elog.info( "mSectionId      : {}", _event.mSectionId );
            elog.info( "mName           : {}", sectionName );
            elog.info( "mTimestamp      : {}", _event.mTimestamp );
            elog.info( "{:-^120}", "/\\" );
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::SetPickable& _event )
    {
        if ( m_verboseDataLog )
            elog.info( "** TODO **" );
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::SetColor& _event )
    {
        if ( m_verboseDataLog )
            elog.info( "** TODO **" );
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::SetIsTopLevel& _event )
    {
        if ( m_verboseDataLog )
        {
            elog.info( "mInstanceId     : {:#x}", _event.mInstanceId );
            elog.info( "mIsTopLevel     : {}", _event.mIsTopLevel );
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::SetCamera& _event )
    {
        if ( m_verboseDataLog )
        {
            elog.info( "mName           : {}", _event.mName );
            elog.info( "mPosition       : {{ {}, {}, {} }}", _event.mPosition.x, _event.mPosition.y, _event.mPosition.z );
            elog.info( "mUp             : {{ {}, {}, {} }}", _event.mUp.x, _event.mUp.y, _event.mUp.z );
            elog.info( "mTarget         : {{ {}, {}, {} }}", _event.mTarget.x, _event.mTarget.y, _event.mTarget.z );
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::AddProfileZone& _event )
    {
        if ( m_verboseDataLog )
        {
            elog.info( "mInstanceId     : {:#x}", _event.mInstanceId );
            elog.info( "mName           : {}", _event.mName );
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::AddProfileZoneEvent& _event )
    {
        if ( m_verboseDataLog )
        {
            elog.info( "mInstanceId     : {:#x}", _event.mInstanceId );
            elog.info( "mName           : {}", _event.mName );
            elog.info( "mEventId        : {}", _event.mEventId );
            elog.info( "mCompileTime    : {}", _event.mCompileTimeEnabled );
        }
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::StreamEndEvent& _event )
    {
        if ( m_verboseDataLog )
            elog.info( "** TODO **" );
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::ErrorMessage& _event )
    {
        if ( m_verboseDataLog )
            elog.info( "** TODO **" );
    }

    void handleEvent( spdlog::logger& elog, const physx::pvdsdk::EventGroup& _group,  const physx::pvdsdk::OriginShift& _event )
    {
        if ( m_verboseDataLog )
            elog.info( "** TODO **" );
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
    PxDb.m_verboseDataLog = true;


    uint64_t lastTimestamp = 0;

    for ( ;; )
    {
        EventGroup eg;
        eg.serialize( eventUnpacker );

        const uint64_t relativeTimestamp = eg.mTimestamp - lastTimestamp;
        lastTimestamp = eg.mTimestamp;

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
                streamLogger->info("\n{:>16} | "#x, relativeTimestamp);                 \
                streamLogger->set_pattern( "                      %v" );                \
                PxDb.handleEvent(*streamLogger, eg, _ev);                               \
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