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
#include "../Engine/LocalizedText.h"
#include <string>
#include <vector>
#include <map>
#include "../Savegame/SavedGame.h"
#include "../Mod/Unit.h"

namespace OpenXcom
{

class TextButton;
class Window;
class Text;
class TextList;
class BattleItem;
class Craft;
class Base;
class Region;
class Country;
class RuleItem;
class BattleUnit;
struct UnitStats;
class Soldier;

struct DebriefingStat
{
	DebriefingStat(const std::string &_item, bool recovery) : item(_item), qty(0), score(0), recovery(recovery), soldier(0), statIncreases(), gainedExperience(0), gainedLevel(0) {};
	DebriefingStat(Soldier* soldier, const std::wstring &text) : text(text), item(), qty(qty), score(0), recovery(recovery), soldier(soldier), statIncreases(), gainedExperience(0), gainedLevel(0) {};
	DebriefingStat(Soldier* soldier, const std::wstring &text, const UnitStats &statIncreases, int gainedExperience, bool gainedLevel, int kills) : text(text), item(), qty(0), score(0), recovery(recovery), soldier(soldier), statIncreases(statIncreases), gainedExperience(gainedExperience), gainedLevel(gainedLevel), kills(kills){};
	std::string item; std::wstring text; int qty; int score; bool recovery; Soldier* soldier; UnitStats statIncreases; int gainedExperience; bool gainedLevel; int kills;
};

struct ReequipStat { std::string item; int qty; std::wstring craft; int listOrder; };

struct RecoveryItem { std::string name; int value; };

/**
 * Debriefing screen shown after a Battlescape
 * mission that displays the results.
 */
class DebriefingState : public State
{
private:
	typedef std::pair<std::wstring, UnitStats> SoldierStatsEntry;

	Region *_region;
	Country *_country;
	Base *_base;
	std::vector<DebriefingStat*> _stats;
	std::vector<SoldierStatsEntry> _soldierStats;
	TextButton *_btnOk, *_btnStats, *_btnSell;
	Window *_window;
	Text *_txtTitle, *_txtItem, *_txtQuantity, *_txtScore, *_txtRecovery, *_txtRating,
	     *_txtSoldier, *_txtStatus;
	TextList *_lstStats, *_lstRecovery, *_lstTotal, *_lstSoldiers, *_lstRecoveredItems;
	std::string _currentTooltip;
	Text *_txtTooltip;
	std::vector<ReequipStat> _missingItems;
	std::map<const RuleItem*, int> _rounds, _roundsPainKiller, _roundsStimulant, _roundsHeal, _recoveredItems;
	Uint8 _ammoColor;
	std::map<int, RecoveryItem*> _recoveryStats;
	bool _positiveScore, _destroyBase, _showSellButton;
	std::map<int, int>  _containmentStateInfo;
	int _limitsEnforced;
	MissionStatistics *_missionStatistics;
	std::vector<Soldier*> _soldiersCommended, _deadSoldiersCommended;
	/// Adds to the debriefing stats.
	void addStat(const std::string &name, int quantity, int score);
	/// Prepares debriefing.
	void prepareDebriefing();
	/// Recovers items from the battlescape.
	void recoverItems(std::vector<BattleItem*> *from, Base *base);
	/// Recovers a civilian from the battlescape.
	void recoverCivilian(BattleUnit *from, Base *base);
	/// Recovers an alien from the battlescape.
	void recoverAlien(BattleUnit *from, Base *base);
	/// Reequips a craft after a mission.
	void reequipCraft(Base *base, Craft *craft, bool vehicleItemsCanBeDestroyed);
	/// 0 = score, 1 = stat improvement, 2 = recovered items
	int _pageNumber;
	/// Sets the visibility according to the _pageNumber
	void applyVisibility();
	/// Creates a string for the soldier stats table from a stat difference value
	std::wstring makeSoldierString(int stat);
public:
	/// Creates the Debriefing state.
	DebriefingState();
	/// Cleans up the Debriefing state.
	~DebriefingState();
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	void init();
	/// Handler for clicking the STATS button.
	void btnStatsClick(Action *action);
	/// Handler for clicking the SELL button.
	void btnSellClick(Action *action);
	/// Handler for showing tooltip.
	void txtTooltipIn(Action *action);
	/// Handler for hiding tooltip.
	void txtTooltipOut(Action *action);
	// Gets the number of recovered items of certain type.
	int getRecoveredItemCount(RuleItem *rule);
	// Sets the visibility of the SELL button.
	void setShowSellButton(bool showSellButton);
};

}
