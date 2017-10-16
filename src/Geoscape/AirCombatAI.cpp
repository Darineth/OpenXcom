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
#include "AirCombatAI.h"
#include "AirActionMenuState.h"
#include "AirCombatState.h"
#include "../Mod/RuleUfo.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/CraftWeapon.h"
#include "../fmath.h"

namespace OpenXcom
{

AirCombatAI::AirCombatAI(AirCombatUnit *unit) : _unit(unit), _mode(AAI_NONE), _idealAttackRange(0), _maxAttackRange(0)
{
	if (_unit->ufo->getWeapons().size())
	{
		int bestDps = 0;
		for (CraftWeapon *weapon : _unit->ufo->getWeapons())
		{
			_maxAttackRange = std::max(_maxAttackRange, weapon->getRules()->getRange());

			double weaponDpt = (double)weapon->getRules()->getDamage() * weapon->getRules()->getShotsAimed() * weapon->getRules()->getAccuracy() / 100 / weapon->getRules()->getCostAimed();
			if (weaponDpt > bestDps)
			{
				_idealAttackRange = weapon->getRules()->getRange();
			}
		}
	}
	else
	{
		_idealAttackRange = _unit->ufoRule->getWeaponRange();
		_maxAttackRange = _idealAttackRange;
	}
}

void AirCombatAI::setState(AirCombatState *state)
{
	_state = state;
}

void AirCombatAI::selectMode()
{
	switch (_mode)
	{
		case AAI_NONE:
			if (!_idealAttackRange)
			{
				_mode = AAI_ESCAPE;
			}
			else if (_unit->ufo->isEscort() && (findThreateningEnemy(_unit) || findThreateningEnemy(_state->getUfoUnit())))
			{
				_mode = AAI_BERSERKER;
			}
			else
			{
				_mode = AAI_SNIPE;
			}

			break;

		case AAI_SNIPE:
			if (_unit->ufo->isEscort())
			{
				if (_state->getUfoUnit()->position > _unit->position + 5)
				{
					_mode = AAI_ESCAPE;
				}
				else if (_unit->getHealthPercent() < 50 || findThreateningEnemy(_state->getUfoUnit()))
				{
					_mode = AAI_BERSERKER;
				}
			}
			else
			{
				if (_unit->getHealthPercent() < 75)
				{
					_mode = AAI_ESCAPE;
				}
			}
			break;

		case AAI_BERSERKER:
			if (!_unit->ufo->isEscort())
			{
				if (canEscape())
				{
					_mode = AAI_ESCAPE;
				}
			}
			else if (_state->getUfoUnit()->position > _unit->position + 5)
			{
				_mode = AAI_ESCAPE;
			}
			break;

		case AAI_ESCAPE:
			if (!_unit->ufo->isEscort() && !_unit->ufo->getEscorts().size() && !canEscape())
			{
				_mode = AAI_BERSERKER;
			}
			break;
	}
}

void AirCombatAI::performTurn()
{
	selectMode();

	AirCombatAction towardAction = moveTowardEnemy();
	AirCombatAction awayAction = moveAwayFromEnemy();
	AirCombatAction attackAction = getAttackTarget();
	AirCombatUnit *primaryUnit = _state->getUfoUnit();
	AirCombatUnit *threat = findThreateningEnemy(_unit);
	AirCombatUnit *closest = findClosestEnemy();

	double estimatedLife = getEstimatedLife(_unit);
	double estimatedLifeToward = getEstimatedLife(_unit, _unit->position - 1);
	double estimatedLifeAway = getEstimatedLife(_unit, _unit->position + 1);

	double targetEstimatedLife = closest ? getEstimatedLife(closest) : 1000000;
	double targetEstimatedLifeToward = closest ? getEstimatedLife(closest, -1, _unit, _unit->position - 1) : 1000000;
	double targetEstimatedLifeAway = closest ? getEstimatedLife(closest, -1, _unit, _unit->position + 1) : 1000000;

	bool tryEscape = canEscape();

	switch (_mode)
	{
		case AAI_SNIPE:
			if (tryEscape)
			{
				_mode = AAI_ESCAPE;
				_state->_currentAction = awayAction;
			}
			else if (targetEstimatedLifeToward < targetEstimatedLife && targetEstimatedLifeToward < estimatedLifeToward)
			{
				_state->_currentAction = attackAction;
			}
			else if (targetEstimatedLifeAway <= targetEstimatedLife && threat && awayAction && estimatedLifeAway > estimatedLife)
			{
				_state->_currentAction = awayAction;
			}
			else if (attackAction)
			{
				_state->_currentAction = attackAction;
			}
			else if (towardAction && closest && targetEstimatedLifeToward < estimatedLifeToward)
			{
				_state->_currentAction = towardAction;
			}
			else if (threat && awayAction && estimatedLifeAway > estimatedLife)
			{
				_state->_currentAction = awayAction;
			}
			else if (towardAction && estimatedLifeToward >= estimatedLife)
			{
				_state->_currentAction = towardAction;
			}
			break;

		case AAI_BERSERKER:
			if (attackAction)
			{
				_state->_currentAction = attackAction;
			}
			else if (towardAction)
			{
				_state->_currentAction = towardAction;
			}
			break;

		case AAI_ESCAPE:
			if (awayAction)
			{
				_state->_currentAction = awayAction;
			}
			break;
	}

	if (!_state->_currentAction)
	{
		_state->_currentAction.action = AA_EVADE;
	}

	_state->_currentAction.updateCost();

	_state->executeAction();
}

AirCombatUnit *AirCombatAI::findThreateningEnemy(AirCombatUnit *unit) const
{
	return findThreateningEnemy(unit->position);
}

AirCombatUnit *AirCombatAI::findThreateningEnemy(int position) const
{
	AirCombatUnit *threat = nullptr;

	for (AirCombatUnit *enemy : _state->_craft)
	{
		if (enemy && (!threat || enemy->position > threat->position) && enemy->canTargetPosition(position))
		{
			threat = enemy;
		}
	}

	return threat;
}

AirCombatUnit *AirCombatAI::findClosestEnemy() const
{
	AirCombatUnit *threat = nullptr;

	for (AirCombatUnit *enemy : _state->_craft)
	{
		if (enemy && (!threat || enemy->position > threat->position))
		{
			threat = enemy;
		}
	}

	return threat;
}

int AirCombatAI::countThreateningEnemies(int position) const
{
	if (position < 0) { position = _unit->position; }
	int threats = 0;
	for (AirCombatUnit *enemy : _state->_craft)
	{
		if (enemy && enemy->canTargetPosition(_unit->position))
		{
			++threats;
		}
	}

	return threats;
}

AirCombatAction AirCombatAI::moveTowardEnemy() const
{
	int maxEnemyPos = 0;

	for (AirCombatUnit *enemy : _state->_craft)
	{
		if (enemy && enemy->position > maxEnemyPos)
		{
			maxEnemyPos = enemy->position;
		}
	}

	if (maxEnemyPos < _unit->position - 1)
	{
		return AirCombatAction(_unit, AA_MOVE_BACKWARD, nullptr);
	}

	return AirCombatAction();
}

AirCombatAction AirCombatAI::moveAwayFromEnemy() const
{
	return AirCombatAction(_unit, AA_MOVE_FORWARD, nullptr);
}

//bool AirCombatAI::getAttackTarget(AirCombatUnit *&target, CraftWeapon *&weapon) const
AirCombatAction AirCombatAI::getAttackTarget() const
{
	AirCombatAction attack(_unit, AA_NONE, nullptr);

	if (_unit->ufoRule->getWeapons().size())
	{
		for (CraftWeapon *testWeapon : _unit->ufo->getWeapons())
		{
			int weaponRange = testWeapon->getRules()->getRange();
			int maxEnemyPosition = 0;
			attack.target = nullptr;
			for (AirCombatUnit *enemy : _state->_craft)
			{
				if (enemy && enemy->position > 0 && enemy->position > maxEnemyPosition)
				{
					maxEnemyPosition = enemy->position;
					attack.target = enemy;
				}
			}

			if (attack.target && (_unit->position - attack.target->position <= weaponRange))
			{
				attack.action = AA_FIRE_WEAPON;
				attack.weapon = testWeapon;
				return attack;
			}
		}
	}
	else
	{
		int weaponRange = _unit->ufoRule->getWeaponRange();
		if (weaponRange)
		{

			int maxEnemyPosition = 0;
			attack.target = nullptr;
			for (AirCombatUnit *enemy : _state->_craft)
			{
				if (enemy && enemy->position > 0 && enemy->position > maxEnemyPosition)
				{
					maxEnemyPosition = enemy->position;
					attack.target = enemy;
				}
			}

			if (attack.target && (_unit->position - attack.target->position <= weaponRange))
			{
				attack.action = AA_FIRE_WEAPON;
				return attack;
			}
		}
	}

	return attack;
}

double AirCombatAI::getEstimatedLife(AirCombatUnit *unit, int position, AirCombatUnit *testAttacker, int attackerPosition, bool escaping) const
{
	double incomingDpt = 0;
	if (position < 0) { position = unit->position; }

	/*if (attacker)
	{
		incomingDpt = getAttackerDpt(attacker, position, attackerPosition);
	}
	else
	{*/
	//std::array<AirCombatUnit*, AirCombatState::MAX_UNITS> &enemies = (unit->enemy ? _state->_craft : _state->_enemies);
	auto &enemies = (unit->enemy ? _state->_craft : _state->_enemies);
	for (AirCombatUnit *enemy : enemies)
	{
		if (enemy)
		{
			incomingDpt += getAttackerDpt(enemy, position, enemy == testAttacker ? attackerPosition : -1);
		}
	}
	//}

	if (AreSame(incomingDpt, 0.0))
	{
		return 1000000;
	}

	if (escaping)
	{
		incomingDpt /= 2.0;
	}

	return unit->getHealth() / incomingDpt;
}

double AirCombatAI::getAttackerDpt(AirCombatUnit *attacker, int position, int attackerPosition) const
{
	if (attackerPosition < 0) attackerPosition = attacker->position;
	int range = abs(attackerPosition - position);

	if (attacker->craft || attacker->ufo->getWeapons().size())
	{
		double bestDpt = 0.0;
		const auto &weapons = attacker->craft ? *attacker->craft->getWeapons() : attacker->ufo->getWeapons();
		for (CraftWeapon *weapon : weapons)
		{
			if (weapon->getRules()->getRange() >= range)
			{
				double weaponDpt = (double)weapon->getRules()->getDamage() * weapon->getRules()->getShotsAimed() * weapon->getRules()->getAccuracy() / 100 / weapon->getRules()->getCostAimed();
				if (weaponDpt > bestDpt)
				{
					bestDpt = weaponDpt;
				}
			}
		}

		return bestDpt;
	}
	else if (attacker->ufo)
	{
		if (attacker->ufoRule->getWeaponRange() >= range)
		{
			return (double)attacker->ufoRule->getWeaponPower() * attacker->ufoRule->getWeaponAccuracy() / 100 / 20;
		}
	}

	return 0.0;
}

double AirCombatAI::getAttackerPursuitTime(AirCombatUnit *attacker) const
{
	if (attacker)
	{
		AirCombatAction pursuit(attacker, AA_MOVE_PURSUE, nullptr);
		pursuit.updateCost();

		int fuel = pursuit.Energy + pursuit.Time / 2;

		int turns = (double)attacker->getCombatFuel() / fuel;

		return turns * pursuit.Time;
	}
	else
	{
		double maxTime = 0.0;

		for (AirCombatUnit *enemy : _state->_craft)
		{
			if (enemy)
			{
				maxTime = std::max(maxTime, getAttackerPursuitTime(enemy));
			}
		}

		return maxTime;
	}
}

bool AirCombatAI::canEscape() const
{
	// Adjust estimated life and pursuit time assuming half DPS time and half pursuit time.
	double estimatedLifeEscape = getEstimatedLife(_unit, 0, nullptr, 0, true) * 2.0;
	double pursuitTime = getAttackerPursuitTime() / 0.5;

	return estimatedLifeEscape > pursuitTime;
}

}
