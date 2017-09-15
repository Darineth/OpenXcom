/*
 * Copyright 2010-2015 OpenXcom Developers.
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
#include "RuleStartingCondition.h"
#include "RuleItem.h"
#include "../Mod/Mod.h"
#include "../Savegame/WeightedOptions.h"
#include <algorithm>

namespace YAML
{
	template<>
	struct convert<OpenXcom::EnvironmentalCondition>
	{
		static Node encode(const OpenXcom::EnvironmentalCondition& rhs)
		{
			Node node;
			node["chancePerTurn"] = rhs.chancePerTurn;
			node["firstTurn"] = rhs.firstTurn;
			node["lastTurn"] = rhs.lastTurn;
			node["message"] = rhs.message;
			node["color"] = rhs.color;
			node["weaponOrAmmo"] = rhs.weaponOrAmmo;
			node["side"] = rhs.side;
			node["bodyPart"] = rhs.bodyPart;
			return node;
		}

		static bool decode(const Node& node, OpenXcom::EnvironmentalCondition& rhs)
		{
			if (!node.IsMap())
				return false;

			rhs.chancePerTurn = node["chancePerTurn"].as<int>(rhs.chancePerTurn);
			rhs.firstTurn = node["firstTurn"].as<int>(rhs.firstTurn);
			rhs.lastTurn = node["lastTurn"].as<int>(rhs.lastTurn);
			rhs.message = node["message"].as<std::string>(rhs.message);
			rhs.color = node["color"].as<int>(rhs.color);
			rhs.weaponOrAmmo = node["weaponOrAmmo"].as<std::string>(rhs.weaponOrAmmo);
			rhs.side = node["side"].as<int>(rhs.side);
			rhs.bodyPart = node["bodyPart"].as<int>(rhs.bodyPart);
			return true;
		}
	};
}

namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of Starting Condition.
 * @param type String defining the type.
 */
RuleStartingCondition::RuleStartingCondition(const std::string &type) : _type(type), _mapBackgroundColor(15)
{
}

/**
 *
 */
RuleStartingCondition::~RuleStartingCondition()
{
}

/**
 * Loads the Starting Conditions from a YAML file.
 * @param node YAML node.
 */
void RuleStartingCondition::load(const YAML::Node &node)
{
	if (const YAML::Node &parent = node["refNode"])
	{
		load(parent);
	}
	_type = node["type"].as<std::string>(_type);
	_environmentalConditions = node["environmentalConditions"].as< std::map<std::string, EnvironmentalCondition> >(_environmentalConditions);
	_armorTransformations = node["armorTransformations"].as< std::map<std::string, std::string> >(_armorTransformations);
	_defaultArmor = node["defaultArmor"].as< std::map<std::string, std::map<std::string, int> > >(_defaultArmor);
	_defaultItems = node["defaultItems"].as< std::map<std::string, int> >(_defaultItems);
	_allowedArmors = node["allowedArmors"].as< std::vector<std::string> >(_allowedArmors);
	_allowedVehicles = node["allowedVehicles"].as< std::vector<std::string> >(_allowedVehicles);
	_allowedItems = node["allowedItems"].as< std::vector<std::string> >(_allowedItems);
	_allowedItemCategories = node["allowedItemCategories"].as< std::vector<std::string> >(_allowedItemCategories);
	_allowedCraft = node["allowedCraft"].as< std::vector<std::string> >(_allowedCraft);
	_mapBackgroundColor = node["mapBackgroundColor"].as<int>(_mapBackgroundColor);
}

/**
 * Returns the language string that names the Starting Condition. Each type has a unique name.
 * @return Starting Condition name.
 */
std::string RuleStartingCondition::getType() const
{
	return _type;
}

/**
* Gets the environmental condition for a given faction.
* @param faction Faction code (STR_FRIENDLY, STR_HOSTILE or STR_NEUTRAL).
* @return Environmental condition definition.
*/
EnvironmentalCondition RuleStartingCondition::getEnvironmetalCondition(const std::string &faction) const
{
	if (!_environmentalConditions.empty())
	{
		std::map<std::string, EnvironmentalCondition>::const_iterator i = _environmentalConditions.find(faction);
		if (i != _environmentalConditions.end())
		{
			return i->second;
		}
	}

	return EnvironmentalCondition();
}

/**
* Returns all allowed armor types.
* @return List of armor types.
*/
const std::vector<std::string> *RuleStartingCondition::getAllowedArmors() const
{
	return &_allowedArmors;
}

/**
* Returns all allowed craft types.
* @return List of craft types.
*/
const std::vector<std::string> *RuleStartingCondition::getAllowedCraft() const
{
	return &_allowedCraft;
}

/**
* Checks if the craft type is allowed.
* @param craftType Craft type name.
* @return True if allowed, false otherwise.
*/
bool RuleStartingCondition::isCraftAllowed(const std::string &craftType) const
{
	return _allowedCraft.empty() || (std::find(_allowedCraft.begin(), _allowedCraft.end(), craftType) != _allowedCraft.end());
}

/**
* Gets the replacement armor.
* @param soldierType Soldier type name.
* @param armorType Existing/old armor type name.
* @return Replacement armor type name (or empty string if no replacement is needed).
*/
std::string RuleStartingCondition::getArmorReplacement(const std::string &soldierType, const std::string &armorType) const
{
	if (!_allowedArmors.empty() && (std::find(_allowedArmors.begin(), _allowedArmors.end(), armorType) == _allowedArmors.end()))
	{
		std::map<std::string, std::map<std::string, int> >::const_iterator j = _defaultArmor.find(soldierType);
		if (j != _defaultArmor.end())
		{
			WeightedOptions w = WeightedOptions();
			for (std::map<std::string, int>::const_iterator k = (j->second).begin(); k != (j->second).end(); ++k)
			{
				w.set(k->first, k->second);
			}
			std::string pick = w.choose();
			return pick == "noChange" ? "" : pick;
		}
	}

	return "";
}

/**
* Gets the transformed armor.
* @param armorType Existing/old armor type name.
* @return Transformed armor type name (or empty string if there is no transformation).
*/
std::string RuleStartingCondition::getArmorTransformation(const std::string &armorType) const
{
	if (!_armorTransformations.empty())
	{
		std::map<std::string, std::string>::const_iterator i = _armorTransformations.find(armorType);
		if (i != _armorTransformations.end())
		{
			return i->second;
		}
	}

	return "";
}

/**
* Checks if the vehicle type is allowed.
* @param vehicleType Vehicle type name.
* @return True if allowed, false otherwise.
*/
bool RuleStartingCondition::isVehicleAllowed(const std::string &vehicleType) const
{
	return _allowedVehicles.empty() || (std::find(_allowedVehicles.begin(), _allowedVehicles.end(), vehicleType) != _allowedVehicles.end());
}

/**
* Checks if the item type is allowed.
* @param itemType Item type name.
* @return True if allowed, false otherwise.
*/
bool RuleStartingCondition::isItemAllowed(const std::string &itemType, Mod *mod) const
{
	if (_allowedItems.empty() && _allowedItemCategories.empty())
	{
		return true; // if nothing is specified, everything is allowed
	}
	else if (std::find(_allowedItems.begin(), _allowedItems.end(), itemType) != _allowedItems.end())
	{
		return true; // item is explicitly allowed
	}
	else
	{
		if (_allowedItemCategories.empty())
			return false; // no categories are allowed, stop looking

		RuleItem *item = mod->getItem(itemType);
		if (item)
		{
			std::vector<std::string> itemCategories = item->getCategories();
			if (!itemCategories.empty())
			{
				// check all categories of the item
				for (std::vector<std::string>::iterator i = itemCategories.begin(); i != itemCategories.end(); ++i)
				{
					if (std::find(_allowedItemCategories.begin(), _allowedItemCategories.end(), (*i)) != _allowedItemCategories.end())
					{
						return true; // found a category that is allowed
					}
				}
			}
		}
	}

	return false; // if everything fails, item is not allowed
}

/**
* Returns all the default items.
* @return List of items.
*/
const std::map<std::string, int> *RuleStartingCondition::getDefaultItems() const
{
	return &_defaultItems;
}

/**
* Returns the battlescape map background color.
* @return Color code.
*/
int RuleStartingCondition::getMapBackgroundColor() const
{
	return _mapBackgroundColor;
}

}
