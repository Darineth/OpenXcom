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
#include "RoleMenuState.h"
#include "RoleIconSelectState.h"
#include "RoleColorSelectState.h"
#include <set>
#include <cctype>
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/Action.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Surface.h"
#include "../Engine/InteractiveSurface.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleRole.h"
#include "../Mod/RuleRoleIcon.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Role.h"

namespace OpenXcom
{

/**
 * Derives a short abbreviation from a role name: the first 3 alphanumeric characters, uppercased.
 */
static std::string deriveShort(const std::string& name)
{
	std::string out;
	for (char c : name)
	{
		if (out.size() >= 3)
			break;
		if (std::isalnum((unsigned char)c))
			out += (char)std::toupper((unsigned char)c);
	}
	return out;
}

/**
 * Initializes all the elements in the Role Menu screen.
 */
RoleMenuState::RoleMenuState() : _sel(-1), _autoShort(true)
{
	_window = new Window(this, 320, 200, 0, 0);
	_txtTitle = new Text(300, 17, 10, 9);
	_lstRoles = new TextList(128, 120, 16, 34);
	// Detail: clickable icon on the left, big role name + mini abbreviation stacked to its right.
	_role = new InteractiveSurface(23, 23, 168, 40);
	_edtName = new TextEdit(this, 120, 17, 196, 39);
	_edtShort = new TextEdit(this, 60, 9, 196, 55);
	_btnColor = new TextButton(140, 16, 168, 76);
	_btnDelete = new TextButton(140, 16, 168, 96);
	_btnNew = new TextButton(100, 16, 16, 176);
	_btnDefault = new TextButton(100, 16, 120, 176);
	_btnOk = new TextButton(80, 16, 224, 176);

	setInterface("roleMenu");

	add(_window, "window", "roleMenu");
	add(_txtTitle, "text", "roleMenu");
	add(_lstRoles, "list", "roleMenu");
	add(_role);
	add(_edtName, "text", "roleMenu");
	add(_edtShort, "text", "roleMenu");
	add(_btnColor, "button", "roleMenu");
	add(_btnDelete, "button", "roleMenu");
	add(_btnNew, "button", "roleMenu");
	add(_btnDefault, "button", "roleMenu");
	add(_btnOk, "button", "roleMenu");

	centerAllSurfaces();

	setWindowBackground(_window, "roleMenu");

	_txtTitle->setText(tr("STR_MANAGE_ROLES"));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_lstRoles->setColumns(1, 120);
	_lstRoles->setSelectable(true);
	_lstRoles->setBackground(_window);
	_lstRoles->setMargin(8);
	_lstRoles->onMouseClick((ActionHandler)&RoleMenuState::lstRolesClick);

	// Clicking the badge opens the icon picker for the selected role.
	_role->onMouseClick((ActionHandler)&RoleMenuState::btnIconClick);

	_edtName->setBig();
	_edtName->onChange((ActionHandler)&RoleMenuState::edtNameChange);

	_edtShort->onChange((ActionHandler)&RoleMenuState::edtShortChange);

	_btnNew->setText(tr("STR_NEW_ROLE"));
	_btnNew->onMouseClick((ActionHandler)&RoleMenuState::btnNewClick);

	_btnDefault->setText(tr("STR_LOAD_DEFAULTS"));
	_btnDefault->onMouseClick((ActionHandler)&RoleMenuState::btnDefaultClick);

	_btnColor->onMouseClick((ActionHandler)&RoleMenuState::btnColorClick);

	_btnDelete->setText(tr("STR_DELETE"));
	_btnDelete->onMouseClick((ActionHandler)&RoleMenuState::btnDeleteClick);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&RoleMenuState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&RoleMenuState::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&RoleMenuState::btnOkClick, Options::keyOk);

	if (!_game->getSavedGame()->getRoles().empty())
	{
		_sel = 0;
	}
}

/**
 *
 */
RoleMenuState::~RoleMenuState()
{
}

/**
 * Refreshes the list and detail panel (e.g. after the icon picker returns).
 */
void RoleMenuState::init()
{
	State::init();
	populateList();
	updateDetail();
}

/**
 * Fills the role list from the savegame, keeping the current selection valid.
 */
void RoleMenuState::populateList()
{
	auto& roles = _game->getSavedGame()->getRoles();
	if (_sel >= (int)roles.size())
	{
		_sel = (int)roles.size() - 1;
	}
	_lstRoles->clearList();
	for (auto* role : roles)
	{
		_lstRoles->addRow(1, tr(role->getName()).c_str());
	}
}

/**
 * Updates the name field and badge for the selected role, enabling/disabling the
 * per-role controls when nothing is selected.
 */
void RoleMenuState::updateDetail()
{
	auto& roles = _game->getSavedGame()->getRoles();
	_role->clear();
	bool hasSel = _sel >= 0 && _sel < (int)roles.size();
	_role->setVisible(hasSel);
	_edtName->setVisible(hasSel);
	_edtShort->setVisible(hasSel);
	_btnColor->setVisible(hasSel);
	_btnDelete->setVisible(hasSel);
	if (!hasSel)
	{
		_edtName->setText("");
		_edtShort->setText("");
		return;
	}
	Role* role = roles[_sel];
	// Colour button shows the current armor colour choice.
	std::string colorName = RoleColorSelectState::getColorName(_game->getMod(), role->getColor());
	_btnColor->setText(tr("STR_ROLE_COLOR_BUTTON").arg(tr(colorName.empty() ? "STR_COLOR_NONE" : colorName)));
	_edtName->setText(tr(role->getName()));
	// Show the authored short name, or the auto-derived one; auto-derive stays on until the
	// player sets a short name explicitly.
	_autoShort = role->getShortName().empty();
	_edtShort->setText(_autoShort ? deriveShort(tr(role->getName())) : role->getShortName());

	const RuleRoleIcon* roleIcon = _game->getMod()->getRoleIcon(role->getIcon().empty() ? "NONE" : role->getIcon(), false);
	if (roleIcon && !roleIcon->getSprite().empty())
	{
		Surface* badge = _game->getMod()->getSurface(roleIcon->getSprite(), false);
		if (badge)
		{
			badge->blitNShade(_role, 0, 0);
		}
	}
}

/**
 * Selects the clicked role.
 * @param action Pointer to an action.
 */
void RoleMenuState::lstRolesClick(Action *)
{
	_sel = (int)_lstRoles->getSelectedRow();
	updateDetail();
}

/**
 * Renames the selected role live.
 * @param action Pointer to an action.
 */
void RoleMenuState::edtNameChange(Action *)
{
	auto& roles = _game->getSavedGame()->getRoles();
	if (_sel >= 0 && _sel < (int)roles.size())
	{
		roles[_sel]->setName(_edtName->getText());
		_lstRoles->setCellText(_sel, 0, _edtName->getText());
		// Auto-fill the short name from the name until the player customizes it. Leaving the
		// role's stored shortName empty keeps it deriving (the lists derive the same way).
		if (_autoShort)
		{
			_edtShort->setText(deriveShort(_edtName->getText()));
		}
	}
}

/**
 * Sets the selected role's short (abbreviation) name; clearing it re-enables auto-derive.
 * @param action Pointer to an action.
 */
void RoleMenuState::edtShortChange(Action *)
{
	auto& roles = _game->getSavedGame()->getRoles();
	if (_sel >= 0 && _sel < (int)roles.size())
	{
		roles[_sel]->setShortName(_edtShort->getText());
		_autoShort = _edtShort->getText().empty();
	}
}

/**
 * Creates a new blank role and selects it.
 * @param action Pointer to an action.
 */
void RoleMenuState::btnNewClick(Action *)
{
	Role* role = _game->getSavedGame()->createRole();
	role->setName(tr("STR_NEW_ROLE"));
	role->setIcon("NONE");
	_sel = (int)_game->getSavedGame()->getRoles().size() - 1;
	populateList();
	updateDetail();
}

/**
 * Re-adds any mod-supplied default (seed) roles the save is missing, matched by name.
 * A renamed or deleted default counts as missing and is re-added fresh.
 * @param action Pointer to an action.
 */
void RoleMenuState::btnDefaultClick(Action *)
{
	Mod* mod = _game->getMod();
	SavedGame* save = _game->getSavedGame();
	std::set<std::string> existing;
	for (auto* role : save->getRoles())
	{
		existing.insert(role->getName());
	}
	for (const auto& seedName : mod->getRolesList())
	{
		if (existing.find(seedName) == existing.end())
		{
			RuleRole* seed = mod->getRole(seedName);
			if (seed)
			{
				save->createRole(seed);
			}
		}
	}
	populateList();
	updateDetail();
}

/**
 * Opens the icon picker for the selected role.
 * @param action Pointer to an action.
 */
void RoleMenuState::btnIconClick(Action *)
{
	auto& roles = _game->getSavedGame()->getRoles();
	if (_sel >= 0 && _sel < (int)roles.size())
	{
		_game->pushState(new RoleIconSelectState(roles[_sel]));
	}
}

/**
 * Opens the armor-colour picker for the selected role.
 * @param action Pointer to an action.
 */
void RoleMenuState::btnColorClick(Action *)
{
	auto& roles = _game->getSavedGame()->getRoles();
	if (_sel >= 0 && _sel < (int)roles.size())
	{
		_game->pushState(new RoleColorSelectState(roles[_sel]));
	}
}

/**
 * Deletes the selected role (clearing it from any soldier that had it).
 * @param action Pointer to an action.
 */
void RoleMenuState::btnDeleteClick(Action *)
{
	auto& roles = _game->getSavedGame()->getRoles();
	if (_sel >= 0 && _sel < (int)roles.size())
	{
		_game->getSavedGame()->removeRole(roles[_sel]->getId());
		populateList();
		updateDetail();
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void RoleMenuState::btnOkClick(Action *)
{
	_game->popState();
}

}
