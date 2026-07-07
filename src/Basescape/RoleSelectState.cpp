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
#include "RoleMenuState.h"
#include "../Battlescape/InventoryState.h"
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
RoleSelectState::RoleSelectState(Soldier *soldier, InventoryState *inv) : _soldier(soldier), _inv(inv)
{
	_screen = false;

	_window = new Window(this, 192, 190, 64, 5, POPUP_BOTH);
	_txtTitle = new Text(182, 9, 69, 14);
	_lstRoles = new TextList(160, 112, 76, 26);
	_btnApply = new TextButton(76, 16, 80, 146);
	_btnSave = new TextButton(76, 16, 160, 146);
	_btnManage = new TextButton(76, 16, 80, 164);
	_btnCancel = new TextButton(76, 16, 160, 164);

	setInterface("roleSelect");

	add(_window, "window", "roleSelect");
	add(_txtTitle, "text", "roleSelect");
	add(_lstRoles, "list", "roleSelect");
	add(_btnApply, "button", "roleSelect");
	add(_btnSave, "button", "roleSelect");
	add(_btnManage, "button", "roleSelect");
	add(_btnCancel, "button", "roleSelect");

	centerAllSurfaces();

	setWindowBackground(_window, "roleSelect");

	_txtTitle->setText(tr("STR_SELECT_ROLE"));
	_txtTitle->setAlign(ALIGN_CENTER);

	_btnApply->setText(tr("STR_APPLY_LOADOUT"));
	_btnApply->onMouseClick((ActionHandler)&RoleSelectState::btnApplyClick);

	_btnSave->setText(tr("STR_SAVE_LOADOUT"));
	_btnSave->onMouseClick((ActionHandler)&RoleSelectState::btnSaveClick);

	_btnManage->setText(tr("STR_MANAGE"));
	_btnManage->onMouseClick((ActionHandler)&RoleSelectState::btnManageClick);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&RoleSelectState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&RoleSelectState::btnCancelClick, Options::keyCancel);

	_lstRoles->setColumns(1, 152);
	_lstRoles->setSelectable(true);
	_lstRoles->setBackground(_window);
	_lstRoles->setMargin(8);
	_lstRoles->onMouseClick((ActionHandler)&RoleSelectState::lstRolesClick);

	// Role-loadout buttons only apply when opened from the inventory and the unit has a role.
	Role *role = _inv ? _inv->getSelectedUnitRole() : nullptr;
	_btnSave->setVisible(role != nullptr);
	_btnApply->setVisible(role != nullptr && !role->getLoadout().empty());
}

/**
 *
 */
RoleSelectState::~RoleSelectState()
{
}

/**
 * (Re)builds the role list - so it refreshes after the management screen changes roles.
 */
void RoleSelectState::init()
{
	State::init();
	_lstRoles->clearList();
	_roleIds.clear();
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
 * Applies the unit's assigned role's loadout to it and closes.
 * @param action Pointer to an action.
 */
void RoleSelectState::btnApplyClick(Action *)
{
	if (_inv)
	{
		_inv->applyRoleLoadout();
	}
	_game->popState();
}

/**
 * Saves the unit's current loadout onto its assigned role and closes.
 * @param action Pointer to an action.
 */
void RoleSelectState::btnSaveClick(Action *)
{
	if (_inv)
	{
		_inv->saveRoleLoadout();
	}
	_game->popState();
}

/**
 * Opens the role management screen. init() rebuilds this list when it returns.
 * @param action Pointer to an action.
 */
void RoleSelectState::btnManageClick(Action *)
{
	_game->pushState(new RoleMenuState());
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
