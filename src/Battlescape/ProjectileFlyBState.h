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
#include <vector>
#include "BattleState.h"
#include "Position.h"

namespace OpenXcom
{

class BattlescapeGame;
class BattleUnit;
class BattleItem;
class ExplosionBState;
class Tile;
class Timer;

/**
 * A projectile state.
 */
class ProjectileFlyBState : public BattleState
{
private:
	BattleUnit *_unit;
	BattleItem *_ammo;
	BattleItem *_weapon;
	BattleItem *_projectileItem;
	Timer *_autoShotTimer;
	Timer *_dualFireTimer;
	Position _origin, _targetVoxel, _originVoxel, _originalTarget;
	ProjectileFlyBState *_dualState;
	bool _subState;
	bool _subStateFinished;
	//int _projectileImpact;
	int _range;
	/// Tries to create a projectile sprite.
	bool createNewProjectile();
	bool _initialized, _targetFloor;
	int _autoShotsRemaining;
	bool _firstProjectile;

	std::vector<ExplosionBState*> _explosionStates;

	void onAutoshotTimer();
	void onDualFireTimer();

	void popParentState();

	void onNewExplosionState(ExplosionBState *subState);

	ProjectileFlyBState(BattlescapeGame *parent, BattleAction action, Position origin, bool subState);

public:
	/// Creates a new ProjectileFly class
	ProjectileFlyBState(BattlescapeGame *parent, BattleAction action);
	ProjectileFlyBState(BattlescapeGame *parent, BattleAction action, Position origin, int range);
	/// Cleans up the ProjectileFly.
	~ProjectileFlyBState();
	/// Initializes the state.
	void init();
	/// Handles a cancel request.
	void cancel();
	/// Runs state functionality every cycle.
	void think();
	/// Validates the throwing range.
	static bool validThrowRange(BattleAction *action, Position origin, Tile *target);
	/// Calculates the maximum throwing range.
	static int getMaxThrowDistance(int weight, int strength, int level);
	/// Set the origin voxel, used for the blaster launcher.
	void setOriginVoxel(const Position& pos);
	/// Set the boolean flag to angle a blaster bomb towards the floor.
	void targetFloor();
	void projectileHitUnit(Position pos);

};

}
