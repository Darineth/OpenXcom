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
#include "SoldiersState.h"
#include <climits>
#include "../Engine/Screen.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/ComboBox.h"
#include "../Engine/Action.h"
#include "../Geoscape/AllocatePsiTrainingState.h"
#include "../Geoscape/AllocateTrainingState.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/Base.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Role.h"
#include "../Mod/RuleCraft.h"
#include "SoldierInfoState.h"
#include "SoldierMemorialState.h"
#include "../Battlescape/InventoryState.h"
#include "../Battlescape/BattlescapeGenerator.h"
#include "../Savegame/SavedBattleGame.h"
#include <algorithm>
#include "../Mod/Unit.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldiers screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
SoldiersState::SoldiersState(Base *base) : _base(base), _origSoldierOrder(*_base->getSoldiers()), _dynGetter(NULL), _showStats(false)
{
	bool isPsiBtnVisible = Options::anytimePsiTraining && _base->getAvailablePsiLabs() > 0;
	bool isTrnBtnVisible = Options::anytimeMartialTraining && _base->getAvailableTraining() > 0;

	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	if (isPsiBtnVisible)
	{
		if (isTrnBtnVisible)
		{
			_btnOk = new TextButton(70, 16, 242, 176);
			_btnPsiTraining = new TextButton(70, 16, 164, 176);
			_btnTraining = new TextButton(70, 16, 86, 176);
			_btnMemorial = new TextButton(70, 16, 8, 176);
		}
		else
		{
			_btnOk = new TextButton(96, 16, 216, 176);
			_btnPsiTraining = new TextButton(96, 16, 112, 176);
			_btnTraining = new TextButton(96, 16, 112, 176);
			_btnMemorial = new TextButton(96, 16, 8, 176);
		}
	}
	else if (isTrnBtnVisible)
	{
		_btnOk = new TextButton(96, 16, 216, 176);
		_btnPsiTraining = new TextButton(96, 16, 112, 176);
		_btnTraining = new TextButton(96, 16, 112, 176);
		_btnMemorial = new TextButton(96, 16, 8, 176);
	}
	else
	{
		_btnOk = new TextButton(148, 16, 164, 176);
		_btnPsiTraining = new TextButton(148, 16, 164, 176);
		_btnTraining = new TextButton(96, 16, 112, 176);
		_btnMemorial = new TextButton(148, 16, 8, 176);
	}
	_txtTitle = new Text(168, 17, 8, 8);

	_btnStats = new TextButton(75, 16, 115, 8);
	_btnRoles = new TextButton(75, 16, 115, 8);
	_cbxSortBy = new ComboBox(this, 120, 16, 192, 8, false);

	_txtName = new Text(114, 9, 8, 26);
	_txtRank = new Text(102, 9, 120, 26);
	_txtCraft = new Text(82, 9, 210, 26);
	_lstSoldiers = new TextList(288, 128, 8, 34);

	//90, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18
	//_txtSoldier     = new Text(90, 9,  16, 24); //16..106 = 90
	_txtTU          = new TaggedText<int>(16, 9, 120, 26); //106
	_txtStamina     = new TaggedText<int>(16, 9, 136, 26); //124
	_txtHealth      = new TaggedText<int>(16, 9, 152, 26); //142
	_txtBravery     = new TaggedText<int>(16, 9, 168, 26); //160
	_txtReactions   = new TaggedText<int>(16, 9, 184, 26); //178
	_txtFiring      = new TaggedText<int>(16, 9, 200, 26); //196
	_txtThrowing    = new TaggedText<int>(16, 9, 216, 26); //214
	_txtMelee       = new TaggedText<int>(16, 9, 232, 26); //232
	_txtStrength    = new TaggedText<int>(16, 9, 248, 26); //250
	_txtPsiStrength = new TaggedText<int>(16, 9, 264, 26); //268
	_txtPsiSkill    = new TaggedText<int>(16, 9, 280, 26); //286..304 = 18

	//_lstSoldierStats = new TextList(288, 128, 16, 32);
	_txtTooltip = new Text(200, 9, 8, 164);

	// Set palette
	setInterface("soldierList");

	add(_window, "window", "soldierList");
	add(_btnOk, "button", "soldierList");
	add(_btnPsiTraining, "button", "soldierList");
	add(_btnTraining, "button", "soldierList");
	add(_btnMemorial, "button", "soldierList");
	add(_txtTitle, "text1", "soldierList");
	add(_btnStats, "button", "soldierList");
	add(_btnRoles, "button", "soldierList");
	add(_txtName, "text2", "soldierList");
	add(_txtRank, "text2", "soldierList");
	add(_txtCraft, "text2", "soldierList");
	add(_lstSoldiers, "list", "soldierList");

	//add(_txtSoldier, "text2", "soldierList");
	add(_txtTU, "text2", "soldierList");
	add(_txtStamina, "text2", "soldierList");
	add(_txtHealth, "text2", "soldierList");
	add(_txtBravery, "text2", "soldierList");
	add(_txtReactions, "text2", "soldierList");
	add(_txtFiring, "text2", "soldierList");
	add(_txtThrowing, "text2", "soldierList");
	add(_txtMelee, "text2", "soldierList");
	add(_txtStrength, "text2", "soldierList");
	add(_txtPsiStrength, "text2", "soldierList");
	add(_txtPsiSkill, "text2", "soldierList");
	//add(_lstSoldierStats, "list", "soldierList");
	add(_txtTooltip, "text2", "soldierList");

	add(_cbxSortBy, "button", "soldierList");

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK02.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&SoldiersState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&SoldiersState::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&SoldiersState::btnInventoryClick, Options::keyBattleInventory);

	_btnStats->setText(tr("STR_SHOW_STATS"));
	_btnStats->onMouseClick((ActionHandler)&SoldiersState::btnToggleStatsClick);

	_btnRoles->setText(tr("STR_SHOW_ROLES"));
	_btnRoles->onMouseClick((ActionHandler)&SoldiersState::btnToggleStatsClick);

	_btnPsiTraining->setText(tr("STR_PSI_TRAINING"));
	_btnPsiTraining->onMouseClick((ActionHandler)&SoldiersState::btnPsiTrainingClick);
	_btnPsiTraining->setVisible(isPsiBtnVisible);

	_btnTraining->setText(tr("STR_TRAINING"));
	_btnTraining->onMouseClick((ActionHandler)&SoldiersState::btnTrainingClick);
	_btnTraining->setVisible(isTrnBtnVisible);

	_btnMemorial->setText(tr("STR_MEMORIAL"));
	_btnMemorial->onMouseClick((ActionHandler)&SoldiersState::btnMemorialClick);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_LEFT);
	_txtTitle->setText(tr("STR_SOLDIER_LIST"));

	_txtName->setText(tr("STR_NAME_UC"));

	_txtRank->setText(tr("STR_RANK"));

	_txtCraft->setText(tr("STR_CRAFT"));

	// populate sort options
	std::vector<std::wstring> sortOptions;
	sortOptions.push_back(tr("STR_ORIGINAL_ORDER"));
	_sortFunctors.push_back(NULL);

#define PUSH_IN(strId, functor, asc, txt) \
	if(txt) { ((TaggedText<int>*)txt)->setTag(sortOptions.size()); ((TaggedText<int>*)txt)->onMouseClick((ActionHandler)&SoldiersState::txtColumnHeaderClick); } \
	sortOptions.push_back(tr(strId)); \
	_sortFunctors.push_back(new SortFunctor(_game, functor, asc));

	PUSH_IN("STR_ID", idStat, true, (void*)0);
	PUSH_IN("STR_FIRST_LETTER", nameStat, true, (void*)0);
	PUSH_IN("STR_RANK", rankStat, false, (void*)0);
	PUSH_IN("STR_MISSIONS2", missionsStat, false, (void*)0);
	PUSH_IN("STR_KILLS2", killsStat, false, (void*)0);
	PUSH_IN("STR_WOUND_RECOVERY2", woundRecoveryStat, false, (void*)0);
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
	if (_showPsi && showPsiSkill)
	{
		PUSH_IN("STR_PSIONIC_SKILL", psiSkillStat, false, _txtPsiSkill);
	}
	else
	{
		_txtPsiSkill->setVisible(false);
	}

#undef PUSH_IN

	_cbxSortBy->setOptions(sortOptions);
	_cbxSortBy->setSelected(0);
	_cbxSortBy->onChange((ActionHandler)&SoldiersState::cbxSortByChange);
	_cbxSortBy->setText(tr("STR_SORT_BY"));

	_lstSoldiers->setAlign(ALIGN_RIGHT, 3);
	_lstSoldiers->setSelectable(true);
	_lstSoldiers->setBackground(_window);
	//_lstSoldiers->setMargin(8);
	_lstSoldiers->onLeftArrowClick((ActionHandler)&SoldiersState::lstItemsLeftArrowClick);
	_lstSoldiers->onRightArrowClick((ActionHandler)&SoldiersState::lstItemsRightArrowClick);
	_lstSoldiers->onMouseClick((ActionHandler)&SoldiersState::lstSoldiersClick);

	_txtTU->setAlign(ALIGN_CENTER);
	_txtTU->setText(tr("STR_TIME_UNITS_ABBREVIATION"));
	_txtTU->setTooltip("STR_TIME_UNITS");
	_txtTU->onMouseIn((ActionHandler)&SoldiersState::txtTooltipIn);
	_txtTU->onMouseOut((ActionHandler)&SoldiersState::txtTooltipOut);

	_txtStamina->setAlign(ALIGN_CENTER);
	_txtStamina->setText(tr("STR_STAMINA_ABBREVIATION"));
	_txtStamina->setTooltip("STR_STAMINA");
	_txtStamina->onMouseIn((ActionHandler)&SoldiersState::txtTooltipIn);
	_txtStamina->onMouseOut((ActionHandler)&SoldiersState::txtTooltipOut);

	_txtHealth->setAlign(ALIGN_CENTER);
	_txtHealth->setText(tr("STR_HEALTH_ABBREVIATION"));
	_txtHealth->setTooltip("STR_HEALTH");
	_txtHealth->onMouseIn((ActionHandler)&SoldiersState::txtTooltipIn);
	_txtHealth->onMouseOut((ActionHandler)&SoldiersState::txtTooltipOut);

	_txtBravery->setAlign(ALIGN_CENTER);
	_txtBravery->setText(tr("STR_BRAVERY_ABBREVIATION"));
	_txtBravery->setTooltip("STR_BRAVERY");
	_txtBravery->onMouseIn((ActionHandler)&SoldiersState::txtTooltipIn);
	_txtBravery->onMouseOut((ActionHandler)&SoldiersState::txtTooltipOut);

	_txtReactions->setAlign(ALIGN_CENTER);
	_txtReactions->setText(tr("STR_REACTIONS_ABBREVIATION"));
	_txtReactions->setTooltip("STR_REACTIONS");
	_txtReactions->onMouseIn((ActionHandler)&SoldiersState::txtTooltipIn);
	_txtReactions->onMouseOut((ActionHandler)&SoldiersState::txtTooltipOut);

	_txtFiring->setAlign(ALIGN_CENTER);
	_txtFiring->setText(tr("STR_FIRING_ACCURACY_ABBREVIATION"));
	_txtFiring->setTooltip("STR_FIRING_ACCURACY");
	_txtFiring->onMouseIn((ActionHandler)&SoldiersState::txtTooltipIn);
	_txtFiring->onMouseOut((ActionHandler)&SoldiersState::txtTooltipOut);

	_txtThrowing->setAlign(ALIGN_CENTER);
	_txtThrowing->setText(tr("STR_THROWING_ACCURACY_ABBREVIATION"));
	_txtThrowing->setTooltip("STR_THROWING_ACCURACY");
	_txtThrowing->onMouseIn((ActionHandler)&SoldiersState::txtTooltipIn);
	_txtThrowing->onMouseOut((ActionHandler)&SoldiersState::txtTooltipOut);

	_txtMelee->setAlign(ALIGN_CENTER);
	_txtMelee->setText(tr("STR_MELEE_ACCURACY_ABBREVIATION"));
	_txtMelee->setTooltip("STR_MELEE_ACCURACY");
	_txtMelee->onMouseIn((ActionHandler)&SoldiersState::txtTooltipIn);
	_txtMelee->onMouseOut((ActionHandler)&SoldiersState::txtTooltipOut);

	_txtStrength->setAlign(ALIGN_CENTER);
	_txtStrength->setText(tr("STR_STRENGTH_ABBREVIATION"));
	_txtStrength->setTooltip("STR_STRENGTH");
	_txtStrength->onMouseIn((ActionHandler)&SoldiersState::txtTooltipIn);
	_txtStrength->onMouseOut((ActionHandler)&SoldiersState::txtTooltipOut);

	_txtPsiStrength->setAlign(ALIGN_CENTER);
	_txtPsiStrength->setText(tr("STR_PSIONIC_STRENGTH_ABBREVIATION"));
	_txtPsiStrength->setTooltip("STR_PSIONIC_STRENGTH");
	_txtPsiStrength->onMouseIn((ActionHandler)&SoldiersState::txtTooltipIn);
	_txtPsiStrength->onMouseOut((ActionHandler)&SoldiersState::txtTooltipOut);

	_txtPsiSkill->setAlign(ALIGN_CENTER);
	_txtPsiSkill->setText(tr("STR_PSIONIC_SKILL_ABBREVIATION"));
	_txtPsiSkill->setTooltip("STR_PSIONIC_SKILL");
	_txtPsiSkill->onMouseIn((ActionHandler)&SoldiersState::txtTooltipIn);
	_txtPsiSkill->onMouseOut((ActionHandler)&SoldiersState::txtTooltipOut);

	/*_lstSoldierStats->setColumns(13, 90, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 0);
	_lstSoldierStats->setAlign(ALIGN_CENTER);
	_lstSoldierStats->setAlign(ALIGN_LEFT, 0);
	_lstSoldierStats->setDot(true);*/
}

/**
 * cleans up dynamic state
 */
SoldiersState::~SoldiersState()
{
	for (std::vector<SortFunctor *>::iterator it = _sortFunctors.begin(); it != _sortFunctors.end(); ++it)
	{
		delete(*it);
	}
}

/**
 * Sorts the soldiers list by the selected criterion
 * @param action Pointer to an action.
 */
void SoldiersState::cbxSortByChange(Action *action)
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
 * Updates the soldiers list
 * after going to other screens.
 */
void SoldiersState::init()
{
	State::init();

	// resets the savegame when coming back from the inventory
	_game->getSavedGame()->setBattleGame(0);
	_base->setInBattlescape(false);

	initList(0);
}

/**
 * Shows the soldiers in a list at specified offset/scroll.
 */
void SoldiersState::initList(size_t scrl)
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
		_lstSoldiers->setColumns(4, 112, 90, 72, 6);
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

	unsigned int row = 0;
	_lstSoldiers->clearList();
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
			// call corresponding getter
			int dynStat = 0;
			std::wostringstream ss;
			if (_dynGetter != NULL) {
				dynStat = (*_dynGetter)(_game, *i);
				ss << dynStat;
			}
			else {
				ss << L"";
			}

			std::wostringstream rank;
			rank << tr((*i)->getRankString());
			Role *role;
			if ((role = (*i)->getRole()) && !role->isBlank())
			{
				rank << "-" << tr((*i)->getRole()->getName() + "_SHORT");
			}

			_lstSoldiers->addRow(4, (*i)->getName(true).c_str(), rank.str().c_str(), (*i)->getCraftString(_game->getLanguage()).c_str(), ss.str().c_str());
		}

		if ((*i)->getCraft() == 0)
		{
			_lstSoldiers->setRowColor(row, _lstSoldiers->getSecondaryColor());
		}
		row++;
	}
	if (scrl)
		_lstSoldiers->scrollTo(scrl);
	_lstSoldiers->draw();
}


/**
 * Reorders a soldier up.
 * @param action Pointer to an action.
 */
void SoldiersState::lstItemsLeftArrowClick(Action *action)
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
void SoldiersState::moveSoldierUp(Action *action, unsigned int row, bool max)
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
void SoldiersState::lstItemsRightArrowClick(Action *action)
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
void SoldiersState::moveSoldierDown(Action *action, unsigned int row, bool max)
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
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldiersState::btnOkClick(Action *)
{
	_game->popState();
}

/**
 * Opens the Psionic Training screen.
 * @param action Pointer to an action.
 */
void SoldiersState::btnPsiTrainingClick(Action *)
{
	_game->pushState(new AllocatePsiTrainingState(_base));
}

/**
 * Opens the Psionic Training screen.
 * @param action Pointer to an action.
 */
void SoldiersState::btnTrainingClick(Action *)
{
	_game->pushState(new AllocateTrainingState(_base));
}

/**
 * Opens the Memorial screen.
 * @param action Pointer to an action.
 */
void SoldiersState::btnMemorialClick(Action *)
{
	_game->pushState(new SoldierMemorialState);
}

/**
* Displays the inventory screen for the soldiers inside the base.
* @param action Pointer to an action.
*/
void SoldiersState::btnInventoryClick(Action *)
{
	if (_base->getAvailableSoldiers(true))
	{
		SavedBattleGame *bgame = new SavedBattleGame();
		_game->getSavedGame()->setBattleGame(bgame);
		bgame->setMissionType("STR_BASE_DEFENSE");

		BattlescapeGenerator bgen = BattlescapeGenerator(_game);
		bgen.setBase(_base);
		bgen.runInventory(0);

		_game->getScreen()->clear();
		_game->pushState(new InventoryState(false, 0, _base, true));
	}
}

/**
 * Shows the selected soldier's info.
 * @param action Pointer to an action.
 */
void SoldiersState::lstSoldiersClick(Action *action)
{
	double mx = action->getAbsoluteXMouse();
	if (mx >= _lstSoldiers->getArrowsLeftEdge() && mx < _lstSoldiers->getArrowsRightEdge())
	{
		return;
	}

	_game->pushState(new SoldierInfoState(_base, _lstSoldiers->getSelectedRow()));
}

/**
 * Shows a tooltip for the appropriate text.
 * @param action Pointer to an action.
 */
void SoldiersState::txtTooltipIn(Action *action)
{
	_currentTooltip = action->getSender()->getTooltip();
	_txtTooltip->setText(tr(_currentTooltip));
}

/**
 * Clears the tooltip text.
 * @param action Pointer to an action.
 */
void SoldiersState::txtTooltipOut(Action *action)
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
void SoldiersState::btnToggleStatsClick(Action *action)
{
	_showStats = !_showStats;
	initList(_lstSoldiers->getScroll());
}

/**
 * Handler for sortable column headers.
 * @param action Pointer to an action.
 */
void SoldiersState::txtColumnHeaderClick(Action *action)
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

}
