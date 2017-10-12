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
#include "NewPossiblePurchaseState.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Mod/Mod.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Mod/RuleItem.h"
#include "../Basescape/PurchaseState.h"
#include "../Engine/Options.h"

namespace OpenXcom
{
/**
 * Initializes all the elements in the EndManufacture screen.
 * @param base Pointer to the base to get info from.
 * @param possibilities List of newly possible items to buy
 */
NewPossiblePurchaseState::NewPossiblePurchaseState(Base * base, const std::vector<RuleItem *> & possibilities) : _base(base)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 288, 180, 16, 10);
	_btnOk = new TextButton(160, 14, 80, 149);
	_btnPurchase = new TextButton(160, 14, 80, 165);
	_txtTitle = new Text(288, 40, 16, 20);
	_lstPossibilities = new TextList(250, 80, 35, 50);

	// Set palette
	setInterface("geoNewItem");

	add(_window, "window", "geoNewItem");
	add(_btnOk, "button", "geoNewItem");
	add(_btnPurchase, "button", "geoNewItem");
	add(_txtTitle, "text1", "geoNewItem");
	add(_lstPossibilities, "text2", "geoNewItem");

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK13.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&NewPossiblePurchaseState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&NewPossiblePurchaseState::btnOkClick, Options::keyCancel);
	_btnPurchase->setText(tr("STR_PURCHASE_HIRE_PERSONNEL"));
	_btnPurchase->onMouseClick((ActionHandler)&NewPossiblePurchaseState::btnPurchaseClick);
	_btnPurchase->onKeyboardPress((ActionHandler)&NewPossiblePurchaseState::btnPurchaseClick, Options::keyOk);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_WE_CAN_NOW_PURCHASE"));

	_lstPossibilities->setColumns(1, 250);
	_lstPossibilities->setBig();
	_lstPossibilities->setAlign(ALIGN_CENTER);
	_lstPossibilities->setScrolling(true, 0);
	for (std::vector<RuleItem *>::const_iterator iter = possibilities.begin(); iter != possibilities.end(); ++iter)
	{
		_lstPossibilities->addRow (1, tr((*iter)->getName()).c_str());
	}
}

/**
 * Closes the screeen.
 * @param action Pointer to an action.
 */
void NewPossiblePurchaseState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Opens the PurchaseState.
 * @param action Pointer to an action.
 */
void NewPossiblePurchaseState::btnPurchaseClick(Action *)
{
	_game->popState();
	_game->pushState(new PurchaseState(_base));
}

}
