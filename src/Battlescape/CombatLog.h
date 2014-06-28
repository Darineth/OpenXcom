/*
 * Copyright 2010-2014 OpenXcom Developers.
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
#ifndef OPENXCOM_COMBATLOG_H
#define OPENXCOM_COMBATLOG_H

#include "../Engine/Surface.h"

namespace OpenXcom
{
enum CombatLogColor { COMBAT_LOG_WHITE, COMBAT_LOG_RED, COMBAT_LOG_GREEN, COMBAT_LOG_BLUE, COMBAT_LOG_ORANGE };

class Text;
class Timer;
class Font;

/**
 * Coloured box with text inside that fades out after it is displayed.
 * Used to display warning/error messages on the Battlescape.
 */
class CombatLog : public Surface
{
private:
	Text *_text[10];
	Timer *_timer;
	Uint8 _fade, _currentLine;

	static const Uint8 MAX_LOG_LINES = 10;

	/// Shifts log lines up a slot.
	void shiftUp();
	/// Expire old messages.
	void hideOldLines();

public:
	/// Creates a new warning message with the specified size and position.
	CombatLog(int width, int height, int x = 0, int y = 0);
	/// Cleans up the warning message.
	~CombatLog();

	/// Clears all log entries.
	void clear();

	/// Initializes the warning message's resources.
	void initText(Font *big, Font *small, Language *lang);
	/// Sets the warning message's palette.
	void setPalette(SDL_Color *colors, int firstcolor = 0, int ncolors = 256);
	/// Adds the messsage to the log.
	void log(const std::wstring &msg, CombatLogColor color);
	/// Handles the timers.
	void think();
	/// Draws the message.
	void draw();
};

}

#endif
