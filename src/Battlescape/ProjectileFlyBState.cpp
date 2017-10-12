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
#include "Pathfinding.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Mod/Mod.h"
#include "../Engine/Sound.h"
#include "../Engine/RNG.h"
#include "../Mod/RuleItem.h"
#include "../Engine/Options.h"
#include "AIModule.h"
#include "../Engine/Timer.h"
#include "Camera.h"
#include "Explosion.h"
#include "BattlescapeState.h"
#include "../Savegame/BattleUnitStatistics.h"
#include "../fmath.h"
#include "../Engine/Logger.h"

namespace OpenXcom
{

/**
 * Sets up an ProjectileFlyBState.
 */
ProjectileFlyBState::ProjectileFlyBState(BattlescapeGame *parent, BattleAction action, Position origin, int range) : BattleState(parent, action), _unit(0), _weapon(0), _ammo(0), _projectileItem(0), _origin(origin), _originVoxel(-1,-1,-1), _range(range), _initialized(false), _targetFloor(false), _autoShotTimer(0),  _dualFireTimer(0), _autoShotsRemaining(0), _explosionStates(), _subState(false), _subStateFinished(false), _dualState(0), _firstProjectile(true)
{
}

ProjectileFlyBState::ProjectileFlyBState(BattlescapeGame *parent, BattleAction action) : BattleState(parent, action), _unit(0), _weapon(0), _ammo(0), _projectileItem(0), _origin(action.actor->getPosition()), _originVoxel(-1,-1,-1), _range(0), _initialized(false), _targetFloor(false), _autoShotTimer(0),  _dualFireTimer(0), _autoShotsRemaining(0), _explosionStates(), _subState(false), _subStateFinished(false), _dualState(0), _firstProjectile(true)
{
}

ProjectileFlyBState::ProjectileFlyBState(BattlescapeGame *parent, BattleAction action, Position origin, bool subState) : BattleState(parent, action), _unit(0), _weapon(0), _ammo(0), _projectileItem(0), _origin(action.actor->getPosition()), _originVoxel(-1,-1,-1), _initialized(false), _targetFloor(false), _autoShotTimer(0),  _dualFireTimer(0), _autoShotsRemaining(0), _explosionStates(), _subState(subState), _subStateFinished(false), _dualState(0), _firstProjectile(!subState)
{
}

/**
 * Deletes the ProjectileFlyBState.
 */
ProjectileFlyBState::~ProjectileFlyBState()
{
	if(_autoShotTimer)
	{
		delete _autoShotTimer;
	}

	if(_dualFireTimer)
	{
		delete _dualFireTimer;
	}

	for(std::vector<ExplosionBState*>::const_iterator ii = _explosionStates.begin(); ii != _explosionStates.end(); ++ii)
	{
		delete *ii;
	}

	if(_dualState)
	{
		delete _dualState;
	}
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

	_weapon = _action.weapon;
	BattleItem *weapon2 = 0;

	BattleActionType dualAction1;
	BattleActionType dualAction2;

	if(_action.type == BA_DUALFIRE && !_action.actor->canDualFire(_weapon, dualAction1, weapon2, dualAction2))
	{
		popParentState();
		return;
	}

	_projectileItem = 0;

	if (!_weapon) // can't shoot without weapon
	{
		popParentState();
		return;
	}

	if (!_parent->getSave()->getTile(_action.target)) // invalid target position
	{
		popParentState();
		return;
	}

	//test TU only on first lunch waypoint or normal shoot
	if (_range == 0 && !_action.haveTU(&_action.result))
	{
		_parent->popState();
		return;
	}

	_unit = _action.actor;

	if (_unit->isOut() || _unit->getHealth() <= 0 || _unit->getHealth() < _unit->getStunlevel())
	{
		// something went wrong - we can't shoot when dead or unconscious, or if we're about to fall over.
		popParentState();
		return;
	}

	BattleAction action2;

	if(_action.type == BA_DUALFIRE)
	{
		_ammo = _weapon->getAmmoForAction(dualAction1);
		BattleItem *ammo2 = weapon2 ? weapon2->getAmmoForAction(dualAction2) : 0;
		if (!_ammo || !ammo2)
		{
			_action.result = "STR_NO_AMMUNITION_LOADED";
			popParentState();
			return;
		}
		if (_ammo->getAmmoQuantity() == 0 || ammo2->getAmmoQuantity() == 0)
		{
			_action.result = "STR_NO_ROUNDS_LEFT";
			popParentState();
			return;
		}

		if (_parent->getTileEngine()->distance(_action.actor->getPosition(), _action.target) > _weapon->getRules()->getMaxRange()
			|| _parent->getTileEngine()->distance(_action.actor->getPosition(), _action.target) > weapon2->getRules()->getMaxRange())
		{
			// out of range
			_action.result = "STR_OUT_OF_RANGE";
			popParentState();
			return;
		}

		action2 = _action;
		action2.type = dualAction2;
		action2.weapon = weapon2;
		action2.dualFiring = true;

		_action.type = dualAction1;
		_action.weapon = _weapon;
		_action.dualFiring = true;
	}
	else
	{
		_ammo = _weapon->getAmmoForAction(_action.type);
	}

	// reaction fire
	if (_unit->getFaction() != _parent->getSave()->getSide())
	{
		// no ammo or target is dead: give the time units back and cancel the shot.
		if (_ammo == 0
			|| !_parent->getSave()->getTile(_action.target)->getUnit()
			|| _parent->getSave()->getTile(_action.target)->getUnit()->isOut()
			|| _parent->getSave()->getTile(_action.target)->getUnit() != _parent->getSave()->getSelectedUnit())
		{
			if (_action.overwatch)
			{
				_unit->cancelOverwatchShot();
			}
			else
			{
				_unit->setTimeUnits(_unit->getTimeUnits() + _unit->getActionTUs(_action.type, _weapon).Time);
			}
			popParentState();
			return;
		}
		_unit->lookAt(_action.target, _unit->getTurretType() != -1);
		while (_unit->getStatus() == STATUS_TURNING)
		{
			_unit->turn();
		}
	}
	
	Tile *endTile = _parent->getSave()->getTile(_action.target);
	int distance = _parent->getTileEngine()->distance(_action.actor->getPosition(), _action.target);
	switch (_action.type)
	{
	case BA_AUTOSHOT:
	case BA_BURSTSHOT:
		_autoShotsRemaining = _weapon->getActionConf(_action.type)->shots;
	case BA_SNAPSHOT:
	case BA_AIMEDSHOT:
	case BA_LAUNCH:
		if (_ammo == 0)
		{
			_action.result = "STR_NO_AMMUNITION_LOADED";
			popParentState();
			return;
		}
		if (_ammo->getAmmoQuantity() == 0)
		{
			_action.result = "STR_NO_ROUNDS_LEFT";
			popParentState();
			return;
		}
		if (distance > _weapon->getRules()->getMaxRange())
		{
			// special handling for short ranges and diagonals
			if (_action.actor->directionTo(_action.target) % 2 == 1)
			{
				// special handling for maxRange 1: allow it to target diagonally adjacent tiles, even though they are technically 2 tiles away.
				if (_weapon->getRules()->getMaxRange() == 1
					&& distance == 2)
				{
					break;
				}
				// special handling for maxRange 2: allow it to target diagonally adjacent tiles on a level above/below, even though they are technically 3 tiles away.
				else if (_weapon->getRules()->getMaxRange() == 2
					&& distance == 3
					&& _action.target.z != _action.actor->getPosition().z)
				{
					break;
				}
			}
			// out of range
			_action.result = "STR_OUT_OF_RANGE";
			popParentState();
			return;
		}
		break;
	case BA_THROW:
		if (!validThrowRange(&_action, _parent->getTileEngine()->getOriginVoxel(_action, 0), _parent->getSave()->getTile(_action.target)))
		{
			// out of range
			_action.result = "STR_OUT_OF_RANGE";
			popParentState();
			return;
		}
		if (endTile &&
			endTile->getTerrainLevel() == -24 &&
			endTile->getPosition().z + 1 < _parent->getSave()->getMapSizeZ())
		{
			_action.target.z += 1;
		}
		_projectileItem = _weapon;
		break;
	default:
		popParentState();
		return;
	}
	
	if(action2.type != BA_NONE)
	{
		_dualState = new ProjectileFlyBState(_parent, action2, _origin, true);
	}

	if (_action.type == BA_LAUNCH || (Options::forceFire && (SDL_GetModState() & KMOD_CTRL) != 0 && _parent->getSave()->getSide() == FACTION_PLAYER) || !_parent->getPanicHandled())
	{
		// target nothing, targets the middle of the tile
		_targetVoxel = Position(_action.target.x*16 + 8, _action.target.y*16 + 8, _action.target.z*24 + 12);
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
	else
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
				_targetVoxel = Position(_action.target.x*16 + 8, _action.target.y*16 + 8, _action.target.z*24);
			}
			else
			{
				if (!_parent->getTileEngine()->canTargetUnit(&originVoxel, targetTile, &_targetVoxel, _unit))
				{
					_targetVoxel = Position(-16,-16,-24); // out of bounds, even after voxel to tile calculation.
				}
			}
		}
		else if (targetTile->getMapData(O_OBJECT) != 0)
		{
			if (!_parent->getTileEngine()->canTargetTile(&originVoxel, targetTile, O_OBJECT, &_targetVoxel, _unit))
			{
				_targetVoxel = Position(_action.target.x*16 + 8, _action.target.y*16 + 8, _action.target.z*24 + 10);
			}
		}
		else if (targetTile->getMapData(O_NORTHWALL) != 0)
		{
			if (!_parent->getTileEngine()->canTargetTile(&originVoxel, targetTile, O_NORTHWALL, &_targetVoxel, _unit))
			{
				_targetVoxel = Position(_action.target.x*16 + 8, _action.target.y*16, _action.target.z*24 + 9);
			}
		}
		else if (targetTile->getMapData(O_WESTWALL) != 0)
		{
			if (!_parent->getTileEngine()->canTargetTile(&originVoxel, targetTile, O_WESTWALL, &_targetVoxel, _unit))
			{
				_targetVoxel = Position(_action.target.x*16, _action.target.y*16 + 8, _action.target.z*24 + 9);
			}
		}
		else if (targetTile->getMapData(O_FLOOR) != 0)
		{
			if (!_parent->getTileEngine()->canTargetTile(&originVoxel, targetTile, O_FLOOR, &_targetVoxel, _unit))
			{
				_targetVoxel = Position(_action.target.x*16 + 8, _action.target.y*16 + 8, _action.target.z*24 + 2);
			}
		}
		else
		{
			// target nothing, targets the middle of the tile
			_targetVoxel = Position(_action.target.x*16 + 8, _action.target.y*16 + 8, _action.target.z*24 + 12);
		}
	}

	_originalTarget = _targetVoxel;

	int autoDelay = 0;

	if(_action.type == BA_AUTOSHOT || _action.type == BA_BURSTSHOT)
	{
		if(!(autoDelay = _weapon->getRules()->getAutoDelay()) && !(autoDelay = _weapon->getAmmoForAction(_action.type)->getRules()->getAutoDelay()))
		{
			autoDelay = 250;
		}
	}

	if(_subState)
	{
		_dualFireTimer = new Timer(100, true);
		_dualFireTimer->onTimer((BattleStateHandler)&ProjectileFlyBState::onDualFireTimer);
		_dualFireTimer->start();
	}
	else if(createNewProjectile())
	{
		if (_range == 0) _action.spendTU();
		_parent->getMap()->setCursorType(CT_NONE);
		_parent->getMap()->getCamera()->stopMouseScrolling();
		if(_action.type == BA_AUTOSHOT || _action.type == BA_BURSTSHOT)
		{
			_autoShotTimer = new Timer(autoDelay, true);
			_autoShotTimer->onTimer((BattleStateHandler)&ProjectileFlyBState::onAutoshotTimer);
			_autoShotTimer->start();
		}
		else if(_subState)
		{
			_subStateFinished = true;
		}
	}
	else if(_subState)
	{
		_subStateFinished = true;
	}

	if(_dualState)
	{
		_dualState->init();
	}
}

/**
 * Fires the next autoshot projectile.
 */
void ProjectileFlyBState::onAutoshotTimer()
{
	if((_weapon->haveAnyAmmo() && (!_weapon->getAmmoForAction(_action.type) || !_weapon->getAmmoForAction(_action.type)->getAmmoQuantity())) || !createNewProjectile() || !_autoShotsRemaining)
	{
		_autoShotTimer->stop();
		if(_subState)
		{
			_subStateFinished = true;
		}
	}
}

/**
 * Starts firing the secondary weapon projectiles.
 */
void ProjectileFlyBState::onDualFireTimer()
{
	if(createNewProjectile())
	{
		if(_action.type == BA_AUTOSHOT || _action.type == BA_BURSTSHOT)
		{
			int autoDelay = 0;

			if(!(autoDelay = _weapon->getRules()->getAutoDelay()) && !(autoDelay = _weapon->getAmmoForAction(_action.type)->getRules()->getAutoDelay()))
			{
				autoDelay = 250;
			}

			_autoShotTimer = new Timer(autoDelay, true);
			_autoShotTimer->onTimer((BattleStateHandler)&ProjectileFlyBState::onAutoshotTimer);
			_autoShotTimer->start();
		}
		else
		{
			_subStateFinished = true;
		}
	}
	else
	{
		_subStateFinished = true;
	}

	_dualFireTimer->stop();
}

/**
 * Tries to create a projectile sprite and add it to the map,
 * calculating its trajectory.
 * @return True, if the projectile was successfully created.
 */
bool ProjectileFlyBState::createNewProjectile()
{
	++_action.autoShotCounter;

	--_autoShotsRemaining;
	
	//int bulletSprite = -1;
	int shots = 1;
	if (_action.type != BA_THROW)
	{
		/*bulletSprite = _ammo->getRules()->getBulletSprite();
		if (bulletSprite == -1)
		{
			bulletSprite = _weapon->getRules()->getBulletSprite();
		}*/
		shots = _ammo->getRules()->getShotgunPellets();
	}
	else
	{	//Is this for cluster grenades? They don't seem to work.
		shots = _weapon->getRules()->getShotgunPellets();
	}
	// create a new projectile
	//Projectile *projectile = new Projectile(_parent->getMod(), _parent->getSave(), _action, _origin, _targetVoxel, _ammo, bulletSprite, false);
	Projectile *projectile = 0;

	std::vector<Projectile*> newProjectiles;

	// add the projectile on the map
	bool shotgun = false;

	_targetVoxel = _originalTarget;

	double accuracyDivider = 100.0;
	// berserking units are half as accurate
	if (!_parent->getPanicHandled())
	{
		accuracyDivider = 200.0;
	}

	// Do not allow shotgun mode for thrown projectiles as the item becomes the projectile.
	if (shots < 2 || _action.type == BA_THROW) //&& _weapon->getRules()->getBattleType() != 4))
	{
		_parent->getMap()->addProjectile(projectile = new Projectile(_parent->getMod(), _parent->getSave(), _action, _origin, _targetVoxel, _ammo, false));
		newProjectiles.push_back(projectile);
	}
	else
	{
		shotgun = true;
		Projectile *accuracySource = new Projectile(_parent->getMod(), _parent->getSave(), _action, _origin, _targetVoxel, _ammo, false);
		int projectileImpact;

		if (_action.type == BA_THROW)
		{
			if (_unit->getFaction() != FACTION_PLAYER && _projectileItem->getRules()->getBattleType() == BT_GRENADE)
			{
				_projectileItem->setFuseTimer(0);
			}
			_projectileItem->moveToOwner(0);
			if (_action.weapon->getGlow())
			{
				_parent->getTileEngine()->calculateLighting(LL_UNITS, _unit->getPosition());
				_parent->getTileEngine()->calculateFOV(_unit->getPosition(), _action.weapon->getGlowRange(), false);
			}
			_parent->getMod()->getSoundByDepth(_parent->getDepth(), Mod::ITEM_THROW)->play(-1, _parent->getMap()->getSoundAngle(_unit->getPosition()));
			_unit->addThrowingExp();
			projectileImpact = accuracySource->calculateThrow(_unit->getThrowingAccuracy() / accuracyDivider);
		}
		else if (_weapon->getRules()->getArcingShot())
		{
			projectileImpact = accuracySource->calculateThrow(_unit->getFiringAccuracy(_action.type, _weapon, _parent->getMod(), shotgun, false) / accuracyDivider);
		}
		else if (_originVoxel != Position(-1,-1,-1))
		{
			projectileImpact = accuracySource->calculateTrajectoryFromVector(_originVoxel);
		}
		else
		{
			projectileImpact = accuracySource->calculateTrajectoryFromVector();
		}

		if(projectileImpact == V_EMPTY || ((_weapon->getRules()->getArcingShot() || _action.type == BA_THROW) && !(projectileImpact == V_FLOOR || projectileImpact == V_UNIT || projectileImpact == V_OBJECT)))
		{
			delete accuracySource;

			// no line of fire
			_parent->getMap()->deleteProjectiles();
			if (_parent->getPanicHandled())
			{
				_action.result = "STR_NO_LINE_OF_FIRE";
			}
			else
			{
				_unit->setTimeUnits(_unit->getTimeUnits() + _action.Time); // refund shot TUs for berserking
			}
			_unit->abortTurn();
			popParentState();
			return false;
		}

		accuracySource->skipTrajectory();
		_targetVoxel = accuracySource->getPosition();

		delete accuracySource;

		for(int ii = 0; ii < shots; ++ii)
		{
			_parent->getMap()->addProjectile(projectile = new Projectile(_parent->getMod(), _parent->getSave(), _action, _origin, _targetVoxel, _ammo, true));
			newProjectiles.push_back(projectile);
		}
	}

	// set the speed of the state think cycle to 16 ms (roughly one think cycle per frame)
	_parent->setStateInterval(1000/60);

	// let it calculate a trajectory
	if (_action.type == BA_THROW)
	{
		bool firstFrameProjectile = true;

		//bool firstProjectile = true;
		for(std::vector<Projectile*>::const_iterator ii = newProjectiles.begin(); ii != newProjectiles.end(); ++ii)
		{
			projectile = *ii;
			int projectileImpact = projectile->calculateThrow(_unit->getThrowingAccuracy() / 100.0);

			if(firstFrameProjectile)
			{
				firstFrameProjectile = false;
				if (projectileImpact == V_FLOOR || projectileImpact == V_UNIT || projectileImpact == V_OBJECT)
				{
					if (_unit->getFaction() != FACTION_PLAYER && _projectileItem->getRules()->getBattleType() == BT_GRENADE)
					{
						_projectileItem->setFuseTimer(0);
					}

					if(_firstProjectile)
					{
						_firstProjectile = false;
						_projectileItem->moveToOwner(0);
						_unit->addThrowingExp();
					}

					_parent->getMod()->getSound("BATTLE.CAT", 39)->play();
				}
				else
				{
					// unable to throw here
					_parent->getMap()->deleteProjectiles();
					_action.result = "STR_UNABLE_TO_THROW_HERE";
					_action.clearTU();
					popParentState();
					return false;
				}
			}
		}
	}
	else if (_weapon->getRules()->getArcingShot()) // special code for the "spit" trajectory
	{
		//const std::vector<Projectile*> projectiles = _parent->getMap()->getProjectiles();
		bool firstFrameProjectile = true;
		
		for(std::vector<Projectile*>::const_iterator ii = newProjectiles.begin(); ii != newProjectiles.end(); ++ii)
		{
			projectile = *ii;
			int projectileImpact = projectile->calculateThrow(_unit->getFiringAccuracy(_action.type, _weapon, _parent->getMod(), shotgun, false) / 100.0);

			if(firstFrameProjectile)
			{
				firstFrameProjectile = false;
				if (_targetVoxel != Position(-16,-16,-24) && projectileImpact != V_EMPTY && projectileImpact != V_OUTOFBOUNDS)
				{
					if(_firstProjectile)
					{
						// set the soldier in an aiming position
						_firstProjectile = false;
						_unit->aim(true);
					}

					// and we have a lift-off
					if (_ammo->getRules()->getFireSound() != -1)
					{
						_parent->getMod()->getSound("BATTLE.CAT", _ammo->getRules()->getFireSound())->play();
					}
					else if (_weapon->getRules()->getFireSound() != -1)
					{
						_parent->getMod()->getSound("BATTLE.CAT", _weapon->getRules()->getFireSound())->play();
					}
					if (!_parent->getSave()->getDebugMode() && _action.type != BA_LAUNCH)
					{
						_weapon->spendAmmoForAction(_action.type, _parent->getSave());
					}
				}
				else
				{
					// no line of fire
					_parent->getMap()->deleteProjectiles();
					_action.result = "STR_NO_LINE_OF_FIRE";
					_unit->abortTurn();
					popParentState();
					return false;
				}
			}
		}
	}
	else
	{
		//const std::vector<Projectile*> projectiles = _parent->getMap()->getProjectiles();
		bool firstFrameProjectile = true;

		for(std::vector<Projectile*>::const_iterator ii = newProjectiles.begin(); ii != newProjectiles.end(); ++ii)
		{
			projectile = *ii;

			int projectileImpact;

			if(_action.type == BA_LAUNCH)
			{
				if (_originVoxel != Position(-1,-1,-1))
				{
					projectileImpact = projectile->calculateTrajectory(_unit->getFiringAccuracy(_action.type, _weapon, _parent->getMod(), shotgun, _action.dualFiring) / 100.0, _originVoxel, shotgun);
				}
				else
				{
					projectileImpact = projectile->calculateTrajectory(_unit->getFiringAccuracy(_action.type, _weapon, _parent->getMod(), shotgun, _action.dualFiring) / 100.0, shotgun);
				}
			}
			else
			{
				if (_originVoxel != Position(-1,-1,-1))
				{
					projectileImpact = projectile->calculateTrajectoryFromVector(_originVoxel, shotgun, false, _action.dualFiring);
				}
				else
				{
					projectileImpact = projectile->calculateTrajectoryFromVector(shotgun, false, _action.dualFiring);
				}
			}

			if(firstFrameProjectile)
			{
				firstFrameProjectile = false;
				
				if (projectileImpact != V_EMPTY || _action.type == BA_LAUNCH || shotgun)
				{
					if(_firstProjectile)
					{
						// set the soldier in an aiming position
						_firstProjectile = false;
						_unit->aim(true);
					}

					// and we have a lift-off
					if (_ammo->getRules()->getFireSound() != -1)
					{
						//_parent->getMod()->getSound("BATTLE.CAT", _ammo->getRules()->getFireSound())->play();
						_parent->getMod()->getSoundByDepth(_parent->getDepth(), _ammo->getRules()->getFireSound())->play(-1, _parent->getMap()->getSoundAngle(projectile->getOrigin()));
					}
					else if (_weapon->getRules()->getFireSound() != -1)
					{
						//_parent->getMod()->getSound("BATTLE.CAT", _weapon->getRules()->getFireSound())->play();
						_parent->getMod()->getSoundByDepth(_parent->getDepth(), _weapon->getRules()->getFireSound())->play(-1, _parent->getMap()->getSoundAngle(projectile->getOrigin()));
					}
					if (!_parent->getSave()->getDebugMode() && _action.type != BA_LAUNCH)
					{
						_weapon->spendAmmoForAction(_action.type, _parent->getSave());
					}
				}
				else
				{
					// no line of fire
					_parent->getMap()->deleteProjectiles();
					//_action.result = "STR_NO_LINE_OF_FIRE";
					if (_parent->getPanicHandled())
					{
						_action.result = "STR_NO_LINE_OF_FIRE";
					}
					else
					{
						_unit->setTimeUnits(_unit->getTimeUnits() + _action.Time); // refund shot TUs for berserking
					}
					_unit->abortTurn();
					popParentState();
					return false;
				}
			}
		}
	}
	
	if (_action.type != BA_THROW && _action.type != BA_LAUNCH)
		_unit->getStatistics()->shotsFiredCounter++;


	_unit->cancelEffects(ECT_ACTIVATE);
	_unit->cancelEffects(ECT_ATTACK);
	if (_action.type == BA_THROW)
	{
		_unit->addBattleExperience("STR_THROW");
		_unit->cancelEffects(ECT_THROW);
	}
	else
	{
		if (_action.dualFiring)
		{
			if (!_subState)
			{
				_unit->addBattleExperience("STR_DUAL_FIRE");
			}
		}
		else
		{
			switch (_action.type)
			{
			case BA_SNAPSHOT:
				_unit->addBattleExperience("STR_SNAP_SHOT");
				break;
			case BA_AUTOSHOT:
				if (_action.autoShotCounter == 1)
				{
					_unit->addBattleExperience("STR_AUTO_SHOT");
				}
				break;
			case BA_BURSTSHOT:
				if(_action.autoShotCounter == 1)
				{
					_unit->addBattleExperience("STR_BURST_SHOT");
				}
				break;
			case BA_AIMEDSHOT:
				_unit->addBattleExperience("STR_AIMED_SHOT");
				break;
			case BA_DUALFIRE:
				_unit->addBattleExperience("STR_DUAL_FIRE");
				break;
			}
		}
		_unit->cancelEffects(ECT_SHOOT);
	}

	return true;
}

/**
 * Animates the projectile (moves to the next point in its trajectory).
 * If the animation is finished the projectile sprite is removed from the map,
 * and this state is finished.
 */
void ProjectileFlyBState::think()
{
	if(_autoShotTimer) { _autoShotTimer->think(0, 0, this); }
	if(_dualFireTimer) { _dualFireTimer->think(0, 0, this); }

	if(_dualState)
	{
		_dualState->think();
	}
	
	if(_subState)
	{
		return;
	}

	_parent->getSave()->getBattleState()->clearMouseScrollingState();
	if (!_parent->getMap()->hasProjectile() && !_explosionStates.size())
	{
		Tile *t = _parent->getSave()->getTile(_action.actor->getPosition());
		Tile *bt = _parent->getSave()->getTile(_action.actor->getPosition() + Position(0,0,-1));
		bool hasFloor = t && !t->hasNoFloor(bt);
		bool unitCanFly = _action.actor->getMovementType() == MT_FLY;

		if ((_action.type == BA_AUTOSHOT || _action.type == BA_BURSTSHOT)
			&& _autoShotsRemaining
			&& !_action.actor->isOut()
			&& _ammo->getAmmoQuantity() != 0
			&& (hasFloor || unitCanFly))
		{
			if (_action.cameraPosition.z != -1)
			{
				_parent->getMap()->getCamera()->setMapOffset(_action.cameraPosition);
				_parent->getMap()->invalidate();
			}
		}
		else if(!_dualState || _dualState->_subStateFinished)
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
			popParentState();
			if (_action.type != BA_PANIC && _action.type != BA_MINDCONTROL && _action.type != BA_CLAIRVOYANCE && _action.type != BA_MINDBLAST && !_parent->getSave()->getUnitsFalling())
			{
				_parent->getTileEngine()->checkReactionFire(_unit, _action);
			}
		}
	}
	else
	{
		std::vector<Projectile*> &projectiles = _parent->getMap()->getProjectiles();

		bool createExplosionState = true;
		bool explosionStateCreated = false;
		bool recalculateProjectiles = false;

		for(std::vector<Projectile*>::const_iterator ii = projectiles.begin(); ii != projectiles.end();)
		{
			Projectile *projectile = *ii;

			if(!projectile->move())
			{
				recalculateProjectiles = true;
				if (_action.type == BA_THROW)
				{
					_parent->getMap()->resetCameraSmoothing();
					Position pos = projectile->getPosition(-1);
					pos.x /= 16;
					pos.y /= 16;
					pos.z /= 24;
					if (pos.y > _parent->getSave()->getMapSizeY())
					{
						pos.y--;
					}
					if (pos.x > _parent->getSave()->getMapSizeX())
					{
						pos.x--;
					}
					BattleItem *item = projectile->getItem();
					_parent->getMod()->getSoundByDepth(_parent->getDepth(), Mod::ITEM_DROP)->play(-1, _parent->getMap()->getSoundAngle(pos));

					if (item->getFuseInstant() || (Options::battleInstantGrenade && item->getRules()->getBattleType() == BT_GRENADE && item->getFuseTimer() == 0))
					{
						// it's a hot grenade to explode immediately
						ExplosionBState *exp = new ExplosionBState(_parent, projectile->getPosition(-1), BattleActionAttack(_action, _action.weapon), 0, false, 0, true);
						onNewExplosionState(exp);
					}
					else
					{
						_parent->dropItem(pos, item);
						if (_unit->getFaction() != FACTION_PLAYER && _projectileItem->getRules()->getBattleType() == BT_GRENADE)
						{
							_parent->getTileEngine()->setDangerZone(pos, item->getRules()->getExplosionRadius(_action.actor), _action.actor);
						}
					}
				}
				else if (_action.type == BA_LAUNCH && _action.waypoints.size() > 1 && projectile->getImpact() == -1)
				{
					_origin = _action.waypoints.front();
					_action.waypoints.pop_front();
					_action.target = _action.waypoints.front();
					// launch the next projectile in the waypoint cascade
					ProjectileFlyBState *nextWaypoint = new ProjectileFlyBState(_parent, _action, _origin, (int)(_range + projectile->getDistance()));
					nextWaypoint->setOriginVoxel(projectile->getPosition(-1));
					if (_origin == _action.target)
					{
						nextWaypoint->targetFloor();
					}
					_parent->statePushNext(nextWaypoint);
				}
				else
				{
					if (_parent->getSave()->getTile(_action.target)->getUnit())
					{
						_parent->getSave()->getTile(_action.target)->getUnit()->getStatistics()->shotAtCounter++; // Only counts for guns, not throws or launches
					}

					_parent->getMap()->resetCameraSmoothing();
					if (_action.type == BA_LAUNCH)
					{
						_action.weapon->spendAmmoForAction(_action.type, _parent->getSave());
					}

					if (projectile->getImpact() != V_OUTOFBOUNDS)
					{
						int offset = 0;
						if (projectile->getAmmo() && projectile->getAmmo()->getRules()->getExplosionRadius(_action.actor) != 0 && projectile->getImpact() != V_UNIT)
						{
							// explosions impact not inside the voxel but two steps back (projectiles generally move 2 voxels at a time)
							offset = -2;
						}
						
						float distance = _range + projectile->getDistance(); //projectile->getPosition(offset).distance(projectile->getOriginVoxel());

						bool lowerWeapon = _action.autoShotCounter >= _weapon->getActionConf(_action.type)->shots;

						/*switch(_action.type)
						{
						case BA_AUTOSHOT:
							break;
						case BA_BURSTSHOT:
							lowerWeapon = _action.autoShotCounter == _weapon->getRules()->getBurstShots() || !_weapon->getAmmoItem();
							break;
						default:
							lowerWeapon = true;
						}*/

						//ExplosionBState *exp = new ExplosionBState(_parent, projectile->getPosition(offset), projectile->getAmmo(), _action.actor, 0, lowerWeapon, false, true, distance);
						// TODO: Use OXCE+ Range instead of falloffdistance.
						ExplosionBState *exp = new ExplosionBState(_parent, projectile->getPosition(offset), BattleActionAttack(_action, _ammo), 0, lowerWeapon, distance, true, distance);
						onNewExplosionState(exp);

						if (projectile->getImpact() == V_UNIT)
						{
							BattleUnit *victim = _parent->getSave()->getTile(projectile->getPosition(offset) / Position(16,16,24))->getUnit();
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
									_unit->getStatistics()->shotsLandedCounter++;
									if (_parent->getTileEngine()->distance(_action.actor->getPosition(), victim->getPosition()) > 30)
									{
										_unit->getStatistics()->longDistanceHitCounter++;
									}
									if (_unit->getFiringAccuracy(_action.type, _action.weapon, _parent->getMod(), false, _action.dualFiring) < _parent->getTileEngine()->distance(_action.actor->getPosition(), victim->getPosition()))
									{
										_unit->getStatistics()->lowAccuracyHitCounter++;
									}
								}
								if (victim->getFaction() == FACTION_HOSTILE)
								{
									AIModule *ai = victim->getAIModule();
									if (ai != 0)
									{
										ai->setWasHitBy(_unit);
										_unit->setTurnsSinceSpotted(0);
									}
									// Record the last unit to hit our victim. If a victim dies without warning*, this unit gets the credit.
									// *Because the unit died in a fire or bled out.
									victim->setMurdererId(_unit->getId());
									if (_action.weapon != 0)
										victim->setMurdererWeapon(_action.weapon->getRules()->getName());
									if (_ammo != 0)
										victim->setMurdererWeaponAmmo(_ammo->getRules()->getName());
								}
							}
						}
					}
				}

				////TODO: ? else if (_action.type != BA_AUTOSHOT || _action.autoShotCounter == _weapon->getRules()->getAutoShots() || !_weapon->getAmmoItem())
				//{
				//	_unit->aim(false);
				//	_unit->setCache(0);
				//	_parent->getMap()->cacheUnits();
				//}

				//_parent->getMap()->removeProjectile(projectile);

				ii = projectiles.erase(ii);
				delete projectile;
			}
			else
			{
				++ii;
			}
		}

		if (recalculateProjectiles && projectiles.size())
		{
			for (std::vector<Projectile*>::const_iterator ii = projectiles.begin(); ii != projectiles.end(); ++ii)
			{
				(*ii)->recalculateTrajectoryFromVector();
			}
		}
	}

	if(_explosionStates.size())
	{
		for(std::vector<ExplosionBState*>::const_iterator ii = _explosionStates.begin(); ii != _explosionStates.end();)
		{
			ExplosionBState *exp = *ii;
			exp->think();
			if(ExplosionBState *newState = exp->getTerrainExplosion())
			{
				size_t pos = ii - _explosionStates.begin();
				onNewExplosionState(newState);
				exp->clearTerrainExplosion();
				ii = _explosionStates.begin() + pos;
			}
			if(exp->getFinished())
			{
				delete exp;
				ii = _explosionStates.erase(ii);
			}
			else
			{
				++ii;
			}
		}
	}


	//else
	//{
		_parent->setStateInterval(1000/60);
	//}
}

/**
 * Flying projectiles cannot be cancelled,
 * but they can be "skipped".
 */
void ProjectileFlyBState::cancel()
{
	if (_parent->getMap()->hasProjectile())
	{
		Position avgPos;
		int avgCount = 0;
		for(std::vector<Projectile*>::const_iterator ii = _parent->getMap()->getProjectiles().begin(); ii != _parent->getMap()->getProjectiles().end(); ++ii)
		{
			Projectile *pp = *ii;
			pp->skipTrajectory();
			//Position p = pp->getPosition();
			avgPos += pp->getPosition();
			++avgCount;
		}

		if(avgCount > 1) { avgPos = avgPos / avgCount; }

		if (!_parent->getMap()->getCamera()->isOnScreen(Position(avgPos.x/16, avgPos.y/16, avgPos.z/24), false, 0, false))
			_parent->getMap()->getCamera()->centerOnPosition(Position(avgPos.x/16, avgPos.y/16, avgPos.z/24));
	}
}

/**
 * Validates the throwing range.
 * @param action Pointer to throw action.
 * @param origin Position to throw from.
 * @param target Tile to throw to.
 * @return True when the range is valid.
 */
bool ProjectileFlyBState::validThrowRange(BattleAction *action, Position origin, Tile *target)
{
	// note that all coordinates and thus also distances below are in number of tiles (not in voxels).
	if (action->type != BA_THROW)
	{
		return true;
	}
	int offset = 2;
	int zd = (origin.z)-((action->target.z * 24 + offset) - target->getTerrainLevel());
	int weight = action->weapon->getTotalWeight();
	double maxDistance = (getMaxThrowDistance(weight, action->actor->getBaseStats()->strength, zd) + 8) / 16.0;
	int xdiff = action->target.x - action->actor->getPosition().x;
	int ydiff = action->target.y - action->actor->getPosition().y;
	double realDistance = sqrt((double)(xdiff*xdiff)+(double)(ydiff*ydiff));

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
	BattleUnit *victim = _parent->getSave()->getTile(pos / Position(16,16,24))->getUnit();
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
			_unit->getStatistics()->shotsLandedCounter++;
			if (_parent->getTileEngine()->distance(_action.actor->getPosition(), victim->getPosition()) > 30)
			{
				_unit->getStatistics()->longDistanceHitCounter++;
			}
			if (_unit->getFiringAccuracy(_action.type, _action.weapon, _parent->getMod(), false, _action.dualFiring) < _parent->getTileEngine()->distance(_action.actor->getPosition(), victim->getPosition()))
			{
				_unit->getStatistics()->lowAccuracyHitCounter++;
			}
		}
		if (victim->getFaction() == FACTION_HOSTILE)
		{
			AIModule *ai = victim->getAIModule();
			if (ai != 0)
			{
				ai->setWasHitBy(_unit);
				_unit->setTurnsSinceSpotted(0);
			}
		}
		// Record the last unit to hit our victim. If a victim dies without warning*, this unit gets the credit.
		// *Because the unit died in a fire or bled out.
		victim->setMurdererId(_unit->getId());
		if (_action.weapon != 0)
			victim->setMurdererWeapon(_action.weapon->getRules()->getName());
		if (_ammo != 0)
			victim->setMurdererWeaponAmmo(_ammo->getRules()->getName());
	}
}

void ProjectileFlyBState::popParentState()
{
	if(!_subState)
	{
		_parent->popState();
	}
	else
	{
		_subStateFinished = true;
	}
}

void ProjectileFlyBState::onNewExplosionState(ExplosionBState *subState)
{
	subState->init();
	_explosionStates.push_back(subState);
	if(ExplosionBState *newState = subState->getTerrainExplosion())
	{
		subState->clearTerrainExplosion();
		onNewExplosionState(newState);
	}
}
}
