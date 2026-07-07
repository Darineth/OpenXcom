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
#include <string>

namespace OpenXcom
{

class Window;
class Text;
class TextButton;
class TextList;
class Role;

/**
 * DX: Role-icon picker popup. Lists the roleIcons registry entries and assigns the
 * chosen one to a role. Opened from the role management screen.
 */
class RoleIconSelectState : public State
{
private:
	Role *_role;
	Window *_window;
	Text *_txtTitle;
	TextButton *_btnCancel;
	TextList *_lstIcons;
	std::vector<std::string> _iconNames; // parallel to the list rows
public:
	/// Turns a roleIcons registry id (e.g. "MACHINE_GUNNER") into a friendly label ("Machine Gunner").
	static std::string prettifyIconName(const std::string& name);
	/// Creates the Role Icon Select state for the given role.
	RoleIconSelectState(Role *role);
	/// Cleans up the Role Icon Select state.
	~RoleIconSelectState();
	/// Handler for clicking an icon in the list.
	void lstIconsClick(Action *action);
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action *action);
};

}
