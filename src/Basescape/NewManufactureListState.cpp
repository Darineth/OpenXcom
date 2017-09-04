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
#include "NewManufactureListState.h"
#include "../Interface/Window.h"
#include "../Interface/TextButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Interface/ComboBox.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleManufacture.h"
#include "../Mod/RuleItem.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Savegame/Production.h"
#include "ManufactureStartState.h"
#include "../Menu/ErrorMessageState.h"
#include "../Mod/RuleInterface.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the productions list screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
NewManufactureListState::NewManufactureListState(Base *base) : _base(base)
{
	_screen = false;

	_window = new Window(this, 320, 156, 0, 22, POPUP_BOTH);
	_btnOk = new TextButton(304, 16, 8, 154);
	_txtTitle = new Text(320, 17, 0, 30);
	_txtItem = new Text(156, 9, 10, 62);
	_txtCategory = new Text(130, 9, 166, 62);
	_lstManufacture = new TextList(288, 80, 8, 70);
	_cbxCategory = new ComboBox(this, 146, 16, 166, 46);

	// Set palette
	setInterface("selectNewManufacture");

	add(_window, "window", "selectNewManufacture");
	add(_btnOk, "button", "selectNewManufacture");
	add(_txtTitle, "text", "selectNewManufacture");
	add(_txtItem, "text", "selectNewManufacture");
	add(_txtCategory, "text", "selectNewManufacture");
	add(_lstManufacture, "list", "selectNewManufacture");
	add(_cbxCategory, "catBox", "selectNewManufacture");

	centerAllSurfaces();

	_window->setBackground(_game->getMod()->getSurface("BACK17.SCR"));

	_txtTitle->setText(tr("STR_PRODUCTION_ITEMS"));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtItem->setText(tr("STR_ITEM"));

	_txtCategory->setText(tr("STR_CATEGORY"));

	_lstManufacture->setColumns(2, 156, 130);
	_lstManufacture->setSelectable(true);
	_lstManufacture->setBackground(_window);
	_lstManufacture->setMargin(2);
	_lstManufacture->onMouseClick((ActionHandler)&NewManufactureListState::lstProdClick, SDL_BUTTON_LEFT);
	_lstManufacture->onMouseClick((ActionHandler)&NewManufactureListState::lstProdRClick, SDL_BUTTON_RIGHT);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&NewManufactureListState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&NewManufactureListState::btnOkClick, Options::keyCancel);

	_possibleProductions.clear();
	_game->getSavedGame()->getAvailableProductions(_possibleProductions, _game->getMod(), _base);
	_catStrings.push_back("STR_ALL_ITEMS");

	for (std::vector<RuleManufacture *>::iterator it = _possibleProductions.begin(); it != _possibleProductions.end(); ++it)
	{
		bool addCategory = true;
		for (size_t x = 0; x < _catStrings.size(); ++x)
		{
			if ((*it)->getCategory() == _catStrings[x])
			{
				addCategory = false;
				break;
			}
		}
		if (addCategory)
		{
			_catStrings.push_back((*it)->getCategory());
		}
	}

	_cbxCategory->setOptions(_catStrings);
	_cbxCategory->onChange((ActionHandler)&NewManufactureListState::cbxCategoryChange);

}

/**
 * Initializes state (fills list of possible productions).
 */
void NewManufactureListState::init()
{
	State::init();
	fillProductionList();
}

/**
 * Returns to the previous screen.
 * @param action A pointer to an Action.
 */
void NewManufactureListState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Opens the Production settings screen.
 * @param action A pointer to an Action.
 */
void NewManufactureListState::lstProdClick(Action *)
{
	RuleManufacture *rule = 0;
	for (std::vector<RuleManufacture *>::iterator it = _possibleProductions.begin(); it != _possibleProductions.end(); ++it)
	{
		if ((*it)->getName() == _displayedStrings[_lstManufacture->getSelectedRow()])
		{
			rule = (*it);
			break;
		}
	}
	_game->pushState(new ManufactureStartState(_base, rule));
}

/**
* Instantly queues up one production order for the item.
* @param action A pointer to an Action.
*/
void NewManufactureListState::lstProdRClick(Action* action)
{
	RuleManufacture *rule = 0;
	for (std::vector<RuleManufacture *>::iterator it = _possibleProductions.begin(); it != _possibleProductions.end(); ++it)
	{
		if ((*it)->getName() == _displayedStrings[_lstManufacture->getSelectedRow()])
		{
			rule = (*it);
			break;
		}
	}
	
	int availableEngineers = _base->getAvailableEngineers();
	int availableWorkspace = _base->getFreeWorkshops();
	int itemWorkspace = rule->getRequiredSpace();

	/*if (itemWorkspace > availableWorkspace)
	{
		_game->pushState(new ErrorMessageState(tr("STR_NOT_ENOUGH_WORK_SPACE"), _palette, _game->getMod()->getInterface("basescape")->getElement("errorMessage")->color, "BACK17.SCR", _game->getMod()->getInterface("basescape")->getElement("errorPalette")->color));
		return;
	}*/

	int addedEngineers = (availableWorkspace - itemWorkspace - availableEngineers) >= 0 ? availableEngineers : availableWorkspace - itemWorkspace;
	addedEngineers = std::max(addedEngineers, 0);

	bool existingProduction = false;
	std::vector<Production *> &Productions = _base->getProductions();
	for (std::vector<Production *>::iterator it = Productions.begin(); it != Productions.end(); ++it)
	{
		if ((*it)->getRules()->getName() == rule->getName())
		{	//Add to the existing production item if one exists
			(*it)->setAmountTotal((*it)->getAmountTotal() + 1);
			(*it)->setAssignedEngineers((*it)->getAssignedEngineers() + addedEngineers);
			_base->setEngineers(_base->getEngineers() - addedEngineers);
			existingProduction = true;
		}
	}
	if (!existingProduction)
	{	//Otherwise, add a new production item.
		Production* prod = new Production(rule, 1);
		prod->setAssignedEngineers(prod->getAssignedEngineers() + addedEngineers);
		_base->addProduction(prod);
		_base->setEngineers(_base->getEngineers() - addedEngineers);
	}
}

/**
 * Updates the production list to match the category filter
 */

void NewManufactureListState::cbxCategoryChange(Action *)
{
	fillProductionList();
}

/**
 * Fills the list of possible productions.
 */
void NewManufactureListState::fillProductionList()
{
	_lstManufacture->clearList();
	_possibleProductions.clear();
	_game->getSavedGame()->getAvailableProductions(_possibleProductions, _game->getMod(), _base);
	_displayedStrings.clear();

	Mod *mod = _game->getMod();
	bool singleItem = false;
	std::string singleItemId;

	for (std::vector<RuleManufacture *>::iterator it = _possibleProductions.begin(); it != _possibleProductions.end(); ++it)
	{
		if (((*it)->getCategory() == _catStrings[_cbxCategory->getSelected()]) || (_catStrings[_cbxCategory->getSelected()] == "STR_ALL_ITEMS"))
		{
			if ((*it)->getCategory() == "STR_AMMUNITION" || (*it)->getCategory() == "STR_CRAFT_AMMUNITION" || (*it)->getCategory() == "STR_HWP_AMMO")
			{
				const std::map<std::string, int> &producedItems = *it ? (*it)->getProducedItems() : (*it)->getProducedItems();
				for (std::map<std::string, int>::const_iterator ii = producedItems.begin(); ii != producedItems.end(); ++ii)
				{
					RuleItem* item = mod->getItem(ii->first);
					if (item)
					{
						if (singleItem)
						{
							singleItem = false;
							break;
						}
						else if (singleItemId.size() == 0)
						{
							singleItem = true;
							singleItemId = ii->first;
							break;
						}
					}
				}
			}

			std::wstring itemName = tr((*it)->getName());
			if (singleItem)
			{
				RuleItem *rule = _game->getMod()->getItem(singleItemId);
				itemName = tr(singleItemId);

				if (rule->getBattleType() == BT_AMMO || (rule->getBattleType() == BT_NONE && rule->getClipSize() > 0))
				{
					//itemName.append(L"(x" + std::to_wstring((long long)(std::max(1, rule->getClipSize()))) + L")");
					itemName = tr("STR_AMMO_COUNT_", rule->getClipSize()).arg(itemName);
				}
			}
			_lstManufacture->addRow(2, itemName.c_str(), tr((*it)->getCategory()).c_str());

			_displayedStrings.push_back((*it)->getName());
			singleItem = false;
			singleItemId = "";
		}
	}
}

}
