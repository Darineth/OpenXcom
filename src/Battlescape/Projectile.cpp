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
#include "Projectile.h"
#include "TileEngine.h"
#include "Map.h"
#include "Camera.h"
#include "Particle.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Surface.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleItem.h"
#include "../Mod/MapData.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Engine/RNG.h"
#include "../Engine/Options.h"
#include "../Engine/Logger.h"
#include "../fmath.h"

namespace OpenXcom
{

/**
 * Sets up a UnitSprite with the specified size and position.
 * @param mod Pointer to mod.
 * @param save Pointer to battlesavegame.
 * @param action An action.
 * @param origin Position the projectile originates from.
 * @param targetVoxel Position the projectile is targeting.
 * @param ammo the ammo that produced this projectile, where applicable.
 */
//Projectile::Projectile(Mod *mod, SavedBattleGame *save, BattleAction action, Position origin, Position targetVoxel, BattleItem *ammo) : _res(res), _save(save), _action(action), _origin(origin), _targetVoxel(targetVoxel), _position(0), _bulletSprite(-1), _reversed(false), _vaporColor(-1), _vaporDensity(-1), _vaporProbability(5)
Projectile::Projectile(Mod *mod, SavedBattleGame *save, BattleAction action, Position origin, Position targetVoxel, BattleItem *ammo, int bulletSprite, bool shotgun) : _mod(mod), _save(save), _action(action), _origin(origin), _targetVoxel(targetVoxel), _position(0), _bulletSprite(bulletSprite), _reversed(false), _vaporColor(-1), _vaporDensity(-1), _vaporProbability(5), _impact(0), _shotgun(shotgun), _calculatedAccuracy(1.0), _ammo(ammo), _originVoxel(), _trajectoryVector(false), _trajectoryFinalVector(), _trajectorySkipUnit(0)
{
	// this is the number of pixels the sprite will move between frames
	_speed = Options::battleFireSpeed;
	if (_action.weapon)
	{
		if (_action.type == BA_THROW)
		{
			_sprite = _mod->getSurfaceSet("FLOOROB.PCK")->getFrame(getItem()->getRules()->getFloorSprite());
		}
		else
		{
			// try to get all the required info from the ammo, if present
			if (ammo)
			{
				_bulletSprite = ammo->getRules()->getBulletSprite();
				_vaporColor = ammo->getRules()->getVaporColor();
				_vaporDensity = ammo->getRules()->getVaporDensity();
				_vaporProbability = ammo->getRules()->getVaporProbability();
				_speed = std::max(1, _speed + ammo->getRules()->getBulletSpeed());
			}

			// no ammo, or the ammo didn't contain the info we wanted, see what the weapon has on offer.
			if (_bulletSprite == -1)
			{
				_bulletSprite = _action.weapon->getRules()->getBulletSprite();
			}
			if (_vaporColor == -1)
			{
				_vaporColor = _action.weapon->getRules()->getVaporColor();
			}
			if (_vaporDensity == -1)
			{
				_vaporDensity = _action.weapon->getRules()->getVaporDensity();
			}
			if (_vaporProbability == 5)
			{
				_vaporProbability = _action.weapon->getRules()->getVaporProbability();
			}
			if (!ammo || (ammo != _action.weapon || ammo->getRules()->getBulletSpeed() == 0))
			{
				_speed = std::max(1, _speed + _action.weapon->getRules()->getBulletSpeed());
			}
		}
	}
	if ((targetVoxel.x - origin.x) + (targetVoxel.y - origin.y) >= 0)
	{
		_reversed = true;
	}
}

/**
 * Deletes the Projectile.
 */
Projectile::~Projectile()
{

}

/**
 * Calculates the trajectory for a straight path.
 * @param accuracy The unit's accuracy.
 * @return The objectnumber(0-3) or unit(4) or out of map (5) or -1 (no line of fire).
 */

int Projectile::calculateTrajectory(double accuracy, bool force, bool ignoreAccuracy)
{
	Position originVoxel = _save->getTileEngine()->getOriginVoxel(_action, _save->getTile(_origin));
	return (_impact = calculateTrajectory(accuracy, originVoxel, force, ignoreAccuracy));
}

int Projectile::calculateTrajectory(double accuracy, const Position& originVoxel, bool excludeUnit, bool force, bool ignoreAccuracy)
{
	Tile *targetTile = _save->getTile(_action.target);
	BattleUnit *bu = _action.actor;
	
	int smoke = 0;
	int test = _save->getTileEngine()->calculateLine(originVoxel, _targetVoxel, false, &_trajectory, excludeUnit ? 0 : bu, true, false, 0, &smoke);

	if(smoke > 0)
	{
		double smokePower = (double)smoke / 3.0;

		if(smokePower > TileEngine::MAX_VOXEL_VIEW_DISTANCE)
		{
			accuracy = 0;
		}
		else if(smokePower > (TileEngine::MAX_VOXEL_VIEW_DISTANCE / 2.0))
		{
			double smokeAccuracyReduction = (smokePower - (TileEngine::MAX_VOXEL_VIEW_DISTANCE / 2.0)) / (TileEngine::MAX_VOXEL_VIEW_DISTANCE / 2.0);
			accuracy *= (1.0 - smokeAccuracyReduction);
		}
	}

	_calculatedAccuracy = accuracy;

	/*if(smoke > 0)
	{
		int a = 5 + smoke;
	}*/

	if (test != V_EMPTY &&
		!_trajectory.empty() &&
		_action.actor->getFaction() == FACTION_PLAYER &&
		(_action.type == BA_THROW || _action.autoShotCounter == 1) &&
		!force &&
		((SDL_GetModState() & KMOD_CTRL) == 0 || !Options::forceFire) &&
		_save->getBattleGame()->getPanicHandled() &&
		_action.type != BA_LAUNCH)
	{
		Position hitPos = Position(_trajectory.at(0).x/16, _trajectory.at(0).y/16, _trajectory.at(0).z/24);
		if (test == V_UNIT && _save->getTile(hitPos) && _save->getTile(hitPos)->getUnit() == 0) //no unit? must be lower
		{
			hitPos = Position(hitPos.x, hitPos.y, hitPos.z-1);
		}

		if (hitPos != _action.target && _action.result.empty())
		{
			if (test == V_NORTHWALL)
			{
				if (hitPos.y - 1 != _action.target.y)
				{
					_trajectory.clear();
					return (_impact = V_EMPTY);
				}
			}
			else if (test == V_WESTWALL)
			{
				if (hitPos.x - 1 != _action.target.x)
				{
					_trajectory.clear();
					return (_impact = V_EMPTY);
				}
			}
			else if (test == V_UNIT)
			{
				BattleUnit *hitUnit = _save->getTile(hitPos)->getUnit();
				BattleUnit *targetUnit = targetTile->getUnit();
				if (hitUnit != targetUnit)
				{
					_trajectory.clear();
					return (_impact = V_EMPTY);
				}
			}
			else
			{
				_trajectory.clear();
				return (_impact = V_EMPTY);
			}
		}
	}

	_trajectory.clear();

	// apply some accuracy modifiers.
	// This will results in a new target voxel
	if(!ignoreAccuracy)
	{
		bool extendLine = true;
		// even guided missiles drift, but how much is based on
		// the shooter's faction, rather than accuracy.
		if (_action.type == BA_LAUNCH)
		{
			if (_action.actor->getFaction() == FACTION_PLAYER)
			{
				accuracy = 0.60;
			}
			else
			{
				accuracy = 0.55;
			}
			extendLine = _action.waypoints.size() <= 1;
		}
		applyAccuracy(originVoxel, &_targetVoxel, accuracy, false, extendLine);
	}

	// finally do a line calculation and store this trajectory.
	return (_impact = _save->getTileEngine()->calculateLine(originVoxel, _targetVoxel, true, &_trajectory, bu));
}

DVec3 Projectile::rotateVectorRandomly(const DVec3& vec, const double angle)
{
	DVec3 pv1, pv2;

	//Try not to overlap the vector if it's pointing down.
	if(vec.z < -0.99 && vec.z > -1.01)
	{
		pv1.x = 1;
		pv1.y = 0;
		pv1.z = 0;
	}
	else
	{
		pv1.x = 0;
		pv1.y = 0;
		pv1.z = -1;
	}

	//These two vectors define the plane perpendicular to vec.
	pv1 = pv1.cross(vec);
	pv2 = pv1.cross(vec);

	//Generate a random angle.
	double randTheta = RNG::generate(0.0, 2.0 * 3.14159265359);

	//Use the random angle to pull a uniformly random vector from the plane perpendicular to vec.
	pv1 = pv1 * cos(randTheta) + pv2 * sin(randTheta);

	return vec * cos(angle) + (pv1.cross(vec)) * sin(angle) + pv1 * (pv1.dot(vec)) * (1 - cos(angle));
}

int Projectile::calculateTrajectoryFromVector(bool force, bool ignoreAccuracy, bool dualFiring)
{
	Position originVoxel = _save->getTileEngine()->getOriginVoxel(_action, _save->getTile(_origin));
	return (_impact = calculateTrajectoryFromVector(originVoxel, force, ignoreAccuracy, dualFiring));
}

int Projectile::calculateTrajectoryFromVector(Position originVoxel, bool force, bool ignoreAccuracy, bool dualFiring)
{
	Tile *targetTile = _save->getTile(_action.target);
	BattleUnit *bu = _action.actor;
	BattleActionType actionType = _action.type;
	BattleItem* item = _action.weapon;
	BattleUnit *targetUnit = targetTile->getUnit();

	double smokeMod = 1;
	int smoke = 0;

	int test = _save->getTileEngine()->calculateLine(originVoxel, _targetVoxel, false, &_trajectory, bu, true, false, 0, &smoke);

	if(smoke > 0)
	{
		double smokePower = (double)smoke / 3.0;

		if(smokePower > TileEngine::MAX_VOXEL_VIEW_DISTANCE)
		{
			smokeMod = 0;
		}
		else if(smokePower > (TileEngine::MAX_VOXEL_VIEW_DISTANCE / 2.0))
		{
			double smokeAccuracyReduction = (smokePower - (TileEngine::MAX_VOXEL_VIEW_DISTANCE / 2.0)) / (TileEngine::MAX_VOXEL_VIEW_DISTANCE / 2.0);
			smokeMod = (1.0 - smokeAccuracyReduction);
		}
	}

	if (test != V_EMPTY &&
		!_trajectory.empty() &&
		_action.actor->getFaction() == FACTION_PLAYER &&
		(_action.type == BA_THROW || _action.autoShotCounter == 1) &&
		!force &&
		((SDL_GetModState() & KMOD_CTRL) == 0 || !Options::forceFire) &&
		_save->getBattleGame()->getPanicHandled() &&
		_action.type != BA_LAUNCH)
	{
		Position hitPos = Position(_trajectory.at(0).x/16, _trajectory.at(0).y/16, _trajectory.at(0).z/24);
		if (test == V_UNIT && _save->getTile(hitPos) && _save->getTile(hitPos)->getUnit() == 0) //no unit? must be lower
		{
			hitPos = Position(hitPos.x, hitPos.y, hitPos.z-1);
		}

		if (hitPos != _action.target && _action.result == "")
		{
			if (test == V_NORTHWALL)
			{
				if (hitPos.y - 1 != _action.target.y)
				{
					_trajectory.clear();
					return (_impact = V_EMPTY);
				}
			}
			else if (test == V_WESTWALL)
			{
				if (hitPos.x - 1 != _action.target.x)
				{
					_trajectory.clear();
					return (_impact = V_EMPTY);
				}
			}
			else if (test == V_UNIT)
			{
				BattleUnit *hitUnit = _save->getTile(hitPos)->getUnit();
				if (hitUnit != targetUnit)
				{
					_trajectory.clear();
					return (_impact = V_EMPTY);
				}
			}
			else
			{
				_trajectory.clear();
				return (_impact = V_EMPTY);
			}
		}
	}

	_trajectory.clear();

	//Grab the direction vector between originVoxel and targetVoxel and store it in a DVec3.
	DVec3 direction;
	direction.x = _targetVoxel.x - originVoxel.x;
	direction.y = _targetVoxel.y - originVoxel.y;
	direction.z = _targetVoxel.z - originVoxel.z;

	//Normalize the direction vector for consistency.
	direction.normalize();

	if(_shotgun) //Shotgun pellets ignore everything but their own accuracy.
	{
		double shotgunAcc = item->getRules()->getAccuracyShotgunSpread();

		//Shotgun's aimcone angle.
		double angle = RNG::boxMuller(0, 0.437 / shotgunAcc * 1.4826);

		direction = rotateVectorRandomly(direction, angle);
	}
	else
	{
		double soldierAcc = 1;
		double weaponAcc = item->getRules()->getBaseAccuracy(); //Weapon accuracy comes straight off the weapon, and isn't influenced by anything.
		if(weaponAcc <= 1)
		{
			weaponAcc = 1;
		}

		if(!item)
		{
			return (_impact = V_EMPTY);
		}

		if(bu->isKneeled())
		{
			int kneelMod = item->getRules()->getKneelModifier();
			if(kneelMod)
			{
				soldierAcc = double(bu->getBaseStats()->firing * kneelMod) / 100.0;
			}
			else
			{
				soldierAcc = double(bu->getBaseStats()->firing * 115) / 100.0;
			}
		}
		else
		{
			soldierAcc = double(bu->getBaseStats()->firing);
		}

		if (item->getRules()->isTwoHanded()
			&& bu->getItem(bu->getWeaponSlot1())
			&& bu->getItem(bu->getWeaponSlot2()))
		{
			soldierAcc *= item->getRules()->getTwoHandedModifier() / 100.0;
		}

		// berserking units are half as accurate
		if (bu->getStatus() == STATUS_BERSERK)
		{
			soldierAcc *= 0.5;
		}

		soldierAcc *= double(bu->getAccuracyModifier(item)) / 100.0;

		// Penalize soldier accuracy for sprinting targets.
		if (_action.targetSprinting || (targetUnit && targetUnit->getMovementAction() == MV_SPRINT))
		{
			soldierAcc *= 0.7;
		}

		double energyRatio = std::max((double)bu->getEnergy() / (double)bu->getBaseStats()->stamina, 0.0);
		if (energyRatio < 0.5)
		{
			soldierAcc *= (0.5 + energyRatio);
		}

		soldierAcc *= smokeMod;

		//Specific shot modes now provide an accuracy modifier to the soldier, rather than the weapon.
		if(actionType == BA_SNAPSHOT)
		{
			soldierAcc *= double(item->getRules()->getAccuracySnap()) / 100.0;
		}
		else if(actionType == BA_AIMEDSHOT)
		{
			soldierAcc *= double(item->getRules()->getAccuracyAimed()) / 100.0;
		}
		else if(actionType == BA_AUTOSHOT)
		{
			soldierAcc *= double(item->getRules()->getAccuracyAuto()) / 100.0;
		}
		else if (actionType == BA_BURSTSHOT)
		{
			soldierAcc *= double(item->getRules()->getAccuracyBurst()) / 100.0;
		}
		else if(actionType == BA_DUALFIRE)
		{
			BattleItem *weapon1;
			BattleActionType action1;
			BattleItem *weapon2;
			BattleActionType action2;
			if(bu->canDualFire(weapon1, action1, weapon2, action2))
			{
				soldierAcc *= ((bu->getFiringAccuracy(action1, weapon1, false, true) + bu->getFiringAccuracy(action2, weapon2, false, true)) / 2.0) * bu->getDualFireAccuracy();

				dualFiring = true;
			}
		}

		if(soldierAcc < 20)
			soldierAcc = 20;

		//Soldier's aimcone angle.
		double angle = RNG::boxMuller(0, 0.437 / (soldierAcc * soldierAcc / 50) * 1.4826 * 2);

		if(!force) //Randomize the direction vector to represent the soldier's firing inaccuracy.
			direction = rotateVectorRandomly(direction, angle);

		//Weapon's aimcone angle.
		angle = RNG::boxMuller(0, 0.437 / weaponAcc * 1.4826);

		if(!force) //Randomize the direction vector again to represent the weapon's firing inaccuracy.
			direction = rotateVectorRandomly(direction, angle);

		_calculatedAccuracy = bu->calculateEffectiveRange(soldierAcc, weaponAcc);
	}

	//Minimize data loss from the upcoming integer truncation.
	direction *= 16000;

	_originVoxel = originVoxel;
	_trajectoryVector = true;
	_trajectoryFinalVector.x = direction.x;
	_trajectoryFinalVector.y = direction.y;
	_trajectoryFinalVector.z = direction.z;
	_trajectorySkipUnit = bu;

	return (_impact = _save->getTileEngine()->calculateLineFromVector(_originVoxel, _trajectoryFinalVector, true, &_trajectory, _trajectorySkipUnit));
}

int Projectile::recalculateTrajectoryFromVector()
{
	if (_trajectoryVector)
	{
		_trajectory.clear();
		return (_impact = _save->getTileEngine()->calculateLineFromVector(_originVoxel, _trajectoryFinalVector, true, &_trajectory, _trajectorySkipUnit));
	}

	return _impact;
}

/**
* Brute force simulation raycaster-based hit chance calculator. It projects hundreds of rays at the target from the shooter, adding
* up the succcessful hits and dividing by the total samples to determine an approximate hit chance. Typical error of ~2%.
* This model allows for all factors, including cover, to be included in the attacker's displayed hit chance.
* @return Chance to hit.
*/
//TODO: Merge repeated code between this function and the other calculateTrajectory functions.
void Projectile::calculateChanceToHit(double& chance, double& coverChance)
{
	//Static cache variables used to avoid recalculating the chance to hit every frame
	static Tile *oldTargetTile = 0;
	static BattleUnit *oldTargetUnit = 0;
	static double oldChance = 0;
	static double oldCoverChance = 0;
	static int oldTurn = 0;
	static double oldWeaponAcc = 0;
	static double oldSoldierAcc = 0;

	Position originVoxel = _save->getTileEngine()->getOriginVoxel(_action, _save->getTile(_origin));

	Tile *targetTile = _save->getTile(_action.target);
	BattleUnit *targetUnit = targetTile->getUnit();

	BattleUnit *bu = _action.actor;
	BattleActionType actionType = _action.type;
	BattleItem* item = _action.weapon;

	double smokeMod = 1;
	int smoke = 0;

	int test = _save->getTileEngine()->calculateLine(originVoxel, _targetVoxel, false, &_trajectory, bu, true, false, 0, &smoke);

	if (smoke > 0)
	{
		double smokePower = (double)smoke / 3.0;

		if (smokePower > TileEngine::MAX_VOXEL_VIEW_DISTANCE)
		{
			smokeMod = 0;
		}
		else if (smokePower > (TileEngine::MAX_VOXEL_VIEW_DISTANCE / 2.0))
		{
			double smokeAccuracyReduction = (smokePower - (TileEngine::MAX_VOXEL_VIEW_DISTANCE / 2.0)) / (TileEngine::MAX_VOXEL_VIEW_DISTANCE / 2.0);
			smokeMod = (1.0 - smokeAccuracyReduction);
		}
	}

	//Grab the direction vector between originVoxel and targetVoxel and store it in a DVec3.
	DVec3 initDirection;
	initDirection.x = _targetVoxel.x - originVoxel.x;
	initDirection.y = _targetVoxel.y - originVoxel.y;
	initDirection.z = _targetVoxel.z - originVoxel.z;

	//Normalize the direction vector for consistency.
	initDirection.normalize();

	double soldierAcc = -1;
	double weaponAcc = -1;

	if (_shotgun) //Shotgun pellets ignore everything but their own accuracy.
	{
		weaponAcc = item->getRules()->getAccuracyShotgunSpread();
	}
	else
	{
		soldierAcc = 1;
		weaponAcc = item->getRules()->getBaseAccuracy(); //Weapon accuracy comes straight off the weapon, and isn't influenced by anything.
		if (weaponAcc <= 1)
		{
			weaponAcc = 1;
		}

		if (!item)
		{
			chance = -1;
			coverChance = -1;
			return;
		}

		if (bu->isKneeled())
		{
			int kneelMod = item->getRules()->getKneelModifier();
			if (kneelMod)
			{
				soldierAcc = double(bu->getBaseStats()->firing * kneelMod) / 100.0;
			}
			else
			{
				soldierAcc = double(bu->getBaseStats()->firing * 115) / 100.0;
			}
		}
		else
		{
			soldierAcc = double(bu->getBaseStats()->firing);
		}

		if (item->getRules()->isTwoHanded()
			&& bu->getItem(bu->getWeaponSlot1())
			&& bu->getItem(bu->getWeaponSlot2()))
		{
			soldierAcc *= item->getRules()->getTwoHandedModifier() / 100.0;
		}

		// berserking units are half as accurate
		if (bu->getStatus() == STATUS_BERSERK)
		{
			soldierAcc *= 0.5;
		}

		soldierAcc *= double(bu->getAccuracyModifier(item)) / 100.0;

		// Penalize soldier accuracy for sprinting targets.
		if (_action.targetSprinting || (targetUnit && targetUnit->getMovementAction() == MV_SPRINT))
		{
			soldierAcc *= 0.7;
		}

		double energyRatio = std::max((double)bu->getEnergy() / (double)bu->getBaseStats()->stamina, 0.0);
		if (energyRatio < 0.5)
		{
			soldierAcc *= (0.5 + energyRatio);
		}

		soldierAcc *= smokeMod;

		//Specific shot modes now provide an accuracy modifier to the soldier, rather than the weapon.
		if (actionType == BA_SNAPSHOT)
		{
			soldierAcc *= double(item->getRules()->getAccuracySnap()) / 100.0;
		}
		else if(actionType == BA_AIMEDSHOT)
		{
			soldierAcc *= double(item->getRules()->getAccuracyAimed()) / 100.0;
		}
		else if(actionType == BA_AUTOSHOT)
		{
			soldierAcc *= double(item->getRules()->getAccuracyAuto()) / 100.0;
		}
		else if(actionType == BA_BURSTSHOT)
		{
			soldierAcc *= double(item->getRules()->getAccuracyBurst()) / 100.0;
		}
		else if (actionType == BA_DUALFIRE)
		{
			BattleItem *weapon1;
			BattleActionType action1;
			BattleItem *weapon2;
			BattleActionType action2;
			if (bu->canDualFire(weapon1, action1, weapon2, action2))
			{
				soldierAcc *= ((bu->getFiringAccuracy(action1, weapon1, false, true) + bu->getFiringAccuracy(action2, weapon2, false, true)) / 2.0) * bu->getDualFireAccuracy();
			}
		}

		if (soldierAcc < 20)
			soldierAcc = 20;
	}

	if(oldSoldierAcc == soldierAcc && oldWeaponAcc == weaponAcc && oldTargetTile == targetTile && oldTargetUnit == targetUnit && oldTurn == _save->getTurn())
	{	//Nothing has changed; don't bother recalculating the chance to hit.
		chance = oldChance;
		coverChance = oldCoverChance;
		return;
	}
	oldTurn = _save->getTurn();
	oldTargetTile = targetTile;
	oldTargetUnit = targetUnit;
	oldSoldierAcc = soldierAcc;
	oldWeaponAcc = weaponAcc;

	double angle = 0;
	int hits = 0;
	int obstructedHits = 0;
	int count = 0;

/*	double eRange = bu->calculateEffectiveRange(soldierAcc, weaponAcc);
	Position difference = _targetVoxel/16 - originVoxel/16;
	double distance = sqrt(double(difference.x * difference.x + difference.y * difference.y + difference.z * difference.z));
	if (eRange < 1)
		eRange = 1;

	//Scale the number of sampling rays to the distance from the target and the accuracy of the weapon.
	//This should probably be changed to give the most rays when the hit chance is ~50%, and least rays when the hit chance is likely to be <10% or >90%.
	int numRays = (int)(100.0 * (distance / eRange) * (distance / eRange));
	if (numRays < 10)
		numRays = 10;
	else if (numRays > 1000)
		numRays = 1000;*/
	const int numRays = 500;

	std::vector<Position> oldTrajectory = _trajectory; //Don't mess up the trajectory stored on the projectile.
	_trajectory.clear();
	uint64_t oldSeed = RNG::getSeed(); //Store the old seed to reset it later.
	RNG::setSeed(100000000); //Use the same seed to ensure the raycasting samples always use the same spread pattern.
	for (int i = 0; i < numRays; i += 1)
	{
		DVec3 direction = initDirection;
		if (soldierAcc > -1)
		{
			uint64_t testSeed = RNG::getSeed();
			//Soldier's aimcone angle.
			angle = RNG::boxMuller(0, 0.437 / (soldierAcc * soldierAcc / 50) * 1.4826 * 2);

			//Randomize the direction vector to represent the soldier's firing inaccuracy.
			direction = rotateVectorRandomly(direction, angle);
		}

		//Weapon's aimcone angle.
		angle = RNG::boxMuller(0, 0.437 / weaponAcc * 1.4826);

		//Randomize the direction vector again to represent the weapon's firing inaccuracy.
		direction = rotateVectorRandomly(direction, angle);

		//Minimize data loss from the upcoming integer truncation.
		direction *= 16000;

		_trajectoryFinalVector.x = direction.x;
		_trajectoryFinalVector.y = direction.y;
		_trajectoryFinalVector.z = direction.z;

		int impactResult = -1;
		if (targetUnit != NULL)
		{
			impactResult = _save->getTileEngine()->calculateLineFromVector(_originVoxel, _trajectoryFinalVector, true, &_trajectory, _action.actor, true, false, 0, 0, targetUnit, 0);
		}
		else
		{
			impactResult = _save->getTileEngine()->calculateLineFromVector(_originVoxel, _trajectoryFinalVector, true, &_trajectory, _action.actor, true, false, 0, 0, 0, targetTile);
		}

		if (impactResult == V_TILETARGETHIT || impactResult == V_UNITTARGETHIT)
		{
			++hits;
		}
		else if (impactResult == V_UNITTARGETOBSTRUCTED || impactResult == V_TILETARGETOBSTRUCTED)
		{
			++obstructedHits;
		}

		++count;
	}
	RNG::setSeed(oldSeed);	//Reset the old seed to avoid ruining the RNG.
	_trajectory = oldTrajectory;

	if (count <= 0)
	{
		chance = -1;
		return;
	}

	chance = ((double)(hits)) / ((double)(count));
	coverChance = ((double)(obstructedHits)) / ((double)(count));
	oldChance = chance;
	oldCoverChance = coverChance;

	return;
}

/**
 * Calculates the trajectory for a curved path.
 * @param accuracy The unit's accuracy.
 * @return True when a trajectory is possible.
 */
int Projectile::calculateThrow(double accuracy, bool ignoreAccuracy)
{
	Tile *targetTile = _save->getTile(_action.target);
		
	Position originVoxel = _save->getTileEngine()->getOriginVoxel(_action, 0);
	Position targetVoxel = _action.target * Position(16,16,24) + Position(8,8, (2 + -targetTile->getTerrainLevel()));

	int impact = calculateTrajectory(1.0, originVoxel, false, ignoreAccuracy);
	if(impact == V_EMPTY || impact == V_OUTOFBOUNDS)
	{
		_calculatedAccuracy = 0;
	}
	_calculatedAccuracy = accuracy = (accuracy * (0.25 + 0.75 * _calculatedAccuracy));

	if (_action.type != BA_THROW)
	{
		BattleUnit *tu = targetTile->getUnit();
		if (!tu && _action.target.z > 0 && targetTile->hasNoFloor(0))
			tu = _save->getTile(_action.target - Position(0, 0, 1))->getUnit();
		if (tu)
		{
			targetVoxel.z += ((tu->getHeight()/2) + tu->getFloatHeight()) - 2;
		}
	}

	double curvature = 0.0;
	int retVal = V_OUTOFBOUNDS;
	if (_save->getTileEngine()->validateThrow(_action, originVoxel, targetVoxel, &curvature, &retVal))
	{
		int test = V_OUTOFBOUNDS;
		// finally do a line calculation and store this trajectory, make sure it's valid.
		while (test == V_OUTOFBOUNDS)
		{
			Position deltas = targetVoxel;
			// apply some accuracy modifiers
			if(!ignoreAccuracy)
			{
				applyAccuracy(originVoxel, &deltas, accuracy, true, false); //calling for best flavor
			}
			deltas -= targetVoxel;
			_trajectory.clear();
			test = _save->getTileEngine()->calculateParabola(originVoxel, targetVoxel, true, &_trajectory, _action.actor, curvature, deltas);

			Position endPoint = _trajectory.back();
			endPoint.x /= 16;
			endPoint.y /= 16;
			endPoint.z /= 24;
			Tile *endTile = _save->getTile(endPoint);
			// check if the item would land on a tile with a blocking object
			if (_action.type == BA_THROW
				&& endTile
				&& endTile->getMapData(O_OBJECT)
				&& endTile->getMapData(O_OBJECT)->getTUCost(MT_WALK) == 255
				&& !(endTile->isBigWall() && (endTile->getMapData(O_OBJECT)->getBigWall()<1 || endTile->getMapData(O_OBJECT)->getBigWall()>3)))
			{
				test = V_OUTOFBOUNDS;
				if(ignoreAccuracy)
				{
					return V_OUTOFBOUNDS;
				}
			}
		}
		return retVal;
	}
	return V_OUTOFBOUNDS;
}

/**
 * Calculates the new target in voxel space, based on the given accuracy modifier.
 * @param origin Startposition of the trajectory in voxels.
 * @param target Endpoint of the trajectory in voxels.
 * @param accuracy Accuracy modifier.
 * @param keepRange Whether range affects accuracy.
 * @param extendLine should this line get extended to maximum distance?
 */
void Projectile::applyAccuracy(Position origin, Position *target, double accuracy, bool keepRange, bool extendLine)
{
	int xdiff = origin.x - target->x;
	int ydiff = origin.y - target->y;
	double realDistance = sqrt((double)(xdiff*xdiff)+(double)(ydiff*ydiff));
	// maxRange is the maximum range a projectile shall ever travel in voxel space
	double maxRange = keepRange?realDistance:16*1000; // 1000 tiles
	maxRange = _action.type == BA_HIT?46:maxRange; // up to 2 tiles diagonally (as in the case of reaper v reaper)
	RuleItem *weapon = _action.weapon->getRules();

	if (_action.type != BA_THROW && _action.type != BA_HIT)
	{
		double modifier = 0.0;
		int upperLimit = weapon->getAimRange();
		int lowerLimit = weapon->getMinRange();
		if (Options::battleUFOExtenderAccuracy)
		{
			if (_action.type == BA_AUTOSHOT)
			{
				upperLimit = weapon->getAutoRange();
			}
			else if (_action.type == BA_BURSTSHOT)
			{
				upperLimit = weapon->getBurstRange();
			}
			else if (_action.type == BA_SNAPSHOT)
			{
				upperLimit = weapon->getSnapRange();
			}
		}
		if (realDistance / 16 < lowerLimit)
		{
			modifier = (weapon->getDropoff() * (lowerLimit - realDistance / 16)) / 100;
		}
		else if (upperLimit < realDistance / 16)
		{
			modifier = (weapon->getDropoff() * (realDistance / 16 - upperLimit)) / 100;
		}
		accuracy = std::max(0.0, accuracy - modifier);
	}

	int xDist = abs(origin.x - target->x);
	int yDist = abs(origin.y - target->y);
	int zDist = abs(origin.z - target->z);
	int xyShift, zShift;

	int deviation = 0;

	if(!_shotgun)
	{
		if (xDist / 2 <= yDist)				//yes, we need to add some x/y non-uniformity
			xyShift = xDist / 4 + yDist;	//and don't ask why, please. it's The Commandment
		else
			xyShift = (xDist + yDist) / 2;	//that's uniform part of spreading

		if (xyShift <= zDist)				//slight z deviation
			zShift = xyShift / 2 + zDist;
		else
			zShift = xyShift + zDist / 2;

		deviation = RNG::generate(0, 100) - (accuracy * 100);

		if (deviation >= 0)
			deviation += 50;				// add extra spread to "miss" cloud
		else
			deviation += 10;				//accuracy of 109 or greater will become 1 (tightest spread)

		deviation = std::max(1, zShift * deviation / 200);	//range ratio
		
		target->x += RNG::generate(0, deviation) - deviation / 2;
		target->y += RNG::generate(0, deviation) - deviation / 2;
		target->z += RNG::generate(0, deviation / 2) / 2 - deviation / 8;
	}
	else
	{	//Shotgun spread
		//Uses a normal distribution to calculate the deviation from the target, multiplied by the distance so that far shots impact farther apart than near shots
		double distance = sqrt(double(origin.x - target->x) * double(origin.x - target->x) + double(origin.y - target->y) * double(origin.y - target->y) + double(origin.z - target->z) * double(origin.z - target->z));

		double shotgunAccuracy = (double)weapon->getAccuracyShotgunSpread();
		if(shotgunAccuracy > 100.0)
			shotgunAccuracy = 100.0;
		else if(shotgunAccuracy <= 0.0)
			shotgunAccuracy = 0.0;

		//Rough accuracy model
		target->x += (int)(RNG::boxMuller(0.0, 50.0 - shotgunAccuracy/2.0) * distance / 160.0);
		target->y += (int)(RNG::boxMuller(0.0, 50.0 - shotgunAccuracy/2.0) * distance / 160.0);
		target->z += (int)(RNG::boxMuller(0.0, 6.25 - shotgunAccuracy/16.0) * distance / 160.0);
		
		//Circular error probability model
		/*double stdDev = shotgunAccuracy * 1.5;
		target->x += (int)(RNG::boxMuller(0.0, stdDev * 1.6) * distance / 160.0);
		target->y += (int)(RNG::boxMuller(0.0, stdDev * 1.6) * distance / 160.0);
		target->z += (int)(RNG::boxMuller(0.0, stdDev * 1.6) * distance / 160.0);*/
	}
	
	if (extendLine)
	{
		double rotation, tilt;
		rotation = atan2(double(target->y - origin.y), double(target->x - origin.x)) * 180 / M_PI;
		tilt = atan2(double(target->z - origin.z),
			sqrt(double(target->x - origin.x)*double(target->x - origin.x)+double(target->y - origin.y)*double(target->y - origin.y))) * 180 / M_PI;
		// calculate new target
		// this new target can be very far out of the map, but we don't care about that right now
		double cos_fi = cos(tilt * M_PI / 180.0);
		double sin_fi = sin(tilt * M_PI / 180.0);
		double cos_te = cos(rotation * M_PI / 180.0);
		double sin_te = sin(rotation * M_PI / 180.0);
		target->x = (int)(origin.x + maxRange * cos_te * cos_fi);
		target->y = (int)(origin.y + maxRange * sin_te * cos_fi);
		target->z = (int)(origin.z + maxRange * sin_fi);
	}
}

/**
 * Moves further in the trajectory.
 * @return false if the trajectory is finished - no new position exists in the trajectory.
 */
bool Projectile::move()
{
	for (int i = 0; i < _speed; ++i)
	{
		if (++_position == _trajectory.size())
		{
			--_position;
			return false;
		}
		if (_save->getDepth() > 0 && _vaporColor != -1 && _action.type != BA_THROW && RNG::percent(_vaporProbability))
		{
			addVaporCloud();
		}
	}
	return true;
}

/**
 * Reset the projectile to the beginning of its trajectory.
 */
void Projectile::resetTrajectory()
{
	_position = 0;
}

/**
 * Gets the current position in voxel space.
 * @param offset Offset.
 * @return Position in voxel space.
 */
Position Projectile::getPosition(int offset) const
{
	int posOffset = (int)_position + offset;
	if (posOffset >= 0 && posOffset < (int)_trajectory.size())
		return _trajectory.at(posOffset);
	else if (_trajectory.size())
		return _trajectory.at(_position);
	else
		return Position(0, 0, 0);
}

/**
 * Returns the last calculated impact for the projectile.
 * @return The last calculated impact type.
 */
int Projectile::getImpact() const
{
	return _impact;
}

/**
 * Gets a particle reference from the projectile surfaces.
 * @param i Index.
 * @return Particle id.
 */
int Projectile::getParticle(int i) const
{
	if (_bulletSprite != -1)
		return _bulletSprite + i;
	else
		return -1;
}

/**
 * Gets the project tile item.
 * Returns 0 when there is no item thrown.
 * @return Pointer to BattleItem.
 */
BattleItem *Projectile::getItem() const
{
	if (_action.type == BA_THROW)
		return _action.weapon;
	else
		return 0;
}

/**
 * Gets the bullet sprite.
 * @return Pointer to Surface.
 */
Surface *Projectile::getSprite() const
{
	return _sprite;
}

/**
 * Skips to the end of the trajectory.
 */
void Projectile::skipTrajectory()
{
	while (move());
}

/**
 * Gets the Position of origin for the projectile
 * @return origin as a tile position.
 */
Position Projectile::getOrigin() const
{
	// instead of using the actor's position, we'll use the voxel origin translated to a tile position
	// this is a workaround for large units.
	return _trajectory.front() / Position(16,16,24);
}

/**
* Gets the Position of the origin voxel for the projectile.
* @return origin as a voxel position.
*/
Position Projectile::getOriginVoxel() const
{
	return _trajectory.front();
}

/**
 * Gets the INTENDED target for this projectile
 * it is important to note that we do not use the final position of the projectile here,
 * but rather the targetted tile
 * @return target as a tile position.
 */
Position Projectile::getTarget() const
{
	return _action.target;
}

/**
 * Is this projectile drawn back to front or front to back?
 * @return return if this is to be drawn in reverse order.
 */
bool Projectile::isReversed() const
{
	return _reversed;
}

/**
 * adds a cloud of vapor at the projectile's current position.
 */
void Projectile::addVaporCloud()
{
	Tile *tile = _save->getTile(_trajectory.at(_position) / Position(16,16,24));
	if (tile)
	{
		Position tilePos, voxelPos;
		_save->getBattleGame()->getMap()->getCamera()->convertMapToScreen(_trajectory.at(_position) / Position(16,16,24), &tilePos);
		tilePos += _save->getBattleGame()->getMap()->getCamera()->getMapOffset();
		_save->getBattleGame()->getMap()->getCamera()->convertVoxelToScreen(_trajectory.at(_position), &voxelPos);
		for (int i = 0; i != _vaporDensity; ++i)
		{
			Particle *particle = new Particle(voxelPos.x - tilePos.x + RNG::seedless(0, 4) - 2, voxelPos.y - tilePos.y + RNG::seedless(0, 4) - 2, RNG::seedless(48, 224), _vaporColor, RNG::seedless(32, 44));
			tile->addParticle(particle);
		}
	}
}

double Projectile::getCalculatedAccuracy() const
{
	return _calculatedAccuracy;
}

BattleItem *Projectile::getAmmo() const
{
	return _ammo;
}
}
