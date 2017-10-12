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
#include "CraftErrorState.h"
#include "CraftNotEnoughPilotsState.h"
#include "ConfirmDestinationState.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Mod/AlienRace.h"
#include "../Mod/RuleStartingCondition.h"
#include "../Mod/AlienDeployment.h"
#include "../Mod/ArticleDefinition.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Target.h"
#include "../Savegame/Waypoint.h"
#include "../Savegame/Base.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/AlienBase.h"
#include "../Engine/Options.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Confirm Destination window.
 * @param game Pointer to the core game.
 * @param craft Pointer to the craft to retarget.
 * @param target Pointer to the selected target (NULL if it's just a point on the globe).
 */
ConfirmDestinationState::ConfirmDestinationState(Craft *craft, Target *target) : _craft(craft), _target(target)
{
	Waypoint *w = dynamic_cast<Waypoint*>(_target);
	_screen = false;

	// Create objects
	_window = new Window(this, 244, 72, 6, 64);
	_btnOk = new TextButton(50, 12, 68, 104);
	_btnCancel = new TextButton(50, 12, 138, 104);
	_txtTarget = new Text(232, 32, 12, 72);

	// Set palette
	setInterface("confirmDestination", w != 0 && w->getId() == 0);

	add(_window, "window", "confirmDestination");
	add(_btnOk, "button", "confirmDestination");
	add(_btnCancel, "button", "confirmDestination");
	add(_txtTarget, "text", "confirmDestination");

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK12.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&ConfirmDestinationState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&ConfirmDestinationState::btnOkClick, Options::keyOk);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&ConfirmDestinationState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&ConfirmDestinationState::btnCancelClick, Options::keyCancel);

	_txtTarget->setBig();
	_txtTarget->setAlign(ALIGN_CENTER);
	_txtTarget->setVerticalAlign(ALIGN_MIDDLE);
	_txtTarget->setWordWrap(true);
	if (w != 0 && w->getId() == 0)
	{
		_txtTarget->setText(tr("STR_TARGET").arg(tr("STR_WAY_POINT")));
	}
	else
	{
		_txtTarget->setText(tr("STR_TARGET").arg(_target->getName(_game->getLanguage())));
	}
}

/**
 *
 */
ConfirmDestinationState::~ConfirmDestinationState()
{

}

/**
* Checks the starting condition.
*/
std::wstring ConfirmDestinationState::checkStartingCondition()
{
	Ufo* u = dynamic_cast<Ufo*>(_target);
	MissionSite* m = dynamic_cast<MissionSite*>(_target);
	AlienBase* b = dynamic_cast<AlienBase*>(_target);

	AlienDeployment *ruleDeploy = 0;
	if (u != 0)
	{
		ruleDeploy = _game->getMod()->getDeployment(u->getRules()->getType());
	}
	else if (m != 0)
	{
		ruleDeploy = _game->getMod()->getDeployment(m->getDeployment()->getType());
	}
	else if (b != 0)
	{
		AlienRace *race = _game->getMod()->getAlienRace(b->getAlienRace());
		ruleDeploy = _game->getMod()->getDeployment(race->getBaseCustomMission());
		if (!ruleDeploy) ruleDeploy = _game->getMod()->getDeployment(b->getDeployment()->getType());
	}
	else
	{
		// for example just a waypoint
		return L"";
	}

	if (ruleDeploy == 0)
	{
		// e.g. UFOs without alien deployment :(
		return L"";
	}

	RuleStartingCondition *rule = _game->getMod()->getStartingCondition(ruleDeploy->getStartingCondition());
	if (rule == 0)
	{
		// rule doesn't exist (mod upgrades?)
		return L"";
	}

	if (rule->isCraftAllowed(_craft->getRules()->getType()))
	{
		// craft is allowed
		return L"";
	}

	// craft is not allowed
	const std::vector<std::string> *list = rule->getAllowedCraft();
	std::wostringstream ss;
	int i = 0;
	for (std::vector<std::string>::const_iterator it = list->begin(); it != list->end(); ++it)
	{
		ArticleDefinition *article = _game->getMod()->getUfopaediaArticle((*it), false);
		if (article && _game->getSavedGame()->isResearched(article->requires))
		{
			if (i > 0)
				ss << L", ";
			ss << tr(*it);
			i++;
		}
	}
	std::wstring message = ss.str();
	if (message.empty())
	{
		// no suitable craft yet
		return tr("STR_UNKNOWN");
	}
	return ss.str();
}

/**
 * Confirms the selected target for the craft.
 * @param action Pointer to an action.
 */
void ConfirmDestinationState::btnOkClick(Action *)
{
	std::wstring message = checkStartingCondition();
	if (!message.empty())
	{
		_game->popState();
		_game->popState();
		_game->pushState(new CraftErrorState(0, tr("STR_STARTING_CONDITION_CRAFT").arg(message)));
		return;
	}

	if (!_craft->arePilotsOnboard())
	{
		_game->popState();
		_game->popState();
		_game->pushState(new CraftNotEnoughPilotsState(_craft));
		return;
	}

	Waypoint *w = dynamic_cast<Waypoint*>(_target);
	if (w != 0 && w->getId() == 0)
	{
		w->setId(_game->getSavedGame()->getId("STR_WAYPOINT"));
		_game->getSavedGame()->getWaypoints()->push_back(w);
	}
	_craft->setDestination(_target);
	if (_craft->getRules()->canAutoPatrol())
	{
		// cancel auto-patrol
		_craft->setIsAutoPatrolling(false);
	}
	_craft->setStatus("STR_OUT");
	if (_craft->getInterceptionOrder() == 0)
	{
		int maxInterceptionOrder = 0;
		for (std::vector<Base*>::iterator baseIt = _game->getSavedGame()->getBases()->begin(); baseIt != _game->getSavedGame()->getBases()->end(); ++baseIt)
		{
			for (std::vector<Craft*>::iterator craftIt = (*baseIt)->getCrafts()->begin(); craftIt != (*baseIt)->getCrafts()->end(); ++craftIt)
			{
				if ((*craftIt)->getInterceptionOrder() > maxInterceptionOrder)
				{
					maxInterceptionOrder = (*craftIt)->getInterceptionOrder();
				}
			}
		}
		_craft->setInterceptionOrder(++maxInterceptionOrder);
	}
	_game->popState();
	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void ConfirmDestinationState::btnCancelClick(Action *)
{
	Waypoint *w = dynamic_cast<Waypoint*>(_target);
	if (w != 0 && w->getId() == 0)
	{
		delete w;
	}
	_game->popState();
}

}
