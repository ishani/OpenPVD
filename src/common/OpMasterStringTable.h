//
//   ____                ___ _   _____ 
//  / __ \___  ___ ___  / _ \ | / / _ \
// / /_/ / _ \/ -_) _ \/ ___/ |/ / // /
// \____/ .__/\__/_//_/_/   |___/____/ 
//     /_/  https://github.com/ishani/OpenPVD
// 
// strings are captured and sent just once in the event stream, all events that refer to strings then sending a 
// pre-ordained handle value to look up the original. OpMasterStringTable is a simple container for accepting
// new string handle lookup values and for resolving strings/namespaces as appropriate
//

#pragma once

#include "PxPvdCommStreamEvents.h"

namespace Op
{

    // ---------------------------------------------------------------------------------------------------------------------
    struct ResolvedStreamNamespacedName
    {
        std::string mNamespace;
        std::string mName;
    };

    // ---------------------------------------------------------------------------------------------------------------------
    struct MasterStringTable
    {
        inline void addFromStringHandleEvent( const physx::pvdsdk::StringHandleEvent& _event )
        {
            m_stringTable.emplace( _event.mHandle, _event.mString );
        }

        [[nodiscard]] inline const std::string& lookupStringByHandle( const uint32_t handle ) const
        {
            const auto it = m_stringTable.find( handle );
            if ( it == m_stringTable.end() )
            {
                spdlog::error( "invalid string handle found : {}", handle );
                return m_invalidString;
            }
            return it->second;
        }

        // this incomplete decl is here to avoid accidentally trying to resolve a StringHandle as a namespace; StreamNamespacedName
        // will silently get constructed with a single StringHandle arg and null out one of the entries otherwise
        inline ResolvedStreamNamespacedName lookupNamespace( const physx::pvdsdk::StringHandle& ) const;

        [[nodiscard]] inline ResolvedStreamNamespacedName lookupNamespace( const physx::pvdsdk::StreamNamespacedName& snn ) const
        {
            return {
                lookupStringByHandle( snn.mNamespace ),
                lookupStringByHandle( snn.mName )
            };
        }

    private:

        using StringTableMap = ankerl::unordered_dense::map< uint32_t, std::string >;

        StringTableMap m_stringTable;                           // table of handles:string build as events are parsed
        const std::string m_invalidString = "<invalid>";        // return value for any unresolved string table lookups
    };

} // namespace Op