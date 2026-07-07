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
#include "RoleSelectState.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/Action.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Role.h"
#include "../Savegame/Soldier.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Role Select window.
 * @param soldier The soldier whose role is being set.
 */
RoleSelectState::RoleSelectState(Soldier *soldier) : _soldier(soldier)
{
	_screen = false;

	_window = new Window(this, 192, 160, 64, 20, POPUP_BOTH);
	_txtTitle = new Text(182, 9, 69, 30);
	_lstRoles = new TextList(160, 89, 76, 42);
	_btnCancel = new TextButton(160, 16, 80, 156);

	setInterface("roleSelect");

	add(_window, "window", "roleSelect");
	add(_txtTitle, "text", "roleSelect");
	add(_lstRoles, "list", "roleSelect");
	add(_btnCancel, "button", "roleSelect");

	centerAllSurfaces();

	setWindowBackground(_window, "roleSelect");

	_txtTitle->setText(tr("STR_SELECT_ROLE"));
	_txtTitle->setAlign(ALIGN_CENTER);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&RoleSelectState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&RoleSelectState::btnCancelClick, Options::keyCancel);

	_lstRoles->setColumns(1, 152);
	_lstRoles->setSelectable(true);
	_lstRoles->setBackground(_window);
	_lstRoles->setMargin(8);
	_lstRoles->onMouseClick((ActionHandler)&RoleSelectState::lstRolesClick);

	// "No role" entry first, then every player role.
	_lstRoles->addRow(1, tr("STR_NO_ROLE").c_str());
	_roleIds.push_back(0);
	for (auto* role : _game->getSavedGame()->getRoles())
	{
		_lstRoles->addRow(1, tr(role->getName()).c_str());
		_roleIds.push_back(role->getId());
	}
}

/**
 *
 */
RoleSelectState::~RoleSelectState()
{
}

/**
 * Assigns the clicked role to the soldier and closes.
 * @param action Pointer to an action.
 */
void RoleSelectState::lstRolesClick(Action *)
{
	size_t row = _lstRoles->getSelectedRow();
	if (row < _roleIds.size())
	{
		_soldier->setRoleId(_roleIds[row]);
	}
	_game->popState();
}

/**
 * Returns without changing the soldier's role.
 * @param action Pointer to an action.
 */
void RoleSelectState::btnCancelClick(Action *)
{
	_game->popState();
}

}
