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
 * @param attack The attack action that caused the explosion.
 * @param tile Tile the explosion is on.
 * @param lowerWeapon Whether the unit causing this explosion should now lower their weapon.
 * @param range Distance between weapon and target.
 * @param subState Indicates this is a child explosion state.
 * @param dropoffDistance ?
 */
ExplosionBState::ExplosionBState(BattlescapeGame *parent, Position center, BattleActionAttack attack, Tile *tile, bool lowerWeapon, int range, bool subState, float dropoffDistance) : BattleState(parent), _attack(attack), _center(center), _damageType(), _tile(tile), _targetPsiOrHit(nullptr), _power(0), _areaOfEffect(false), _lowerWeapon(lowerWeapon), _hit(false), _psi(false), _subState(subState), _dropoffDistance(dropoffDistance), _explosions(), _finished(false), _delayExplosion(false), _animationTimer(0), _terrainExplosion(0)
{

}

/**
 * Deletes the ExplosionBState.
 */
ExplosionBState::~ExplosionBState()
{
	delete _animationTimer;
	if (_terrainExplosion)
	{
		delete _terrainExplosion;
		_terrainExplosion = 0;
	}
}

/**
 * Set new value to reference if new value is not equal -1.
 * @param oldValue old value to change.
 * @param newValue new value to set, but only if is not equal -1.
 */
void ExplosionBState::optValue(int& oldValue, int newValue) const
{
	if (newValue != -1)
	{
		oldValue = newValue;
	}
}

/**
 * Initializes the explosion.
 * The animation and sound starts here.
 * If the animation is finished, the actual effect takes place.
 */
void ExplosionBState::init()
{
	_animationTimer = new Timer(BattlescapeState::DEFAULT_ANIM_SPEED / 4);
	_animationTimer->onTimer((BattleStateHandler)&ExplosionBState::thinkTimer);
	_animationTimer->start();

	BattleType type = BT_NONE;
	BattleActionType action = _attack.type;
	const RuleItem* itemRule = 0;
	bool miss = false;
	if (_attack.damage_item)
	{
		itemRule = _attack.damage_item->getRules();
		type = itemRule->getBattleType();
		 
		_power = 0;
		_hit = action == BA_HIT;
		_psi = type == BT_PSIAMP && action != BA_USE && !_hit;
		if (_hit && type != BT_MELEE)
		{
			_power += itemRule->getMeleeBonus(_attack.attacker);

			_radius = 0;
			_damageType = itemRule->getMeleeType();
		}
		else
		{
			_power += itemRule->getPowerBonus(_attack.attacker);
			_power -= itemRule->getPowerRangeReduction(_range);

			_radius = itemRule->getExplosionRadius(_attack.attacker);
			_damageType = itemRule->getDamageType();
		}

		if (type == BT_PSIAMP || _hit)
		{
			Position targetPos = _center.toTile();
			_targetPsiOrHit = _parent->getSave()->getTile(targetPos)->getUnit();
			if (!_targetPsiOrHit && targetPos.z > 0)
			{
				_targetPsiOrHit = _parent->getSave()->getTile(targetPos - Position(0, 0, 1))->getUnit();
			}
		}

		//testing if we hit target
		if (type == BT_PSIAMP && !_hit)
		{
			if (action != BA_USE)
			{
				_power = 0;
			}
			if (!_parent->getTileEngine()->psiAttack(_attack, _targetPsiOrHit))
			{
				_power = 0;
				miss = true;
			}
			else
			{
				_parent->psiAttackMessage(_attack, _targetPsiOrHit);
			}
		}
		else if (type == BT_MELEE || _hit)
		{
			if (!_parent->getTileEngine()->meleeAttack(_attack, _targetPsiOrHit))
			{
				_power = 0;
				miss = true;
			}
		}
		else if (type == BT_FIREARM)
		{
			if (_power <= 0)
			{
				miss = true;
			}
		}

		_areaOfEffect = type != BT_MELEE && _radius != 0 &&
			(type != BT_PSIAMP || action == BA_USE) &&
			!_hit && !miss;
	}
	else if (_tile)
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
		_power = _tile->getExplosive();
		_tile->setExplosive(0, 0, true);
		_damageType = _parent->getMod()->getDamageType(DT);
		_radius = _power / 10;
		_areaOfEffect = true;
	}
	else if (_attack.attacker && (_attack.attacker->getSpecialAbility() == SPECAB_EXPLODEONDEATH || _attack.attacker->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE))
	{
		RuleItem* corpse = _parent->getMod()->getItem(_attack.attacker->getArmor()->getCorpseGeoscape(), true);
		_power = corpse->getPowerBonus(_attack.attacker);
		_damageType = corpse->getDamageType();
		_radius = corpse->getExplosionRadius(_attack.attacker);
		_areaOfEffect = true;
		if (!RNG::percent(corpse->getSpecialChance()))
		{
			_power = 0;
		}
	}
	else
	{
		_power = 120;
		_damageType = _parent->getMod()->getDamageType(DT_HE);
		_areaOfEffect = true;
	}

	//Tile *t = _parent->getSave()->getTile(Position(_center.x/16, _center.y/16, _center.z/24));
	if (_areaOfEffect)
	{
		if (_power > 0)
		{
			int frame = Mod::EXPLOSION_OFFSET;
			int sound = _power <= 80 ? Mod::SMALL_EXPLOSION : Mod::LARGE_EXPLOSION;

			if (itemRule)
			{
				frame = itemRule->getHitAnimation();
				optValue(sound, itemRule->getExplosionHitSound());
			}
			if (_parent->getDepth() > 0)
			{
				frame -= Explosion::EXPLODE_FRAMES;
			}
			int frameDelay = 0;
			int explosionSkew = itemRule ? (itemRule->getExplosionRadius(_attack.attacker) * 20) : _power;
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
			_parent->playSound(sound);
			/*if (_power <= 80)
				_parent->getMod()->getSoundByDepth(_parent->getDepth(), Mod::SMALL_EXPLOSION)->play();
			else
				_parent->getMod()->getSoundByDepth(_parent->getDepth(), Mod::LARGE_EXPLOSION)->play();*/

			_parent->getMap()->getCamera()->centerOnPosition(_center.toTile(), false);
		}
		else
		{
			_finished = true;
			if (!_subState) _parent->popState();
			return;
		}
	}
	else
		// create a bullet hit
	{
		if (itemRule && (_dropoffDistance >= 0))
		{
			float voxelDropoff = itemRule->getDamageType()->DistanceDropoff;
			if (voxelDropoff > 0.0f)
			{
				int powerLoss = (int)floor(_dropoffDistance * voxelDropoff / 16.0);
				//Log(LOG_INFO) << "Distance: [" << _dropoffDistance << "] dropoff: [" << powerLoss << "]";
				_power -= powerLoss;
				if (_power < 0) _power = 0;
			}
		}

		//if(!_subState) { _parent->setStateInterval(std::max(1, ((BattlescapeState::DEFAULT_ANIM_SPEED/2) - (10 * _item->getRules()->getExplosionSpeed())))); }
		_animationTimer->setInterval(std::max(1, ((BattlescapeState::DEFAULT_ANIM_SPEED / 4) - (10 * itemRule->getExplosionSpeed()))));
		int anim = -1;
		int sound = -1;

		const RuleItem *weaponRule = _attack.weapon_item->getRules();
		const RuleItem *damageRule = _attack.weapon_item != _attack.damage_item ? itemRule : nullptr;

		if (_hit || _psi)
		{
			anim = weaponRule->getMeleeAnimation();
			if (_psi)
			{
				// psi attack sound is based weapon hit sound
				sound = weaponRule->getHitSound();

				optValue(anim, weaponRule->getPsiAnimation());
				optValue(sound, weaponRule->getPsiSound());
			}
			else
			{
				sound = weaponRule->getMeleeSound();
				if (damageRule)
				{
					optValue(anim, damageRule->getMeleeAnimation());
					optValue(sound, damageRule->getMeleeSound());
				}
			}
		}
		else
		{
			anim = itemRule->getHitAnimation();
			sound = itemRule->getHitSound();
		}

		if (miss)
		{
			if (_hit || _psi)
			{
				optValue(anim, weaponRule->getMeleeMissAnimation());
				if (_psi)
				{
					// psi attack sound is based weapon hit sound
					optValue(sound, weaponRule->getHitMissSound());

					optValue(anim, weaponRule->getPsiMissAnimation());
					optValue(sound, weaponRule->getPsiMissSound());
				}
				else
				{
					optValue(sound, weaponRule->getMeleeMissSound());
					if (damageRule)
					{
						optValue(anim, damageRule->getMeleeMissAnimation());
						optValue(sound, damageRule->getMeleeMissSound());
					}
				}
			}
			else
			{
				optValue(anim, itemRule->getHitMissAnimation());
				optValue(sound, itemRule->getHitMissSound());
			}
		}

		if (anim != -1)
		{
			Explosion *explosion = new Explosion(_center, anim, 0, false, (_hit || _psi)); // Don't burn the tile
			_parent->getMap()->getExplosions()->push_back(explosion);
			_explosions.push_back(explosion);
		}

		_parent->getMap()->getCamera()->setViewLevel(_center.z / 24);

		if (_targetPsiOrHit && _parent->getSave()->getSide() == FACTION_HOSTILE && _targetPsiOrHit->getFaction() == FACTION_PLAYER)
		{
			_parent->getMap()->getCamera()->centerOnPosition(_center.toTile(), false);
		}
		// bullet hit sound
		_parent->playSound(sound, _center.toTile());
	}

	if (_action.type != BA_MINDCONTROL && _action.type != BA_PANIC && _action.type != BA_CLAIRVOYANCE && _action.type != BA_MINDBLAST)
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
	// last minute adjustment: determine if we actually
	if (_hit)
	{
		if (_attack.attacker && !_attack.attacker->isOut())
		{
			_attack.attacker->aim(false);
		}

		if (_power <= 0)
		{
			_parent->popState();
			return;
		}

		int sound = _attack.weapon_item->getRules()->getMeleeHitSound();
		if (_attack.weapon_item != _attack.damage_item)
		{
			// melee weapon with ammo
			optValue(sound, _attack.damage_item->getRules()->getMeleeHitSound());
		}
		_parent->playSound(sound, _center.toTile());
	}

	bool range = !(_hit || (_attack.weapon_item && _attack.weapon_item->getRules()->getBattleType() == BT_PSIAMP));

	// after the animation is done, the real explosion/hit takes place
	/*if (_item)
	{
		if (!_unit && _item->getPreviousOwner())
		{
			_unit = _item->getPreviousOwner();
		}*/

	if (_areaOfEffect)
	{
		save->getTileEngine()->explode(_attack, _center, _power, _damageType, _radius, range);
	}
	else //if (!_cosmetic)
	{

		if (_damageType->ResistType == DT_TELEPORT)
		{
			Tile *destination = _tile ? _tile : save->getTile(Position(_center.x / 16, _center.y / 16, _center.z / 24));
			if (_attack.attacker && destination && !destination->getUnit())
			{
				save->getTile(_attack.attacker->getPosition())->setUnit(0);
				const Position& oldPosition = _attack.attacker->getPosition();
				_attack.attacker->setPosition(destination->getPosition());
				destination->setUnit(_attack.attacker);
				save->getTileEngine()->calculateFOV(oldPosition);
				save->getTileEngine()->calculateFOV(destination->getPosition());
			}
		}
		else
		{
			BattleUnit *victim = save->getTileEngine()->hit(_attack, _center, _power, _action.type, _damageType, range);

			const RuleItem *hitItem = _attack.damage_item->getRules();

			// check if this unit turns others into zombies
			if (!hitItem->getZombieUnit().empty()
				&& RNG::percent(hitItem->getSpecialChance())
				&& victim
				&& victim->getArmor()->getZombiImmune() == false
				&& victim->getSpawnUnit().empty()
				&& victim->getOriginalFaction() != FACTION_HOSTILE)
			{
				// converts the victim to a zombie on death
				victim->setRespawn(true);
				victim->setSpawnUnit(hitItem->getZombieUnit());
			}
		}
	}
	//}

	if (_tile)
	{
		/*ItemDamageType DT;
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
			_tile->setExplosive(0, 0, true);
		}
		save->getTileEngine()->explode(_center, _power, _parent->getCurrentAction()->type, DT, _power / 10, 0);*/
		terrainExplosion = true;
	}
	if (!_tile && !_attack.damage_item)
	{
		/*int radius = 6;
		// explosion not caused by terrain or an item, must be by a unit (cyberdisc)
		if (_unit && (_unit->getSpecialAbility() == SPECAB_EXPLODEONDEATH || _unit->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE))
		{
			radius = _parent->getMod()->getItem(_unit->getArmor()->getCorpseGeoscape(), true)->getExplosionRadius();
		}
		save->getTileEngine()->explode(_center, _power, _parent->getCurrentAction()->type, DT_HE, radius, 0);*/
		terrainExplosion = true;
	}

	// now check for new casualties
	_parent->checkForCasualties(_damageType, _attack, false, terrainExplosion, _subState);
	// revive units if damage could give hp or reduce stun
	_parent->getSave()->reviveUnconsciousUnits(true);

	// if this explosion was caused by a unit shooting, now it's the time to put the gun down
	if (_attack.attacker && !_attack.attacker->isOut() && _lowerWeapon)
	{
		_attack.attacker->aim(false);
	}

	if (_attack.damage_item && (_attack.damage_item->getRules()->getBattleType() == BT_GRENADE || _attack.damage_item->getRules()->getBattleType() == BT_PROXIMITYGRENADE))
	{
		_parent->getSave()->removeItem(_attack.damage_item);
	}
	//_finished = true;
	//if(!_subState) _parent->popState();

	// check for terrain explosions
	Tile *t = save->getTileEngine()->checkForTerrainExplosions();
	if (t)
	{
		Position p = t->getPosition().toVexel();
		p += Position(8, 8, 0);
		{
			//_parent->statePushFront(new ExplosionBState(_parent, p, BattleActionAttack{ BA_NONE, _attack.attacker, }, t));
			if (!_subState)
			{
				_parent->statePushFront(new ExplosionBState(_parent, p, BattleActionAttack{ BA_NONE, _attack.attacker, }, t));
			}
			else
			{
				_terrainExplosion = new ExplosionBState(_parent, p, BattleActionAttack{ BA_NONE, _attack.attacker, }, t, false, false, true);
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
