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
#include "RoleColorSelectState.h"
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
 * Returns the STR key naming a colour value from the mod's soldierArmorBaseColors
 * config, or empty if unknown. -1 (no recolour) is handled by the caller.
 */
std::string RoleColorSelectState::getColorName(const Mod *mod, int color)
{
	for (const auto &c : mod->getSoldierArmorBaseColors())
	{
		if (c.second == color)
			return c.first;
	}
	return std::string();
}

/**
 * Initializes all the elements in the Role Color Select window.
 * @param role The role whose armor colour is being set.
 */
RoleColorSelectState::RoleColorSelectState(Role *role) : _role(role)
{
	_screen = false;

	_window = new Window(this, 192, 182, 64, 9, POPUP_BOTH);
	_txtTitle = new Text(182, 9, 69, 18);
	_lstColors = new TextList(160, 120, 76, 32);
	_btnCancel = new TextButton(160, 16, 80, 158);

	setInterface("roleColorSelect");

	add(_window, "window", "roleColorSelect");
	add(_txtTitle, "text", "roleColorSelect");
	add(_lstColors, "list", "roleColorSelect");
	add(_btnCancel, "button", "roleColorSelect");

	centerAllSurfaces();

	setWindowBackground(_window, "roleColorSelect");

	_txtTitle->setText(tr("STR_SELECT_COLOR"));
	_txtTitle->setAlign(ALIGN_CENTER);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&RoleColorSelectState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&RoleColorSelectState::btnCancelClick, Options::keyCancel);

	_lstColors->setColumns(1, 152);
	_lstColors->setSelectable(true);
	_lstColors->setBackground(_window);
	_lstColors->setMargin(8);
	_lstColors->onMouseClick((ActionHandler)&RoleColorSelectState::lstColorsClick);

	// "No recolour" first, then the mod-defined colour set (legacy soldierArmorBaseColors:
	// palette-dependent values, so they live in the ruleset, not in code).
	_lstColors->addRow(1, tr("STR_COLOR_NONE").c_str());
	_colorValues.push_back(-1);
	for (const auto &c : _game->getMod()->getSoldierArmorBaseColors())
	{
		_lstColors->addRow(1, tr(c.first).c_str());
		_colorValues.push_back(c.second);
	}
}

/**
 *
 */
RoleColorSelectState::~RoleColorSelectState()
{
}

/**
 * Assigns the clicked colour to the role and closes.
 * @param action Pointer to an action.
 */
void RoleColorSelectState::lstColorsClick(Action *)
{
	size_t row = _lstColors->getSelectedRow();
	if (row < _colorValues.size())
	{
		_role->setColor(_colorValues[row]);
	}
	_game->popState();
}

/**
 * Returns without changing the role's colour.
 * @param action Pointer to an action.
 */
void RoleColorSelectState::btnCancelClick(Action *)
{
	_game->popState();
}

}
