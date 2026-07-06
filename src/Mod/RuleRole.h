#pragma once
/*
 * Copyright 2010-2016 OpenXcom Developers.
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
#include <string>
#include "../Engine/Yaml.h"

namespace OpenXcom
{

/**
 * DX: Seed definition for a player-authored soldier role.
 *
 * Roles in DX are player-owned savegame data (see Savegame/Role): the player
 * creates, renames, re-icons, recolors, and deletes them in-game. A RuleRole is
 * only a mod-supplied *seed* - a starter role copied into a new game's editable
 * role list so a fresh game has usable defaults out of the box. It intentionally
 * carries no loadout (roles get their loadout from the player); just an identity
 * (name), a role-icon reference (a roleIcons registry name), and a marker/tint colour.
 */
class RuleRole
{
private:
	std::string _name;
	std::string _icon;
	int _color;
public:
	/// Creates a blank role seed with the given (STR) id.
	RuleRole(const std::string& name);
	/// Cleans up the role seed.
	~RuleRole() = default;
	/// Loads the role seed from YAML.
	void load(const YAML::YamlNodeReader& reader);
	/// Gets the role's (STR) id / display name.
	const std::string& getName() const { return _name; }
	/// Gets the role-icon name (a roleIcons registry entry; empty = none).
	const std::string& getIcon() const { return _icon; }
	/// Gets the role's marker/tint palette colour.
	int getColor() const { return _color; }
};

}
