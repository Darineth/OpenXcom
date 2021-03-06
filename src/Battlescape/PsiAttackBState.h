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
#include "BattleState.h"
#include "BattlescapeGame.h"

namespace OpenXcom
{

class BattlescapeGame;
class BattleUnit;
class BattleItem;
class Tile;
class BattleActionAttack;
class RuleDamageType;

/**
 * A Psi Attack state.
 */
class PsiAttackBState : public BattleState
{
private:
	BattleUnit *_unit, *_target;
	BattleItem *_item;
	bool _initialized;
	BattleActionAttack _attack;
	const RuleDamageType *_psychicDamage;
public:
	/// Creates a new PsiAttack state.
	PsiAttackBState(BattlescapeGame *parent, BattleAction action);
	/// Cleans up the PsiAttack.
	~PsiAttackBState();
	/// Initializes the state.
	void init();
	/// Runs state functionality every cycle.
	void think();
	/// Attempts a panic or mind control action.
	void psiAttack();

};

}
