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
#ifndef OPENXCOM_ROLECHANGESTATE_H
#define OPENXCOM_ROLECHANGESTATE_H

#include "../Engine/State.h"
#include <string>

namespace OpenXcom
{
	class Window;
	class TextButton;
	class TextList;
	class Text;
	class RoleMenuState;
	class Role;

	/**
	 * Notifies the player about things like soldiers going unconscious or dying from wounds.
	 */
	class RoleChangeState : public State
	{
	private:
		RoleMenuState *_parentState;
		Window *_window;
		TextList *_rolesList;
		TextButton *_btnCancel;

	public:
		/// Creates the RoleChangeState.
		RoleChangeState(Game *game, RoleMenuState *parentState);
		/// Cleans up the RoleChangeState.
		~RoleChangeState();
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action *action);
		/// Handler for clicking the roles list.
		void rolesListClick(Action *action);
	};
}

#endif
