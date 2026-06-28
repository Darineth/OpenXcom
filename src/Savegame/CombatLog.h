#pragma once
/*
 * Copyright 2010-2024 OpenXcom Developers.
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
#include <deque>

namespace OpenXcom
{

/**
 * The "tone" of a combat log entry, used to pick its display color.
 * Outcomes are judged from the player's perspective (e.g. an enemy going down
 * is GOOD, one of our own getting hurt is BAD).
 */
enum CombatLogOutcome : int
{
	OUTCOME_NEUTRAL,
	OUTCOME_GOOD,
	OUTCOME_WARNING,
	OUTCOME_BAD,
	OUTCOME_MAX
};

/**
 * A single line in the floating combat log: an already-localized string plus the
 * outcome that colors it and the time it was added (for fade-out).
 */
struct CombatLogEntry
{
	std::string text;
	CombatLogOutcome outcome;
	unsigned int born; ///< SDL ticks (ms) when the entry was added.
};

/**
 * Transient, in-battle event log shown floating over the Battlescape.
 *
 * This is DX-specific and deliberately NOT serialized: it lives only for the
 * duration of a battle and self-empties over time. Combat code pushes
 * already-localized lines into it; the BattlescapeState's panel reads them back.
 * Distinct from the on-demand OXCE HitLog (which is a per-turn text blob).
 */
class CombatLog
{
private:
	std::deque<CombatLogEntry> _entries;
	size_t _maxEntries;
public:
	/// Creates a combat log with the given visible cap (entry lifetime comes from the live option).
	CombatLog(size_t maxEntries = 20);
	/// Adds an already-localized entry with the given outcome.
	void add(const std::string &text, CombatLogOutcome outcome);
	/// Sets the maximum number of entries kept (older ones are pruned on add).
	void setMaxEntries(size_t maxEntries);
	/// Drops entries older than their lifetime; returns true if anything was removed.
	bool prune();
	/// Removes all entries.
	void clear();
	/// Gets the current entries, oldest first.
	const std::deque<CombatLogEntry> &getEntries() const { return _entries; }
};

}
