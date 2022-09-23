//
//   ____                ___ _   _____ 
//  / __ \___  ___ ___  / _ \ | / / _ \
// / /_/ / _ \/ -_) _ \/ ___/ |/ / // /
// \____/ .__/\__/_//_/_/   |___/____/ 
//     /_/  https://github.com/ishani/OpenPVD
// 
// some parts of the physx lib expect a functioning Foundation instance to be around so that memory allocation and
// logging is hooked up; instance an OpFoundation to get the basics running
//

#include "pch.h"
#include "common/OpFoundation.h"

#include "PxPhysicsVersion.h"
#include "foundation/PxErrorCallback.h"

namespace Op
{

    // ---------------------------------------------------------------------------------------------------------------------
    // all allocations must be 16b aligned
    class PxDefaultAllocator : public physx::PxAllocatorCallback
    {
    public:
        void* allocate( size_t size, const char*, const char*, int ) override
        {
            void* ptr = _aligned_malloc( size, 16 );
            return ptr;
        }

        void deallocate( void* ptr ) override
        {
            _aligned_free( ptr );
        }
    };

    // ---------------------------------------------------------------------------------------------------------------------
    class PxDefaultErrorCallback : public physx::PxErrorCallback
    {
    public:

        void reportError( physx::PxErrorCode::Enum code, const char* message, const char* file, int line ) override
        {
            spdlog::error( "({}:{}) : {}", file, line, message );
        }
    };

    // ---------------------------------------------------------------------------------------------------------------------
    Foundation::Foundation()
    {
        static PxDefaultAllocator gDefaultAllocatorCallback;
        static PxDefaultErrorCallback gDefaultErrorCallback;

        // :wave:
        spdlog::info( R"(   ____                ___ _   _____ )" );
        spdlog::info( R"(  / __ \___  ___ ___  / _ \ | / / _ \)" );
        spdlog::info( R"( / /_/ / _ \/ -_) _ \/ ___/ |/ / // /)" );
        spdlog::info( R"( \____/ .__/\__/_//_/_/   |___/____/ )" );
        spdlog::info( R"(     /_/  https://github.com/ishani/OpenPVD)" );
        spdlog::info( R"( )" );

        m_foundation = PxCreateFoundation( PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback );
    }

    // ---------------------------------------------------------------------------------------------------------------------
    Foundation::~Foundation()
    {
        if ( m_foundation != nullptr )
            m_foundation->release();
    }

} // namespace Op