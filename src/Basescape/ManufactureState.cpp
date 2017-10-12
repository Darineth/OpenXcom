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
#include "ManufactureState.h"
#include <sstream>
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/Language.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"
#include "../Mod/RuleManufacture.h"
#include "../Savegame/Production.h"
#include "NewManufactureListState.h"
#include "ManufactureInfoState.h"
#include "TechTreeViewerState.h"
#include <algorithm>
#include "../Engine/Action.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Manufacture screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
ManufactureState::ManufactureState(Base *base) : _base(base)
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnNew = new TextButton(148, 16, 8, 176);
	_btnOk = new TextButton(148, 16, 164, 176);
	_txtTitle = new Text(310, 17, 5, 8);
	_txtAvailable = new Text(150, 9, 8, 24);
	_txtAllocated = new Text(150, 9, 160, 24);
	_txtSpace = new Text(150, 9, 8, 34);
	_txtFunds = new Text(150, 9, 160, 34);
	_txtItem = new Text(80, 9, 10, 52);
	_txtEngineers = new Text(56, 18, 112, 44);
	_txtProduced = new Text(56, 18, 168, 44);
	_txtCost = new Text(44, 27, 222, 44);
	_txtTimeLeft = new Text(60, 27, 260, 44);
	_lstManufacture = new TextList(288, 88, 8, 80);

	// Set palette
	setInterface("manufactureMenu");

	add(_window, "window", "manufactureMenu");
	add(_btnNew, "button", "manufactureMenu");
	add(_btnOk, "button", "manufactureMenu");
	add(_txtTitle, "text1", "manufactureMenu");
	add(_txtAvailable, "text1", "manufactureMenu");
	add(_txtAllocated, "text1", "manufactureMenu");
	add(_txtSpace, "text1", "manufactureMenu");
	add(_txtFunds, "text1", "manufactureMenu");
	add(_txtItem, "text2", "manufactureMenu");
	add(_txtEngineers, "text2", "manufactureMenu");
	add(_txtProduced, "text2", "manufactureMenu");
	add(_txtCost, "text2", "manufactureMenu");
	add(_txtTimeLeft, "text2", "manufactureMenu");
	add(_lstManufacture, "list", "manufactureMenu");

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK17.SCR"));

	_btnNew->setText(tr("STR_NEW_PRODUCTION"));
	_btnNew->onMouseClick((ActionHandler)&ManufactureState::btnNewProductionClick);
	_btnNew->onKeyboardPress((ActionHandler)&ManufactureState::btnNewProductionClick, Options::keyToggleQuickSearch);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&ManufactureState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&ManufactureState::btnOkClick, Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_CURRENT_PRODUCTION"));

	_txtFunds->setText(tr("STR_CURRENT_FUNDS").arg(Text::formatFunding(_game->getSavedGame()->getFunds())));

	_txtItem->setText(tr("STR_ITEM"));

	_txtEngineers->setText(tr("STR_ENGINEERS__ALLOCATED"));
	_txtEngineers->setWordWrap(true);

	_txtProduced->setText(tr("STR_UNITS_PRODUCED"));
	_txtProduced->setWordWrap(true);

	_txtCost->setText(tr("STR_COST__PER__UNIT"));
	_txtCost->setWordWrap(true);

	_txtTimeLeft->setText(tr("STR_DAYS_HOURS_LEFT"));
	_txtTimeLeft->setWordWrap(true);

	_lstManufacture->setArrowColumn(90, ARROW_VERTICAL);
	_lstManufacture->setColumns(5, 115, 15, 52, 56, 48);
	_lstManufacture->setAlign(ALIGN_RIGHT);
	_lstManufacture->setAlign(ALIGN_LEFT, 0);
	_lstManufacture->setSelectable(true);
	_lstManufacture->setBackground(_window);
	_lstManufacture->setMargin(2);
	_lstManufacture->setWordWrap(true);
	_lstManufacture->onLeftArrowClick((ActionHandler)&ManufactureState::lstManufactureLeftArrowClick);
	_lstManufacture->onRightArrowClick((ActionHandler)&ManufactureState::lstManufactureRightArrowClick);
	_lstManufacture->onMouseClick((ActionHandler)&ManufactureState::lstManufactureClickLeft, SDL_BUTTON_LEFT);
	_lstManufacture->onMouseClick((ActionHandler)&ManufactureState::lstManufactureClickMiddle, SDL_BUTTON_MIDDLE);
	_lstManufacture->onMousePress((ActionHandler)&ManufactureState::lstManufactureMousePress);
	fillProductionList(0);
}

/**
 *
 */
ManufactureState::~ManufactureState()
{

}

/**
 * Updates the production list
 * after going to other screens.
 */
void ManufactureState::init()
{
	State::init();
	fillProductionList(0);
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void ManufactureState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Opens the screen with the list of possible productions.
 * @param action Pointer to an action.
 */
void ManufactureState::btnNewProductionClick(Action *)
{
	_game->pushState(new NewManufactureListState(_base));
}

/**
 * Fills the list of base productions.
 */
void ManufactureState::fillProductionList(size_t scroll)
{
	const std::vector<Production *> &productions = _base->getProductions();
	_lstManufacture->clearList();
	for (std::vector<Production *>::const_iterator iter = productions.begin(); iter != productions.end(); ++iter)
	{
		std::wostringstream s1;
		s1 << (*iter)->getAssignedEngineers();
		std::wostringstream s2;
		s2 << (*iter)->getAmountProduced() << "/";
		if ((*iter)->getInfiniteAmount()) s2 << Language::utf8ToWstr("∞");
		else s2 << (*iter)->getAmountTotal();
		if ((*iter)->getSellItems()) s2 << " $";
		std::wostringstream s3;
		s3 << Text::formatFunding((*iter)->getRules()->getManufactureCost());
		std::wostringstream s4;
		if ((*iter)->getInfiniteAmount())
		{
			s4 << Language::utf8ToWstr("∞");
		}
		else if ((*iter)->getAssignedEngineers() > 0)
		{
			int timeLeft = (*iter)->getAmountTotal() * (*iter)->getRules()->getManufactureTime() - (*iter)->getTimeSpent();
			int numEffectiveEngineers = (*iter)->getAssignedEngineers();
			// ensure we round up since it takes an entire hour to manufacture any part of that hour's capacity
			int hoursLeft = (timeLeft + numEffectiveEngineers - 1) / numEffectiveEngineers;
			int daysLeft = hoursLeft / 24;
			int hours = hoursLeft % 24;
			s4 << daysLeft << "/" << hours;
		}
		else
		{

			s4 << L"-";
		}
		_lstManufacture->addRow(5, tr((*iter)->getRules()->getName()).c_str(), s1.str().c_str(), s2.str().c_str(), s3.str().c_str(), s4.str().c_str());
	}

	if (scroll)
		_lstManufacture->scrollTo(scroll);
	_lstManufacture->draw();

	_txtAvailable->setText(tr("STR_ENGINEERS_AVAILABLE").arg(_base->getAvailableEngineers()));
	_txtAllocated->setText(tr("STR_ENGINEERS_ALLOCATED").arg(_base->getAllocatedEngineers()));
	_txtSpace->setText(tr("STR_WORKSHOP_SPACE_AVAILABLE").arg(_base->getFreeWorkshops()));
}

/**
 * Opens the screen displaying production settings.
 * @param action Pointer to an action.
 */
void ManufactureState::lstManufactureClickLeft(Action *action)
{
	double mx = action->getAbsoluteXMouse();
	if (mx >= _lstManufacture->getArrowsLeftEdge() && mx < _lstManufacture->getArrowsRightEdge())
	{
		return;
	}

	const std::vector<Production*> productions(_base->getProductions());
	_game->pushState(new ManufactureInfoState(_base, productions[_lstManufacture->getSelectedRow()]));
}

/**
* Opens the TechTreeViewer for the corresponding topic.
* @param action Pointer to an action.
*/
void ManufactureState::lstManufactureClickMiddle(Action *)
{
	const std::vector<Production*> productions(_base->getProductions());
	const RuleManufacture *selectedTopic = productions[_lstManufacture->getSelectedRow()]->getRules();
	_game->pushState(new TechTreeViewerState(0, selectedTopic));
}

/**
* Moves a production up on the list.
* @param action Pointer to an action.
* @param row Selected production row.
* @param max Move the production to the top?
*/
void ManufactureState::moveProductionUp(Action *action, unsigned int row, bool max)
{
	if (_lstManufacture->getRows() < 2) return;

	Production *s = _base->getProductions().at(row);
	if (max)
	{
		_base->getProductions().erase(_base->getProductions().begin() + row);

		int engineers = 0;
		for (std::vector<Production*>::iterator ii = _base->getProductions().begin(); ii != _base->getProductions().end(); ++ii)
		{
			Production *prod = *ii;
			if (prod->getAssignedEngineers())
			{
				engineers += prod->getAssignedEngineers();
				prod->setAssignedEngineers(0);
			}
		}

		s->setAssignedEngineers(s->getAssignedEngineers() + engineers);

		_base->getProductions().insert(_base->getProductions().begin(), s);
	}
	else
	{
		Production *swap = _base->getProductions().at(row) = _base->getProductions().at(row - 1);
		if (swap->getAssignedEngineers())
		{
			s->setAssignedEngineers(s->getAssignedEngineers() + swap->getAssignedEngineers());
			swap->setAssignedEngineers(0);
		}
		_base->getProductions().at(row - 1) = s;
		if (row != _lstManufacture->getScroll())
		{
			SDL_WarpMouse(action->getLeftBlackBand() + action->getXMouse(), action->getTopBlackBand() + action->getYMouse() - static_cast<Uint16>(8 * action->getYScale()));
		}
		else
		{
			_lstManufacture->scrollUp(false);
		}
	}
	fillProductionList(_lstManufacture->getScroll());
}

/**
* Reorders a production up.
* @param action Pointer to an action.
*/
void ManufactureState::lstManufactureLeftArrowClick(Action *action)
{
	unsigned int row = _lstManufacture->getSelectedRow();
	if (row > 0)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			moveProductionUp(action, row);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		{
			moveProductionUp(action, row, true);
		}
	}
}

/**
* Moves a production down on the list.
* @param action Pointer to an action.
* @param row Selected production row.
* @param max Move the production to the bottom?
*/
void ManufactureState::moveProductionDown(Action *action, unsigned int row, bool max)
{
	if (_lstManufacture->getRows() < 2) return;

	Production *s = _base->getProductions().at(row);
	if (max)
	{
		_base->getProductions().erase(_base->getProductions().begin() + row);
		_base->getProductions().insert(_base->getProductions().end(), s);

		if (s->getAssignedEngineers())
		{
			_base->getProductions().front()->setAssignedEngineers(_base->getProductions().front()->getAssignedEngineers() + s->getAssignedEngineers());
			s->setAssignedEngineers(0);
		}
	}
	else
	{
		Production *swap = _base->getProductions().at(row) = _base->getProductions().at(row + 1);
		if (s->getAssignedEngineers())
		{
			swap->setAssignedEngineers(s->getAssignedEngineers() + swap->getAssignedEngineers());
			s->setAssignedEngineers(0);
		}
		_base->getProductions().at(row + 1) = s;
		
		if (row != _lstManufacture->getVisibleRows() - 1 + _lstManufacture->getScroll())
		{
			SDL_WarpMouse(action->getLeftBlackBand() + action->getXMouse(), action->getTopBlackBand() + action->getYMouse() + static_cast<Uint16>(8 * action->getYScale()));
		}
		else
		{
			_lstManufacture->scrollDown(false);
		}
	}
	fillProductionList(_lstManufacture->getScroll());
}

/**
* Reorders a production down.
* @param action Pointer to an action.
*/
void ManufactureState::lstManufactureRightArrowClick(Action *action)
{
	unsigned int row = _lstManufacture->getSelectedRow();
	size_t numProductions = _base->getProductions().size();
	if (0 < numProductions && INT_MAX >= numProductions && row < numProductions - 1)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			moveProductionDown(action, row);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		{
			moveProductionDown(action, row, true);
		}
	}
}

/**
* Handles the mouse-wheels on the arrow-buttons.
* @param action Pointer to an action.
*/
void ManufactureState::lstManufactureMousePress(Action *action)
{
	if (Options::changeValueByMouseWheel == 0)
		return;
	unsigned int row = _lstManufacture->getSelectedRow();
	size_t numProductions = _base->getProductions().size();
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP &&
		row > 0)
	{
		if (action->getAbsoluteXMouse() >= _lstManufacture->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstManufacture->getArrowsRightEdge())
		{
			moveProductionUp(action, row);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN &&
		0 < numProductions && INT_MAX >= numProductions && row < numProductions - 1)
	{
		if (action->getAbsoluteXMouse() >= _lstManufacture->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstManufacture->getArrowsRightEdge())
		{
			moveProductionDown(action, row);
		}
	}
}

}
