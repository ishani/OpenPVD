//
//   ____                ___ _   _____ 
//  / __ \___  ___ ___  / _ \ | / / _ \
// / /_/ / _ \/ -_) _ \/ ___/ |/ / // /
// \____/ .__/\__/_//_/_/   |___/____/ 
//     /_/  https://github.com/ishani/OpenPVD
// 
// PVD types have serialize() functions that are designed to write their data into an outbound data stream;
// although they are largely designed to be write-only, we abuse this system with our own custom serialiser type
// that can read instead of write, semi-leakily allocate dynamic data and ultimately make use of all the original
// untouched PVD types decoded straight out of the stream
//

#pragma once

#include <vector>

#include "pvd/PxPvd.h"

#include "PxPvdObjectModelBaseTypes.h"
#include "PxPvdCommStreamEvents.h"
#include "PxPvdCommStreamTypes.h"

namespace pvd = physx::pvdsdk;

namespace Op
{
    // take a copy of the event type as an enum-class
    enum class PvdEventType
    {
        Unknown = 0,
#define DECLARE_PVD_COMM_STREAM_EVENT(x) x,
#define DECLARE_PVD_COMM_STREAM_EVENT_NO_COMMA(x) x
        DECLARE_COMM_STREAM_EVENTS
#undef DECLARE_PVD_COMM_STREAM_EVENT_NO_COMMA
#undef DECLARE_PVD_COMM_STREAM_EVENT
        , Last
    };

    inline const char* eventTypeToString( const PvdEventType evt )
    {
        switch ( evt )
        {
#define DECLARE_PVD_COMM_STREAM_EVENT(x) case PvdEventType::x: return #x;
#define DECLARE_PVD_COMM_STREAM_EVENT_NO_COMMA(x) DECLARE_PVD_COMM_STREAM_EVENT(x)
            DECLARE_COMM_STREAM_EVENTS
#undef DECLARE_PVD_COMM_STREAM_EVENT_NO_COMMA
#undef DECLARE_PVD_COMM_STREAM_EVENT
        }
        return "<unknown>";
    }

    // used to check if a resolved event is actually known 
    inline bool eventTypeValid( const PvdEventType evt )
    {
        switch ( evt )
        {
#define DECLARE_PVD_COMM_STREAM_EVENT(x) case PvdEventType::x:
#define DECLARE_PVD_COMM_STREAM_EVENT_NO_COMMA(x) DECLARE_PVD_COMM_STREAM_EVENT(x)
            DECLARE_COMM_STREAM_EVENTS
#undef DECLARE_PVD_COMM_STREAM_EVENT_NO_COMMA
#undef DECLARE_PVD_COMM_STREAM_EVENT
            return true;

        default:
            return false;
        }
    }

    template <typename TStreamType>
    struct EventUnpacker : public pvd::PvdEventSerializer
    {
        std::size_t             m_allocatedMemorySize = 0;
        std::vector< void* >    m_allocations;

        template< typename _mType >
        inline _mType* allocate( std::size_t quantity )
        {
            m_allocatedMemorySize += sizeof( _mType ) * quantity;
            void* newAlloc = _aligned_malloc( sizeof( _mType ) * quantity, 16 );
            m_allocations.push_back( newAlloc );

            return (_mType*)newAlloc;
        }

        TStreamType& mBuffer;
        EventUnpacker( TStreamType& buf ) : mBuffer( buf )
        {
        }

        // TODO deallocate from m_allocations

        template <typename TDataType>
        void read( TDataType& type )
        {
            mBuffer.read( reinterpret_cast<uint8_t*>(&type), sizeof( TDataType ) );
        }
        template <typename TDataType>
        void read( TDataType* type, uint32_t count )
        {
            mBuffer.read( reinterpret_cast<uint8_t*>(type), count * sizeof( TDataType ) );
        }

        void read( PvdEventType& val )
        {
            uint8_t detyped = static_cast<uint8_t>(val);
            read( detyped );
            val = static_cast<PvdEventType>(detyped);
        }

        void readRef( pvd::DataRef<const uint8_t>& data )
        {
            uint32_t amount;
            read( amount );

            if ( amount == 0 )
            {
                data = pvd::DataRef<const uint8_t>();
                return;
            }

            uint8_t* dataIn = allocate<uint8_t>( amount );
            read< uint8_t >( dataIn, amount );

            data = pvd::DataRef<const uint8_t>( dataIn, amount );
        }

        void readRef( pvd::DataRef<pvd::StringHandle>& data )
        {
            uint32_t amount;
            read( amount );

            if ( amount == 0 )
            {
                data = pvd::DataRef<pvd::StringHandle>();
                return;
            }

            pvd::StringHandle* dataIn = allocate<pvd::StringHandle>( amount );
            read< pvd::StringHandle >( dataIn, amount );

            data = pvd::DataRef<pvd::StringHandle>( dataIn, amount );
        }

        template <typename TDataType>
        void readRef( pvd::DataRef<TDataType>& data )
        {
            uint32_t amount;
            read( amount );

            if ( amount == 0 )
            {
                data = pvd::DataRef<TDataType>();
                return;
            }

            TDataType* dataIn = allocate<TDataType>( amount );
            TDataType* readIn = dataIn;
            for ( uint32_t idx = 0; idx < amount; ++idx )
            {
                new (readIn) TDataType();
                readIn->serialize( *this );
                readIn++;
            }

            data = pvd::DataRef<TDataType>( dataIn, amount );
        }

        virtual void streamify( uint16_t& val )
        {
            read( val );
        }
        virtual void streamify( uint8_t& val )
        {
            read( val );
        }
        virtual void streamify( uint32_t& val )
        {
            read( val );
        }
        virtual void streamify( float& val )
        {
            read( val );
        }
        virtual void streamify( uint64_t& val )
        {
            read( val );
        }
        virtual void streamify( pvd::PvdDebugText& val )
        {
            read( val.color );
            read( val.position );
            read( val.size );
            streamify( val.string );
        }

        virtual void streamify( pvd::String& val )
        {
            uint32_t len = 0;
            read( len );
            char* new_val = allocate<char>( len );
            read( new_val, len );
            val = new_val;
        }
        virtual void streamify( pvd::DataRef<const uint8_t>& val )
        {
            readRef( val );
        }
        virtual void streamify( pvd::DataRef<pvd::NameHandleValue>& val )
        {
            readRef( val );
        }
        virtual void streamify( pvd::DataRef<pvd::StreamPropMessageArg>& val )
        {
            readRef( val );
        }
        virtual void streamify( pvd::DataRef<pvd::StringHandle>& val )
        {
            readRef( val );
        }

        EventUnpacker& operator=( const EventUnpacker& ) = delete;
    };

} // namespace physx::pvdsdk::decoder