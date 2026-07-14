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
#include "../Engine/Unicode.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Mod/Mod.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleInventory.h"
#include "ActionMenuItem.h"
#include "BattlescapeState.h"
#include "Projectile.h"
#include "PrimeGrenadeState.h"
#include "MedikitState.h"
#include "ScannerState.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Savegame/HitLog.h"
#include "Pathfinding.h"
#include "TileEngine.h"
#include "../Interface/Text.h"

namespace OpenXcom
{

namespace
{
/// The on-row hotkey label for a key, capitalized (e.g. "R" rather than SDL's lowercase "r").
std::string hotkeyLabel(SDLKey key)
{
	std::string name = SDL_GetKeyName(key);
	Unicode::upperCase(name);
	return name;
}
}

/**
 * Default constructor, used by SkillMenuState.
 */
ActionMenuState::ActionMenuState(BattleAction *action) : _action(action)
{
}

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

	for (int i = 0; i < 8; ++i)
	{
		_actionMenu[i] = new ActionMenuItem(i, _game, x, y);
		add(_actionMenu[i]);
		_actionMenu[i]->setVisible(false);
		_actionMenu[i]->onMouseClick((ActionHandler)&ActionMenuState::btnActionMenuItemClick);
	}

	// Build up the popup menu
	int id = 0;
	const RuleItem *weapon = _action->weapon->getRules();

	// DX: visible Quick Reload - surfaces the R-key reload (BattleUnit::reloadWeapon) as a menu row so
	// it's discoverable. Shown for a firearm using external clips with an empty ammo slot. Added first
	// so it sits at the very bottom of the menu (the last item). Hotkey keyBattleReload (R) matches the
	// global bind.
	if (weapon->getBattleType() == BT_FIREARM && _action->weapon->isWeaponWithAmmo() && !_action->weapon->haveAllAmmo())
	{
		addItem(BA_RELOAD, "STR_RELOAD", &id, Options::keyBattleReload);
	}

	// throwing (if not a fixed weapon). Also blocked when the item's slot won't release it:
	// a combat-locked slot (allowCombatSwap:false) forbids moving the item out during combat,
	// and a utility slot holds "use in place" gear that shouldn't be thrown. Throwing removes
	// the item from the slot, so it would otherwise bypass the combat-swap lock.
	const RuleInventory *slot = _action->weapon->getSlot();
	bool slotReleasesForThrow = !slot || (slot->getAllowCombatSwap() && slot->getType() != INV_UTILITY);
	if (!weapon->isFixed() && weapon->getCostThrow().Time > 0 && slotReleasesForThrow)
	{
		addItem(BA_THROW, "STR_THROW", &id, Options::keyBattleActionItem5);
	}

	if (weapon->isPsiRequired() && _action->actor->getBaseStats()->psiSkill <= 0)
	{
		return;
	}

	if (weapon->isManaRequired() && _action->actor->getOriginalFaction() == FACTION_PLAYER)
	{
		if (!_game->getMod()->isManaFeatureEnabled() || !_game->getSavedGame()->isManaUnlocked(_game->getMod()))
		{
			return;
		}
	}

	// priming
	if (weapon->getFuseTimerType() != BFT_NONE)
	{
		bool normalWeapon = weapon->getBattleType() != BT_GRENADE && weapon->getBattleType() != BT_FLARE && weapon->getBattleType() != BT_PROXIMITYGRENADE;
		if (_action->weapon->getFuseTimer() == -1)
		{
			if (weapon->getCostPrime().Time > 0)
			{
				addItem(BA_PRIME, weapon->getPrimeActionName(), &id, normalWeapon ? SDLK_UNKNOWN : Options::keyBattleActionItem1);
			}
		}
		else
		{
			if (weapon->getCostUnprime().Time > 0 && !weapon->getUnprimeActionName().empty())
			{
				addItem(BA_UNPRIME, weapon->getUnprimeActionName(), &id, normalWeapon ? SDLK_UNKNOWN : Options::keyBattleActionItem2);
			}
		}
	}

	if (weapon->getBattleType() == BT_FIREARM)
	{
		bool isLauncher = _action->weapon->getCurrentWaypoints() != 0;
		int slotLauncher = _action->weapon->getActionConf(BA_LAUNCH)->ammoSlot;
		int slotSnap = _action->weapon->getActionConf(BA_SNAPSHOT)->ammoSlot;
		int slotAuto = _action->weapon->getActionConf(BA_AUTOSHOT)->ammoSlot;
		int slotBurst = _action->weapon->getActionConf(BA_BURSTSHOT)->ammoSlot;

		// Menu rows stack with later-added items HIGHER on screen (y - id*25). Add order here is
		// bottom-to-top, so the displayed order (top->bottom) is: Aimed, Snap, Burst, Auto, Dual,
		// then Throw, then Reload at the very bottom (Reload and Throw were both added above this
		// block, Reload first so it's the last/bottom item).

		// DX dual-fire: fire both hands at once (shown when the unit holds two loaded, fire-capable
		// firearms). Each hand fires its own best mode; both hands are involved regardless of which
		// weapon this menu was opened for.
		if (_action->actor->canDualFire())
		{
			addItem(BA_DUALFIRE, "STR_DUAL_FIRE", &id, Options::keyBattleActionItem7);
		}

		if ((!isLauncher || slotLauncher != slotAuto) && weapon->getCostAuto().Time > 0)
		{
			addItem(BA_AUTOSHOT, weapon->getConfigAuto()->name, &id, Options::keyBattleActionItem3);
		}

		// DX: burst is a fourth fire mode between snap and auto; opt-in via tuBurst (like auto's tuAuto).
		// Uses the DX-added 6th hotkey (item5 is taken by Throw on throwable firearms).
		if ((!isLauncher || slotLauncher != slotBurst) && weapon->getCostBurst().Time > 0)
		{
			addItem(BA_BURSTSHOT, weapon->getConfigBurst()->name, &id, Options::keyBattleActionItem6);
		}

		if ((!isLauncher || slotLauncher != slotSnap) && weapon->getCostSnap().Time > 0)
		{
			addItem(BA_SNAPSHOT,  weapon->getConfigSnap()->name, &id, Options::keyBattleActionItem2);
		}

		if (isLauncher)
		{
			addItem(BA_LAUNCH, "STR_LAUNCH_MISSILE", &id, Options::keyBattleActionItem1);
		}
		else if (weapon->getCostAimed().Time > 0)
		{
			addItem(BA_AIMEDSHOT,  weapon->getConfigAimed()->name, &id, Options::keyBattleActionItem1);
		}

		// DX overwatch: set-and-hold reaction fire over a cone. Shown for a non-launcher firearm that
		// has overwatch enabled (range > 0) and can fire its configured overwatch shot mode.
		if (!isLauncher && weapon->getOverwatchRange() > 0 &&
			_action->actor->getActionTUs(weapon->getOverwatchShot(), _action->weapon).Time > 0)
		{
			addItem(BA_OVERWATCH, "STR_OVERWATCH", &id, Options::keyBattleActionItem8);
		}
	}

	if (weapon->getCostMelee().Time > 0)
	{
		std::string name = weapon->getConfigMelee()->name;
		if (name.empty())
		{
			// stun rod
			if (weapon->getBattleType() == BT_MELEE && weapon->getDamageType()->ResistType == DT_STUN)
			{
				name = "STR_STUN";
			}
			else
			// melee weapon
			{
				name = "STR_HIT_MELEE";
			}
		}
		addItem(BA_HIT, name, &id, Options::keyBattleActionItem4);
	}

	// special items
	if (weapon->getBattleType() == BT_MEDIKIT)
	{
		addItem(BA_USE, weapon->getMedikitActionName(), &id, Options::keyBattleActionItem1);
	}
	else if (weapon->getBattleType() == BT_SCANNER)
	{
		addItem(BA_USE, weapon->getPsiAttackName().empty() ? "STR_USE_SCANNER" : weapon->getPsiAttackName(), &id, Options::keyBattleActionItem1);
	}
	else if (weapon->getBattleType() == BT_PSIAMP)
	{
		// DX: a channeling unit can let its thralls go, for free, at any time. Offered ALONGSIDE the
		// normal psi actions rather than replacing them - a controller keeps all his abilities, he is
		// simply limited by the resources his upkeep is eating.
		if (_action->actor->isChanneling())
		{
			addItem(BA_RELEASE_MIND_CONTROL, "STR_DX_RELEASE_MIND_CONTROL", &id, Options::keyBattleActionItem4);
		}
		if (weapon->getCostMind().Time > 0)
		{
			addItem(BA_MINDCONTROL, "STR_MIND_CONTROL", &id, Options::keyBattleActionItem3);
		}
		if (weapon->getCostPanic().Time > 0)
		{
			addItem(BA_PANIC, "STR_PANIC_UNIT", &id, Options::keyBattleActionItem2);
		}
		if (weapon->getCostUse().Time > 0)
		{
			addItem(BA_USE, weapon->getPsiAttackName(), &id, Options::keyBattleActionItem1);
		}
	}
	else if (weapon->getBattleType() == BT_MINDPROBE)
	{
		addItem(BA_USE, weapon->getPsiAttackName().empty() ? "STR_USE_MIND_PROBE" : weapon->getPsiAttackName(), &id, Options::keyBattleActionItem1);
	}
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
void ActionMenuState::addItem(BattleActionType ba, const std::string &name, int *id, SDLKey key)
{
	std::string s1, s2;

	// DX dual-fire spans both hands (its own weapon/ammo/mode each), so it doesn't fit the
	// single-weapon accuracy/ammo/shots machinery below - give it its own compact row.
	if (ba == BA_DUALFIRE)
	{
		int dualTu = _action->actor->getDualFireCost().Time;
		s2 = tr("STR_TIME_UNITS_SHORT").arg(dualTu);
		_actionMenu[*id]->setAction(ba, tr(name), s1, s2, dualTu); // s1 empty: no single accuracy for two weapons
		_actionMenu[*id]->setVisible(true);
		if (key != SDLK_UNKNOWN)
		{
			_actionMenu[*id]->setHotkey(hotkeyLabel(key));
			_actionMenu[*id]->onKeyboardPress((ActionHandler)&ActionMenuState::btnActionMenuItemClick, key);
		}
		if (_action->actor->getTimeUnits() < dualTu)
		{
			_actionMenu[*id]->setUnaffordable(tr("STR_ACTION_NO_TU"));
		}
		(*id)++;
		return;
	}

	// DX Quick Reload: a compact utility row (name + reload TU), not a firing action - no accuracy or
	// shots. Flagged red when no compatible clip is carried (No Ammo) or unaffordable (No TU), so the
	// option is discoverable even when it can't currently be used.
	if (ba == BA_RELOAD)
	{
		int reloadCost = _action->actor->getReloadCost(_action->weapon);
		if (reloadCost >= 0)
		{
			s2 = tr("STR_TIME_UNITS_SHORT").arg(reloadCost);
		}
		_actionMenu[*id]->setAction(ba, tr(name), s1, s2, reloadCost >= 0 ? reloadCost : 0);
		_actionMenu[*id]->setVisible(true);
		if (key != SDLK_UNKNOWN)
		{
			_actionMenu[*id]->setHotkey(hotkeyLabel(key));
			_actionMenu[*id]->onKeyboardPress((ActionHandler)&ActionMenuState::btnActionMenuItemClick, key);
		}
		if (reloadCost < 0)
		{
			_actionMenu[*id]->setUnaffordable(tr("STR_ACTION_NO_AMMO"));
		}
		else if (_action->actor->getTimeUnits() < reloadCost)
		{
			_actionMenu[*id]->setUnaffordable(tr("STR_ACTION_NO_TU"));
		}
		(*id)++;
		return;
	}

	// DX overwatch: a compact row showing the overwatch shot's TU cost (the minimum to arm it; the
	// action then converts the actor's remaining TU into that many reaction shots). Red if unaffordable.
	if (ba == BA_OVERWATCH)
	{
		int owTu = _action->actor->getActionTUs(_action->weapon->getRules()->getOverwatchShot(), _action->weapon).Time;
		s2 = tr("STR_TIME_UNITS_SHORT").arg(owTu);
		_actionMenu[*id]->setAction(ba, tr(name), s1, s2, owTu);
		_actionMenu[*id]->setVisible(true);
		if (key != SDLK_UNKNOWN)
		{
			_actionMenu[*id]->setHotkey(hotkeyLabel(key));
			_actionMenu[*id]->onKeyboardPress((ActionHandler)&ActionMenuState::btnActionMenuItemClick, key);
		}
		if (!_action->weapon->getAmmoForAction(_action->weapon->getRules()->getOverwatchShot()))
		{
			_actionMenu[*id]->setUnaffordable(tr("STR_ACTION_NO_AMMO"));
		}
		else if (_action->actor->getTimeUnits() < owTu)
		{
			_actionMenu[*id]->setUnaffordable(tr("STR_ACTION_NO_TU"));
		}
		(*id)++;
		return;
	}

	int acc = BattleUnit::getFiringAccuracy(BattleActionAttack::GetBeforeShoot(ba, _action->actor, _action->weapon), _game->getMod());
	int tu = _action->actor->getActionTUs(ba, _action->weapon).Time;

	const RuleItem *weaponRule = _action->weapon->getRules();
	// aim-cone weapons: the per-mode "accuracy %" is only the soldier-cone input, not a hit chance,
	// so for direct-fire modes show the 50%-hit effective range (tiles) instead - the number that
	// actually tells the player how far this mode stays reliable.
	bool coneShot = weaponRule->getBaseAccuracy() > 0
		&& (ba == BA_SNAPSHOT || ba == BA_AIMEDSHOT || ba == BA_AUTOSHOT || ba == BA_BURSTSHOT);
	if (coneShot)
	{
		int spread = 100;
		const BattleItem *coneAmmo = _action->weapon->getAmmoForAction(ba);
		if (coneAmmo && coneAmmo->getRules()->getShotgunPellets() != 0)
			spread = coneAmmo->getRules()->getShotgunSpread();
		int effRange = Projectile::calculateEffectiveRange(acc, weaponRule->getBaseAccuracy(), spread);
		s1 = tr("STR_EFFECTIVE_RANGE_SHORT").arg(effRange);
	}
	else if (ba == BA_THROW || ba == BA_AIMEDSHOT || ba == BA_SNAPSHOT || ba == BA_AUTOSHOT || ba == BA_BURSTSHOT || ba == BA_LAUNCH || ba == BA_HIT)
		s1 = tr("STR_ACCURACY_SHORT").arg(Unicode::formatPercentage(acc));
	s2 = tr("STR_TIME_UNITS_SHORT").arg(tu);
	_actionMenu[*id]->setAction(ba, tr(name), s1, s2, tu);
	_actionMenu[*id]->setVisible(true);

	// on-row hotkey label
	if (key != SDLK_UNKNOWN)
	{
		_actionMenu[*id]->setHotkey(hotkeyLabel(key));
		_actionMenu[*id]->onKeyboardPress((ActionHandler)&ActionMenuState::btnActionMenuItemClick, key);
	}

	// action config + loaded ammo for this mode (conf/ammo are null for non-shot actions)
	const RuleItemAction *conf = _action->weapon->getActionConf(ba);
	const BattleItem *ammo = nullptr;
	if (ba == BA_SNAPSHOT || ba == BA_AIMEDSHOT || ba == BA_AUTOSHOT || ba == BA_BURSTSHOT || ba == BA_LAUNCH || ba == BA_HIT)
	{
		ammo = _action->weapon->getAmmoForAction(ba);
	}

	// shot count for multi-shot modes, plus shotgun pellets (shown separately)
	int shots = conf ? conf->shots : 0;
	int pellets = ammo ? ammo->getRules()->getShotgunPellets() : 0;
	std::string shotsText;
	if (shots > 1)
	{
		shotsText = tr("STR_ACTION_SHOTS_SHORT").arg(shots);
	}
	if (pellets > 0)
	{
		std::string pelletsText = tr("STR_ACTION_PELLETS_SHORT").arg(pellets);
		shotsText = shotsText.empty() ? pelletsText : shotsText + " " + pelletsText;
	}
	if (!shotsText.empty())
	{
		_actionMenu[*id]->setShots(shotsText);
	}

	// affordability (all flagged but still clickable):
	//   red     - can't perform at all: not enough TU, or no usable ammo
	//   warning - some ammo, but fewer rounds than a full multi-shot burst needs
	std::string reason;
	bool partialAmmo = false;
	if (_action->actor->getTimeUnits() < tu)
	{
		reason = tr("STR_ACTION_NO_TU");
	}
	else if (ba == BA_SNAPSHOT || ba == BA_AIMEDSHOT || ba == BA_AUTOSHOT || ba == BA_BURSTSHOT || ba == BA_LAUNCH || ba == BA_HIT)
	{
		const int perShot = (conf && conf->spendPerShot > 0) ? conf->spendPerShot : 1;
		// self-powered weapons return themselves from getAmmoForAction - not clip-limited
		const bool clipBased = ammo && ammo != _action->weapon;
		const int available = clipBased ? ammo->getAmmoQuantity() : -1;
		if (!ammo || (clipBased && available < perShot))
		{
			reason = tr("STR_ACTION_NO_AMMO");
		}
		else if (clipBased && shots > 1 && available < shots * perShot)
		{
			partialAmmo = true;
		}
	}
	if (!reason.empty())
	{
		_actionMenu[*id]->setUnaffordable(reason);
	}
	else if (partialAmmo)
	{
		_actionMenu[*id]->setPartialAmmo();
	}

	(*id)++;
}

/**
 * Closes the window on right-click.
 * @param action Pointer to an action.
 */
void ActionMenuState::handle(Action *action)
{
	State::handle(action);
	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN && _game->isRightClick(action))
	{
		_game->popState();
	}
	else if (action->getDetails()->type == SDL_KEYDOWN)
	{
		auto key = action->getDetails()->key.keysym.sym;
		if (key == Options::keyCancel || key == Options::keyBattleUseLeftHand || key == Options::keyBattleUseRightHand || key == Options::keyBattleUseUtility)
		{
			if (key != Options::keyBattleActionItem1 &&
				key != Options::keyBattleActionItem2 &&
				key != Options::keyBattleActionItem3 &&
				key != Options::keyBattleActionItem4 &&
				key != Options::keyBattleActionItem5 &&
				key != Options::keyBattleActionItem6)
			{
				_game->popState();
			}
		}
	}
}

/**
 * Executes the action corresponding to this action menu item.
 * @param action Pointer to an action.
 */
void ActionMenuState::btnActionMenuItemClick(Action *action)
{
	// Two menu items can share a hotkey (e.g. Throw and Burst), so a single keypress
	// may invoke this handler more than once in the same event dispatch. Only ever act
	// on the first one - a second action (and its extra popState) must not slip through.
	if (_actionChosen)
	{
		return;
	}

	_game->getSavedGame()->getSavedBattle()->getPathfinding()->removePreview();

	int btnID = -1;

	if (_game->getSavedGame()->getSavedBattle()->isPreview())
	{
		_actionChosen = true;
		_action->result = "STR_UNABLE_TO_USE_ALIEN_ARTIFACT_UNTIL_RESEARCHED";
		_game->popState();
		return;
	}

	// got to find out which button was pressed
	for (size_t i = 0; i < std::size(_actionMenu) && btnID == -1; ++i)
	{
		if (action->getSender() == _actionMenu[i])
		{
			btnID = i;
		}
	}

	if (btnID != -1)
	{
		_actionChosen = true;
		_action->type = _actionMenu[btnID]->getAction();
		_action->skillRules = nullptr;
		_action->updateTU();

		handleAction();
	}
}

void ActionMenuState::handleAction()
{
	// reset potential garbage from the previous action
	_action->terrainMeleeTilePart = 0;

	{
		const RuleItem *weapon = _action->weapon->getRules();
		bool newHitLog = false;
		std::string actionResult = "STR_UNKNOWN"; // needs a non-empty default/fall-back !

		// BA_RELOAD is a handling action (loading ammo), not "using" the weapon's function, so - like
		// BA_THROW - it's exempt from the research and canUseWeapon (needs-ammo) gates below; those
		// would otherwise block reloading the very unloaded weapon we're trying to reload.
		if (_action->type != BA_THROW && _action->type != BA_RELOAD &&
			_action->actor->getOriginalFaction() == FACTION_PLAYER &&
			!_game->getSavedGame()->isResearched(weapon->getRequirements()))
		{
			_action->result = "STR_UNABLE_TO_USE_ALIEN_ARTIFACT_UNTIL_RESEARCHED";
			_game->popState();
		}
		else if (_action->type != BA_THROW && _action->type != BA_RELOAD &&
			!_game->getSavedGame()->getSavedBattle()->canUseWeapon(_action->weapon, _action->actor, false, _action->type, &actionResult))
		{
			_action->result = actionResult;
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
		else if (_action->type == BA_OVERWATCH && _action->actor->isOnOverwatch())
		{
			// DX: selecting Overwatch while already on it toggles it off (a free cancel). When not on
			// overwatch it falls through to the generic targeting setup below (aim the cone).
			_action->actor->clearOverwatch();
			SavedBattleGame *save = _game->getSavedGame()->getSavedBattle();
			if (save->getBattleState())
			{
				save->getBattleState()->updateSoldierInfo();
			}
			_action->type = BA_NONE;
			_game->popState();
		}
		else if (_action->type == BA_RELEASE_MIND_CONTROL)
		{
			// DX: free the thralls. Costs nothing - letting go is not an effort, and charging for it would
			// just trap a controller who can no longer afford the upkeep.
			SavedBattleGame *save = _game->getSavedGame()->getSavedBattle();
			save->breakAllMindControl(_action->actor);
			_action->actor->setChannelWeapon("", nullptr);
			if (save->getBattleState())
			{
				save->getBattleState()->updateSoldierInfo();
			}
			save->getTileEngine()->calculateFOV(_action->actor->getPosition());
			_action->type = BA_NONE;
			_game->popState();
		}
		else if (_action->type == BA_RELOAD)
		{
			// DX: reload this weapon immediately (no targeting) via the shared reload-and-report call,
			// so the menu item and the R key behave identically (reload + sound + refresh).
			SavedBattleGame *save = _game->getSavedGame()->getSavedBattle();
			if (save->getBattleState())
			{
				save->getBattleState()->quickReload(_action->actor, _action->weapon);
			}
			_game->popState();
		}
		else if (_action->type == BA_USE && weapon->getBattleType() == BT_MEDIKIT)
		{
			BattleUnit *targetUnit = 0;
			TileEngine *tileEngine = _game->getSavedGame()->getSavedBattle()->getTileEngine();
			for (auto* bu : *_game->getSavedGame()->getSavedBattle()->getUnits())
			{
				// we can heal a unit that is at the same position, unconscious and healable(=woundable)
				if (bu->getPosition() == _action->actor->getPosition() &&
					bu != _action->actor &&
					bu->getStatus() == STATUS_UNCONSCIOUS &&
					(bu->isWoundable() || weapon->getAllowTargetImmune()) &&
					weapon->getAllowTargetGround())
				{
					if (bu->isBigUnit())
					{
						// never EVER apply anything to 2x2 units on the ground
						continue;
					}
					if ((weapon->getAllowTargetFriendGround() && bu->getOriginalFaction() == FACTION_PLAYER) ||
						(weapon->getAllowTargetNeutralGround() && bu->getOriginalFaction() == FACTION_NEUTRAL) ||
						(weapon->getAllowTargetHostileGround() && bu->getOriginalFaction() == FACTION_HOSTILE))
					{
						targetUnit = bu;
						break; // loop finished
					}
				}
			}
			if (!targetUnit && weapon->getAllowTargetStanding())
			{
				if (tileEngine->validMeleeRange(
					_action->actor->getPosition(),
					_action->actor->getDirection(),
					_action->actor,
					0, &_action->target, false))
				{
					Tile *tile = _game->getSavedGame()->getSavedBattle()->getTile(_action->target);
					if (tile != 0 && tile->getUnit() && (tile->getUnit()->isWoundable() || weapon->getAllowTargetImmune()))
					{
						if ((weapon->getAllowTargetFriendStanding() && tile->getUnit()->getOriginalFaction() == FACTION_PLAYER) ||
							(weapon->getAllowTargetNeutralStanding() && tile->getUnit()->getOriginalFaction() == FACTION_NEUTRAL) ||
							(weapon->getAllowTargetHostileStanding() && tile->getUnit()->getOriginalFaction() == FACTION_HOSTILE))
						{
							targetUnit = tile->getUnit();
						}
					}
				}
			}
			if (!targetUnit && weapon->getAllowTargetSelf())
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
										if (targetUnit->getFatalWound((UnitBodyPart)i))
										{
											tileEngine->medikitUse(_action, targetUnit, BMA_HEAL, (UnitBodyPart)i);
											tileEngine->medikitRemoveIfEmpty(_action);
											break;
										}
									}
								}
								else
								{
									tileEngine->medikitUse(_action, targetUnit, BMA_HEAL, BODYPART_TORSO);
									tileEngine->medikitRemoveIfEmpty(_action);
								}
								break;
							case BMT_STIMULANT:
								tileEngine->medikitUse(_action, targetUnit, BMA_STIMULANT, BODYPART_TORSO);
								tileEngine->medikitRemoveIfEmpty(_action);
								break;
							case BMT_PAINKILLER:
								tileEngine->medikitUse(_action, targetUnit, BMA_PAINKILLER, BODYPART_TORSO);
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
				_game->pushState (new ScannerState(_action));
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
				if (!_game->getSavedGame()->getSavedBattle()->getTileEngine()->validTerrainMeleeRange(_action))
				{
					_action->result = "STR_THERE_IS_NO_ONE_THERE";
				}
			}
			else
			{
				newHitLog = true;
			}
			_game->popState();
		}
		else
		{
			_action->targeting = true;
			newHitLog = true;
			_game->popState();
		}

		// meleeAttackBState won't be available to clear the action type, do it here instead.
		if (_action->type == BA_HIT && !_action->result.empty())
		{
			_action->type = BA_NONE;
		}

		if (newHitLog)
		{
			_game->getSavedGame()->getSavedBattle()->appendToHitLog(HITLOG_PLAYER_FIRING, FACTION_PLAYER, tr(weapon->getType()));
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
