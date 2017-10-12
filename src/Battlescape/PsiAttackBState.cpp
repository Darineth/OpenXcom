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
#include "PsiAttackBState.h"
#include "ExplosionBState.h"
#include "BattlescapeGame.h"
#include "BattlescapeState.h"
#include "TileEngine.h"
#include "InfoboxState.h"
#include "Map.h"
#include "Camera.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Engine/Game.h"
#include "../Engine/RNG.h"
#include "../Engine/Language.h"
#include "../Engine/Sound.h"
#include "../Mod/Mod.h"
#include "../Savegame/BattleUnitStatistics.h"

namespace OpenXcom
{

/**
 * Sets up a PsiAttackBState.
 */
PsiAttackBState::PsiAttackBState(BattlescapeGame *parent, BattleAction action) : BattleState(parent, action), _unit(0), _item(0), _initialized(false), _attack(action.type, action.actor, action.weapon, action.ammo), _psychicDamage(parent->getMod()->getDamageType(DT_PSYCHIC))
{
}

/**
 * Deletes the PsiAttackBState.
 */
PsiAttackBState::~PsiAttackBState()
{
}

/**
 * Initializes the sequence:
 * - checks if the action is valid,
 * - adds a psi attack animation to the world.
 * - from that point on, the explode state takes precedence (and perform psi attack).
 * - when that state pops, we'll do our first think()
 */
void PsiAttackBState::init()
{
	if (_initialized) return;
	_initialized = true;

	_item = _action.weapon;

	if (!_item) // can't make a psi attack without a weapon
	{
		_parent->popState();
		return;
	}

	if (!_parent->getSave()->getTile(_action.target)) // invalid target position
	{
		_parent->popState();
		return;
	}

	_unit = _action.actor;

	if (_parent->getTileEngine()->distance(_action.actor->getPosition(), _action.target) > _action.weapon->getRules()->getMaxRange())
	{
		// out of range
		_action.result = "STR_OUT_OF_RANGE";
		_parent->popState();
		return;
	}

	_action.updateTU();
	std::string tuMessage;
	if (!_action.haveTU(&tuMessage)) // not enough time units
	{
		_action.result = tuMessage.empty() ? "STR_NOT_ENOUGH_TIME_UNITS" : tuMessage;
		_parent->popState();
		return;
	}

	_target = _parent->getSave()->getTile(_action.target)->getUnit();

	int psiCost = 0;
	bool requireTarget = true;
	if(_action.weapon)
	{
		switch (_action.type)
		{
		case BA_PANIC:
			psiCost = _action.weapon->getRules()->getCostPanic().Ammo;
			break;

		case BA_MINDCONTROL:
			psiCost = _action.weapon->getRules()->getCostMind().Ammo;
			break;

		case BA_CLAIRVOYANCE:
			psiCost = _action.weapon->getRules()->getCostClairvoyance().Ammo;
			requireTarget = false;
			break;

		case BA_MINDBLAST:
			psiCost = _action.weapon->getRules()->getCostMindBlast().Ammo;

			Position targetVoxel;
			_parent->getSave()->getTileEngine()->getTargetVoxel(&_action, _action.target, targetVoxel);
			Position originVoxel = _parent->getSave()->getTileEngine()->getOriginVoxel(_action, _parent->getSave()->getTile(_action.actor->getPosition()));
			int hit = _parent->getSave()->getTileEngine()->calculateLine(originVoxel, targetVoxel, false, 0, _action.actor, true, true, _target);

			if (hit != V_UNIT || !_action.actor->checkSquadSight(_parent->getSave(), _target, false))
			{
				_action.result = "STR_NO_LINE_OF_FIRE";
				_parent->popState();
				return;
			}
			break;
		}
	}

	if(psiCost)
	{
		int slot = _action.weapon->getActionConf(_action.type)->ammoSlot;
		BattleItem *ammo = _action.weapon->getAmmoForAction(_action.type, &_action.result);
		if(!ammo)
		{
			_action.result = "STR_NO_AMMUNITION_LOADED";
			_parent->popState();
			return;
		}
		else if(ammo->getAmmoQuantity() < psiCost)
		{
			_action.result = "STR_NO_ROUNDS_LEFT";
			_parent->popState();
			return;
		}
		else if(!ammo->spendBullet(psiCost))
		{
			_parent->getSave()->removeItem(ammo);
			_action.weapon->setAmmoForSlot(slot, nullptr);
		}
	}


	if (requireTarget && !_target) // invalid target
	{
		_parent->popState();
		return;
	}

	if (!_item) // can't make a psi attack without a weapon
	{
		_parent->popState();
		return;
	}
	else if (_item->getRules()->getHitSound() != -1)
	{
		_parent->getMod()->getSoundByDepth(_parent->getDepth(), _item->getRules()->getHitSound())->play(-1, _parent->getMap()->getSoundAngle(_action.target));
	}

	_unit->cancelEffects(ECT_ACTIVATE);
	_action.actor->addBattleExperience("STR_PSIONICS");

	// make a cosmetic explosion
	if (_target)
	{
		int height = _target->getFloatHeight() + (_target->getHeight() / 2) - _parent->getSave()->getTile(_action.target)->getTerrainLevel();
		Position voxel = _action.target * Position(16, 16, 24) + Position(8, 8, height);
		_parent->statePushFront(new ExplosionBState(_parent, voxel, _attack));
	}
	else
	{
		Position voxel = _action.target * Position(16, 16, 24) + Position(8, 8, 12);
		_parent->statePushFront(new ExplosionBState(_parent, voxel, _attack));
	}
}


/**
 * After the explosion animation is done doing its thing,
 * make the actual psi attack, and restore the camera/cursor.
 */
void PsiAttackBState::think()
{
	//make the psi attack.
	psiAttack();

	if (_action.cameraPosition.z != -1)
	{
		_parent->getMap()->getCamera()->setMapOffset(_action.cameraPosition);
		_parent->getMap()->invalidate();
	}
	if (_parent->getSave()->getSide() == FACTION_PLAYER || _parent->getSave()->getDebugMode())
	{
		_parent->setupCursor();
	}
	_parent->popState();
}

/**
 * Attempts a panic or mind control action.
 */
void PsiAttackBState::psiAttack()
{
	TileEngine *tiles = _parent->getSave()->getTileEngine();

	if (_action.type == BA_CLAIRVOYANCE)
	{
		int psiScore = _action.actor->getBaseStats()->psiStrength + _action.actor->getBaseStats()->psiSkill;

		if (psiScore < 100)
		{
			return;
		}

		int radius = (int)floor(0.223607 * sqrt((double)(4 * (psiScore - 100) + 5)) - 0.49);
		//int radius = (psiScore -= 100) / 30;
		int radiusSq = radius * radius;

		for (int xx = -radius; xx <= radius; ++xx)
		{
			for (int yy = -radius; yy <= radius; ++yy)
			{
				Position test(_action.target.x + xx, _action.target.y + yy, _action.target.z);

				Tile *tile;
				if ((tiles->distanceSq(_action.target, test) <= radiusSq) && (tile = _parent->getSave()->getTile(test)))
				{
					tile->setDiscovered(true, 0);
					tile->setDiscovered(true, 1);
					tile->setDiscovered(true, 2);
					tile->changeVisibleCounts(1, 0);

					if (tile->getUnit())
					{
						tile->setKnownHiddenUnit();
					}
				}
			}
		}
		_action.actor->addPsiSkillExp();
		return;
	}

	BattleUnit *victim = _parent->getSave()->getTile(_action.target)->getUnit();
	if (!victim)
		return;

	bool success = false;

	double attackStrength = 0.0;
	double defenseStrength = 0.0;
	double difference = 0.0;
	double dist = 0.0;

	switch (_action.type)
	{
	case BA_PANIC:
	case BA_MINDCONTROL:
		attackStrength = _action.actor->getBaseStats()->psiStrength * _action.actor->getBaseStats()->psiSkill / 50.0;
		defenseStrength = victim->getBaseStats()->psiStrength + ((victim->getBaseStats()->psiSkill > 0) ? 10.0 + victim->getBaseStats()->psiSkill / 5.0 : 10.0);
		dist = tiles->distance(_action.actor->getPosition(), _action.target);
		attackStrength -= dist;
		attackStrength += RNG::generate(0, 55);
		break;

	case BA_MINDBLAST:
		attackStrength = _action.actor->getBaseStats()->psiStrength + _action.actor->getBaseStats()->psiSkill + RNG::generate(-15, 15) - 10;
		defenseStrength = victim->getBaseStats()->psiStrength + victim->getBaseStats()->psiSkill;
		difference = attackStrength - defenseStrength;
		dist = tiles->distance(_action.actor->getPosition(), _action.target);
		if (dist > 0)
		{
			dist = 10 / (dist + 10);
		}
		else
		{
			dist = 1;
		}
		break;
	}

	if (_action.type == BA_MINDCONTROL)
	{
		defenseStrength += 20;
	}
	
	_action.actor->addPsiSkillExp();
	if (Options::allowPsiStrengthImprovement) victim->addPsiStrengthExp();
	if (attackStrength > defenseStrength)
	{
		_action.actor->addPsiSkillExp();
		_action.actor->addPsiSkillExp();

		BattleUnitKills killStat;
		killStat.setUnitStats(_target);
		killStat.setTurn(_parent->getSave()->getTurn(), _parent->getSave()->getSide());
		killStat.weapon = _action.weapon->getRules()->getName();
		killStat.weaponAmmo = _action.weapon->getRules()->getName(); //Psi weapons got no ammo, just filling up the field
		killStat.faction = _target->getFaction();
		killStat.mission = _parent->getSave()->getGeoscapeSave()->getMissionStatistics()->size();
		killStat.id = _target->getId();

		switch (_action.type)
		{
		case BA_PANIC:
		{
			int moraleLoss = (110 - _parent->getSave()->getTile(_action.target)->getUnit()->getBaseStats()->bravery);
			if (moraleLoss > 0)
				_target->moraleChange(-moraleLoss);
				// Award Panic battle unit kill
				if (!_unit->getStatistics()->duplicateEntry(STATUS_PANICKING, _target->getId()))
				{
					killStat.status = STATUS_PANICKING;
					_unit->getStatistics()->kills.push_back(new BattleUnitKills(killStat));
					_target->setMurdererId(_unit->getId());
				}
			break;
		}
		case BA_MINDBLAST:
		{
			if (difference > 20)
			{
				// Decisive victory
				int power = difference / 6.0 * dist * RNG::generate(1.0, 3.0);
				victim->damage(tiles, _action.actor, Position(), power, _psychicDamage, _parent->getSave(), _attack);
			}
			else
			{
				// Victory/backlash
				victim->damage(tiles, _action.actor, Position(), RNG::generate(3, 6) * dist * RNG::generate(1.0, 3.0), _psychicDamage, _parent->getSave(), _attack);
				_action.actor->damage(tiles, victim, Position(), RNG::generate(2, 4) * dist * RNG::generate(1.0, 3.0), _psychicDamage, _parent->getSave(), _attack);
			}
			_parent->checkForCasualties(_psychicDamage, _attack);
			break;
		}
		case BA_MINDCONTROL:
		{
			// Award MC battle unit kill
			if (!_unit->getStatistics()->duplicateEntry(STATUS_TURNING, _target->getId()))
			{
				killStat.status = STATUS_TURNING;
				_unit->getStatistics()->kills.push_back(new BattleUnitKills(killStat));
				_target->setMurdererId(_unit->getId());
			}
			_action.actor->setControlling(victim);
			if (_action.actor->getTimeUnits() > 5)
			{
				_action.actor->spendTimeUnits(_action.actor->getTimeUnits() - 5);
			}
			tiles->calculateFOV(victim->getPosition());
			tiles->calculateLighting(LL_UNITS, victim->getPosition());
			victim->recoverTimeUnits();
			victim->allowReselect();
			victim->abortTurn(); // resets unit status to STANDING

			// if all units from either faction are mind controlled - auto-end the mission.
			if (_parent->getSave()->getSide() == FACTION_PLAYER && Options::battleAutoEnd && Options::allowPsionicCapture)
			{
				if (Options::allowPsionicCapture)
				{
					_parent->autoEndBattle();
				}
			}
		}
		break;
		}

		success = true;
	}
	else
	{
		if (_action.type == BA_MINDBLAST)
		{
			int power = (-difference + RNG::generate(1, 2)) / 3.0 * dist * RNG::generate(2.0, 4.0);
			_action.actor->damage(tiles, victim, Position(), power, _psychicDamage, _parent->getSave(), _attack);
			_parent->checkForCasualties(_psychicDamage, _attack, false, false, false);
		}

		if (Options::allowPsiStrengthImprovement)
		{
			victim->addPsiStrengthExp();
		}
		//return false;
	}

	if (_action.type != BA_MINDBLAST)
	{
		tiles->displayDamage(_action.actor, victim, _action.type, nullptr, success ? 1 : 0, 0, 0);
	}
}

}
