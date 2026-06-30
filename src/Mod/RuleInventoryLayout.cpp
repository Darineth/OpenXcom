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
#include "RuleInventoryLayout.h"
#include <set>
#include "Mod.h"
#include "RuleInventory.h"
#include "../Engine/Exception.h"

namespace OpenXcom
{

/**
 * Creates a blank inventory layout.
 * @param id String defining the id.
 * @param listOrder The list weight for this layout.
 */
RuleInventoryLayout::RuleInventoryLayout(const std::string& id, int listOrder): _id(id), _listOrder(listOrder)
{
}

/**
 * Cleans up the layout.
 */
RuleInventoryLayout::~RuleInventoryLayout()
{
}

/**
 * Loads the layout from YAML. Supports the standard `refNode` parent mechanic
 * (the parent is loaded first, then this node's values override it). An `invs:`
 * list (when present) fully replaces the inherited section list, so a child node
 * can keep a parent's sections by omitting `invs:` or replace them by providing
 * its own.
 *
 * A layout is just an ordered list of global `invs` section ids:
 *   invs: [STR_RIGHT_HAND, STR_LEFT_HAND, STR_BELT, STR_GROUND]
 * To restrict a section to particular layouts, define it as a normal global `invs:`
 * section and only list it in the layouts that should have it.
 * @param reader YAML reader.
 */
void RuleInventoryLayout::load(const YAML::YamlNodeReader& reader)
{
	if (const auto& parent = reader["refNode"])
	{
		load(parent);
	}

	reader.tryRead("listOrder", _listOrder);

	if (reader["sections"])
	{
		throw Exception("Inventory layout " + _id + " uses the obsolete `sections:` block; list the "
			"global section ids directly with `invs: [STR_RIGHT_HAND, ...]` instead.");
	}

	// an `invs:` list redefinition replaces the previous section list wholesale
	if (const auto& invs = reader["invs"])
	{
		_refs.clear();
		invs.tryReadVal(_refs);
	}
}

/**
 * Resolves section references against the global inventory set, validates the
 * resulting list, and guarantees a ground section so items can always be dropped.
 * @param mod The mod.
 */
void RuleInventoryLayout::afterLoad(const Mod* mod)
{
	_sections.clear();
	_sections.reserve(_refs.size() + 1);

	bool hasGround = false;
	std::set<std::string> seen;
	for (const auto& ref : _refs)
	{
		const RuleInventory* section = mod->getInventory(ref, true); // throws if the ref is unknown
		if (!section)
		{
			continue;
		}
		if (!seen.insert(section->getId()).second)
		{
			throw Exception("Inventory layout " + _id + " contains duplicate section " + section->getId());
		}
		if (section->getType() == INV_GROUND)
		{
			hasGround = true;
		}
		_sections.push_back(section);
	}

	if (!hasGround)
	{
		// every layout needs a ground section; fall back to the global one
		const RuleInventory* ground = mod->getInventoryGround();
		if (ground)
		{
			_sections.push_back(ground);
		}
	}

	resolveHands();
}

/**
 * Resolves this layout's hand sections from its resolved section list, by the `hand:` property.
 * Handedness is unique per layout: two sections of the same side in one layout is a ruleset error
 * (the same `hand: left` section is fine in many different layouts, just not twice in one).
 */
void RuleInventoryLayout::resolveHands()
{
	_rightHand = nullptr;
	_leftHand = nullptr;
	for (const auto* s : _sections)
	{
		if (s->isRightHand())
		{
			if (_rightHand)
				throw Exception("Inventory layout " + _id + " has two right-hand sections (" + _rightHand->getId() + ", " + s->getId() + "); only one is allowed per layout.");
			_rightHand = s;
		}
		if (s->isLeftHand())
		{
			if (_leftHand)
				throw Exception("Inventory layout " + _id + " has two left-hand sections (" + _leftHand->getId() + ", " + s->getId() + "); only one is allowed per layout.");
			_leftHand = s;
		}
	}
}

/**
 * Checks whether a section belongs to this layout. Section identity is by pointer:
 * `ref:` entries share the global RuleInventory pointer, inline ones are unique to
 * the layout, so pointer comparison is exact.
 * @param section The section to test.
 * @return True if the section is part of this layout.
 */
bool RuleInventoryLayout::hasSection(const RuleInventory* section) const
{
	for (const auto* s : _sections)
	{
		if (s == section)
		{
			return true;
		}
	}
	return false;
}

}
