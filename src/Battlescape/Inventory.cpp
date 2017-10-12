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
#include "Inventory.h"
#include <algorithm>
#include <cmath>
#include "../Mod/Mod.h"
#include "../Mod/RuleInventory.h"
#include "../Mod/RuleInterface.h"
#include "../Engine/Game.h"
#include "../Engine/Timer.h"
#include "../Interface/Text.h"
#include "../Interface/NumberText.h"
#include "../Engine/Font.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Engine/SurfaceSet.h"
#include "../Savegame/BattleItem.h"
#include "../Mod/RuleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Engine/Action.h"
#include "../Engine/Sound.h"
#include "../Engine/Script.h"
#include "WarningMessage.h"
#include "../Savegame/Tile.h"
#include "PrimeGrenadeState.h"
#include <algorithm>
#include "../Ufopaedia/Ufopaedia.h"
#include <unordered_map>
#include "../Engine/Screen.h"
#include "../Mod/RuleInventoryLayout.h"

namespace OpenXcom
{

/**
 * Sets up an inventory with the specified size and position.
 * @param game Pointer to core game.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 * @param base Is the inventory being called from the basescape?
 */
Inventory::Inventory(Game *game, int width, int height, int x, int y, bool base) : InteractiveSurface(width, height, x, y), _game(game), _selUnit(0), _selItem(0), _tu(true), _base(base), _mouseOverItem(0), _groundOffset(0), _animFrame(0), _locked(false)
{
	_depth = _game->getSavedGame()->getSavedBattle()->getDepth();
	_grid = new Surface(width, height, 0, 0);
	_items = new Surface(width, height, 0, 0);
	_selection = new Surface(RuleInventory::MAX_W * RuleInventory::SLOT_W, RuleInventory::MAX_H * RuleInventory::SLOT_H, x, y);
	_warning = new WarningMessage(224, 24, 48, 176);
	_stackNumber = new NumberText(15, 15, 0, 0);
	_stackNumber->setBordered(true);
	_ammoNumber = new NumberText(15, 15, 0, 0);
	_ammoNumber->setBordered(true);

	_warning->initText(_game->getMod()->getFont("FONT_BIG"), _game->getMod()->getFont("FONT_SMALL"), _game->getLanguage());
	_warning->setColor(_game->getMod()->getInterface("battlescape")->getElement("warning")->color2);
	_warning->setTextColor(_game->getMod()->getInterface("battlescape")->getElement("warning")->color);

	_animTimer = new Timer(100);
	_animTimer->onTimer((SurfaceHandler)&Inventory::animate);
	_animTimer->start();

	_stunIndicator = _game->getMod()->getSurface("BigStunIndicator", false);
	_woundIndicator = _game->getMod()->getSurface("BigWoundIndicator", false);
	_burnIndicator = _game->getMod()->getSurface("BigBurnIndicator", false);

	_inventorySlotRightHand = _game->getMod()->getInventory("STR_RIGHT_HAND", true);
	_inventorySlotLeftHand = _game->getMod()->getInventory("STR_LEFT_HAND", true);
	_inventorySlotBackPack = _game->getMod()->getInventory("STR_BACK_PACK", true);
	_inventorySlotBelt = _game->getMod()->getInventory("STR_BELT", true);
	_inventorySlotGround = _game->getMod()->getInventory("STR_GROUND", true);

	_inventoryLayout = _game->getMod()->getInventoryLayout("STR_STANDARD_INV");
}

/**
 * Deletes inventory surfaces.
 */
Inventory::~Inventory()
{
	delete _grid;
	delete _items;
	delete _selection;
	delete _warning;
	delete _stackNumber;
	delete _ammoNumber;
	delete _animTimer;
}

/**
 * Replaces a certain amount of colors in the inventory's palette.
 * @param colors Pointer to the set of colors.
 * @param firstcolor Offset of the first color to replace.
 * @param ncolors Amount of colors to replace.
 */
void Inventory::setPalette(SDL_Color *colors, int firstcolor, int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	_grid->setPalette(colors, firstcolor, ncolors);
	_items->setPalette(colors, firstcolor, ncolors);
	_selection->setPalette(colors, firstcolor, ncolors);
	_warning->setPalette(colors, firstcolor, ncolors);
	_stackNumber->setPalette(getPalette());
	_ammoNumber->setPalette(getPalette());
}

/**
 * Changes the inventory's Time Units mode.
 * When True, inventory actions cost soldier time units (for battle).
 * When False, inventory actions don't cost anything (for pre-equip).
 * @param tu Time Units mode.
 */
void Inventory::setTuMode(bool tu)
{
	_tu = tu;
}

/**
 * Returns the currently selected (i.e. displayed) unit.
 * @return Pointer to selected unit, or 0 if none.
 */
BattleUnit *Inventory::getSelectedUnit() const
{
	return _selUnit;
}

/**
 * Changes the unit to display the inventory of.
 * @param unit Pointer to battle unit.
 */
void Inventory::setSelectedUnit(BattleUnit *unit)
{
	RuleInventoryLayout *newLayout = _game->getMod()->getInventoryLayout(unit ? unit->getInventoryLayout() : "STR_STANDARD_INV");
	bool arrange = !_selUnit || (_selUnit->isVehicle() != unit->isVehicle()) || (newLayout != _inventoryLayout);
	_selUnit = unit;
	_inventoryLayout = newLayout;
	if (arrange)
	{
		_groundOffset = 999;
		drawGrid();
		arrangeGround();
	}
	else
	{
		drawItems();
	}
}

/**
 * Draws the inventory elements.
 */
void Inventory::draw()
{
	drawGrid();
	drawItems();
}

/**
 * Draws the inventory grid for item placement.
 */
void Inventory::drawGrid()
{
	_grid->clear();
	Text text(80, 18, 0, 0);
	text.setPalette(_grid->getPalette());
	text.initText(_game->getMod()->getFont("FONT_BIG"), _game->getMod()->getFont("FONT_SMALL"), _game->getLanguage());

	RuleInterface *interface = _game->getMod()->getInterface("inventory");
	Element *grid = interface->getElement("grid");
	Element *gridOk = interface->getElement("gridOk");
	Element *gridError = interface->getElement("gridError");
	Element *gridReload = interface->getElement("gridReload");
	text.setHighContrast(true);

	Uint8 color = interface->getElement("grid")->color;

	std::string dummy;

	for (std::vector<RuleInventory*>::const_iterator ii = _inventoryLayout->getSlots()->begin(); ii != _inventoryLayout->getSlots()->end(); ++ii)
	{
		RuleInventory *rule = *ii;

		Uint8 colorGrid = color;
		Uint8 colorBackground = 0;
		bool canMove = false;
		int moveCost = 0;
		if (_selItem)
		{
			canMove = _selUnit->moveItem(_selItem, rule, -1, -1, dummy, false, true, &moveCost);
			colorGrid = canMove ? gridOk->color : gridError->color;
			colorBackground = canMove ? gridOk->color2 : gridError->color2;
			if (moveCost < 0)
			{
				moveCost = 0;
			}
		}

		bool reloadable = false;
		int reloadCost = 0;
		int slotWidth;
		// Draw grid
		switch (rule->getType())
		{
			case INV_SLOT:
			{
				slotWidth = 0;
				for (std::vector<RuleSlot>::iterator j = rule->getSlots()->begin(); j != rule->getSlots()->end(); ++j)
				{
					SDL_Rect r;
					r.x = rule->getX() + RuleInventory::SLOT_W * j->x;
					r.y = rule->getY() + RuleInventory::SLOT_H * j->y;
					r.w = RuleInventory::SLOT_W + 1;
					r.h = RuleInventory::SLOT_H + 1;
					_grid->drawRect(&r, colorGrid);
					r.x++;
					r.y++;
					r.w -= 2;
					r.h -= 2;
					_grid->drawRect(&r, colorBackground);

					slotWidth = std::max(slotWidth, r.x + r.w - rule->getX());
				}
			}
			break;
			case INV_HAND:
			case INV_UTILITY:
			case INV_EQUIP:
			{
				if (_selUnit)
				{
					BattleItem *weapon = _selUnit->getItem(rule->getId());
					if (_selItem && weapon && weapon->haveAnyAmmo() && !weapon->haveFullAmmo() && _selUnit->loadWeapon(weapon, _selItem, dummy, false, true, &reloadCost))
					{
						reloadable = true;
						colorGrid = gridReload->color;
						colorBackground = gridReload->color2;
					}
				}
				SDL_Rect r;
				r.x = rule->getX();
				r.y = rule->getY();
				switch (rule->getType())
				{
					case INV_HAND:
						r.w = RuleInventory::HAND_W * RuleInventory::SLOT_W + 1;
						r.h = RuleInventory::HAND_H * RuleInventory::SLOT_H + 1;
						break;
					case INV_UTILITY:
						r.w = RuleInventory::UTILITY_W * RuleInventory::SLOT_W + 1;
						r.h = RuleInventory::UTILITY_H * RuleInventory::SLOT_H + 1;
						break;
					case INV_EQUIP:
						r.w = rule->getWidth() * RuleInventory::SLOT_W + 1;;
						r.h = rule->getHeight() * RuleInventory::SLOT_H + 1;;
						break;
				}
				slotWidth = r.w;
				_grid->drawRect(&r, colorGrid);
				r.x++;
				r.y++;
				r.w -= 2;
				r.h -= 2;
				_grid->drawRect(&r, colorBackground);
			}
			break;
			case INV_GROUND:
			{
				for (int x = rule->getX(); x <= 320; x += RuleInventory::SLOT_W)
				{
					for (int y = rule->getY(); y <= 200; y += RuleInventory::SLOT_H)
					{
						SDL_Rect r;
						r.x = x;
						r.y = y;
						r.w = RuleInventory::SLOT_W + 1;
						r.h = RuleInventory::SLOT_H + 1;
						_grid->drawRect(&r, colorGrid);
						r.x++;
						r.y++;
						r.w -= 2;
						r.h -= 2;
						_grid->drawRect(&r, colorBackground);
					}
				}
				slotWidth = 0;
			}
			break;
		}

		// Draw label
		if (_selItem && _selItem->getSlot() && (rule != _selItem->getSlot()))
		{
			if (reloadable)
			{
				text.setText(_game->getLanguage()->getString("STR_RELOAD_COST").arg(reloadCost).arg(_selUnit->getTimeUnits()));
			}
			else if (!canMove && moveCost <= 0)
			{
				text.setText(L"-");
			}
			else //if(_tu)
			{
				text.setText(_game->getLanguage()->getString("STR_TIME_UNITS_SHORT").arg(moveCost).arg(_selUnit->getTimeUnits()));
			}
			/*else
			{
				text.setText(_game->getLanguage()->getString(rule->getId()));
			}*/
		}
		else
		{
			text.setText(_game->getLanguage()->getString(rule->getId()));
		}

		if (rule->getAllowCombatSwap())
		{
			text.setColor(interface->getElement("textSlots")->color);
		}
		else
		{
			text.setColor(interface->getElement("textSlots")->color2);
		}

		text.setWidth(text.getTextWidth() + 1);

		switch ((TextHAlign)rule->getTextAlign())
		{
			case ALIGN_LEFT:
				text.setAlign(ALIGN_LEFT);
				text.setX(rule->getX());
				break;
			case ALIGN_CENTER:
				text.setAlign(ALIGN_CENTER);
				text.setX(rule->getX() + ((slotWidth - text.getWidth()) / 2));
				break;
			case ALIGN_RIGHT:
				text.setAlign(ALIGN_RIGHT);
				text.setX(rule->getX() + slotWidth - text.getWidth());
				break;
		}

		text.setY(rule->getY() - text.getFont()->getHeight() - text.getFont()->getSpacing());

		if (text.getText().find(L'\n') != std::wstring::npos)
		{
			text.setY(text.getY() - text.getFont()->getHeight() - text.getFont()->getSpacing());
		}
		text.blit(_grid);
	}
}

/**
 * Draws the items contained in the soldier's inventory.
 */
void Inventory::drawItems()
{
	ScriptWorkerBlit work;
	_items->clear();
	_grenadeIndicators.clear();
	_stunnedIndicators.clear();
	_woundedIndicators.clear();
	_burningIndicators.clear();
	Uint8 color = _game->getMod()->getInterface("inventory")->getElement("numStack")->color;
	Uint8 color2 = _game->getMod()->getInterface("inventory")->getElement("numStack")->color2;
	Element *textAmmo = _game->getMod()->getInterface("inventory")->getElement("textAmmo");
	if (_selUnit != 0)
	{
		SurfaceSet *texture = _game->getMod()->getSurfaceSet("BIGOBS.PCK");
		// Soldier items
		for (std::vector<BattleItem*>::iterator i = _selUnit->getInventory()->begin(); i != _selUnit->getInventory()->end(); ++i)
		{
			Surface *frame = (*i)->getBigSprite(texture);

			BattleItem *item = *i;
			if (item == _selItem || !frame)
				continue;

			//bool twoHanded = false;

			int x, y;
			switch (item->getSlot()->getType())
			{
				case INV_SLOT:
					x = (item->getSlot()->getX() + item->getSlotX() * RuleInventory::SLOT_W);
					y = (item->getSlot()->getY() + item->getSlotY() * RuleInventory::SLOT_H);
					//twoHanded = false;
					break;
				case INV_HAND:
					x = (item->getSlot()->getX() + (RuleInventory::HAND_W - item->getRules()->getInventoryWidth()) * RuleInventory::SLOT_W / 2);
					y = (item->getSlot()->getY() + (RuleInventory::HAND_H - item->getRules()->getInventoryHeight()) * RuleInventory::SLOT_H / 2);
					//twoHanded = item->getRules()->isTwoHanded();
					break;
				case INV_UTILITY:
					x = (item->getSlot()->getX() + (RuleInventory::UTILITY_W - item->getRules()->getInventoryWidth()) * RuleInventory::SLOT_W / 2);
					y = (item->getSlot()->getY() + (RuleInventory::UTILITY_H - item->getRules()->getInventoryHeight()) * RuleInventory::SLOT_H / 2);
					break;
				case INV_EQUIP:
					x = (item->getSlot()->getX() + (item->getSlot()->getWidth() - item->getRules()->getInventoryWidth()) * RuleInventory::SLOT_W / 2);
					y = (item->getSlot()->getY() + (item->getSlot()->getHeight() - item->getRules()->getInventoryHeight()) * RuleInventory::SLOT_H / 2);
					break;
			}
			BattleItem::ScriptFill(&work, *i, BODYPART_ITEM_INVENTORY, _animFrame, 0);
			work.executeBlit(frame, _items, x, y, 0);

			//texture->getFrame(item->getRules()->getBigSprite())->blit(_items);

			// grenade primer indicators
			if (item->getGrenadeLive())
			{
				_grenadeIndicators.push_back(std::make_pair(x, y));
			}

			/*if(twoHanded)
			{
				std::string otherHandSlot = item->getSlot()->getId() == _selUnit->getWeaponSlot1() ? _selUnit->getWeaponSlot2() : _selUnit->getWeaponSlot1();
				BattleItem *otherHandItem = _selUnit->getItem(otherHandSlot);
				RuleInventory *slot = Game::getMod()->getInventory(otherHandSlot);
				if(slot && !otherHandItem)
				{
					frame->setX(slot->getX() + (RuleInventory::HAND_W - item->getRules()->getInventoryWidth()) * RuleInventory::SLOT_W/2);
					frame->setY(slot->getY() + (RuleInventory::HAND_H - item->getRules()->getInventoryHeight()) * RuleInventory::SLOT_H/2);
					Surface *drawFrame = texture->getFrame(item->getRules()->getBigSprite());
					drawFrame->blit(_items);
					//drawFrame->blitNShade(_items, slot->getX() + (RuleInventory::HAND_W - item->getRules()->getInventoryWidth()) * RuleInventory::SLOT_W/2, slot->getY() + (RuleInventory::HAND_H - item->getRules()->getInventoryHeight()) * RuleInventory::SLOT_H/2, 0, false, Palette::blockOffset(0));
				}
			}*/

			BattleItem *ammo = item;
			if (ammo->getAmmoForSlot(0))
			{
				ammo = ammo->getAmmoForSlot(0);
			}

			if (ammo && ammo->getAmmoQuantity() > 0 && (ammo->getRules()->getClipSize() > 1 || ammo->getRules()->getBattleClipSize() > 1))
			{
				switch (item->getSlot()->getType())
				{
					case INV_HAND:
						_ammoNumber->setX(item->getSlot()->getX() + (RuleInventory::HAND_W * RuleInventory::SLOT_W) - 5);
						_ammoNumber->setY(item->getSlot()->getY() + 1);
						break;
					case INV_UTILITY:
						_ammoNumber->setX(item->getSlot()->getX() + (RuleInventory::UTILITY_W * RuleInventory::SLOT_W) - 5);
						_ammoNumber->setY(item->getSlot()->getY() + 1);
						break;
					case INV_EQUIP:
						_ammoNumber->setX(item->getSlot()->getX() + (item->getSlot()->getWidth() * RuleInventory::SLOT_W) - 5);
						_ammoNumber->setY(item->getSlot()->getY() + 1);
						break;
					default:
						_ammoNumber->setX((item->getSlot()->getX() + ((item->getSlotX() + item->getRules()->getInventoryWidth())) * RuleInventory::SLOT_W) - 5);
						_ammoNumber->setY(item->getSlot()->getY() + (item->getSlotY() * RuleInventory::SLOT_H) + 1);
				}
				if (ammo->getAmmoQuantity() > 99)
				{
					_ammoNumber->setX(_ammoNumber->getX() - 8);
				}
				else if (ammo->getAmmoQuantity() > 9)
				{
					_ammoNumber->setX(_ammoNumber->getX() - 4);
				}

				_ammoNumber->setValue(ammo->getAmmoQuantity());
				_ammoNumber->draw();
				_ammoNumber->setColor(textAmmo->color);
				_ammoNumber->blit(_items);
			}
			else if (item->haveAnyAmmo() && !item->getAmmoForSlot(0) && !item->getRules()->getPrimaryCompatibleAmmo()->empty())
			{
				switch (item->getSlot()->getType())
				{
					case INV_HAND:
						_ammoNumber->setX(item->getSlot()->getX() + (RuleInventory::HAND_W * RuleInventory::SLOT_W) - 5);
						_ammoNumber->setY(item->getSlot()->getY() + 1);
						break;
					case INV_UTILITY:
						_ammoNumber->setX(item->getSlot()->getX() + (RuleInventory::UTILITY_W * RuleInventory::SLOT_W) - 5);
						_ammoNumber->setY(item->getSlot()->getY() + 1);
						break;
					case INV_EQUIP:
						_ammoNumber->setX(item->getSlot()->getX() + (item->getSlot()->getWidth() * RuleInventory::SLOT_W) - 5);
						_ammoNumber->setY(item->getSlot()->getY() + 1);
						break;
					default:
						_ammoNumber->setX((item->getSlot()->getX() + ((item->getSlotX() + item->getRules()->getInventoryWidth())) * RuleInventory::SLOT_W) - 5);
						_ammoNumber->setY(item->getSlot()->getY() + (item->getSlotY() * RuleInventory::SLOT_H) + 1);
				}
				_ammoNumber->setValue(0);
				_ammoNumber->draw();
				_ammoNumber->setColor(textAmmo->color2);
				_ammoNumber->blit(_items);
			}
		}
		Surface stackLayer(getWidth(), getHeight(), 0, 0);
		stackLayer.setPalette(getPalette());
		// Ground items
		for (std::vector<BattleItem*>::iterator i = _selUnit->getTile()->getInventory()->begin(); i != _selUnit->getTile()->getInventory()->end(); ++i)
		{
			BattleItem *item = *i;
			Surface *frame = texture->getFrame(item->getRules()->getBigSprite());
			// note that you can make items invisible by setting their width or height to 0 (for example used with tank corpse items)
			if (item == _selItem || (item->getSlotX() + item->getRules()->getInventoryWidth()) <= _groundOffset || item->getRules()->getInventoryHeight() == 0 || item->getRules()->getInventoryWidth() == 0 || !frame)
				continue;
			int x, y;
			x = ((*i)->getSlot()->getX() + ((*i)->getSlotX() - _groundOffset) * RuleInventory::SLOT_W);
			y = ((*i)->getSlot()->getY() + (*i)->getSlotY() * RuleInventory::SLOT_H);
			BattleItem::ScriptFill(&work, *i, BODYPART_ITEM_INVENTORY, _animFrame, 0);
			work.executeBlit(frame, _items, x, y, 0);

			// grenade primer indicators
			if (item->getGrenadeLive())
			{
				_grenadeIndicators.push_back(std::make_pair(x, y));
			}

			// fatal wounds
			int fatalWounds = 0;
			if ((*i)->getUnit())
			{
				// don't show on dead units
				if ((*i)->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
				{
					fatalWounds = (*i)->getUnit()->getFatalWounds();
					if (_burnIndicator && (*i)->getUnit()->getFire() > 0)
					{
						_burningIndicators.push_back(std::make_pair(x, y));
					}
					else if (_woundIndicator && fatalWounds > 0)
					{
						_woundedIndicators.push_back(std::make_pair(x, y));
					}
					else if (_stunIndicator)
					{
						_stunnedIndicators.push_back(std::make_pair(x, y));
					}
				}
			}
			if (fatalWounds > 0)
			{
				_stackNumber->setX(((*i)->getSlot()->getX() + (((*i)->getSlotX() + (*i)->getRules()->getInventoryWidth()) - _groundOffset) * RuleInventory::SLOT_W) - 5);
				if (fatalWounds > 9)
				{
					_stackNumber->setX(_stackNumber->getX() - 4);
				}
				_stackNumber->setY(((*i)->getSlot()->getY() + ((*i)->getSlotY() + (*i)->getRules()->getInventoryHeight()) * RuleInventory::SLOT_H) - 7);
				_stackNumber->setValue(fatalWounds);
				_stackNumber->draw();
				_stackNumber->setColor(color2);
				_stackNumber->blit(&stackLayer);
			}

			BattleItem *ammo = item;
			if (ammo->getAmmoForSlot(0))
			{
				ammo = ammo->getAmmoForSlot(0);
			}

			if (ammo && ammo->getAmmoQuantity() > 0 && (ammo->getRules()->getClipSize() > 1 || ammo->getRules()->getBattleClipSize() > 1))
			{
				_ammoNumber->setX((item->getSlot()->getX() + ((item->getSlotX() + item->getRules()->getInventoryWidth()) - _groundOffset) * RuleInventory::SLOT_W) - 5);
				if (ammo->getAmmoQuantity() > 99)
				{
					_ammoNumber->setX(_ammoNumber->getX() - 8);
				}
				else if (ammo->getAmmoQuantity() > 9)
				{
					_ammoNumber->setX(_ammoNumber->getX() - 4);
				}

				_ammoNumber->setY(item->getSlot()->getY() + (item->getSlotY() * RuleInventory::SLOT_H) + 1);
				_ammoNumber->setValue(ammo->getAmmoQuantity());
				_ammoNumber->draw();
				_ammoNumber->setColor(textAmmo->color);
				_ammoNumber->blit(&stackLayer);
			}
			else if (item->haveAnyAmmo() && !item->getAmmoForSlot(0) && !item->getRules()->getPrimaryCompatibleAmmo()->empty())
			{
				_ammoNumber->setX((item->getSlot()->getX() + ((item->getSlotX() + item->getRules()->getInventoryWidth()) - _groundOffset) * RuleInventory::SLOT_W) - 5);
				if (ammo->getAmmoQuantity() > 99)
				{
					_ammoNumber->setX(_ammoNumber->getX() - 8);
				}
				else if (ammo->getAmmoQuantity() > 9)
				{
					_ammoNumber->setX(_ammoNumber->getX() - 4);
				}

				_ammoNumber->setY(item->getSlot()->getY() + (item->getSlotY() * RuleInventory::SLOT_H) + 1);
				_ammoNumber->setValue(0);
				_ammoNumber->draw();
				_ammoNumber->setColor(textAmmo->color2);
				_ammoNumber->blit(&stackLayer);
			}

			// item stacking
			if (_stackLevel[item->getSlotX()][item->getSlotY()] > 1)
			{
				_stackNumber->setX((item->getSlot()->getX() + ((item->getSlotX() + item->getRules()->getInventoryWidth()) - _groundOffset) * RuleInventory::SLOT_W) - 5);
				if (_stackLevel[item->getSlotX()][item->getSlotY()] > 9)
				{
					_stackNumber->setX(_stackNumber->getX() - 4);
				}
				_stackNumber->setY((item->getSlot()->getY() + (item->getSlotY() + item->getRules()->getInventoryHeight()) * RuleInventory::SLOT_H) - 7);
				_stackNumber->setValue(_stackLevel[item->getSlotX()][item->getSlotY()]);
				_stackNumber->draw();
				_stackNumber->setColor(color);
				_stackNumber->blit(&stackLayer);
			}
		}

		stackLayer.blit(_items);
	}
}

/**
 * Draws the selected item.
 */
void Inventory::drawSelectedItem()
{
	if (_selItem)
	{
		_selection->clear();
		_selItem->getRules()->drawHandSprite(_game->getMod()->getSurfaceSet("BIGOBS.PCK"), _selection, _selItem, RuleInventory::MAX_W, RuleInventory::MAX_H, _animFrame);
	}
}

/**
 * Moves an item to a specified slot in the
 * selected player's inventory.
 * @param item Pointer to battle item.
 * @param slot Inventory slot, or NULL if none.
 * @param x X position in slot.
 * @param y Y position in slot.
 */
void Inventory::moveItem(BattleItem *item, RuleInventory *slot, int x, int y)
{
	// Make items vanish (eg. ammo in weapons)
	if (slot == 0)
	{
		if (item->getSlot()->getType() == INV_GROUND)
		{
			_selUnit->getTile()->removeItem(item);
		}
		else
		{
			item->moveToOwner(0);
		}
	}
	else
	{
		// Handle dropping from/to ground.
		if (slot != item->getSlot())
		{
			if (slot->getType() == INV_GROUND)
			{
				item->moveToOwner(0);
				_selUnit->getTile()->addItem(item, slot);
				if (item->getUnit() && item->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
				{
					item->getUnit()->setPosition(_selUnit->getPosition());
				}
			}
			else if (item->getSlot() == 0 || item->getSlot()->getType() == INV_GROUND)
			{
				item->moveToOwner(_selUnit);
				_selUnit->getTile()->removeItem(item);
				item->setTurnFlag(false);
				if (item->getUnit() && item->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
				{
					item->getUnit()->setPosition(Position(-1, -1, -1));
				}
			}
		}
		item->setSlot(slot);
		item->setSlotX(x);
		item->setSlotY(y);
	}
}

/**
 * Checks if an item in a certain slot position would
 * overlap with any other inventory item.
 * @param unit Pointer to current unit.
 * @param item Pointer to battle item.
 * @param slot Inventory slot, or NULL if none.
 * @param x X position in slot.
 * @param y Y position in slot.
 * @return If there's overlap.
 */
bool Inventory::overlapItems(BattleUnit *unit, BattleItem *item, RuleInventory *slot, int x, int y)
{
	if (slot->getType() != INV_GROUND)
	{
		for (std::vector<BattleItem*>::const_iterator i = unit->getInventory()->begin(); i != unit->getInventory()->end(); ++i)
		{
			if ((*i)->getSlot() == slot && (*i)->occupiesSlot(x, y, item))
			{
				return true;
			}
		}
	}
	else if (unit->getTile() != 0)
	{
		for (std::vector<BattleItem*>::const_iterator i = unit->getTile()->getInventory()->begin(); i != unit->getTile()->getInventory()->end(); ++i)
		{
			if ((*i)->occupiesSlot(x, y, item))
			{
				return true;
			}
		}
	}
	return false;
}

/**
 * Gets the inventory slot located in the specified mouse position.
 * @param x Mouse X position. Returns the slot's X position.
 * @param y Mouse Y position. Returns the slot's Y position.
 * @return Slot rules, or NULL if none.
 */
RuleInventory *Inventory::getSlotInPosition(int *x, int *y) const
{
	for (std::vector<RuleInventory*>::const_iterator ii = _inventoryLayout->getSlots()->begin(); ii != _inventoryLayout->getSlots()->end(); ++ii)
		//for (std::map<std::string, RuleInventory*>::iterator i = _game->getMod()->getInventories()->begin(); i != _game->getMod()->getInventories()->end(); ++i)
	{
		if ((*ii)->checkSlotInPosition(x, y))
		{
			return *ii;
		}
	}
	return 0;
}

/**
 * Returns the item currently grabbed by the player.
 * @return Pointer to selected item, or NULL if none.
 */
BattleItem *Inventory::getSelectedItem() const
{
	return _selItem;
}

/**
 * Changes the item currently grabbed by the player.
 * @param item Pointer to selected item, or NULL if none.
 */
void Inventory::setSelectedItem(BattleItem *item)
{
	_selItem = (item && !item->getRules()->isFixed()) ? item : 0;
	if (_selItem)
	{
		if (_selItem->getSlot()->getType() == INV_GROUND)
		{
			_stackLevel[_selItem->getSlotX()][_selItem->getSlotY()] -= 1;
		}
	}
	else
	{
		_selection->clear();
	}
	drawSelectedItem();

	// 1. first draw the grid
	if (_tu)
	{
		drawGrid();
	}

	// 2. then the items
	drawItems();

	// 3. lastly re-draw the grid labels (so that they are not obscured by the items)
	/*if (_tu)
	{
		drawGridLabels(true);
	}*/
}

/**
* Changes the item currently grabbed by the player.
* @param item Pointer to selected item, or NULL if none.
*/
void Inventory::setSearchString(const std::wstring &searchString)
{
	_searchString = searchString;
	for (auto & c : _searchString) c = towupper(c);
	arrangeGround(true);
}

/**
 * Returns the item currently under mouse cursor.
 * @return Pointer to selected item, or 0 if none.
 */
BattleItem *Inventory::getMouseOverItem() const
{
	return _mouseOverItem;
}

/**
 * Changes the item currently under mouse cursor.
 * @param item Pointer to selected item, or NULL if none.
 */
void Inventory::setMouseOverItem(BattleItem *item)
{
	_mouseOverItem = (item && !item->getRules()->isFixed()) ? item : 0;
}

/**
 * Handles timers.
 */
void Inventory::think()
{
	_warning->think();
	_animTimer->think(0, this);
}

/**
 * Blits the inventory elements.
 * @param surface Pointer to surface to blit onto.
 */
void Inventory::blit(Surface *surface)
{
	clear();
	_grid->blit(this);
	_items->blit(this);
	_selection->blit(this);
	_warning->blit(this);
	Surface::blit(surface);
}

/**
 * Moves the selected item.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void Inventory::mouseOver(Action *action, State *state)
{
	_selection->setX((int)floor(action->getAbsoluteXMouse()) - _selection->getWidth() / 2 - getX());
	_selection->setY((int)floor(action->getAbsoluteYMouse()) - _selection->getHeight() / 2 - getY());
	if (_selUnit == 0)
		return;

	int x = (int)floor(action->getAbsoluteXMouse()) - getX(),
		y = (int)floor(action->getAbsoluteYMouse()) - getY();
	RuleInventory *slot = getSlotInPosition(&x, &y);
	if (slot != 0)
	{
		if (slot->getType() == INV_GROUND)
		{
			x += _groundOffset;
		}
		BattleItem *item = _selUnit->getItem(slot, x, y);
		setMouseOverItem(item);
	}
	else
	{
		setMouseOverItem(0);
	}

	_selection->setX((int)floor(action->getAbsoluteXMouse()) - _selection->getWidth() / 2 - getX());
	_selection->setY((int)floor(action->getAbsoluteYMouse()) - _selection->getHeight() / 2 - getY());
	InteractiveSurface::mouseOver(action, state);
}

/**
 * Picks up / drops an item.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void Inventory::mouseClick(Action *action, State *state)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if (_locked) return;
		if (_selUnit == 0)
			return;
		// Pickup item
		if (_selItem == 0)
		{
			int x = (int)floor(action->getAbsoluteXMouse()) - getX(),
				y = (int)floor(action->getAbsoluteYMouse()) - getY();
			RuleInventory *slot = getSlotInPosition(&x, &y);
			if (slot != 0)
			{
				if (slot->getType() == INV_GROUND)
				{
					x += _groundOffset;
				}
				BattleItem *item = _selUnit->getItem(slot, x, y);
				if (item != 0)
				{
					if ((SDL_GetModState() & KMOD_CTRL))
					{
						// cannot move fixed items with Ctrl+click
						if (item->getRules()->isFixed())
						{
							_warning->showMessage(_game->getLanguage()->getString("STR_FIXED_ITEMS_CANNOT_BE_MOVED"));
							return;
						}

						RuleInventory *newSlot = _inventorySlotGround;
						std::string warning = "STR_NOT_ENOUGH_SPACE";
						bool placed = false;

						if (slot->getType() == INV_GROUND)
						{
							switch (item->getRules()->getBattleType())
							{
								case BT_FIREARM:
									newSlot = _game->getMod()->getInventory("STR_RIGHT_HAND", true);
									break;
								case BT_MINDPROBE:
								case BT_PSIAMP:
								case BT_MELEE:
								case BT_CORPSE:
									newSlot = _game->getMod()->getInventory("STR_LEFT_HAND", true);
									break;
								default:
									if (item->getRules()->getInventoryHeight() > 2)
									{
										newSlot = _game->getMod()->getInventory("STR_BACK_PACK", true);
									}
									else
									{
										newSlot = _game->getMod()->getInventory("STR_BELT", true);
									}
									break;
							}
						}

						if (newSlot->getType() != INV_GROUND)
						{
							_stackLevel[item->getSlotX()][item->getSlotY()] -= 1;

							placed = (std::find(_inventoryLayout->getSlots()->begin(), _inventoryLayout->getSlots()->end(), newSlot) != _inventoryLayout->getSlots()->end()) && _selUnit->moveItem(item, newSlot, -1, -1, warning, _tu);

							if (!placed)
							{
								for (std::vector<RuleInventory*>::const_iterator wildCard = _inventoryLayout->getSlots()->begin(); wildCard != _inventoryLayout->getSlots()->end() && !placed; ++wildCard)
								{
									newSlot = *wildCard;
									if (newSlot->getType() == INV_GROUND)
									{
										continue;
									}
									placed = _selUnit->moveItem(item, newSlot, -1, -1, warning, _tu);
								}
							}
							if (!placed)
							{
								_stackLevel[item->getSlotX()][item->getSlotY()] += 1;
							}
							else
							{
								_game->getMod()->getSoundByDepth(_depth, Mod::ITEM_DROP)->play();
								drawItems();
							}
						}
						else
						{
							if (placed = _selUnit->moveItem(item, newSlot, 0, 0, warning, _tu))
							{
								_game->getMod()->getSoundByDepth(_depth, Mod::ITEM_DROP)->play();
							}
							arrangeGround(false);
						}

						if (!placed)
						{
							_warning->showMessage(_game->getLanguage()->getString(warning));
						}
					}
					else
					{
						setSelectedItem(item);
						if (item->getGrenadeLive())
						{
							_warning->showMessage(_game->getLanguage()->getString("STR_GRENADE_IS_ACTIVATED"));
						}
					}
				}
			}
		}
		// Drop item
		else
		{
			int x = _selection->getX() + (RuleInventory::MAX_W - _selItem->getRules()->getInventoryWidth()) * RuleInventory::SLOT_W / 2 + RuleInventory::SLOT_W / 2,
				y = _selection->getY() + (RuleInventory::MAX_H - _selItem->getRules()->getInventoryHeight()) * RuleInventory::SLOT_H / 2 + RuleInventory::SLOT_H / 2;
			RuleInventory *slot = getSlotInPosition(&x, &y);
			if (slot != 0)
			{
				if (slot->getType() == INV_GROUND)
				{
					x += _groundOffset;
				}
				BattleItem *item = _selUnit->getItem(slot, x, y);

				bool canStack = slot->getType() == INV_GROUND && canBeStacked(item, _selItem);
				bool validTargetSlot = slot->getType() == INV_GROUND || _selItem->getRules()->canBePlacedIntoInventorySection(slot);

				// Put item in empty slot, or stack it, if possible.
				std::string warning;

				if (validTargetSlot && (item == 0 || item == _selItem || canStack))
				{
					if (slot->getType() == INV_GROUND)
					{
						if (canStack)
						{
							x = item->getSlotX();
							y = item->getSlotY();
						}
						if ((canStack || (!_selUnit->overlapItems(item, slot, x, y) && slot->fitItemInSlot(_selItem->getRules(), x, y))) && _selUnit->moveItem(_selItem, slot, x, y, warning, _tu))
						{
							if (canStack)
							{
								_stackLevel[item->getSlotX()][item->getSlotY()] += 1;
							}
							else
							{
								_stackLevel[x][y] += 1;
							}
							setSelectedItem(0);
							_game->getMod()->getSoundByDepth(_depth, Mod::ITEM_DROP)->play();
							drawItems();
						}
						else if (warning.size())
						{
							_warning->showMessage(_game->getLanguage()->getString(warning));
						}
					}
					else if (_selUnit->moveItem(_selItem, slot, x, y, warning, _tu))
					{
						setSelectedItem(0);
						_game->getMod()->getSoundByDepth(_depth, Mod::ITEM_DROP)->play();
						drawItems();
					}
					else
					{
						_warning->showMessage(_game->getLanguage()->getString(warning));
					}
				}
				// Put item in weapon
				else if (item)
				{
					if (_selUnit->loadWeapon(item, _selItem, warning))
					{
						setSelectedItem(0);
						_game->getMod()->getSoundByDepth(_depth, Mod::ITEM_RELOAD)->play();
					}
					else
					{
						_warning->showMessage(_game->getLanguage()->getString(warning));
					}
				}
				else if (!validTargetSlot)
				{
					_warning->showMessage(_game->getLanguage()->getString("STR_CANNOT_PLACE_ITEM_INTO_THIS_SECTION"));
				}
				// else swap the item positions?
			}
			else
			{
				// try again, using the position of the mouse cursor, not the item (slightly more intuitive for stacking)
				x = (int)floor(action->getAbsoluteXMouse()) - getX();
				y = (int)floor(action->getAbsoluteYMouse()) - getY();
				slot = getSlotInPosition(&x, &y);
				if (slot != 0 && slot->getType() == INV_GROUND)
				{
					x += _groundOffset;
					BattleItem *item = _selUnit->getItem(slot, x, y);
					if (canBeStacked(item, _selItem))
					{
						std::string warning;
						if (_selUnit->moveItem(_selItem, slot, item->getSlotX(), item->getSlotY(), warning, _tu))
						{
							_stackLevel[item->getSlotX()][item->getSlotY()] += 1;
							setSelectedItem(0);
							_game->getMod()->getSoundByDepth(_depth, Mod::ITEM_DROP)->play();
						}
						else
						{
							_warning->showMessage(_game->getLanguage()->getString(warning));
						}
					}
				}
			}
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		if (_locked) return;
		if (_selItem == 0)
		{
			if (!_base || Options::includePrimeStateInSavedLayout)
			{
				if (!_tu)
				{
					int x = (int)floor(action->getAbsoluteXMouse()) - getX(),
						y = (int)floor(action->getAbsoluteYMouse()) - getY();
					RuleInventory *slot = getSlotInPosition(&x, &y);
					if (slot != 0)
					{
						if (slot->getType() == INV_GROUND)
						{
							x += _groundOffset;
						}
						BattleItem *item = _selUnit->getItem(slot, x, y);
						if (item != 0)
						{
							const BattleFuseType fuseType = item->getRules()->getFuseTimerType();
							if (fuseType != BFT_NONE)
							{
								if (!item->getGrenadeLive())
								{
									// Prime that grenade!
									if (fuseType == BFT_SET)
									{
										_game->pushState(new PrimeGrenadeState(0, true, item));
									}
									else
									{
										_warning->showMessage(_game->getLanguage()->getString(item->getRules()->getPrimeActionMessage()));
										item->setFuseTimer(item->getRules()->getFuseTimerDefault());
										arrangeGround(false);
									}
								}
								else
								{
									_warning->showMessage(_game->getLanguage()->getString(item->getRules()->getUnprimeActionMessage()));
									item->setFuseTimer(-1);  // Unprime the grenade
									drawItems();
								}
							}
						}
					}
				}
				else
				{
					_game->popState(); // Closes the inventory window on right-click (if not in preBattle equip screen!)
				}
			}
		}
		else
		{
			if (_selItem->getSlot()->getType() == INV_GROUND)
			{
				_stackLevel[_selItem->getSlotX()][_selItem->getSlotY()] += 1;
			}
			// Return item to original position
			setSelectedItem(0);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_MIDDLE)
	{
		if (_selUnit == 0)
			return;

		int x = (int)floor(action->getAbsoluteXMouse()) - getX(),
			y = (int)floor(action->getAbsoluteYMouse()) - getY();
		RuleInventory *slot = getSlotInPosition(&x, &y);
		if (slot != 0)
		{
			if (slot->getType() == INV_GROUND)
			{
				x += _groundOffset;
			}
			BattleItem *item = _selUnit->getItem(slot, x, y);
			if (item != 0)
			{
				std::string articleId = item->getRules()->getType();
				Ufopaedia::openArticle(_game, articleId);
			}
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		changeGroundOffset(-1);
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		changeGroundOffset(1);
	}
	InteractiveSurface::mouseClick(action, state);
}

/**
 * Unloads the selected weapon, placing the gun
 * on the right hand and the ammo on the left hand.
 * @return The success of the weapon being unloaded.
 */
bool Inventory::unload()
{
	// Must be holding an item
	if (_selItem == 0)
	{
		return false;
	}

	const auto type = _selItem->getRules()->getBattleType();
	const bool grenade = type == BT_GRENADE || type == BT_PROXIMITYGRENADE;
	const bool weapon = type == BT_FIREARM || type == BT_MELEE;
	int slotForAmmoUnload = -1;
	int toForAmmoUnload = 0;

	// Item should be able to unload or unprimed.
	if (grenade)
	{
		// Item must be primed
		if (_selItem->getFuseTimer() == -1)
		{
			return false;
		}
	}
	else if (weapon)
	{
		// Item must be loaded
		bool showError = false;
		auto checkSlot = [&](int slot)
		{
			if (!_selItem->needsAmmoForSlot(slot))
			{
				return false;
			}

			// TODO: Use unit to determine unload cost.  Compare with OXC+ BattleUnit::unload.
			auto tu = _selItem->getRules()->getTUUnload(slot);
			if (tu == 0 && !_tu)
			{
				return false;
			}

			auto ammo = _selItem->getAmmoForSlot(slot);
			if (ammo)
			{
				toForAmmoUnload = tu;
				slotForAmmoUnload = slot;
				return true;
			}
			else
			{
				showError = true;
				return false;
			}
		};
		if (!(SDL_GetModState() & KMOD_SHIFT))
		{
			for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
			{
				if (checkSlot(slot))
				{
					break;
				}
			}
		}
		else
		{
			for (int slot = RuleItem::AmmoSlotMax - 1; slot >= 0; --slot)
			{
				if (checkSlot(slot))
				{
					break;
				}
			}
		}
		if (slotForAmmoUnload == -1)
		{
			if (showError)
			{
				_warning->showMessage(_game->getLanguage()->getString("STR_NO_AMMUNITION_LOADED"));
			}
			return false;
		}
	}
	else
	{
		// not weapon or grenade, can't use unload button
		return false;
	}

	// Hands must be free
	for (std::vector<BattleItem*>::iterator i = _selUnit->getInventory()->begin(); i != _selUnit->getInventory()->end(); ++i)
	{
		if ((*i)->getSlot()->getType() == INV_HAND && (*i) != _selItem)
		{
			_warning->showMessage(_game->getLanguage()->getString("STR_BOTH_HANDS_MUST_BE_EMPTY"));
			return false;
		}
	}

	BattleActionCost cost(BA_NONE, _selUnit, _selItem, nullptr);
	if (grenade)
	{
		cost.type = BA_UNPRIME;
		cost.updateTU();
	}
	else
	{
		cost.Time += toForAmmoUnload;
	}

	if (cost.haveTU() && _selItem->getSlot()->getType() != INV_HAND)
	{
		cost.Time += _selItem->getSlot()->getCost(_inventorySlotRightHand);
	}

	std::string err;
	if (!_tu || cost.spendTU(&err))
	{
		moveItem(_selItem, _inventorySlotRightHand, 0, 0);
		if (grenade)
		{
			_selItem->setFuseTimer(-1);
			_warning->showMessage(_game->getLanguage()->getString(_selItem->getRules()->getUnprimeActionMessage()));
		}
		else
		{
			auto oldAmmo = _selItem->setAmmoForSlot(slotForAmmoUnload, nullptr);
			moveItem(oldAmmo, _inventorySlotLeftHand, 0, 0);
		}
		setSelectedItem(0);
		return true;
	}
	else
	{
		if (!err.empty())
		{
			_warning->showMessage(_game->getLanguage()->getString(err));
		}
		return false;
	}
}

/**
* Checks whether the given item is visible with the current search string.
* @param item The item to check.
* @return True if item should be shown. False otherwise.
*/
bool Inventory::isInSearchString(BattleItem *item)
{
	if (!_searchString.length())
	{
		// No active search string.
		return true;
	}

	std::wstring itemLocalName;
	if (!_game->getSavedGame()->isResearched(item->getRules()->getRequirements()))
	{
		// Alien artifact, shouldn't match on the real name.
		itemLocalName = _game->getLanguage()->getString("STR_ALIEN_ARTIFACT");
	}
	else
	{
		itemLocalName = _game->getLanguage()->getString(item->getRules()->getName());
	}
	std::transform(itemLocalName.begin(), itemLocalName.end(), itemLocalName.begin(), towupper);
	if (itemLocalName.find(_searchString) != std::wstring::npos)
	{
		// Name match.
		return true;
	}

	// If present in the Ufopaedia, check categories for a match as well.
	ArticleDefinition *articleID = _game->getMod()->getUfopaediaArticle(item->getRules()->getType());
	if (articleID && Ufopaedia::isArticleAvailable(_game->getSavedGame(), articleID))
	{
		std::vector<std::string> itemCategories = item->getRules()->getCategories();
		for (std::vector<std::string>::iterator i = itemCategories.begin(); i != itemCategories.end(); ++i)
		{
			std::wstring catLocalName = _game->getLanguage()->getString((*i));
			std::transform(catLocalName.begin(), catLocalName.end(), catLocalName.begin(), towupper);
			if (catLocalName.find(_searchString) != std::wstring::npos)
			{
				// Category match
				return true;
			}
		}

		for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
		{
			if (item->getAmmoForSlot(slot))
			{
				std::vector<std::string> itemAmmoCategories = item->getAmmoForSlot(slot)->getRules()->getCategories();
				for (std::vector<std::string>::iterator i = itemAmmoCategories.begin(); i != itemAmmoCategories.end(); ++i)
				{
					std::wstring catLocalName = _game->getLanguage()->getString((*i));
					std::transform(catLocalName.begin(), catLocalName.end(), catLocalName.begin(), towupper);
					if (catLocalName.find(_searchString) != std::wstring::npos)
					{
						// Category match
						return true;
					}
				}
			}
		}
	}
	return false;
}

/**
* Arranges items on the ground for the inventory display.
* Since items on the ground aren't assigned to anyone,
* they don't actually have permanent slot positions.
* @param alterOffset Whether to alter the ground offset.
*/
void Inventory::arrangeGround(bool alterOffset)
{
	RuleInventory *ground = _inventorySlotGround;

	int slotsX = (Screen::ORIGINAL_WIDTH - ground->getX()) / RuleInventory::SLOT_W;
	int slotsY = (Screen::ORIGINAL_HEIGHT - ground->getY()) / RuleInventory::SLOT_H;
	int x = 0;
	int y = 0;
	bool donePlacing = false;
	bool canPlace = false;
	int xMax = 0;
	_stackLevel.clear();

	if (_selUnit != 0)
	{
		std::unordered_map< std::string, std::vector< std::vector<BattleItem*> > > typeItemLists; // Lists of items of the same type
		std::vector<BattleItem*> itemListOrder; // Placement order of item type stacks.
		std::vector< std::vector<int> > startIndexCacheX; // Cache for skipping to last known available position of a given size.
														  // Create chart of free slots for later rapid lookup.
		std::vector< std::vector<bool> > occupiedSlots;
		occupiedSlots.resize(slotsY, std::vector<bool>(slotsX * 2, false));

		// Move items out of the way and find which stack they'll end up in within the inventory.
		for (auto& i : *(_selUnit->getTile()->getInventory()))
		{
			// first move all items out of the way - a big number in X direction
			i->setSlot(ground);
			i->setSlotX(1000000);
			i->setSlotY(0);

			// Only handle visible items from this point on.
			if (i->getRules()->getInventoryWidth() && i->getRules()->isVehicleItem() == _selUnit->isVehicle())
			{
				// Each item type has a list of stacks. Find / create a suitable one for this item.
				auto iterItemList = typeItemLists.find(i->getRules()->getType());
				if (iterItemList == typeItemLists.end())
				{
					// New item type, create a list of item stacks for it.
					iterItemList = typeItemLists.insert(std::pair<std::string, std::vector< std::vector<BattleItem*> > >(i->getRules()->getType(), { { i } })).first;
					itemListOrder.push_back(i);
					// Figure out the largest item size present:
					x = std::max(x, i->getRules()->getInventoryWidth());
					y = std::max(y, i->getRules()->getInventoryHeight());
				}
				else
				{
					// With the item type found. Find which stack of this item type to add the item to.
					bool stacked = false;
					for (auto& itemStack : iterItemList->second)
					{
						if (canBeStacked(i, itemStack.at(0)))
						{
							itemStack.push_back(i);
							stacked = true;
							break;
						}
					}
					if (!stacked) {
						// Not able to be stacked with previously placed items of this type. Make a new stack containing only this item.
						iterItemList->second.push_back({ i });
					}
				}
			}
		}
		// Create the cache of last placed index for a given item size.
		startIndexCacheX.resize(y + 1, std::vector<int>(x + 1, 0));

		// Before we place the items, we should sort the itemListOrder vector using the 'list order' of the items.
		std::sort(itemListOrder.begin(), itemListOrder.end(), [](BattleItem* const &a, BattleItem* const &b)
		{
			return a->getRules()->getListOrder() < b->getRules()->getListOrder();
		}
		);

		// Now for each item type, find the most topleft position that is not occupied and will fit.
		for (auto& i : itemListOrder)
		{
			// Fetch the list of item stacks for this item type. Then place each stack.
			auto iterItemList = typeItemLists.find(i->getRules()->getType());
			for (auto& itemStack : iterItemList->second)
			{
				BattleItem* itemTypeSample = itemStack.at(0); // Grab a sample of the stack of item type we're trying to place.
				if (!isInSearchString(itemTypeSample))
				{
					// quick search
					// Not a match with the active search string, skip this item stack. (Will remain outside the visible inventory)
					break;
				}

				// Start searching at the x value where we last placed an item of this size.
				// But also don't let more than half a screen behind the furtherst item (because we want to keep similar items together).
				x = std::max(xMax - slotsX / 2, startIndexCacheX[itemTypeSample->getRules()->getInventoryHeight()][itemTypeSample->getRules()->getInventoryWidth()]);
				y = 0;
				donePlacing = false;
				while (!donePlacing)
				{
					canPlace = true; // Assume we can put the item type here, if one of the following checks fails, we can't.
					for (int xd = 0; xd < itemTypeSample->getRules()->getInventoryWidth() && canPlace; xd++)
					{
						if ((x + xd) % slotsX < x % slotsX)
						{
							canPlace = false;
						}
						else
						{
							for (int yd = 0; yd < itemTypeSample->getRules()->getInventoryHeight() && canPlace; yd++)
							{
								// If there's room and no item, we can place it here. (No need to check for stackability as nothing stackable has been placed)
								canPlace = !occupiedSlots[y + yd][x + xd];
							}
						}
					}
					if (canPlace) // Found a place for this item stack.
					{
						xMax = std::max(xMax, x + itemTypeSample->getRules()->getInventoryWidth());
						if ((x + startIndexCacheX[0].size()) >= occupiedSlots[0].size())
						{
							// Filled enough for the widest item to potentially request occupancy checks outside of current cache. Expand slot cache.
							size_t newCacheSize = occupiedSlots[0].size() * 2;
							for (std::vector< std::vector<bool> >::iterator j = occupiedSlots.begin(); j != occupiedSlots.end(); ++j) {
								j->resize(newCacheSize, false);
							}
						}
						// Reserve the slots this item will occupy.
						for (int xd = 0; xd < itemTypeSample->getRules()->getInventoryWidth() && canPlace; xd++)
						{
							for (int yd = 0; yd < itemTypeSample->getRules()->getInventoryHeight() && canPlace; yd++)
							{
								occupiedSlots[y + yd][x + xd] = true;
							}
						}

						// Place all items from this stack in the location we just reserved.
						for (auto& j : itemStack)
						{
							j->setSlotX(x);
							j->setSlotY(y);
							_stackLevel[x][y] += 1;
						}
						donePlacing = true;
					}
					if (!donePlacing)
					{
						y++;
						if (y > slotsY - itemTypeSample->getRules()->getInventoryHeight())
						{
							y = 0;
							x++;
						}
					}
				}
				// Now update the shortcut cache so that items to follow are able to skip a decent chunk of their search,
				// as there can be no spot before this x-address with available slots for items that are larger in one or more dimensions.
				int firstPossibleX = itemTypeSample->getRules()->getInventoryHeight() * 2 > slotsY ? x + itemTypeSample->getRules()->getInventoryWidth() : x;
				for (size_t offsetY = itemTypeSample->getRules()->getInventoryHeight(); offsetY < startIndexCacheX.size(); ++offsetY)
				{
					for (size_t offsetX = itemTypeSample->getRules()->getInventoryWidth(); offsetX < startIndexCacheX[offsetY].size(); ++offsetX)
					{
						startIndexCacheX[offsetY][offsetX] = std::max(firstPossibleX, startIndexCacheX[offsetY][offsetX]);
					}
				}
			} // end stacks for this item type
		} // end of item types
	}
	if (alterOffset)
	{
		if (xMax >= _groundOffset + slotsX)
		{
			_groundOffset += slotsX;
		}
		else
		{
			_groundOffset = 0;
		}
	}
	drawItems();
}

void Inventory::changeGroundOffset(int offset)
{
	int xMax = 0;

	for (std::vector<BattleItem*>::iterator ii = _selUnit->getTile()->getInventory()->begin(); ii != _selUnit->getTile()->getInventory()->end(); ++ii)
	{
		BattleItem* item = *ii;
		int width = item->getRules()->getInventoryWidth();
		if (width) { xMax = std::max(xMax, item->getSlotX() + width); }
	}

	_groundOffset = std::max(0, std::min(_groundOffset + offset, xMax - 1));

	drawItems();
}

/**
 * Attempts to place the item in the inventory slot.
 * @param newSlot Where to place the item.
 * @param item Item to be placed.
 * @param warning Warning message if item could not be placed.
 * @return True, if the item was successfully placed in the inventory.
 */
bool Inventory::fitItem(RuleInventory *newSlot, BattleItem *item, std::string &warning)
{
	// Check if this inventory section supports the item
	if (!item->getRules()->canBePlacedIntoInventorySection(newSlot))
	{
		warning = "STR_CANNOT_PLACE_ITEM_INTO_THIS_SECTION";
		return false;
	}

	bool placed = false;
	int maxSlotX = 0;
	int maxSlotY = 0;
	for (std::vector<RuleSlot>::iterator j = newSlot->getSlots()->begin(); j != newSlot->getSlots()->end(); ++j)
	{
		if (j->x > maxSlotX) maxSlotX = j->x;
		if (j->y > maxSlotY) maxSlotY = j->y;
	}
	for (int y2 = 0; y2 <= maxSlotY && !placed; ++y2)
	{
		for (int x2 = 0; x2 <= maxSlotX && !placed; ++x2)
		{
			if (!overlapItems(_selUnit, item, newSlot, x2, y2) && newSlot->fitItemInSlot(item->getRules(), x2, y2))
			{
				if (!_tu || _selUnit->spendTimeUnits(item->getSlot()->getCost(newSlot)))
				{
					placed = true;
					moveItem(item, newSlot, x2, y2);
					_game->getMod()->getSoundByDepth(_depth, Mod::ITEM_DROP)->play();
					drawItems();
				}
				else
				{
					warning = "STR_NOT_ENOUGH_TIME_UNITS";
				}
			}
		}
	}
	return placed;
}

/**
 * Checks if two items can be stacked on one another.
 * @param itemA First item.
 * @param itemB Second item.
 * @return True, if the items can be stacked on one another.
 */
bool Inventory::canBeStacked(BattleItem *itemA, BattleItem *itemB)
{
	//both items actually exist
	if (!itemA || !itemB) return false;

	//both items have the same ruleset
	if (itemA->getRules() != itemB->getRules()) return false;

	for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
	{
		auto ammoA = itemA->getAmmoForSlot(slot);
		auto ammoB = itemB->getAmmoForSlot(slot);
		// or they both have ammo
		if (ammoA && ammoB)
		{
			// and the same ammo type
			if (ammoA->getRules() != ammoB->getRules()) return false;
			// and the same ammo quantity
			if (ammoA->getAmmoQuantity() != ammoB->getAmmoQuantity()) return false;
		}
		else if (ammoA || ammoB)
		{
			return false;
		}
	}

	return (
		// and neither is set to explode
		!itemA->getGrenadeLive() && !itemB->getGrenadeLive() &&
		// and neither is a corpse or unconscious unit
		itemA->getUnit() == 0 && itemB->getUnit() == 0 &&
		// and if it's a medkit, it has the same number of charges
		itemA->getPainKillerQuantity() == itemB->getPainKillerQuantity() &&
		itemA->getHealQuantity() == itemB->getHealQuantity() &&
		itemA->getStimulantQuantity() == itemB->getStimulantQuantity());
}

/**
 * Shows a warning message.
 * @param msg The message to show.
 */
void Inventory::showWarning(const std::wstring &msg)
{
	_warning->showMessage(msg);
}

/**
 * Shows primer warnings on all live grenades.
 * Also shows stunned-indicator on unconscious units.
 */
void Inventory::drawPrimers()
{
	const int Pulsate[8] = { 0, 1, 2, 3, 4, 3, 2, 1 };

	// grenades
	Surface *tempSurface = _game->getMod()->getSurfaceSet("SCANG.DAT")->getFrame(6);
	for (std::vector<std::pair<int, int> >::const_iterator i = _grenadeIndicators.begin(); i != _grenadeIndicators.end(); ++i)
	{
		tempSurface->blitNShade(_items, (*i).first, (*i).second, Pulsate[_animFrame % 8]);
	}

	// burning units
	for (std::vector<std::pair<int, int> >::const_iterator i = _burningIndicators.begin(); i != _burningIndicators.end(); ++i)
	{
		_burnIndicator->blitNShade(_items, (*i).first, (*i).second, Pulsate[_animFrame % 8]);
	}

	// wounded units
	for (std::vector<std::pair<int, int> >::const_iterator i = _woundedIndicators.begin(); i != _woundedIndicators.end(); ++i)
	{
		_woundIndicator->blitNShade(_items, (*i).first, (*i).second, Pulsate[_animFrame % 8]);
	}

	// stunned units
	for (std::vector<std::pair<int, int> >::const_iterator i = _stunnedIndicators.begin(); i != _stunnedIndicators.end(); ++i)
	{
		_stunIndicator->blitNShade(_items, (*i).first, (*i).second, Pulsate[_animFrame % 8]);
	}
}

/**
 * Animate surface.
 */
void Inventory::animate()
{
	if (_tu)
	{
		SavedBattleGame* save = _game->getSavedGame()->getSavedBattle();
		save->nextAnimFrame();
		_animFrame = save->getAnimFrame();
	}
	else
	{
		_animFrame++;
	}

	drawItems();
	drawPrimers();
	drawSelectedItem();
}

void Inventory::setLocked(bool locked)
{
	_locked = locked;
}

bool Inventory::getLocked() const
{
	return _locked;
}

}
