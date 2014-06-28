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
#include "RoleMenuState.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Interface/TextButton.h"
#include "../Interface/Text.h"
#include "../Interface/Cursor.h"
#include "../Engine/Options.h"
#include "InventoryState.h"
#include "../Interface/Window.h"
#include "../Engine/Surface.h"
#include "../Resource/ResourcePack.h"
#include "RoleChangeState.h"
#include "../Savegame/Role.h"

namespace OpenXcom
{

/**
 * Initializes all the elements.
 * @param game Pointer to the core game.
 * @param msg Message string.
 */
RoleMenuState::RoleMenuState(InventoryState* parentState) : _parentState(parentState)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 200, 123, 60, 35, POPUP_BOTH);
	_txtTitle = new Text(190, 20, 65, 45);
	_btnChangeRole = new TextButton(180, 18, 70, 65);
	_btnSaveLayout = new TextButton(180, 18, 70, 85);
	_btnLoadLayout = new TextButton(180, 18, 70, 105);
	_btnClose = new TextButton(90, 18, 115, 130);

	// Set palette
	setPalette("PAL_BATTLESCAPE");

	add(_window);
	add(_btnChangeRole);
	add(_btnSaveLayout);
	add(_btnLoadLayout);
	add(_btnClose);
	add(_txtTitle);

	centerAllSurfaces();

	// Set up objects
	_window->setColor(Palette::blockOffset(7)+8);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));
	_window->setHighContrast(true);

	_txtTitle->setColor(Palette::blockOffset(0)-1);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setVerticalAlign(ALIGN_MIDDLE);
	_txtTitle->setHighContrast(true);
	_txtTitle->setWordWrap(true);

	std::wostringstream roleTitle;

	roleTitle << tr("STR_ROLE") << ": " << tr(_parentState->getSelectedSoldier()->getRole()->getName());

	_txtTitle->setText(roleTitle.str());

	_btnChangeRole->setColor(Palette::blockOffset(0)-1);
	_btnChangeRole->setText(tr("STR_CHANGE_ROLE"));
	_btnChangeRole->setHighContrast(true);
	_btnChangeRole->onMouseClick((ActionHandler)&RoleMenuState::btnChangeRoleClick);

	_btnSaveLayout->setColor(Palette::blockOffset(0)-1);
	_btnSaveLayout->setText(tr("STR_SAVE_ROLE_LAYOUT"));
	_btnSaveLayout->setHighContrast(true);
	_btnSaveLayout->onMouseClick((ActionHandler)&RoleMenuState::btnSaveLayoutClick);

	_btnLoadLayout->setColor(Palette::blockOffset(0)-1);
	_btnLoadLayout->setText(tr("STR_LOAD_ROLE_LAYOUT"));
	_btnLoadLayout->setHighContrast(true);
	_btnLoadLayout->onMouseClick((ActionHandler)&RoleMenuState::btnLoadLayoutClick);

	_btnClose->setColor(Palette::blockOffset(0)-1);
	_btnClose->setText(tr("STR_OK"));
	_btnClose->onMouseClick((ActionHandler)&RoleMenuState::btnCloseClick);
	_btnClose->onKeyboardPress((ActionHandler)&RoleMenuState::btnCloseClick, Options::keyOk);
	_btnClose->onKeyboardPress((ActionHandler)&RoleMenuState::btnCloseClick, Options::keyCancel);
	_btnClose->setHighContrast(true);

	_game->getCursor()->setVisible(true);

	updateDisplay();
}

/**
 *
 */
RoleMenuState::~RoleMenuState()
{

}

/**
 * Opens a sub-menu to change the unit's role.
 * @param action Pointer to an action.
 */
void RoleMenuState::btnChangeRoleClick(Action *action)
{
	_game->pushState(new RoleChangeState(this));
}

/**
 * Overwrites the current role's default equipment layout with the current unit's layout.
 * @param action Pointer to an action.
 */
void RoleMenuState::btnSaveLayoutClick(Action *action)
{
	std::vector<EquipmentLayoutItem*> layout;
	_parentState->getUnitEquipmentLayout(&layout);
	_parentState->getSelectedSoldier()->getRole()->setDefaultLayout(layout);
	_game->popState();
}

/**
 * Loads the role's default layout into the unit's inventory.
 * @param action Pointer to an action.
 */
void RoleMenuState::btnLoadLayoutClick(Action *action)
{
	_parentState->applyEquipmentLayout(_parentState->getSelectedSoldier()->getRole()->getDefaultLayout());
	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void RoleMenuState::btnCloseClick(Action *)
{
	_game->popState();
}

void RoleMenuState::updateDisplay()
{
	Soldier *s = _parentState->getSelectedSoldier();
	_role = s ? s->getRole() : 0;

	_btnSaveLayout->setVisible(_role && !_role->isBlank());
	_btnLoadLayout->setVisible(_role && !_role->isBlank());
}

/// Change the selected unit's role.
void RoleMenuState::changeRole(const std::string &role)
{
	_parentState->setRole(role);
	_game->popState();
}
}
