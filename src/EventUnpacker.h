#include "pvd/PxPvd.h"

#include "PxPvdObjectModelBaseTypes.h"
#include "PxPvdCommStreamEvents.h"
#include "PxPvdCommStreamTypes.h"

namespace physx::pvdsdk::decoder
{
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

    template <typename TStreamType>
    struct EventUnpacker : public PvdEventSerializer
    {
        TStreamType& mBuffer;
        EventUnpacker( TStreamType& buf ) : mBuffer( buf )
        {
        }

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

        void readRef( DataRef<const uint8_t>& data )
        {
            uint32_t amount;
            read( amount );

            if ( amount == 0 )
            {
                data = DataRef<const uint8_t>();
                return;
            }

            uint8_t* dataIn = (uint8_t*)malloc( sizeof( uint8_t ) * amount );
            read< uint8_t >( dataIn, amount );

            data = DataRef<const uint8_t>( dataIn, amount );
        }

        void readRef( DataRef<StringHandle>& data )
        {
            uint32_t amount;
            read( amount );

            if ( amount == 0 )
            {
                data = DataRef<StringHandle>();
                return;
            }

            StringHandle* dataIn = (StringHandle*)malloc( sizeof( StringHandle ) * amount );
            read< StringHandle >( dataIn, amount );

            data = DataRef<StringHandle>( dataIn, amount );
        }

        template <typename TDataType>
        void readRef( DataRef<TDataType>& data )
        {
            uint32_t amount;
            read( amount );

            if ( amount == 0 )
            {
                data = DataRef<TDataType>();
                return;
            }

            TDataType* dataIn = (TDataType*)malloc( sizeof( TDataType ) * amount );
            TDataType* readIn = dataIn;
            for ( uint32_t idx = 0; idx < amount; ++idx )
            {
                new (readIn) TDataType();
                readIn->serialize( *this );
                readIn++;
            }

            data = DataRef<TDataType>( dataIn, amount );
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
        virtual void streamify( PvdDebugText& val )
        {
            read( val.color );
            read( val.position );
            read( val.size );
            streamify( val.string );
        }

        virtual void streamify( String& val )
        {
            uint32_t len = 0;
            read( len );
            char* new_val = (char*)malloc( sizeof( char ) * len );
            read( new_val, len );
            val = new_val;
        }
        virtual void streamify( DataRef<const uint8_t>& val )
        {
            readRef( val );
        }
        virtual void streamify( DataRef<NameHandleValue>& val )
        {
            readRef( val );
        }
        virtual void streamify( DataRef<StreamPropMessageArg>& val )
        {
            readRef( val );
        }
        virtual void streamify( DataRef<StringHandle>& val )
        {
            readRef( val );
        }

    private:
        EventUnpacker& operator=( const EventUnpacker& );
    };

} // namespace physx::pvdsdk::decoder