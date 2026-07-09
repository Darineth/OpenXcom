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

namespace OpenXcom
{

class Window;
class Text;
class TextEdit;
class TextButton;
class TextList;
class InteractiveSurface;

/**
 * DX: Role management screen. Lists the save's player-authored roles and lets the
 * player create, rename, re-icon, and delete them. Reached from the role picker.
 * (Colour and loadout editing are follow-ons.)
 */
class RoleMenuState : public State
{
private:
	Window *_window;
	Text *_txtTitle;
	TextList *_lstRoles;
	TextEdit *_edtName;
	TextEdit *_edtShort;
	InteractiveSurface *_role; // clickable role badge (opens the icon picker)
	TextButton *_btnNew, *_btnDefault, *_btnColor, *_btnDelete, *_btnOk;
	int _sel; // index into SavedGame::getRoles(), or -1 for none selected
	bool _autoShort; // while true, the short name auto-derives from the role name
	/// Rebuilds the role list from the savegame.
	void populateList();
	/// Refreshes the detail panel (name field + badge) for the selected role.
	void updateDetail();
public:
	/// Creates the Role Menu state.
	RoleMenuState();
	/// Cleans up the Role Menu state.
	~RoleMenuState();
	/// Refreshes the screen (e.g. after returning from the icon picker).
	void init() override;
	/// Handler for clicking a role in the list.
	void lstRolesClick(Action *action);
	/// Handler for editing the selected role's name.
	void edtNameChange(Action *action);
	/// Handler for editing the selected role's short (abbreviation) name.
	void edtShortChange(Action *action);
	/// Handler for clicking the New button.
	void btnNewClick(Action *action);
	/// Handler for clicking the Load Defaults button (re-adds missing seed roles).
	void btnDefaultClick(Action *action);
	/// Handler for clicking the Change Icon button.
	void btnIconClick(Action *action);
	/// Handler for clicking the Color button.
	void btnColorClick(Action *action);
	/// Handler for clicking the Delete button.
	void btnDeleteClick(Action *action);
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
};

}
