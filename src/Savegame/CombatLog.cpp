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
#include "CombatLog.h"
#include <SDL.h>

namespace OpenXcom
{

/**
 * Creates a new, empty combat log.
 * @param maxEntries Maximum number of entries kept (older ones scroll off the top).
 * @param lifetime How long, in milliseconds, an entry stays before fading off.
 */
CombatLog::CombatLog(size_t maxEntries, unsigned int lifetime) : _maxEntries(maxEntries), _lifetime(lifetime)
{
}

/**
 * Appends an entry to the log, capping the total so the oldest scrolls off.
 * @param text Already-localized display string.
 * @param outcome Outcome tone used to color the entry.
 */
void CombatLog::add(const std::string &text, CombatLogOutcome outcome)
{
	_entries.push_back(CombatLogEntry{ text, outcome, SDL_GetTicks() });
	while (_entries.size() > _maxEntries)
	{
		_entries.pop_front();
	}
}

/**
 * Removes entries that have outlived their lifetime. Entries are stored oldest-first,
 * so expired ones are always at the front.
 * @return True if at least one entry was removed.
 */
bool CombatLog::prune()
{
	bool changed = false;
	Uint32 now = SDL_GetTicks();
	while (!_entries.empty() && (now - _entries.front().born) >= _lifetime)
	{
		_entries.pop_front();
		changed = true;
	}
	return changed;
}

/**
 * Clears the log entirely.
 */
void CombatLog::clear()
{
	_entries.clear();
}

}
