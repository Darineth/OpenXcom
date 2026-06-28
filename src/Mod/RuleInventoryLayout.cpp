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
 * Cleans up the layout, deleting any inline sections it owns.
 */
RuleInventoryLayout::~RuleInventoryLayout()
{
	for (auto* section : _owned)
	{
		delete section;
	}
}

/**
 * Loads the layout from YAML. Supports the standard `refNode` parent mechanic
 * (the parent is loaded first, then this node's values override it). A `sections:`
 * block (when present) fully replaces the inherited section list, so a child node
 * can keep a parent's sections by omitting `sections:` or replace them by providing
 * its own.
 * @param reader YAML reader.
 */
void RuleInventoryLayout::load(const YAML::YamlNodeReader& reader)
{
	if (const auto& parent = reader["refNode"])
	{
		load(parent);
	}

	reader.tryRead("listOrder", _listOrder);

	const auto& sections = reader["sections"];
	if (sections && sections.isSeq())
	{
		// a redefinition replaces the previous section list wholesale
		for (auto* section : _owned)
		{
			delete section;
		}
		_owned.clear();
		_spec.clear();

		int order = 0;
		for (const auto& sectionReader : sections.children())
		{
			SectionSpec spec;
			const auto& refNode = sectionReader["ref"];
			if (refNode)
			{
				refNode.tryReadVal(spec.ref);
			}
			else
			{
				std::string id;
				if (!sectionReader.tryRead("id", id) || id.empty())
				{
					throw Exception("Inventory layout " + _id + " has an inline section without an 'id'");
				}
				RuleInventory* inv = new RuleInventory(id, order);
				inv->load(sectionReader);
				_owned.push_back(inv);
				spec.inlineSection = inv;
			}
			_spec.push_back(spec);
			++order;
		}
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
	_sections.reserve(_spec.size() + 1);

	bool hasGround = false;
	std::set<std::string> seen;
	for (const auto& spec : _spec)
	{
		const RuleInventory* section = spec.inlineSection;
		if (!section)
		{
			section = mod->getInventory(spec.ref, true); // throws if the ref is unknown
		}
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
