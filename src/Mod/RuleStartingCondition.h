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
#ifndef OPENXCOM_RULESTARTINGCONDITION_H
#define OPENXCOM_RULESTARTINGCONDITION_H

#include <vector>
#include <string>
#include <yaml-cpp/yaml.h>

namespace OpenXcom
{
class Mod;

struct EnvironmentalCondition
{
	int chancePerTurn;
	int firstTurn, lastTurn;
	std::string message;
	int color;
	std::string weaponOrAmmo;
	int side;
	int bodyPart;
	EnvironmentalCondition() : chancePerTurn(0), firstTurn(1), lastTurn(1000), color(29), side(-1), bodyPart(-1) { /*Empty by Design*/ };
};

/**
 * Represents a specific Starting Condition.
 */
class RuleStartingCondition
{
private:
	std::string _type;
	std::map<std::string, EnvironmentalCondition> _environmentalConditions;
	std::map<std::string, std::string> _armorTransformations;
	std::map<std::string, std::map<std::string, int> > _defaultArmor;
	std::map<std::string, int> _defaultItems;
	std::vector<std::string> _allowedArmors;
	std::vector<std::string> _allowedVehicles;
	std::vector<std::string> _allowedItems;
	std::vector<std::string> _allowedItemCategories;
	std::vector<std::string> _allowedCraft;
	int _mapBackgroundColor;
public:
	/// Creates a blank Starting Conditions ruleset.
	RuleStartingCondition(const std::string &type);
	/// Cleans up the Starting Conditions ruleset.
	~RuleStartingCondition();
	/// Loads Starting Conditions data from YAML.
	void load(const YAML::Node& node);
	/// Gets the Starting Conditions's type.
	std::string getType() const;
	/// Gets the environmental condition for a given faction.
	EnvironmentalCondition getEnvironmetalCondition(const std::string &faction) const;
	/// Gets the allowed armor types.
	const std::vector<std::string> *getAllowedArmors() const;
	/// Gets the allowed craft types.
	const std::vector<std::string> *getAllowedCraft() const;
	/// Checks if the craft type is allowed.
	bool isCraftAllowed(const std::string &craftType) const;
	/// Gets the replacement armor.
	std::string getArmorReplacement(const std::string &soldierType, const std::string &armorType) const;
	/// Gets the transformed armor.
	std::string getArmorTransformation(const std::string &armorType) const;
	/// Checks if the vehicle type is allowed.
	bool isVehicleAllowed(const std::string &vehicleType) const;
	/// Checks if the item type is allowed.
	bool isItemAllowed(const std::string &itemType, Mod *mod) const;
	/// Gets the default items.
	const std::map<std::string, int> *getDefaultItems() const;
	/// Gets the battlescape map background color.
	int getMapBackgroundColor() const;
};

}
#endif
