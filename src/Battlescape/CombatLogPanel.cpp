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
#include "CombatLogPanel.h"
#include "../Interface/Text.h"

namespace OpenXcom
{

/**
 * Sets up a blank combat log panel with the specified size and position.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 */
CombatLogPanel::CombatLogPanel(int width, int height, int x, int y) : Surface(width, height, x, y), _log(0), _lineHeight(9), _lastCount(0)
{
	_text = new Text(width, _lineHeight, 0, 0);
	_text->setHighContrast(true);
	_text->setAlign(ALIGN_CENTER);

	for (int i = 0; i < OUTCOME_MAX; ++i)
	{
		_colors[i] = 0;
	}

	setVisible(true);
}

/**
 * Deletes the text helper.
 */
CombatLogPanel::~CombatLogPanel()
{
	delete _text;
}

/**
 * Sets the display color for a given outcome type.
 * @param outcome Outcome type.
 * @param color Palette color index.
 */
void CombatLogPanel::setOutcomeColor(CombatLogOutcome outcome, Uint8 color)
{
	if (outcome >= 0 && outcome < OUTCOME_MAX)
	{
		_colors[outcome] = color;
	}
}

/**
 * Changes the resources needed for text rendering.
 * @param big Pointer to large-size font.
 * @param small Pointer to small-size font.
 * @param lang Pointer to current language.
 */
void CombatLogPanel::initText(Font *big, Font *small, Language *lang)
{
	_text->initText(big, small, lang);
}

/**
 * Replaces a certain amount of colors in the surface's palette.
 * @param colors Pointer to the set of colors.
 * @param firstcolor Offset of the first color to replace.
 * @param ncolors Amount of colors to replace.
 */
void CombatLogPanel::setPalette(const SDL_Color *colors, int firstcolor, int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	_text->setPalette(colors, firstcolor, ncolors);
}

/**
 * Ages out expired entries and requests a redraw whenever the log content changes.
 */
void CombatLogPanel::think()
{
	if (!_log)
	{
		return;
	}
	bool changed = _log->prune();
	if (changed || _log->getEntries().size() != _lastCount)
	{
		_lastCount = _log->getEntries().size();
		_redraw = true;
	}
}

/**
 * Draws the combat log entries top-down, newest at the bottom.
 */
void CombatLogPanel::draw()
{
	Surface::draw();
	if (!_log)
	{
		return;
	}
	int y = 0;
	for (const auto &entry : _log->getEntries())
	{
		if (y + _lineHeight > getHeight())
		{
			break;
		}
		_text->setY(y);
		_text->setText(entry.text);
		_text->setColor(_colors[entry.outcome]);
		_text->blit(this->getSurface());
		y += _lineHeight;
	}
}

}
