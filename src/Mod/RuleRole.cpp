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
#include "RuleRole.h"

namespace OpenXcom
{
RuleRole *RuleRole::s_defaultRole = 0;

/**
 * Creates a blank ruleset for a certain
 * type of Role section.
 * @param id String defining the id.
 */
RuleRole::RuleRole(const std::string &name) : _name(name), _iconSprite(""), _smallIconSprite(), _isBlank(false), _defaultArmorColor("")
{
}

RuleRole::~RuleRole()
{
	if(s_defaultRole == this) { s_defaultRole = 0; }
}

/**
 * Loads the Role from a YAML file.
 * @param node YAML node.
 * @param listOrder The list weight for this Role.
 */
void RuleRole::load(const YAML::Node &node, int listOrder)
{
	_name = node["name"].as<std::string>(_name);
	_iconSprite = node["iconSprite"].as<std::string>(_iconSprite);
	_smallIconSprite = node["smallIconSprite"].as<std::string>(_smallIconSprite);
	_isBlank = node["isBlank"].as<bool>(_isBlank);
	_defaultArmorColor = node["defaultArmorColor"].as<std::string>(_defaultArmorColor);

	if(_isBlank)
	{
		s_defaultRole = this;
	}
}

/**
 * Gets the language string that names
 * this Role section. Each section has a unique name.
 * @return The role name.
 */
const std::string &RuleRole::getName() const
{
	return _name;
}

/**
 * Gets the surface name for the role's icon sprite.
 * @return Icon sprite surface name.
 */
const std::string &RuleRole::getIconSprite() const
{
	return _iconSprite;
}

/**
 * Gets the surface name for the role's small icon sprite.
 * @return Icon sprite surface name.
 */
const std::string &RuleRole::getSmallIconSprite() const
{
	return _smallIconSprite;
}

const std::string &RuleRole::getDefaultArmorColor() const
{
	return _defaultArmorColor;
}

/**
 * Gets if the role is the blank role.
 * @return True if the role is the blank role.
 */
bool RuleRole::isBlank() const
{
	return _isBlank;
}

/**
 * Sets the default role.
 * @param The default role for soldiers to use.
 */
void RuleRole::setDefaultRole(RuleRole *role)
{
	s_defaultRole = role;
}

/**
 * Gets the default role.
 * @return The default role for soldiers to use.
 */
RuleRole *RuleRole::getDefaultRole()
{
	return s_defaultRole;
}
}