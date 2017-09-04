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
#ifndef OPENXCOM_RULEINVENTORYLAYOUT_H
#define OPENXCOM_RULEINVENTORYLAYOUT_H

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace OpenXcom
{

class Mod;
class RuleInventory;

/**
 * Represents a specific section of the inventory,
 * containing information like available slots and
 * screen position.
 */
class RuleInventoryLayout
{
private:
	std::string _id;
	std::vector<std::string> _slotIds;
	std::vector<RuleInventory*> _slots;
	int _invSpriteOffset;
public:
	RuleInventoryLayout(const std::string &id);
	~RuleInventoryLayout();
	/// Loads inventory layout from YAML.
	void load(const YAML::Node& node, Mod *mod);
	/// Gets the inventory layout's id.
	std::string getId() const;
	/// Gets all the slots in the inventory layout.
	const std::vector<RuleInventory*> *getSlots() const;
	bool hasSlot(const std::string &id) const;
	int getInvSpriteOffset() const;
};

}

#endif
