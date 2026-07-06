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
 * DX: A named soldier-role icon (registry entry).
 *
 * Bundles a badge + battlescape-marker sprite pair under a stable string name.
 * Both sprites are referenced by *surface name* (resolved via Mod::getSurface),
 * NOT by SurfaceSet frame index - so the icon identity is a string end-to-end
 * (role -> roleIcons name -> surface names) and survives mod-list / load-order
 * changes with no sprite-offset or shared-frame handling. A Role stores this
 * entry's name; the picker enumerates the registry.
 */
class RuleRoleIcon
{
private:
	std::string _name;
	std::string _sprite;
	std::string _mapSprite;
public:
	/// Creates a role-icon registry entry with the given name.
	RuleRoleIcon(const std::string& name);
	/// Cleans up the role-icon entry.
	~RuleRoleIcon() = default;
	/// Loads the role-icon entry from YAML.
	void load(const YAML::YamlNodeReader& reader);
	/// Gets the entry's stable name (what a Role stores).
	const std::string& getName() const { return _name; }
	/// Gets the badge surface name (inventory + battlescape stats bar).
	const std::string& getSprite() const { return _sprite; }
	/// Gets the battlescape map-marker surface name (may be empty -> keep default arrow).
	const std::string& getMapSprite() const { return _mapSprite; }
};

}
