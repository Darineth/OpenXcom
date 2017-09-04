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
 * along with OpenXcom.  If not, see <http:///www.gnu.org/licenses/>.
 */
#include "../Engine/State.h"

namespace OpenXcom
{

class Game;
class Text;
class InteractiveSurface;
class Frame;
class Surface;
class BattleItem;
struct BattleAction;

/**
 * Window that allows the player
 * to set the timer of an explosive.
 */
class PrimeGrenadeState : public State
{
private:
	static const int MAX_PRIMER_DELAY = 24;
	BattleAction *_action;
	bool _inInventoryView;
	BattleItem *_grenadeInInventory;
	Text *_number[MAX_PRIMER_DELAY];
	Text *_title;
	Frame *_frame;
	InteractiveSurface *_btnInstant;
	Text *_txtInstant;
	InteractiveSurface *_button[MAX_PRIMER_DELAY];
	Surface *_bg;
public:
	/// Creates the Prime Grenade state.
	PrimeGrenadeState(BattleAction *action, bool inInventoryView, BattleItem *grenadeInInventory);
	/// Cleans up the Prime Grenade state.
	~PrimeGrenadeState();
	/// Handler for right-clicking anything.
	void handle(Action *action);
	/// Handler for clicking a button.
	void btnClick(Action *action);
	/// Handler for clicking the instant fuse button.
	void btnInstantClick(Action *action);
};

}
