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
#ifndef OPENXCOM_RULEROLE_H
#define OPENXCOM_RULEROLE_H

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace OpenXcom
{

/**
 * Represents a specific section of the Role,
 * containing information like available slots and
 * screen position.
 */
class RuleRole
{
private:
	std::string _name;
	std::string _iconSprite;
	std::string _smallIconSprite;
	bool _isBlank;

	static RuleRole *s_defaultRole;

public:
	/// Creates a blank Role ruleset.
	RuleRole(const std::string &name);
	/// Cleans up the Role ruleset.
	~RuleRole();
	/// Loads Role data from YAML.
	void load(const YAML::Node& node, int listOrder);
	/// Gets the Role's name.
	std::string getName() const;
	/// Get role icon
	std::string getIconSprite() const;
	/// Get small role icon
	std::string getSmallIconSprite() const;
	/// Gets if the role is the blank role.
	bool isBlank() const;
	/// Sets the default role.
	static void setDefaultRole(RuleRole *role);
	/// Gets the default role.
	static RuleRole *getDefaultRole();
};

}

#endif
