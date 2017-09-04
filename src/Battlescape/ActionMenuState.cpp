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
#include "ActionMenuState.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Action.h"
#include "../Engine/Sound.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Mod/RuleItem.h"
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
#include "../Mod/RuleInventory.h"
#include "../Mod/RuleInterface.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Action Menu window.
 * @param game Pointer to the core game.
 * @param action Pointer to the action.
 * @param x Position on the x-axis.
 * @param y position on the y-axis.
 */
ActionMenuState::ActionMenuState(BattleAction *action, int x, int y) : _action(action)
{
	_screen = false;

	// Set palette
	_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);

	for (int i = 0; i < MAX_ACTIONS; ++i)
	{
		_actionMenu[i] = new ActionMenuItem(i, _game, x, y);
		add(_actionMenu[i]);
		_actionMenu[i]->setVisible(false);
		_actionMenu[i]->onMouseClick((ActionHandler)&ActionMenuState::btnActionMenuItemClick);
	}

	// Build up the popup menu
	int id = 0;
	BattleItem *weapon = _action->weapon;
	BattleItem *ammo = _action->weapon->getAmmoItem();
	RuleItem *weaponRules = _action->weapon->getRules();

	switch(_action->actor->getOngoingAction())
	{
	case BA_OVERWATCH:
		addItem(BA_OVERWATCH, "STR_CANCEL_OVERWATCH", &id);
		break;
	case BA_MINDCONTROL:
		addItem(BA_MINDCONTROL, "STR_CANCEL_MIND_CONTROL", &id);
		break;
	default:
		// throwing (if not a fixed weapon)
		if (!weaponRules->isFixed() && !_action->actor->isVehicle() && weapon->getSlot()->getAllowCombatSwap())
		{
			addItem(BA_THROW, "STR_THROW", &id);
		}

		// priming
		if ((weaponRules->getBattleType() == BT_GRENADE || weaponRules->getBattleType() == BT_PROXIMITYGRENADE)
			&& !_action->weapon->getGrenadeLive())
		{
			addItem(BA_PRIME, "STR_PRIME_GRENADE", &id);
		}

		if (weaponRules->getBattleType() == BT_FIREARM)
		{
			if (weaponRules->getWaypoints() > 1 || (weapon->getAmmoItem() && weapon->getAmmoItem()->getRules()->getWaypoints() > 1))
			{
				addItem(BA_LAUNCH, "STR_LAUNCH_MISSILE", &id);
			}
			else
			{
				if (weaponRules->getAccuracyAuto() != 0)
				{
					addItem(BA_AUTOSHOT, "STR_AUTO_SHOT", &id);
				}
				if(weaponRules->getAccuracyBurst())
				{
					addItem(BA_BURSTSHOT, "STR_BURST_SHOT", &id);
				}
				if (weaponRules->getAccuracySnap() != 0)
				{
					addItem(BA_SNAPSHOT, "STR_SNAP_SHOT", &id);
				}
				if (weaponRules->getAccuracyAimed() != 0)
				{
					addItem(BA_AIMEDSHOT, "STR_AIMED_SHOT", &id);
				}
			}

			//if(_game->getSavedGame()->getSavedBattle()->getTileEngine()->canMakeReactionShot(_action->actor, 0, true, weapon))
			if(_action->actor->getOverwatchShotAction(weaponRules) != BA_NONE && !_action->actor->isVehicle())
			{
				if(_action->actor->onOverwatch())
				{
					addItem(BA_OVERWATCH, "STR_CANCEL_OVERWATCH", &id);
				}
				else
				{
					addItem(BA_OVERWATCH, "STR_OVERWATCH", &id);
				}
			}
		}

		if(_action->actor->canDualFire())
		{
			addItem(BA_DUALFIRE, "STR_DUAL_FIRE", &id);
		}

		std::string dummy;
		if(_action->actor->hasInventory()
			&& weapon->needsAmmo()
			&& (weapon->getAmmoItem() == 0 
				|| (weapon->getAmmoItem()->getAmmoQuantity() < std::max(weapon->getAmmoItem()->getRules()->getClipSize(), weapon->getAmmoItem()->getRules()->getBattleClipSize())))
			&& _action->actor->unloadWeapon(weapon, dummy, false, true))
			/*&& (_action->actor->isVehicle()
				|| !(weapon->getSlot()->getId() == "STR_LEFT_HAND" ? action->actor->getItem("STR_RIGHT_HAND") : action->actor->getItem("STR_LEFT_HAND"))))*/
		{
			addItem(BA_RELOAD, "STR_RELOAD", &id);
		}

		if (weaponRules->getTUMelee())
		{
			// stun rod
			if (weaponRules->getBattleType() == BT_MELEE && weaponRules->getDamageType() == DT_STUN)
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
		else if (weaponRules->getBattleType() == BT_MEDIKIT)
		{
			addItem(BA_USE, "STR_USE_MEDI_KIT", &id);
		}
		else if (weaponRules->getBattleType() == BT_SCANNER)
		{
			addItem(BA_USE, "STR_USE_SCANNER", &id);
		}
		else if (weaponRules->getBattleType() == BT_PSIAMP && _action->actor->getBaseStats()->psiSkill > 0)
		{
			addItem(BA_MINDBLAST, "STR_MIND_BLAST", &id);

			if (_action->actor->getBaseStats()->psiStrength + _action->actor->getBaseStats()->psiSkill >= 100)
			{
				addItem(BA_CLAIRVOYANCE, "STR_CLAIRVOYANCE", &id);
			}
			addItem(BA_PANIC, "STR_PANIC_UNIT", &id);
			addItem(BA_MINDCONTROL, "STR_MIND_CONTROL", &id);
		}
		else if (weaponRules->getBattleType() == BT_MINDPROBE)
		{
			addItem(BA_USE, "STR_USE_MIND_PROBE", &id);
		}
	}

	Element *actionMenu = Game::getGame()->getMod()->getInterface("battlescape")->getElement("actionMenu");
	
	_selectedItem = new Text(270, 9, x + 25, y);
	add(_selectedItem);
	_selectedItem->setColor(actionMenu->color);
	_selectedItem->setHighContrast(true);
	_selectedItem->setY(y - (id*25) - 16);
	_selectedItem->setAlign(ALIGN_CENTER);

	std::wostringstream ss;
	ss << tr(weaponRules->getName());
	if(ammo && (ammo != weapon)) { ss << " [" << tr(ammo->getRules()->getName()) << "]"; }
	_selectedItem->setText(ss.str());
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
	int acc = _action->actor->getFiringAccuracy(ba, _action->weapon, false, ba == BA_DUALFIRE);
	if (ba == BA_THROW)
		acc = (int)(_action->actor->getThrowingAccuracy());
	int tu = _action->actor->getActionTUs(ba, _action->weapon);

	BattleItem* ammoItem = _action->weapon->getAmmoItem();
	int ammo = ammoItem ? ammoItem->getAmmoQuantity() : 0;
	bool ammoError;

	int shots = 1;
	int bulletsPerShot = 1;

	if(ammoItem)
	{
		bulletsPerShot = ammoItem->getRules()->getShotgunPellets();
	}

	if(bulletsPerShot < 1) { bulletsPerShot = 1; }

	if(ba == _action->actor->getOngoingAction())
	{
		shots = 0;
		ammoError = false;
		tu = 0;
	}
	else
	{
		switch(ba)
		{
		case BA_AUTOSHOT:
			shots = _action->weapon->getRules()->getAutoShots();
			ammoError = _action->weapon->needsAmmo() && (ammo < shots);
			break;

		case BA_BURSTSHOT:
			shots = _action->weapon->getRules()->getBurstShots();
			ammoError = _action->weapon->needsAmmo() && (ammo < shots);
			break;

		case BA_PANIC:
			ammoError = (shots = _action->weapon->getRules()->getPsiCostPanic()) > ammo;
			break;

		case BA_MINDCONTROL:
			ammoError = (shots = _action->weapon->getRules()->getPsiCostMindControl()) > ammo;
			break;

		case BA_CLAIRVOYANCE:
			ammoError = (shots = _action->weapon->getRules()->getPsiCostClairvoyance()) > ammo;
			break;

		case BA_MINDBLAST:
			ammoError = (shots = _action->weapon->getRules()->getPsiCostMindBlast()) > ammo;
			break;

		case BA_HIT:
		case BA_PRIME:
		case BA_RETHINK:
		case BA_THROW:
		case BA_TURN:
		case BA_USE:
		case BA_WALK:
			shots = 0;
			ammoError = false;
			break;

		case BA_RELOAD:
			shots = 0;
			ammoError = !_action->actor->findQuickAmmo(_action->weapon);

			/*tu = _action->actor->getActionTUs(this->_game->getSavedGame()->getSavedBattle()

			if(_action->weapon->getAmmoItem())
			{
				tu += _action->weapon->getSlot()->getCost(_game->getMod()->getInventory("STR_GROUND"));
			}

			if(_action->actor->isVehicle())
			{
				tu = 0;
			}*/

			break;

		case BA_OVERWATCH:
			{
				if(_action->actor->onOverwatch())
				{
					shots = 0;
					ammoError = false;
					break;
				}
				else
				{
					BattleActionType overwatchShot = _action->actor->getOverwatchShotAction(_action->weapon->getRules());
					switch(overwatchShot)
					{
					case BA_AUTOSHOT:
						shots = _action->weapon->getRules()->getAutoShots();
						break;
					case BA_BURSTSHOT:
						shots = _action->weapon->getRules()->getBurstShots();
						break;
					case BA_SNAPSHOT:
						shots = 1;
						break;
					case BA_AIMEDSHOT:
						shots = 1;
						break;
					default:
						shots = 1;
					}
				}
			}

			ammoError = ammo < shots;

			break;

		case BA_DUALFIRE:
			shots = 1;
			bulletsPerShot = 1;

			BattleItem *weapon1;
			BattleActionType ba1;
			BattleItem *weapon2;
			BattleActionType ba2;
			int tu1;
			int tu2;
			if(_action->actor->canDualFire(weapon1, ba1, tu1, weapon2, ba2, tu2))
			{
				int shots1;
				switch (ba1)
				{
				case BA_AUTOSHOT:
					shots1 = weapon1->getRules()->getAutoShots();
					break;
				case BA_BURSTSHOT:
					shots1 = weapon1->getRules()->getBurstShots();
					break;
				default:
					shots1 = 1;
				}

				int shots2;
				switch (ba2)
				{
				case BA_AUTOSHOT:
					shots2 = weapon2->getRules()->getAutoShots();
					break;
				case BA_BURSTSHOT:
					shots2 = weapon2->getRules()->getBurstShots();
					break;
				default:
					shots2 = 1;
				}

				shots = shots1 + shots2;
				BattleItem *ammo1 = weapon1->getAmmoItem();
				BattleItem *ammo2 = weapon2->getAmmoItem();
				ammoError = (weapon1->needsAmmo() && (!ammo1 || shots1 > ammo1->getAmmoQuantity())) || (weapon2->needsAmmo() && (!ammo2 || shots2 > ammo2->getAmmoQuantity()));
			}

			break;

		default:
			shots = 1;
			ammoError = (ammo < shots);
		}

		if (shots && (ba == BA_PANIC || ba == BA_MINDCONTROL || ba == BA_CLAIRVOYANCE || ba == BA_MINDBLAST))
		{
			s1 = Text::formatNumber(shots).append(L"x");
		}
		else if (shots  && (ba == BA_THROW || ba == BA_LAUNCH || ba == BA_HIT))
		{
			if(shots > 1 && bulletsPerShot > 1)
			{
				s1 = tr("STR_ACCURACY_SHORT").arg(Text::formatPercentage(acc).append(L" ").append(Text::formatNumber(shots)).append(L"x").append(Text::formatNumber(bulletsPerShot)));
			}
			else if(shots > 1)
			{
				s1 = tr("STR_ACCURACY_SHORT").arg(Text::formatPercentage(acc).append(L" ").append(Text::formatNumber(shots)).append(L"x"));
			}
			else if(bulletsPerShot > 1)
			{
				s1 = tr("STR_ACCURACY_SHORT").arg(Text::formatPercentage(acc).append(L" x").append(Text::formatNumber(bulletsPerShot)));
			}
			else
			{
				s1 = tr("STR_ACCURACY_SHORT").arg(Text::formatPercentage(acc));
			}
		}
		else if (shots && (ba == BA_AIMEDSHOT || ba == BA_SNAPSHOT || ba == BA_AUTOSHOT || ba == BA_OVERWATCH || ba == BA_DUALFIRE || ba == BA_BURSTSHOT))
		{	//New accuracy model
			BattleUnit* actor = _action->actor;
			acc = actor->calculateEffectiveRangeForAction(ba, _action->weapon, false);

			if(shots > 1 && bulletsPerShot > 1)
			{
				s1 = tr("STR_RANGE_SHORT").arg(Text::formatNumber(acc).append(L" ").append(Text::formatNumber(shots)).append(L"x").append(Text::formatNumber(bulletsPerShot)));
			}
			else if(shots > 1)
			{
				s1 = tr("STR_RANGE_SHORT").arg(Text::formatNumber(acc).append(L" ").append(Text::formatNumber(shots)).append(L"x"));
			}
			else if(bulletsPerShot > 1)
			{
				s1 = tr("STR_RANGE_SHORT").arg(Text::formatNumber(acc).append(L" x").append(Text::formatNumber(bulletsPerShot)));
			}
			else
			{
				s1 = tr("STR_RANGE_SHORT").arg(Text::formatNumber(acc));
			}
		}
	}

	int key = (*id) + 1;

	s2 = tr("STR_TIME_UNITS_SHORT").arg(Text::formatNumber(tu)).arg(Text::formatNumber(_action->actor->getTimeUnits()));
	_actionMenu[*id]->setAction(ba, Text::formatNumber(key) + L". " + tr(name).c_str(), s1, s2, tu, ammoError, tu > _action->actor->getAvailableTimeUnits(), tu > _action->actor->getTimeUnits());
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
		if (_action->type != BA_THROW &&
			_action->actor->getOriginalFaction() == FACTION_PLAYER &&
			!_game->getSavedGame()->isResearched(weapon->getRequirements()))
		{
			_action->result = "STR_UNABLE_TO_USE_ALIEN_ARTIFACT_UNTIL_RESEARCHED";
			_game->popState();
		}
		else if (_action->type != BA_THROW &&
			!_game->getSavedGame()->getSavedBattle()->isItemUsable(weapon))
		{
			if (weapon->isWaterOnly())
			{
				_action->result = "STR_UNDERWATER_EQUIPMENT";
			}
			else if (weapon->isLandOnly())
			{
				_action->result = "STR_LAND_EQUIPMENT";
			}
			_game->popState();
		}
		else if(_action->actor->getOngoingAction() != BA_NONE && _action->type == _action->actor->getOngoingAction())
		{
			_game->popState();
		}
		else if (_action->type == BA_PRIME)
		{
			if (weapon->getBattleType() == BT_PROXIMITYGRENADE)
			{
				_action->value = 0;
				_game->popState();
			}
			else
			{
				_game->pushState(new PrimeGrenadeState(_action, false, 0));
			}
		}
		else if (_action->type == BA_USE && weapon->getBattleType() == BT_MEDIKIT)
		{
			BattleUnit *targetUnit = NULL;
			std::vector<BattleUnit*> *const units (_game->getSavedGame()->getSavedBattle()->getUnits());
			for (std::vector<BattleUnit*>::const_iterator i = units->begin(); i != units->end() && !targetUnit; ++i)
			{
				// we can heal a unit that is at the same position, unconscious and healable(=woundable)
				if ((*i)->getPosition() == _action->actor->getPosition() && *i != _action->actor && (*i)->getStatus() == STATUS_UNCONSCIOUS && (*i)->isWoundable())
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
					0, &_action->target, false))
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
				_game->pushState (new MedikitState(targetUnit, _action));
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
			if (_action->actor->spendTimeUnits(_action->TU))
			{
				_game->popState();
				_game->getSavedGame()->getSavedBattle()->getTileEngine()->detectMotion(_action);
				_game->pushState(new ScannerState(_action));
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
			if (_action->TU > _action->actor->getAvailableTimeUnits())
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
		else if (_action->type == BA_HIT)
		{
			// check beforehand if we have enough time units
			if (_action->TU > _action->actor->getAvailableTimeUnits())
			{
				_action->result = "STR_NOT_ENOUGH_TIME_UNITS";
			}
			else if (!_game->getSavedGame()->getSavedBattle()->getTileEngine()->validMeleeRange(
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
			_action->actor->reloadWeapon(_action->weapon, _action->result, true, false, &_action->TU);
			/*BattleItem *quickAmmo = _action->actor->findQuickAmmo(_action->weapon);

			int tu = _action->actor->getActionTUs(BA_RELOAD, _action->weapon);

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
			}*/

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
