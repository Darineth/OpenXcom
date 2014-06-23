/*
 * Copyright 2010-2014 OpenXcom Developers.
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
#include "ActionMenuState.h"
#include <sstream>
#include <cmath>
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Resource/ResourcePack.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Engine/Action.h"
#include "../Engine/Sound.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Ruleset/RuleItem.h"
#include "ActionMenuItem.h"
#include "PrimeGrenadeState.h"
#include "MedikitState.h"
#include "ScannerState.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "Pathfinding.h"
#include "TileEngine.h"
#include "../Interface/Text.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/Ruleset.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Action Menu window.
 * @param game Pointer to the core game.
 * @param action Pointer to the action.
 * @param x Position on the x-axis.
 * @param y position on the y-axis.
 */
ActionMenuState::ActionMenuState(Game *game, BattleAction *action, int x, int y) : State(game), _action(action)
{
	_screen = false;

	// Set palette
	setPalette("PAL_BATTLESCAPE");

	for (int i = 0; i < MAX_ACTIONS; ++i)
	{
		_actionMenu[i] = new ActionMenuItem(i, _game, x, y);
		add(_actionMenu[i]);
		_actionMenu[i]->setVisible(false);
		_actionMenu[i]->onMouseClick((ActionHandler)&ActionMenuState::btnActionMenuItemClick);
	}

	// Build up the popup menu
	int id = 0;
	RuleItem *weapon = _action->weapon->getRules();

	// throwing (if not a fixed weapon)
	if (!weapon->isFixed())
	{
		addItem(BA_THROW, "STR_THROW", &id);
	}

	// priming
	if ((weapon->getBattleType() == BT_GRENADE || weapon->getBattleType() == BT_PROXIMITYGRENADE)
		&& _action->weapon->getFuseTimer() == -1)
	{
		addItem(BA_PRIME, "STR_PRIME_GRENADE", &id);
	}

	if (weapon->getBattleType() == BT_FIREARM)
	{
		if (weapon->isWaypoint() || (_action->weapon->getAmmoItem() && _action->weapon->getAmmoItem()->getRules()->isWaypoint()))
		{
			addItem(BA_LAUNCH, "STR_LAUNCH_MISSILE", &id);
		}
		else
		{
			if (weapon->getAccuracyAuto() != 0)
			{
				addItem(BA_AUTOSHOT, "STR_AUTO_SHOT", &id);
			}
			if (weapon->getAccuracySnap() != 0)
			{
				addItem(BA_SNAPSHOT, "STR_SNAP_SHOT", &id);
			}
			if (weapon->getAccuracyAimed() != 0)
			{
				addItem(BA_AIMEDSHOT, "STR_AIMED_SHOT", &id);
			}
		}

		if(_action->actor->hasInventory()
			&&_action->weapon->needsAmmo()
			&& (_action->weapon->getAmmoItem() == 0 
				|| (_action->weapon->getAmmoItem()->getAmmoQuantity() < _action->weapon->getAmmoItem()->getRules()->getClipSize())
					&& !(_action->weapon->getSlot()->getId() == "STR_LEFT_HAND" ? action->actor->getItem("STR_RIGHT_HAND") : action->actor->getItem("STR_LEFT_HAND"))))
		{
			addItem(BA_RELOAD, "STR_RELOAD", &id);
		}
	}

	if (weapon->getTUMelee())
	{
		// stun rod
		if (weapon->getBattleType() == BT_MELEE && weapon->getDamageType() == DT_STUN)
		{
			addItem(BA_HIT, "STR_STUN", &id);
		}
		else
		// melee weapon
		{
			addItem(BA_HIT, "STR_HIT_MELEE", &id);
		}
	}
	// special items
	else if (weapon->getBattleType() == BT_MEDIKIT)
	{
		addItem(BA_USE, "STR_USE_MEDI_KIT", &id);
	}
	else if (weapon->getBattleType() == BT_SCANNER)
	{
		addItem(BA_USE, "STR_USE_SCANNER", &id);
	}
	else if (weapon->getBattleType() == BT_PSIAMP && _action->actor->getStats()->psiSkill > 0)
	{
		addItem(BA_MINDCONTROL, "STR_MIND_CONTROL", &id);
		addItem(BA_PANIC, "STR_PANIC_UNIT", &id);
	}
	else if (weapon->getBattleType() == BT_MINDPROBE)
	{
		addItem(BA_USE, "STR_USE_MIND_PROBE", &id);
	}
}

/**
 * Deletes the ActionMenuState.
 */
ActionMenuState::~ActionMenuState()
{

}

/**
 * Adds a new menu item for an action.
 * @param ba Action type.
 * @param name Action description.
 * @param id Pointer to the new item ID.
 */
void ActionMenuState::addItem(BattleActionType ba, const std::string &name, int *id)
{
	std::wstring s1, s2;
	int acc = _action->actor->getFiringAccuracy(ba, _action->weapon);
	if (ba == BA_THROW)
		acc = (int)(_action->actor->getThrowingAccuracy());
	int tu = _action->actor->getActionTUs(ba, _action->weapon);

	BattleItem* ammoItem = _action->weapon->getAmmoItem();
	int ammo = ammoItem ? ammoItem->getAmmoQuantity() : 0;
	bool ammoError;

	int shots;

	switch(ba)
	{
	case BA_AUTOSHOT:
		shots = _action->weapon->getRules()->getAutoShots();
		ammoError = (shots > ammo);
		break;

	case BA_MINDCONTROL:
	case BA_PANIC:
	case BA_PRIME:
	case BA_RETHINK:
	case BA_STUN:
	case BA_THROW:
	case BA_TURN:
	case BA_USE:
	case BA_WALK:
		shots = 0;
		ammoError = false;
		break;

	case BA_RELOAD:
		shots = 0;
		ammoError = !_action->actor->findQuickAmmo(_action->weapon, &tu);

		if(_action->weapon->getAmmoItem())
		{
			tu += _action->weapon->getSlot()->getCost(_game->getRuleset()->getInventory("STR_GROUND"));
		}

		break;

	default:
		shots = 1;
		ammoError = (ammo == 0);
	}
	
	if (ba == BA_THROW || ba == BA_AIMEDSHOT || ba == BA_SNAPSHOT || ba == BA_AUTOSHOT || ba == BA_LAUNCH || ba == BA_HIT)
	{
		if(shots > 1)
		{
			s1 = tr("STR_ACCURACY_SHORT").arg(Text::formatPercentage(acc).append(L"x").append(Text::formatNumber(shots)));
		}
		else
		{
			s1 = tr("STR_ACCURACY_SHORT").arg(Text::formatPercentage(acc));
		}
	}

	int key = (*id) + 1;

	s2 = tr("STR_TIME_UNITS_SHORT").arg(tu);
	_actionMenu[*id]->setAction(ba, Text::formatNumber(key) + L". " + tr(name).c_str(), s1, s2, tu, ammoError, tu > _action->actor->getTimeUnits());
	_actionMenu[*id]->setVisible(true);
	_actionMenu[*id]->onKeyboardPress((ActionHandler)&ActionMenuState::btnActionMenuItemClick, (SDLKey)((int)SDLK_0 + key));
	(*id)++;
}

/**
 * Closes the window on right-click.
 * @param action Pointer to an action.
 */
void ActionMenuState::handle(Action *action)
{
	State::handle(action);
	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN && action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_game->popState();
	}
	else if (action->getDetails()->type == SDL_KEYDOWN &&
		(action->getDetails()->key.keysym.sym == Options::keyCancel ||
		action->getDetails()->key.keysym.sym == Options::keyBattleUseLeftHand ||
		action->getDetails()->key.keysym.sym == Options::keyBattleUseRightHand))
	{
		_game->popState();
	}
}

/**
 * Executes the action corresponding to this action menu item.
 * @param action Pointer to an action.
 */
void ActionMenuState::btnActionMenuItemClick(Action *action)
{
	_game->getSavedGame()->getSavedBattle()->getPathfinding()->removePreview();

	int btnID = -1;
	RuleItem *weapon = _action->weapon->getRules();

	// got to find out which button was pressed
	for (size_t i = 0; i < sizeof(_actionMenu)/sizeof(_actionMenu[0]) && btnID == -1; ++i)
	{
		if (action->getSender() == _actionMenu[i])
		{
			btnID = i;
		}
	}

	if (btnID != -1)
	{
		_action->type = _actionMenu[btnID]->getAction();
		_action->description = _actionMenu[btnID]->getDescription();
		_action->TU = _actionMenu[btnID]->getTUs();
		if (_action->type == BA_PRIME)
		{
			if (weapon->getBattleType() == BT_PROXIMITYGRENADE)
			{
				_action->value = 0;
				_game->popState();
			}
			else
			{
				_game->pushState(new PrimeGrenadeState(_game, _action, false, 0));
			}
		}
		else if (_action->type == BA_USE && weapon->getBattleType() == BT_MEDIKIT)
		{
			BattleUnit *targetUnit = NULL;
			std::vector<BattleUnit*> *const units (_game->getSavedGame()->getSavedBattle()->getUnits());
			for(std::vector<BattleUnit*>::const_iterator i = units->begin (); i != units->end () && !targetUnit; ++i)
			{
				// we can heal a unit that is at the same position, unconscious and healable(=woundable)
				if ((*i)->getPosition() == _action->actor->getPosition() && *i != _action->actor && (*i)->getStatus () == STATUS_UNCONSCIOUS && (*i)->isWoundable())
				{
					targetUnit = *i;
				}
			}
			if (!targetUnit)
			{
				if (_game->getSavedGame()->getSavedBattle()->getTileEngine()->validMeleeRange(
					_action->actor->getPosition(),
					_action->actor->getDirection(),
					_action->actor,
					0, &_action->target))
				{
					Tile * tile (_game->getSavedGame()->getSavedBattle()->getTile(_action->target));
					if (tile != 0 && tile->getUnit() && tile->getUnit()->isWoundable())
					{
						targetUnit = tile->getUnit();
					}
				}
			}
			if (targetUnit)
			{
				_game->popState();
				_game->pushState (new MedikitState (_game, targetUnit, _action));
			}
			else
			{
				_action->result = "STR_THERE_IS_NO_ONE_THERE";
				_game->popState();
			}
		}
		else if (_action->type == BA_USE && weapon->getBattleType() == BT_SCANNER)
		{
			// spend TUs first, then show the scanner
			if (_action->actor->spendTimeUnits (_action->TU))
			{
				_game->popState();
				_game->pushState (new ScannerState (_game, _action));
			}
			else
			{
				_action->result = "STR_NOT_ENOUGH_TIME_UNITS";
				_game->popState();
			}
		}
		else if (_action->type == BA_LAUNCH)
		{
			// check beforehand if we have enough time units
			if (_action->TU > _action->actor->getTimeUnits())
			{
				_action->result = "STR_NOT_ENOUGH_TIME_UNITS";
			}
			else if (_action->weapon->getAmmoItem() ==0 || (_action->weapon->getAmmoItem() && _action->weapon->getAmmoItem()->getAmmoQuantity() == 0))
			{
				_action->result = "STR_NO_AMMUNITION_LOADED";
			}
			else
			{
				_action->targeting = true;
			}
			_game->popState();
		}
		else if (_action->type == BA_STUN || _action->type == BA_HIT)
		{
			if (!_game->getSavedGame()->getSavedBattle()->getTileEngine()->validMeleeRange(
				_action->actor->getPosition(),
				_action->actor->getDirection(),
				_action->actor,
				0, &_action->target))
			{
				_action->result = "STR_THERE_IS_NO_ONE_THERE";
			}
			_game->popState();
		}
		else if(_action->type == BA_RELOAD)
		{
			int tu = 0;
			BattleItem *quickAmmo = _action->actor->findQuickAmmo(_action->weapon, &tu);

			RuleInventory *ground = _game->getRuleset()->getInventory("STR_GROUND");

			if(_action->weapon->getAmmoItem())
			{
				// Adjusted unload cost: Magazine weight + hand->hand
				tu += Options::battleAdjustReloadCost ? (_action->weapon->getAmmoItem()->getRules()->getWeight() + _action->weapon->getSlot()->getCost(ground)) : 8;
			}

			if(!quickAmmo)
			{
				// Temporary mesage.
				_action->result = "STR_NO_ROUNDS_LEFT";
			}
			else if(!_action->actor->spendTimeUnits(tu))
			{
				_action->result = "STR_NOT_ENOUGH_TIME_UNITS";
			}
			else
			{
				_action->TU = tu;
			}

			_game->popState();
		}
		else
		{
			_action->targeting = true;
			_game->popState();
		}
	}
}

/**
 * Updates the scale.
 * @param dX delta of X;
 * @param dY delta of Y;
 */
void ActionMenuState::resize(int &dX, int &dY)
{
	State::recenter(dX, dY * 2);
}

}
