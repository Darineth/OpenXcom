#pragma once
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
#include "../Engine/State.h"
#include <vector>
#include "SoldierSortUtil.h"

namespace OpenXcom
{

class TextButton;
class Window;
class Text;
class TextList;
class ComboBox;
class Base;
class Soldier;
template <typename TagType> class TaggedText;
struct SortFunctor;

/**
 * Select Squad screen that lets the player
 * pick the soldiers to assign to a craft.
 */
class CraftSoldiersState : public State
{
private:
	TextButton *_btnOk, *_btnStats, *_btnRoles;
	Window *_window;
	Text *_txtTitle, *_txtName, *_txtRank, *_txtCraft, *_txtAvailable, *_txtUsed;
	ComboBox *_cbxSortBy;
	TextList *_lstSoldiers;
	TaggedText<int> *_txtTU, *_txtStamina, *_txtHealth, *_txtBravery, *_txtReactions, *_txtFiring,
		*_txtThrowing, *_txtMelee, *_txtStrength, *_txtPsiStrength, *_txtPsiSkill;
	Text *_txtTooltip;
	std::string _currentTooltip;
	bool _showStats, _showPsi;
	Base *_base;
	size_t _craft;
	bool _craftOut;
	Uint8 _otherCraftColor;
	std::vector<Soldier *> _origSoldierOrder;
	std::vector<SortFunctor *> _sortFunctors;
	getStatFn_t _dynGetter;
	/// initializes the display list based on the craft soldier's list and the position to display
	void initList(size_t scrl);
public:
	/// Creates the Craft Soldiers state.
	CraftSoldiersState(Base *base, size_t craft);
	/// Cleans up the Craft Soldiers state.
	~CraftSoldiersState();
	/// Handler for changing the sort by combobox.
	void cbxSortByChange(Action *action);
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Updates the soldiers list.
	void init();
	/// Handler for clicking the Soldiers reordering button.
	void lstItemsLeftArrowClick(Action *action);
	/// Moves a soldier up.
	void moveSoldierUp(Action *action, unsigned int row, bool max = false);
	/// Handler for clicking the Soldiers reordering button.
	void lstItemsRightArrowClick(Action *action);
	/// Moves a soldier down.
	void moveSoldierDown(Action *action, unsigned int row, bool max = false);
	/// Handler for clicking the Soldiers list.
	void lstSoldiersClick(Action *action);
	/// Handler for pressing-down a mouse-button in the list.
	void lstSoldiersMousePress(Action *action);
	/// Handler for clicking the De-assign All Soldiers button.
	void btnDeassignAllSoldiersClick(Action *action);
	/// Handler for displaying tooltips.
	void txtTooltipIn(Action *action);
	/// Handler for hiding tooltips.
	void txtTooltipOut(Action *action);
	/// Handler for toggling the stats list.
	void btnToggleStatsClick(Action *action);
	/// Handler for sortable column headers.
	void txtColumnHeaderClick(Action *action);
};

}
