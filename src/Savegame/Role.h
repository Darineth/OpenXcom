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

#ifndef OPENXCOM_ROLE_H
#define OPENXCOM_ROLE_H

#include <vector>
#include <yaml-cpp/yaml.h>

namespace OpenXcom
{
class EquipmentLayoutItem;
class SavedGame;
class RuleRole;

class Role
{
private:
	std::string _name;
	RuleRole *_rules;
	std::vector<EquipmentLayoutItem*> _defaultLayout;

	void clearDefaultLayout();

public:
	/// Creates a new role.
	Role(std::string name, RuleRole *rules);
	/// Cleans up the role.
	~Role();
	/// Loads the role from YAML.
	void load(const YAML::Node& node, SavedGame *save);
	/// Saves the role to YAML.
	YAML::Node save() const;
	/// Gets the rules that define the role.
	RuleRole *getRules() const;
	/// Sets the default equipment layout for this role.
	void setDefaultLayout(const std::vector<EquipmentLayoutItem*> &layout);
	/// Gets the default equipment layout for this role.
	std::vector<EquipmentLayoutItem*> getDefaultLayout() const;
	/// Gets if the role is the empty role.
	bool isBlank() const;
	/// Returns the role type name.
	std::string getName() const;
};
}

#endif
