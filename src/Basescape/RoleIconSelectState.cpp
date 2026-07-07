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
#include "RoleIconSelectState.h"
#include <cctype>
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/Action.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Mod/Mod.h"
#include "../Savegame/Role.h"

namespace OpenXcom
{

/**
 * Turns a roleIcons registry id (e.g. "MACHINE_GUNNER") into a friendlier label
 * ("Machine Gunner") for display.
 */
std::string RoleIconSelectState::prettifyIconName(const std::string& name)
{
	std::string out;
	bool startWord = true;
	for (char c : name)
	{
		if (c == '_')
		{
			out += ' ';
			startWord = true;
		}
		else if (startWord)
		{
			out += c;
			startWord = false;
		}
		else
		{
			out += (char)tolower((unsigned char)c);
		}
	}
	return out;
}

/**
 * Initializes all the elements in the Role Icon Select window.
 * @param role The role whose icon is being set.
 */
RoleIconSelectState::RoleIconSelectState(Role *role) : _role(role)
{
	_screen = false;

	_window = new Window(this, 192, 182, 64, 9, POPUP_BOTH);
	_txtTitle = new Text(182, 9, 69, 18);
	_lstIcons = new TextList(160, 120, 76, 32);
	_btnCancel = new TextButton(160, 16, 80, 158);

	setInterface("roleIconSelect");

	add(_window, "window", "roleIconSelect");
	add(_txtTitle, "text", "roleIconSelect");
	add(_lstIcons, "list", "roleIconSelect");
	add(_btnCancel, "button", "roleIconSelect");

	centerAllSurfaces();

	setWindowBackground(_window, "roleIconSelect");

	_txtTitle->setText(tr("STR_SELECT_ICON"));
	_txtTitle->setAlign(ALIGN_CENTER);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&RoleIconSelectState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&RoleIconSelectState::btnCancelClick, Options::keyCancel);

	_lstIcons->setColumns(1, 152);
	_lstIcons->setSelectable(true);
	_lstIcons->setBackground(_window);
	_lstIcons->setMargin(8);
	_lstIcons->onMouseClick((ActionHandler)&RoleIconSelectState::lstIconsClick);

	for (const auto& iconName : _game->getMod()->getRoleIconsList())
	{
		_lstIcons->addRow(1, prettifyIconName(iconName).c_str());
		_iconNames.push_back(iconName);
	}
}

/**
 *
 */
RoleIconSelectState::~RoleIconSelectState()
{
}

/**
 * Assigns the clicked icon to the role and closes.
 * @param action Pointer to an action.
 */
void RoleIconSelectState::lstIconsClick(Action *)
{
	size_t row = _lstIcons->getSelectedRow();
	if (row < _iconNames.size())
	{
		_role->setIcon(_iconNames[row]);
	}
	_game->popState();
}

/**
 * Returns without changing the role's icon.
 * @param action Pointer to an action.
 */
void RoleIconSelectState::btnCancelClick(Action *)
{
	_game->popState();
}

}
