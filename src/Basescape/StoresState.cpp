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
#include "StoresState.h"
#include <sstream>
#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/ArrowButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/ToggleTextButton.h"
#include "../Interface/Window.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/ResearchProject.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Transfer.h"
#include "../Savegame/Vehicle.h"
#include "../Mod/Armor.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleResearch.h"
#include <algorithm>

namespace OpenXcom
{

struct compareItemName : public std::binary_function<StoredItem&, StoredItem&, bool>
{
	bool _reverse;

	compareItemName(bool reverse) : _reverse(reverse) {}

	bool operator()(const StoredItem &a, const StoredItem &b) const
	{
		return CrossPlatform::naturalCompare(a.name, b.name);
	}
};

struct compareItemQuantity : public std::binary_function<StoredItem&, StoredItem&, bool>
{
	bool _reverse;

	compareItemQuantity(bool reverse) : _reverse(reverse) {}

	bool operator()(const StoredItem &a, const StoredItem &b) const
	{
		return (a.quantity < b.quantity) || ((a.quantity == b.quantity) && CrossPlatform::naturalCompare(a.name, b.name));
	}
};

struct compareItemSize : public std::binary_function<StoredItem&, StoredItem&, bool>
{
	bool _reverse;

	compareItemSize(bool reverse) : _reverse(reverse) {}

	bool operator()(const StoredItem &a, const StoredItem &b) const
	{
		return (a.size < b.size) || ((a.size == b.size) && CrossPlatform::naturalCompare(a.name, b.name));
	}
};

struct compareItemSpaceUsed : public std::binary_function<StoredItem&, StoredItem&, bool>
{
	bool _reverse;

	compareItemSpaceUsed(bool reverse) : _reverse(reverse) {}

	bool operator()(const StoredItem &a, const StoredItem &b) const
	{
		return (a.spaceUsed < b.spaceUsed) || ((a.spaceUsed == b.spaceUsed) && CrossPlatform::naturalCompare(a.name, b.name));
	}
};


/**
 * Initializes all the elements in the Stores window.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
StoresState::StoresState(Base *base) : _base(base)
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnQuickSearch = new TextEdit(this, 48, 9, 10, 20);
	_btnOk = new TextButton(148, 16, 164, 176);
	_btnGrandTotal = new ToggleTextButton(148, 16, 8, 176);
	_txtTitle = new Text(310, 17, 5, 8);
	_txtItem = new Text(142, 9, 10, 32);
	_txtQuantity = new Text(54, 9, 152, 32);
	_txtSize = new Text(54, 9, 212, 32);
	_txtSpaceUsed = new Text(54, 9, 248, 32);
	_lstStores = new TextList(288, 128, 8, 40);
	_sortName = new ArrowButton(ARROW_NONE, 11, 8, 10, 32);
	_sortQuantity = new ArrowButton(ARROW_NONE, 11, 8, 152, 32);
	_sortSize = new ArrowButton(ARROW_NONE, 11, 8, 212, 32);
	_sortSpaceUsed = new ArrowButton(ARROW_NONE, 11, 8, 248, 32);

	// Set palette
	setInterface("storesInfo");

	add(_window, "window", "storesInfo");
	add(_btnQuickSearch, "button", "storesInfo");
	add(_btnOk, "button", "storesInfo");
	add(_btnGrandTotal, "button", "storesInfo");
	add(_txtTitle, "text", "storesInfo");
	add(_txtItem, "text", "storesInfo");
	add(_txtQuantity, "text", "storesInfo");
	add(_txtSize, "text", "storesInfo");
	add(_txtSpaceUsed, "text", "storesInfo");
	add(_lstStores, "list", "storesInfo");
	add(_sortName, "text", "storesInfo");
	add(_sortQuantity, "text", "storesInfo");
	add(_sortSize, "text", "storesInfo");
	add(_sortSpaceUsed, "text", "storesInfo");

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK13.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&StoresState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&StoresState::btnOkClick, Options::keyOk);
	_btnOk->onKeyboardPress((ActionHandler)&StoresState::btnOkClick, Options::keyCancel);

	_btnGrandTotal->setText(tr("STR_GRAND_TOTAL"));
	_btnGrandTotal->onMouseClick((ActionHandler)&StoresState::btnGrandTotalClick);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_STORES"));

	_txtItem->setText(tr("STR_ITEM"));

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));
	_txtSize->setText(tr("STR_SIZE_UC"));
	_txtSpaceUsed->setText(tr("STR_SPACE_USED_UC"));

	_lstStores->setColumns(4, 162, 40, 50, 34);
	_lstStores->setSelectable(true);
	_lstStores->setBackground(_window);
	_lstStores->setMargin(2);

	_sortName->setX(_sortName->getX() + _txtItem->getTextWidth() + 4);
	_sortName->onMouseClick((ActionHandler)&StoresState::sortNameClick);

	_sortQuantity->setX(_sortQuantity->getX() + _txtQuantity->getTextWidth() + 4);
	_sortQuantity->onMouseClick((ActionHandler)&StoresState::sortQuantityClick);

	_sortSize->setX(_sortSize->getX() + _txtSize->getTextWidth() + 4);
	_sortSize->onMouseClick((ActionHandler)&StoresState::sortSizeClick);

	_sortSpaceUsed->setX(_sortSpaceUsed->getX() + _txtSpaceUsed->getTextWidth() + 4);
	_sortSpaceUsed->onMouseClick((ActionHandler)&StoresState::sortSpaceUsedClick);

	itemOrder = ITEM_SORT_NONE;
	updateArrows();

	_btnQuickSearch->setText(L""); // redraw
	_btnQuickSearch->onEnter((ActionHandler)&StoresState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(false);

	_btnOk->onKeyboardRelease((ActionHandler)&StoresState::btnQuickSearchToggle, Options::keyToggleQuickSearch);
}

/**
 *
 */
StoresState::~StoresState()
{

}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void StoresState::btnOkClick(Action *)
{
	_game->popState();
}

/**
* Quick search toggle.
* @param action Pointer to an action.
*/
void StoresState::btnQuickSearchToggle(Action *action)
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
void StoresState::btnQuickSearchApply(Action *)
{
	initList(_btnGrandTotal->getPressed());
}

/**
 * Reloads the item list.
 */
void StoresState::initList(bool grandTotal)
{
	std::wstring searchString = _btnQuickSearch->getText();
	for (auto & c : searchString) c = towupper(c);

	// clear everything
	_lstStores->clearList();
	_itemList.clear();

	// find relevant items
	const std::vector<std::string> &items = _game->getMod()->getItemsList();
	for (std::vector<std::string>::const_iterator item = items.begin(); item != items.end(); ++item)
	{
		// quick search
		if (searchString != L"")
		{
			std::wstring projectName = tr((*item));
			for (auto & c : projectName) c = towupper(c);
			if (projectName.find(searchString) == std::string::npos)
			{
				continue;
			}
		}

		int qty = 0;
		if (!grandTotal)
		{
			// items in stores from this base only
			qty += _base->getStorageItems()->getItem(*item);
		}
		else
		{
			// items from all bases
			for (std::vector<Base*>::iterator base = _game->getSavedGame()->getBases()->begin(); base != _game->getSavedGame()->getBases()->end(); ++base)
			{
				// 1. items in base stores
				qty += (*base)->getStorageItems()->getItem(*item);

				// 2. items from craft
				for (std::vector<Craft*>::iterator craft = (*base)->getCrafts()->begin(); craft != (*base)->getCrafts()->end(); ++craft)
				{
					// 2a. craft equipment
					qty += (*craft)->getItems()->getItem(*item);

					// 2b. craft weapons + ammo
					for (std::vector<CraftWeapon*>::iterator craftWeapon = (*craft)->getWeapons()->begin(); craftWeapon != (*craft)->getWeapons()->end(); ++craftWeapon)
					{
						if (*craftWeapon)
						{
							if ((*craftWeapon)->getRules()->getLauncherItem() == (*item))
							{
								qty += 1;
							}
							else if ((*craftWeapon)->getRules()->getClipItem() == (*item))
							{
								qty += (*craftWeapon)->getClipsLoaded(_game->getMod());
							}
						}
					}

					// 2c. craft vehicles + ammo
					for (std::vector<Vehicle*>::iterator vehicle = (*craft)->getVehicles()->begin(); vehicle != (*craft)->getVehicles()->end(); ++vehicle)
					{
						if ((*vehicle)->getRules()->getType() == (*item))
						{
							qty += 1;
						}
						else if (!(*vehicle)->getRules()->getPrimaryCompatibleAmmo()->empty())
						{
							if ((*vehicle)->getRules()->getPrimaryCompatibleAmmo()->front() == (*item))
							{
								qty += (*vehicle)->getAmmo();
							}
						}
					}
				}

				// 3. armor in use (worn by soldiers)
				for (std::vector<Soldier*>::iterator soldier = (*base)->getSoldiers()->begin(); soldier != (*base)->getSoldiers()->end(); ++soldier)
				{
					if ((*soldier)->getArmor()->getStoreItem() == (*item))
					{
						qty += 1;
					}
				}

				// 4. items/aliens in research
				for (std::vector<ResearchProject *>::const_iterator research = (*base)->getResearch().begin(); research != (*base)->getResearch().end(); ++research)
				{
					if ((*research)->getRules()->needItem() && (*research)->getRules()->getName() == (*item))
					{
						if ((*research)->getRules()->destroyItem())
						{
							qty += 1;
							break;
						}
					}
				}

				// 5. items in transfer
				for (std::vector<Transfer*>::iterator transfer = (*base)->getTransfers()->begin(); transfer != (*base)->getTransfers()->end(); ++transfer)
				{
					Craft *craft2 = (*transfer)->getCraft();
					if (craft2)
					{
						// 5a. craft equipment
						qty += craft2->getItems()->getItem(*item);

						// 5b. craft weapons + ammo
						for (std::vector<CraftWeapon*>::iterator craftWeapon = craft2->getWeapons()->begin(); craftWeapon != craft2->getWeapons()->end(); ++craftWeapon)
						{
							if (*craftWeapon)
							{
								if ((*craftWeapon)->getRules()->getLauncherItem() == (*item))
								{
									qty += 1;
								}
								else if ((*craftWeapon)->getRules()->getClipItem() == (*item))
								{
									qty += (*craftWeapon)->getClipsLoaded(_game->getMod());
								}
							}
						}

						// 5c. craft vehicles + ammo
						for (std::vector<Vehicle*>::iterator vehicle = craft2->getVehicles()->begin(); vehicle != craft2->getVehicles()->end(); ++vehicle)
						{
							if ((*vehicle)->getRules()->getType() == (*item))
							{
								qty += 1;
							}
							else if (!(*vehicle)->getRules()->getPrimaryCompatibleAmmo()->empty())
							{
								if ((*vehicle)->getRules()->getPrimaryCompatibleAmmo()->front() == (*item))
								{
									qty += (*vehicle)->getAmmo();
								}
							}
						}
					}
					else if ((*transfer)->getItems() == (*item))
					{
						// 5d. items in transfer
						qty += (*transfer)->getQuantity();
					}
				}
			}
		}

		if (qty > 0)
		{
			RuleItem *rule = _game->getMod()->getItem(*item, true);
			std::wostringstream ss, ss2;
			ss << qty;
			ss2 << qty * rule->getSize();
			std::wstring itemName = tr(*item);
			if (rule->getBattleType() == BT_AMMO || (rule->getBattleType() == BT_NONE && rule->getClipSize() > 0))
			{
				itemName = tr("STR_AMMO_COUNT_", rule->getClipSize()).arg(itemName);
			}
			_itemList.push_back(StoredItem(itemName, qty, rule->getSize(), qty * rule->getSize()));
		}
	}

	sortList(itemOrder);
}

/**
 * Refreshes the item list.
 */
void StoresState::init()
{
	State::init();

	initList(false);
}

/**
 * Includes items from all bases.
 * @param action Pointer to an action.
 */
void StoresState::btnGrandTotalClick(Action *action)
{
	initList(_btnGrandTotal->getPressed());
}

/**
 * Updates the sorting arrows based
 * on the current setting.
 */
void StoresState::updateArrows()
{
	_sortName->setShape(ARROW_NONE);
	_sortQuantity->setShape(ARROW_NONE);
	_sortSize->setShape(ARROW_NONE);
	_sortSpaceUsed->setShape(ARROW_NONE);
	switch (itemOrder)
	{
	case ITEM_SORT_NONE:
		break;
	case ITEM_SORT_NAME_ASC:
		_sortName->setShape(ARROW_SMALL_UP);
		break;
	case ITEM_SORT_NAME_DESC:
		_sortName->setShape(ARROW_SMALL_DOWN);
		break;
	case ITEM_SORT_QUANTITY_ASC:
		_sortQuantity->setShape(ARROW_SMALL_UP);
		break;
	case ITEM_SORT_QUANTITY_DESC:
		_sortQuantity->setShape(ARROW_SMALL_DOWN);
		break;
	case ITEM_SORT_SIZE_ASC:
		_sortSize->setShape(ARROW_SMALL_UP);
		break;
	case ITEM_SORT_SIZE_DESC:
		_sortSize->setShape(ARROW_SMALL_DOWN);
		break;
	case ITEM_SORT_SPACE_USED_ASC:
		_sortSpaceUsed->setShape(ARROW_SMALL_UP);
		break;
	case ITEM_SORT_SPACE_USED_DESC:
		_sortSpaceUsed->setShape(ARROW_SMALL_DOWN);
		break;
	default:
		break;
	}
}

/**
 * Sorts the item list.
 * @param sort Order to sort the items in.
 */
void StoresState::sortList(ItemSort sort)
{
	switch (sort)
	{
	case ITEM_SORT_NONE:
		break;
	case ITEM_SORT_NAME_ASC:
		std::sort(_itemList.begin(), _itemList.end(), compareItemName(false));
		break;
	case ITEM_SORT_NAME_DESC:
		std::sort(_itemList.rbegin(), _itemList.rend(), compareItemName(true));
		break;
	case ITEM_SORT_QUANTITY_ASC:
		std::sort(_itemList.begin(), _itemList.end(), compareItemQuantity(false));
		break;
	case ITEM_SORT_QUANTITY_DESC:
		std::sort(_itemList.rbegin(), _itemList.rend(), compareItemQuantity(true));
		break;
	case ITEM_SORT_SIZE_ASC:
		std::sort(_itemList.begin(), _itemList.end(), compareItemSize(false));
		break;
	case ITEM_SORT_SIZE_DESC:
		std::sort(_itemList.rbegin(), _itemList.rend(), compareItemSize(true));
		break;
	case ITEM_SORT_SPACE_USED_ASC:
		std::sort(_itemList.begin(), _itemList.end(), compareItemSpaceUsed(false));
		break;
	case ITEM_SORT_SPACE_USED_DESC:
		std::sort(_itemList.rbegin(), _itemList.rend(), compareItemSpaceUsed(true));
		break;
	}
	updateList();
}

/**
 * Updates the item list with the current list
 * of available items.
 */
void StoresState::updateList()
{
	for (std::vector<StoredItem>::const_iterator j = _itemList.begin(); j != _itemList.end(); ++j)
	{
		std::wostringstream ss, ss2, ss3;
		ss << (*j).quantity;
		ss2 << (*j).size;
		ss3 << (*j).spaceUsed;
		_lstStores->addRow(4, (*j).name.c_str(), ss.str().c_str(), ss2.str().c_str(), ss3.str().c_str());
	}
}

/**
 * Sorts the items by name.
 * @param action Pointer to an action.
 */
void StoresState::sortNameClick(Action *)
{
	if (itemOrder == ITEM_SORT_NAME_ASC)
	{
		itemOrder = ITEM_SORT_NAME_DESC;
	}
	else
	{
		itemOrder = ITEM_SORT_NAME_ASC;
	}
	updateArrows();
	_lstStores->clearList();
	sortList(itemOrder);
}

/**
 * Sorts the items by quantity.
 * @param action Pointer to an action.
 */
void StoresState::sortQuantityClick(Action *)
{
	if (itemOrder == ITEM_SORT_QUANTITY_ASC)
	{
		itemOrder = ITEM_SORT_QUANTITY_DESC;
	}
	else
	{
		itemOrder = ITEM_SORT_QUANTITY_ASC;
	}
	updateArrows();
	_lstStores->clearList();
	sortList(itemOrder);
}

/**
 * Sorts the items by size.
 * @param action Pointer to an action.
 */
void StoresState::sortSizeClick(Action *)
{
	if (itemOrder == ITEM_SORT_SIZE_ASC)
	{
		itemOrder = ITEM_SORT_SIZE_DESC;
	}
	else
	{
		itemOrder = ITEM_SORT_SIZE_ASC;
	}
	updateArrows();
	_lstStores->clearList();
	sortList(itemOrder);
}

/**
 * Sorts the items by space used.
 * @param action Pointer to an action.
 */
void StoresState::sortSpaceUsedClick(Action *)
{
	if (itemOrder == ITEM_SORT_SPACE_USED_ASC)
	{
		itemOrder = ITEM_SORT_SPACE_USED_DESC;
	}
	else
	{
		itemOrder = ITEM_SORT_SPACE_USED_ASC;
	}
	updateArrows();
	_lstStores->clearList();
	sortList(itemOrder);
}

}
