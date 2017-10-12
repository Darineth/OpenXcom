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
#include "ShaderDrawHelper.h"
#include "HelperMeta.h"
#include <tuple>

namespace OpenXcom
{

/**
 * Universal blit function implementation.
 * @param f called function.
 * @param src source surfaces control objects.
 */
template<typename Func, typename... SrcType>
static inline void ShaderDrawImpl(Func&& f, helper::controler<SrcType>... src)
{
	//get basic draw range in 2d space
	GraphSubset end_temp = std::get<0>(std::tie(src...)).get_range();

	//intersections with src ranges
	(void)helper::DummySeq
	{
		(src.mod_range(end_temp), 0)...
	};

	const GraphSubset end = end_temp;
	if (end.size_x() == 0 || end.size_y() == 0)
		return;
	//set final draw range in 2d space
	(void)helper::DummySeq
	{
		(src.set_range(end), 0)...
	};


	int begin_y = 0, end_y = end.size_y();
	//determining iteration range in y-axis
	(void)helper::DummySeq
	{
		(src.mod_y(begin_y, end_y), 0)...
	};
	if(begin_y>=end_y)
		return;
	//set final iteration range
	(void)helper::DummySeq
	{
		(src.set_y(begin_y, end_y), 0)...
	};

	//iteration on y-axis
	for (int y = end_y-begin_y; y>0; --y, (void)helper::DummySeq{ (src.inc_y(), 0)... })
	{
		int begin_x = 0, end_x = end.size_x();
		//determining iteration range in x-axis
		(void)helper::DummySeq
		{
			(src.mod_x(begin_x, end_x), 0)...
		};
		if (begin_x>=end_x)
			continue;
		//set final iteration range
		(void)helper::DummySeq
		{
			(src.set_x(begin_x, end_x), 0)...
		};

		//iteration on x-axis
		for (int x = end_x-begin_x; x>0; --x, (void)helper::DummySeq{ (src.inc_x(), 0)... })
		{
			f(src.get_ref()...);
		}
	}

};

/**
 * Universal blit function.
 * @tparam ColorFunc class that contains static function `func`.
 * function is used to modify these arguments.
 * @param src_frame destination and source surfaces modified by function.
 */
template<typename ColorFunc, typename... SrcType>
static inline void ShaderDraw(const SrcType&... src_frame)
{
	ShaderDrawImpl(ColorFunc::func, helper::controler<SrcType>(src_frame)...);
}

/**
 * Universal blit function.
 * @param f function that modify other arguments.
 * @param src_frame destination and source surfaces modified by function.
 */
template<typename Func, typename... SrcType>
static inline void ShaderDrawFunc(Func&& f, const SrcType&... src_frame)
{
	ShaderDrawImpl(std::forward<Func>(f), helper::controler<SrcType>(src_frame)...);
}

namespace helper
{

const Uint8 ColorGroup = 0xF0;
const Uint8 ColorShade = 0x0F;

/**
 * help class used for Surface::blitNShade
 */
struct ColorReplace
{
	/**
	* Function used by ShaderDraw in Surface::blitNShade
	* set shade and replace color in that surface
	* @param dest destination pixel
	* @param src source pixel
	* @param shade value of shade of this surface
	* @param newColor new color to set (it should be offseted by 4)
	*/
	static inline void func(Uint8& dest, const Uint8& src, const int& shade, const int& newColor)
	{
		if (src)
		{
			const int newShade = (src & ColorShade) + shade;
			if (newShade > ColorShade)
				// so dark it would flip over to another color - make it black instead
				dest = ColorShade;
			else
				dest = newColor | newShade;
		}
	}

};

/**
 * help class used for Surface::blitNShade
 */
struct StandardShade
{
	/**
	* Function used by ShaderDraw in Surface::blitNShade
	* set shade
	* @param dest destination pixel
	* @param src source pixel
	* @param shade value of shade of this surface
	* @param notused
	* @param notused
	*/
	static inline void func(Uint8& dest, const Uint8& src, const int& shade)
	{
		if (src)
		{
			const int newShade = (src & ColorShade) + shade;
			if (newShade > ColorShade)
				// so dark it would flip over to another color - make it black instead
				dest = ColorShade;
			else
				dest = (src & ColorGroup) | newShade;
		}
	}

};
/**
 * helper class used for bliting dying unit with overkill
 */
struct BurnShade
{
	static inline void func(Uint8& dest, const Uint8& src, const int& burn, const int& shade)
	{
		if (src)
		{
			if (burn)
			{
				const Uint8 tempBurn = (src & ColorShade) + burn;
				if (tempBurn > 26)
				{
					//nothing
				}
				else if (tempBurn > 15)
				{
					StandardShade::func(dest, ColorShade, shade);
				}
				else
				{
					StandardShade::func(dest, (src & ColorGroup) + tempBurn, shade);
				}
			}
			else
			{
				StandardShade::func(dest, src, shade);
			}
		}
	}
};

}//namespace helper

template<typename T>
static inline helper::Scalar<T> ShaderScalar(T& t)
{
	return helper::Scalar<T>(t);
}
template<typename T>
static inline helper::Scalar<const T> ShaderScalar(const T& t)
{
	return helper::Scalar<const T>(t);
}

/**
 * Create warper from vector
 * @param s vector
 * @return
 */
template<typename Pixel>
inline helper::ShaderBase<Pixel> ShaderSurface(std::vector<Pixel>& s, int max_x, int max_y)
{
	return helper::ShaderBase<Pixel>(s, max_x, max_y);
}

/**
 * Create warper from array
 * @param s array
 * @return
 */
template<typename Pixel, int Size>
inline helper::ShaderBase<Pixel> ShaderSurface(Pixel(&s)[Size], int max_x, int max_y)
{
	return helper::ShaderBase<Pixel>(s, max_x, max_y, max_x*sizeof(Pixel));
}

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

	static inline void func(Uint8& dest, const Uint8& src, const std::pair<Uint8, Uint8> *color, int size)
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

}//namespace OpenXcom
