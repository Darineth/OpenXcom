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
#include "../Engine/ShaderDrawHelper.h"

namespace OpenXcom
{

class BattleUnit;
class BattleItem;
class SurfaceSet;

/**
 * A class that renders a specific unit, given its render rules
 * combining the right frames from the surfaceset.
 */
class UnitSprite : public Surface
{
private:
	BattleUnit *_unit;
	BattleItem *_itemR, *_itemL;
	SurfaceSet *_unitSurface, *_itemSurfaceR, *_itemSurfaceL;
	int _part, _animationFrame, _drawingRoutine;
	bool _helmet;
	const std::pair<Uint8, Uint8> *_color;
	int _colorSize;

	/// Drawing routine for XCom soldiers in overalls, sectoids (routine 0),
	/// mutons (routine 10),
	/// aquanauts (routine 13),
	/// aquatoids, calcinites, deep ones, gill men, lobster men, tasoths (routine 14).
	void drawRoutine0();
	/// Drawing routine for floaters.
	void drawRoutine1();
	/// Drawing routine for XCom tanks.
	void drawRoutine2();
	/// Drawing routine for cyberdiscs.
	void drawRoutine3();
	/// Drawing routine for civilians, ethereals, zombies (routine 4),
	/// tftd civilians, tftd zombies (routine 16), more tftd civilians (routine 17).
	void drawRoutine4();
	/// Drawing routine for sectopods and reapers.
	void drawRoutine5();
	/// Drawing routine for snakemen.
	void drawRoutine6();
	/// Drawing routine for chryssalid.
	void drawRoutine7();
	/// Drawing routine for silacoids.
	void drawRoutine8();
	/// Drawing routine for celatids.
	void drawRoutine9();
	/// Drawing routine for TFTD tanks.
	void drawRoutine11();
	/// Drawing routine for hallucinoids (routine 12) and biodrones (routine 15).
	void drawRoutine12();
	/// Drawing routine for tentaculats.
	void drawRoutine19();
	/// Drawing routine for triscenes.
	void drawRoutine20();
	/// Drawing routine for xarquids.
	void drawRoutine21();
	/// sort two handed sprites out.
	void sortRifles();
	/// Draw surface with changed colors.
	void drawRecolored(Surface *src);
public:
	/// Creates a new UnitSprite at the specified position and size.
	UnitSprite(int width, int height, int x, int y, bool helmet);
	/// Cleans up the UnitSprite.
	~UnitSprite();
	/// Sets surfacesets for rendering.
	void setSurfaces(SurfaceSet *unitSurface, SurfaceSet *itemSurfaceA, SurfaceSet *itemSurfaceB);
	/// Sets the battleunit to be rendered.
	void setBattleUnit(BattleUnit *unit, int part = 0);
	/// Sets the animation frame.
	void setAnimationFrame(int frame);
	/// Draws the unit.
	void draw();
};

/**
* This is scalar argument to `ShaderDraw`.
* when used in `ShaderDraw` return value of `t` to `ColorFunc::func` for every pixel
*/
class CurrentPixel
{
public:
	int x;
	int y;
	inline CurrentPixel() : x(0), y(0)
	{
	}
};

namespace helper
{
	/// implementation for current pixel type
	template<>
	struct controler<CurrentPixel>
	{
		CurrentPixel _pixel;
		int yMin, yMax, xMin, xMax;

		inline controler(const CurrentPixel& pixel) : _pixel()
		{
		}

		//cant use this function
		//inline GraphSubset get_range()

		inline void mod_range(GraphSubset&)
		{
			//nothing
		}
		inline void set_range(const GraphSubset&)
		{
			//nothing
		}

		inline void mod_y(int&, int&)
		{
			//nothing
		}
		inline void set_y(const int& begin, const int& end)
		{
			yMin = begin;
			yMax = end;
			_pixel.y = yMin;
		}
		inline void inc_y()
		{
			++_pixel.y;
			if (_pixel.y > yMax) _pixel.y = yMin;
		}


		inline void mod_x(int&, int&)
		{
			//nothing
		}
		inline void set_x(const int& begin, const int& end)
		{
			xMin = begin;
			xMax = end;
			_pixel.x = xMin;
		}
		inline void inc_x()
		{
			++_pixel.x;
			if (_pixel.x > xMax) _pixel.x = xMin;
		}

		inline CurrentPixel& get_ref()
		{
			return _pixel;
		}
	};
}

static inline CurrentPixel ShaderCurrentPixel()
{
	return CurrentPixel();
}

struct Recolor
{
	static const Uint8 ColorGroup = 15 << 4;
	static const Uint8 ColorShade = 15;

	static inline bool loop(Uint8& dest, const Uint8& src, const std::pair<Uint8, Uint8>& face_color)
	{
		if ((src & ColorGroup) == face_color.first)
		{
			switch (face_color.second & ColorShade)
			{
			case 1:
				switch (src & ColorShade)
				{
				case 0:
				case 1:
					dest = 0;
					break;
				case 2:
				case 3:
					dest = 1;
					break;
				case 4:
				case 5:
					dest = 2;
					break;
				case 6:
				case 7:
					dest = 3;
					break;
				default:
					dest = (src & ColorShade) - 4;
				}

				dest = (face_color.second & ColorGroup) ? (dest + (face_color.second & ColorGroup)) : dest + 1;
				break;
			case 15:
				dest = (face_color.second & ColorGroup) + ((src & ColorShade) >> 1) + 8;
				break;
			default:
				dest = face_color.second + (src & ColorShade);
				dest = dest > 0 ? dest : 1;
			}
			return true;
		}
		else
		{
			return false;
		}
	}

	static inline void func(Uint8& dest, const Uint8& src, const std::pair<Uint8, Uint8> *color, int size, const int&)
	{
		if (src)
		{
			for (int i = 0; i < size; ++i)
			{
				if (loop(dest, src, color[i]))
				{
					return;
				}
			}
			dest = src;
		}
	}
};

struct RecolorStealth
{
	static inline void func(Uint8& dest, const Uint8& src, const std::pair<Uint8, Uint8> *color, int size, CurrentPixel &pixel)
	{
		if ((pixel.y) % 2 == 1)
		{
			dest = 0;
		}
		else if (src)
		{
			for (int i = 0; i < size; ++i)
			{
				if (Recolor::loop(dest, src, color[i]))
				{
					return;
				}
			}
			dest = src;
		}
	}
};

}
