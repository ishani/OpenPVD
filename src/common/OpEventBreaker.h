//
//   ____                ___ _   _____ 
//  / __ \___  ___ ___  / _ \ | / / _ \
// / /_/ / _ \/ -_) _ \/ ___/ |/ / // /
// \____/ .__/\__/_//_/_/   |___/____/ 
//     /_/  https://github.com/ishani/OpenPVD
// 
// 
//

#pragma once

#include "common/OpMasterStringTable.h"

namespace Op
{
    struct FilterState
    {
        using InstanceSet = ankerl::unordered_dense::set< uint64_t >;
        using InstanceLimit = ankerl::unordered_dense::map < std::string, uint64_t >;

        InstanceSet     m_filteredInstanceIDs;
        InstanceLimit   m_instanceLimits;


        void addInstance( const uint64_t instanceID )
        {
            m_filteredInstanceIDs.emplace( instanceID );
        }

        bool isInstanceFiltered( const uint64_t instanceID )
        {
            return m_filteredInstanceIDs.contains( instanceID );
        }

        void removeInstance( const uint64_t instanceID )
        {
            m_filteredInstanceIDs.erase( instanceID );
        }
    };

    struct EventBreaker : public MasterStringTable
    {
        using InstanceTypeMap = ankerl::unordered_dense::map < uint64_t, std::string >;
        using InstancePayload = ankerl::unordered_dense::map < std::string, uint64_t >;

        std::shared_ptr< spdlog::logger >    m_verboseLog;

        InstanceTypeMap m_instanceTypeMap;
        InstancePayload m_instanceDataSizes;
        InstancePayload m_instanceCount;

        uint64_t        m_currentFrame = 0;

        bool            m_multiSetPropertyValueActive       = false;
        uint64_t        m_multiSetPropertyValueInstanceID   = 0;


        EventBreaker()
        {
            m_instanceTypeMap.reserve( 2048 );
            m_instanceDataSizes.reserve( 512 );
            m_instanceCount.reserve( 512 );
        }

        void logSummary();

        inline std::string getInstanceTypeFromID( const uint64_t id ) const
        {
            std::string instanceType = "unknown";
            if ( auto it = m_instanceTypeMap.find( id ); it != m_instanceTypeMap.end() )
                instanceType = it->second;

            return instanceType;
        }

        void logStartEvent( const char* eventTitle );

        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::StringHandleEvent& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::CreateClass& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::DeriveClass& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::CreateProperty& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::CreatePropertyMessage& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::CreateInstance& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::SetPropertyValue& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::BeginSetPropertyValue& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::AppendPropertyValueData& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::EndSetPropertyValue& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::SetPropertyMessage& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::BeginPropertyMessageGroup& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::SendPropertyMessageFromGroup& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::EndPropertyMessageGroup& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::DestroyInstance& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::PushBackObjectRef& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::RemoveObjectRef& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::BeginSection& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::EndSection& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::SetPickable& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::SetColor& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::SetIsTopLevel& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::SetCamera& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::AddProfileZone& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::AddProfileZoneEvent& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::StreamEndEvent& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::ErrorMessage& _event );
        bool handleEvent( FilterState& _filtering, const physx::pvdsdk::EventGroup& _group, const physx::pvdsdk::OriginShift& _event );
    };
}