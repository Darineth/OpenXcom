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
#include "PlaceFacilityState.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "BaseView.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/ItemContainer.h"
#include "../Mod/RuleBaseFacility.h"
#include "../Savegame/SavedGame.h"
#include "../Menu/ErrorMessageState.h"
#include "../Engine/Options.h"
#include "../Mod/RuleInterface.h"
#include <climits>

namespace OpenXcom
{

/**
 * Initializes all the elements in the Place Facility window.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param rule Pointer to the facility ruleset to build.
 */
PlaceFacilityState::PlaceFacilityState(Base *base, RuleBaseFacility *rule, BaseFacility *origFac) : _base(base), _rule(rule), _origFac(origFac)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 128, 160, 192, 40);
	_view = new BaseView(192, 192, 0, 8);
	_btnCancel = new TextButton(112, 16, 200, 176);
	_txtFacility = new Text(110, 9, 202, 50);
	_txtCost = new Text(110, 9, 202, 62);
	_numCost = new Text(110, 17, 202, 70);
	_txtTime = new Text(110, 9, 202, 90);
	_numTime = new Text(110, 17, 202, 98);
	_txtMaintenance = new Text(110, 9, 202, 118);
	_numMaintenance = new Text(110, 17, 202, 126);

	// Set palette
	setInterface("placeFacility");

	add(_window, "window", "placeFacility");
	add(_view, "baseView", "basescape");
	add(_btnCancel, "button", "placeFacility");
	add(_txtFacility, "text", "placeFacility");
	add(_txtCost, "text", "placeFacility");
	add(_numCost, "numbers", "placeFacility");
	add(_txtTime, "text", "placeFacility");
	add(_numTime, "numbers", "placeFacility");
	add(_txtMaintenance, "text", "placeFacility");
	add(_numMaintenance, "numbers", "placeFacility");

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK01.SCR"));

	_view->setTexture(_game->getMod()->getSurfaceSet("BASEBITS.PCK"));
	_view->setBase(_base);
	_view->setSelectable(rule->getSize());
	_view->onMouseClick((ActionHandler)&PlaceFacilityState::viewClick);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&PlaceFacilityState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&PlaceFacilityState::btnCancelClick, Options::keyCancel);

	_txtFacility->setText(tr(_rule->getType()));

	_txtCost->setText(tr("STR_COST_UC"));

	_numCost->setBig();
	_numCost->setText(Text::formatFunding(_origFac != 0 ? _game->getMod()->getTheBiggestRipOffEver() : _rule->getBuildCost()));

	_txtTime->setText(tr("STR_CONSTRUCTION_TIME_UC"));

	_numTime->setBig();
	_numTime->setText(tr("STR_DAY", _origFac != 0 ? 0 : _rule->getBuildTime()));

	_txtMaintenance->setText(tr("STR_MAINTENANCE_UC"));

	_numMaintenance->setBig();
	_numMaintenance->setText(Text::formatFunding(_rule->getMonthlyCost()));
}

/**
 *
 */
PlaceFacilityState::~PlaceFacilityState()
{

}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void PlaceFacilityState::btnCancelClick(Action *)
{
	_game->popState();
}

/**
 * Processes clicking on facilities.
 * @param action Pointer to an action.
 */
void PlaceFacilityState::viewClick(Action *)
{
	if (_origFac != 0)
	{
		// EXPERIMENTAL!: just moving an existing facility
		// FIXME: can lead to disconnected bases (easy to check, but would not be able to move the Access Lift)
		// FIXME: queued facilities' build time is not recalculated (easy to do)
		//        ... fixed moving cost per tile? or dynamic based on size, type, distance, etc.?)
		if (_view->getGridX() == _origFac->getX() && _view->getGridY() == _origFac->getY())
		{
			// unchanged location -> no message, no cost.
			_game->popState();
		}
		else if (!_view->isPlaceable(_rule, _origFac))
		{
			_game->pushState(new ErrorMessageState(tr("STR_CANNOT_BUILD_HERE"), _palette, _game->getMod()->getInterface("placeFacility")->getElement("errorMessage")->color, "BACK01.SCR", _game->getMod()->getInterface("placeFacility")->getElement("errorPalette")->color));
		}
		else if (_game->getSavedGame()->getFunds() < _game->getMod()->getTheBiggestRipOffEver())
		{
			_game->popState();
			_game->pushState(new ErrorMessageState(tr("STR_NOT_ENOUGH_MONEY"), _palette, _game->getMod()->getInterface("placeFacility")->getElement("errorMessage")->color, "BACK01.SCR", _game->getMod()->getInterface("placeFacility")->getElement("errorPalette")->color));
		}
		else
		{
			_origFac->setX(_view->getGridX());
			_origFac->setY(_view->getGridY());
			if (Options::allowBuildingQueue)
			{
				// first reset (maybe the moved facility is not queued anymore)
				if (abs(_origFac->getBuildTime()) > _rule->getBuildTime()) _origFac->setBuildTime(_rule->getBuildTime());
				// if it is still in the queue though, recalc
				if (_origFac->getBuildTime() > 0 && _view->isQueuedBuilding(_rule)) _origFac->setBuildTime(INT_MAX);
				_view->reCalcQueuedBuildings();
			}
			_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() - _game->getMod()->getTheBiggestRipOffEver());
			_game->popState();
		}
	}
	else
	{
		// placing a brand new facility
		if (!_view->isPlaceable(_rule))
		{
			_game->pushState(new ErrorMessageState(tr("STR_CANNOT_BUILD_HERE"), _palette, _game->getMod()->getInterface("placeFacility")->getElement("errorMessage")->color, "BACK01.SCR", _game->getMod()->getInterface("placeFacility")->getElement("errorPalette")->color));
		}
		else if (_base->isMaxAllowedLimitReached(_rule))
		{
			_game->popState();
			_game->pushState(new ErrorMessageState(tr("STR_CANNOT_BUILD_MORE_OF_THIS_FACILITY_TYPE_AT_BASE"), _palette, _game->getMod()->getInterface("placeFacility")->getElement("errorMessage")->color, "BACK01.SCR", _game->getMod()->getInterface("placeFacility")->getElement("errorPalette")->color));
		}
		else if (_game->getSavedGame()->getFunds() < _rule->getBuildCost())
		{
			_game->popState();
			_game->pushState(new ErrorMessageState(tr("STR_NOT_ENOUGH_MONEY"), _palette, _game->getMod()->getInterface("placeFacility")->getElement("errorMessage")->color, "BACK01.SCR", _game->getMod()->getInterface("placeFacility")->getElement("errorPalette")->color));
		}
		else
		{
			const std::map<std::string, std::pair<int, int> > &itemCost = _rule->getBuildCostItems();
			for (std::map<std::string, std::pair<int, int> >::const_iterator i = itemCost.begin(); i != itemCost.end(); ++i)
			{
				int needed = i->second.first - _base->getStorageItems()->getItem(i->first);
				if (needed > 0)
				{
					_game->popState();
					_game->pushState(new ErrorMessageState(tr("STR_NOT_ENOUGH_ITEMS").arg(tr(i->first)).arg(needed), _palette, _game->getMod()->getInterface("placeFacility")->getElement("errorMessage")->color, "BACK01.SCR", _game->getMod()->getInterface("placeFacility")->getElement("errorPalette")->color));
					return;
				}
			}
			BaseFacility *fac = new BaseFacility(_rule, _base);
			fac->setX(_view->getGridX());
			fac->setY(_view->getGridY());
			fac->setBuildTime(_rule->getBuildTime());
			_base->getFacilities()->push_back(fac);
			if (Options::allowBuildingQueue)
			{
				if (_view->isQueuedBuilding(_rule)) fac->setBuildTime(INT_MAX);
				_view->reCalcQueuedBuildings();
			}
			_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() - _rule->getBuildCost());
			for (std::map<std::string, std::pair<int, int> >::const_iterator i = itemCost.begin(); i != itemCost.end(); ++i)
			{
				_base->getStorageItems()->removeItem(i->first, i->second.first);
			}
			_game->popState();
		}
	}
}

}
