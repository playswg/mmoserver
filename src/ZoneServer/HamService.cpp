/*
---------------------------------------------------------------------------------------
This source file is part of SWG:ANH (Star Wars Galaxies - A New Hope - Server Emulator)

For more information, visit http://www.swganh.com

Copyright (c) 2006 - 2010 The SWG:ANH Team
---------------------------------------------------------------------------------------
Use of this source code is governed by the GPL v3 license that can be found
in the COPYING file or at http://www.gnu.org/licenses/gpl-3.0.html

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
---------------------------------------------------------------------------------------
*/

#include "ZoneServer/HamService.h"
#include "Common/LogManager.h"
#include "SwgProtocol/ObjectControllerEvents.h"
#include "ZoneServer/PlayerObject.h"
#include "ZoneServer/WorldManager.h"

using ::common::ApplicationService;
using ::common::IEventPtr;
using ::common::EventType;
using ::common::EventListener;
using ::common::EventListenerType;

namespace zone {

HamService::HamService(::common::EventDispatcher& event_dispatcher, const CmdPropertyMap& command_property_map)
: ::common::ApplicationService(event_dispatcher)
, command_property_map_(command_property_map) {}

HamService::~HamService() {}

void HamService::onInitialize() {
    event_dispatcher_.Connect(::swg_protocol::object_controller::PreCommandExecuteEvent::type, EventListener(EventListenerType("HamService::handleSuccessfulObjectControllerCommand"), std::bind(&HamService::handlePreCommandExecuteEvent, this, std::placeholders::_1)));
}

bool HamService::handlePreCommandExecuteEvent(IEventPtr triggered_event) {
    // Cast the IEvent to the PreCommandEvent.
    std::shared_ptr<::swg_protocol::object_controller::PreCommandExecuteEvent> pre_event = std::dynamic_pointer_cast<::swg_protocol::object_controller::PreCommandExecuteEvent>(triggered_event);
    if (!pre_event) {
        assert(!"Received an invalid event!");
        return false;
    }
    
    // This command doesn't exist in the properties map and shouldn't be executed further.
    CmdPropertyMap::const_iterator it = command_property_map_.find(pre_event->command_crc());
    if (it == command_property_map_.end()) {
        return false;
    }

    // Lookup the creature and ensure it is a valid object.
    PlayerObject* object = dynamic_cast<PlayerObject*>(gWorldManager->getObjectById(pre_event->subject()));
    if (!object) {
        return false;
    }
    
    uint32 actioncost = (*it).second->mActionCost;
    uint32 healthcost = (*it).second->mHealthCost;
    uint32 mindcost	  = (*it).second->mMindCost;

    if (!object->getHam()->checkMainPools(healthcost, actioncost, mindcost)) {
        gMessageLib->SendSystemMessage(L"You cannot <insert command> right now.", object); // the stf doesn't work!
        return false;
    }

    object->getHam()->updatePropertyValue(HamBar_Action, HamProperty_CurrentHitpoints, -static_cast<int32>(actioncost), true);
    object->getHam()->updatePropertyValue(HamBar_Health, HamProperty_CurrentHitpoints, -static_cast<int32>(healthcost), true);
    object->getHam()->updatePropertyValue(HamBar_Mind, HamProperty_CurrentHitpoints, -static_cast<int32>(mindcost), true);

    gLogger->log(LogManager::CRITICAL, "HAM Service - PreCommandEvent successfully processed");

    return true;
}

}  // namespace zone
