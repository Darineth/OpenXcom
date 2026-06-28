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
class RuleInventory;

/**
 * Represents a named set of inventory sections (slots) that can be assigned to
 * a unit via its armor. A layout either reuses globally-defined `invs` sections
 * (by `ref:`) or defines sections inline; the resolved, ordered section list is
 * what the inventory UI and item-placement code iterate for that unit.
 *
 * With no layouts defined, the engine synthesizes an implicit default layout
 * from the global `invs` set so behavior is unchanged (see Mod).
 */
class RuleInventoryLayout
{
private:
	/// One ordered entry in the layout: either a ref to a global section or an owned inline section.
	struct SectionSpec
	{
		std::string ref;                  // non-empty => reference a global `invs` section by id
		RuleInventory* inlineSection = 0; // owned inline section (when ref is empty)
	};
	std::string _id;
	int _listOrder;
	std::vector<SectionSpec> _spec;                  // load-time order, preserved across ref/inline mix
	std::vector<RuleInventory*> _owned;              // inline sections owned by this layout
	std::vector<const RuleInventory*> _sections;     // resolved, ordered section list
public:
	/// Creates a blank inventory layout ruleset.
	RuleInventoryLayout(const std::string& id, int listOrder);
	/// Cleans up the inventory layout ruleset (and any inline sections it owns).
	~RuleInventoryLayout();
	/// Loads the layout from YAML.
	void load(const YAML::YamlNodeReader& reader);
	/// Resolves section refs and guarantees a ground section.
	void afterLoad(const Mod* mod);
	/// Gets the layout's id.
	const std::string& getId() const { return _id; }
	/// Gets the list order weight.
	int getListOrder() const { return _listOrder; }
	/// Gets the resolved, ordered sections of this layout.
	const std::vector<const RuleInventory*>& getSections() const { return _sections; }
	/// Directly sets the resolved sections (used by Mod to synthesize the implicit default layout).
	void setSectionsDirectly(std::vector<const RuleInventory*> sections) { _sections = std::move(sections); }
};

}
