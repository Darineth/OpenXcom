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
#include <tuple>

namespace OpenXcom
{

template<typename First, typename... Rest>
static inline First&& GetFirst(First&& f, Rest&&... r)
{
	return std::forward<First>(f);
}

/**
 * Universal blit function implementation.
 * @param f called function.
 * @param src source surfaces control objects.
 */
template<typename Func, typename... SrcType>
static inline void ShaderDrawImpl(Func&& f, helper::controler<SrcType>... src)
{
	//get basic draw range in 2d space
	GraphSubset end_temp = GetFirst(src...).get_range();

	//intersections with src ranges
	(src.mod_range(end_temp), ...);

	const GraphSubset end = end_temp;
	if (!end)
		return;

	//set final draw range in 2d space
	(src.set_range(end), ...);


	int begin_y = 0, end_y = end.size_y();

	//determining iteration range in y-axis
	(src.mod_y(begin_y, end_y), ...);

	if(begin_y>=end_y)
		return;

	//set final iteration range
	(src.set_y(begin_y, end_y), ...);

	//iteration on y-axis
	for (int y = end_y-begin_y; y>0; --y, (src.inc_y(), ...))
	{
		int begin_x = 0, end_x = end.size_x();

		//determining iteration range in x-axis
		(src.mod_x(begin_x, end_x), ...);

		if (begin_x>=end_x)
			continue;

		//set final iteration range
		(src.set_x(begin_x, end_x), ...);

		int size_x = end_x-begin_x;
		//iteration on x-axis
		for (int x = size_x / 4; x>0; --x)
		{
			f(src.get_ref()...); (src.inc_x(), ...);
			f(src.get_ref()...); (src.inc_x(), ...);
			f(src.get_ref()...); (src.inc_x(), ...);
			f(src.get_ref()...); (src.inc_x(), ...);
		}
		if (size_x & 2)
		{
			f(src.get_ref()...); (src.inc_x(), ...);
			f(src.get_ref()...); (src.inc_x(), ...);
		}
		if (size_x & 1)
		{
			f(src.get_ref()...); (src.inc_x(), ...);
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
	ShaderDrawImpl([](auto&&... a){ ColorFunc::func(std::forward<decltype(a)>(a)...); }, helper::controler<SrcType>(src_frame)...);
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
 * DX: Remaps a matched pixel's shade into a replacement colour, using the legacy DX
 * recolor semantics keyed off the replacement's low nibble:
 *  - 1  = "lighten" mode: source shades are compressed/shifted ~4 steps lighter
 *         (0,1->0; 2,3->1; 4,5->2; 6,7->3; else shade-4); block-0 results are bumped
 *         +1 so they never land on the transparent index.
 *  - 15 = "darken" mode: shades are compressed into the dark half of the target block
 *         (block + shade/2 + 8) - e.g. "Black" renders as a dark-grey->black ramp.
 *  - anything else: plain replacement + shade (OXCE stock behaviour), with a 0->1
 *         bump to avoid the transparent index.
 * @param srcShade the source pixel's shade (low nibble).
 * @param replacement the replacement colour value (block | mode/offset nibble).
 * @return the remapped pixel.
 */
inline Uint8 RecolorShade(Uint8 srcShade, Uint8 replacement)
{
	const Uint8 block = replacement & ColorGroup;
	switch (replacement & ColorShade)
	{
	case 1: // lighten
	{
		Uint8 s = srcShade < 8 ? srcShade / 2 : srcShade - 4;
		return block ? block + s : s + 1;
	}
	case 15: // darken
		return block + (srcShade >> 1) + 8;
	default:
	{
		Uint8 p = replacement + srcShade;
		return p > 0 ? p : 1;
	}
	}
}

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
	* @param newColor new color to set (it should be offset by 4)
	*/
	static inline void func(Uint8& dest, const Uint8& src, const int& shade, const int& newColor)
	{
#ifdef OXCE_VECTORIZATION_FRIENDLY
		//more vectorization friendly code
		auto n = dest;
		if (src)
		{
			const Uint8 newShade = (src & ColorShade) + shade;
			if (newShade & ColorGroup)
				// so dark it would flip over to another color - make it black instead
				n = ColorShade;
			else
				n = newColor | newShade;
		}
		dest = n;
#else
		if (src)
		{
			const Uint8 newShade = (src & ColorShade) + shade;
			if (newShade & ColorGroup)
				// so dark it would flip over to another color - make it black instead
				dest = ColorShade;
			else
				dest = newColor | newShade;
		}
#endif
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
	* @param not used
	* @param not used
	*/
	static inline void func(Uint8& dest, const Uint8& src, const int& shade)
	{
#ifdef OXCE_VECTORIZATION_FRIENDLY
		//more vectorization friendly code
		auto n = dest;
		if (src)
		{
			const Uint8 newShade = src + shade;
			if ((newShade ^ src) & ColorGroup)
				// so dark it would flip over to another color - make it black instead
				n = ColorShade;
			else
				n = newShade;
		}
		dest = n;
#else
		if (src)
		{
			const Uint8 newShade = src + shade;
			if ((newShade ^ src) & ColorGroup)
				// so dark it would flip over to another color - make it black instead
				dest = ColorShade;
			else
				dest = newShade;
		}
#endif
	}

};
/**
 * helper class used for blitting dying unit with overkill
 */
struct BurnShade
{
	static inline void func(Uint8& dest, const Uint8& src, const int& burn, const int& shade)
	{
		auto n = dest;
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
					StandardShade::func(n, ColorShade, shade);
				}
				else
				{
					StandardShade::func(n, (src & ColorGroup) + tempBurn, shade);
				}
			}
			else
			{
				StandardShade::func(n, src, shade);
			}
		}
		dest = n;
	}
};

}//namespace helper

/**
 * DX: `ShaderDraw` argument that reports the coordinates of the pixel currently being written, in the
 * DESTINATION surface's own coordinate space. Needed by any shader whose output depends on *where* the
 * pixel lands rather than just its color - e.g. the cloaked-unit ghost, a checkerboard dither.
 *
 * Destination-space (rather than blit-range-relative) coordinates matter: a unit is blitted as several
 * separate body parts with different source offsets, so a range-relative parity would put the dither
 * out of phase between the parts and show seams.
 */
class CurrentPixel
{
public:
	int x = 0;
	int y = 0;
};

namespace helper
{

/// Implementation of the current-pixel argument: tracks the raster position as ShaderDraw walks it.
template<>
struct controler<CurrentPixel>
{
	CurrentPixel _pixel;
	int _originX = 0, _originY = 0, _rowX = 0;

	inline controler(const CurrentPixel&)
	{
	}

	// This argument imposes no range of its own; it only observes the one the real surfaces agree on.
	inline void mod_range(GraphSubset&) { }
	inline void set_range(const GraphSubset& range)
	{
		_originX = range.beg_x;
		_originY = range.beg_y;
	}

	inline void mod_y(int&, int&) { }
	inline void set_y(const int& begin, const int&)
	{
		_pixel.y = _originY + begin;
	}
	inline void inc_y()
	{
		++_pixel.y;
	}

	inline void mod_x(int&, int&) { }
	inline void set_x(const int& begin, const int&)
	{
		_rowX = _originX + begin;
		_pixel.x = _rowX;
	}
	inline void inc_x()
	{
		++_pixel.x;
	}

	inline CurrentPixel& get_ref()
	{
		return _pixel;
	}
};

}//namespace helper

static inline CurrentPixel ShaderCurrentPixel()
{
	return CurrentPixel();
}

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

}//namespace OpenXcom
