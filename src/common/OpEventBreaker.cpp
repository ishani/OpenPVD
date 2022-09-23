//
//   ____                ___ _   _____ 
//  / __ \___  ___ ___  / _ \ | / / _ \
// / /_/ / _ \/ -_) _ \/ ___/ |/ / // /
// \____/ .__/\__/_//_/_/   |___/____/ 
//     /_/  https://github.com/ishani/OpenPVD
// 
// 

#include "pch.h"
#include "common/OpEventBreaker.h"

namespace Op
{
    // ---------------------------------------------------------------------------------------------------------------------
    // https://stackoverflow.com/questions/1094841/reusable-library-to-get-human-readable-version-of-file-size
    //
    inline std::string humaniseByteSize( const uint64_t bytes )
    {
        if ( bytes == 0 )
            return "0 bytes";
        else
            if ( bytes == 1 )
                return "1 byte";
            else
            {
                const auto exponent = (int32_t)(std::log( bytes ) / std::log( 1024 ));
                const auto quotient = double( bytes ) / std::pow( 1024, exponent );

                // done via a switch as fmt::format needs a consteval format arg
                switch ( exponent )
                {
                case 0: return fmt::format( "{:.0f} bytes", quotient );
                case 1: return fmt::format( "{:.0f} kB", quotient );
                case 2: return fmt::format( "{:.1f} MB", quotient );
                case 3: return fmt::format( "{:.2f} GB", quotient );
                case 4: return fmt::format( "{:.2f} TB", quotient );
                case 5: return fmt::format( "{:.2f} PB", quotient );
                default:
                    return "unknown";
                    break;
                }
            }
    }

    void EventBreaker::logSummary()
    {
        spdlog::info( "{:>32} = {} ", "number of frames", m_currentFrame );

        spdlog::info( "{:>32}", "memory usage by instance type" );
        for ( const auto& payload : m_instanceDataSizes )
        {
            spdlog::info( "{:>32} = {:>8}x = {} ", payload.first, m_instanceCount[payload.first], humaniseByteSize(payload.second) );
        }
    }

    void EventBreaker::logStartEvent( const char* eventTitle )
    {
        if ( m_verboseLog != nullptr )
        {
            m_verboseLog->set_pattern( "%v" );
            m_verboseLog->info( "\n{}", eventTitle );
            m_verboseLog->set_pattern( "      %v" );
        }
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::StringHandleEvent& _event )
    {
        addFromStringHandleEvent( _event );

        if ( m_verboseLog != nullptr )
        {
            m_verboseLog->info( "[{}]           <= \"{}\"", _event.mHandle, _event.mString );
        }
        return true;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::CreateClass& _event )
    {
        const auto nsName = lookupNamespace( _event.mName );

        if ( m_verboseLog != nullptr )
        {
            m_verboseLog->info( "mName           : [{}.{}]", nsName.mNamespace, nsName.mName );
        }
        return true;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::DeriveClass& _event )
    {
        const auto nsParent = lookupNamespace( _event.mParent );
        const auto nsChild = lookupNamespace( _event.mChild );

        if ( m_verboseLog != nullptr )
        {
            m_verboseLog->info( "mParent         : [{}.{}]", nsParent.mNamespace, nsParent.mName );
            m_verboseLog->info( "mChild          : [{}.{}]", nsChild.mNamespace, nsChild.mName );
        }
        return true;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::CreateProperty& _event )
    {
        const auto propClass = lookupNamespace( _event.mClass );
        const auto propName = lookupStringByHandle( _event.mName );
        const auto propSemantic = (_event.mSemantic == 0) ? "n/a" : lookupStringByHandle( _event.mSemantic );
        const auto propDatatype = lookupNamespace( _event.mDatatypeName );

        if ( m_verboseLog != nullptr )
        {
            m_verboseLog->info( "mClass          : [{}.{}]", propClass.mNamespace, propClass.mName );
            m_verboseLog->info( "mName           : {}", propName );
            m_verboseLog->info( "mSemantic       : {}", propSemantic );
            m_verboseLog->info( "mDatatypeName   : [{}.{}]", propDatatype.mNamespace, propDatatype.mName );
        }
        return true;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::CreatePropertyMessage& _event )
    {
        const auto propClass = lookupNamespace( _event.mClass );
        const auto propMsg = lookupNamespace( _event.mMessageName );

        if ( m_verboseLog != nullptr )
        {
            m_verboseLog->info( "mClass          : [{}.{}]", propClass.mNamespace, propClass.mName );
            m_verboseLog->info( "mMessageName    : [{}.{}]", propMsg.mNamespace, propMsg.mName );
            m_verboseLog->info( "mMessageByteSz  : {}", _event.mMessageByteSize );
        }

        const auto messageCount = _event.mMessageEntries.size();
        for ( uint32_t idx = 0; idx < messageCount; ++idx )
        {
            const auto& dtype( const_cast<const physx::pvdsdk::StreamPropMessageArg&>(_event.mMessageEntries[idx]) );

            const auto msgDatatype = lookupNamespace( dtype.mDatatypeName );
            const auto msgName = lookupStringByHandle( dtype.mPropertyName );

            if ( m_verboseLog != nullptr )
            {
                m_verboseLog->info( " {:>2} -> [{}.{}] {}", idx, msgDatatype.mNamespace, msgDatatype.mName, msgName );
                m_verboseLog->info( "       {} bytes, {} offset", idx, dtype.mByteSize, dtype.mMessageOffset );
            }
        }
        return true;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::CreateInstance& _event )
    {
        const auto instClass = lookupNamespace( _event.mClass );

        bool shouldFilter = false;

        const auto it = _filtering.m_instanceLimits.find( instClass.mName );
        if ( it != _filtering.m_instanceLimits.end() )
        {
            shouldFilter = (m_instanceCount[instClass.mName] >= it->second);
        }

        if ( shouldFilter )
        {
            _filtering.addInstance( _event.mInstanceId );
            if ( m_verboseLog != nullptr )
                m_verboseLog->info( "== filtered ==" );
        }
        else
        {
            m_instanceTypeMap.emplace( _event.mInstanceId, instClass.mName );
            m_instanceCount[instClass.mName] ++;
        }


        if ( m_verboseLog != nullptr )
        {
            m_verboseLog->info( "mClass          : [{}.{}]", instClass.mNamespace, instClass.mName );
            m_verboseLog->info( "mInstanceId     : {:#x}", _event.mInstanceId );
        }

        return !_filtering.isInstanceFiltered( _event.mInstanceId );
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::SetPropertyValue& _event )
    {
        const auto instanceType = getInstanceTypeFromID( _event.mInstanceId );
        const auto propName = lookupStringByHandle( _event.mPropertyName );
        const auto propTypeName = lookupNamespace( _event.mIncomingTypeName );
        const bool isFiltered = _filtering.isInstanceFiltered( _event.mInstanceId );

        if ( !isFiltered )
        {
            m_instanceDataSizes[instanceType] += _event.mData.size();
        }

        if ( m_verboseLog != nullptr )
        {
            if ( isFiltered )
                m_verboseLog->info( "== filtered ==" );

            m_verboseLog->info( "mInstanceId     : {:#x} ({})", _event.mInstanceId, instanceType );
            m_verboseLog->info( "mPropertyName   : {}", propName );

            // special case to pull a u32 out and just print the value inline
            if ( propTypeName.mName == "PvdU32" && _event.mNumItems == 1 )
            {
                auto u32 = (const uint32_t*)_event.mData.begin();
                m_verboseLog->info( "                = {}", *u32 );
            }
            else
            {
                m_verboseLog->info( "mIncTypeName    : [{}.{}]", propTypeName.mNamespace, propTypeName.mName );
                m_verboseLog->info( "mNumItems       : {}", _event.mNumItems );
                m_verboseLog->info( "mData.size()    : {}", _event.mData.size() );
            }
        }

        return !isFiltered;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::BeginSetPropertyValue& _event )
    {
        const auto instanceType = getInstanceTypeFromID( _event.mInstanceId );
        const auto propName = lookupStringByHandle( _event.mPropertyName );
        const auto propTypeName = lookupNamespace( _event.mIncomingTypeName );
        const bool isFiltered = _filtering.isInstanceFiltered( _event.mInstanceId );

        if ( m_verboseLog != nullptr )
        {
            if ( isFiltered )
                m_verboseLog->info( "== filtered ==" );

            m_verboseLog->info( "mInstanceId     : {:#x} ({})", _event.mInstanceId, instanceType );
            m_verboseLog->info( "mPropertyName   : {}", propName );
            m_verboseLog->info( "mIncTypeName    : [{}.{}]", propTypeName.mNamespace, propTypeName.mName );
        }

        m_multiSetPropertyValueActive = !isFiltered;
        m_multiSetPropertyValueInstanceID = _event.mInstanceId;
        return m_multiSetPropertyValueActive;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::AppendPropertyValueData& _event )
    {
        const auto instanceType = getInstanceTypeFromID( m_multiSetPropertyValueInstanceID );

        if ( m_verboseLog != nullptr )
        {
            m_verboseLog->info( "mInstanceId     : {:#x} ({})", m_multiSetPropertyValueInstanceID, instanceType );
            m_verboseLog->info( "mData.size()    : {}", _event.mData.size() );
        }

        m_instanceDataSizes[instanceType] += _event.mData.size();

        return m_multiSetPropertyValueActive;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::EndSetPropertyValue& _event )
    {
        m_multiSetPropertyValueInstanceID = 0;
        return m_multiSetPropertyValueActive;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::SetPropertyMessage& _event )
    {
        const auto instanceType = getInstanceTypeFromID( _event.mInstanceId );
        const auto nsMessage = lookupNamespace( _event.mMessageName );
        const bool isFiltered = _filtering.isInstanceFiltered( _event.mInstanceId );

        if ( m_verboseLog != nullptr )
        {
            if ( isFiltered )
                m_verboseLog->info( "== filtered ==" );

            m_verboseLog->info( "mInstanceId     : {:#x} ({})", _event.mInstanceId, instanceType );
            m_verboseLog->info( "mMsgName        : [{}.{}]", nsMessage.mNamespace, nsMessage.mName );
            m_verboseLog->info( "mData.size()    : {}", _event.mData.size() );
        }

        return !isFiltered;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::BeginPropertyMessageGroup& _event )
    {
        const auto nsMessage = lookupNamespace( _event.mMsgName );

        if ( m_verboseLog != nullptr )
        {
            m_verboseLog->info( "mMsgName        : [{}.{}]", nsMessage.mNamespace, nsMessage.mName );
        }
        return true;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::SendPropertyMessageFromGroup& _event )
    {
        if ( m_verboseLog != nullptr )
        {
            m_verboseLog->info( "mInstance       : {:#x}", _event.mInstance );
            m_verboseLog->info( "mData.size()    : {}", _event.mData.size() );
        }
        return true;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::EndPropertyMessageGroup& _event )
    {
        return true;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::DestroyInstance& _event )
    {
        const auto instanceType = getInstanceTypeFromID( _event.mInstanceId );
        const bool isFiltered = _filtering.isInstanceFiltered( _event.mInstanceId );

        if ( m_verboseLog != nullptr )
        {
            if ( isFiltered )
                m_verboseLog->info( "== filtered ==" );

            m_verboseLog->info( "mInstanceId     : {:#x} ({})", _event.mInstanceId, instanceType );
        }

        if ( isFiltered )
            _filtering.removeInstance( _event.mInstanceId );

        m_instanceTypeMap.erase( _event.mInstanceId );

        return !isFiltered;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::PushBackObjectRef& _event )
    {
        const auto instanceType         = getInstanceTypeFromID( _event.mInstanceId );
        const auto instanceTypeObject   = getInstanceTypeFromID( _event.mObjectRef );
        const auto propName             = lookupStringByHandle( _event.mProperty );
        const bool isFiltered           = _filtering.isInstanceFiltered( _event.mObjectRef );

        if ( m_verboseLog != nullptr )
        {
            if ( isFiltered )
                m_verboseLog->info( "== filtered ==" );

            m_verboseLog->info( "mInstanceId     : {:#x} ({})", _event.mInstanceId, instanceType );
            m_verboseLog->info( "mPropertyName   : {}", propName );
            m_verboseLog->info( "mObjectRef      : {:#x} ({})", _event.mObjectRef, instanceTypeObject );
        }

        return !isFiltered;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::RemoveObjectRef& _event )
    {
        const auto instanceType         = getInstanceTypeFromID( _event.mInstanceId );
        const auto propName             = lookupStringByHandle( _event.mProperty );
        const bool isFiltered           = _filtering.isInstanceFiltered( _event.mObjectRef );

        if ( m_verboseLog != nullptr )
        {
            if ( isFiltered )
                m_verboseLog->info( "== filtered ==" );

            m_verboseLog->info( "mInstanceId     : {:#x} ({})", _event.mInstanceId, instanceType );
            m_verboseLog->info( "mPropertyName   : {}", propName );
            m_verboseLog->info( "mObjectRef      : {:#x}", _event.mObjectRef );
        }

        return !isFiltered;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::BeginSection& _event )
    {
        const auto sectionName = lookupStringByHandle( _event.mName );

        if ( m_verboseLog != nullptr )
        {
            m_verboseLog->info( "mSectionId      : {}", _event.mSectionId );
            m_verboseLog->info( "mName           : {}", sectionName );
            m_verboseLog->info( "mTimestamp      : {}", _event.mTimestamp );
            m_verboseLog->info( "{:-^120}", "\\/" );
        }
        if ( sectionName == "frame" )
        {
            m_currentFrame++;
            if ( m_verboseLog != nullptr )
            {
                m_verboseLog->info( "{: ^120}", m_currentFrame );
            }
        }

        return true;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::EndSection& _event )
    {
        const auto sectionName = lookupStringByHandle( _event.mName );

        if ( m_verboseLog != nullptr )
        {
            m_verboseLog->info( "mSectionId      : {}", _event.mSectionId );
            m_verboseLog->info( "mName           : {}", sectionName );
            m_verboseLog->info( "mTimestamp      : {}", _event.mTimestamp );
            m_verboseLog->info( "{:-^120}", "/\\" );
        }

        return true;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::SetPickable& _event )
    {
        if ( m_verboseLog != nullptr )
            m_verboseLog->info( "** TODO **" );

        return true;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::SetColor& _event )
    {
        if ( m_verboseLog != nullptr )
            m_verboseLog->info( "** TODO **" );

        return true;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::SetIsTopLevel& _event )
    {
        if ( m_verboseLog != nullptr )
        {
            m_verboseLog->info( "mInstanceId     : {:#x}", _event.mInstanceId );
            m_verboseLog->info( "mIsTopLevel     : {}", _event.mIsTopLevel );
        }
        return true;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::SetCamera& _event )
    {
        if ( m_verboseLog != nullptr )
        {
            m_verboseLog->info( "mName           : {}", _event.mName );
            m_verboseLog->info( "mPosition       : {{ {}, {}, {} }}", _event.mPosition.x, _event.mPosition.y, _event.mPosition.z );
            m_verboseLog->info( "mUp             : {{ {}, {}, {} }}", _event.mUp.x, _event.mUp.y, _event.mUp.z );
            m_verboseLog->info( "mTarget         : {{ {}, {}, {} }}", _event.mTarget.x, _event.mTarget.y, _event.mTarget.z );
        }
        return true;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::AddProfileZone& _event )
    {
        const bool isFiltered = _filtering.isInstanceFiltered( _event.mInstanceId );

        if ( m_verboseLog != nullptr )
        {
            if ( isFiltered )
                m_verboseLog->info( "== filtered ==" );

            m_verboseLog->info( "mInstanceId     : {:#x}", _event.mInstanceId );
            m_verboseLog->info( "mName           : {}", _event.mName );
        }
        return !isFiltered;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::AddProfileZoneEvent& _event )
    {
        const bool isFiltered = _filtering.isInstanceFiltered( _event.mInstanceId );

        if ( m_verboseLog != nullptr )
        {
            if ( isFiltered )
                m_verboseLog->info( "== filtered ==" );

            m_verboseLog->info( "mInstanceId     : {:#x}", _event.mInstanceId );
            m_verboseLog->info( "mName           : {}", _event.mName );
            m_verboseLog->info( "mEventId        : {}", _event.mEventId );
            m_verboseLog->info( "mCompileTime    : {}", _event.mCompileTimeEnabled );
        }
        return !isFiltered;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::StreamEndEvent& _event )
    {
        if ( m_verboseLog != nullptr )
            m_verboseLog->info( "** TODO **" );

        return true;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::ErrorMessage& _event )
    {
        if ( m_verboseLog != nullptr )
            m_verboseLog->info( "** TODO **" );

        return true;
    }

    bool EventBreaker::handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::OriginShift& _event )
    {
        if ( m_verboseLog != nullptr )
            m_verboseLog->info( "** TODO **" );

        return true;
    }
} // namespace Op