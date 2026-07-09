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
#include "Role.h"
#include <cctype>
#include "EquipmentLayoutItem.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleRole.h"
#include "../Mod/Armor.h"
#include "../Engine/Collections.h"
#include "../Engine/Logger.h"
#include "../Engine/Exception.h"

namespace OpenXcom
{

/**
 * Creates a blank role.
 * @param id The role's stable numeric id.
 */
Role::Role(int id) : _id(id), _color(-1), _loadoutArmor(nullptr)
{
}

/**
 * Creates a role initialised from a mod seed definition.
 * @param id The role's stable numeric id.
 * @param seed The mod-supplied seed to copy identity from.
 */
Role::Role(int id, const RuleRole* seed) :
	_id(id), _name(seed->getName()), _shortName(seed->getShortName()),
	_icon(seed->getIcon()), _color(seed->getColor()),
	_loadoutArmor(nullptr)
{
}

/**
 * Cleans up the role, freeing its loadout template.
 */
Role::~Role()
{
	Collections::deleteAll(_loadout);
}

/**
 * Returns the role's abbreviation for compact displays (e.g. the "MRK-Rookie" rank readout):
 * the authored short name if set, else the first 3 alphanumeric characters of the given
 * (already-localized) display name, uppercased.
 * @param displayName The role's localized display name.
 * @return A short abbreviation (may be empty if displayName has no usable characters).
 */
std::string Role::getAbbreviation(const std::string& displayName) const
{
	if (!_shortName.empty())
		return _shortName;
	std::string abbr;
	for (char c : displayName)
	{
		if (abbr.size() >= 3)
			break;
		if (std::isalnum((unsigned char)c))
			abbr += (char)std::toupper((unsigned char)c);
	}
	return abbr;
}

/**
 * Frees and clears the role's loadout template.
 */
void Role::clearLoadout()
{
	Collections::deleteAll(_loadout);
}

/**
 * Loads the role from a YAML file.
 * @param reader YAML reader.
 * @param mod Mod for the game.
 */
void Role::load(const YAML::YamlNodeReader& reader, const Mod* mod)
{
	reader.tryRead("id", _id);
	reader.tryRead("name", _name);
	reader.tryRead("shortName", _shortName);
	reader.tryRead("icon", _icon);
	reader.tryRead("color", _color);
	for (const auto& layoutItem : reader["loadout"].children())
	{
		try
		{
			_loadout.push_back(new EquipmentLayoutItem(layoutItem, mod));
		}
		catch (Exception& ex)
		{
			Log(LOG_ERROR) << "Error loading Role loadout: " << ex.what();
		}
	}
	if (reader["loadoutArmor"])
		_loadoutArmor = mod->getArmor(reader["loadoutArmor"].readVal<std::string>());
}

/**
 * Saves the role to a YAML file.
 * @param writer YAML writer.
 */
void Role::save(YAML::YamlNodeWriter writer) const
{
	writer.setAsMap();
	writer.write("id", _id);
	if (!_name.empty())
		writer.write("name", _name);
	if (!_shortName.empty())
		writer.write("shortName", _shortName);
	if (!_icon.empty())
		writer.write("icon", _icon);
	if (_color >= 0)
		writer.write("color", _color);
	if (!_loadout.empty())
		writer.write("loadout", _loadout,
			[](YAML::YamlNodeWriter& w, EquipmentLayoutItem* i)
			{ i->save(w.write()); });
	if (_loadoutArmor)
		writer.write("loadoutArmor", _loadoutArmor->getType());
}

}
