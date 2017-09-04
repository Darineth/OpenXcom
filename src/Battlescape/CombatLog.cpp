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
#include "CombatLog.h"
#include <SDL.h>
#include <string>
#include "../Engine/Timer.h"
#include "../Interface/Text.h"
#include "../Engine/Palette.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleInterface.h"

namespace OpenXcom
{

/**
 * Sets up a blank warning message with the specified size and position.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 */
	CombatLog::CombatLog(int width, int height, int x, int y) : Surface(width, height, x, y), _fade(0), _currentLine(0), _maxLines(height / 10), _colorWhite(0), _colorRed(32), _colorGreen(64), _colorBlue(128), _colorOrange(16)
{
	RuleInterface *combatLog = Game::getMod()->getInterface("combatLog");
	if(Element* combatLogText = combatLog ? combatLog->getElement("text") : 0)
	{
		_colorWhite = combatLogText->border;
		_colorRed = combatLogText->color;
		_colorGreen = combatLogText->color2;
		_colorBlue = combatLogText->color3;
		_colorOrange = combatLogText->color4;
	}

	_text = new Text*[_maxLines];
	for(int ii = 0; ii < _maxLines; ++ii)
	{
		Text *text = _text[ii] = new Text(width, 9, x, y + ii * 10);
		text->setHighContrast(true);
		text->setAlign(ALIGN_CENTER);
		text->setSecondaryColor(Palette::blockOffset(1));
	}

	_timer = new Timer(5000);
	_timer->onTimer((SurfaceHandler)&CombatLog::hideOldLines);

	setVisible(false);
}

/**
 * Deletes timers.
 */
CombatLog::~CombatLog()
{
	delete _timer;
	for(int ii = 0; ii < _maxLines; ++ii)
	{
		delete _text[ii];
	}

	delete _text;
}

void CombatLog::initText(Font *big, Font *small, Language *lang)
{
	for(int ii = 0; ii < _maxLines; ++ii)
	{
		_text[ii]->initText(big, small, lang);
	}
}

void CombatLog::clear()
{
	if(_currentLine)
	{
		for(int ii = 0; ii < _currentLine; ++ii)
		{
			_text[ii]->clear();
		}
		setVisible(false);
		_timer->stop();
		_currentLine = 0;
	}
}

void CombatLog::shiftUp()
{
	if(_currentLine > 0)
	{
		if(_currentLine > 1)
		{
			for(int ii = 1; ii < _currentLine; ++ii)
			{
				Text* top = _text[ii - 1];
				Text* bottom = _text[ii];

				top->setText(bottom->getText());
				top->setColor(bottom->getColor());
				top->setSecondaryColor(_colorOrange);
			}
		}

		_redraw = true;
		_text[--_currentLine]->clear();
	}
}

void CombatLog::setPalette(SDL_Color *colors, int firstcolor, int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	for(int ii = 0; ii < _maxLines; ++ii)
	{
		_text[ii]->setPalette(colors, firstcolor, ncolors);
	}
}

void CombatLog::log(const std::wstring &msg, CombatLogColor color)
{
	if(_currentLine == _maxLines)
	{
		shiftUp();
	}

	Text* text = _text[_currentLine++];
	text->setText(msg);

	switch(color)
	{
	case COMBAT_LOG_RED:
		text->setColor(_colorRed);
		break;
	case COMBAT_LOG_GREEN:
		text->setColor(_colorGreen);
		break;
	case COMBAT_LOG_BLUE:
		text->setColor(_colorBlue);
		break;
	case COMBAT_LOG_ORANGE:
		text->setColor(_colorOrange);
		break;

	default:
		text->setColor(_colorWhite);
	}

	text->setSecondaryColor(_colorOrange);

	_redraw = true;
	setVisible(true);
	if(!_timer->isRunning())
	{
		_timer->start();
	}
}

void CombatLog::think()
{
	_timer->think(0, this);
}

void CombatLog::hideOldLines()
{
	if(_currentLine)
	{
		shiftUp();

		if(!_currentLine)
		{
			setVisible(false);
			_timer->stop();
		}
	}
}

void CombatLog::draw()
{
	Surface::draw();
	//drawRect(0, 0, getWidth(), getHeight(), _color + (_fade > 24 ? 12 : (int)(_fade / 2)));
	for(int ii = 0; ii < _currentLine; ++ii)
	{
		_text[ii]->blit(this);
	}
}

}
