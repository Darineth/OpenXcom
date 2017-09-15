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
#include "SellState.h"
#include "ManufactureDependenciesTreeState.h"
#include <algorithm>
#include <sstream>
#include <climits>
#include <cmath>
#include <iomanip>
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Interface/ComboBox.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Base.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/Vehicle.h"
#include "../Mod/RuleItem.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleCraft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Engine/Timer.h"
#include "../Engine/Options.h"
#include "../Mod/RuleInterface.h"
#include "../Battlescape/DebriefingState.h"
#include "TransferBaseState.h"
#include "../Ufopaedia/Ufopaedia.h"
#include "../Mod/RuleResearch.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Sell/Sack screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param origin Game section that originated this state.
 */
SellState::SellState(Base *base, DebriefingState *debriefingState, OptionsOrigin origin) : _base(base), _debriefingState(debriefingState), _sel(0), _total(0), _spaceChange(0), _origin(origin), _reset(false)
{
	bool overfull = _debriefingState == 0 && Options::storageLimitsEnforced && _base->storesOverfull();

	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnQuickSearch = new TextEdit(this, 48, 9, 10, 13);
	//_btnOk = new TextButton(overfull? 288:148, 16, overfull? 16:8, 176);
	_btnOk = new TextButton(148, 16, 8, 176);
	_btnCancel = new TextButton(148, 16, 164, 176);
	_btnTransfer = new TextButton(148, 16, 164, 176);
	_txtTitle = new Text(310, 17, 5, 8);
	_txtSales = new Text(150, 9, 10, 24);
	_txtFunds = new Text(150, 9, 160, 24);
	_txtSpaceUsed = new Text(150, 9, 160, 34);
	_txtQuantity = new Text(54, 9, 136, 44);
	_txtSell = new Text(96, 9, 190, 44);
	_txtValue = new Text(40, 9, 270, 44);
	_cbxCategory = new ComboBox(this, 120, 16, 10, 36);
	_lstItems = new TextList(287, 120, 8, 54);

	// Set palette
	setInterface("sellMenu");

	_ammoColor = _game->getMod()->getInterface("sellMenu")->getElement("ammoColor")->color;

	add(_window, "window", "sellMenu");
	add(_btnQuickSearch, "button", "sellMenu");
	add(_btnOk, "button", "sellMenu");
	add(_btnCancel, "button", "sellMenu");
	add(_btnTransfer, "button", "sellMenu");
	add(_txtTitle, "text", "sellMenu");
	add(_txtSales, "text", "sellMenu");
	add(_txtFunds, "text", "sellMenu");
	add(_txtSpaceUsed, "text", "sellMenu");
	add(_txtQuantity, "text", "sellMenu");
	add(_txtSell, "text", "sellMenu");
	add(_txtValue, "text", "sellMenu");
	add(_lstItems, "list", "sellMenu");
	add(_cbxCategory, "text", "sellMenu");

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK13.SCR"));

	_btnOk->setText(tr("STR_SELL_SACK"));
	_btnOk->onMouseClick((ActionHandler)&SellState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&SellState::btnOkClick, Options::keyOk);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&SellState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&SellState::btnCancelClick, Options::keyCancel);

	_btnTransfer->setText(tr("STR_GO_TO_TRANSFERS"));
	_btnTransfer->onMouseClick((ActionHandler)&SellState::btnTransferClick);

	_btnCancel->setVisible(!overfull);
	_btnOk->setVisible(!overfull);
	_btnTransfer->setVisible(overfull);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SELL_ITEMS_SACK_PERSONNEL"));

	_txtFunds->setText(tr("STR_FUNDS").arg(Text::formatFunding(_game->getSavedGame()->getFunds())));

	//_txtSpaceUsed->setVisible(Options::storageLimitsEnforced);

	std::wostringstream ss;
	ss << " " << _base->getUsedStores() << "/" << _base->getAvailableStores();
	_txtSpaceUsed->setText(ss.str());
	_txtSpaceUsed->setText(tr("STR_SPACE_USED").arg(ss.str()));

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_txtSell->setText(tr("STR_SELL_SACK"));

	_txtValue->setText(tr("STR_VALUE"));

	_lstItems->setArrowColumn(182, ARROW_VERTICAL);
	_lstItems->setColumns(4, 156, 54, 24, 53);
	_lstItems->setSelectable(true);
	_lstItems->setBackground(_window);
	_lstItems->setMargin(2);
	_lstItems->onLeftArrowPress((ActionHandler)&SellState::lstItemsLeftArrowPress);
	_lstItems->onLeftArrowRelease((ActionHandler)&SellState::lstItemsLeftArrowRelease);
	_lstItems->onLeftArrowClick((ActionHandler)&SellState::lstItemsLeftArrowClick);
	_lstItems->onRightArrowPress((ActionHandler)&SellState::lstItemsRightArrowPress);
	_lstItems->onRightArrowRelease((ActionHandler)&SellState::lstItemsRightArrowRelease);
	_lstItems->onRightArrowClick((ActionHandler)&SellState::lstItemsRightArrowClick);
	_lstItems->onMousePress((ActionHandler)&SellState::lstItemsMousePress);
	
	_cats.push_back("STR_ALL_ITEMS");

	const std::vector<std::string> &cw = _game->getMod()->getCraftWeaponsList();
	for (std::vector<std::string>::const_iterator i = cw.begin(); i != cw.end(); ++i)
	{
		RuleCraftWeapon *rule = _game->getMod()->getCraftWeapon(*i);
		_craftWeapons.insert(rule->getLauncherItem());
		_craftWeapons.insert(rule->getClipItem());
	}
	const std::vector<std::string> &ar = _game->getMod()->getArmorsList();
	for (std::vector<std::string>::const_iterator i = ar.begin(); i != ar.end(); ++i)
	{
		Armor *rule = _game->getMod()->getArmor(*i);
		_armors.insert(rule->getStoreItem());
	}

	for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		if ((*i)->getCraft() == 0 && _debriefingState == 0)
		{
			TransferRow row = { TRANSFER_SOLDIER, (*i), (*i)->getName(true), 0, 1, 0, 0 };
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}
	for (std::vector<Craft*>::iterator i = _base->getCrafts()->begin(); i != _base->getCrafts()->end(); ++i)
	{
		if ((*i)->getStatus() != "STR_OUT" && _debriefingState == 0)
		{
			TransferRow row = { TRANSFER_CRAFT, (*i), (*i)->getName(_game->getLanguage()), (*i)->getRules()->getSellCost(), 1, 0, 0 };
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}
	if (_base->getAvailableScientists() > 0 && _debriefingState == 0)
	{
		TransferRow row = { TRANSFER_SCIENTIST, 0, tr("STR_SCIENTIST"), 0, _base->getAvailableScientists(), 0, 0 };
		_items.push_back(row);
		std::string cat = getCategory(_items.size() - 1);
		if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
		{
			_cats.push_back(cat);
		}
	}
	if (_base->getAvailableEngineers() > 0 && _debriefingState == 0)
	{
		TransferRow row = { TRANSFER_ENGINEER, 0, tr("STR_ENGINEER"), 0, _base->getAvailableEngineers(), 0, 0 };
		_items.push_back(row);
		std::string cat = getCategory(_items.size() - 1);
		if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
		{
			_cats.push_back(cat);
		}
	}

	const std::vector<std::string> allResearchIds = _game->getMod()->getResearchList();
	std::vector<RuleResearch*> allResearch;
	allResearch.reserve(allResearchIds.size());

	for (size_t ii = 0; ii < allResearchIds.size(); ++ii)
	{
		allResearch.push_back(_game->getMod()->getResearch(allResearchIds[ii]));
	}

	const std::vector<std::string> &items = _game->getMod()->getItemsList();
	for (std::vector<std::string>::const_iterator i = items.begin(); i != items.end(); ++i)
	{
		int qty = _base->getStorageItems()->getItem(*i);
		if (Options::storageLimitsEnforced && _origin == OPT_BATTLESCAPE)
		{
			for (std::vector<Transfer*>::iterator j = _base->getTransfers()->begin(); j != _base->getTransfers()->end(); ++j)
			{
				if ((*j)->getItems() == *i)
				{
					qty += (*j)->getQuantity();
				}
			}
			for (std::vector<Craft*>::iterator j = _base->getCrafts()->begin(); j != _base->getCrafts()->end(); ++j)
			{
				qty += (*j)->getItems()->getItem(*i);
			}
		}
		RuleItem *rule = _game->getMod()->getItem(*i, true);
		if (_debriefingState != 0)
		{
			qty = _debriefingState->getRecoveredItemCount(rule);
		}
		if (qty > 0 && (Options::canSellLiveAliens || !rule->isAlien()))
		{
			const std::string &itemName = *i;
			bool needsResearch = false;
			for (std::vector<RuleResearch*>::const_iterator jj = allResearch.begin(); jj != allResearch.end(); ++jj)
			{
				RuleResearch *rr = *jj;
				if (!_game->getSavedGame()->isResearched(rr->getName()))
				{
					if (rr->needItem() && rr->getName() == itemName)
					{
						needsResearch = true;
						break;
					}
					else if (std::find(rr->getNeededItemsAll().begin(), rr->getNeededItemsAll().end(), itemName) != rr->getNeededItemsAll().end())
					{
						needsResearch = true;
						break;
					}
					else if (std::find(rr->getNeededItemsAny().begin(), rr->getNeededItemsAny().end(), itemName) != rr->getNeededItemsAny().end())
					{
						needsResearch = true;
						break;
					}
				}
			}

			TransferRow row = { TRANSFER_ITEM, rule, tr(*i), rule->getSellCost(), qty, 0, 0 };
			if ((_debriefingState != 0) && (_game->getSavedGame()->getAutosell(rule)))
			{
				row.amount = qty;
				_total += row.cost * qty;
				_spaceChange -= qty * rule->getSize();
			}
			_items.push_back(row);
			std::string cat = getCategory(_items.size() - 1);
			if (std::find(_cats.begin(), _cats.end(), cat) == _cats.end())
			{
				_cats.push_back(cat);
			}
		}
	}

	if (_game->getMod()->getUseCustomCategories())
	{
		// first find all relevant item categories
		std::vector<std::string> tempCats;
		for (std::vector<TransferRow>::iterator i = _items.begin(); i != _items.end(); ++i)
		{
			if ((*i).type == TRANSFER_ITEM)
			{
				RuleItem *rule = (RuleItem*)((*i).rule);
				for (std::vector<std::string>::const_iterator j = rule->getCategories().begin(); j != rule->getCategories().end(); ++j)
				{
					if (std::find(tempCats.begin(), tempCats.end(), (*j)) == tempCats.end())
					{
						tempCats.push_back((*j));
					}
				}
			}
		}
		// then use them nicely in order
		_cats.clear();
		_cats.push_back("STR_ALL_ITEMS");
		const std::vector<std::string> &categories = _game->getMod()->getItemCategoriesList();
		for (std::vector<std::string>::const_iterator k = categories.begin(); k != categories.end(); ++k)
		{
			if (std::find(tempCats.begin(), tempCats.end(), (*k)) != tempCats.end())
			{
				_cats.push_back((*k));
			}
		}
	}

	_txtSales->setText(tr("STR_VALUE_OF_SALES").arg(Text::formatFunding(_total)));

	_cbxCategory->setOptions(_cats);
	_cbxCategory->onChange((ActionHandler)&SellState::cbxCategoryChange);
	_cbxCategory->onKeyboardPress((ActionHandler)&SellState::btnSellAllClick, Options::keySelectAll);

	_btnQuickSearch->setText(L""); // redraw
	_btnQuickSearch->onEnter((ActionHandler)&SellState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(false);

	// OK button is not always visible, so bind it here
	_cbxCategory->onKeyboardRelease((ActionHandler)&SellState::btnQuickSearchToggle, Options::keyToggleQuickSearch);

	updateList();

	_timerInc = new Timer(250);
	_timerInc->onTimer((StateHandler)&SellState::increase);
	_timerDec = new Timer(250);
	_timerDec->onTimer((StateHandler)&SellState::decrease);
}

/**
 *
 */
SellState::~SellState()
{
	delete _timerInc;
	delete _timerDec;
}

/**
* Resets stuff when coming back from other screens.
*/
void SellState::init()
{
	State::init();

	if (_reset)
	{
		_game->popState();
		_game->pushState(new SellState(_base, _debriefingState, _origin));
	}
}

/**
 * Runs the arrow timers.
 */
void SellState::think()
{
	State::think();

	_timerInc->think(this, 0);
	_timerDec->think(this, 0);
}

/**
 * Determines the category a row item belongs in.
 * @param sel Selected row.
 * @returns Item category.
 */
std::string SellState::getCategory(int sel) const
{
	RuleItem *rule = 0;
	switch (_items[sel].type)
	{
	case TRANSFER_SOLDIER:
	case TRANSFER_SCIENTIST:
	case TRANSFER_ENGINEER:
		return "STR_PERSONNEL";
	case TRANSFER_CRAFT:
		return "STR_CRAFT_ARMAMENT";
	case TRANSFER_ITEM:
		rule = (RuleItem*)_items[sel].rule;
		if (rule->getBattleType() == BT_CORPSE || rule->isAlien())
		{
			return "STR_ALIENS";
		}
		if (rule->getBattleType() == BT_NONE)
		{
			if (_craftWeapons.find(rule->getType()) != _craftWeapons.end())
			{
				return "STR_CRAFT_ARMAMENT";
			}
			if (_armors.find(rule->getType()) != _armors.end())
			{
				return "STR_EQUIPMENT";
			}
			return "STR_COMPONENTS";
		}
		return "STR_EQUIPMENT";
	}
	return "STR_ALL_ITEMS";
}

/**
 * Determines if a row item belongs to a given category.
 * @param sel Selected row.
 * @param cat Category.
 * @returns True if row item belongs to given category, otherwise False.
 */
bool SellState::belongsToCategory(int sel, const std::string &cat) const
{
	switch (_items[sel].type)
	{
	case TRANSFER_SOLDIER:
	case TRANSFER_SCIENTIST:
	case TRANSFER_ENGINEER:
	case TRANSFER_CRAFT:
		return false;
	case TRANSFER_ITEM:
		RuleItem *rule = (RuleItem*)_items[sel].rule;
		return rule->belongsToCategory(cat);
	}
	return false;
}

/**
* Quick search toggle.
* @param action Pointer to an action.
*/
void SellState::btnQuickSearchToggle(Action *action)
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
void SellState::btnQuickSearchApply(Action *)
{
	updateList();
}

/**
 * Filters the current list of items.
 */
void SellState::updateList()
{
	std::wstring searchString = _btnQuickSearch->getText();
	for (auto & c : searchString) c = towupper(c);

	_lstItems->clearList();
	_rows.clear();
	for (size_t i = 0; i < _items.size(); ++i)
	{
		// filter
		std::string cat = _cats[_cbxCategory->getSelected()];
		if (_game->getMod()->getUseCustomCategories())
		{
			if (cat != "STR_ALL_ITEMS" && !belongsToCategory(i, cat))
			{
				continue;
			}
		}
		else
		{
			if (cat != "STR_ALL_ITEMS" && cat != getCategory(i))
			{
				continue;
			}
		}

		// quick search
		if (searchString != L"")
		{
			std::wstring projectName = _items[i].name;
			for (auto & c : projectName) c = towupper(c);
			if (projectName.find(searchString) == std::string::npos)
			{
				continue;
			}
		}

		std::wstring name = _items[i].name;
		bool ammo = false;
		if (_items[i].type == TRANSFER_ITEM)
		{
			RuleItem *rule = (RuleItem*)_items[i].rule;
			ammo = (rule->getBattleType() == BT_AMMO || (rule->getBattleType() == BT_NONE && rule->getClipSize() > 0));
			if (ammo)
			{
				name.insert(0, L"  ");
			}
		}

		if (_items[i].needsResearch)
		{
			name.append(tr("STR_NOT_RESEARCHED"));
		}

		std::wostringstream ssQty, ssAmount;
		ssQty << _items[i].qtySrc - _items[i].amount;
		ssAmount << _items[i].amount;
		_lstItems->addRow(4, name.c_str(), ssQty.str().c_str(), ssAmount.str().c_str(), Text::formatFunding(_items[i].cost).c_str());
		_rows.push_back(i);
		if (_items[i].amount > 0)
		{
			_lstItems->setRowColor(_rows.size() - 1, _lstItems->getSecondaryColor());
		}
		else if (ammo)
		{
			_lstItems->setRowColor(_rows.size() - 1, _ammoColor);
		}
	}
}

/**
 * Sells the selected items.
 * @param action Pointer to an action.
 */
void SellState::btnOkClick(Action *)
{
	_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() + _total);
	Soldier *soldier;
	Craft *craft;
	for (std::vector<TransferRow>::const_iterator i = _items.begin(); i != _items.end(); ++i)
	{
		if (i->amount > 0)
		{
			switch (i->type)
			{
			case TRANSFER_SOLDIER:
				soldier = (Soldier*)i->rule;
				for (std::vector<Soldier*>::iterator s = _base->getSoldiers()->begin(); s != _base->getSoldiers()->end(); ++s)
				{
					if (*s == soldier)
					{
						if ((*s)->getArmor()->getStoreItem() != Armor::NONE)
						{
							_base->getStorageItems()->addItem((*s)->getArmor()->getStoreItem());
						}
						_base->getSoldiers()->erase(s);
						break;
					}
				}
				delete soldier;
				break;
			case TRANSFER_CRAFT:
				craft = (Craft*)i->rule;

				// Unload craft
				craft->unload(_game->getMod());

				// Clear hangar
				for (std::vector<BaseFacility*>::iterator f = _base->getFacilities()->begin(); f != _base->getFacilities()->end(); ++f)
				{
					if ((*f)->getCraft() == craft)
					{
						(*f)->setCraft(0);
						break;
					}
				}

				// Remove craft
				for (std::vector<Craft*>::iterator c = _base->getCrafts()->begin(); c != _base->getCrafts()->end(); ++c)
				{
					if (*c == craft)
					{
						_base->getCrafts()->erase(c);
						break;
					}
				}
				delete craft;
				break;
			case TRANSFER_SCIENTIST:
				_base->setScientists(_base->getScientists() - i->amount);
				break;
			case TRANSFER_ENGINEER:
				_base->setEngineers(_base->getEngineers() - i->amount);
				break;
			case TRANSFER_ITEM:
				RuleItem *item = (RuleItem*)i->rule;
				if (_base->getStorageItems()->getItem(item->getType()) < i->amount)
				{
					int toRemove = i->amount - _base->getStorageItems()->getItem(item->getType());

					// remove all of said items from base
					_base->getStorageItems()->removeItem(item->getType(), INT_MAX);

					// if we still need to remove any, remove them from the crafts first, and keep a running tally
					for (std::vector<Craft*>::iterator j = _base->getCrafts()->begin(); j != _base->getCrafts()->end() && toRemove; ++j)
					{
						if ((*j)->getItems()->getItem(item->getType()) < toRemove)
						{
							toRemove -= (*j)->getItems()->getItem(item->getType());
							(*j)->getItems()->removeItem(item->getType(), INT_MAX);
						}
						else
						{
							(*j)->getItems()->removeItem(item->getType(), toRemove);
							toRemove = 0;
						}
					}

					// if there are STILL any left to remove, take them from the transfers, and if necessary, delete it.
					for (std::vector<Transfer*>::iterator j = _base->getTransfers()->begin(); j != _base->getTransfers()->end() && toRemove;)
					{
						if ((*j)->getItems() == item->getType())
						{
							if ((*j)->getQuantity() <= toRemove)
							{
								toRemove -= (*j)->getQuantity();
								delete *j;
								j = _base->getTransfers()->erase(j);
							}
							else
							{
								(*j)->setItems((*j)->getItems(), (*j)->getQuantity() - toRemove);
								toRemove = 0;
							}
						}
						else
						{
							++j;
						}
					}
				}
				else
				{
					_base->getStorageItems()->removeItem(item->getType(), i->amount);
				}
				if (_debriefingState != 0)
				{
					// set autosell status if we sold all of the item
					_game->getSavedGame()->setAutosell(item, (i->qtySrc == i->amount));
				}

				break;
			}
		}
		else
		{
			if (_debriefingState != 0 && i->type == TRANSFER_ITEM)
			{
				// disable autosell since we haven't sold any of the item.
				_game->getSavedGame()->setAutosell((RuleItem*)i->rule, false);
			}
		}
	}
	if (_debriefingState != 0)
	{
		_debriefingState->setShowSellButton(false);
	}
	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SellState::btnCancelClick(Action *)
{
	_game->popState();
}

/**
* Opens the Transfer UI and gives the player an option to transfer stuff instead of selling it.
* Returns back to this screen when finished.
* @param action Pointer to an action.
*/
void SellState::btnTransferClick(Action *)
{
	_reset = true;
	_game->pushState(new TransferBaseState(_base));
}

/**
* Increase all items to max, i.e. sell everything.
* @param action Pointer to an action.
*/
void SellState::btnSellAllClick(Action *)
{
	size_t backup = _sel;
	for (size_t i = 0; i < _lstItems->getRows(); ++i)
	{
		_sel = i;
		changeByValue(INT_MAX, 1);
	}
	_sel = backup;
}

/**
 * Starts increasing the item.
 * @param action Pointer to an action.
 */
void SellState::lstItemsLeftArrowPress(Action *action)
{
	_sel = _lstItems->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT && !_timerInc->isRunning()) _timerInc->start();
}

/**
 * Stops increasing the item.
 * @param action Pointer to an action.
 */
void SellState::lstItemsLeftArrowRelease(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerInc->stop();
	}
}

/**
 * Increases the selected item;
 * by one on left-click, to max on right-click.
 * @param action Pointer to an action.
 */
void SellState::lstItemsLeftArrowClick(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) changeByValue(INT_MAX, 1);
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		changeByValue(1,1);
		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
}

/**
 * Starts decreasing the item.
 * @param action Pointer to an action.
 */
void SellState::lstItemsRightArrowPress(Action *action)
{
	_sel = _lstItems->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT && !_timerDec->isRunning()) _timerDec->start();
}

/**
 * Stops decreasing the item.
 * @param action Pointer to an action.
 */
void SellState::lstItemsRightArrowRelease(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timerDec->stop();
	}
}

/**
 * Decreases the selected item;
 * by one on left-click, to 0 on right-click.
 * @param action Pointer to an action.
 */
void SellState::lstItemsRightArrowClick(Action *action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) changeByValue(INT_MAX, -1);
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		changeByValue(1,-1);
		_timerInc->setInterval(250);
		_timerDec->setInterval(250);
	}
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action Pointer to an action.
 */
void SellState::lstItemsMousePress(Action *action)
{
	_sel = _lstItems->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		_timerInc->stop();
		_timerDec->stop();
		if (action->getAbsoluteXMouse() >= _lstItems->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstItems->getArrowsRightEdge())
		{
			changeByValue(Options::changeValueByMouseWheel, 1);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		_timerInc->stop();
		_timerDec->stop();
		if (action->getAbsoluteXMouse() >= _lstItems->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstItems->getArrowsRightEdge())
		{
			changeByValue(Options::changeValueByMouseWheel, -1);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		if (action->getAbsoluteXMouse() >= _lstItems->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstItems->getArrowsRightEdge())
		{
			return;
		}
		if (getRow().type == TRANSFER_ITEM)
		{
			RuleItem *rule = (RuleItem*)getRow().rule;
			if (rule != 0)
			{
				_game->pushState(new ManufactureDependenciesTreeState(rule->getType()));
			}
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_MIDDLE)
	{
		if (getRow().type == TRANSFER_ITEM)
		{
			RuleItem *rule = (RuleItem*)getRow().rule;
			if (rule != 0)
			{
				std::string articleId = rule->getType();
				Ufopaedia::openArticle(_game, articleId);
			}
		}
		else if (getRow().type == TRANSFER_CRAFT)
		{
			Craft *rule = (Craft*)getRow().rule;
			if (rule != 0)
			{
				std::string articleId = rule->getRules()->getType();
				Ufopaedia::openArticle(_game, articleId);
			}
		}
	}
}

/**
 * Increases the quantity of the selected item to sell by one.
 */
void SellState::increase()
{
	_timerDec->setInterval(50);
	_timerInc->setInterval(50);
	changeByValue(1,1);
}

/**
 * Increases or decreases the quantity of the selected item to sell.
 * @param change How much we want to add or remove.
 * @param dir Direction to change, +1 to increase or -1 to decrease.
 */
void SellState::changeByValue(int change, int dir)
{
	if (dir > 0)
	{
		if (0 >= change || getRow().qtySrc <= getRow().amount) return;
		change = std::min(getRow().qtySrc - getRow().amount, change);
	}
	else
	{
		if (0 >= change || 0 >= getRow().amount) return;
		change = std::min(getRow().amount, change);
	}
	getRow().amount += dir * change;
	_total += dir * getRow().cost * change;

	// Calculate the change in storage space.
	Craft *craft;
	Soldier *soldier;
	RuleItem *armor, *item, *weapon, *ammo;
	double total = 0.0;
	switch (getRow().type)
	{
	case TRANSFER_SOLDIER:
		soldier = (Soldier*)getRow().rule;
		if (soldier->getArmor()->getStoreItem() != Armor::NONE)
		{
			armor = _game->getMod()->getItem(soldier->getArmor()->getStoreItem(), true);
			_spaceChange += dir * armor->getSize();
		}
		break;
	case TRANSFER_CRAFT:
		craft = (Craft*)getRow().rule;
		for (std::vector<CraftWeapon*>::iterator w = craft->getWeapons()->begin(); w != craft->getWeapons()->end(); ++w)
		{
			if (*w)
			{
				weapon = _game->getMod()->getItem((*w)->getRules()->getLauncherItem(), true);
				total += weapon->getSize();
				ammo = _game->getMod()->getItem((*w)->getRules()->getClipItem());
				if (ammo)
					total += ammo->getSize() * (*w)->getClipsLoaded(_game->getMod());
			}
		}
		_spaceChange += dir * total;
		break;
	case TRANSFER_ITEM:
		item = (RuleItem*)getRow().rule;
		_spaceChange -= dir * change * item->getSize();
		break;
	default:
		break;
	}

	updateItemStrings();
}

/**
 * Decreases the quantity of the selected item to sell by one.
 */
void SellState::decrease()
{
	_timerInc->setInterval(50);
	_timerDec->setInterval(50);
	changeByValue(1,-1);
}

/**
 * Updates the quantity-strings of the selected item.
 */
void SellState::updateItemStrings()
{
	std::wostringstream ss, ss2, ss3;
	ss << getRow().amount;
	_lstItems->setCellText(_sel, 2, ss.str());
	ss2 << getRow().qtySrc - getRow().amount;
	_lstItems->setCellText(_sel, 1, ss2.str());
	_txtSales->setText(tr("STR_VALUE_OF_SALES").arg(Text::formatFunding(_total)));

	if (getRow().amount > 0)
	{
		_lstItems->setRowColor(_sel, _lstItems->getSecondaryColor());
	}
	else
	{
		_lstItems->setRowColor(_sel, _lstItems->getColor());
		if (getRow().type == TRANSFER_ITEM)
		{
			RuleItem *rule = (RuleItem*)getRow().rule;
			if (rule->getBattleType() == BT_AMMO || (rule->getBattleType() == BT_NONE && rule->getClipSize() > 0))
			{
				_lstItems->setRowColor(_sel, _ammoColor);
			}
		}
	}

	ss3 << _base->getUsedStores();
	if (std::abs(_spaceChange) > 0.05)
	{
		ss3 << "(";
		if (_spaceChange > 0.05)
			ss3 << "+";
		ss3 << std::fixed << std::setprecision(1) << _spaceChange << ")";
	}
	ss3 << "/" << _base->getAvailableStores();
	_txtSpaceUsed->setText(tr("STR_SPACE_USED").arg(ss3.str()));
	if (_debriefingState == 0 && Options::storageLimitsEnforced)
	{
		_btnOk->setVisible(!_base->storesOverfull(_spaceChange));
	}
}

/**
* Updates the production list to match the category filter.
*/
void SellState::cbxCategoryChange(Action *)
{
	updateList();
}

}
