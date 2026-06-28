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
#include "../Engine/Surface.h"
#include "../Savegame/CombatLog.h"

namespace OpenXcom
{

class Text;
class Font;
class Language;
class CombatLog;

/**
 * Floating, centered, color-coded event log drawn over the top of the Battlescape.
 * Reads its lines from the transient CombatLog held by the SavedBattleGame and
 * fades them out over time. The newest entry is drawn at the bottom.
 */
class CombatLogPanel : public Surface
{
private:
	Text *_text;
	CombatLog *_log;
	Uint8 _colors[OUTCOME_MAX];
	int _lineHeight;
	size_t _lastRevision;
public:
	/// Creates a combat log panel with the specified size and position.
	CombatLogPanel(int width, int height, int x = 0, int y = 0);
	/// Cleans up the combat log panel.
	~CombatLogPanel();
	/// Sets the log this panel reads from.
	void setLog(CombatLog *log);
	/// Sets the display color for an outcome type.
	void setOutcomeColor(CombatLogOutcome outcome, Uint8 color);
	/// Initializes the panel's text resources.
	void initText(Font *big, Font *small, Language *lang) override;
	/// Sets the panel's palette.
	void setPalette(const SDL_Color *colors, int firstcolor = 0, int ncolors = 256) override;
	/// Ages out expired entries and flags a redraw when the log changes.
	void think() override;
	/// Draws the log lines.
	void draw() override;
};

}
