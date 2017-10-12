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
#include <algorithm>
#include "NewManufactureListState.h"
#include "../Interface/Window.h"
#include "../Interface/TextButton.h"
#include "../Interface/ToggleTextButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Interface/ComboBox.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/RuleManufacture.h"
#include "../Mod/RuleItem.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/Production.h"
#include "ManufactureStartState.h"
#include "TechTreeViewerState.h"
#include "../Menu/ErrorMessageState.h"
#include "../Mod/RuleInterface.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the productions list screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
NewManufactureListState::NewManufactureListState(Base *base) : _base(base), _showRequirements(false), _detailClicked(false)
{
	_screen = false;

	_window = new Window(this, 320, 156, 0, 22, POPUP_BOTH);
	_btnQuickSearch = new TextEdit(this, 48, 9, 10, 35);
	_btnOk = new TextButton(148, 16, 164, 154);
	_btnShowOnlyNew = new ToggleTextButton(148, 16, 8, 154);
	_txtTitle = new Text(320, 17, 0, 30);
	_txtItem = new Text(156, 9, 10, 62);
	_txtCategory = new Text(130, 9, 166, 62);
	_lstManufacture = new TextList(288, 80, 8, 70);
	_cbxFilter = new ComboBox(this, 146, 16, 10, 46);
	_cbxCategory = new ComboBox(this, 146, 16, 166, 46);

	// Set palette
	setInterface("selectNewManufacture");

	_hiddenColor = _game->getMod()->getInterface("selectNewManufacture")->getElement("listExtended")->color;
	_facilityRequiredColor = _game->getMod()->getInterface("selectNewManufacture")->getElement("listExtended")->color2;

	add(_window, "window", "selectNewManufacture");
	add(_btnQuickSearch, "button", "selectNewManufacture");
	add(_btnOk, "button", "selectNewManufacture");
	add(_btnShowOnlyNew, "button", "selectNewManufacture");
	add(_txtTitle, "text", "selectNewManufacture");
	add(_txtItem, "text", "selectNewManufacture");
	add(_txtCategory, "text", "selectNewManufacture");
	add(_lstManufacture, "list", "selectNewManufacture");
	add(_cbxFilter, "catBox", "selectNewManufacture");
	add(_cbxCategory, "catBox", "selectNewManufacture");

	centerAllSurfaces();

	_window->setBackground(_game->getMod()->getSurface("BACK17.SCR"));

	_txtTitle->setText(tr("STR_PRODUCTION_ITEMS"));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtItem->setText(tr("STR_ITEM"));

	_txtCategory->setText(tr("STR_CATEGORY"));

	_lstManufacture->setColumns(3, 156, 120, 10);
	_lstManufacture->setSelectable(true);
	_lstManufacture->setBackground(_window);
	_lstManufacture->setMargin(2);
	_lstManufacture->onMouseClick((ActionHandler)&NewManufactureListState::lstProdClickLeft, SDL_BUTTON_LEFT);
	_lstManufacture->onMouseClick((ActionHandler)&NewManufactureListState::lstProdClickRight, SDL_BUTTON_RIGHT);
	_lstManufacture->onMouseClick((ActionHandler)&NewManufactureListState::lstProdClickMiddle, SDL_BUTTON_MIDDLE);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&NewManufactureListState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&NewManufactureListState::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&NewManufactureListState::btnMarkAllAsSeenClick, Options::keyMarkAllAsSeen);

	_btnShowOnlyNew->setText(tr("STR_SHOW_ONLY_NEW"));
	_btnShowOnlyNew->onMouseClick((ActionHandler)&NewManufactureListState::btnShowOnlyNewClick);

	std::vector<std::string> filterOptions;
	filterOptions.push_back("STR_FILTER_DEFAULT");
	filterOptions.push_back("STR_FILTER_DEFAULT_SUPPLIES_OK");
	filterOptions.push_back("STR_FILTER_DEFAULT_NO_SUPPLIES");
	filterOptions.push_back("STR_FILTER_FACILITY_REQUIRED");
	filterOptions.push_back("STR_FILTER_HIDDEN");
	_cbxFilter->setOptions(filterOptions);
	_cbxFilter->onChange((ActionHandler)&NewManufactureListState::cbxFilterChange);

	_catStrings.push_back("STR_ALL_ITEMS");
	_cbxCategory->setOptions(_catStrings);

	_btnQuickSearch->setText(L""); // redraw
	_btnQuickSearch->onEnter((ActionHandler)&NewManufactureListState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(false);

	_btnOk->onKeyboardRelease((ActionHandler)&NewManufactureListState::btnQuickSearchToggle, Options::keyToggleQuickSearch);
}

/**
 * Initializes state (fills list of possible productions).
 */
void NewManufactureListState::init()
{
	State::init();
	fillProductionList(!_detailClicked);
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
void NewManufactureListState::lstProdClickLeft(Action *)
{
	ManufacturingFilterType basicFilter = (ManufacturingFilterType)(_cbxFilter->getSelected());
	if (basicFilter == MANU_FILTER_FACILITY_REQUIRED)
		return;

	RuleManufacture *rule = 0;
	for (std::vector<RuleManufacture *>::iterator it = _possibleProductions.begin(); it != _possibleProductions.end(); ++it)
	{
		if ((*it)->getName() == _displayedStrings[_lstManufacture->getSelectedRow()])
		{
			rule = (*it);
			break;
		}
	}

	// check and display error messages only further down the chain
	_detailClicked = true;
	_game->pushState(new ManufactureStartState(_base, rule));
}

/**
* Instantly queues up one production order for the item.
* @param action A pointer to an Action.
*/
void NewManufactureListState::lstProdClickRight(Action* action)
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
* Opens the TechTreeViewer for the corresponding topic.
* @param action Pointer to an action.
*/
void NewManufactureListState::lstProdClickMiddle(Action *)
{
	const RuleManufacture *selectedTopic = _game->getMod()->getManufacture(_displayedStrings[_lstManufacture->getSelectedRow()]);
	_game->pushState(new TechTreeViewerState(0, selectedTopic));
}

/**
* Updates the production list to match the basic filter
*/

void NewManufactureListState::cbxFilterChange(Action *)
{
	fillProductionList(true);
}

/**
 * Updates the production list to match the category filter
 */

void NewManufactureListState::cbxCategoryChange(Action *)
{
	fillProductionList(false);
}

/**
* Quick search toggle.
* @param action Pointer to an action.
*/
void NewManufactureListState::btnQuickSearchToggle(Action *action)
{
	if (_btnQuickSearch->getVisible())
	{
		_btnQuickSearch->setText(L"");
		_btnQuickSearch->setVisible(false);
		btnQuickSearchApply(action);
	}
	else
	{
		_btnQuickSearch->setVisible(true);
		_btnQuickSearch->setFocus(true);
	}
}

/**
* Quick search.
* @param action Pointer to an action.
*/
void NewManufactureListState::btnQuickSearchApply(Action *)
{
	fillProductionList(false);
}

/**
* Filter to display only new items.
* @param action Pointer to an action.
*/
void NewManufactureListState::btnShowOnlyNewClick(Action *)
{
	fillProductionList(false);
}

/**
* Marks all items as seen
* @param action Pointer to an action.
*/
void NewManufactureListState::btnMarkAllAsSeenClick(Action *)
{
	ManufacturingFilterType basicFilter = (ManufacturingFilterType)(_cbxFilter->getSelected());
	if (basicFilter == MANU_FILTER_FACILITY_REQUIRED)
		return;

	for (std::vector<std::string>::const_iterator i = _displayedStrings.begin(); i != _displayedStrings.end(); ++i)
	{
		// mark all (new) manufacture items as normal
		if (_game->getSavedGame()->getManufactureRuleStatus((*i)) != RuleManufacture::MANU_STATUS_HIDDEN)
		{
			_game->getSavedGame()->setManufactureRuleStatus((*i), RuleManufacture::MANU_STATUS_NORMAL);
		}
	}

	fillProductionList(false);
}

/**
 * Fills the list of possible productions.
 */
void NewManufactureListState::fillProductionList(bool refreshCategories)
{
	std::wstring searchString = _btnQuickSearch->getText();
	for (auto & c : searchString) c = towupper(c);

	if (refreshCategories)
	{
		_cbxCategory->onChange(0);
		_cbxCategory->setSelected(0);
	}

	_showRequirements = false;
	_detailClicked = false;

	_lstManufacture->clearList();
	_possibleProductions.clear();
	ManufacturingFilterType basicFilter = (ManufacturingFilterType)(_cbxFilter->getSelected());
	_game->getSavedGame()->getAvailableProductions(_possibleProductions, _game->getMod(), _base, basicFilter);
	_displayedStrings.clear();

	ItemContainer * itemContainer (_base->getStorageItems());
	int row = 0;
	bool hasUnseen = false;

	Mod *mod = _game->getMod();
	bool singleItem = false;
	std::string singleItemId;

	for (std::vector<RuleManufacture *>::iterator it = _possibleProductions.begin(); it != _possibleProductions.end(); ++it)
	{
		if (((*it)->getCategory() == _catStrings[_cbxCategory->getSelected()]) || (_catStrings[_cbxCategory->getSelected()] == "STR_ALL_ITEMS"))
		{
			// filter
			bool isHidden = _game->getSavedGame()->getManufactureRuleStatus((*it)->getName()) == RuleManufacture::MANU_STATUS_HIDDEN;
			if (basicFilter == MANU_FILTER_DEFAULT && isHidden)
				continue;
			if (basicFilter == MANU_FILTER_DEFAULT_SUPPLIES_OK && isHidden)
				continue;
			if (basicFilter == MANU_FILTER_DEFAULT_NO_SUPPLIES && isHidden)
				continue;
			if (basicFilter == MANU_FILTER_HIDDEN && !isHidden)
				continue;

			bool isNew = _game->getSavedGame()->getManufactureRuleStatus((*it)->getName()) == RuleManufacture::MANU_STATUS_NEW;
			if (_btnShowOnlyNew->getPressed())
			{
				if (!isNew)
				{
					continue;
				}
			}

			// quick search
			if (searchString != L"")
			{
				std::wstring projectName = tr((*it)->getName());
				for (auto & c : projectName) c = towupper(c);
				if (projectName.find(searchString) == std::string::npos)
				{
					continue;
				}
			}                                                   
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

			// supplies calculation
			int productionPossible = 10; // max
			if ((*it)->getManufactureCost() > 0)
			{
				int byFunds = _game->getSavedGame()->getFunds() / (*it)->getManufactureCost();
				productionPossible = std::min(productionPossible, byFunds);
			}
			const std::map<std::string, int> & requiredItems ((*it)->getRequiredItems());
			for (std::map<std::string, int>::const_iterator iter = requiredItems.begin(); iter != requiredItems.end(); ++iter)
			{
				productionPossible = std::min(productionPossible, itemContainer->getItem(iter->first) / iter->second);
			}
			std::wostringstream ss;
			if (productionPossible <= 0)
			{
				if (basicFilter == MANU_FILTER_DEFAULT_SUPPLIES_OK)
					continue;
				ss << L'-';
			}
			else
			{
				if (basicFilter == MANU_FILTER_DEFAULT_NO_SUPPLIES)
					continue;
				if (productionPossible < 10)
				{
					ss << productionPossible;
				}
				else
				{
					ss << L'+';
				}
			}

			_lstManufacture->addRow(2, itemName.c_str(), tr((*it)->getCategory()).c_str(), ss.str().c_str());
			_displayedStrings.push_back((*it)->getName().c_str());

			// colors
			if (basicFilter == MANU_FILTER_FACILITY_REQUIRED)
			{
				_lstManufacture->setRowColor(row, _facilityRequiredColor);
			}
			else
			{
				if (isHidden)
				{
					_lstManufacture->setRowColor(row, _hiddenColor);
				}
				else if (isNew)
				{
					_lstManufacture->setRowColor(row, _lstManufacture->getSecondaryColor());
					hasUnseen = true;
				}
			}
			row++;
		}
	}

	std::wstring label = tr("STR_SHOW_ONLY_NEW");
	_btnShowOnlyNew->setText((hasUnseen ? L"* " : L"") + label);

	if (refreshCategories)
	{
		_catStrings.clear();
		_catStrings.push_back("STR_ALL_ITEMS");

		for (size_t r = 0; r < _lstManufacture->getRows(); ++r)
		{
			RuleManufacture *info = _game->getMod()->getManufacture(_displayedStrings[r]);
			if (info)
			{
				bool addCategory = true;
				for (size_t x = 0; x < _catStrings.size(); ++x)
				{
					if (info->getCategory().c_str() == _catStrings[x])
					{
						addCategory = false;
						break;
					}
				}
				if (addCategory)
				{
					_catStrings.push_back(info->getCategory().c_str());
				}
			}
		}

		_cbxCategory->setOptions(_catStrings);
		_cbxCategory->onChange((ActionHandler)&NewManufactureListState::cbxCategoryChange);
	}
}

}
