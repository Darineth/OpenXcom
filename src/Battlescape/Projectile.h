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
#include "Position.h"
#include "BattlescapeGame.h"

namespace OpenXcom
{

class BattleItem;
class BattleUnit;
class SavedBattleGame;
class Surface;
class Tile;
class Mod;
struct BattleActionAttack;

/**
 * A double-precision 3D direction vector used by the aim-cone firing model
 * (Position is integer voxels; cone deflection needs sub-voxel angular precision).
 */
struct AimVector
{
	double x, y, z;
};

/// Outcome of one sampled shot/throw, for colouring the spread dot cloud.
enum SpreadOutcome
{
	SPREAD_MISS = 0,  ///< genuine aim miss (landed off the target, not blocked by cover)
	SPREAD_HIT,       ///< landed on the intended target (its unit/wall/tile; the target tile for throws)
	SPREAD_COVER,     ///< aimed on the target but stopped by intervening terrain (cover)
};

/**
 * One sampled shot/throw outcome for the spread visualization (Alt dot cloud): the voxel where the
 * round came down and how it turned out, so hits / cover-blocks / misses can be drawn differently.
 */
struct SpreadSample
{
	Position pos;
	SpreadOutcome outcome;
};

/**
 * A class that represents a projectile. Map is the owner of an instance of this class during its short life.
 * It calculates its own trajectory and then moves along this pre-calculated trajectory in voxel space.
 */
class Projectile
{
public:
	/// Offset of voxel path where item should be drop.
	static const int ItemDropVoxelOffset = -2;

	/// Get Position at offset from start from trajectory vector.
	static Position getPositionFromStart(const std::vector<Position>& trajectory, int pos);
	/// Get Position at offset from end from trajectory vector.
	static Position getPositionFromEnd(const std::vector<Position>& trajectory, int pos);

	/// Aim-cone model: soldier-cone stddev (radians) for a percent-scale effective soldier accuracy.
	static double soldierConeSigma(double soldierAcc);
	/// Aim-cone model: weapon-cone stddev (radians) for a weapon baseAccuracy (scaled by ammo shotgunSpread%).
	static double weaponConeSigma(int baseAccuracy, int shotgunSpread = 100);
	/// Aim-cone model: estimated physical hit chance (0-100%) against a target, for the crosshair
	/// readout. Voxel-traces sampled shots against real terrain (cover-aware); cache the result per aim.
	static int calculateHitChancePercent(SavedBattleGame* save, BattleAction* action, Position targetPos, BattleItem* ammo, Mod* mod, bool hasLOS, int* outCoverReduction = nullptr, std::vector<SpreadSample>* outSampleImpacts = nullptr);
	/// Aim-cone model: the 50%-hit "effective range" (tiles) against a standard target in the open -
	/// a target/terrain-independent property of the shooter+weapon, for the action-menu readout.
	static int calculateEffectiveRange(double soldierAcc, int baseAccuracy, int shotgunSpread = 100);
	/// Realistic throwing: estimated chance (0-100%) a thrown item lands on the exact target tile,
	/// for the throw-cursor readout. Monte-Carlos the launch error through the real parabola.
	static int calculateThrowLandChancePercent(SavedBattleGame* save, BattleAction* action, Position targetPos, Mod* mod, std::vector<SpreadSample>* outSampleLandings = nullptr);

private:
	Mod *_mod;
	SavedBattleGame *_save;
	BattleAction _action;
	const BattleItem* _ammo = nullptr;
	Position _origin, _targetVoxel;
	std::vector<Position> _trajectory;
	size_t _position;
	float _distance;
	float _distanceMax;
	int _speed;
	int _bulletSprite;
	bool _reversed;
	int _vaporColor, _vaporDensity, _vaporProbability;
	int _impact = 0;
	/// Aim-cone model: the soldier-cone-deflected "true aim" line of the volley (unit vector).
	/// Shared across a shotgun volley so every pellet deviates off the same shooter error.
	AimVector _coneTrueAim = { 0.0, 0.0, 0.0 };
	bool _hasConeTrueAim = false;
	void applyAccuracy(Position origin, Position *target, double accuracy, bool keepRange, bool extendLine);
	/// Realistic throwing (battleRealisticThrowing): launch-error landing offset (short/long + lateral).
	Position computeThrowLaunchError(Position originVoxel, Position targetVoxel, double accuracy) const;
	/// Aim-cone model: replaces the scatter deviation for opted-in weapons (baseAccuracy > 0).
	void applyAimCone(Position origin, Position *target, double soldierAcc);
	/// The weapon's no-LOS accuracy multiplier for the current target tile (100 = no penalty).
	int getNoLOSAccuracyPenaltyFactor(const Position &targetVoxel);
	/// Whether this shot uses the aim-cone model instead of the native scatter model.
	bool useAimCone() const;
public:
	/// Creates a new Projectile.
	Projectile(Mod *mod, SavedBattleGame *save, BattleAction action, Position origin, Position target, BattleItem *ammo);
	/// Cleans up the Projectile.
	~Projectile();
	/// Calculates the trajectory for a straight path.
	int calculateTrajectory(double accuracy);
	int calculateTrajectory(double accuracy, const Position& originVoxel, bool excludeUnit = true);
	/// Calculates the trajectory for a curved path.
	int calculateThrow(double accuracy, bool ignoreAccuracy = false);
	/// Traces the ideal (undeviated) straight line-of-fire for the aiming preview and stores it.
	int calculatePreviewTrajectory();
	/// Re-traces a straight trajectory against the current terrain (e.g. after a prior impact destroyed an obstacle); returns true if the path now extends further.
	bool recalculateImpact();
	/// Moves the projectile one step in its trajectory.
	bool move();
	/// Gets the current position in voxel space.
	Position getPosition(int offset = 0) const;
	/// Gets the two last position in voxel space.
	LastPositions getLastPositions(int offset = 0) const { return LastPositions(getPosition(offset), getPosition(offset + ItemDropVoxelOffset)); }
	/// Gets the impact position from the precomputed trajectory (valid right after calculateTrajectory/Throw).
	Position getImpactPosition(int offset = 0) const { return getPositionFromEnd(_trajectory, offset); }
	/// Gets the full precomputed voxel trajectory (used to draw the aiming preview).
	const std::vector<Position>& getTrajectory() const { return _trajectory; }
	/// Gets a particle from the particle array.
	int getParticle(int i) const;
	/// Gets the item.
	BattleItem *getItem() const;
	/// Gets the action that fired this projectile (its own weapon/type - needed to resolve impacts
	/// per-projectile when different weapons fly at once, e.g. dual-fire).
	const BattleAction& getAction() const { return _action; }
	/// Gets the ammo that fired this projectile (may differ per projectile under dual-fire).
	const BattleItem* getAmmo() const { return _ammo; }
	/// Skips the bullet flight.
	void skipTrajectory();
	/// Gets the Position of origin for the projectile.
	Position getOrigin() const;
	/// Gets the targetted tile for the projectile.
	Position getTarget() const;
	/// Gets the distance that projectile traveled.
	float getDistance() const;
	/// Stores the voxel-type this projectile is going to impact (computed at fire time).
	void setImpact(int impact) { _impact = impact; }
	/// Gets the voxel-type this projectile impacts.
	int getImpact() const { return _impact; }
	/// Is this projectile being drawn back-to-front or front-to-back?
	bool isReversed() const;
	/// adds a cloud of particles at the projectile's location
	void addVaporCloud();
	/// Aim-cone model: did this projectile roll (or receive) a volley "true aim" line?
	bool hasConeTrueAim() const { return _hasConeTrueAim; }
	/// Aim-cone model: gets the soldier-cone-deflected volley aim line (valid after calculateTrajectory).
	AimVector getConeTrueAim() const { return _coneTrueAim; }
	/// Aim-cone model: presets the volley aim line, so this projectile (a follow-up shotgun
	/// pellet) skips the soldier cone and only rolls its own weapon-cone deflection.
	void setConeTrueAim(const AimVector &aim) { _coneTrueAim = aim; _hasConeTrueAim = true; }
};

}
