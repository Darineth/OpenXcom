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
#include "AirActionMenuState.h"
#include "AirCombatState.h"
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
#include "../Savegame/SavedGame.h"
#include "../Interface/Text.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Savegame/CraftWeapon.h"
#include "../Interface/Frame.h"
#include "../Savegame/Ufo.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Action Menu window.
 * @param game Pointer to the core game.
 * @param action Pointer to the action.
 * @param x Position on the x-axis.
 * @param y position on the y-axis.
 */
AirActionMenuState::AirActionMenuState(AirCombatState *parent, AirCombatAction *action, AirCombatActionType options, int x, int y) : _parent(parent), _action(action)
{
	_screen = false;

	// Set palette
	//_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);
	setPalette("PAL_BATTLESCAPE");

	for (int i = 0; i < MAX_ACTIONS; ++i)
	{
		_actionMenu[i] = new AirActionMenuItem(i, _game, x, y);
		add(_actionMenu[i]);
		_actionMenu[i]->setVisible(false);
		_actionMenu[i]->onMouseClick((ActionHandler)&AirActionMenuState::btnAirActionMenuItemClick);
	}

	// Build up the popup menu
	int id = 0;

	action->action = options;
	switch (options)
	{
		case AA_MOVE:
			addItem(AA_MOVE_RETREAT, nullptr, "STR_AIR_MOVE_RETREAT", &id, SDLK_4);
			addItem(AA_MOVE_PURSUE, nullptr, "STR_AIR_MOVE_PURSUE", &id, SDLK_3);
			addItem(AA_MOVE_BACKWARD, nullptr, "STR_AIR_MOVE_BACKWARD", &id, SDLK_2);
			addItem(AA_MOVE_FORWARD, nullptr, "STR_AIR_MOVE_FORWARD", &id, SDLK_1);
			break;
		case AA_ATTACK:
			if (_action->unit->craft)
			{
				/*int weapon = 0;
				for (auto ii = _action->unit->craft->getWeapons()->rbegin(); ii != _action->unit->craft->getWeapons()->rend(); ++ii)
				{
					if (*ii)
					{
						addItem(AA_FIRE_WEAPON, *ii, (*ii)->getRules()->getType(), &id, (SDLKey)(SDLK_1 + id));
					}
				}*/

				int weaponCount = _action->unit->craft->getNumWeapons();
				auto &weapons = *_action->unit->craft->getWeapons();

				if (weaponCount > 3 && weapons[3])
				{
					addItem(AA_FIRE_WEAPON, weapons[3], weapons[3]->getRules()->getType(), &id, Options::keyAirWeapon4);
				}
				if (weaponCount > 2 && weapons[2])
				{
					addItem(AA_FIRE_WEAPON, weapons[2], weapons[2]->getRules()->getType(), &id, Options::keyAirWeapon3);
				}
				if (weaponCount > 1 && weapons[1])
				{
					addItem(AA_FIRE_WEAPON, weapons[1], weapons[1]->getRules()->getType(), &id, Options::keyAirWeapon2);
				}
				if (weaponCount > 0 && weapons[0])
				{
					addItem(AA_FIRE_WEAPON, weapons[0], weapons[0]->getRules()->getType(), &id, Options::keyAirWeapon1);
				}
			}
			else if (_action->unit->ufo->getRules()->getWeaponPower())
			{
				addItem(AA_FIRE_WEAPON, nullptr, "STR_UFO_ATTACK", &id, (SDLKey)(SDLK_1 + id));
			}
			break;
		case AA_SPECIAL:
			break;
		case AA_WAIT:
			addItem(AA_EVADE, nullptr, "STR_AIR_EVADE", &id, (SDLKey)(SDLK_2));
			addItem(AA_HOLD, nullptr, "STR_AIR_HOLD", &id, (SDLKey)(SDLK_1));
			break;
	}

	if (id == 0)
	{
		switch (options)
		{
			case AA_MOVE:
				break;
			case AA_ATTACK:
				_parent->showWarning(tr("STR_NO_CRAFT_WEAPONS").arg(_action->unit->getDisplayName()));
				break;
			case AA_SPECIAL:
				_parent->showWarning(tr("STR_NO_CRAFT_SPECIALS").arg(_action->unit->getDisplayName()));
				break;
		}
	}

	Element *actionMenu = Game::getGame()->getMod()->getInterface("battlescape")->getElement("actionMenu");

	_selectedItem = new Text(270, 9, x + 25, y);
	add(_selectedItem);
	_selectedItem->setColor(actionMenu->color);
	_selectedItem->setHighContrast(true);
	_selectedItem->setY(y - (id * 25) - 16);
	_selectedItem->setAlign(ALIGN_CENTER);
}

/**
 * Deletes the AirActionMenuState.
 */
AirActionMenuState::~AirActionMenuState()
{

}

/**
 * Init function.
 */
void AirActionMenuState::init()
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
void AirActionMenuState::addItem(AirCombatActionType aa, CraftWeapon *weapon, const std::string &name, int *id, SDLKey key)
{
	std::wstring accuracy;
	std::wstring ammo;
	std::wstring range;
	bool ammoError = false;
	if (weapon)
	{
		// TODO: Include accuracy bonus from craft?
		accuracy = tr("STR_ACCURACY_SHORT").arg(weapon->getRules()->getAccuracy());
		ammo = tr("STR_AMMO_SHORT").arg(weapon->getAmmo());
		range = tr("STR_RANGE_SHORT").arg(weapon->getRules()->getRange());

		ammoError = weapon->getAmmo() == 0;
	}
	else if (aa == AA_FIRE_WEAPON && _action->unit->ufo)
	{
		accuracy = tr("STR_ACCURACY_SHORT").arg(_action->unit->ufoRule->getWeaponAccuracy());
		//ammo = tr("STR_AMMO_SHORT").arg(weapon->getAmmo());
		range = tr("STR_RANGE_SHORT").arg(_action->unit->ufoRule->getWeaponRange());
	}

	_action->action = aa;
	_action->weapon = weapon;
	_action->targeting = aa == AA_FIRE_WEAPON;
	_action->updateCost();

	std::wstring time;
	if (_action->Time)
	{
		time = tr("STR_TIME_SHORT").arg(_action->Time);
	}

	std::wstring fuel;
	if (_action->Energy)
	{
		fuel = tr("STR_FUEL_SHORT").arg(_action->Energy);
	}

	if (!_usedHotkeys.insert(key).second)
	{
		key = SDLK_UNKNOWN;
	}

	std::wstring keyName;
	_actionMenu[*id]->setVisible(true);
	if (key != SDLK_UNKNOWN)
	{
		keyName = Language::utf8ToWstr(SDL_GetKeyName(key)) + L". ";
		std::transform(keyName.begin(), keyName.end(), keyName.begin(), toupper);
		_actionMenu[*id]->onKeyboardPress((ActionHandler)&AirActionMenuState::btnAirActionMenuItemClick, key);
	}
	_actionMenu[*id]->setAction(_action, weapon, keyName + tr(name).c_str(), ammo, accuracy, range, time, fuel, ammoError);
	(*id)++;

	_action->targeting = false;
}

/**
 * Closes the window on right-click.
 * @param action Pointer to an action.
 */
void AirActionMenuState::handle(Action *action)
{
	State::handle(action);
	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN && action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_parent->cancelAction();
		_game->popState();
	}
	else if (action->getDetails()->type == SDL_KEYDOWN &&
		(action->getDetails()->key.keysym.sym == Options::keyCancel))
	{
		_parent->cancelAction();
		_game->popState();
	}
}

/**
 * Executes the action corresponding to this action menu item.
 * @param action Pointer to an action.
 */
void AirActionMenuState::btnAirActionMenuItemClick(Action *action)
{
	int btnID = -1;

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

		(*_action) = _actionMenu[btnID]->getAction();
		_game->popState();
		_parent->executeAction();
	}
}

/**
 * Updates the scale.
 * @param dX delta of X;
 * @param dY delta of Y;
 */
void AirActionMenuState::resize(int &dX, int &dY)
{
	State::recenter(dX, dY * 2);
}

//////// Menu Items //////////

/**
* Sets up an Action menu item.
* @param id The unique identifier of the menu item.
* @param game Pointer to the game.
* @param x Position on the x-axis.
* @param y Position on the y-asis.
*/
AirActionMenuState::AirActionMenuItem::AirActionMenuItem(int id, Game *game, int x, int y) : InteractiveSurface(300, 20, x + 10, y - (id * 25) - 25), _highlighted(false), _action(), _description(L"AirActionMenuItem"), _weapon(nullptr)
{
	Font *big = game->getMod()->getFont("FONT_BIG"), *small = game->getMod()->getFont("FONT_SMALL");
	Language *lang = game->getLanguage();

	Element *actionMenu = game->getMod()->getInterface("battlescape")->getElement("actionMenu");

	_highlightModifier = actionMenu->TFTDMode ? 12 : 3;

	_frame = new Frame(getWidth(), getHeight(), 0, 0);
	_frame->setHighContrast(true);
	_frame->setColor(actionMenu->border);
	_frame->setSecondaryColor(actionMenu->color2);
	_frame->setThickness(3);

	_txtDescription = new Text(200, 9, 5, 6);
	_txtDescription->initText(big, small, lang);
	_txtDescription->setHighContrast(true);
	_txtDescription->setColor(actionMenu->color);

	_txtAmmo = new Text(50, 9, 100, 6);
	_txtAmmo->initText(big, small, lang);
	_txtAmmo->setHighContrast(true);
	_txtAmmo->setColor(actionMenu->color);

	_txtRange = new Text(50, 9, 140, 6);
	_txtRange->initText(big, small, lang);
	_txtRange->setHighContrast(true);
	_txtRange->setColor(actionMenu->color);

	_txtAcc = new Text(50, 9, 180, 6);
	_txtAcc->initText(big, small, lang);
	_txtAcc->setHighContrast(true);
	_txtAcc->setColor(actionMenu->color);

	_txtTU = new Text(50, 9, 220, 6);
	_txtTU->initText(big, small, lang);
	_txtTU->setHighContrast(true);
	_txtTU->setColor(actionMenu->color);

	_txtFuel = new Text(50, 9, 260, 6);
	_txtFuel->initText(big, small, lang);
	_txtFuel->setHighContrast(true);
	_txtFuel->setColor(actionMenu->color);
}

/**
* Deletes the ActionMenuItem.
*/
AirActionMenuState::AirActionMenuItem::~AirActionMenuItem()
{
	delete _frame;
	delete _txtDescription;
	delete _txtAmmo;
	delete _txtRange;
	delete _txtAcc;
	delete _txtTU;
	delete _txtFuel;
}

/**
* Links with an action and fills in the text fields.
* @param action The battlescape action.
* @param description The actions description.
* @param accuracy The actions accuracy, including the Acc> prefix.
* @param timeunits The timeunits string, including the TUs> prefix.
* @param tu The timeunits value.
*/
void AirActionMenuState::AirActionMenuItem::setAction(AirCombatAction *action, CraftWeapon *weapon, const std::wstring &description, const std::wstring &ammo, const std::wstring &accuracy, const std::wstring &range, const std::wstring &time, const std::wstring &fuel, bool ammoError)
{
	_action = *action;
	_weapon = weapon;

	Element *actionMenu = Game::getGame()->getMod()->getInterface("battlescape")->getElement("actionMenu");
	Element *actionMenuWarning = Game::getGame()->getMod()->getInterface("battlescape")->getElement("actionMenuWarning");
	Element *actionMenuError = Game::getGame()->getMod()->getInterface("battlescape")->getElement("actionMenuError");

	std::wostringstream desc;
	desc << description << L"   " << accuracy << L"   " << _action.Time;
	_description = desc.str();
	_txtDescription->setText(description);
	_txtAmmo->setText(ammo);
	_txtRange->setText(range);
	_txtAcc->setText(accuracy);
	_txtTU->setText(time);
	_txtFuel->setText(fuel);

	if (ammoError)
	{
		_frame->setColor(actionMenuError->border);
	}

	_redraw = true;
}

/**
* Replaces a certain amount of colors in the surface's palette.
* @param colors Pointer to the set of colors.
* @param firstcolor Offset of the first color to replace.
* @param ncolors Amount of colors to replace.
*/
void AirActionMenuState::AirActionMenuItem::setPalette(SDL_Color *colors, int firstcolor, int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	_frame->setPalette(colors, firstcolor, ncolors);
	_txtDescription->setPalette(colors, firstcolor, ncolors);
	_txtAmmo->setPalette(colors, firstcolor, ncolors);
	_txtRange->setPalette(colors, firstcolor, ncolors);
	_txtAcc->setPalette(colors, firstcolor, ncolors);
	_txtTU->setPalette(colors, firstcolor, ncolors);
	_txtFuel->setPalette(colors, firstcolor, ncolors);
}

/**
* Draws the bordered box.
*/
void AirActionMenuState::AirActionMenuItem::draw()
{
	_frame->blit(this);
	_txtDescription->blit(this);
	_txtAmmo->blit(this);
	_txtRange->blit(this);
	_txtAcc->blit(this);
	_txtTU->blit(this);
	_txtFuel->blit(this);
}

/**
* Processes a mouse hover in event.
* @param action Pointer to an action.
* @param state Pointer to a state.
*/
void AirActionMenuState::AirActionMenuItem::mouseIn(Action *action, State *state)
{
	_highlighted = true;
	_frame->setSecondaryColor(_frame->getSecondaryColor() - _highlightModifier);
	draw();
	InteractiveSurface::mouseIn(action, state);
}

/**
* Processes a mouse hover out event.
* @param action Pointer to an action.
* @param state Pointer to a state.
*/
void AirActionMenuState::AirActionMenuItem::mouseOut(Action *action, State *state)
{
	_highlighted = false;
	_frame->setSecondaryColor(_frame->getSecondaryColor() + _highlightModifier);
	draw();
	InteractiveSurface::mouseOut(action, state);
}

std::wstring AirActionMenuState::AirActionMenuItem::getDescription() const
{
	return _description;
}

CraftWeapon *AirActionMenuState::AirActionMenuItem::getWeapon() const
{
	return _weapon;
}

/////////// Actions ////////////

/**
* Gets the action that was linked to this menu item.
* @return Action that was linked to this menu item.
*/
const AirCombatAction &AirActionMenuState::AirActionMenuItem::getAction() const
{
	return _action;
}

void AirCombatAction::updateCost()
{
	unit->getActionCost(this);
}

bool AirCombatAction::canSpendCost(std::string &message) const
{
	if (Energy && unit->getCombatFuel() < Energy)
	{
		message = "STR_NOT_ENOUGH_FUEL";
		return false;
	}

	if (Ammo && weapon && weapon->getAmmo() < Ammo)
	{
		message = "STR_NOT_ENOUGH_AMMO";
		return false;
	}

	return true;
}

bool AirCombatAction::spendCost(std::string &message)
{
	if (canSpendCost(message))
	{
		unit->spendCombatFuel(Energy);
		unit->spendTime(Time);
		return true;
	}

	return false;
}

}
