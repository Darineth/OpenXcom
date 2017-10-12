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

namespace OpenXcom
{

class Window;
class TextButton;
class Text;
class ComboBox;
class TextList;
class MapBlock;

/**
 * A state for testing most common modding mistakes.
 */
class TestState : public State
{
private:
	Window *_window;
	TextButton *_btnLowContrast, *_btnHighContrast, *_btnPreview;
	TextButton *_btnRun, *_btnCancel;
	Text *_txtPalette;
	Text *_txtTitle, *_txtTestCase, *_txtDescription;
	ComboBox *_cbxPalette;
	ComboBox *_cbxTestCase;
	TextList *_lstOutput;
	std::vector<std::string> _paletteList;
	std::vector<std::string> _testCases;
	/// Test cases.
	void testCase0();
	int checkRMP(MapBlock *mapblock);
	int loadMAP(MapBlock *mapblock);
public:
	/// Creates the Test state.
	TestState();
	/// Cleans up the Test state.
	~TestState();
	/// Handler for changing the Test Case combobox.
	void cbxTestCaseChange(Action *action);
	/// Handler for clicking the Run button.
	void btnRunClick(Action *action);
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action *action);
	/// Handler for changing the Palette combobox.
	void cbxPaletteChange(Action *action);
	/// Handler for clicking the LowContrast button.
	void btnLowContrastClick(Action *action);
	/// Handler for clicking the HighContrast button.
	void btnHighContrastClick(Action *action);
};

}
