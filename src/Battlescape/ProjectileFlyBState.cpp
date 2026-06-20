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
#include <algorithm>
#include "ProjectileFlyBState.h"
#include "ExplosionBState.h"
#include "Projectile.h"
#include "TileEngine.h"
#include "Map.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Savegame/HitLog.h"
#include "../Mod/Mod.h"
#include "../Engine/Sound.h"
#include "../Engine/RNG.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleItem.h"
#include "../Engine/Options.h"
#include "AIModule.h"
#include "Camera.h"
#include "Explosion.h"
#include "BattlescapeState.h"
#include "../Savegame/BattleUnitStatistics.h"
#include "../fmath.h"

namespace OpenXcom
{

/**
 * Sets up an ProjectileFlyBState.
 */
ProjectileFlyBState::ProjectileFlyBState(BattlescapeGame *parent, BattleAction action, Position origin, int range) : BattleState(parent, action), _unit(0), _ammo(0), _origin(origin), _originVoxel(-1,-1,-1), _projectileImpact(0), _range(range), _initialized(false), _targetFloor(false)
{
}

ProjectileFlyBState::ProjectileFlyBState(BattlescapeGame *parent, BattleAction action) : BattleState(parent, action), _unit(0), _ammo(0), _origin(action.actor->getPosition()), _originVoxel(-1,-1,-1), _projectileImpact(0), _range(0), _initialized(false), _targetFloor(false)
{
}

/**
 * Deletes the ProjectileFlyBState.
 */
ProjectileFlyBState::~ProjectileFlyBState()
{
}

/**
 * Initializes the sequence:
 * - checks if the shot is valid,
 * - calculates the base accuracy.
 */
void ProjectileFlyBState::init()
{
	if (_initialized) return;
	_initialized = true;

	BattleItem *weapon = _action.weapon;

	if (!weapon) // can't shoot without weapon
	{
		_parent->popState();
		return;
	}

	if (!_parent->getSave()->getTile(_action.target)) // invalid target position
	{
		_parent->popState();
		return;
	}

	//test TU only on first lunch waypoint or normal shoot
	if (_range == 0 && !_action.haveTU(&_action.result))
	{
		_parent->popState();
		return;
	}

	_unit = _action.actor;

	bool reactionShoot = _unit->getFaction() != _parent->getSave()->getSide();
	if (_action.type != BA_THROW)
	{
		_ammo = _action.weapon->getAmmoForAction(_action.type, reactionShoot ? nullptr : &_action.result);
		if (!_ammo)
		{
			_parent->popState();
			return;
		}
	}

	if (_unit->isOut() || _unit->isOutThresholdExceed())
	{
		// something went wrong - we can't shoot when dead or unconscious, or if we're about to fall over.
		_parent->popState();
		return;
	}

	// reaction fire
	if (reactionShoot)
	{
		BattleUnit* target = _parent->getSave()->getTile(_action.target)->getUnit();
		// target is dead: cancel the shot.
		if (!target || target->isOut() || target->isOutThresholdExceed() || target != _parent->getSave()->getSelectedUnit())
		{
			_parent->popState();
			return;
		}
		_unit->lookAt(_action.target, _unit->getTurretType() != -1);
		while (_unit->getStatus() == STATUS_TURNING)
		{
			_unit->turn(_unit->getTurretType() != -1);
		}
	}

	Tile *endTile = _parent->getSave()->getTile(_action.target);
	int distanceSq = _action.actor->distance3dToPositionSq(_action.target);
	bool isPlayer = _parent->getSave()->getSide() == FACTION_PLAYER;
	if (isPlayer) _parent->getMap()->resetObstacles();
	switch (_action.type)
	{
	case BA_SNAPSHOT:
	case BA_AIMEDSHOT:
	case BA_AUTOSHOT:
	case BA_LAUNCH:
		if (weapon->getRules()->isOutOfRange(distanceSq))
		{
			// out of range
			_action.result = "STR_OUT_OF_RANGE";
			_parent->popState();
			return;
		}
		break;
	case BA_THROW:
		if (!validThrowRange(&_action, _parent->getTileEngine()->getOriginVoxel(_action, 0), _parent->getSave()->getTile(_action.target), _parent->getSave()->getDepth()))
		{
			// out of range
			_action.result = "STR_OUT_OF_RANGE";
			_parent->popState();
			return;
		}
		if (endTile &&
			endTile->getTerrainLevel() == -24 &&
			endTile->getPosition().z + 1 < _parent->getSave()->getMapSizeZ())
		{
			_action.target.z += 1;
		}
		break;
	default:
		_parent->popState();
		return;
	}

	// Check for close quarters combat
	if (_parent->getMod()->getEnableCloseQuartersCombat() && _action.type != BA_THROW && _action.type != BA_LAUNCH && _unit->getTurretType() == -1 && !_unit->getArmor()->getIgnoresMeleeThreat())
	{
		// Start by finding 'targets' for the check
		std::vector<BattleUnit*> closeQuartersTargetList;
		int surroundingTilePositions [8][2] = {
			{0, -1}, // north (-y direction)
			{1, -1}, // northeast
			{1, 0}, // east (+ x direction)
			{1, 1}, // southeast
			{0, 1}, // south (+y direction)
			{-1, 1}, // southwest
			{-1, 0}, // west (-x direction)
			{-1, -1}}; // northwest
		for (int dir = 0; dir < 8; dir++)
		{
			Position tileToCheck = _origin;
			tileToCheck.x += surroundingTilePositions[dir][0];
			tileToCheck.y += surroundingTilePositions[dir][1];

			if (_parent->getSave()->getTile(tileToCheck)) // Make sure the tile is in bounds
			{
				BattleUnit* closeQuartersTarget = _parent->getSave()->selectUnit(tileToCheck);
				// Variable for LOS check
				int checkDirection = _parent->getTileEngine()->getDirectionTo(tileToCheck, _unit->getPosition());
				if (closeQuartersTarget && _unit->getFaction() != closeQuartersTarget->getFaction() // Unit must exist and not be same faction
					&& closeQuartersTarget->getArmor()->getCreatesMeleeThreat() // Unit must be valid defender, 2x2 default false here
					&& closeQuartersTarget->getTimeUnits() >= _parent->getMod()->getCloseQuartersTuCostGlobal() // Unit must have enough TUs
					&& closeQuartersTarget->getEnergy() >= _parent->getMod()->getCloseQuartersEnergyCostGlobal() // Unit must have enough Energy
					&& _parent->getTileEngine()->validMeleeRange(closeQuartersTarget, _unit, checkDirection) // Unit must be able to see the unit attempting to fire
					&& !(_unit->getFaction() == FACTION_PLAYER && closeQuartersTarget->getFaction() == FACTION_NEUTRAL) // Civilians don't inhibit player
					&& !(_unit->getFaction() == FACTION_NEUTRAL && closeQuartersTarget->getFaction() == FACTION_PLAYER)) // Player doesn't inhibit civilians
				{
					if (RNG::percent(_parent->getMod()->getCloseQuartersSneakUpGlobal()))
					{
						if (_unit->getFaction() == FACTION_HOSTILE) // alien attacker (including mind-controlled xcom)
						{
							if (!closeQuartersTarget->hasVisibleUnit(_unit))
							{
								continue; // the xcom/civilian victim *DOES NOT SEE* the attacker and cannot defend itself
							}
						}
						else // xcom/civilian attacker (including mind-controlled aliens)
						{
							if (_unit->getTurnsSinceSpotted() > 1)
							{
								continue; // the aliens (as a collective) *ARE NOT AWARE* of the attacker and cannot defend themselves
							}
						}
					}
					closeQuartersTargetList.push_back(closeQuartersTarget);
				}
			}
		}

		if (!closeQuartersTargetList.empty())
		{
			int closeQuartersFailedResults[6] = {
				0,   // Fire straight down
				0,   // Fire straight up
				6,   // Fire left 90 degrees
				7,   // Fire left 45 degrees
				1,   // Fire right 45 degrees
				2 }; // Fire right 90 degrees

			for (auto* bu : closeQuartersTargetList)
			{
				BattleActionAttack attack;
				attack.type = BA_CQB;
				attack.attacker = _action.actor;
				attack.weapon_item = _action.weapon;
				attack.damage_item = _action.weapon;

				// Roll for the check
				if (!_parent->getTileEngine()->meleeAttack(attack, bu))
				{
					// Failed the check, roll again to see result
					if (_parent->getSave()->getSide() == FACTION_PLAYER) // Only show message during player's turn
					{
						_action.result = "STR_FAILED_CQB_CHECK";
					}
					int rng = RNG::generate(0, 5);
					Position closeQuartersFailedNewTarget = _unit->getPosition();
					if (rng == 1)
					{
						closeQuartersFailedNewTarget.z += 1;
					}
					else if (rng > 1)
					{
						int newFacing = (_unit->getDirection() + closeQuartersFailedResults[rng]) % 8;
						closeQuartersFailedNewTarget.x += surroundingTilePositions[newFacing][0];
						closeQuartersFailedNewTarget.y += surroundingTilePositions[newFacing][1];
					}

					// Make sure the new target is in bounds
					if (!_parent->getSave()->getTile(closeQuartersFailedNewTarget))
					{
						// Default to firing at our feet
						closeQuartersFailedNewTarget = _unit->getPosition();
					}

					// Turn to look at new target
					_action.target = closeQuartersFailedNewTarget;
					_unit->lookAt(_action.target, _unit->getTurretType() != -1);
					while (_unit->getStatus() == STATUS_TURNING)
					{
						_unit->turn(_unit->getTurretType() != -1);
					}

					// We're done, spend TUs and Energy; and don't check remaining CQB candidates anymore
					bu->spendTimeUnits(_parent->getMod()->getCloseQuartersTuCostGlobal());
					bu->spendEnergy(_parent->getMod()->getCloseQuartersEnergyCostGlobal());
					break;
				}
			}
		}
	}

	bool forceEnableObstacles = false;
	if (_action.type == BA_LAUNCH || (Options::forceFire && _parent->getSave()->isCtrlPressed(true) && isPlayer) || !_parent->getPanicHandled())
	{
		// target nothing, targets the middle of the tile
		_targetVoxel = _action.target.toVoxel() + TileEngine::voxelTileCenter;
		if (_action.type == BA_LAUNCH)
		{
			if (_targetFloor)
			{
				// launched missiles with two waypoints placed on the same tile: target the floor.
				_targetVoxel.z -= 10;
			}
			else
			{
				// launched missiles go slightly higher than the middle.
				_targetVoxel.z += 4;
			}
		}
	}
	else if (!_action.weapon->getArcingShot(_action.type))
	{
		// determine the target voxel.
		// aim at the center of the unit, the object, the walls or the floor (in that priority)
		// if there is no LOF to the center, try elsewhere (more outward).
		// Store this target voxel.
		Tile *targetTile = _parent->getSave()->getTile(_action.target);
		Position originVoxel = _parent->getTileEngine()->getOriginVoxel(_action, _parent->getSave()->getTile(_origin));
		if (targetTile->getUnit() &&
			((_unit->getFaction() != FACTION_PLAYER) ||
			targetTile->getUnit()->getVisible()))
		{
			if (_origin == _action.target || targetTile->getUnit() == _unit)
			{
				// don't shoot at yourself but shoot at the floor
				_targetVoxel = _action.target.toVoxel() + Position(8, 8, 0);
			}
			else
			{
				bool foundLoF = _parent->getTileEngine()->canTargetUnit(&originVoxel, targetTile, &_targetVoxel, _unit, isPlayer);

				if (!foundLoF && Options::oxceEnableOffCentreShooting)
				{
					// If we can't target from the standard shooting position, try a bit left and right from the centre.
					for (auto& rel_pos : { BattleActionOrigin::LEFT, BattleActionOrigin::RIGHT })
					{
						_action.relativeOrigin = rel_pos;
						originVoxel = _parent->getTileEngine()->getOriginVoxel(_action, _parent->getSave()->getTile(_origin));
						foundLoF = _parent->getTileEngine()->canTargetUnit(&originVoxel, targetTile, &_targetVoxel, _unit, isPlayer);
						if (foundLoF)
						{
							break;
						}
					}
				}

				if (!foundLoF)
				{
					// Failed to find LOF
					_action.relativeOrigin = BattleActionOrigin::CENTRE; // reset to the normal origin

					_targetVoxel = TileEngine::invalid.toVoxel(); // out of bounds, even after voxel to tile calculation.
					if (isPlayer)
					{
						forceEnableObstacles = true;
					}
				}
			}
		}
		else if (targetTile->getMapData(O_OBJECT) != 0)
		{
			if (!_parent->getTileEngine()->canTargetTile(&originVoxel, targetTile, O_OBJECT, &_targetVoxel, _unit, isPlayer))
			{
				_targetVoxel = _action.target.toVoxel() + Position(8, 8, 10);
			}
		}
		else if (targetTile->getMapData(O_NORTHWALL) != 0)
		{
			if (!_parent->getTileEngine()->canTargetTile(&originVoxel, targetTile, O_NORTHWALL, &_targetVoxel, _unit, isPlayer))
			{
				_targetVoxel = _action.target.toVoxel() + Position(8, 0, 9);
			}
		}
		else if (targetTile->getMapData(O_WESTWALL) != 0)
		{
			if (!_parent->getTileEngine()->canTargetTile(&originVoxel, targetTile, O_WESTWALL, &_targetVoxel, _unit, isPlayer))
			{
				_targetVoxel = _action.target.toVoxel() + Position(0, 8, 9);
			}
		}
		else if (targetTile->getMapData(O_FLOOR) != 0)
		{
			if (!_parent->getTileEngine()->canTargetTile(&originVoxel, targetTile, O_FLOOR, &_targetVoxel, _unit, isPlayer))
			{
				_targetVoxel = _action.target.toVoxel() + Position(8, 8, 2);
			}
		}
		else
		{
			// dummy attempt (only to highlight obstacles)
			_parent->getTileEngine()->canTargetTile(&originVoxel, targetTile, MapData::O_DUMMY, &_targetVoxel, _unit, isPlayer);

			// target nothing, targets the middle of the tile
			_targetVoxel = _action.target.toVoxel() + TileEngine::voxelTileCenter;
		}
	}

	if (createNewProjectile())
	{
		auto* conf = weapon->getActionConf(_action.type);
		if (_parent->getMap()->isAltPressed() || (conf && !conf->followProjectiles))
		{
			// temporarily turn off camera following projectiles to prevent annoying flashing effects (e.g. on minigun-like weapons)
			_parent->getMap()->setFollowProjectile(false);
		}
		if (_range == 0) _action.spendTU();
		_parent->getMap()->setCursorType(CT_NONE);
		_parent->getMap()->getCamera()->stopMouseScrolling();
		_parent->getMap()->disableObstacles();
	}
	else if (isPlayer && (_targetVoxel.z >= 0 || forceEnableObstacles))
	{
		_parent->getMap()->enableObstacles();
	}
}

/**
 * Tries to create a projectile sprite and add it to the map,
 * calculating its trajectory.
 * @return True, if the projectile was successfully created.
 */
bool ProjectileFlyBState::createNewProjectile()
{
	++_action.autoShotCounter;

	// Special handling for "spray" auto-attack, get target positions from the action's waypoints, starting from the back
	if (_action.sprayTargeting)
	{
		// Since we're just spraying, target the middle of the tile
		_targetVoxel = _action.waypoints.back();
		Position targetPosition = _targetVoxel.toTile();

		// The waypoint targeting is possibly out of range of the gun, so move the voxel to the max range of the gun if it is
		int distanceSq = _action.actor->distance3dToPositionSq(targetPosition);
		if (_action.weapon->getRules()->isOutOfRange(distanceSq))
		{
			Position actorPosition = _action.actor->getPosition();
			int maxRange = _action.weapon->getRules()->getMaxRange();
			int distance = (int)std::ceil(sqrt(float(distanceSq)));
			_targetVoxel = (actorPosition + (targetPosition - actorPosition) * maxRange / distance).toVoxel() + TileEngine::voxelTileCenter;
			targetPosition = _targetVoxel.toTile();
		}

		// Turn at the end (to a potentially modified target position)
		_unit->lookAt(targetPosition, _unit->getTurretType() != -1);
		while (_unit->getStatus() == STATUS_TURNING)
		{
			_unit->turn(_unit->getTurretType() != -1);
		}

		_action.waypoints.pop_back();
	}

	// create a new projectile
	Projectile *projectile = new Projectile(_parent->getMod(), _parent->getSave(), _action, _origin, _targetVoxel, _ammo);

	// add the projectile on the map
	_parent->getMap()->addProjectile(projectile);

	// set the speed of the state think cycle to 16 ms (roughly one think cycle per frame)
	_parent->setStateInterval(1000/60);

	// let it calculate a trajectory
	_projectileImpact = V_EMPTY;

	double accuracyDivider = 100.0;
	// berserking units are half as accurate
	if (!_parent->getPanicHandled())
	{
		accuracyDivider = 200.0;
	}

	BattleActionAttack attack = BattleActionAttack::GetAferShoot(_action, _ammo);
	if (_action.type == BA_THROW)
	{
		_projectileImpact = projectile->calculateThrow(BattleUnit::getFiringAccuracy(attack, _parent->getMod()) / accuracyDivider);
		const RuleItem *ruleItem = _action.weapon->getRules();
		if (_projectileImpact == V_FLOOR || _projectileImpact == V_UNIT || _projectileImpact == V_OBJECT)
		{
			if (_unit->getFaction() != FACTION_PLAYER && ruleItem->isGrenadeOrProxy())
			{
				_action.weapon->setFuseTimer(ruleItem->getFuseTimerDefault());
			}
			_action.weapon->moveToOwner(nullptr);
			if (_action.weapon->getGlow())
			{
				_parent->getTileEngine()->calculateLighting(LL_UNITS, _unit->getPosition());
				_parent->getTileEngine()->calculateFOV(_unit->getPosition(), _action.weapon->getGlowRange(), false);
			}
			_parent->getMod()->getSoundByDepth(_parent->getDepth(), Mod::ITEM_THROW)->play(-1, _parent->getMap()->getSoundAngle(_unit->getPosition()));
			if (!Mod::EXTENDED_EXPERIENCE_AWARD_SYSTEM)
			{
				// vanilla compatibility (throwing anything anywhere gives throwing exp)
				_unit->addThrowingExp();
			}
		}
		else
		{
			// unable to throw here
			_parent->getMap()->removeProjectile(projectile);
			_action.result = "STR_UNABLE_TO_THROW_HERE";
			_action.clearTU();
			_parent->popState();
			return false;
		}
	}
	else if (_action.weapon->getArcingShot(_action.type)) // special code for the "spit" trajectory
	{
		_projectileImpact = projectile->calculateThrow(BattleUnit::getFiringAccuracy(attack, _parent->getMod()) / accuracyDivider);
		if (_projectileImpact != V_EMPTY && _projectileImpact != V_OUTOFBOUNDS)
		{
			// set the soldier in an aiming position
			_unit->aim(true);
			// and we have a lift-off
			if (_ammo->getRules()->getFireSound() != Mod::NO_SOUND)
			{
				_parent->getMod()->getSoundByDepth(_parent->getDepth(), _ammo->getRules()->getFireSound())->play(-1, _parent->getMap()->getSoundAngle(_unit->getPosition()));
			}
			else if (_action.weapon->getRules()->getFireSound() != Mod::NO_SOUND)
			{
				_parent->getMod()->getSoundByDepth(_parent->getDepth(), _action.weapon->getRules()->getFireSound())->play(-1, _parent->getMap()->getSoundAngle(_unit->getPosition()));
			}
			if (_action.type != BA_LAUNCH)
			{
				_action.weapon->spendAmmoForAction(_action.type, _parent->getSave());
			}
		}
		else
		{
			// no line of fire
			_parent->getMap()->removeProjectile(projectile);
			if (_parent->getPanicHandled())
			{
				_action.result = "STR_NO_TRAJECTORY";
			}
			_unit->abortTurn();
			_parent->popState();
			return false;
		}
	}
	else
	{
		if (_originVoxel != TileEngine::invalid)
		{
			_projectileImpact = projectile->calculateTrajectory(BattleUnit::getFiringAccuracy(attack, _parent->getMod()) / accuracyDivider, _originVoxel, false);
		}
		else
		{
			_projectileImpact = projectile->calculateTrajectory(BattleUnit::getFiringAccuracy(attack, _parent->getMod()) / accuracyDivider);
		}
		if (_targetVoxel != TileEngine::invalid.toVoxel() && (_projectileImpact != V_EMPTY || _action.type == BA_LAUNCH))
		{
			// set the soldier in an aiming position
			_unit->aim(true);
			// and we have a lift-off
			if (_ammo->getRules()->getFireSound() != Mod::NO_SOUND)
			{
				_parent->getMod()->getSoundByDepth(_parent->getDepth(), _ammo->getRules()->getFireSound())->play(-1, _parent->getMap()->getSoundAngle(projectile->getOrigin()));
			}
			else if (_action.weapon->getRules()->getFireSound() != Mod::NO_SOUND)
			{
				_parent->getMod()->getSoundByDepth(_parent->getDepth(), _action.weapon->getRules()->getFireSound())->play(-1, _parent->getMap()->getSoundAngle(projectile->getOrigin()));
			}
			if (_action.type != BA_LAUNCH)
			{
				_action.weapon->spendAmmoForAction(_action.type, _parent->getSave());
			}
		}
		else
		{
			// no line of fire
			_parent->getMap()->removeProjectile(projectile);
			if (_parent->getPanicHandled())
			{
				_action.result = "STR_NO_LINE_OF_FIRE";
			}
			_unit->abortTurn();
			_parent->popState();
			return false;
		}
	}

	if (_action.type != BA_THROW && _action.type != BA_LAUNCH)
		_unit->getStatistics()->shotsFiredCounter++;

	// hit log - new bullet
	if (_action.actor)
	{
		_parent->getSave()->appendToHitLog(HITLOG_NEW_SHOT, _action.actor->getFaction());
	}

	// combat log - one line per action (not per burst shot): "throws" for throws, "fires" otherwise
	if (_action.autoShotCounter == 1)
	{
		if (_action.type == BA_THROW)
		{
			_parent->getSave()->logThrowEvent(_action.actor, _action.weapon);
		}
		else
		{
			// resolve the (weapon-customizable) shot-mode label for the combat log
			std::string shotType;
			switch (_action.type)
			{
			case BA_SNAPSHOT:  shotType = _action.weapon->getRules()->getConfigSnap()->name; break;
			case BA_AIMEDSHOT: shotType = _action.weapon->getRules()->getConfigAimed()->name; break;
			case BA_AUTOSHOT:  shotType = _action.weapon->getRules()->getConfigAuto()->name; break;
			case BA_LAUNCH:    shotType = "STR_LAUNCH_MISSILE"; break;
			default: break;
			}
			_parent->getSave()->logFireEvent(_action.actor, _action.weapon, _action.reaction, shotType);
		}
	}

	// combat log - notify (once) when this shot emptied the weapon
	if (_action.type != BA_THROW && _action.type != BA_LAUNCH && !_action.weapon->getAmmoForAction(_action.type))
	{
		_parent->getSave()->logOutOfAmmoEvent(_action.actor, _action.weapon);
	}

	// remember this projectile's own impact type, so overlapping shots each resolve correctly
	projectile->setImpact(_projectileImpact);

	// Shotgun: launch the remaining pellets as real, concurrently-flying projectiles
	// (instead of instantly tracing them at impact). Each pellet then spreads, flies, and
	// resolves its own impact via the normal projectile path in think().
	if (_action.type != BA_THROW && _action.type != BA_LAUNCH &&
		_ammo && _ammo->getRules()->getShotgunPellets() != 0 && _ammo->getRules()->getDamageType()->isDirect())
	{
		const int pelletCount = _ammo->getRules()->getShotgunPellets();
		const int behaviorType = _ammo->getRules()->getShotgunBehaviorType();
		const int spread = _ammo->getRules()->getShotgunSpread();
		const int choke = _action.weapon->getRules()->getShotgunChoke();
		const Position originalTarget = _targetVoxel;
		// the lead pellet's actual landing voxel (from its just-computed trajectory),
		// used to re-center the spread for behaviorType 1
		const Position leadImpact = projectile->getImpactPosition(-2);
		const Position firingOriginVoxel = _parent->getSave()->getTileEngine()->getOriginVoxel(_action, _parent->getSave()->getTile(_origin));

		for (int i = 1; i < pelletCount; ++i)
		{
			Position pelletTarget = originalTarget;
			if (behaviorType == 1)
			{
				// spread around the lead pellet's impact (unless it landed on the firing voxel)
				pelletTarget = (leadImpact != firingOriginVoxel) ? leadImpact : originalTarget;
			}

			Projectile *pellet = new Projectile(_parent->getMod(), _parent->getSave(), _action, _origin, pelletTarget, _ammo);
			int pelletImpact;
			if (behaviorType == 1)
			{
				// pellet spread based on spread and choke values
				pelletImpact = pellet->calculateTrajectory(std::max(0.0, (1.0 - spread / 100.0) * choke / 100.0));
			}
			else
			{
				// pellet spread based on firing accuracy with a diminishing per-pellet formula
				// (identical to the vanilla formula when spread = 100)
				pelletImpact = pellet->calculateTrajectory(std::max(0.0, (BattleUnit::getFiringAccuracy(attack, _parent->getMod()) / 100.0) - i * 5.0 * spread / 100.0));
			}

			if (pelletImpact == V_EMPTY)
			{
				// no line of fire for this pellet; drop it
				delete pellet;
				continue;
			}
			pellet->setImpact(pelletImpact);
			_parent->getMap()->addProjectile(pellet);
		}
	}

	// schedule the next burst/spray shot on a timer (cosmetic cadence, tunable per weapon).
	// think() runs at ~1000/60 ms per cycle, so convert the millisecond interval into think-cycles.
	_shotCooldown = (_action.weapon->getRules()->getFireInterval() * 60 + 500) / 1000;
	if (_shotCooldown < 1) _shotCooldown = 1;

	return true;
}

/**
 * Deinitialize the state.
 */
void ProjectileFlyBState::deinit()
{
	_parent->getMap()->setFollowProjectile(true); // turn back on when done shooting
}

/**
 * Animates the projectile (moves to the next point in its trajectory).
 * If the animation is finished the projectile sprite is removed from the map,
 * and this state is finished.
 */
void ProjectileFlyBState::think()
{
	/// checks if a weapon has any more shots to fire.
	auto noMoreShotsToShoot = [this]() { return !_action.weapon->haveNextShotsForAction(_action.type, _action.autoShotCounter) || !_action.weapon->getAmmoForAction(_action.type); };

	// Re-assert our think cadence every pass. A resolved impact runs an ExplosionBState
	// (which sets its own, slower state interval); when it pops and we resume to advance
	// any still-airborne projectiles, the interval would otherwise be left at the
	// explosion's speed, making the remaining bullets fly at the wrong rate.
	_parent->setStateInterval(1000/60);

	_parent->getSave()->getBattleState()->clearMouseScrollingState();

	// Set true if any projectile impacts this pass. An impact resolves by pushing an
	// ExplosionBState (or, for BA_LAUNCH, the next waypoint state) to the FRONT of the
	// queue, which makes that state - not this one - the queue front. We must therefore
	// not create another projectile or run the finish-up logic in the same pass, because
	// _parent->popState() (called directly here on finish, or indirectly by
	// createNewProjectile() on a failed shot) would pop the freshly-pushed state instead
	// of this one, leaving explosions unfinished and firing erratic. We bail out and
	// resume on a later think() once the pushed state has run.
	bool impactResolvedThisPass = false;

	// First, advance and resolve every projectile already in flight.
	if (_parent->getMap()->hasProjectiles())
	{
		BattleActionAttack attack = BattleActionAttack::GetAferShoot(_action, _ammo);
		// snapshot the list: removeProjectile() modifies the live vector, so iterate a copy
		const std::vector<Projectile*> inFlight(_parent->getMap()->getProjectiles());
		for (Projectile* flyingProj : inFlight)
		{
			if (!flyingProj->move())
			{
				// The trajectory (and its impact point) is precomputed at fire time. With
				// several rounds airborne at once, an earlier impact may have destroyed the
				// obstacle this round was going to hit. Re-trace straight shots against the
				// current terrain before committing: if the path now extends past the old
				// impact point (the obstacle is gone), keep the round flying and let it
				// resolve on a later pass instead of detonating on a wall that no longer exists.
				if (_action.type != BA_THROW && !_action.weapon->getArcingShot(_action.type)
					&& flyingProj->recalculateImpact())
				{
					continue;
				}
				// impact !
				int impact = flyingProj->getImpact();
				if (_action.type == BA_THROW)
				{
					// A throw is a single action that ends after it lands; pushing the
					// resulting drop/explosion to the front means we must yield this pass.
					impactResolvedThisPass = true;
					_parent->getMap()->resetCameraSmoothing();
					Position pos = flyingProj->getPosition(Projectile::ItemDropVoxelOffset).toTile();
					if (pos.y > _parent->getSave()->getMapSizeY())
					{
						pos.y--;
					}
					if (pos.x > _parent->getSave()->getMapSizeX())
					{
						pos.x--;
					}

					_parent->getMod()->getSoundByDepth(_parent->getDepth(), Mod::ITEM_DROP)->play(-1, _parent->getMap()->getSoundAngle(pos));
					const RuleItem *ruleItem = _action.weapon->getRules();
					if (_action.weapon->fuseThrowEvent())
					{
						if (ruleItem->getBattleType() == BT_GRENADE || ruleItem->getBattleType() == BT_PROXIMITYGRENADE)
						{
							// it's a hot grenade to explode immediately
							_parent->statePushFront(new ExplosionBState(_parent, flyingProj->getLastPositions(Projectile::ItemDropVoxelOffset), attack));
						}
						else
						{
							_parent->getSave()->removeItem(_action.weapon);
						}
					}
					else
					{
						_parent->dropItem(pos, _action.weapon);
						if (_unit->getFaction() != FACTION_PLAYER && ruleItem->isGrenadeOrProxy())
						{
							_parent->getTileEngine()->setDangerZone(pos, ruleItem->getExplosionRadius(attack), _action.actor);
						}
					}
				}
				else if (_action.type == BA_LAUNCH && _action.waypoints.size() > 1 && impact == V_EMPTY)
				{
					// The waypoint cascade hands off to a fresh fly state; yield this pass.
					impactResolvedThisPass = true;
					_origin = _action.waypoints.front();
					_action.waypoints.pop_front();
					_action.target = _action.waypoints.front();
					// launch the next projectile in the waypoint cascade
					ProjectileFlyBState *nextWaypoint = new ProjectileFlyBState(_parent, _action, _origin, _range + flyingProj->getDistance());
					nextWaypoint->setOriginVoxel(flyingProj->getPosition(-1));
					if (_origin == _action.target)
					{
						nextWaypoint->targetFloor();
					}
					_parent->statePushNext(nextWaypoint);
				}
				else
				{
					auto* tmpUnit = _parent->getSave()->getTile(_action.target)->getUnit();
					if (tmpUnit && tmpUnit != _unit)
					{
						tmpUnit->getStatistics()->shotAtCounter++; // Only counts for guns, not throws or launches
					}

					_parent->getMap()->resetCameraSmoothing();
					if (_action.type == BA_LAUNCH)
					{
						_action.weapon->spendAmmoForAction(_action.type, _parent->getSave());
						// combat log - launch ammo is spent here (not in createNewProjectile); report if it ran dry
						if (!_action.weapon->getAmmoForAction(_action.type))
						{
							_parent->getSave()->logOutOfAmmoEvent(_action.actor, _action.weapon);
						}
					}

					if (impact != V_OUTOFBOUNDS)
					{
						int offset = 0;
						// explosions impact not inside the voxel but two steps back (projectiles generally move 2 voxels at a time)
						if (_ammo && _ammo->getRules()->getExplosionRadius(attack) != 0 && impact != V_UNIT)
						{
							offset = -2;
						}

						// Every projectile - including shotgun pellets, which are now real
						// concurrently-flying projectiles - resolves its own impact using its
						// own travelled distance for range-based power falloff.
						// Push the explosion as a CONCURRENT state so it animates alongside
						// the rest of the volley instead of freezing every round in flight.
						// statePushConcurrent() marks the state concurrent itself.
						_parent->statePushConcurrent(new ExplosionBState(
							_parent, flyingProj->getLastPositions(offset),
							attack, 0,
							noMoreShotsToShoot(),
							_range + flyingProj->getDistance()
						));

						if (impact == V_UNIT)
						{
							projectileHitUnit(flyingProj->getPosition(offset));
						}
					}
					else if (noMoreShotsToShoot())
					{
						_unit->aim(false);
					}
				}

				_parent->getMap()->removeProjectile(flyingProj);
			}
		} // end for (flyingProj)
	}

	// A throw or a launch-waypoint handoff pushed a state to the queue FRONT this pass;
	// yield so it can run. Firearm/AoE impacts now spawn CONCURRENT explosions (which do
	// not take the front), so they fall through and the volley keeps firing/finishing.
	if (impactResolvedThisPass)
	{
		return;
	}

	// Tick down the inter-shot cooldown, then fire the next shot on a timer -
	// independently of whether earlier shots are still airborne.
	if (_shotCooldown > 0)
	{
		--_shotCooldown;
	}

	bool hasFloor = _action.actor->haveNoFloorBelow() == false;
	bool unitCanFly = _action.actor->getMovementType() == MT_FLY;
	bool canFireMore =
		_action.weapon->haveNextShotsForAction(_action.type, _action.autoShotCounter)
		&& !_action.actor->isOut()
		&& _ammo->getAmmoQuantity() != 0
		&& (hasFloor || unitCanFly);

	if (canFireMore)
	{
		if (_shotCooldown <= 0)
		{
			createNewProjectile();
			if (_action.cameraPosition.z != -1)
			{
				_parent->getMap()->getCamera()->setMapOffset(_action.cameraPosition);
				_parent->getMap()->invalidate();
			}
		}
		// still shooting (or waiting for the cadence timer) - don't wrap up yet
		return;
	}

	// No more shots queued: finish up once every projectile has left the field.
	if (!_parent->getMap()->hasProjectiles())
	{
		if (_action.cameraPosition.z != -1 && _action.waypoints.size() <= 1)
		{
			_parent->getMap()->getCamera()->setMapOffset(_action.cameraPosition);
			_parent->getMap()->invalidate();
		}
		if (!_parent->getSave()->getUnitsFalling() && _parent->getPanicHandled())
		{
			_parent->getTileEngine()->checkReactionFire(_unit, _action);
		}
		if (!_unit->isOut())
		{
			_unit->abortTurn();
		}
		if (_parent->getSave()->getSide() == FACTION_PLAYER || _parent->getSave()->getDebugMode())
		{
			_parent->setupCursor();
		}
		_parent->convertInfected();
		_parent->popState();
	}
}

/**
 * Flying projectiles cannot be cancelled,
 * but they can be "skipped".
 */
void ProjectileFlyBState::cancel()
{
	const auto& projectiles = _parent->getMap()->getProjectiles();
	for (Projectile* proj : projectiles)
		proj->skipTrajectory();
	if (!projectiles.empty())
	{
		Position p = projectiles[0]->getPosition().toTile();
		if (!_parent->getMap()->getCamera()->isOnScreen(p, false, 0, false))
			_parent->getMap()->getCamera()->centerOnPosition(p);
	}
	if (_parent->areAllEnemiesNeutralized())
	{
		// stop autoshots when battle auto-ends
		_action.autoShotCounter = 1000;

		// Rationale: if there are any fatally wounded soldiers
		// the game still allows the player to resume playing the current turn (and heal them)
		// but we don't want to resume auto-shooting (it just looks silly)
	}
}

/**
 * Validates the throwing range.
 * @param action Pointer to throw action.
 * @param origin Position to throw from.
 * @param target Tile to throw to.
 * @param depth Battlescape depth.
 * @return True when the range is valid.
 */
bool ProjectileFlyBState::validThrowRange(BattleAction *action, Position origin, Tile *target, int depth)
{
	// note that all coordinates and thus also distances below are in number of tiles (not in voxels).
	if (action->type != BA_THROW)
	{
		return true;
	}
	int xdiff = action->target.x - action->actor->getPosition().x;
	int ydiff = action->target.y - action->actor->getPosition().y;
	int realDistanceSq = (xdiff * xdiff) + (ydiff * ydiff);

	int compatibilityDistanceSq = action->actor->distance3dToPositionSq(action->target); // 3d distance for compatibility with Map::drawTerrain()
	if (action->weapon->getRules()->isOutOfThrowRange(compatibilityDistanceSq, depth))
	{
		// if out of item's throw range, stop... no need to check weight- and strength-based range
		return false;
	}

	double realDistance = sqrt((double)realDistanceSq);

	int offset = 2;
	int zd = (origin.z)-((action->target.z * 24 + offset) - target->getTerrainLevel());
	int weight = action->weapon->getTotalWeight();
	double maxDistance = (getMaxThrowDistance(weight, action->actor->getBaseStats()->strength, zd) + 8) / 16.0;

	if (depth > 0 && Mod::EXTENDED_UNDERWATER_THROW_FACTOR > 0)
	{
		maxDistance = maxDistance * (double)Mod::EXTENDED_UNDERWATER_THROW_FACTOR / 100.0;
	}

	return realDistance <= maxDistance;
}

/**
 * Validates the throwing range.
 * @param weight the weight of the object.
 * @param strength the strength of the thrower.
 * @param level the difference in height between the thrower and the target.
 * @return the maximum throwing range.
 */
int ProjectileFlyBState::getMaxThrowDistance(int weight, int strength, int level)
{
	double curZ = level + 0.5;
	double dz = 1.0;
	int dist = 0;
	while (dist < 4000) //just in case
	{
		dist += 8;
		if (dz<-1)
			curZ -= 8;
		else
			curZ += dz * 8;

		if (curZ < 0 && dz < 0) //roll back
		{
			dz = std::max(dz, -1.0);
			if (std::abs(dz)>1e-10) //rollback horizontal
				dist -= curZ / dz;
			break;
		}
		dz -= (double)(50 * weight / strength)/100;
		if (dz <= -2.0) //become falling
			break;
	}
	return dist;
}

/**
 * Set the origin voxel, used for the blaster launcher.
 * @param pos the origin voxel.
 */
void ProjectileFlyBState::setOriginVoxel(const Position& pos)
{
	_originVoxel = pos;
}

/**
 * Set the boolean flag to angle a blaster bomb towards the floor.
 */
void ProjectileFlyBState::targetFloor()
{
	_targetFloor = true;
}

void ProjectileFlyBState::projectileHitUnit(Position pos)
{
	BattleUnit *victim = _parent->getSave()->getTile(pos.toTile())->getOverlappingUnit(_parent->getSave());
	BattleUnit *targetVictim = _parent->getSave()->getTile(_action.target)->getUnit(); // Who we were aiming at (not necessarily who we hit)
	if (victim && !victim->isOut())
	{
		victim->getStatistics()->hitCounter++;
		if (_unit->getOriginalFaction() == FACTION_PLAYER && victim->getOriginalFaction() == FACTION_PLAYER)
		{
			victim->getStatistics()->shotByFriendlyCounter++;
			_unit->getStatistics()->shotFriendlyCounter++;
		}
		if (victim == targetVictim) // Hit our target
		{
			int distanceSq = _action.actor->distance3dToUnitSq(victim);
			int distance = (int)std::ceil(sqrt(float(distanceSq)));
			int accuracy = BattleUnit::getFiringAccuracy(BattleActionAttack::GetAferShoot(_action, _ammo), _parent->getMod());

			{
				int upperLimit, lowerLimit;
				int dropoff = _action.weapon->getRules()->calculateLimits(upperLimit, lowerLimit, _parent->getSave()->getDepth(), _action.type);

				if (distance > upperLimit)
				{
					accuracy -= (distance - upperLimit) * dropoff;
				}
				else if (distance < lowerLimit)
				{
					accuracy -= (lowerLimit - distance) * dropoff;
				}
				if (accuracy < 0)
				{
					accuracy = 0;
				}
			}

			_unit->getStatistics()->shotsLandedCounter++;
			if (distance > 30)
			{
				_unit->getStatistics()->longDistanceHitCounter++;
			}
			if (accuracy < distance)
			{
				_unit->getStatistics()->lowAccuracyHitCounter++;
			}
		}
	}
}

}
