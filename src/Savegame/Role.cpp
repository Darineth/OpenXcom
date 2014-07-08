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
#include "Role.h"

#include "../Ruleset/RuleRole.h"
#include "EquipmentLayoutItem.h"

namespace OpenXcom
{
/// Creates a new role.
Role::Role(std::string name, RuleRole *rules) : _name(name), _rules(rules), _defaultLayout()
{
}

/// Cleans up the role.
Role::~Role()
{
	clearDefaultLayout();
}

/// Clears the default layout vector.
void Role::clearDefaultLayout()
{
	for(auto ii = _defaultLayout.begin(); ii != _defaultLayout.end(); ++ii)
	{
		delete *ii;
	}

	_defaultLayout.clear();
}

/// Loads the role from YAML.
void Role::load(const YAML::Node& node, SavedGame *save)
{
	_name = node["name"].as<std::string>(_name);

	clearDefaultLayout();
	for (YAML::const_iterator ii = node["defaultLayout"].begin(); ii != node["defaultLayout"].end(); ++ii)
	{
		_defaultLayout.push_back(new EquipmentLayoutItem(*ii));
	}
}

/// Saves the role to YAML.
YAML::Node Role::save() const
{
	YAML::Node node;

	node["name"] = _name;
	
	for(auto ii = _defaultLayout.begin(); ii != _defaultLayout.end(); ++ii)
	{
		node["defaultLayout"].push_back((*ii)->save());
	}

	return node;
}

RuleRole *Role::getRules() const
{
	return _rules;
}

/// Sets the default equipment layout for this role.
void Role::setDefaultLayout(const std::vector<EquipmentLayoutItem*> &layout)
{
	clearDefaultLayout();

	for(auto ii = layout.begin(); ii != layout.end(); ++ii)
	{
		_defaultLayout.push_back(new EquipmentLayoutItem(**ii));
	}
}

/// Gets the default equipment layout for this role.
std::vector<EquipmentLayoutItem*> Role::getDefaultLayout() const
{
	return _defaultLayout;
}

bool Role::isBlank() const
{
	return _rules->isBlank();
}

/// Returns the role type name.
std::string Role::getName() const
{
	return _name;
}
}
