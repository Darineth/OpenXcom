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
#include "../Interface/TextButton.h"
#include "../Savegame/EquipmentLayoutItem.h"


namespace OpenXcom
{

class Surface;
class Text;
class InteractiveSurface;
class Inventory;
class SavedBattleGame;
class BattlescapeState;
class BattleUnit;
class BattlescapeButton;
class Soldier;
class BattleItem;

/**
 * Screen which displays soldier's inventory.
 */
class InventoryState : public State
{
private:
	Surface *_bg, *_soldier;
	Text *_txtName, *_txtRole, *_txtItem, *_txtAmmo, *_txtWeight, *_txtTus, *_txtHealth, *_txtReact, *_txtFAcc, *_txtTAcc, *_txtPSkill, *_txtPStr, *_txtArmor;
	BattlescapeButton *_btnOk, *_btnPrev, *_btnNext, *_btnUnload, *_btnGround, *_btnRank, *_btnRole;
	BattlescapeButton *_btnCreateTemplate, *_btnApplyTemplate;
	Surface *_selAmmo;
	Inventory *_inv;
	BattleItem *_mouseOverItem;
	std::vector<EquipmentLayoutItem*> _curInventoryTemplate;
	std::string _curInventoryTemplateArmorColor;
	SavedBattleGame *_battleGame;
	const bool _tu;
	BattlescapeState *_parent;
	std::string _currentTooltip;
	ComboBox *_cmbArmorColor;
	std::map<int, std::string> *_armorColors;
public:
	/// Creates the Inventory state.
	InventoryState(bool tu, BattlescapeState *parent);
	/// Cleans up the Inventory state.
	~InventoryState();
	/// Updates all soldier info.
	void init();
	/// Updates the soldier info (Weight, TU).
	void updateStats();
	/// Saves the soldiers' equipment-layout.
	void saveEquipmentLayout();
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handler for clicking the Previous button.
	void btnPrevClick(Action *action);
	/// Handler for clicking the Next button.
	void btnNextClick(Action *action);
	/// Handler for clicking the Unload button.
	void btnUnloadClick(Action *action);
	/// Handler for clicking on the Ground -> button.
	void btnGroundClick(Action *action);
	/// Handler for clicking the Rank button.
	void btnRankClick(Action *action);
	/// Handler for clicking on the Role button.
	void btnRoleClick(Action *action);
	/// Handler for changing armor color.
	void cmbArmorColorChange(Action *action);
	/// Handler for clicking on the Create Template button.
	void btnCreateTemplateClick(Action *action);
	/// Handler for clicking the Apply Template button.
	void btnApplyTemplateClick(Action *action);
	/// Handler for hitting the Clear Inventory hotkey.
	void onClearInventory(Action *action);
	/// Handler for hitting the Autoequip hotkey.
	void onAutoequip(Action *action);
	/// Gets the selected unit's equipment layout.
	void InventoryState::getUnitEquipmentLayout(std::vector<EquipmentLayoutItem*> *layout, std::string &armorColor) const;
	/// Apply an equipment layout to the current unit.
	void applyEquipmentLayout(const std::vector<EquipmentLayoutItem*> &layout, const std::string &armorColor, bool ignoreEmpty = false);
	/// Handler for clicking on the inventory.
	void invClick(Action *action);
	/// Handler for showing item info.
	void invMouseOver(Action *action);
	/// Handler for hiding item info.
	void invMouseOut(Action *action);
	/// Handles keypresses.
	void handle(Action *action);
	/// Handler for showing tooltip.
	void txtTooltipIn(Action *action);
	/// Handler for hiding tooltip.
	void txtTooltipOut(Action *action);
	/// Gets the current battle unit.
	BattleUnit *getSelectedUnit() const;
	/// Gets the current soldier.
	Soldier *getSelectedSoldier() const;
	/// Change the current soldier's role.
	void setRole(const std::string &role);
	/// Handler for CTRL being pressed/released
	void onCtrlToggled(Action *action);

private:
	/// Update the visibility and icons for the template buttons
	void _updateTemplateButtons(bool isVisible);
	/// Refresh the hover status of the mouse
	void _refreshMouse() const;
};

}
