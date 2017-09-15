/*
 * Copyright 2010-2015 OpenXcom Developers.
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
#ifndef OPENXCOM_TECHTREEVIEWERSTATE_H
#define OPENXCOM_TECHTREEVIEWERSTATE_H

#include "../Engine/State.h"
#include <vector>
#include <map>
#include <string>
#include <unordered_set>

namespace OpenXcom
{

class TextButton;
class Window;
class Text;
class TextList;
class RuleResearch;
class RuleManufacture;

/**
 * TechTreeViewer screen, where you can browse the Tech Tree.
 */
class TechTreeViewerState : public State
{
private:
	TextButton *_btnOk, *_btnNew;
	Window *_window;
	Text *_txtTitle, *_txtSelectedTopic, *_txtProgress;
	TextList *_lstLeft, *_lstRight;
	Uint8 _purple, _pink, _blue, _white, _gold;
	std::string _selectedTopic;
	int _selectedFlag;
	std::vector<std::string> _leftTopics, _rightTopics;
	std::vector<int> _leftFlags, _rightFlags; // 0=none, 1=research, 2=manufacturing
	std::unordered_set<std::string> _alreadyAvailableResearch, _alreadyAvailableManufacture;
	void initLists();
	void onSelectLeftTopic(Action *action);
	void onSelectRightTopic(Action *action);
public:
	/// Creates the Tech Tree Viewer state.
	TechTreeViewerState(const RuleResearch *selectedTopicResearch = 0, const RuleManufacture *selectedTopicManufacture = 0);
	/// Cleans up the Tech Tree Viewer state.
	~TechTreeViewerState();
	/// Initializes the state.
	void init();
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handler for clicking the New button.
	void btnNewClick(Action *action);
	/// Sets the selected topic.
	void setSelectedTopic(const std::string &selectedTopic, bool isManufacturingTopic);
	/// Is given research topic discovered/available?
	bool isDiscoveredResearch(const std::string &topic) const;
	/// Is given manufacture topic discovered/available?
	bool isDiscoveredManufacture(const std::string &topic) const;
};

}

#endif
