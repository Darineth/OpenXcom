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
#include "TestState.h"
#include "TestPaletteState.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"
#include "../Mod/Mod.h"
#include <fstream>
#include "../Engine/Exception.h"
#include "../Engine/FileMap.h"
#include "../Engine/Logger.h"
#include "../Mod/RuleTerrain.h"
#include "../Mod/MapBlock.h"
#include "../Mod/RuleUfo.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the test screen.
 */
TestState::TestState()
{
	// Create objects
	_window = new Window(this, 320, 200, 0, 0);
	_txtTitle = new Text(300, 17, 10, 7);
	_txtPalette = new Text(86, 9, 10, 30);
	_cbxPalette = new ComboBox(this, 114, 16, 98, 26);
	_btnLowContrast = new TextButton(42, 16, 220, 26);
	_btnHighContrast = new TextButton(42, 16, 270, 26);
	_btnPreview = new TextButton(92, 16, 220, 26);
	_txtTestCase = new Text(86, 9, 10, 50);
	_cbxTestCase = new ComboBox(this, 214, 16, 98, 46);
	_txtDescription = new Text(300, 25, 10, 66);
	_lstOutput = new TextList(284, 80, 10, 94);
	_btnRun = new TextButton(146, 16, 10, 176);
	_btnCancel = new TextButton(146, 16, 164, 176);

	// Set palette
	setInterface("tests");

	add(_window, "window", "tests");
	add(_txtTitle, "heading", "tests");
	add(_txtPalette, "text", "tests");
	add(_btnLowContrast, "button2", "tests");
	add(_btnHighContrast, "button2", "tests");
	add(_btnPreview, "button2", "tests");
	add(_txtTestCase, "text", "tests");
	add(_txtDescription, "heading", "tests");
	add(_lstOutput, "text", "tests");
	add(_btnRun, "button2", "tests");
	add(_btnCancel, "button2", "tests");
	add(_cbxTestCase, "button1", "tests"); // add as last (display over all other components)
	add(_cbxPalette, "button1", "tests"); // add as last (display over all other components)

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK05.SCR"));

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_TEST_SCREEN"));

	_txtPalette->setText(tr("STR_PALETTE"));

	bool isTFTD = false;
	for (std::vector< std::pair<std::string, bool> >::const_iterator i = Options::mods.begin(); i != Options::mods.end(); ++i)
	{
		if (i->second)
		{
			if (i->first == "xcom2")
			{
				isTFTD = true;
				break;
			}
		}
	}

	_paletteList.push_back("PAL_GEOSCAPE");
	_paletteList.push_back("PAL_BASESCAPE");
	_paletteList.push_back("PAL_GRAPHS");
	if (!isTFTD)
	{
		_paletteList.push_back("PAL_UFOPAEDIA");
		_paletteList.push_back("PAL_BATTLEPEDIA");
	}
	_paletteList.push_back("PAL_BATTLESCAPE");
	if (isTFTD)
	{
		_paletteList.push_back("PAL_BATTLESCAPE_1");
		_paletteList.push_back("PAL_BATTLESCAPE_2");
		_paletteList.push_back("PAL_BATTLESCAPE_3");
	}

	_cbxPalette->setOptions(_paletteList);
	_cbxPalette->onChange((ActionHandler)&TestState::cbxPaletteChange);

	_btnLowContrast->setText(tr("STR_LOW_CONTRAST"));
	_btnLowContrast->onMouseClick((ActionHandler)&TestState::btnLowContrastClick);
	_btnLowContrast->setVisible(false);

	_btnHighContrast->setText(tr("STR_HIGH_CONTRAST"));
	_btnHighContrast->onMouseClick((ActionHandler)&TestState::btnHighContrastClick);
	_btnHighContrast->setVisible(false);

	_btnPreview->setText(tr("STR_PREVIEW"));
	_btnPreview->onMouseClick((ActionHandler)&TestState::btnLowContrastClick);
	_btnPreview->setVisible(true);

	_txtTestCase->setText(tr("STR_TEST_CASE"));

	_testCases.push_back("STR_BAD_NODES");

	_cbxTestCase->setOptions(_testCases);
	_cbxTestCase->onChange((ActionHandler)&TestState::cbxTestCaseChange);

	_txtDescription->setWordWrap(true);
	cbxTestCaseChange(0); // update description

	_lstOutput->setColumns(1, 284);
	_lstOutput->setBackground(_window);
	_lstOutput->setWordWrap(true);

	_btnRun->setText(tr("STR_RUN"));
	_btnRun->onMouseClick((ActionHandler)&TestState::btnRunClick);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)&TestState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&TestState::btnCancelClick, Options::keyCancel);
}

TestState::~TestState()
{
}

/**
* Updates the test case description.
* @param action Pointer to an action.
*/
void TestState::cbxTestCaseChange(Action *)
{
	size_t index = _cbxTestCase->getSelected();
	_txtDescription->setText(tr(_testCases[index]+"_DESC"));
}

/**
* Returns to the previous screen.
* @param action Pointer to an action.
*/
void TestState::btnRunClick(Action *action)
{
	_lstOutput->clearList();

	size_t index = _cbxTestCase->getSelected();
	switch (index)
	{
		case 0: testCase0(); break;
		default: break;
	}
}

/**
* Returns to the previous screen.
* @param action Pointer to an action.
*/
void TestState::btnCancelClick(Action *action)
{
	_game->popState();
}

/**
* Updates the UI buttons.
* @param action Pointer to an action.
*/
void TestState::cbxPaletteChange(Action *)
{
	size_t index = _cbxPalette->getSelected();
	if (_paletteList[index].find("PAL_BATTLESCAPE") != std::string::npos)
	{
		_btnPreview->setVisible(false);
		_btnLowContrast->setVisible(true);
		_btnHighContrast->setVisible(true);
	}
	else
	{
		_btnPreview->setVisible(true);
		_btnLowContrast->setVisible(false);
		_btnHighContrast->setVisible(false);
	}
}

/**
* Shows palette preview with low contrast.
* @param action Pointer to an action.
*/
void TestState::btnLowContrastClick(Action *action)
{
	size_t index = _cbxPalette->getSelected();
	const std::string palette = _paletteList[index];
	_game->pushState(new TestPaletteState(palette, false));
}

/**
* Shows palette preview with high contrast.
* @param action Pointer to an action.
*/
void TestState::btnHighContrastClick(Action *action)
{
	size_t index = _cbxPalette->getSelected();
	const std::string palette = _paletteList[index];
	_game->pushState(new TestPaletteState(palette, true));
}

void TestState::testCase0()
{
	_lstOutput->addRow(1, tr("STR_TESTS_STARTING").c_str());
	_lstOutput->addRow(1, tr("STR_BAD_NODES_CHECKING_TERRAIN").c_str());
	int total = 0;
	for (std::vector<std::string>::const_iterator i = _game->getMod()->getTerrainList().begin(); i != _game->getMod()->getTerrainList().end(); ++i)
	{
		RuleTerrain *terrRule = _game->getMod()->getTerrain((*i));
		for (std::vector<MapBlock*>::iterator j = terrRule->getMapBlocks()->begin(); j != terrRule->getMapBlocks()->end(); ++j)
		{
			total += checkRMP((*j));
		}
	}
	_lstOutput->addRow(1, tr("STR_BAD_NODES_CHECKING_UFOS").c_str());
	for (std::vector<std::string>::const_iterator i = _game->getMod()->getUfosList().begin(); i != _game->getMod()->getUfosList().end(); ++i)
	{
		RuleUfo *ufoRule = _game->getMod()->getUfo((*i));
		if (ufoRule->getBattlescapeTerrainData())
		{
			for (std::vector<MapBlock*>::iterator j = ufoRule->getBattlescapeTerrainData()->getMapBlocks()->begin(); j != ufoRule->getBattlescapeTerrainData()->getMapBlocks()->end(); ++j)
			{
				total += checkRMP((*j));
			}
		}
	}
	_lstOutput->addRow(1, tr("STR_BAD_NODES_CHECKING_CRAFT").c_str());
	for (std::vector<std::string>::const_iterator i = _game->getMod()->getCraftsList().begin(); i != _game->getMod()->getCraftsList().end(); ++i)
	{
		RuleCraft *craftRule = _game->getMod()->getCraft((*i));
		if (craftRule->getBattlescapeTerrainData())
		{
			for (std::vector<MapBlock*>::iterator j = craftRule->getBattlescapeTerrainData()->getMapBlocks()->begin(); j != craftRule->getBattlescapeTerrainData()->getMapBlocks()->end(); ++j)
			{
				total += checkRMP((*j));
			}
		}
	}
	if (total > 0)
	{
		_lstOutput->addRow(1, tr("STR_TESTS_ERRORS_FOUND").arg(total).c_str());
		_lstOutput->addRow(1, tr("STR_BAD_NODES_DETAILED_INFO").c_str());
	}
	else
	{
		_lstOutput->addRow(1, tr("STR_TESTS_NO_ERRORS_FOUND").c_str());
	}
	_lstOutput->addRow(1, tr("STR_TESTS_FINISHED").c_str());
}

int TestState::checkRMP(MapBlock *mapblock)
{
	// Z-dimension is not in the ruleset, it's hacked on-the-fly
	int mapblockSizeZ = loadMAP(mapblock);

	unsigned char value[24];
	std::ostringstream filename;
	filename << "ROUTES/" << mapblock->getName() << ".RMP";

	// Load file
	std::ifstream mapFile(FileMap::getFilePath(filename.str()).c_str(), std::ios::in | std::ios::binary);
	if (!mapFile)
	{
		throw Exception(filename.str() + " not found");
	}

	int nodesAdded = 0;
	int errors = 0;
	while (mapFile.read((char*)&value, sizeof(value)))
	{
		int pos_x = value[1];
		int pos_y = value[0];
		int pos_z = value[2];
		if (pos_x >= 0 && pos_x < mapblock->getSizeX() &&
			pos_y >= 0 && pos_y < mapblock->getSizeY() &&
			pos_z >= 0 && pos_z < mapblockSizeZ) // using "mapblockSizeZ" instead of "mapblock->getSizeZ()"!!
		{
			// all ok
		}
		else
		{
			Log(LOG_INFO) << "Bad node in RMP file: " << filename.str() << " Node #" << nodesAdded << " is outside map boundaries at X:" << pos_x << " Y:" << pos_y << " Z:" << pos_z << ". Found using OXCE+ test cases.";
			errors++;
		}
		nodesAdded++;
	}

	if (!mapFile.eof())
	{
		throw Exception("Invalid RMP file: " + filename.str());
	}

	mapFile.close();

	return errors;
}

int TestState::loadMAP(MapBlock *mapblock)
{
	int sizez;
	char size[3];
	std::ostringstream filename;
	filename << "MAPS/" << mapblock->getName() << ".MAP";

	// Load file
	std::ifstream mapFile(FileMap::getFilePath(filename.str()).c_str(), std::ios::in | std::ios::binary);
	if (!mapFile)
	{
		throw Exception(filename.str() + " not found");
	}

	mapFile.read((char*)&size, sizeof(size));
	//sizey = (int)size[0];
	//sizex = (int)size[1];
	sizez = (int)size[2];

	//mapblock->setSizeZ(sizez); // commented out, for testing purposes we don't need to HACK it like in real code

	mapFile.close();

	return sizez;
}

}
