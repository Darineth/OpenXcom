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

#include "../Engine/Game.h"
#include "AirActionMenuState.h"

namespace OpenXcom
{

class Ufo;
class AirCombatUnit;
class AirCombatState;
class CraftWeapon;

enum AirCombatActionType : Uint8;

enum AirAIMode { AAI_NONE, AAI_SNIPE, AAI_BERSERKER, AAI_ESCAPE };

/*struct AirCombatAIUnitData
{
	int idealAttackRange;
	int maxAttackRange;
	int idealRangeDps;
	int maxRangeDps;
};*/

class AirCombatAI
{
private:
	AirCombatUnit *_unit;
	AirCombatState *_state;

	AirAIMode _mode;

	int _idealAttackRange;
	int _maxAttackRange;

	AirCombatUnit *findThreateningEnemy(AirCombatUnit *unit) const;
	AirCombatUnit *findThreateningEnemy(int position) const;
	AirCombatUnit *findClosestEnemy() const;
	int countThreateningEnemies(int position = -1) const;
	AirCombatAction moveTowardEnemy() const;
	AirCombatAction moveAwayFromEnemy() const;
	AirCombatAction getAttackTarget() const;

	double getEstimatedLife(AirCombatUnit *unit, int position = -1, AirCombatUnit *testAttacker = nullptr, int attackerPosition = -1, bool escaping = false) const;
	double getAttackerDpt(AirCombatUnit *attacker, int position, int attackerPosition = -1) const;

	double getAttackerPursuitTime(AirCombatUnit *attacker = nullptr) const;

	bool canEscape() const;

	//bool getAttackTarget(AirCombatUnit *&target, CraftWeapon *&weapon) const;

	void selectMode();

public:
	AirCombatAI(AirCombatUnit *unit);
	~AirCombatAI() {}

	void setState(AirCombatState *state);

	void performTurn();
};

}
