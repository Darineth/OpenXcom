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

#include "ExplosionBState.h"
#include "BattlescapeState.h"
#include "Explosion.h"
#include "TileEngine.h"
#include "Map.h"
#include "Camera.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Mod/Mod.h"
#include "../Engine/Sound.h"
#include "../Mod/RuleItem.h"
#include "../Mod/Armor.h"
#include "../Engine/RNG.h"
#include "../Engine/Timer.h"
#include "../Engine/Logger.h"

namespace OpenXcom
{

/**
 * Sets up an ExplosionBState.
 * @param parent Pointer to the BattleScape.
 * @param center Center position in voxelspace.
 * @param item Item involved in the explosion (eg grenade).
 * @param unit Unit involved in the explosion (eg unit throwing the grenade).
 * @param tile Tile the explosion is on.
 * @param lowerWeapon Whether the unit causing this explosion should now lower their weapon.
 */
ExplosionBState::ExplosionBState(BattlescapeGame *parent, Position center, BattleItem *item, BattleUnit *unit, Tile *tile, bool lowerWeapon, bool cosmetic, bool subState, float dropoffDistance) : BattleState(parent), _unit(unit), _center(center), _item(item), _tile(tile), _power(0), _areaOfEffect(false), _lowerWeapon(lowerWeapon), _cosmetic(cosmetic), _subState(subState), _dropoffDistance(dropoffDistance), _explosions(), _finished(false), _delayExplosion(false), _animationTimer(0), _terrainExplosion(0)
{
}

/**
 * Deletes the ExplosionBState.
 */
ExplosionBState::~ExplosionBState()
{
	delete _animationTimer;
	if(_terrainExplosion)
	{
		delete _terrainExplosion;
		_terrainExplosion = 0;
	}
}

/**
 * Initializes the explosion.
 * The animation and sound starts here.
 * If the animation is finished, the actual effect takes place.
 */
void ExplosionBState::init()
{
	_animationTimer = new Timer(BattlescapeState::DEFAULT_ANIM_SPEED/4);
	_animationTimer->onTimer((BattleStateHandler)&ExplosionBState::thinkTimer);
	_animationTimer->start();

	BattleActionType actionType = _parent->getCurrentAction()->type;
	if (_item)
	{
		_power = _item->getRules()->getPower();

		// this usually only applies to melee, but as a concession for modders i'll leave it here in case they wanna make bows or something.
		if (_item->getRules()->isStrengthApplied() && _unit)
		{
			_power += _unit->getBaseStats()->strength;
		}

		_areaOfEffect = _item->getRules()->getBattleType() != BT_MELEE &&
						_item->getRules()->getExplosionRadius() != 0 &&
						!_cosmetic;
	}
	else if (_tile)
	{
		_power = _tile->getExplosive();
		_areaOfEffect = true;
	}
	else if (_unit && (_unit->getSpecialAbility() == SPECAB_EXPLODEONDEATH || _unit->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE))
	{
		_power = _parent->getMod()->getItem(_unit->getArmor()->getCorpseGeoscape(), true)->getPower();
		_areaOfEffect = true;
	}
	else
	{
		_power = 120;
		_areaOfEffect = true;
	}

	Tile *t = _parent->getSave()->getTile(Position(_center.x/16, _center.y/16, _center.z/24));
	if (_areaOfEffect)
	{
		if (_power)
		{
			int frame = Mod::EXPLOSION_OFFSET;
			if (_item)
			{
				frame = _item->getRules()->getHitAnimation();
			}
			if (_parent->getDepth() > 0)
			{
				frame -= Explosion::EXPLODE_FRAMES;
			}
			int frameDelay = 0;
			int explosionSkew = _item ? (_item->getRules()->getExplosionRadius() * 20) : _power;
			int counter = std::max(1, (explosionSkew / 5) / 5);
			_parent->getMap()->setBlastFlash(true);
			for (int i = 0; i < explosionSkew / 5; i++)
			{
				int X = RNG::generate(-explosionSkew / 2, explosionSkew / 2);
				int Y = RNG::generate(-explosionSkew / 2, explosionSkew / 2);
				Position p = _center;
				p.x += X; p.y += Y;
				Explosion *explosion = new Explosion(p, frame, frameDelay, true);
				// add the explosion on the map
				_parent->getMap()->getExplosions()->push_back(explosion);
				_explosions.push_back(explosion);
				if (i > 0 && i % counter == 0)
				{
					frameDelay++;
				}
			}
			//if(!_subState) { _parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED/2); }
			//_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED/2);
			// explosion sound
			if (_power <= 80)
				_parent->getMod()->getSoundByDepth(_parent->getDepth(), Mod::SMALL_EXPLOSION)->play();
			else
				_parent->getMod()->getSoundByDepth(_parent->getDepth(), Mod::LARGE_EXPLOSION)->play();

			_parent->getMap()->getCamera()->centerOnPosition(t->getPosition(), false);
		}
		else
		{
			_finished = true;
			if(!_subState) _parent->popState();
			return;
		}
	}
	else
	// create a bullet hit
	{
		if (_item && (_dropoffDistance >= 0))
		{
			float voxelDropoff = _parent->getMod()->getDamageDropoff(_item->getRules()->getDamageType());
			if (voxelDropoff > 0)
			{
				int powerLoss = (int)floor(_dropoffDistance * voxelDropoff / 16.0);
				//Log(LOG_INFO) << "Distance: [" << _dropoffDistance << "] dropoff: [" << powerLoss << "]";
				_power -= powerLoss;
				if (_power < 0) _power = 0;
			}
		}

		//if(!_subState) { _parent->setStateInterval(std::max(1, ((BattlescapeState::DEFAULT_ANIM_SPEED/2) - (10 * _item->getRules()->getExplosionSpeed())))); }
		_animationTimer->setInterval(std::max(1, ((BattlescapeState::DEFAULT_ANIM_SPEED/4) - (10 * _item->getRules()->getExplosionSpeed()))));
		int anim = _item->getRules()->getHitAnimation();
		int sound = _item->getRules()->getHitSound();
		if (_cosmetic) // Play melee animation on hitting and psiing
		{
			anim = _item->getRules()->getMeleeAnimation();
		}

		if (anim != -1)
		{
			Explosion *explosion = new Explosion(_center, anim, 0, false, _cosmetic);
			_parent->getMap()->getExplosions()->push_back(explosion);
			_explosions.push_back(explosion);
		}

		_parent->getMap()->getCamera()->setViewLevel(_center.z / 24);

		BattleUnit *target = t->getUnit();
		if (_cosmetic && _parent->getSave()->getSide() == FACTION_HOSTILE && target && target->getFaction() == FACTION_PLAYER)
		{
			_parent->getMap()->getCamera()->centerOnPosition(t->getPosition(), false);
		}
		if (sound != -1 && !_cosmetic)
		{
			// bullet hit sound
			_parent->getMod()->getSoundByDepth(_parent->getDepth(), sound)->play(-1, _parent->getMap()->getSoundAngle(_center / Position(16,16,24)));
		}
	}

	if(actionType != BA_MINDCONTROL && actionType != BA_PANIC && actionType != BA_CLAIRVOYANCE && actionType != BA_MINDBLAST)
	{
		explode();
	}
	else
	{
		_delayExplosion = true;
	}
}

void ExplosionBState::think()
{
	_animationTimer->think(0, 0, this);
}

/**
 * Animates explosion sprites. If their animation is finished remove them from the list.
 * If the list is empty, this state is finished and the actual calculations take place.
 */
void ExplosionBState::thinkTimer()
{
	if (!_parent->getMap()->getBlastFlash())
	{
		if (_explosions.empty())
		{
			if (_delayExplosion) explode();
			_finished = true;
		}
		else
		{
			for (std::vector<Explosion*>::iterator ii = _explosions.begin(); ii != _explosions.end();)
			{
				if (!(*ii)->animate())
				{
					std::list<Explosion*> *mapExplosions = _parent->getMap()->getExplosions();
					for (std::list<Explosion*>::const_iterator jj = mapExplosions->begin(); jj != mapExplosions->end(); ++jj)
					{
						if (*jj == *ii)
						{
							mapExplosions->erase(jj);
							break;
						}
					}

					delete (*ii);
					ii = _explosions.erase(ii);

					if (_explosions.empty())
					{
						_finished = true;
						if (!_subState) _parent->popState();
						if (_delayExplosion) explode();
						return;
					}
				}
				else
				{
					++ii;
				}
			}
		}
	}
}

/**
 * Explosions cannot be cancelled.
 */
void ExplosionBState::cancel()
{
}

/**
 * Calculates the effects of the explosion.
 */
void ExplosionBState::explode()
{
	bool terrainExplosion = false;
	SavedBattleGame *save = _parent->getSave();

	// after the animation is done, the real explosion/hit takes place
	if (_item)
	{
		if (!_unit && _item->getPreviousOwner())
		{
			_unit = _item->getPreviousOwner();
		}

		BattleUnit *victim = 0;

		if (_areaOfEffect)
		{
			save->getTileEngine()->explode(_center, _power, _parent->getCurrentAction()->type, _item->getRules()->getDamageType(), _item->getRules()->getExplosionRadius(), _item->getRules()->getExplosionDropoff(), _unit);
		}
		else if (!_cosmetic)
		{
			ItemDamageType type = _item->getRules()->getDamageType();

			if (type == DT_TELEPORT)
			{
				Tile *destination = _tile ? _tile : save->getTile(Position(_center.x / 16, _center.y / 16, _center.z / 24));
				if (_unit && destination && !destination->getUnit())
				{
					save->getTile(_unit->getPosition())->setUnit(0);
					const Position& oldPosition = _unit->getPosition();
					_unit->setPosition(destination->getPosition());
					destination->setUnit(_unit);
					save->getTileEngine()->calculateFOV(oldPosition);
					save->getTileEngine()->calculateFOV(destination->getPosition());
				}
			}
			else
			{
				victim = save->getTileEngine()->hit(_center, _power, save->getBattleGame()->getCurrentAction()->type, type, _unit, _item);
			}
		}

		// check if this unit turns others into zombies
		if (!_item->getRules()->getZombieUnit().empty()
			&& victim
			&& victim->getArmor()->getSize() == 1
			&& !victim->isVehicle()
			&& (victim->getGeoscapeSoldier() || victim->getUnitRules()->getRace() == "STR_CIVILIAN")
			&& victim->getSpawnUnit().empty())
		{
			// converts the victim to a zombie on death
			victim->setRespawn(true);
			victim->setSpawnUnit(_item->getRules()->getZombieUnit());
		}
	}
	if (_tile)
	{
		ItemDamageType DT;
		switch (_tile->getExplosiveType())
		{
		case 0:
			DT = DT_HE;
			break;
		case 5:
			DT = DT_IN;
			break;
		case 6:
			DT = DT_STUN;
			break;
		default:
			DT = DT_SMOKE;
			break;
		}
		if (DT != DT_HE)
		{
			_tile->setExplosive(0,0,true);
		}
		save->getTileEngine()->explode(_center, _power, _parent->getCurrentAction()->type, DT, _power/10, 0);
		terrainExplosion = true;
	}
	if (!_tile && !_item)
	{
		int radius = 6;
		// explosion not caused by terrain or an item, must be by a unit (cyberdisc)
		if (_unit && (_unit->getSpecialAbility() == SPECAB_EXPLODEONDEATH || _unit->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE))
		{
			radius = _parent->getMod()->getItem(_unit->getArmor()->getCorpseGeoscape(), true)->getExplosionRadius();
		}
		save->getTileEngine()->explode(_center, _power, _parent->getCurrentAction()->type, DT_HE, radius, 0);
		terrainExplosion = true;
	}

	if (!_cosmetic)
	{
		// now check for new casualties
		_parent->checkForCasualties(_item, _unit, false, terrainExplosion, _subState);
	}

	// if this explosion was caused by a unit shooting, now it's the time to put the gun down
	if (_unit && !_unit->isOut() && _lowerWeapon)
	{
		_unit->aim(false);
		_unit->setCache(0);
	}
	_parent->getMap()->cacheUnits();
	//_finished = true;
	//if(!_subState) _parent->popState();

	// check for terrain explosions
	Tile *t = save->getTileEngine()->checkForTerrainExplosions();
	if(t)
	{
		Position p = Position(t->getPosition().x * 16, t->getPosition().y * 16, t->getPosition().z * 24);
		p += Position(8, 8, 0);
		{
			//_parent->statePushFront(new ExplosionBState(_parent, p, 0, _unit, t));
			if(!_subState)
			{
				_parent->statePushFront(new ExplosionBState(_parent, p, 0, _unit, t));
			}
			else
			{
				_terrainExplosion = new ExplosionBState(_parent, p, 0, _unit, t, false, false, true);
			}
		}
	}

	if (_item && !_item->isAmmo() && (_item->getRules()->getBattleType() == BT_GRENADE || _item->getRules()->getBattleType() == BT_PROXIMITYGRENADE))
	{
		for (std::vector<BattleItem*>::iterator j = _parent->getSave()->getItems()->begin(); j != _parent->getSave()->getItems()->end(); ++j)
		{
			if (_item->getId() == (*j)->getId())
			{
				_parent->getSave()->removeItem(_item);
				break;
			}
		}
	}
}

/**
 * Returns if the current explosion is AoE.
 * @return True if the explosion is an area of effect.
 */
bool ExplosionBState::getAreaOfEffect() const
{
	return _areaOfEffect;
}

/**
 * Returns If the state is a substate and has finished processing.
 * @return True if the state is a substate and has finished processing.
 */
bool ExplosionBState::getFinished() const
{
	return _finished;
}

/**
 * Returns the new terrain explosion state if one was caused by explode() for a substate.
 * @return The new explosion state.
 */
ExplosionBState *ExplosionBState::getTerrainExplosion() const
{
	return _terrainExplosion;
}

void ExplosionBState::clearTerrainExplosion()
{
	_terrainExplosion = 0;
}
}
