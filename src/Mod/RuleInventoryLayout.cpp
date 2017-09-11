/*
 * Copyright 2010-2014 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "RuleInventoryLayout.h"
#include "RuleInventory.h"
#include "Mod.h"

namespace OpenXcom
{
RuleInventoryLayout::RuleInventoryLayout(const std::string &id) : _id(id), _slotIds(), _slots(), _invSpriteOffset(0)
{
}

RuleInventoryLayout::~RuleInventoryLayout()
{
	_slotIds.clear();
	_slots.clear();
}

void RuleInventoryLayout::load(const YAML::Node& node, Mod *mod)
{
	if (const YAML::Node &parent = node["refNode"])
	{
		load(parent, mod);
	}
	_slotIds = node["invs"].as< std::vector<std::string> >(_slotIds);
	_invSpriteOffset = node["invSpriteOffset"].as<int>(_invSpriteOffset);

	_slots.clear();
	for(std::vector<std::string>::const_iterator ii = _slotIds.begin(); ii != _slotIds.end(); ++ii)
	{
		_slots.push_back(mod->getInventory(*ii));
	}
}

/**
 * Gets the language string that names
 * this inventory section. Each section has a unique name.
 * @return The section name.
 */
std::string RuleInventoryLayout::getId() const
{
	return _id;
}

/**
 * Gets all the slots in the inventory section.
 * @return The list of slots.
 */
const std::vector<RuleInventory*> *RuleInventoryLayout::getSlots() const
{
	return &_slots;
}

bool RuleInventoryLayout::hasSlot(const std::string &id) const
{
	for(std::vector<RuleInventory*>::const_iterator ii = _slots.begin(); ii != _slots.end(); ++ii)
	{
		if((*ii)->getId() == id)
			return true;
	}
	return false;
}

int RuleInventoryLayout::getInvSpriteOffset() const
{
	return _invSpriteOffset;
}
}
