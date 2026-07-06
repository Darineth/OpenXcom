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
#include "RuleRoleIcon.h"

namespace OpenXcom
{

/**
 * Creates a role-icon registry entry.
 * @param name The entry's stable name.
 */
RuleRoleIcon::RuleRoleIcon(const std::string& name) : _name(name)
{
}

/**
 * Loads the role-icon entry from a YAML file.
 * @param reader YAML reader.
 */
void RuleRoleIcon::load(const YAML::YamlNodeReader& reader)
{
	reader.tryRead("sprite", _sprite);
	reader.tryRead("mapSprite", _mapSprite);
}

}
