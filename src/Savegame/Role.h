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
#include <vector>
#include "../Engine/Yaml.h"

namespace OpenXcom
{

class Mod;
class Armor;
class RuleRole;
class EquipmentLayoutItem;

/**
 * DX: A player-authored soldier role (savegame data).
 *
 * Unlike the mod-supplied RuleRole *seed*, a Role is live, mutable, per-save data
 * the player owns: they create, rename, re-icon, recolour, and delete roles
 * in-game. Each role also carries its own equipment-loadout template (the same
 * EquipmentLayoutItem list the global template library uses), so assigning a role
 * can apply its loadout to a soldier. Roles are referenced from Soldier by their
 * stable numeric id (so a rename never breaks an assignment).
 */
class Role
{
private:
	int _id;
	std::string _name;
	std::string _icon;
	int _color;
	std::vector<EquipmentLayoutItem*> _loadout;
	const Armor* _loadoutArmor;
public:
	/// Creates a blank role with the given id (used before loading from YAML).
	Role(int id);
	/// Creates a role initialised from a mod seed definition.
	Role(int id, const RuleRole* seed);
	/// Cleans up the role (frees its loadout).
	~Role();
	/// Loads the role from YAML.
	void load(const YAML::YamlNodeReader& reader, const Mod* mod);
	/// Saves the role to YAML.
	void save(YAML::YamlNodeWriter writer) const;
	/// Gets the role's stable numeric id.
	int getId() const { return _id; }
	/// Gets the role's display name (may be an STR id for seeded roles, or raw player text).
	const std::string& getName() const { return _name; }
	/// Sets the role's display name.
	void setName(const std::string& name) { _name = name; }
	/// Gets the role-icon name (a roleIcons registry entry; empty = none).
	const std::string& getIcon() const { return _icon; }
	/// Sets the role-icon name (a roleIcons registry entry).
	void setIcon(const std::string& icon) { _icon = icon; }
	/// Gets the role's marker/tint palette colour.
	int getColor() const { return _color; }
	/// Sets the role's marker/tint palette colour.
	void setColor(int color) { _color = color; }
	/// Gets the role's loadout template (mutable, so it can be (re)filled from the inventory).
	std::vector<EquipmentLayoutItem*>& getLoadout() { return _loadout; }
	/// Gets the role's loadout template (const).
	const std::vector<EquipmentLayoutItem*>& getLoadout() const { return _loadout; }
	/// Frees and clears the role's loadout template.
	void clearLoadout();
	/// Gets the armor the loadout template was built for (may be null).
	const Armor* getLoadoutArmor() const { return _loadoutArmor; }
	/// Sets the armor the loadout template was built for.
	void setLoadoutArmor(const Armor* armor) { _loadoutArmor = armor; }
};

}
