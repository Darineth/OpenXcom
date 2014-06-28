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
#include "RoleChangeState.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Interface/TextButton.h"
#include "../Interface/Text.h"
#include "../Interface/Cursor.h"
#include "../Engine/Options.h"
#include "InventoryState.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"
#include "../Engine/Surface.h"
#include "../Resource/ResourcePack.h"
#include "../Ruleset/Ruleset.h"
#include "../Savegame/Role.h"
#include "RoleMenuState.h"
#include <vector>

namespace OpenXcom
{
/**
 * Initializes all the elements.
 * @param game Pointer to the core game.
 * @param msg Message string.
 */
RoleChangeState::RoleChangeState(RoleMenuState* parentState) : _parentState(parentState)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 150, 180, 85, 10, POPUP_BOTH);
	_rolesList = new TextList(140, 148, 90, 17);
	_btnCancel = new TextButton(136, 18, 92, 165);

	// Set palette
	setPalette("PAL_BATTLESCAPE");

	add(_window);
	add(_rolesList);
	add(_btnCancel);

	centerAllSurfaces();

	// Set up objects
	_window->setColor(Palette::blockOffset(8)+5);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));
	_window->setHighContrast(true);

	_rolesList->setColor(Palette::blockOffset(0));
	_rolesList->setArrowColor(Palette::blockOffset(15)-1);
	_rolesList->setColumns(1, 120);
	_rolesList->setSelectable(true);
	_rolesList->setBackground(_window);
	_rolesList->setMargin(8);
	_rolesList->setAlign(ALIGN_CENTER);
	_rolesList->setHighContrast(true);
	_rolesList->onMouseClick((ActionHandler)&RoleChangeState::rolesListClick);

	std::vector<std::string> roles = _game->getRuleset()->getRolesList();
	for(std::vector<std::string>::const_iterator ii = roles.begin(); ii != roles.end(); ++ii)
	{
		_rolesList->addRow(1, tr(*ii).c_str());
	}

	_btnCancel->setColor(Palette::blockOffset(0)-1);
	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&RoleChangeState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&RoleChangeState::btnCancelClick, Options::keyOk);
	_btnCancel->onKeyboardPress((ActionHandler)&RoleChangeState::btnCancelClick, Options::keyCancel);
	_btnCancel->setHighContrast(true);

	_game->getCursor()->setVisible(true);
}

/**
 *
 */
RoleChangeState::~RoleChangeState()
{

}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void RoleChangeState::btnCancelClick(Action *action)
{
	_game->popState();
}

void RoleChangeState::rolesListClick(Action *action)
{
	_parentState->changeRole(_game->getRuleset()->getRolesList()[_rolesList->getSelectedRow()]);
	_game->popState();
}
}
