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
#include "../Engine/Surface.h"

namespace OpenXcom
{

/**
 * Bar graphic that represents a certain value.
 * Drawn with a coloured border and partially
 * filled content to contrast two values, typically
 * used for showing base and soldier stats.
 */
class Bar : public Surface
{
private:
	Uint8 _color, _color2, _color3, _color4, _borderColor;
	double _scale, _max, _value, _value2, _value3;
	bool _secondOnTop, _autoScale, _bordered, _fullBorder;

	void determineAutoScale();

public:
	/// Creates a new bar with the specified size and position.
	Bar(int width, int height, int x = 0, int y = 0);
	/// Cleans up the bar.
	~Bar();
	/// Sets the bar's color.
	void setColor(Uint8 color);
	/// Gets the bar's color.
	Uint8 getColor() const;
	/// Sets the bar's second color.
	void setSecondaryColor(Uint8 color);
	/// Gets the bar's second color.
	Uint8 getSecondaryColor() const;
	/// Sets the bar's third color.
	void setTertiaryColor(Uint8 color) override;
	/// Gets the bar's third color.
	Uint8 getTertiaryColor() const;
	/// Sets the bar's fourth color.
	void setQuaternaryColor(Uint8 color) override;
	/// Gets the bar's fourth color.
	Uint8 getQuaternaryColor() const;
	/// Sets the bar's background color.
	void setColorBackground(Uint8 color);
	/// Gets the bar's background color.
	Uint8 getColorBackground() const;
	/// Sets the bar's scale.
	void setScale(double scale);
	/// Gets the bar's scale.
	double getScale() const;
	/// Sets if the bar has a border.
	void setBordered(bool bordered);
	/// Gets if the bar has a border.
	bool getBordered() const;
	/// Sets if the bar has a full border.
	void setFullBordered(bool fullBorder);
	/// Gets if the bar has a full border.
	bool getFullBordered() const;
	/// Sets the bar's maximum value.
	void setMax(double max);
	/// Gets the bar's maximum value.
	double getMax() const;
	/// Sets the bar's current value.
	void setValue(double value);
	/// Gets the bar's current value.
	double getValue() const;
	/// Sets the bar's second current value.
	void setValue2(double value);
	/// Gets the bar's second current value.
	double getValue2() const;
	/// Sets the bar's third current value.
	void setValue3(double value);
	/// Gets the bar's third current value.
	double getValue3() const;
	/// Defines whether the second value should be drawn on top.
	void setSecondValueOnTop(bool onTop);
	/// Draws the bar.
	void draw();
	/// set the outline color for the bar.
	void setBorderColor(Uint8 bc);
	/// Sets auto scale mode.
	void setAutoScale(bool autoScale);
	/// Gets if auto scale mode is enabled.
	bool getAutoScale() const;
};

}
