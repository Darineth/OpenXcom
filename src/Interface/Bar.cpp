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
#include "Bar.h"
#include <SDL.h>

namespace OpenXcom
{

/**
 * Sets up a blank bar with the specified size and position.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 */
Bar::Bar(int width, int height, int x, int y) : Surface(width, height, x, y), _color(0), _color2(0), _color3(0), _color4(0), _borderColor(0), _scale(0), _max(0), _value(0), _value2(0), _value3(0), _secondOnTop(true), _autoScale(false), _fullBorder(false), _bordered(true)
{

}

/**
 *
 */
Bar::~Bar()
{
}

/**
 * Changes the color used to draw the border and contents.
 * @param color Color value.
 */
void Bar::setColor(Uint8 color)
{
	_color = color;
	_redraw = true;
}

/**
 * Returns the color used to draw the bar.
 * @return Color value.
 */
Uint8 Bar::getColor() const
{
	return _color;
}

/**
 * Changes the color used to draw the second contents.
 * @param color Color value.
 */
void Bar::setSecondaryColor(Uint8 color)
{
	_color2 = color;
	_redraw = true;
}

/**
 * Returns the second color used to draw the bar.
 * @return Color value.
 */
Uint8 Bar::getSecondaryColor() const
{
	return _color2;
}

/**
 * Changes the color used to draw the third contents.
 * @param color Color value.
 */
void Bar::setTertiaryColor(Uint8 color)
{
	_color3 = color;
	_redraw = true;
}

/**
 * Returns the third color used to draw the bar.
 * @return Color value.
 */
Uint8 Bar::getTertiaryColor() const
{
	return _color3;
}

/**
 * Changes the color used to draw the third contents.
 * @param color Color value.
 */
void Bar::setQuaternaryColor(Uint8 color)
{
	setColorBackground(color);
}

/**
 * Returns the third color used to draw the bar.
 * @return Color value.
 */
Uint8 Bar::getQuaternaryColor() const
{
	return getColorBackground();
}

/**
* Changes the color used to draw the third contents.
* @param color Color value.
*/
void Bar::setColorBackground(Uint8 color)
{
	_color4 = color;
	_redraw = true;
}

/**
* Returns the third color used to draw the bar.
* @return Color value.
*/
Uint8 Bar::getColorBackground() const
{
	return _color4;
}

/**
 * Changes the scale factor used to draw the bar values.
 * @param scale Scale in pixels/unit.
 */
void Bar::setScale(double scale)
{
	_scale = scale;
	_redraw = true;
}

/**
 * Returns the scale factor used to draw the bar values.
 * @return Scale in pixels/unit.
 */
double Bar::getScale() const
{
	return _scale;
}

/**
 * Sets if the bar has a border.
 */
void Bar::setBordered(bool bordered)
{
	_bordered = bordered;
	_redraw = true;
}

/**
 * Gets if the bar has a border.
 */
bool Bar::getBordered() const
{
	return _bordered;
}

/**
 * Sets if the bar has a full border.
 */
void Bar::setFullBordered(bool fullBorder)
{
	_fullBorder = fullBorder;
	_redraw = true;
}

/**
 * Gets if the bar has a full border.
 */
bool Bar::getFullBordered() const
{
	return _fullBorder;
}

/**
 * Changes the maximum value used to draw the outer border.
 * @param max Maximum value.
 */
void Bar::setMax(double max)
{
	_max = max;
	_redraw = true;

	if (_autoScale)
	{
		determineAutoScale();
	}
}

/**
 * Returns the maximum value used to draw the outer border.
 * @return Maximum value.
 */
double Bar::getMax() const
{
	return _max;
}

/**
 * Changes the value used to draw the inner contents.
 * @param value Current value.
 */
void Bar::setValue(double value)
{
	_value = (value < 0.0) ? 0.0 : value;
	_redraw = true;
}

/**
 * Returns the value used to draw the inner contents.
 * @return Current value.
 */
double Bar::getValue() const
{
	return _value;
}

/**
 * Changes the value used to draw the second inner contents.
 * @param value Current value.
 */
void Bar::setValue2(double value)
{
	_value2 = (value < 0.0) ? 0.0 : value;
	_redraw = true;
}

/**
 * Returns the value used to draw the second inner contents.
 * @return Current value.
 */
double Bar::getValue2() const
{
	return _value2;
}

/**
 * Changes the value used to draw the third inner contents.
 * @param value Current value.
 */
void Bar::setValue3(double value)
{
	_value3 = value;
	_redraw = true;
}

/**
 * Returns the value used to draw the third inner contents.
 * @return Current value.
 */
double Bar::getValue3() const
{
	return _value3;
}

/**
 * Defines whether the second value should be drawn on top. Default this is true.
 * @param onTop Second value on top?
 */
void Bar::setSecondValueOnTop(bool onTop)
{
	_secondOnTop = onTop;
}

/**
 * Draws the bordered bar filled according
 * to its values.
 */
void Bar::draw()
{
	Surface::draw();
	SDL_Rect square;

	square.x = 0;
	square.y = 0;
	if (_autoScale)
	{
		square.w = getWidth();
		if (_bordered)
		{
			if (_fullBorder)
			{
				_scale = (square.w - 2) / _max;
			}
			else
			{
				_scale = (square.w - 1) / _max;
			}
		}
		else
		{
			_scale = square.w / _max;
		}
	}
	else
	{
		square.w = (Uint16)(_scale * _max) + 1;
	}
	square.h = getHeight();

	if (_bordered)
	{
		if (_borderColor)
			drawRect(&square, _borderColor);
		else
			drawRect(&square, _color + 4);

		if (_fullBorder)
		{
			square.y++;
			square.x++;
			square.w -= 2;
			square.h -= 2;
		}
		else
		{
			square.y++;
			square.w--;
			square.h -= 2;
		}
	}
	else
	{
		if (!_color4)
		{
			_color4 = _color + 6;
		}
	}

	drawRect(&square, _color4);

	if (_secondOnTop)
	{
		square.w = (Uint16)ceil(_scale * _value);
		drawRect(&square, _color);
		square.w = (Uint16)ceil(_scale * _value2);
		drawRect(&square, _color2);
	}
	else
	{
		square.w = (Uint16)ceil(_scale * _value2);
		drawRect(&square, _color2);
		square.w = (Uint16)ceil(_scale * _value);
		drawRect(&square, _color);
	}

	if (_value3)
	{
		square.h += 2;
		square.y--;
		square.w = 1;

		for (Uint16 ii = 0; ii < _value3; ++ii)
		{
			//square.w = (Uint16)(_scale * _value3);
			square.x = (Uint16)(_scale * ii * 2);
			drawRect(&square, _color3);
		}
	}
}

/**
 * sets the border color for the bar.
 * @param bc the color for the outline of the bar.
 * @note will use base colour + 4 if none is defined here.
 */
void Bar::setBorderColor(Uint8 bc)
{
	_borderColor = bc;
}

/// Sets auto scale mode.
void Bar::setAutoScale(bool autoScale)
{
	_autoScale = autoScale;
	if (_autoScale)
	{
		determineAutoScale();
	}
	_redraw = true;
}

/// Gets if auto scale mode is enabled.
bool Bar::getAutoScale() const
{
	return _autoScale;
}

void Bar::determineAutoScale()
{
	_scale = getWidth() / _max;
}


}
