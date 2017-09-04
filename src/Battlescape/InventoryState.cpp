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
#include "InventoryState.h"
#include <algorithm>
#include "Inventory.h"
#include "../Engine/Game.h"
#include "../Engine/FileMap.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Screen.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"
#include "../Interface/Text.h"
#include "../Interface/BattlescapeButton.h"
#include "../Engine/Action.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Sound.h"
#include "../Engine/SurfaceSet.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Soldier.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleInventory.h"
#include "../Mod/Armor.h"
#include "../Engine/Options.h"
#include "../Basescape/CraftSoldiersState.h"
#include "UnitInfoState.h"
#include "BattlescapeState.h"
#include "BattlescapeGenerator.h"
#include "TileEngine.h"
#include "../Mod/RuleInterface.h"
#include "RoleMenuState.h"
#include "../Savegame/Role.h"
#include "../Mod/RuleRole.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Base.h"
#include "../Mod/RuleInventoryLayout.h"
#include "../Interface/ComboBox.h"
#include "../Engine/ShaderDraw.h"
#include "../Engine/ShaderMove.h"
#include "UnitSprite.h"

namespace OpenXcom
{

static const int _templateBtnX = 288;
static const int _createTemplateBtnY = 91;
static const int _applyTemplateBtnY  = 114;

/**
 * Initializes all the elements in the Inventory screen.
 * @param game Pointer to the core game.
 * @param tu Does Inventory use up Time Units?
 * @param parent Pointer to parent Battlescape.
 */
InventoryState::InventoryState(bool tu, BattlescapeState *parent) : _tu(tu), _parent(parent), _mouseOverItem(0), _armorColors(0)
{
	_battleGame = _game->getSavedGame()->getSavedBattle();

	if (Options::maximizeInfoScreens)
	{
		_game->getScreen()->pushMaximizeInfoScreen(_parent != 0);
	}
	else if (!_battleGame->getTileEngine())
	{
		Screen::updateScale(Options::battlescapeScale, Options::battlescapeScale, Options::baseXBattlescape, Options::baseYBattlescape, true);
		_game->getScreen()->resetDisplay(false);
	}

	// Create objects
	_bg = new Surface(320, 200, 0, 0);
	_soldier = new Surface(320, 200, 0, 0);
	_txtName = new Text(210, 9, 57, 2);
	_txtRole = new Text(100, 9, 57, 12);

	_txtWeight = new Text(75, 9, 243, 24);
	_txtHealth = new Text(75, 9, 243, 32);

	_txtTus = new Text(75, 9, 243, 40);
	_txtReact = new Text(75, 9, 243, 48);

	_txtFAcc = new Text(75, 40, 243, 56);
	_txtTAcc = new Text(75, 9, 243, 64);

	_txtPSkill = new Text(75, 9, 243, 72);
	_txtPStr = new Text(75, 9, 243, 80);
	_txtArmor = new Text(40, 50, 243, 88);

	_txtItem = new Text(160, 9, 128, 143);
	_btnOk = new BattlescapeButton(35, 22, 237, 1);
	_btnPrev = new BattlescapeButton(23, 22, 273, 1);
	_btnNext = new BattlescapeButton(23, 22, 297, 1);
	_btnUnload = new BattlescapeButton(32, 25, 288, 65);
	_btnGround = new BattlescapeButton(32, 15, 288, 137);
	_btnRank = new BattlescapeButton(26, 23, 0, 0);
	_btnRole = new BattlescapeButton(26, 23, 29, 0);
	_cmbArmorColor = new ComboBox(this, 75, 14, 160, 12);

	_btnCreateTemplate = new BattlescapeButton(32, 22, _templateBtnX, _createTemplateBtnY);
	_btnApplyTemplate = new BattlescapeButton(32, 22, _templateBtnX, _applyTemplateBtnY);
	_txtAmmo = new Text(32, 24, 286, 71);
	_selAmmo = new Surface(RuleInventory::HAND_W * RuleInventory::SLOT_W, RuleInventory::HAND_H * RuleInventory::SLOT_H, 288, 89);
	_inv = new Inventory(_game, 320, 200, 0, 0, _parent == 0);

	// Set palette
	setPalette("PAL_BATTLESCAPE");

	add(_bg);

	// Set up objects
	//_game->getMod()->getSurface("TAC01.SCR")->blit(_bg);

	add(_soldier);
	add(_txtName, "textName", "inventory", _bg);
	add(_txtRole, "textRole", "inventory", _bg);
	add(_txtTus, "textTUs", "inventory", _bg);
	add(_txtHealth, "textHealth", "inventory", _bg);
	add(_txtWeight, "textWeight", "inventory", _bg);
	add(_txtFAcc, "textFiring", "inventory", _bg);
	add(_txtTAcc, "textThrowing", "inventory", _bg);
	add(_txtReact, "textReaction", "inventory", _bg);
	add(_txtPSkill, "textPsiSkill", "inventory", _bg);
	add(_txtPStr, "textPsiStrength", "inventory", _bg);
	add(_txtArmor, "textArmor", "inventory", _bg);
	add(_txtItem, "textItem", "inventory", _bg);
	add(_txtAmmo, "textAmmo", "inventory", _bg);
	add(_btnOk, "buttonOK", "inventory", _bg);
	add(_btnPrev, "buttonPrev", "inventory", _bg);
	add(_btnNext, "buttonNext", "inventory", _bg);
	add(_btnUnload, "buttonUnload", "inventory", _bg);
	add(_btnGround, "buttonGround", "inventory", _bg);
	add(_btnRank, "rank", "inventory", _bg);
	add(_btnRole, "role", "inventory", _bg);
	add(_btnCreateTemplate, "buttonCreate", "inventory", _bg);
	add(_btnApplyTemplate, "buttonApply", "inventory", _bg);
	add(_selAmmo);
	add(_inv);
	add(_cmbArmorColor, "armorColor", "inventory", _bg);

	_game->getMod()->getSurface("InvOk")->blit(_btnOk);
	_btnOk->initSurfaces();
	_game->getMod()->getSurface("InvPrev")->blit(_btnPrev);
	_btnPrev->initSurfaces();
	_game->getMod()->getSurface("InvNext")->blit(_btnNext);
	_btnNext->initSurfaces();
	_game->getMod()->getSurface("InvUnload")->blit(_btnUnload);
	_btnUnload->initSurfaces();
	_game->getMod()->getSurface("InvGround")->blit(_btnGround);
	_btnGround->initSurfaces();

	// move the TU display down to make room for the weight display
	/*if (Options::showMoreStatsInInventoryView)
	{
		_txtTus->setY(_txtTus->getY() + 8);
	}*/

	centerAllSurfaces();

	_txtRole->setBig();
	_txtName->setHighContrast(true);

	_txtRole->setBig();
	_txtRole->setHighContrast(true);

	_txtWeight->setHighContrast(true);

	_txtTus->setHighContrast(true);

	_txtHealth->setHighContrast(true);

	_txtFAcc->setHighContrast(true);

	_txtTAcc->setHighContrast(true);

	_txtReact->setHighContrast(true);

	_txtPSkill->setHighContrast(true);

	_txtPStr->setHighContrast(true);

	_txtArmor->setHighContrast(true);

	_txtItem->setHighContrast(true);

	_txtAmmo->setAlign(ALIGN_CENTER);
	_txtAmmo->setHighContrast(true);

	_btnOk->onMouseClick((ActionHandler)&InventoryState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::btnOkClick, Options::keyBattleInventory);
	_btnOk->setTooltip("STR_OK");
	_btnOk->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnOk->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnPrev->onMouseClick((ActionHandler)&InventoryState::btnPrevClick);
	_btnPrev->onKeyboardPress((ActionHandler)&InventoryState::btnPrevClick, Options::keyBattlePrevUnit);
	_btnPrev->setTooltip("STR_PREVIOUS_UNIT");
	_btnPrev->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnPrev->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnNext->onMouseClick((ActionHandler)&InventoryState::btnNextClick);
	_btnNext->onKeyboardPress((ActionHandler)&InventoryState::btnNextClick, Options::keyBattleNextUnit);
	_btnNext->setTooltip("STR_NEXT_UNIT");
	_btnNext->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnNext->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnUnload->onMouseClick((ActionHandler)&InventoryState::btnUnloadClick);
	_btnUnload->setTooltip("STR_UNLOAD_WEAPON");
	_btnUnload->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnUnload->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnGround->onMouseClick((ActionHandler)&InventoryState::btnGroundClick);
	_btnGround->setTooltip("STR_SCROLL_RIGHT");
	_btnGround->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnGround->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnRank->onMouseClick((ActionHandler)&InventoryState::btnRankClick, _tu ? SDL_BUTTON_LEFT : 0);
	_btnRank->setTooltip("STR_UNIT_STATS");
	_btnRank->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnRank->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnNext->onKeyboardPress((ActionHandler)&InventoryState::onCtrlToggled, SDLK_LCTRL);
	_btnNext->onKeyboardRelease((ActionHandler)&InventoryState::onCtrlToggled, SDLK_LCTRL);

	if(!_tu)
	{
		_btnRole->onMouseClick((ActionHandler)&InventoryState::btnRoleClick, 0);
		_btnRole->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
		_btnRole->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);
		_btnRole->setTooltip("STR_ROLE_OPTIONS");
	}

	const std::vector<std::string> &modArmorColors = _game->getMod()->getSoldierUtileColorIndex();
	if (!tu && modArmorColors.size())
	{
		_armorColors = new std::map<int, std::string>();
		std::map<int, std::string> &armorColors(*_armorColors);
		std::vector<std::string> colorOptions;
		colorOptions.push_back("STR_NONE");
		armorColors[0] = "STR_NONE";
		for (auto ii = modArmorColors.cbegin(); ii != modArmorColors.cend(); ++ii)
		{
			colorOptions.push_back(*ii);
			armorColors[colorOptions.size() - 1] = *ii;
		}
		_cmbArmorColor->setOptions(colorOptions);
		_cmbArmorColor->setHighContrast(true);
		_cmbArmorColor->onChange((ActionHandler)&InventoryState::cmbArmorColorChange);
	}
	else
	{
		_cmbArmorColor->setVisible(false);
	}

	_btnCreateTemplate->onMouseClick((ActionHandler)&InventoryState::btnCreateTemplateClick);
	_btnCreateTemplate->onKeyboardPress((ActionHandler)&InventoryState::btnCreateTemplateClick, Options::keyInvCreateTemplate);
	_btnCreateTemplate->setTooltip("STR_CREATE_INVENTORY_TEMPLATE");
	_btnCreateTemplate->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnCreateTemplate->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnApplyTemplate->onMouseClick((ActionHandler)&InventoryState::btnApplyTemplateClick);
	_btnApplyTemplate->onKeyboardPress((ActionHandler)&InventoryState::btnApplyTemplateClick, Options::keyInvApplyTemplate);
	_btnApplyTemplate->onKeyboardPress((ActionHandler)&InventoryState::onClearInventory, Options::keyInvClear);
	_btnApplyTemplate->onKeyboardPress((ActionHandler)&InventoryState::onAutoequip, Options::keyInvAutoEquip);
	_btnApplyTemplate->setTooltip("STR_APPLY_INVENTORY_TEMPLATE");
	_btnApplyTemplate->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnApplyTemplate->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);


	// The role button only becomes available when the current unit is a soldier
	_btnRole->setVisible(false);

	// only use copy/paste buttons in setup (i.e. non-tu) mode
	if (_tu)
	{
		_btnCreateTemplate->setVisible(false);
		_btnApplyTemplate->setVisible(false);
	}
	else
	{
		_updateTemplateButtons(true);
	}

	_inv->draw();
	_inv->setTuMode(_tu);
	_inv->setSelectedUnit(_game->getSavedGame()->getSavedBattle()->getSelectedUnit());
	_inv->onMouseClick((ActionHandler)&InventoryState::invClick, 0);
	_inv->onMouseOver((ActionHandler)&InventoryState::invMouseOver);
	_inv->onMouseOut((ActionHandler)&InventoryState::invMouseOut);

	/*_txtTus->setVisible(_tu);
	_txtWeight->setVisible(Options::showMoreStatsInInventoryView);
	_txtFAcc->setVisible(Options::showMoreStatsInInventoryView && !_tu);
	_txtReact->setVisible(Options::showMoreStatsInInventoryView && !_tu);
	_txtPSkill->setVisible(Options::showMoreStatsInInventoryView && !_tu);
	_txtPStr->setVisible(Options::showMoreStatsInInventoryView && !_tu);*/
	
	_currentTooltip = "";
}

static void _clearInventoryTemplate(std::vector<EquipmentLayoutItem*> &inventoryTemplate)
{
	for (std::vector<EquipmentLayoutItem*>::iterator eraseIt = inventoryTemplate.begin();
		 eraseIt != inventoryTemplate.end();
		 eraseIt = inventoryTemplate.erase(eraseIt))
	{
		delete *eraseIt;
	}
}

/**
 *
 */
InventoryState::~InventoryState()
{
	_clearInventoryTemplate(_curInventoryTemplate);

	if (_battleGame->getTileEngine())
	{
		if (Options::maximizeInfoScreens)
		{
			_game->getScreen()->popMaximizeInfoScreen();
		}
		Tile *inventoryTile = _battleGame->getSelectedUnit()->getTile();
		_battleGame->getTileEngine()->applyGravity(inventoryTile);
		_battleGame->getTileEngine()->calculateTerrainLighting(); // dropping/picking up flares
		_battleGame->getTileEngine()->recalculateFOV();
	}
	else
	{
		_game->getScreen()->popMaximizeInfoScreen();
		//Screen::updateScale(Options::geoscapeScale, Options::geoscapeScale, Options::baseXGeoscape, Options::baseYGeoscape, true);
		//_game->getScreen()->resetDisplay(false);
	}

	if (_armorColors)
	{
		delete _armorColors;
		_armorColors = 0;
	}
}

/**
 * Updates all soldier stats when the soldier changes.
 */
void InventoryState::init()
{
	State::init();
	BattleUnit *unit = _battleGame->getSelectedUnit();

	// no selected unit, close inventory
	if (unit == 0)
	{
		btnOkClick(0);
		return;
	}
	// skip to the first unit with inventory
	if (!unit->hasInventory())
	{
		if (_parent)
		{
			_parent->selectNextPlayerUnit(false, false, true);
		}
		else
		{
			_battleGame->selectNextPlayerUnit(false, false, true);
		}
		// no available unit, close inventory
		if (_battleGame->getSelectedUnit() == 0 || !_battleGame->getSelectedUnit()->hasInventory())
		{
			// starting a mission with just vehicles
			btnOkClick(0);
			return;
		}
		else
		{
			unit = _battleGame->getSelectedUnit();
		}
	}

	unit->setCache(0);
	_soldier->clear();
	_btnRank->clear();
	_btnRole->clear();
	_txtRole->setVisible(false);
	
	_txtName->setText(unit->getName(_game->getLanguage()));

	if(Soldier *soldier = getSelectedSoldier())
	{
		std::wostringstream title;
		Role *role = soldier->getRole();
		if(role && !role->isBlank())
		{
			title << tr(soldier->getRole()->getName()) << " - ";
		}

		title << tr(unit->getRankString());

		_txtRole->setText(title.str());
		_txtRole->setVisible(true);
	}

	_inv->setSelectedUnit(unit);

	//_inv->setLocked(_parent && unit->isVehicle());

	updateStats();

	_refreshMouse();
}

/**
 * Updates the soldier stats (Weight, TU).
 */
void InventoryState::updateStats()
{
	BattleUnit *unit = getSelectedUnit();
	Soldier *s = getSelectedSoldier();

	const RuleInventoryLayout *layout = _game->getMod()->getInventoryLayout(unit->getInventoryLayout());
	if (layout)
	{
		_soldier->setX(layout->getInvSpriteOffset());
	}
	else
	{
		_soldier->setX(0);
	}

	if (s && !s->isVehicle())
	{
		SurfaceSet *texture = _game->getMod()->getSurfaceSet("SMOKE.PCK");
		texture->getFrame(20 + s->getRank())->setX(0);
		texture->getFrame(20 + s->getRank())->setY(0);
		texture->getFrame(20 + s->getRank())->blit(_btnRank);

		if(Role *role = s->getRole())
		{
			_btnRole->setVisible(true);
			_game->getMod()->getSurface(role->getRules()->getIconSprite())->blit(_btnRole);
		}

		std::string look = s->getArmor()->getSpriteInventory();
		if (s->getGender() == GENDER_MALE)
			look += "M";
		else
			look += "F";
		if (s->getLook() == LOOK_BLONDE)
			look += "0";
		if (s->getLook() == LOOK_BROWNHAIR)
			look += "1";
		if (s->getLook() == LOOK_ORIENTAL)
			look += "2";
		if (s->getLook() == LOOK_AFRICAN)
			look += "3";
		look += ".SPK";
		const std::set<std::string> &ufographContents = FileMap::getVFolderContents("UFOGRAPH");
		std::string lcaseLook = look;
		std::transform(lcaseLook.begin(), lcaseLook.end(), lcaseLook.begin(), tolower);
		if (ufographContents.find("lcaseLook") == ufographContents.end() && !_game->getMod()->getSurface(look, false))
		{
			look = s->getArmor()->getSpriteInventory() + ".SPK";
		}

		std::map<std::string, Uint8>::const_iterator armorColor = Game::getMod()->getSoldierUtileColors().find(s->getArmorColor());
		if (s->getArmor() && s->getArmor()->getUtileColorGroup() > 0 && (armorColor != Game::getMod()->getSoldierUtileColors().cend() || s->getArmor()->getDefaultUtileColor() >= 0))
		{
			_soldier->lock();
			std::pair<Uint8, Uint8> recolor = std::make_pair(s->getArmor()->getUtileColorGroup() << 4, (armorColor != Game::getMod()->getSoldierUtileColors().cend()) ? armorColor->second : s->getArmor()->getDefaultUtileColor());

			ShaderMove<Uint8> src(_game->getMod()->getSurface(look), _soldier->getX(), 0);

			if (unit->getEffectComponent(EC_STEALTH))
			{
				ShaderDraw<RecolorStealth>(ShaderSurface(_soldier), src, ShaderScalar(&recolor), ShaderScalar(1), ShaderCurrentPixel());
			}
			else
			{
				ShaderDraw<Recolor>(ShaderSurface(_soldier), src, ShaderScalar(&recolor), ShaderScalar(1));
			}
			_soldier->unlock();
		}
		else
		{
			_game->getMod()->getSurface(look)->blit(_soldier);
		}
	}
	else
	{
		_btnRole->setVisible(false);
		Surface *armorSurface = _game->getMod()->getSurface(unit->getArmor()->getSpriteInventory(), false);
		if (armorSurface)
		{
			armorSurface->blit(_soldier);
		}
	}

	if (s)
	{
		if (_armorColors && _armorColors->size())
		{
			_cmbArmorColor->setSelected(0);
			for (auto ii = _armorColors->cbegin(); ii != _armorColors->cend(); ++ii)
			{
				if (ii->second == s->getArmorColor())
				{
					_cmbArmorColor->setSelected(ii->first);
					break;
				}
			}
		}
	}

	if(_inv->getSelectedItem() || (_mouseOverItem && !_mouseOverItem->getOwner() && (_mouseOverItem != _inv->getSelectedItem()) && !(SDL_GetModState() & KMOD_CTRL)))
	{
		RuleItem *mouseOverRules = _inv->getSelectedItem() ? _inv->getSelectedItem()->getRules() : _mouseOverItem->getRules();
		int weight = mouseOverRules->getWeight();
		UnitStats stats = *mouseOverRules->getStats();
		UnitStats modifiers = *mouseOverRules->getStatModifiers();
		//UnitStats stats = unit->calculateStats(_inv->getSelectedItem());
		//unit->updateStats(_inv->getSelectedItem());
		//UnitStats stats = *unit->getBaseStats();
		_txtWeight->setText(tr("STR_WEIGHT").arg(weight).arg(stats.strength));
		_txtWeight->setSecondaryColor(_game->getMod()->getInterface("inventory")->getElement("weight")->color);
		if (weight > stats.strength)
		{
			_txtWeight->setSecondaryColor(_game->getMod()->getInterface("inventory")->getElement("weightHeavy")->color);
		}
		else if (weight > stats.strength / 2)
		{
			_txtWeight->setSecondaryColor(_game->getMod()->getInterface("inventory")->getElement("weightMedium")->color);
		}
		else
		{
			_txtWeight->setSecondaryColor(_game->getMod()->getInterface("inventory")->getElement("weight")->color);
		}
	
		_txtTus->setText(tr("STR_TIME_UNITS_SHORT").arg(Text::formatStatModifier(stats.tu, modifiers.tu)).arg("-"));
		_txtHealth->setText(tr("STR_HEALTH_SHORT").arg(Text::formatStatModifier(stats.health, modifiers.health)).arg("-"));

		if(mouseOverRules->getBattleType() == BT_FIREARM)
		{
			_txtReact->setText(tr("STR_REACTIONS_WEAP_SHORT").arg(Text::formatPercentageModifier(mouseOverRules->getReactionsModifier())).arg(Text::formatPercentageModifier(mouseOverRules->getOverwatchModifier())));
			_txtFAcc->setText(tr("STR_ACCURACY_WEAP_SHORT").arg(Text::formatPercentageModifier(mouseOverRules->getBaseAccuracy())).arg(Text::formatPercentageModifier(mouseOverRules->getAccuracyAimed())).arg(Text::formatPercentageModifier(mouseOverRules->getAccuracyAuto())).arg(Text::formatPercentageModifier(mouseOverRules->getAccuracySnap())));
			_txtTAcc->clear();
			_txtArmor->clear();
			_txtPSkill->clear();
			_txtPStr->clear();
		}
		else
		{
			_txtReact->setText(tr("STR_REACTIONS_SHORT").arg(Text::formatStatModifier(stats.reactions, modifiers.reactions)));
			_txtFAcc->setText(tr("STR_INV_ACCURACY_SHORT").arg(Text::formatStatModifier(stats.firing, modifiers.firing)));
			_txtTAcc->setText(tr("STR_THROWN_ACCURACY_SHORT").arg(Text::formatStatModifier(stats.throwing, modifiers.throwing)));
			_txtArmor->setText(tr("STR_INV_ARMOR_SHORT").arg(Text::formatModifier(mouseOverRules->getFrontArmor())).arg(Text::formatModifier(mouseOverRules->getSideArmor())).arg(Text::formatModifier(mouseOverRules->getSideArmor())).arg(Text::formatModifier(mouseOverRules->getRearArmor())).arg(Text::formatModifier(mouseOverRules->getUnderArmor())));
	
			if (!unit->isVehicle() && stats.psiSkill > 0)
			{
				_txtPSkill->setText(tr("STR_PSIONIC_SKILL_SHORT").arg(Text::formatStatModifier(stats.psiSkill, modifiers.psiSkill)));
			}
			else
			{
				_txtPSkill->setText(L"");
				//_txtPSkill->setText(tr("STR_PSIONIC_SKILL_SHORT").arg("??"));
			}

			if (!unit->isVehicle() && (stats.psiSkill > 0 || (Options::psiStrengthEval && _game->getSavedGame()->isResearched(_game->getMod()->getPsiRequirements()))))
			{
				_txtPStr->setText(tr("STR_PSIONIC_STRENGTH_SHORT").arg(Text::formatStatModifier(stats.psiStrength, modifiers.psiStrength)));
			}
			else
			{
				_txtPStr->setText(L"");
				//_txtPStr->setText(tr("STR_PSIONIC_STRENGTH_SHORT").arg("??"));
			}
		}
	}
	else
	{
		int weight = unit->getCarriedWeight(_inv->getSelectedItem());
		//UnitStats stats = unit->calculateStats(_inv->getSelectedItem());
		unit->updateStats(_inv->getSelectedItem());
		UnitStats stats = *unit->getBaseStats();
		_txtWeight->setText(tr("STR_WEIGHT").arg(weight).arg(stats.strength));
		if (weight > stats.strength)
		{
			_txtWeight->setSecondaryColor(_game->getMod()->getInterface("inventory")->getElement("weightHeavy")->color);
		}
		else if (weight > stats.strength / 2)
		{
			_txtWeight->setSecondaryColor(_game->getMod()->getInterface("inventory")->getElement("weightMedium")->color);
		}
		else
		{
			_txtWeight->setSecondaryColor(_game->getMod()->getInterface("inventory")->getElement("weight")->color);
		}
	
		_txtTus->setText(tr("STR_TIME_UNITS_SHORT").arg(unit->getTimeUnits()).arg(stats.tu));
		_txtHealth->setText(tr("STR_HEALTH_SHORT").arg(unit->getHealth()).arg(stats.health));
		_txtReact->setText(tr("STR_REACTIONS_SHORT").arg(stats.reactions));
		_txtFAcc->setText(tr("STR_INV_ACCURACY_SHORT").arg(stats.health < 1 ? 0 : (int)(stats.firing * unit->getHealth()) / stats.health));
		_txtTAcc->setText(tr("STR_THROWN_ACCURACY_SHORT").arg(stats.health < 1 ? 0 : (int)(stats.throwing * unit->getHealth()) / stats.health));

		_txtArmor->setText(tr("STR_INV_ARMOR_SHORT").arg(unit->getArmor(SIDE_FRONT)).arg(unit->getArmor(SIDE_LEFT)).arg(unit->getArmor(SIDE_RIGHT)).arg(unit->getArmor(SIDE_REAR)).arg(unit->getArmor(SIDE_UNDER)));
	
		if (!unit->isVehicle() && stats.psiSkill > 0)
		{
			_txtPSkill->setText(tr("STR_PSIONIC_SKILL_SHORT").arg(stats.psiSkill));
		}
		else
		{
			_txtPSkill->setText(L"");
			//_txtPSkill->setText(tr("STR_PSIONIC_SKILL_SHORT").arg("??"));
		}

		if (!unit->isVehicle() && (stats.psiSkill > 0 || (Options::psiStrengthEval && _game->getSavedGame()->isResearched(_game->getMod()->getPsiRequirements()))))
		{
			_txtPStr->setText(tr("STR_PSIONIC_STRENGTH_SHORT").arg(stats.psiStrength));
		}
		else
		{
			_txtPStr->setText(L"");
			//_txtPStr->setText(tr("STR_PSIONIC_STRENGTH_SHORT").arg("??"));
		}
	}
}

/**
 * Saves the soldiers' equipment-layout.
 */
void InventoryState::saveEquipmentLayout()
{
	for (std::vector<BattleUnit*>::iterator i = _battleGame->getUnits()->begin(); i != _battleGame->getUnits()->end(); ++i)
	{
		// we need X-Com soldiers only
		if ((*i)->getGeoscapeSoldier() == 0) continue;

		std::vector<EquipmentLayoutItem*> *layoutItems = (*i)->getGeoscapeSoldier()->getEquipmentLayout();

		// clear the previous save
		if (!layoutItems->empty())
		{
			for (std::vector<EquipmentLayoutItem*>::iterator j = layoutItems->begin(); j != layoutItems->end(); ++j)
				delete *j;
			layoutItems->clear();
		}

		// save the soldier's items
		// note: with using getInventory() we are skipping the ammos loaded, (they're not owned) because we handle the loaded-ammos separately (inside)
		for (std::vector<BattleItem*>::iterator j = (*i)->getInventory()->begin(); j != (*i)->getInventory()->end(); ++j)
		{
			std::string ammo;
			if ((*j)->needsAmmo() && 0 != (*j)->getAmmoItem()) ammo = (*j)->getAmmoItem()->getRules()->getType();
			else ammo = "NONE";
			layoutItems->push_back(new EquipmentLayoutItem(
				(*j)->getRules()->getType(),
				(*j)->getSlot()->getId(),
				(*j)->getSlotX(),
				(*j)->getSlotY(),
				ammo,
				(*j)->getFuseTimer()
			));
		}
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void InventoryState::btnOkClick(Action *)
{
	if (_inv->getSelectedItem() != 0)
		return;
	_game->popState();
	Tile *inventoryTile = _battleGame->getSelectedUnit()->getTile();
	if (!_tu)
	{
		saveEquipmentLayout();
		_battleGame->resetUnitTiles();
		if (_battleGame->getTurn() == 1)
		{
			_battleGame->randomizeItemLocations(inventoryTile);
			if (inventoryTile->getUnit())
			{
				// make sure we select the unit closest to the ramp.
				_battleGame->setSelectedUnit(inventoryTile->getUnit());
			}
		}

		// initialize xcom units for battle
		for (std::vector<BattleUnit*>::iterator j = _battleGame->getUnits()->begin(); j != _battleGame->getUnits()->end(); ++j)
		{
			if ((*j)->getOriginalFaction() != FACTION_PLAYER || (*j)->isOut())
			{
				continue;
			}

			(*j)->prepareNewTurn();
		}
	}
}

/**
 * Selects the previous soldier.
 * @param action Pointer to an action.
 */
void InventoryState::btnPrevClick(Action *)
{
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	if (_parent)
	{
		_parent->selectPreviousPlayerUnit(false, false, true);
	}
	else
	{
		_battleGame->selectPreviousPlayerUnit(false, false, true);
	}
	init();
}

/**
 * Selects the next soldier.
 * @param action Pointer to an action.
 */
void InventoryState::btnNextClick(Action *)
{
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}
	if (_parent)
	{
		_parent->selectNextPlayerUnit(false, false, true);
	}
	else
	{
		_battleGame->selectNextPlayerUnit(false, false, true);
	}
	init();
}

/**
 * Unloads the selected weapon.
 * @param action Pointer to an action.
 */
void InventoryState::btnUnloadClick(Action *)
{
	if (_inv->unload())
	{
		_txtItem->setText(L"");
		_txtAmmo->setText(L"");
		_selAmmo->clear();
		updateStats();
		_game->getMod()->getSoundByDepth(0, Mod::ITEM_DROP)->play();
	}
}

/**
 * Shows more ground items / rearranges them.
 * @param action Pointer to an action.
 */
void InventoryState::btnGroundClick(Action *)
{
	_inv->arrangeGround(true);
}

/**
 * Shows the unit info screen.
 * @param action Pointer to an action.
 */
void InventoryState::btnRankClick(Action *action)
{
	Soldier *ss;
	Craft *cc;
	Base *bb;
	if(action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_game->pushState(new UnitInfoState(_battleGame->getSelectedUnit(), _parent, true, false));
	}
	else if(action->getDetails()->button.button == SDL_BUTTON_RIGHT
			&& !_tu
			&& (ss = getSelectedSoldier())
			&& (cc = ss->getCraft())
			&& (bb = cc->getBase()))
	{
		int craft = 0;
		for(std::vector<Craft*>::const_iterator ii = bb->getCrafts()->begin(); ii != bb->getCrafts()->end(); ++ii)
		{
			if(cc == *ii)
			{
				_game->pushState(new CraftSoldiersState(bb, craft));
				return;
			}
			else
			{
				++craft;
			}
		}
	}
}

void InventoryState::btnRoleClick(Action *action)
{
	if(action->isMouseAction() && action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if(!_tu)
		{
			_game->pushState(new RoleMenuState(this));
		}
	}
	else if(action->isMouseAction() && action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		applyEquipmentLayout(getSelectedSoldier()->getRole()->getDefaultLayout(), getSelectedSoldier()->getRole()->getDefaultArmorColor());
	}
}

void InventoryState::cmbArmorColorChange(Action *action)
{
	if (Soldier *soldier = getSelectedSoldier())
	{
		soldier->setArmorColor((*_armorColors)[_cmbArmorColor->getSelected()]);
		updateStats();
	}
}

void InventoryState::btnCreateTemplateClick(Action *)
{
	// don't accept clicks when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	// clear current template
	_clearInventoryTemplate(_curInventoryTemplate);

	getUnitEquipmentLayout(&_curInventoryTemplate, _curInventoryTemplateArmorColor);

	// give audio feedback
	_game->getMod()->getSound("BATTLE.CAT", 38)->play();
	_refreshMouse();
}

void InventoryState::btnApplyTemplateClick(Action *action)
{
	// don't accept clicks when moving items or when the template is empty
	if (_inv->getSelectedItem() != 0 || _curInventoryTemplate.empty())
	{
		return;
	}

	applyEquipmentLayout(_curInventoryTemplate, _curInventoryTemplateArmorColor);

	// give audio feedback
	_game->getMod()->getSound("BATTLE.CAT", 38)->play();
}

/// Gets the selected unit's equipment layout.
void InventoryState::getUnitEquipmentLayout(std::vector<EquipmentLayoutItem*> *layout, std::string &armorColor) const
{
	// copy inventory instead of just keeping a pointer to it.  that way
	// create/apply can be used as an undo button for a single unit and will
	// also work as expected if inventory is modified after 'create' is clicked
	std::vector<BattleItem*> *unitInv = _battleGame->getSelectedUnit()->getInventory();
	for (std::vector<BattleItem*>::iterator j = unitInv->begin(); j != unitInv->end(); ++j)
	{
		if ((*j)->getRules()->isFixed()) {
			// don't copy fixed weapons into the template
			continue;
		}

		std::string ammo;
		if ((*j)->needsAmmo() && (*j)->getAmmoItem())
		{
			ammo = (*j)->getAmmoItem()->getRules()->getType();
		}
		else
		{
			ammo = "NONE";
		}

		layout->push_back(new EquipmentLayoutItem(
				(*j)->getRules()->getType(),
				(*j)->getSlot()->getId(),
				(*j)->getSlotX(),
				(*j)->getSlotY(),
				ammo,
				(*j)->getFuseTimer()));
	}

	if (_battleGame->getSelectedUnit()->getGeoscapeSoldier())
	{
		armorColor = _battleGame->getSelectedUnit()->getGeoscapeSoldier()->getArmorColor();
	}
	else
	{
		armorColor = "";
	}

	// give audio feedback
	_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
	_refreshMouse();
}

static void _clearInventory(Game *game, std::vector<BattleItem*> *unitInv, Tile *groundTile)
{
	RuleInventory *groundRuleInv = game->getMod()->getInventory("STR_GROUND", true);

	// clear unit's inventory (i.e. move everything to the ground)
	for (std::vector<BattleItem*>::iterator i = unitInv->begin(); i != unitInv->end(); )
	{
		if ((*i)->getRules()->isFixed()) {
			// don't drop fixed weapons
			++i;
			continue;
		}
		BattleItem *item = *i;
		item->setOwner(NULL);
		item->setFuseTimer(BattleItem::GRENADE_INACTIVE);
		groundTile->addItem(item, groundRuleInv);
		i = unitInv->erase(i);
	}
}

void InventoryState::applyEquipmentLayout(const std::vector<EquipmentLayoutItem*> &layout, const std::string &armorColor, bool ignoreEmpty)
{
	// don't accept clicks when moving items
	// it's ok if the template is empty -- it will just result in clearing the
	// unit's inventory
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	BattleUnit               *unit          = _battleGame->getSelectedUnit();
	std::vector<BattleItem*> *unitInv       = unit->getInventory();
	Tile                     *groundTile    = unit->getTile();
	std::vector<BattleItem*> *groundInv     = groundTile->getInventory();
	RuleInventory            *groundRuleInv = _game->getMod()->getInventory("STR_GROUND", true);

	_clearInventory(_game, unitInv, groundTile);

	if (unit->getGeoscapeSoldier() && armorColor.size())
	{
		unit->getGeoscapeSoldier()->setArmorColor(armorColor);
	}

	if (!ignoreEmpty || layout.size())
	{
		// attempt to replicate inventory template by grabbing corresponding items
		// from the ground.  if any item is not found on the ground, display warning
		// message, but continue attempting to fulfill the template as best we can
		bool itemMissing = false;
		std::vector<EquipmentLayoutItem*>::const_iterator templateIt;

		RuleInventoryLayout *invLayout = _game->getMod()->getInventoryLayout(unit->getInventoryLayout());

		for (templateIt = layout.begin(); templateIt != layout.end(); ++templateIt)
		{
			if (!invLayout || invLayout->hasSlot((*templateIt)->getSlot()))
			{
				// search for template item in ground inventory
				std::vector<BattleItem*>::iterator groundItem;
				const bool needsAmmo = !_game->getMod()->getItem((*templateIt)->getItemType(), true)->getCompatibleAmmo()->empty();
				bool found = false;
				bool rescan = true;
				while (rescan)
				{
					rescan = false;

					const std::string targetAmmo = (*templateIt)->getAmmoItem();
					BattleItem *matchedWeapon = NULL;
					BattleItem *matchedAmmo = NULL;
					for (groundItem = groundInv->begin(); groundItem != groundInv->end(); ++groundItem)
					{
						// if we find the appropriate ammo, remember it for later for if we find
						// the right weapon but with the wrong ammo
						const std::string groundItemName = (*groundItem)->getRules()->getType();
						if (needsAmmo && targetAmmo == groundItemName)
						{
							matchedAmmo = *groundItem;
						}

						if ((*templateIt)->getItemType() == groundItemName)
						{
							// if the loaded ammo doesn't match the template item's,
							// remember the weapon for later and continue scanning
							BattleItem *loadedAmmo = (*groundItem)->getAmmoItem();
							if ((needsAmmo && loadedAmmo && targetAmmo != loadedAmmo->getRules()->getType())
								|| (needsAmmo && !loadedAmmo))
							{
								// remember the last matched weapon for simplicity (but prefer empty weapons if any are found)
								if (!matchedWeapon || matchedWeapon->getAmmoItem())
								{
									matchedWeapon = *groundItem;
								}
								continue;
							}

							// move matched item from ground to the appropriate inv slot
							(*groundItem)->setOwner(unit);
							(*groundItem)->setSlot(_game->getMod()->getInventory((*templateIt)->getSlot(), true));
							(*groundItem)->setSlotX((*templateIt)->getSlotX());
							(*groundItem)->setSlotY((*templateIt)->getSlotY());
							(*groundItem)->setFuseTimer((*templateIt)->getFuseTimer());
							unitInv->push_back(*groundItem);
							groundInv->erase(groundItem);
							found = true;
							break;
						}
					}

					// if we failed to find an exact match, but found unloaded ammo and
					// the right weapon, unload the target weapon, load the right ammo, and use it
					if (!found && matchedWeapon && (!needsAmmo || matchedAmmo))
					{
						// unload the existing ammo (if any) from the weapon
						BattleItem *loadedAmmo = matchedWeapon->getAmmoItem();
						if (loadedAmmo)
						{
							groundTile->addItem(loadedAmmo, groundRuleInv);
							matchedWeapon->setAmmoItem(NULL);
						}

						// load the correct ammo into the weapon
						if (matchedAmmo)
						{
							matchedWeapon->setAmmoItem(matchedAmmo);
							groundTile->removeItem(matchedAmmo);
						}

						// rescan and pick up the newly-loaded/unloaded weapon
						rescan = true;
					}
				}

				if (!found)
				{
					itemMissing = true;
				}
			}
		}

		if (itemMissing)
		{
			_inv->showWarning(tr("STR_NOT_ENOUGH_ITEMS_FOR_TEMPLATE"));
		}
	}

	// refresh ui
	_inv->arrangeGround(false);
	updateStats();
	_refreshMouse();

	// give audio feedback
	_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
}

void InventoryState::_refreshMouse() const
{
	// send a mouse motion event to refresh any hover actions
	int x, y;
	SDL_GetMouseState(&x, &y);
	SDL_WarpMouse(x+1, y);

	// move the mouse back to avoid cursor creep
	SDL_WarpMouse(x, y);
}

void InventoryState::onClearInventory(Action *)
{
	// don't act when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	BattleUnit               *unit       = _battleGame->getSelectedUnit();
	std::vector<BattleItem*> *unitInv    = unit->getInventory();
	Tile                     *groundTile = unit->getTile();

	_clearInventory(_game, unitInv, groundTile);

	// refresh ui
	_inv->arrangeGround(false);
	updateStats();
	_refreshMouse();

	// give audio feedback
	_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
}

void InventoryState::onAutoequip(Action *)
{
	// don't act when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	BattleUnit               *unit          = _battleGame->getSelectedUnit();
	Tile                     *groundTile    = unit->getTile();
	std::vector<BattleItem*> *groundInv     = groundTile->getInventory();
	Mod                      *mod           = _game->getMod();
	RuleInventory            *groundRuleInv = mod->getInventory("STR_GROUND", true);
	int                       worldShade    = _battleGame->getGlobalShade();

	std::vector<BattleUnit*> units;
	units.push_back(unit);
	BattlescapeGenerator::autoEquip(units, mod, NULL, groundInv, groundRuleInv, worldShade, true, true);

	// refresh ui
	_inv->arrangeGround(false);
	updateStats();
	_refreshMouse();

	// give audio feedback
	_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
}

/**
 * Updates item info.
 * @param action Pointer to an action.
 */
void InventoryState::invClick(Action *)
{
	updateStats();
}

/**
 * Shows item info.
 * @param action Pointer to an action.
 */
void InventoryState::invMouseOver(Action *action)
{
	if (_mouseOverItem != _inv->getMouseOverItem())
	{
		_mouseOverItem = _inv->getMouseOverItem();
		updateStats();
	}

	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	BattleItem *item = _inv->getMouseOverItem();
	if (item != 0)
	{
		BattleUnit *bu = item->getUnit();
		if (bu && bu->getFaction() == FACTION_PLAYER)
		{
			if (bu->getStatus() == STATUS_DEAD)
			{
				_txtItem->setText(tr("STR_CORPSE_ITEM").arg(bu->getName(_game->getLanguage())).arg(tr("STR_CORPSE")));
			}
			else if (bu->getBleedingOut())
			{
				_txtItem->setText(tr("STR_CORPSE_ITEM").arg(bu->getName(_game->getLanguage())).arg(tr("STR_MEDIKIT_BLEEDING")));
			}
			else if (bu->getHealth() <= 0)
			{
				_txtItem->setText(tr("STR_CORPSE_ITEM").arg(bu->getName(_game->getLanguage())).arg(tr("STR_MEDIKIT_STABILIZED")));
			}
			else if (bu->getStunlevel() >= bu->getHealth())
			{
				_txtItem->setText(tr("STR_CORPSE_ITEM").arg(bu->getName(_game->getLanguage())).arg(tr("STR_MEDIKIT_STUNNED")));
			}
			else
			{
				// Shouldn't hit this?
				_txtItem->setText(tr("STR_CORPSE_ITEM").arg(bu->getName(_game->getLanguage())).arg("?"));
			}
		}
		else if(bu && bu->getStatus() == STATUS_UNCONSCIOUS)
		{
			_txtItem->setText(bu->getName(_game->getLanguage()));
		}
		else if (_game->getSavedGame()->isResearched(item->getRules()->getRequirements()))
		{
			_txtItem->setText(tr(item->getRules()->getName()));
		}
		else
		{
			_txtItem->setText(tr("STR_ALIEN_ARTIFACT"));
		}

		std::wstring s;
		if (item->getAmmoItem() != 0 && item->needsAmmo())
		{
			s = tr("STR_AMMO_ROUNDS_LEFT").arg(item->getAmmoItem()->getAmmoQuantity());
			SDL_Rect r;
			r.x = 0;
			r.y = 0;
			r.w = RuleInventory::HAND_W * RuleInventory::SLOT_W;
			r.h = RuleInventory::HAND_H * RuleInventory::SLOT_H;
			_selAmmo->drawRect(&r, _game->getMod()->getInterface("inventory")->getElement("grid")->color);
			r.x++;
			r.y++;
			r.w -= 2;
			r.h -= 2;
			//r.x -= (RuleInventory::MAX_W - RuleInventory::HAND_W / 2) * RuleInventory::SLOT_W;
			_selAmmo->drawRect(&r, Palette::blockOffset(0)+15);
			item->getAmmoItem()->getRules()->drawHandSprite(_game->getMod()->getSurfaceSet("BIGOBS.PCK"), _selAmmo);
			_updateTemplateButtons(false);
		}
		else
		{
			_selAmmo->clear();
			_updateTemplateButtons(!_tu);
		}
		if (item->getAmmoQuantity() != 0 && item->needsAmmo())
		{
			s = tr("STR_AMMO_ROUNDS_LEFT").arg(item->getAmmoQuantity());
		}
		else if (item->getRules()->getBattleType() == BT_MEDIKIT)
		{
			s = tr("STR_MEDI_KIT_QUANTITIES_LEFT").arg(item->getPainKillerQuantity()).arg(item->getStimulantQuantity()).arg(item->getHealQuantity());
		}
		_txtAmmo->setText(s);
	}
	else
	{
		/*std::string toolTip = action->getSender()->getTooltip();

		if(toolTip.length())
		{
			_txtItem->setText(tr(toolTip));
		}
		else*/

		if (_currentTooltip.empty())
		{
			_txtItem->setText(L"");
		}
		_txtAmmo->setText(L"");
		_selAmmo->clear();
		_updateTemplateButtons(!_tu);
	}
}

/**
 * Hides item info.
 * @param action Pointer to an action.
 */
void InventoryState::invMouseOut(Action *)
{
	_txtItem->setText(L"");
	_txtAmmo->setText(L"");
	_selAmmo->clear();
	_updateTemplateButtons(!_tu);
}

/**
 * Takes care of any events from the core game engine.
 * @param action Pointer to an action.
 */
void InventoryState::handle(Action *action)
{
	State::handle(action);


#ifndef __MORPHOS__
	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_X1)
		{
			btnNextClick(action);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_X2)
		{
			btnPrevClick(action);
		}
	}
#endif
}

/**
 * Shows a tooltip for the appropriate button.
 * @param action Pointer to an action.
 */
void InventoryState::txtTooltipIn(Action *action)
{
	if (_inv->getSelectedItem() == 0 && Options::battleTooltips)
	{
		_currentTooltip = action->getSender()->getTooltip();
		_txtItem->setText(tr(_currentTooltip));
	}
}

/**
 * Clears the tooltip text.
 * @param action Pointer to an action.
 */
void InventoryState::txtTooltipOut(Action *action)
{
	if (_inv->getSelectedItem() == 0 && Options::battleTooltips)
	{
		if (_currentTooltip == action->getSender()->getTooltip())
		{
			_currentTooltip = "";
			_txtItem->setText(L"");
		}
	}
}

void InventoryState::_updateTemplateButtons(bool isVisible)
{
	if (isVisible)
	{
		if (_curInventoryTemplate.empty())
		{
			// use "empty template" icons
			_game->getMod()->getSurface("InvCopy")->blit(_btnCreateTemplate);
			_game->getMod()->getSurface("InvPasteEmpty")->blit(_btnApplyTemplate);
			_btnApplyTemplate->setTooltip("STR_CLEAR_INVENTORY");
		}
		else
		{
			// use "active template" icons
			_game->getMod()->getSurface("InvCopyActive")->blit(_btnCreateTemplate);
			_game->getMod()->getSurface("InvPaste")->blit(_btnApplyTemplate);
			_btnApplyTemplate->setTooltip("STR_APPLY_INVENTORY_TEMPLATE");
		}
		_btnCreateTemplate->initSurfaces();
		_btnApplyTemplate->initSurfaces();
	}
	else
	{
		_btnCreateTemplate->clear();
		_btnApplyTemplate->clear();
	}
}

/// Gets the current battle unit.
BattleUnit *InventoryState::getSelectedUnit() const
{
	return _battleGame->getSelectedUnit();
}


/// Gets the current soldier.
Soldier *InventoryState::getSelectedSoldier() const
{
	return getSelectedUnit()->getGeoscapeSoldier();
}

/// Change the current soldier's role.
void InventoryState::setRole(const std::string &role)
{
	Soldier *s = getSelectedSoldier();
	Role *r = _game->getSavedGame()->getRole(role);
	s->setRole(r);

	if(!r->isBlank())
	{
		applyEquipmentLayout(r->getDefaultLayout(), r->getDefaultArmorColor(), true);
	}
	updateStats();
}

void InventoryState::onCtrlToggled(Action *action)
{
	updateStats();
}

}
