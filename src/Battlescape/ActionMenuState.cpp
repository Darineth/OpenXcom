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
#include "../Engine/Language.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Action.h"
#include "../Engine/Sound.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Mod/Mod.h"
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
	//BattleItem *ammo = _action->weapon->getAmmoItem();
	const RuleItem *weaponRules = weapon->getRules();

	switch (_action->actor->getOngoingAction())
	{
		case BA_OVERWATCH:
			addItem(BA_OVERWATCH, "STR_CANCEL_OVERWATCH", &id, Options::keyActionOverwatch);
			break;
		case BA_MINDCONTROL:
			addItem(BA_MINDCONTROL, "STR_CANCEL_MIND_CONTROL", &id, Options::keyActionOverwatch);
			break;
		default:
			// throwing (if not a fixed weapon)
			if (!weaponRules->isFixed() && !_action->actor->isVehicle() && weapon->getSlot()->getAllowCombatSwap())
			{
				addItem(BA_THROW, "STR_THROW", &id, Options::keyActionThrow);
			}

			// priming
			if ((weaponRules->getBattleType() == BT_GRENADE || weaponRules->getBattleType() == BT_PROXIMITYGRENADE))
			{
				if (!_action->weapon->getGrenadeLive())
				{
					if (weaponRules->getCostPrime().Time > 0)
					{
						addItem(BA_PRIME, weaponRules->getPrimeActionName(), &id, Options::keyActionPrime);
					}
				}
				else
				{
					if (weaponRules->getCostPrime().Time > 0)
					{
						addItem(BA_UNPRIME, weaponRules->getUnprimeActionName(), &id, Options::keyActionPrime);
					}
				}
			}

			if (weaponRules->getBattleType() == BT_FIREARM)
			{
				if (_action->actor->getOverwatchShotAction(weaponRules) != BA_NONE && !_action->actor->isVehicle())
				{
					if (_action->actor->onOverwatch())
					{
						addItem(BA_OVERWATCH, "STR_CANCEL_OVERWATCH", &id, Options::keyActionOverwatch);
					}
					else
					{
						addItem(BA_OVERWATCH, "STR_OVERWATCH", &id, Options::keyActionOverwatch);
					}
				}

				if (_action->actor->canDualFire())
				{
					addItem(BA_DUALFIRE, "STR_DUAL_FIRE", &id, Options::keyActionDualFire);
				}

				const RuleItemAction *launch = weapon->getActionConf(BA_LAUNCH);
				const RuleItemAction *autoShot = weapon->getActionConf(BA_AUTOSHOT);
				const RuleItemAction *burstShot = weapon->getActionConf(BA_BURSTSHOT);
				const RuleItemAction *aimedShot = weapon->getActionConf(BA_AIMEDSHOT);
				const RuleItemAction *snapShot = weapon->getActionConf(BA_SNAPSHOT);

				if (weaponRules->getWaypoints() > 1 || (weapon->getAmmoForSlot(launch->ammoSlot) && weapon->getAmmoForSlot(launch->ammoSlot)->getRules()->getWaypoints() > 1))
				{
					addItem(BA_LAUNCH, "STR_LAUNCH_MISSILE", &id, Options::keyActionLaunch);
				}
				else
				{
					if (weaponRules->getAccuracyAuto() != 0)
					{
						addItem(BA_AUTOSHOT, "STR_AUTO_SHOT", &id, Options::keyActionAuto);
					}
					if (weaponRules->getAccuracyBurst() != 0)
					{
						addItem(BA_BURSTSHOT, "STR_BURST_SHOT", &id, Options::keyActionBurst);
					}
					if (weaponRules->getAccuracyAimed() != 0)
					{
						addItem(BA_AIMEDSHOT, "STR_AIMED_SHOT", &id, Options::keyActionAimed);
					}
					if (weaponRules->getAccuracySnap() != 0)
					{
						addItem(BA_SNAPSHOT, "STR_SNAP_SHOT", &id, Options::keyActionSnap);
					}
				}
			}

			std::string dummy;
			if (_action->actor->hasInventory()
				&& weapon->isWeaponWithAmmo()
				&& !weapon->haveFullAmmo()
				&& _action->actor->reloadWeapon(weapon, dummy, false, true))
			{
				addItem(BA_RELOAD, "STR_RELOAD", &id, Options::keyActionReload);
			}

			if (weaponRules->getCostMelee().Time > 0)
			{
				std::string name = weaponRules->getConfigMelee()->name;
				if (name.empty())
				{
					// stun rod
					if (weaponRules->getBattleType() == BT_MELEE && weaponRules->getDamageType()->ResistType == DT_STUN)
					{
						name = "STR_STUN";
					}
					else
						// melee weapon
					{
						name = "STR_HIT_MELEE";
					}
				}
				addItem(BA_HIT, name, &id, Options::keyActionMelee);
			}
			// special items
			else if (weaponRules->getBattleType() == BT_MEDIKIT)
			{
				addItem(BA_USE, "STR_USE_MEDI_KIT", &id, Options::keyActionUse);
			}
			else if (weaponRules->getBattleType() == BT_SCANNER)
			{
				addItem(BA_USE, "STR_USE_SCANNER", &id, Options::keyActionUse);
			}
			else if (weaponRules->getBattleType() == BT_PSIAMP && _action->actor->getBaseStats()->psiSkill > 0)
			{
				addItem(BA_MINDBLAST, "STR_MIND_BLAST", &id, Options::keyActionPsi1);

				if (_action->actor->getBaseStats()->psiStrength + _action->actor->getBaseStats()->psiSkill >= 100)
				{
					addItem(BA_CLAIRVOYANCE, "STR_CLAIRVOYANCE", &id, Options::keyActionPsi2);
				}
				addItem(BA_PANIC, "STR_PANIC_UNIT", &id, Options::keyActionPsi3);
				addItem(BA_MINDCONTROL, "STR_MIND_CONTROL", &id, Options::keyActionPsi4);
			}
			else if (weaponRules->getBattleType() == BT_MINDPROBE)
			{
				addItem(BA_USE, "STR_USE_MIND_PROBE", &id, Options::keyActionUse);
			}
	}

	Element *actionMenu = Game::getGame()->getMod()->getInterface("battlescape")->getElement("actionMenu");

	_selectedItem = new Text(270, 9, x + 25, y);
	add(_selectedItem);
	_selectedItem->setColor(actionMenu->color);
	_selectedItem->setHighContrast(true);
	_selectedItem->setY(y - (id * 25) - 16);
	_selectedItem->setAlign(ALIGN_CENTER);

	std::wostringstream ss;
	ss << tr(weaponRules->getName());
	BattleItem *primaryAmmo = weapon->getAmmoForSlot(0);
	if (primaryAmmo && (primaryAmmo != weapon)) { ss << " [" << tr(primaryAmmo->getRules()->getName()) << "]"; }
	_selectedItem->setText(ss.str());
}

/**
 * Deletes the ActionMenuState.
 */
ActionMenuState::~ActionMenuState()
{

}

/**
 * Init function.
 */
void ActionMenuState::init()
{
	if (!_actionMenu[0]->getVisible())
	{
		// Item don't have any actions, close popup.
		_game->popState();
	}
}

/**
 * Adds a new menu item for an action.
 * @param ba Action type.
 * @param name Action description.
 * @param id Pointer to the new item ID.
 */
void ActionMenuState::addItem(BattleActionType ba, const std::string &name, int *id, SDLKey key, BattleItem *secondaryWeapon)
{
	std::wstring s1, s2;
	int acc = _action->actor->getFiringAccuracy(ba, _action->weapon, Game::getMod(), false, ba == BA_DUALFIRE);
	if (ba == BA_THROW)
		acc = (int)(_action->actor->getThrowingAccuracy());
	int tu = _action->actor->getActionTUs(ba, _action->weapon).Time;

	if (!_usedHotkeys.insert(key).second)
	{
		key = SDLK_UNKNOWN;
	}

	std::string ammoErrorMessage;
	const RuleItemAction *conf = _action->weapon->getActionConf(ba);

	int ammoSlot = conf ? conf->ammoSlot : -1;
	bool needsAmmo = ammoSlot > -1;

	BattleItem* ammoItem = needsAmmo ? _action->weapon->getAmmoForSlot(ammoSlot) : nullptr;
	int ammo = ammoItem ? ammoItem->getAmmoQuantity() : 0;
	bool ammoError;

	int shots = 1;
	int bulletsPerShot = 1;

	if (ammoItem)
	{
		bulletsPerShot = ammoItem->getRules()->getShotgunPellets();
	}

	if (bulletsPerShot < 1) { bulletsPerShot = 1; }

	if (ba == _action->actor->getOngoingAction())
	{
		shots = 0;
		ammoError = false;
		tu = 0;
	}
	else
	{
		switch (ba)
		{
			case BA_AUTOSHOT:
				shots = _action->weapon->getRules()->getConfigAuto()->shots;
				ammoError = needsAmmo && (ammo < shots);
				break;

			case BA_BURSTSHOT:
				shots = _action->weapon->getRules()->getConfigBurst()->shots;
				ammoError = needsAmmo && (ammo < shots);
				break;

			case BA_PANIC:
				ammoError = (shots = _action->weapon->getRules()->getCostPanic().Ammo) > ammo;
				break;

			case BA_MINDCONTROL:
				ammoError = (shots = _action->weapon->getRules()->getCostMind().Ammo) > ammo;
				break;

			case BA_CLAIRVOYANCE:
				ammoError = (shots = _action->weapon->getRules()->getCostClairvoyance().Ammo) > ammo;
				break;

			case BA_MINDBLAST:
				ammoError = (shots = _action->weapon->getRules()->getCostMindBlast().Ammo) > ammo;
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
				if (_action->actor->onOverwatch())
				{
					shots = 0;
					ammoError = false;
					break;
				}
				else
				{
					BattleActionType overwatchShot = _action->actor->getOverwatchShotAction(_action->weapon->getRules());
					switch (overwatchShot)
					{
						case BA_AUTOSHOT:
							shots = _action->weapon->getRules()->getConfigAuto()->shots;
							break;
						case BA_BURSTSHOT:
							shots = _action->weapon->getRules()->getConfigBurst()->shots;
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

				ammoError = ammo < shots;

				break;
			}

			case BA_DUALFIRE:
				shots = 1;
				bulletsPerShot = 1;

				BattleItem *weapon1;
				BattleActionType ba1;
				const RuleItemAction *ia1;
				BattleItem *weapon2;
				BattleActionType ba2;
				const RuleItemAction *ia2;
				int tu1;
				int tu2;
				if (_action->actor->canDualFire(weapon1, ba1, tu1, weapon2, ba2, tu2))
				{
					int shots1;
					switch (ba1)
					{
						case BA_AUTOSHOT:
							shots1 = weapon1->getRules()->getConfigAuto()->shots;
							break;
						case BA_BURSTSHOT:
							shots1 = weapon1->getRules()->getConfigBurst()->shots;
							break;
						default:
							shots1 = 1;
					}

					ia1 = weapon1->getActionConf(ba1);

					int shots2;
					switch (ba2)
					{
						case BA_AUTOSHOT:
							shots2 = weapon2->getRules()->getConfigAuto()->shots;
							break;
						case BA_BURSTSHOT:
							shots2 = weapon2->getRules()->getConfigBurst()->shots;
							break;
						default:
							shots2 = 1;
					}

					ia2 = weapon2->getActionConf(ba2);

					shots = shots1 + shots2;
					BattleItem *ammo1 = weapon1->getAmmoForAction(ba1);
					BattleItem *ammo2 = weapon2->getAmmoForAction(ba2);
					ammoError = (ia1->ammoSlot > -1 && (!ammo1 || shots1 > ammo1->getAmmoQuantity())) || (ia2->ammoSlot > -1 && (!ammo2 || shots2 > ammo2->getAmmoQuantity()));
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
		else if (shots && (ba == BA_THROW || ba == BA_LAUNCH || ba == BA_HIT))
		{
			if (shots > 1 && bulletsPerShot > 1)
			{
				s1 = tr("STR_ACCURACY_SHORT").arg(Text::formatPercentage(acc).append(L" ").append(Text::formatNumber(shots)).append(L"x").append(Text::formatNumber(bulletsPerShot)));
			}
			else if (shots > 1)
			{
				s1 = tr("STR_ACCURACY_SHORT").arg(Text::formatPercentage(acc).append(L" ").append(Text::formatNumber(shots)).append(L"x"));
			}
			else if (bulletsPerShot > 1)
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

			if (shots > 1 && bulletsPerShot > 1)
			{
				s1 = tr("STR_RANGE_SHORT").arg(Text::formatNumber(acc).append(L" ").append(Text::formatNumber(shots)).append(L"x").append(Text::formatNumber(bulletsPerShot)));
			}
			else if (shots > 1)
			{
				s1 = tr("STR_RANGE_SHORT").arg(Text::formatNumber(acc).append(L" ").append(Text::formatNumber(shots)).append(L"x"));
			}
			else if (bulletsPerShot > 1)
			{
				s1 = tr("STR_RANGE_SHORT").arg(Text::formatNumber(acc).append(L" x").append(Text::formatNumber(bulletsPerShot)));
			}
			else
			{
				s1 = tr("STR_RANGE_SHORT").arg(Text::formatNumber(acc));
			}
		}
	}

	std::wstring keyName;

	s2 = tr("STR_TIME_UNITS_SHORT").arg(Text::formatNumber(tu)).arg(Text::formatNumber(_action->actor->getTimeUnits()));
	_actionMenu[*id]->setVisible(true);
	if (key != SDLK_UNKNOWN)
	{
		keyName = Language::utf8ToWstr(SDL_GetKeyName(key)) + L". ";
		std::transform(keyName.begin(), keyName.end(), keyName.begin(), toupper);
		_actionMenu[*id]->onKeyboardPress((ActionHandler)&ActionMenuState::btnActionMenuItemClick, key);
	}
	_actionMenu[*id]->setAction(ba, keyName + tr(name).c_str(), s1, s2, tu, ammoError, tu > _action->actor->getAvailableTimeUnits(), tu > _action->actor->getTimeUnits());
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
	const RuleItem *weapon = _action->weapon->getRules();

	// got to find out which button was pressed
	for (size_t i = 0; i < sizeof(_actionMenu) / sizeof(_actionMenu[0]) && btnID == -1; ++i)
	{
		if (action->getSender() == _actionMenu[i])
		{
			btnID = i;
		}
	}

	if (btnID != -1)
	{
		bool newHitLog = false;

		_action->type = _actionMenu[btnID]->getAction();
		_action->description = _actionMenu[btnID]->getDescription();
		_action->updateTU();
		if (_action->type != BA_THROW &&
			_action->actor->getOriginalFaction() == FACTION_PLAYER &&
			!_game->getSavedGame()->isResearched(weapon->getRequirements()))
		{
			_action->result = "STR_UNABLE_TO_USE_ALIEN_ARTIFACT_UNTIL_RESEARCHED";
			_game->popState();
		}
		else if (weapon->isWaterOnly() &&
			_game->getSavedGame()->getSavedBattle()->getDepth() == 0 &&
			_action->type != BA_THROW)
		{
			_action->result = "STR_UNDERWATER_EQUIPMENT";
			_game->popState();
		}
		else if (_action->type != BA_THROW &&
			_action->actor->getFaction() == FACTION_PLAYER &&
			weapon->isBlockingBothHands() &&
			_action->actor->getWeapon1() != 0 &&
			_action->actor->getWeapon2() != 0)
		{
			_action->result = "STR_MUST_USE_BOTH_HANDS";
			_game->popState();
		}
		else if (_action->type == BA_PRIME)
		{
			const BattleFuseType fuseType = weapon->getFuseTimerType();
			if (fuseType == BFT_SET)
			{
				_game->pushState(new PrimeGrenadeState(_action, false, 0));
			}
			else
			{
				_action->value = weapon->getFuseTimerDefault();
				_game->popState();
			}
		}
		else if (_action->type == BA_UNPRIME)
		{
			_game->popState();
		}
		else if (_action->type == BA_USE && weapon->getBattleType() == BT_MEDIKIT)
		{
			BattleUnit *targetUnit = 0;
			TileEngine *tileEngine = _game->getSavedGame()->getSavedBattle()->getTileEngine();
			const std::vector<BattleUnit*> *units = _game->getSavedGame()->getSavedBattle()->getUnits();
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
				if (tileEngine->validMeleeRange(
					_action->actor->getPosition(),
					_action->actor->getDirection(),
					_action->actor,
					0, &_action->target, false))
				{
					Tile *tile = _game->getSavedGame()->getSavedBattle()->getTile(_action->target);
					if (tile != 0 && tile->getUnit() && tile->getUnit()->isWoundable())
					{
						targetUnit = tile->getUnit();
					}
				}
			}
			if (!targetUnit && weapon->getAllowSelfHeal())
			{
				targetUnit = _action->actor;
			}
			if (targetUnit)
			{
				_game->popState();
				BattleMediKitType type = weapon->getMediKitType();
				if (type)
				{
					if ((type == BMT_HEAL && _action->weapon->getHealQuantity() > 0) ||
						(type == BMT_STIMULANT && _action->weapon->getStimulantQuantity() > 0) ||
						(type == BMT_PAINKILLER && _action->weapon->getPainKillerQuantity() > 0))
					{
						if (_action->spendTU(&_action->result))
						{
							switch (type)
							{
								case BMT_HEAL:
									if (targetUnit->getFatalWounds())
									{
										for (int i = 0; i < BODYPART_MAX; ++i)
										{
											if (targetUnit->getFatalWound(i))
											{
												tileEngine->medikitHeal(_action, targetUnit, i);
												tileEngine->medikitRemoveIfEmpty(_action);
												break;
											}
										}
									}
									else
									{
										tileEngine->medikitHeal(_action, targetUnit, BODYPART_TORSO);
										tileEngine->medikitRemoveIfEmpty(_action);
									}
									break;
								case BMT_STIMULANT:
									tileEngine->medikitStimulant(_action, targetUnit);
									tileEngine->medikitRemoveIfEmpty(_action);
									break;
								case BMT_PAINKILLER:
									tileEngine->medikitPainKiller(_action, targetUnit);
									tileEngine->medikitRemoveIfEmpty(_action);
									break;
								case BMT_NORMAL:
									break;
							}
						}
					}
					else
					{
						_action->result = "STR_NO_USES_LEFT";
					}
				}
				else
				{
					_game->pushState(new MedikitState(targetUnit, _action, tileEngine));
				}
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
			if (_action->spendTU(&_action->result))
			{
				_game->popState();
				_game->pushState(new ScannerState(_action));
			}
			else
			{
				_game->popState();
			}
		}
		else if (_action->type == BA_LAUNCH)
		{
			// check beforehand if we have enough time units
			if (!_action->haveTU(&_action->result))
			{
				//nothing
			}
			else if (!_action->weapon->getAmmoForAction(BA_LAUNCH, &_action->result))
			{
				//nothing
			}
			else
			{
				_action->targeting = true;
				newHitLog = true;
			}
			_game->popState();
		}
		else if (_action->type == BA_HIT)
		{
			// check beforehand if we have enough time units
			if (!_action->haveTU(&_action->result))
			{
				//nothing
			}
			else if (!_game->getSavedGame()->getSavedBattle()->getTileEngine()->validMeleeRange(
				_action->actor->getPosition(),
				_action->actor->getDirection(),
				_action->actor,
				0, &_action->target))
			{
				_action->result = "STR_THERE_IS_NO_ONE_THERE";
			}
			else
			{
				newHitLog = true;
			}
			_game->popState();
		}
		else if (_action->type == BA_EXECUTE)
		{
			if (_action->spendTU(&_action->result))
			{
				if (_action->origWeapon != 0)
				{
					// kill unit
					_action->origWeapon->getUnit()->instaKill();
					// convert inventory item to corpse
					RuleItem *corpseRules = _game->getMod()->getItem(_action->origWeapon->getUnit()->getArmor()->getCorpseBattlescape()[0]); // we're in an inventory, so we must be a 1x1 unit
					_action->origWeapon->convertToCorpse(corpseRules);
					// inform the player
					_action->result = "STR_TARGET_WAS_EXECUTED";
					// audio feedback
					if (_action->weapon->getRules()->getMeleeHitSound() > -1)
					{
						_game->getMod()->getSoundByDepth(_game->getSavedGame()->getSavedBattle()->getDepth(), _action->weapon->getRules()->getMeleeHitSound())->play();
					}
				}
			}
			_game->popState();
		}
		else if (_action->type == BA_RELOAD)
		{
			_action->actor->reloadWeapon(_action->weapon, _action->result, true, false, &_action->Time, &_action->ammo);
			_game->popState();
		}
		else
		{
			_action->targeting = true;
			//newHitLog = true;
			_game->popState();
		}

		/* TODO: Investigate this Hit Log thing?  It's marked with FIXME in OXCE+
		if (newHitLog)
		{
			// start new hit log
			_game->getSavedGame()->getSavedBattle()->hitLog.str(L"");
			_game->getSavedGame()->getSavedBattle()->hitLog.clear();
			// log weapon
			_game->getSavedGame()->getSavedBattle()->hitLog << tr("STR_HIT_LOG_WEAPON") << L": " << tr(weapon->getType()) << L"\n\n";
		}*/
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
