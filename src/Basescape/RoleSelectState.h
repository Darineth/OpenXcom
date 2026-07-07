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
#include "../Engine/State.h"
#include <vector>

namespace OpenXcom
{

class Window;
class Text;
class TextButton;
class TextList;
class Soldier;
class InventoryState;

/**
 * DX: Role picker popup. Lists the save's player-authored roles (plus a "no
 * role" entry) and assigns the chosen one to a soldier. Opened from the
 * Soldier Info screen; sets the soldier's role id and returns.
 */
class RoleSelectState : public State
{
private:
	Soldier *_soldier;
	InventoryState *_inv; // set when opened from the inventory; enables role-loadout apply/save
	Window *_window;
	Text *_txtTitle;
	TextButton *_btnApply, *_btnSave, *_btnManage, *_btnCancel;
	TextList *_lstRoles;
	std::vector<int> _roleIds; // parallel to list rows; 0 for the "no role" row
public:
	/// Creates the Role Select state for the given soldier (inv enables role-loadout apply/save).
	RoleSelectState(Soldier *soldier, InventoryState *inv = nullptr);
	/// Cleans up the Role Select state.
	~RoleSelectState();
	/// Rebuilds the role list (refreshes after returning from role management).
	void init() override;
	/// Handler for clicking a role in the list.
	void lstRolesClick(Action *action);
	/// Handler for clicking the Apply Loadout button.
	void btnApplyClick(Action *action);
	/// Handler for clicking the Save Loadout button.
	void btnSaveClick(Action *action);
	/// Handler for clicking the Manage button.
	void btnManageClick(Action *action);
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action *action);
};

}
