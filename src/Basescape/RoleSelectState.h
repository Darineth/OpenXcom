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

/**
 * DX: Role picker popup. Lists the save's player-authored roles (plus a "no
 * role" entry) and assigns the chosen one to a soldier. Opened from the
 * Soldier Info screen; sets the soldier's role id and returns.
 */
class RoleSelectState : public State
{
private:
	Soldier *_soldier;
	Window *_window;
	Text *_txtTitle;
	TextButton *_btnCancel;
	TextList *_lstRoles;
	std::vector<int> _roleIds; // parallel to list rows; -1 for the "no role" row
public:
	/// Creates the Role Select state for the given soldier.
	RoleSelectState(Soldier *soldier);
	/// Cleans up the Role Select state.
	~RoleSelectState();
	/// Handler for clicking a role in the list.
	void lstRolesClick(Action *action);
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action *action);
};

}
