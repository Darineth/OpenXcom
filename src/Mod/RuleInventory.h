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
#include <map>
#include "../Engine/Yaml.h"

namespace OpenXcom
{

struct RuleSlot
{
	int x, y;
};

// Append-only: INV_SLOT/HAND/GROUND must stay 0/1/2 (saved games and rulesets use the
// ordinals via `type:`). INV_UTILITY/INV_EQUIP are single-occupant "hand-family" slots.
enum InventoryType
{
	// An inventory slot that can hold multiple items, and is drawn as a grid of slots.
	INV_SLOT,
	// A hand slot that can hold a single item, typically a weapon.  Displayed on the battlescape as an interactible item.
	INV_HAND,
	// A ground slot that can hold multiple items, and is drawn as a grid of slots.  Not unit-associated.
	INV_GROUND, 
	// A slot that can hold a single item, typically a utility item.  Displayed on the battlescape as an interactible item.
	INV_UTILITY,
	// A slot that can hold a single item, typically an equipment item.  Intended for equippable "gear" such as night vision goggles.
	INV_EQUIP 
};

class RuleItem;
class ScriptParserBase;

/**
 * Represents a specific section of the inventory,
 * containing information like available slots and
 * screen position.
 */
class RuleInventory
{
private:
	std::string _id;
	int _x, _y;
	InventoryType _type;
	/// Rule-defined box size (in slot cells) for utility/equip slots; 0 = use the type's default constant.
	int _width, _height;
	std::vector<RuleSlot> _slots;
	std::map<std::string, int> _costs;
	int _listOrder;
	int _hand;
	/// Slot-side item filter: stored as BattleType (BT_NONE = accept anything). Kept as int so this
	/// widely-included header doesn't have to pull in the heavy RuleItem.h just for the enum.
	int _battleType;
	bool _allowCombatSwap;
	bool _countStats;
	/// Slot-declared armor facing: an item's directional armor lands on this side only.
	/// Stored as a UnitSide ordinal, -1 = unset (the item's own per-side values apply as declared).
	/// Kept as int so this widely-included header doesn't have to pull in Unit.h just for the enum.
	int _armorSide;
public:
	static const int SLOT_W = 16;
	static const int SLOT_H = 16;
	/// Fallback TU cost for moving into a section with no explicit `costs` entry (custom-layout sections).
	static const int DEFAULT_MOVE_COST = 8;
	static const int HAND_W = 2;
	static const int HAND_H = 3;
	/// Default box size for utility/equip slots when the rule sets no `width`/`height`.
	static const int UTILITY_W = 2;
	static const int UTILITY_H = 2;
	static const int EQUIP_W = 2;
	static const int EQUIP_H = 2;
	static const int PAPERDOLL_W = 40;
	static const int PAPERDOLL_H = 70;
	static const int PAPERDOLL_X = 60;
	static const int PAPERDOLL_Y = 65;

	/// Name of class used in script.
	static constexpr const char *ScriptName = "RuleInventory";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);

	/// Creates a blank inventory ruleset.
	RuleInventory(const std::string &id, int listOrder);
	/// Cleans up the inventory ruleset.
	~RuleInventory();
	/// Loads inventory data from YAML.
	void load(const YAML::YamlNodeReader& reader);
	/// Gets the inventory's id.
	const std::string& getId() const;
	/// Gets the X position of the inventory.
	int getX() const;
	/// Gets the Y position of the inventory.
	int getY() const;
	/// Gets the inventory type.
	InventoryType getType() const;
	/// Gets whether this section holds a single item (hand/utility/equip): one occupant, drawn
	/// as a single bounding box rather than an N×M slot grid. Hands accept any item size; utility/
	/// equip size-check the item against their box dimensions (see fitItemInSlot).
	bool isSingleItem() const;
	/// Gets the bounding-box width (in slot cells) for a single-item slot; 0 for slot/ground.
	/// Hands are fixed; utility/equip use the rule's `width` (defaulting to the type constant).
	int getBoxWidth() const;
	/// Gets the bounding-box height (in slot cells) for a single-item slot; 0 for slot/ground.
	/// Hands are fixed; utility/equip use the rule's `height` (defaulting to the type constant).
	int getBoxHeight() const;
	/// Gets if this slot is right hand;
	bool isRightHand() const;
	/// Gets if this slot is left hand;
	bool isLeftHand() const;
	/// Gets all the slots in the inventory.
	const std::vector<struct RuleSlot> *getSlots() const;
	/// Checks for a slot in a certain position.
	bool checkSlotInPosition(int *x, int *y) const;
	/// Checks if an item fits in a slot.
	bool fitItemInSlot(const RuleItem *item, int x, int y) const;
	/// Gets a certain cost in the inventory.
	int getCost(const RuleInventory *slot) const;
	/// Gets the battle type this slot is restricted to (BT_NONE = unrestricted).
	int getBattleType() const;
	/// Checks whether an item's battle type is accepted by this slot's `battleType` filter.
	bool canAcceptBattleType(const RuleItem *item) const;
	/// Gets whether items can be moved into/out of this slot once combat is underway.
	bool getAllowCombatSwap() const;
	/// Gets whether items in this slot contribute their stat bonuses to the wearer.
	bool getCountStats() const;
	/// Gets the armor facing this slot reinforces as a UnitSide ordinal (-1 = unset).
	int getArmorSide() const { return _armorSide; }
	int getListOrder() const;
};

// helper overloads for deserialization-only
bool read(ryml::ConstNodeRef const& n, RuleSlot* val);

}
