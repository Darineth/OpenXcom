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
 * a unit via its armor. A layout is an ordered `invs:` list of globally-defined `invs`
 * section ids; the resolved, ordered section list is what the inventory UI and
 * item-placement code iterate for that unit. A section that should only appear in
 * specific layouts is simply not listed in the others.
 *
 * The standard slot set ships as the `STR_STANDARD_INV` layout in the base game data;
 * an armor that sets no `inventoryLayout` falls back to it (or, if a mod doesn't define
 * it, to an implicit layout synthesized from all global `invs` - see Mod).
 */
class RuleInventoryLayout
{
private:
	std::string _id;
	int _listOrder;
	std::vector<std::string> _refs;                  // ordered global-section ids referenced by this layout
	std::vector<const RuleInventory*> _sections;     // resolved, ordered section list
	const RuleInventory* _rightHand = nullptr;       // this layout's right/left hand sections (by `hand:`)
	const RuleInventory* _leftHand = nullptr;
	const RuleInventory* _utilitySlot = nullptr;     // this layout's first INV_UTILITY section, if any
	/// Resolves the cached hand and utility sections from _sections; throws if a layout has two same-side hands.
	void resolveHands();
public:
	/// Creates a blank inventory layout ruleset.
	RuleInventoryLayout(const std::string& id, int listOrder);
	/// Cleans up the inventory layout ruleset.
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
	/// Gets this layout's right-hand section (the one flagged `hand: right`), or null if it has none.
	const RuleInventory* getRightHand() const { return _rightHand; }
	/// Gets this layout's left-hand section (the one flagged `hand: left`), or null if it has none.
	const RuleInventory* getLeftHand() const { return _leftHand; }
	/// Gets this layout's utility section (the first INV_UTILITY section), or null if it has none.
	const RuleInventory* getUtilitySlot() const { return _utilitySlot; }
	/// Checks whether a given inventory section belongs to this layout.
	bool hasSection(const RuleInventory* section) const;
	/// Directly sets the resolved sections (used by Mod to synthesize the implicit default layout).
	void setSectionsDirectly(std::vector<const RuleInventory*> sections) { _sections = std::move(sections); resolveHands(); }
};

}
