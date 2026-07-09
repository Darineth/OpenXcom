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
class Role;
class Mod;

/**
 * DX: Role armor-colour picker popup. Lists a curated set of battlescape-palette
 * colour blocks; the chosen block base becomes the role's armor accent colour
 * (0 = no recolour). Opened from the role management screen.
 */
class RoleColorSelectState : public State
{
private:
	Role *_role;
	Window *_window;
	Text *_txtTitle;
	TextButton *_btnCancel;
	TextList *_lstColors;
	std::vector<int> _colorValues; // parallel to list rows; 0 = "no colour"
public:
	/// Creates the Role Color Select state for the given role.
	RoleColorSelectState(Role *role);
	/// Cleans up the Role Color Select state.
	~RoleColorSelectState();
	/// Gets the display name of a mod-defined armor colour value (STR key; empty if unknown).
	static std::string getColorName(const Mod *mod, int color);
	/// Handler for clicking a colour in the list.
	void lstColorsClick(Action *action);
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action *action);
};

}
