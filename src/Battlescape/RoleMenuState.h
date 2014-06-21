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
#ifndef OPENXCOM_ROLEMENUSTATE_H
#define OPENXCOM_ROLEMENUSTATE_H

#include "../Engine/State.h"
#include <string>

namespace OpenXcom
{

class TextButton;
class Window;
class Text;
class InventoryState;

/**
 * Notifies the player about things like soldiers going unconscious or dying from wounds.
 */
class RoleMenuState : public State
{
private:
	InventoryState *_parentState;
	TextButton *_btnChangeRole, *_btnSaveLayout, *_btnLoadLayout, *_btnCancel;
	Window *_window;
	Text *_txtTitle;
public:
	/// Creates the RoleMenuState.
	RoleMenuState(Game *game, InventoryState *parentState);
	/// Cleans up the RoleMenuState.
	~RoleMenuState();
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action *action);
	/// Handler for clicking the Change Role button.
	void btnChangeRoleClick(Action *action);
	/// Handler for clicking the Save Layout button.
	void btnSaveLayoutClick(Action *action);
	/// Handler for clicking the Load Layout button.
	void btnLoadLayoutClick(Action *action);
};

}

#endif
