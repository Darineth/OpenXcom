/*
 * Copyright 2010-2015 OpenXcom Developers.
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
#ifndef OPENXCOM_ITEMSPRITE_H
#define OPENXCOM_ITEMSPRITE_H

#include "../Engine/Surface.h"
#include "../Engine/Script.h"

namespace OpenXcom
{

class BattleUnit;
class BattleItem;
class SurfaceSet;
class Mod;

/**
 * A class that renders a specific unit, given its render rules
 * combining the right frames from the surfaceset.
 */
class ItemSprite
{
private:
	SurfaceSet *_itemSurface;
	Mod *_mod;
	int _animationFrame;
	Surface *_dest;

public:
	/// Creates a new ItemSprite at the specified position and size.
	ItemSprite(Surface* dest, Mod* mod, int frame);
	/// Cleans up the ItemSprite.
	~ItemSprite();
	/// Draws the item.
	void draw(BattleItem* item, int x, int y, int shade, bool half = false);
	/// Draws the item shadow.
	void drawShadow(BattleItem* item, int x, int y, bool half = false);
};

} //namespace OpenXcom

#endif
