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
#include "CraftSoldiersState.h"
#include <climits>
#include <algorithm>
#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/ComboBox.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Menu/ErrorMessageState.h"
#include "../Savegame/Base.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Craft.h"
#include "../Savegame/SavedGame.h"
#include "SoldierInfoState.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleInterface.h"
#include "../Savegame/Role.h"
#include "SoldierSortUtil.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Craft Soldiers screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param craft ID of the selected craft.
 */
CraftSoldiersState::CraftSoldiersState(Base *base, size_t craft)
		:  _base(base), _craft(craft), _otherCraftColor(0), _origSoldierOrder(*_base->getSoldiers()), _dynGetter(NULL), _craftOut(false), _showStats(false)
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_btnOk = new TextButton(96, 16, 216, 176);
	_txtTitle = new Text(300, 17, 8, 7);
	_txtName = new Text(114, 9, 8, 32);
	_txtRank = new Text(102, 9, 120, 32);
	_txtCraft = new Text(84, 9, 210, 32);
	_txtAvailable = new Text(110, 9, 8, 24);
	_txtUsed = new Text(110, 9, 120, 24);
	_cbxSortBy = new ComboBox(this, 96, 16, 8, 176, true);
	_btnStats = new TextButton(96, 16, 112, 176);
	_btnRoles = new TextButton(96, 16, 112, 176);
	_lstSoldiers = new TextList(288, 120, 8, 40);

	_txtTU          = new TaggedText<int>(16, 9, 120, 32); //106
	_txtStamina     = new TaggedText<int>(16, 9, 136, 32); //124
	_txtHealth      = new TaggedText<int>(16, 9, 152, 32); //142
	_txtBravery     = new TaggedText<int>(16, 9, 168, 32); //160
	_txtReactions   = new TaggedText<int>(16, 9, 184, 32); //178
	_txtFiring      = new TaggedText<int>(16, 9, 200, 32); //196
	_txtThrowing    = new TaggedText<int>(16, 9, 216, 32); //214
	_txtMelee       = new TaggedText<int>(16, 9, 232, 32); //232
	_txtStrength    = new TaggedText<int>(16, 9, 248, 32); //250
	_txtPsiStrength = new TaggedText<int>(16, 9, 264, 32); //268
	_txtPsiSkill    = new TaggedText<int>(16, 9, 280, 32); //286..304 = 18

	_txtTooltip = new Text(200, 9, 8, 164);

	// Set palette
	setInterface("craftSoldiers");

	add(_window, "window", "craftSoldiers");
	add(_btnOk, "button", "craftSoldiers");
	add(_txtTitle, "text", "craftSoldiers");
	add(_txtName, "text", "craftSoldiers");
	add(_txtRank, "text", "craftSoldiers");
	add(_txtCraft, "text", "craftSoldiers");
	add(_txtAvailable, "text", "craftSoldiers");
	add(_txtUsed, "text", "craftSoldiers");
	add(_txtTU, "text", "craftSoldiers");
	add(_txtStamina, "text", "craftSoldiers");
	add(_txtHealth, "text", "craftSoldiers");
	add(_txtBravery, "text", "craftSoldiers");
	add(_txtReactions, "text", "craftSoldiers");
	add(_txtFiring, "text", "craftSoldiers");
	add(_txtThrowing, "text", "craftSoldiers");
	add(_txtMelee, "text", "craftSoldiers");
	add(_txtStrength, "text", "craftSoldiers");
	add(_txtPsiStrength, "text", "craftSoldiers");
	add(_txtPsiSkill, "text", "craftSoldiers");
	add(_txtTooltip, "text", "craftSoldiers");
	add(_lstSoldiers, "list", "craftSoldiers");
	add(_btnStats, "button", "soldierList");
	add(_btnRoles, "button", "soldierList");
	add(_cbxSortBy, "button", "craftSoldiers");

	_otherCraftColor = _game->getMod()->getInterface("craftSoldiers")->getElement("otherCraft")->color;

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK02.SCR"));

	_btnStats->setText(tr("STR_SHOW_STATS"));
	_btnStats->onMouseClick((ActionHandler)&CraftSoldiersState::btnToggleStatsClick);

	_btnRoles->setText(tr("STR_SHOW_ROLES"));
	_btnRoles->onMouseClick((ActionHandler)&CraftSoldiersState::btnToggleStatsClick);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&CraftSoldiersState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&CraftSoldiersState::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&CraftSoldiersState::btnDeassignAllSoldiersClick, Options::keyResetAll);

	_txtTitle->setBig();
	Craft *c = _base->getCrafts()->at(_craft);
	_craftOut = c->getStatus() == "STR_OUT";
	_txtTitle->setText(tr("STR_SELECT_SQUAD_FOR_CRAFT").arg(c->getName(_game->getLanguage())));

	_txtName->setText(tr("STR_NAME_UC"));

	_txtRank->setText(tr("STR_RANK"));

	_txtCraft->setText(tr("STR_CRAFT"));

	// populate sort options
	std::vector<std::wstring> sortOptions;
	sortOptions.push_back(tr("STR_ORIGINAL_ORDER"));
	_sortFunctors.push_back(NULL);

#define PUSH_IN(strId, functor, asc, txt) \
	if(txt) { ((TaggedText<int>*)txt)->setTag(sortOptions.size()); ((TaggedText<int>*)txt)->onMouseClick((ActionHandler)&CraftSoldiersState::txtColumnHeaderClick); } \
	sortOptions.push_back(tr(strId)); \
	_sortFunctors.push_back(new SortFunctor(_game, functor, asc));

	PUSH_IN("STR_ID", idStat, true, (void*)0);
	PUSH_IN("STR_FIRST_LETTER", nameStat, true, (void*)0);
	PUSH_IN("STR_RANK", rankStat, false, (void*)0);
	PUSH_IN("STR_MISSIONS2", missionsStat, false, (void*)0);
	PUSH_IN("STR_KILLS2", killsStat, false, (void*)0);
	PUSH_IN("STR_WOUND_RECOVERY2", woundRecoveryStat, false, (void*)0);
	PUSH_IN("STR_TOTAL_STATS", totalStatsStat, false, (void*)0);
	PUSH_IN("STR_TIME_UNITS", tuStat, false, _txtTU);
	PUSH_IN("STR_STAMINA", staminaStat, false, _txtStamina);
	PUSH_IN("STR_HEALTH", healthStat, false, _txtHealth);
	PUSH_IN("STR_BRAVERY", braveryStat, false, _txtBravery);
	PUSH_IN("STR_REACTIONS", reactionsStat, false, _txtReactions);
	PUSH_IN("STR_FIRING_ACCURACY", firingStat, false, _txtFiring);
	PUSH_IN("STR_THROWING_ACCURACY", throwingStat, false, _txtThrowing);
	PUSH_IN("STR_MELEE_ACCURACY", meleeStat, false, _txtMelee);
	PUSH_IN("STR_STRENGTH", strengthStat, false, _txtStrength);

	// don't show psionic sort options until they actually have data they can use
	_showPsi = Options::psiStrengthEval
		&& _game->getSavedGame()->isResearched(_game->getMod()->getPsiRequirements());
	bool showPsiSkill = false;
	for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		if (showPsiSkill) { break; }
		if ((*i)->getCurrentStats()->psiSkill > 0)
		{
			showPsiSkill = true;
		}
	}
	if (_showPsi)
	{
		PUSH_IN("STR_PSIONIC_STRENGTH", psiStrengthStat, false, _txtPsiStrength);
	}
	else
	{
		_txtPsiStrength->setVisible(false);
	}
	if (_showPsi && _showPsi)
	{
		PUSH_IN("STR_PSIONIC_SKILL", psiSkillStat, false, _txtPsiSkill);
	}
	else
	{
		_txtPsiSkill->setVisible(false);
	}

#undef PUSH_IN

	/*_lstSoldiers->setArrowColumn(192, ARROW_VERTICAL);
	_lstSoldiers->setColumns(4, 106, 86, 72, 16);
	_lstSoldiers->setAlign(ALIGN_RIGHT, 3);*/
	_lstSoldiers->setSelectable(true);
	_lstSoldiers->setBackground(_window);

	_lstSoldiers->onLeftArrowClick((ActionHandler)&CraftSoldiersState::lstItemsLeftArrowClick);
	_lstSoldiers->onRightArrowClick((ActionHandler)&CraftSoldiersState::lstItemsRightArrowClick);
	_lstSoldiers->onMouseClick((ActionHandler)&CraftSoldiersState::lstSoldiersClick, 0);
	_lstSoldiers->onMousePress((ActionHandler)&CraftSoldiersState::lstSoldiersMousePress);

	_cbxSortBy->setOptions(sortOptions);
	_cbxSortBy->setSelected(0);
	_cbxSortBy->onChange((ActionHandler)&CraftSoldiersState::cbxSortByChange);
	_cbxSortBy->setText(tr("STR_SORT_BY"));

	_txtTU->setAlign(ALIGN_CENTER);
	_txtTU->setText(tr("STR_TIME_UNITS_ABBREVIATION"));
	_txtTU->setTooltip("STR_TIME_UNITS");
	_txtTU->onMouseIn((ActionHandler)&CraftSoldiersState::txtTooltipIn);
	_txtTU->onMouseOut((ActionHandler)&CraftSoldiersState::txtTooltipOut);

	_txtStamina->setAlign(ALIGN_CENTER);
	_txtStamina->setText(tr("STR_STAMINA_ABBREVIATION"));
	_txtStamina->setTooltip("STR_STAMINA");
	_txtStamina->onMouseIn((ActionHandler)&CraftSoldiersState::txtTooltipIn);
	_txtStamina->onMouseOut((ActionHandler)&CraftSoldiersState::txtTooltipOut);

	_txtHealth->setAlign(ALIGN_CENTER);
	_txtHealth->setText(tr("STR_HEALTH_ABBREVIATION"));
	_txtHealth->setTooltip("STR_HEALTH");
	_txtHealth->onMouseIn((ActionHandler)&CraftSoldiersState::txtTooltipIn);
	_txtHealth->onMouseOut((ActionHandler)&CraftSoldiersState::txtTooltipOut);

	_txtBravery->setAlign(ALIGN_CENTER);
	_txtBravery->setText(tr("STR_BRAVERY_ABBREVIATION"));
	_txtBravery->setTooltip("STR_BRAVERY");
	_txtBravery->onMouseIn((ActionHandler)&CraftSoldiersState::txtTooltipIn);
	_txtBravery->onMouseOut((ActionHandler)&CraftSoldiersState::txtTooltipOut);

	_txtReactions->setAlign(ALIGN_CENTER);
	_txtReactions->setText(tr("STR_REACTIONS_ABBREVIATION"));
	_txtReactions->setTooltip("STR_REACTIONS");
	_txtReactions->onMouseIn((ActionHandler)&CraftSoldiersState::txtTooltipIn);
	_txtReactions->onMouseOut((ActionHandler)&CraftSoldiersState::txtTooltipOut);

	_txtFiring->setAlign(ALIGN_CENTER);
	_txtFiring->setText(tr("STR_FIRING_ACCURACY_ABBREVIATION"));
	_txtFiring->setTooltip("STR_FIRING_ACCURACY");
	_txtFiring->onMouseIn((ActionHandler)&CraftSoldiersState::txtTooltipIn);
	_txtFiring->onMouseOut((ActionHandler)&CraftSoldiersState::txtTooltipOut);

	_txtThrowing->setAlign(ALIGN_CENTER);
	_txtThrowing->setText(tr("STR_THROWING_ACCURACY_ABBREVIATION"));
	_txtThrowing->setTooltip("STR_THROWING_ACCURACY");
	_txtThrowing->onMouseIn((ActionHandler)&CraftSoldiersState::txtTooltipIn);
	_txtThrowing->onMouseOut((ActionHandler)&CraftSoldiersState::txtTooltipOut);

	_txtMelee->setAlign(ALIGN_CENTER);
	_txtMelee->setText(tr("STR_MELEE_ACCURACY_ABBREVIATION"));
	_txtMelee->setTooltip("STR_MELEE_ACCURACY");
	_txtMelee->onMouseIn((ActionHandler)&CraftSoldiersState::txtTooltipIn);
	_txtMelee->onMouseOut((ActionHandler)&CraftSoldiersState::txtTooltipOut);

	_txtStrength->setAlign(ALIGN_CENTER);
	_txtStrength->setText(tr("STR_STRENGTH_ABBREVIATION"));
	_txtStrength->setTooltip("STR_STRENGTH");
	_txtStrength->onMouseIn((ActionHandler)&CraftSoldiersState::txtTooltipIn);
	_txtStrength->onMouseOut((ActionHandler)&CraftSoldiersState::txtTooltipOut);

	_txtPsiStrength->setAlign(ALIGN_CENTER);
	_txtPsiStrength->setText(tr("STR_PSIONIC_STRENGTH_ABBREVIATION"));
	_txtPsiStrength->setTooltip("STR_PSIONIC_STRENGTH");
	_txtPsiStrength->onMouseIn((ActionHandler)&CraftSoldiersState::txtTooltipIn);
	_txtPsiStrength->onMouseOut((ActionHandler)&CraftSoldiersState::txtTooltipOut);

	_txtPsiSkill->setAlign(ALIGN_CENTER);
	_txtPsiSkill->setText(tr("STR_PSIONIC_SKILL_ABBREVIATION"));
	_txtPsiSkill->setTooltip("STR_PSIONIC_SKILL");
	_txtPsiSkill->onMouseIn((ActionHandler)&CraftSoldiersState::txtTooltipIn);
	_txtPsiSkill->onMouseOut((ActionHandler)&CraftSoldiersState::txtTooltipOut);
}

/**
 * cleans up dynamic state
 */
CraftSoldiersState::~CraftSoldiersState()
{
	for (std::vector<SortFunctor *>::iterator it = _sortFunctors.begin();
		it != _sortFunctors.end(); ++it)
	{
		delete(*it);
	}
}

/**
 * Sorts the soldiers list by the selected criterion
 * @param action Pointer to an action.
 */
void CraftSoldiersState::cbxSortByChange(Action *)
{
	bool ctrlPressed = SDL_GetModState() & KMOD_CTRL;
	size_t selIdx = _cbxSortBy->getSelected();
	if (selIdx == (size_t)-1)
	{
		return;
	}

	SortFunctor *compFunc = _sortFunctors[selIdx];
	if (compFunc)
	{
		// if CTRL is pressed, we only want to show the dynamic column, without actual sorting
		if (!ctrlPressed)
		{
			std::stable_sort(_base->getSoldiers()->begin(), _base->getSoldiers()->end(), *compFunc);
			bool shiftPressed = SDL_GetModState() & KMOD_SHIFT;
			if (shiftPressed)
			{
				std::reverse(_base->getSoldiers()->begin(), _base->getSoldiers()->end());
			}
		}
	}
	else
	{
		_dynGetter = NULL;
		// restore original ordering, ignoring (of course) those
		// soldiers that have been sacked since this state started
		for (std::vector<Soldier *>::const_iterator it = _origSoldierOrder.begin();
			it != _origSoldierOrder.end(); ++it)
		{
			std::vector<Soldier *>::iterator soldierIt =
				std::find(_base->getSoldiers()->begin(), _base->getSoldiers()->end(), *it);
			if (soldierIt != _base->getSoldiers()->end())
			{
				Soldier *s = *soldierIt;
				_base->getSoldiers()->erase(soldierIt);
				_base->getSoldiers()->insert(_base->getSoldiers()->end(), s);
			}
		}
	}

	size_t originalScrollPos = _lstSoldiers->getScroll();
	if (compFunc)
	{
		_dynGetter = compFunc->getGetter();
	}
	initList(originalScrollPos);
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void CraftSoldiersState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Shows the soldiers in a list at specified offset/scroll.
 */
void CraftSoldiersState::initList(size_t scrl)
{
	if (_showStats)
	{
		_lstSoldiers->setColumns(13, 112, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 0);
		_lstSoldiers->setArrowColumn(88, ARROW_VERTICAL);
		_lstSoldiers->setAlign(ALIGN_CENTER);
		_lstSoldiers->setAlign(ALIGN_LEFT, 0);
		_btnStats->setVisible(false);
		_btnRoles->setVisible(true);
	}
	else
	{
		_lstSoldiers->setColumns(4, 112, 90, 70, 16);
		_lstSoldiers->setArrowColumn(176, ARROW_VERTICAL);
		_lstSoldiers->setAlign(ALIGN_LEFT);
		_lstSoldiers->setAlign(ALIGN_RIGHT, 3);
		_btnStats->setVisible(true);
		_btnRoles->setVisible(false);
	}

	_txtRank->setVisible(!_showStats);
	_txtCraft->setVisible(!_showStats);
	_txtTU->setVisible(_showStats);
	_txtStamina->setVisible(_showStats);
	_txtHealth->setVisible(_showStats);
	_txtBravery->setVisible(_showStats);
	_txtReactions->setVisible(_showStats);
	_txtFiring->setVisible(_showStats);
	_txtThrowing->setVisible(_showStats);
	_txtMelee->setVisible(_showStats);
	_txtStrength->setVisible(_showStats);
	_txtPsiStrength->setVisible(_showStats);
	_txtPsiSkill->setVisible(_showStats);
	_txtTooltip->setVisible(_showStats);

	size_t originalScrollPos = _lstSoldiers->getScroll();
	int row = 0;
	_lstSoldiers->clearList();
	Craft *c = _base->getCrafts()->at(_craft);
	float absBonus = _base->getSickBayAbsoluteBonus();
	float relBonus = _base->getSickBayRelativeBonus();
	for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		if (_showStats)
		{
			UnitStats *stats = (*i)->getCurrentStats();

			const wchar_t* ps;
			const wchar_t* pk;

			if (_showPsi && stats->psiSkill > 0)
			{
				ps = Text::formatNumber(stats->psiStrength).c_str();
				pk = Text::formatNumber(stats->psiSkill).c_str();
			}
			else
			{
				ps = L"-";
				pk = L"-";
			}

			_lstSoldiers->addRow(13, (*i)->getName().c_str(),
				Text::formatNumber(stats->tu).c_str(),
				Text::formatNumber(stats->stamina).c_str(),
				Text::formatNumber(stats->health).c_str(),
				Text::formatNumber(stats->bravery).c_str(),
				Text::formatNumber(stats->reactions).c_str(),
				Text::formatNumber(stats->firing).c_str(),
				Text::formatNumber(stats->throwing).c_str(),
				Text::formatNumber(stats->melee).c_str(),
				Text::formatNumber(stats->strength).c_str(),
				ps,
				pk,
				L"");
		}
		else
		{
			std::wostringstream rank;

			Role *role;
			if ((role = (*i)->getRole()) && !role->isBlank())
			{
				rank << tr(role->getName() + "_SHORT") << "-";
			}

			rank << tr((*i)->getRankString());

			// call corresponding getter
			int dynStat = 0;
			std::wostringstream ss;
			if (_dynGetter != NULL) {
				dynStat = (*_dynGetter)(_game, *i);
				ss << dynStat;
			} else {
				ss << L"";
			}

			_lstSoldiers->addRow(4,
				(*i)->getName(true, 19).c_str(),
				rank.str().c_str(),
				(*i)->getCraftString(_game->getLanguage(), absBonus, relBonus).c_str(),
				ss.str().c_str());
		}

		Uint8 color;
		if ((*i)->getCraft() == c)
		{
			color = _lstSoldiers->getSecondaryColor();
		}
		else if ((*i)->getCraft() != 0)
		{
			color = _otherCraftColor;
		}
		else
		{
			color = _lstSoldiers->getColor();
		}
		_lstSoldiers->setRowColor(row, color);
		row++;
	}
	if (scrl)
		_lstSoldiers->scrollTo(scrl);
	_lstSoldiers->draw();

	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(c->getSpaceAvailable()));
	_txtUsed->setText(tr("STR_SPACE_USED").arg(c->getSpaceUsed()));
}

/**
 * Shows the soldiers in a list.
 */
void CraftSoldiersState::init()
{
	State::init();
	initList(0);

}

/**
 * Reorders a soldier up.
 * @param action Pointer to an action.
 */
void CraftSoldiersState::lstItemsLeftArrowClick(Action *action)
{
	unsigned int row = _lstSoldiers->getSelectedRow();
	if (row > 0)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			moveSoldierUp(action, row);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		{
			moveSoldierUp(action, row, true);
		}
	}
	_cbxSortBy->setText(tr("STR_SORT_BY"));
	_cbxSortBy->setSelected(-1);
}

/**
 * Moves a soldier up on the list.
 * @param action Pointer to an action.
 * @param row Selected soldier row.
 * @param max Move the soldier to the top?
 */
void CraftSoldiersState::moveSoldierUp(Action *action, unsigned int row, bool max)
{
	Soldier *s = _base->getSoldiers()->at(row);
	if (max)
	{
		_base->getSoldiers()->erase(_base->getSoldiers()->begin() + row);
		_base->getSoldiers()->insert(_base->getSoldiers()->begin(), s);
	}
	else
	{
		_base->getSoldiers()->at(row) = _base->getSoldiers()->at(row - 1);
		_base->getSoldiers()->at(row - 1) = s;
		if (row != _lstSoldiers->getScroll())
		{
			SDL_WarpMouse(action->getLeftBlackBand() + action->getXMouse(), action->getTopBlackBand() + action->getYMouse() - static_cast<Uint16>(8 * action->getYScale()));
		}
		else
		{
			_lstSoldiers->scrollUp(false);
		}
	}
	initList(_lstSoldiers->getScroll());
}

/**
 * Reorders a soldier down.
 * @param action Pointer to an action.
 */
void CraftSoldiersState::lstItemsRightArrowClick(Action *action)
{
	unsigned int row = _lstSoldiers->getSelectedRow();
	size_t numSoldiers = _base->getSoldiers()->size();
	if (0 < numSoldiers && INT_MAX >= numSoldiers && row < numSoldiers - 1)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			moveSoldierDown(action, row);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		{
			moveSoldierDown(action, row, true);
		}
	}
	_cbxSortBy->setText(tr("STR_SORT_BY"));
	_cbxSortBy->setSelected(-1);
}

/**
 * Moves a soldier down on the list.
 * @param action Pointer to an action.
 * @param row Selected soldier row.
 * @param max Move the soldier to the bottom?
 */
void CraftSoldiersState::moveSoldierDown(Action *action, unsigned int row, bool max)
{
	Soldier *s = _base->getSoldiers()->at(row);
	if (max)
	{
		_base->getSoldiers()->erase(_base->getSoldiers()->begin() + row);
		_base->getSoldiers()->insert(_base->getSoldiers()->end(), s);
	}
	else
	{
		_base->getSoldiers()->at(row) = _base->getSoldiers()->at(row + 1);
		_base->getSoldiers()->at(row + 1) = s;
		if (row != _lstSoldiers->getVisibleRows() - 1 + _lstSoldiers->getScroll())
		{
			SDL_WarpMouse(action->getLeftBlackBand() + action->getXMouse(), action->getTopBlackBand() + action->getYMouse() + static_cast<Uint16>(8 * action->getYScale()));
		}
		else
		{
			_lstSoldiers->scrollDown(false);
		}
	}
	initList(_lstSoldiers->getScroll());
}

/**
 * Shows the selected soldier's info.
 * @param action Pointer to an action.
 */
void CraftSoldiersState::lstSoldiersClick(Action *action)
{
	double mx = action->getAbsoluteXMouse();
	if (mx >= _lstSoldiers->getArrowsLeftEdge() && mx < _lstSoldiers->getArrowsRightEdge())
	{
		return;
	}
	int row = _lstSoldiers->getSelectedRow();
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		Craft *c = _base->getCrafts()->at(_craft);
		Soldier *s = _base->getSoldiers()->at(_lstSoldiers->getSelectedRow());
		Uint8 color = _lstSoldiers->getColor();
		if (s->getCraft() == c)
		{
			s->setCraft(0);
			if (!_showStats)
			{
				_lstSoldiers->setCellText(row, 2, tr("STR_NONE_UC"));
			}
		}
		else if (s->getCraft() && s->getCraft()->getStatus() == "STR_OUT")
		{
			color = _otherCraftColor;
		}
		else if (s->hasFullHealth())
		{
			auto space = c->getSpaceAvailable();
			auto armorSize = s->getArmor()->getSize();
			if (space >= s->getArmor()->getTotalSize() && (armorSize == 1 || (c->getNumVehicles() < c->getRules()->getVehicles())))
			{
				s->setCraft(c);
				if (!_showStats)
				{
					_lstSoldiers->setCellText(row, 2, c->getName(_game->getLanguage()));
				}
				color = _lstSoldiers->getSecondaryColor();
			}
			else if (armorSize == 2 && space > 0)
			{
				_game->pushState(new ErrorMessageState(tr("STR_NOT_ENOUGH_CRAFT_SPACE"), _palette, _game->getMod()->getInterface("soldierInfo")->getElement("errorMessage")->color, "BACK01.SCR", _game->getMod()->getInterface("soldierInfo")->getElement("errorPalette")->color));
			}
		}
		_lstSoldiers->setRowColor(row, color);

		_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(c->getSpaceAvailable()));
		_txtUsed->setText(tr("STR_SPACE_USED").arg(c->getSpaceUsed()));
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_game->pushState(new SoldierInfoState(_base, row));
	}
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action Pointer to an action.
 */
void CraftSoldiersState::lstSoldiersMousePress(Action *action)
{
	if (Options::changeValueByMouseWheel == 0)
		return;
	unsigned int row = _lstSoldiers->getSelectedRow();
	size_t numSoldiers = _base->getSoldiers()->size();
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP &&
		row > 0)
	{
		if (action->getAbsoluteXMouse() >= _lstSoldiers->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstSoldiers->getArrowsRightEdge())
		{
			moveSoldierUp(action, row);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN &&
		0 < numSoldiers && INT_MAX >= numSoldiers && row < numSoldiers - 1)
	{
		if (action->getAbsoluteXMouse() >= _lstSoldiers->getArrowsLeftEdge() &&
			action->getAbsoluteXMouse() <= _lstSoldiers->getArrowsRightEdge())
		{
			moveSoldierDown(action, row);
		}
	}
}

/**
 * De-assign all soldiers from all craft located in the base (i.e. not out on a mission).
 * @param action Pointer to an action.
 */
void CraftSoldiersState::btnDeassignAllSoldiersClick(Action *action)
{
	Uint8 color = _lstSoldiers->getColor();

	int row = 0;
	for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		color = _lstSoldiers->getColor();
		if ((*i)->getCraft() && (*i)->getCraft()->getStatus() != "STR_OUT")
		{
			(*i)->setCraft(0);
			_lstSoldiers->setCellText(row, 2, tr("STR_NONE_UC"));
		}
		else if ((*i)->getCraft() && (*i)->getCraft()->getStatus() == "STR_OUT")
		{
			color = _otherCraftColor;
		}
		_lstSoldiers->setRowColor(row, color);

		row++;
	}

	Craft *c = _base->getCrafts()->at(_craft);
	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(c->getSpaceAvailable()));
	_txtUsed->setText(tr("STR_SPACE_USED").arg(c->getSpaceUsed()));
}

/**
 * Handler for sortable column headers.
 * @param action Pointer to an action.
 */
void CraftSoldiersState::txtColumnHeaderClick(Action *action)
{
	TaggedText<int> *txt = dynamic_cast<TaggedText<int>*>(action->getSender());

	if (txt)
	{
		if (_cbxSortBy->getSelected() == txt->getTag())
		{
			_cbxSortBy->setSelected(0);
			_cbxSortBy->setText(tr("STR_SORT_BY"));
		}
		else
		{
			_cbxSortBy->setSelected(txt->getTag());
		}

		cbxSortByChange(action);
	}
}

/**
* Shows a tooltip for the appropriate text.
* @param action Pointer to an action.
*/
void CraftSoldiersState::txtTooltipIn(Action *action)
{
	_currentTooltip = action->getSender()->getTooltip();
	_txtTooltip->setText(tr(_currentTooltip));
}

/**
* Clears the tooltip text.
* @param action Pointer to an action.
*/
void CraftSoldiersState::txtTooltipOut(Action *action)
{
	if (_currentTooltip == action->getSender()->getTooltip())
	{
		_txtTooltip->setText(L"");
	}
}

/**
* Toggles the stats/roles list display.
* @param action Pointer to an action.
*/
void CraftSoldiersState::btnToggleStatsClick(Action *action)
{
	_showStats = !_showStats;
	initList(_lstSoldiers->getScroll());
}

}
