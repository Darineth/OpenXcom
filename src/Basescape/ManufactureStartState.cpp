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
#include "ManufactureStartState.h"
#include <sstream>
#include "../Interface/Window.h"
#include "../Interface/TextButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Menu/ErrorMessageState.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/RuleManufacture.h"
#include "../Mod/RuleItem.h"
#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "ManufactureInfoState.h"
#include "../Savegame/SavedGame.h"
#include "../Menu/ErrorMessageState.h"
#include "../Mod/RuleInterface.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the productions start screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param item The RuleManufacture to produce.
 */
ManufactureStartState::ManufactureStartState(Base *base, RuleManufacture *item) : _base(base), _item(item)
{
	_screen = false;

	_window = new Window(this, 320, 180, 0, 20);
	_btnCancel = new TextButton(136, 16, 16, 175);
	_txtTitle = new Text(320, 17, 0, 30);
	_txtManHour = new Text(290, 9, 16, 50);
	_txtCost = new Text(290, 9, 16, 60);
	_txtSell = new Text(290, 9, 16, 80);
	_txtCurrent = new Text(290, 9, 16, 90);
	_txtWorkSpace = new Text(290, 9, 16, 70);

	_txtRequiredItemsTitle = new Text(290, 9, 16, 104);
	_txtItemNameColumn = new Text(60, 16, 30, 112);
	_txtUnitRequiredColumn = new Text(60, 16, 155, 112);
	_txtUnitAvailableColumn = new Text(60, 16, 230, 112);
	_lstRequiredItems = new TextList(270, 40, 30, 128);

	_btnStart = new TextButton(136, 16, 168, 175);

	// Set palette
	setInterface("allocateManufacture");

	add(_window, "window", "allocateManufacture");
	add(_txtTitle, "text", "allocateManufacture");
	add(_txtManHour, "text", "allocateManufacture");
	add(_txtCost, "text", "allocateManufacture");
	add(_txtSell, "text", "allocateManufacture");
	add(_txtCurrent, "text", "allocateManufacture");
	add(_txtWorkSpace, "text", "allocateManufacture");
	add(_btnCancel, "button", "allocateManufacture");

	add(_txtRequiredItemsTitle, "text", "allocateManufacture");
	add(_txtItemNameColumn, "text", "allocateManufacture");
	add(_txtUnitRequiredColumn, "text", "allocateManufacture");
	add(_txtUnitAvailableColumn, "text", "allocateManufacture");
	add(_lstRequiredItems, "list", "allocateManufacture");

	add(_btnStart, "button", "allocateManufacture");

	centerAllSurfaces();

	_window->setBackground(_game->getMod()->getSurface("BACK17.SCR"));

	_txtTitle->setText(tr(_item->getName()));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtManHour->setText(tr("STR_ENGINEER_HOURS_TO_PRODUCE_ONE_UNIT").arg(_item->getManufactureTime()));

	_txtCost->setText(tr("STR_COST_PER_UNIT_").arg(Text::formatFunding(_item->getManufactureCost())));

	Mod *mod = _game->getMod();

	bool singleItem = false;
	std::string singleItemId;

	if (singleItem)
	{
		_txtCurrent->setText(tr("STR_CURRENT_STORES").arg(Text::formatNumber(_base->getStorageItems()->getItem(singleItemId))));
	}
	else
	{
		_txtCurrent->setVisible(false);
	}

	if (singleItem)
	{
		_txtCurrent->setText(LocalizedText(L"Current Stores>\x01{0}").arg(Text::formatNumber(_base->getStorageItems()->getItem(singleItemId))));
	}
	else
	{
		_txtCurrent->setVisible(false);
	}

	_txtWorkSpace->setText(tr("STR_WORK_SPACE_REQUIRED").arg(_item->getRequiredSpace()));

	/*	CONFIRM:
	_txtCost->setColor(Palette::blockOffset(13)+10);
	_txtCost->setSecondaryColor(Palette::blockOffset(13));*/
	_txtCost->setText(tr("STR_COST_PER_UNIT_").arg(Text::formatFunding(_item->getManufactureCost())));

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&ManufactureStartState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&ManufactureStartState::btnCancelClick, Options::keyCancel);

	const std::map<std::string, int> & requiredItems(_item->getRequiredItems());
	const std::map<std::string, int> & producedItems(_item->getProducedItems());

	int sellPrice = 0;
	for (std::map<std::string, int>::const_iterator ii = producedItems.begin(); ii != producedItems.end(); ++ii)
	{
		RuleItem* item = mod->getItem(ii->first);
		if (item)
		{
			if (singleItem)
			{
				singleItem = false;
			}
			else if (singleItemId.size() == 0)
			{
				singleItem = true;
				singleItemId = ii->first;
			}

			sellPrice += item->getSellCost() * ii->second;
		}
	}
	_txtSell->setText(tr("STR_SELL_PER_UNIT").arg(Text::formatFunding(sellPrice)));

	int availableWorkSpace = _base->getFreeWorkshops();
	bool productionPossible = _game->getSavedGame()->getFunds() > _item->getManufactureCost();
	//productionPossible &= (availableWorkSpace > 0);

	_txtRequiredItemsTitle->setText(tr("STR_SPECIAL_MATERIALS_REQUIRED"));
	_txtRequiredItemsTitle->setAlign(ALIGN_CENTER);

	_txtItemNameColumn->setText(tr("STR_ITEM_REQUIRED"));
	_txtItemNameColumn->setWordWrap(true);

	_txtUnitRequiredColumn->setText(tr("STR_UNITS_REQUIRED"));
	_txtUnitRequiredColumn->setWordWrap(true);

	_txtUnitAvailableColumn->setText(tr("STR_UNITS_AVAILABLE"));
	_txtUnitAvailableColumn->setWordWrap(true);

	_lstRequiredItems->setColumns(3, 140, 75, 55);
	_lstRequiredItems->setBackground(_window);

	int row = 0;
	for (std::map<std::string, int>::const_iterator iter = requiredItems.begin(); iter != requiredItems.end(); ++iter)
	{
		std::wostringstream s1, s2;
		s1 << iter->second;
		if (_game->getMod()->getItem(iter->first) != 0)
		{
			s2 << base->getStorageItems()->getItem(iter->first);
			productionPossible &= (base->getStorageItems()->getItem(iter->first) >= iter->second);
		}
		else if (_game->getMod()->getCraft(iter->first) != 0)
		{
			s2 << base->getCraftCount(iter->first);
			productionPossible &= (base->getCraftCount(iter->first) >= iter->second);
		}
		_lstRequiredItems->addRow(3, tr(iter->first).c_str(), s1.str().c_str(), s2.str().c_str());
		_lstRequiredItems->setCellColor(row, 1, _lstRequiredItems->getSecondaryColor());
		_lstRequiredItems->setCellColor(row, 2, _lstRequiredItems->getSecondaryColor());
		row++;
	}
	if (_item->getSpawnedPersonType() != "")
	{
		if (base->getAvailableQuarters() <= base->getUsedQuarters())
		{
			productionPossible = false;
		}

		// separator line
		_lstRequiredItems->addRow(1, tr("STR_PERSON_JOINING").c_str());
		_lstRequiredItems->setCellColor(row, 0, _lstRequiredItems->getSecondaryColor());
		row++;

		// person joining
		std::wostringstream s1;
		s1 << L'\x01' << 1;
		_lstRequiredItems->addRow(2, tr(_item->getSpawnedPersonName() != "" ? _item->getSpawnedPersonName() : _item->getSpawnedPersonType()).c_str(), s1.str().c_str());
		row++;
	}
	if (!producedItems.empty())
	{
		// separator line
		_lstRequiredItems->addRow(1, tr("STR_UNITS_PRODUCED").c_str());
		_lstRequiredItems->setCellColor(row, 0, _lstRequiredItems->getSecondaryColor());
		row++;

		// produced items
		for (std::map<std::string, int>::const_iterator iter = producedItems.begin(); iter != producedItems.end(); ++iter)
		{
			std::wostringstream s1;
			s1 << L'\x01' << iter->second;
			_lstRequiredItems->addRow(2, tr(iter->first).c_str(), s1.str().c_str());
			row++;
		}
	}

	_txtRequiredItemsTitle->setVisible(!requiredItems.empty());
	_txtItemNameColumn->setVisible(!requiredItems.empty());
	_txtUnitRequiredColumn->setVisible(!requiredItems.empty());
	_txtUnitAvailableColumn->setVisible(!requiredItems.empty());
	_lstRequiredItems->setVisible(!requiredItems.empty() || !producedItems.empty());

	_btnStart->setText(tr("STR_START_PRODUCTION"));
	_btnStart->onMouseClick((ActionHandler)&ManufactureStartState::btnStartClick);
	_btnStart->onKeyboardPress((ActionHandler)&ManufactureStartState::btnStartClick, Options::keyOk);
	_btnStart->setVisible(productionPossible);

	if (_item)
	{
		// mark new as normal
		if (_game->getSavedGame()->getManufactureRuleStatus(_item->getName()) == RuleManufacture::MANU_STATUS_NEW)
		{
			_game->getSavedGame()->setManufactureRuleStatus(_item->getName(), RuleManufacture::MANU_STATUS_NORMAL);
		}
	}
}

/**
 * Returns to previous screen.
 * @param action A pointer to an Action.
 */
void ManufactureStartState::btnCancelClick(Action *)
{
	_game->popState();
}

/**
 * Go to the Production settings screen.
 * @param action A pointer to an Action.
 */
void ManufactureStartState::btnStartClick(Action *)
{
	if (_item->getCategory() == "STR_CRAFT" && _base->getAvailableHangars() - _base->getUsedHangars() <= 0)
	{
		_game->pushState(new ErrorMessageState(tr("STR_NO_FREE_HANGARS_FOR_CRAFT_PRODUCTION"), _palette, _game->getMod()->getInterface("basescape")->getElement("errorMessage")->color, "BACK17.SCR", _game->getMod()->getInterface("basescape")->getElement("errorPalette")->color));
	}
	else if (_item->getCategory() == "STR_VEHICLE" && _base->getAvailableQuarters() - _base->getUsedQuarters() <= 0)
	{
		// TODO: Vehicle storage space requirement?
		//_game->pushState(new ErrorMessageState(tr("STR_NOT_ENOUGH_LIVING_SPACE"), _palette, _game->getMod()->getInterface("basescape")->getElement("errorMessage")->color, "BACK17.SCR", _game->getMod()->getInterface("basescape")->getElement("errorPalette")->color));
	}
	/*	else if (_item->getRequiredSpace() > _base->getFreeWorkshops())
		{
			_game->pushState(new ErrorMessageState(tr("STR_NOT_ENOUGH_WORK_SPACE"), _palette, _game->getMod()->getInterface("basescape")->getElement("errorMessage")->color, "BACK17.SCR", _game->getMod()->getInterface("basescape")->getElement("errorPalette")->color));
		}*/
	else
	{
		_game->pushState(new ManufactureInfoState(_base, _item));
	}
}

}
