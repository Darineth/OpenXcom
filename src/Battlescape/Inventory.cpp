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
#include "WarningMessage.h"
#include "../Savegame/Tile.h"
#include "PrimeGrenadeState.h"
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

	_animTimer = new Timer(125);
	_animTimer->onTimer((SurfaceHandler)&Inventory::drawPrimers);
	_animTimer->start();

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
 * Changes the unit to display the inventory of.
 * @param unit Pointer to battle unit.
 */
void Inventory::setSelectedUnit(BattleUnit *unit)
{
	RuleInventoryLayout *newLayout = _game->getMod()->getInventoryLayout(unit ? unit->getInventoryLayout() : "STR_STANDARD_INV");
	bool arrange = !_selUnit || (_selUnit->isVehicle() != unit->isVehicle()) || (newLayout != _inventoryLayout);
	_selUnit = unit;
	_inventoryLayout = newLayout;
	if(arrange)
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
		if(_selItem)
		{
			canMove = _selUnit->moveItem(_selItem, rule, -1, -1, dummy, false, true, &moveCost);
			colorGrid = canMove ? gridOk->color : gridError->color;
			colorBackground = canMove ? gridOk->color2 : gridError->color2;
			if(moveCost < 0)
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
					if (_selItem && weapon && weapon->needsAmmo() && !weapon->getAmmoItem() && _selUnit->loadWeapon(weapon, _selItem, dummy, false, true, &reloadCost))
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
		if(_selItem && _selItem->getSlot() && (rule != _selItem->getSlot()))
		{
			if(reloadable)
			{
				text.setText(_game->getLanguage()->getString("STR_RELOAD_COST").arg(reloadCost).arg(_selUnit->getTimeUnits()));
			}
			else if(!canMove && moveCost <= 0)
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

		if(text.getText().find(L'\n') != std::wstring::npos)
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
	_items->clear();
	_grenadeIndicators.clear();
	Uint8 color = _game->getMod()->getInterface("inventory")->getElement("numStack")->color;
	Element *textAmmo = _game->getMod()->getInterface("inventory")->getElement("textAmmo");
	if (_selUnit != 0)
	{
		SurfaceSet *texture = _game->getMod()->getSurfaceSet("BIGOBS.PCK");
		// Soldier items
		for (std::vector<BattleItem*>::iterator i = _selUnit->getInventory()->begin(); i != _selUnit->getInventory()->end(); ++i)
		{
			BattleItem *item = *i;
			if (item == _selItem)
				continue;

			//bool twoHanded = false;

			Surface *frame = texture->getFrame(item->getRules()->getBigSprite());
			switch (item->getSlot()->getType())
			{
			case INV_SLOT:
				frame->setX(item->getSlot()->getX() + item->getSlotX() * RuleInventory::SLOT_W);
				frame->setY(item->getSlot()->getY() + item->getSlotY() * RuleInventory::SLOT_H);
				//twoHanded = false;
				break;
			case INV_HAND:
				frame->setX(item->getSlot()->getX() + (RuleInventory::HAND_W - item->getRules()->getInventoryWidth()) * RuleInventory::SLOT_W / 2);
				frame->setY(item->getSlot()->getY() + (RuleInventory::HAND_H - item->getRules()->getInventoryHeight()) * RuleInventory::SLOT_H / 2);
				//twoHanded = item->getRules()->isTwoHanded();
				break;
			case INV_UTILITY:
				frame->setX(item->getSlot()->getX() + (RuleInventory::UTILITY_W - item->getRules()->getInventoryWidth()) * RuleInventory::SLOT_W / 2);
				frame->setY(item->getSlot()->getY() + (RuleInventory::UTILITY_H - item->getRules()->getInventoryHeight()) * RuleInventory::SLOT_H / 2);
				break;
			case INV_EQUIP:
				frame->setX(item->getSlot()->getX() + (item->getSlot()->getWidth() - item->getRules()->getInventoryWidth()) * RuleInventory::SLOT_W / 2);
				frame->setY(item->getSlot()->getY() + (item->getSlot()->getHeight() - item->getRules()->getInventoryHeight()) * RuleInventory::SLOT_H / 2);
				break;
			}

			texture->getFrame(item->getRules()->getBigSprite())->blit(_items);

			// grenade primer indicators
			if (item->getGrenadeLive())
			{
				_grenadeIndicators.push_back(std::make_pair(frame->getX(), frame->getY()));
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
			if(ammo->getAmmoItem())
			{
				ammo = ammo->getAmmoItem();
			}

			if(ammo && ammo->getAmmoQuantity() > 0 && (ammo->getRules()->getClipSize() > 1 || ammo->getRules()->getBattleClipSize() > 1))
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
					_ammoNumber->setX(_ammoNumber->getX()-8);
				}
				else if (ammo->getAmmoQuantity() > 9)
				{
					_ammoNumber->setX(_ammoNumber->getX()-4);
				}

				_ammoNumber->setValue(ammo->getAmmoQuantity());
				_ammoNumber->draw();
				_ammoNumber->setColor(textAmmo->color);
				_ammoNumber->blit(_items);
			}
			else if(item->needsAmmo() && !item->getAmmoItem() && !item->getRules()->getCompatibleAmmo()->empty())
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
		Surface *stackLayer = new Surface(getWidth(), getHeight(), 0, 0);
		stackLayer->setPalette(getPalette());
		// Ground items
		for (std::vector<BattleItem*>::iterator i = _selUnit->getTile()->getInventory()->begin(); i != _selUnit->getTile()->getInventory()->end(); ++i)
		{
			BattleItem *item = *i;
			Surface *frame = texture->getFrame(item->getRules()->getBigSprite());
			// note that you can make items invisible by setting their width or height to 0 (for example used with tank corpse items)
			if (item == _selItem || (item->getSlotX() + item->getRules()->getInventoryWidth()) <= _groundOffset || item->getRules()->getInventoryHeight() == 0 || item->getRules()->getInventoryWidth() == 0 || !frame)
				continue;
			frame->setX(item->getSlot()->getX() + (item->getSlotX() - _groundOffset) * RuleInventory::SLOT_W);
			frame->setY(item->getSlot()->getY() + item->getSlotY() * RuleInventory::SLOT_H);
			frame->blit(_items);

			// grenade primer indicators
			if (item->getGrenadeLive())
			{
				_grenadeIndicators.push_back(std::make_pair(frame->getX(), frame->getY()));
			}

			// item stacking
			if (_stackLevel[item->getSlotX()][item->getSlotY()] > 1)
			{
				_stackNumber->setX((item->getSlot()->getX() + ((item->getSlotX() + item->getRules()->getInventoryWidth()) - _groundOffset) * RuleInventory::SLOT_W)-5);
				if (_stackLevel[item->getSlotX()][item->getSlotY()] > 9)
				{
					_stackNumber->setX(_stackNumber->getX()-4);
				}
				_stackNumber->setY((item->getSlot()->getY() + (item->getSlotY() + item->getRules()->getInventoryHeight()) * RuleInventory::SLOT_H)-6);
				_stackNumber->setValue(_stackLevel[item->getSlotX()][item->getSlotY()]);
				_stackNumber->draw();
				_stackNumber->setColor(color);
				_stackNumber->blit(stackLayer);
			}

			BattleItem *ammo = item;
			if(ammo->getAmmoItem())
			{
				ammo = ammo->getAmmoItem();
			}

			if(ammo && ammo->getAmmoQuantity() > 0 && (ammo->getRules()->getClipSize() > 1 || ammo->getRules()->getBattleClipSize() > 1))
			{
				_ammoNumber->setX((item->getSlot()->getX() + ((item->getSlotX() + item->getRules()->getInventoryWidth()) - _groundOffset) * RuleInventory::SLOT_W)-5);
				if (ammo->getAmmoQuantity() > 99)
				{
					_ammoNumber->setX(_ammoNumber->getX()-8);
				}
				else if (ammo->getAmmoQuantity() > 9)
				{
					_ammoNumber->setX(_ammoNumber->getX()-4);
				}

				_ammoNumber->setY(item->getSlot()->getY() + (item->getSlotY() * RuleInventory::SLOT_H) + 1);
				_ammoNumber->setValue(ammo->getAmmoQuantity());
				_ammoNumber->draw();
				_ammoNumber->setColor(textAmmo->color);
				_ammoNumber->blit(stackLayer);
			}
			else if(item->needsAmmo() && !item->getAmmoItem() && !item->getRules()->getCompatibleAmmo()->empty())
			{
				_ammoNumber->setX((item->getSlot()->getX() + ((item->getSlotX() + item->getRules()->getInventoryWidth()) - _groundOffset) * RuleInventory::SLOT_W)-5);
				if (ammo->getAmmoQuantity() > 99)
				{
					_ammoNumber->setX(_ammoNumber->getX()-8);
				}
				else if (ammo->getAmmoQuantity() > 9)
				{
					_ammoNumber->setX(_ammoNumber->getX()-4);
				}

				_ammoNumber->setY(item->getSlot()->getY() + (item->getSlotY() * RuleInventory::SLOT_H) + 1);
				_ammoNumber->setValue(0);
				_ammoNumber->draw();
				_ammoNumber->setColor(textAmmo->color2);
				_ammoNumber->blit(stackLayer);
			}
		}

		stackLayer->blit(_items);
		delete stackLayer;
	}
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
	if (_selItem == 0)
	{
		_selection->clear();
	}
	else
	{
		if (_selItem->getSlot()->getType() == INV_GROUND)
		{
			_stackLevel[_selItem->getSlotX()][_selItem->getSlotY()] -= 1;
		}
		_selItem->getRules()->drawHandSprite(_game->getMod()->getSurfaceSet("BIGOBS.PCK"), _selection, RuleInventory::MAX_W, RuleInventory::MAX_H);
	}
	drawGrid();
	drawItems();
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
	_animTimer->think(0,this);
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
	_selection->setX((int)floor(action->getAbsoluteXMouse()) - _selection->getWidth()/2 - getX());
	_selection->setY((int)floor(action->getAbsoluteYMouse()) - _selection->getHeight()/2 - getY());
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

	_selection->setX((int)floor(action->getAbsoluteXMouse()) - _selection->getWidth()/2 - getX());
	_selection->setY((int)floor(action->getAbsoluteYMouse()) - _selection->getHeight()/2 - getY());
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
				if (item != 0 && !item->getRules()->isFixed())
				{
					if ((SDL_GetModState() & KMOD_CTRL))
					{
						RuleInventory *newSlot = _game->getMod()->getInventory("STR_GROUND", true);
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
							if(placed = _selUnit->moveItem(item, newSlot, 0, 0, warning, _tu))
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
			int x = _selection->getX() + (RuleInventory::MAX_W - _selItem->getRules()->getInventoryWidth()) * RuleInventory::SLOT_W/2 + RuleInventory::SLOT_W/2,
				y = _selection->getY() + (RuleInventory::MAX_H - _selItem->getRules()->getInventoryHeight()) * RuleInventory::SLOT_H/2 + RuleInventory::SLOT_H/2;
			RuleInventory *slot = getSlotInPosition(&x, &y);
			if (slot != 0)
			{
				if (slot->getType() == INV_GROUND)
				{
					x += _groundOffset;
				}
				BattleItem *item = _selUnit->getItem(slot, x, y);

				bool canStack = slot->getType() == INV_GROUND && canBeStacked(item, _selItem);
				bool validTargetSlot = slot->getType() == INV_GROUND || _selItem->getRules()->isValidSlot(slot);

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
					else if(_selUnit->moveItem(_selItem, slot, x, y, warning, _tu))
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
					if(_selUnit->loadWeapon(item, _selItem, warning))
					{
						setSelectedItem(0);
						_game->getMod()->getSoundByDepth(_depth, Mod::ITEM_RELOAD)->play();
					}
					else
					{
						_warning->showMessage(_game->getLanguage()->getString(warning));
					}
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
						if(_selUnit->moveItem(_selItem, slot, item->getSlotX(), item->getSlotY(), warning, _tu))
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
							BattleType itemType = item->getRules()->getBattleType();
							if (BT_GRENADE == itemType || BT_PROXIMITYGRENADE == itemType)
							{
								if (!item->getGrenadeLive())
								{
									// Prime that grenade!
									if (BT_PROXIMITYGRENADE == itemType)
									{
										_warning->showMessage(_game->getLanguage()->getString("STR_GRENADE_IS_ACTIVATED"));
										item->setFuseTimer(0);
										arrangeGround(false);
									}
									else _game->pushState(new PrimeGrenadeState(0, true, item));
								}
								else
								{
									_warning->showMessage(_game->getLanguage()->getString("STR_GRENADE_IS_DEACTIVATED"));
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
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		arrangeGround(true, -1);
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		arrangeGround(true, 1);
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

	std::string warning;
	BattleItem *ammo = _selItem->getAmmoItem();
	if(_selUnit->unloadWeapon(_selItem, warning, _tu))
	{
		bool arrange = _selItem->getSlot()->getType() == INV_GROUND || (ammo && ammo->getSlot()->getType() == INV_GROUND);
		setSelectedItem(0);
		if(arrange)
		{
			arrangeGround(false);
		}
		else
		{
			drawItems();
		}
		return true;
	}
	else
	{
		_warning->showMessage(_game->getLanguage()->getString(warning));
		return false;
	}
}

/**
 * Arranges items on the ground for the inventory display.
 * Since items on the ground aren't assigned to anyone,
 * they don't actually have permanent slot positions.
 * @param alterOffset Whether to alter the ground offset.
 */
void Inventory::arrangeGround(bool alterOffset, int offset)
{
	RuleInventory *ground = _game->getMod()->getInventory("STR_GROUND", true);
	int slotsX = (320 - ground->getX()) / RuleInventory::SLOT_W;
	int slotsY = (200 - ground->getY()) / RuleInventory::SLOT_H;

	if(offset)
	{
		int xMax = 0;

		for (std::vector<BattleItem*>::iterator ii = _selUnit->getTile()->getInventory()->begin(); ii != _selUnit->getTile()->getInventory()->end(); ++ii)
		{
			BattleItem* item = *ii;
			int width = item->getRules()->getInventoryWidth();
			if(width) { xMax = std::max(xMax, item->getSlotX() + width); }
		}

		_groundOffset = std::max(0, std::min(_groundOffset + offset, xMax - 1));
	}
	else
	{
		int x = 0;
		int y = 0;
		bool ok = false;
		int xMax = 0;
		_stackLevel.clear();

		if (_selUnit != 0)
		{
			// first move all items out of the way - a big number in X direction
			for (std::vector<BattleItem*>::iterator i = _selUnit->getTile()->getInventory()->begin(); i != _selUnit->getTile()->getInventory()->end(); ++i)
			{
				(*i)->setSlot(ground);
				(*i)->setSlotX(1000000);
				(*i)->setSlotY(0);
			}

			// now for each item, find the most topleft position that is not occupied and will fit
			for (std::vector<BattleItem*>::iterator i = _selUnit->getTile()->getInventory()->begin(); i != _selUnit->getTile()->getInventory()->end(); ++i)
			{
				BattleItem *item = *i;
				if(!_tu && (item->getRules() && (item->getRules()->isVehicleItem() != _selUnit->isVehicle())))
				{
					continue;
				}

				x = 0;
				y = 0;
				ok = false;
				while (!ok)
				{
					ok = true; // assume we can put the item here, if one of the following checks fails, we can't.
					for (int xd = 0; xd < item->getRules()->getInventoryWidth() && ok; xd++)
					{
						if ((x + xd) % slotsX < x % slotsX)
						{
							ok = false;
						}
						else
						{
							for (int yd = 0; yd < item->getRules()->getInventoryHeight() && ok; yd++)
							{
								BattleItem *item = _selUnit->getItem(ground, x + xd, y + yd);
								ok = item == 0;
								if (canBeStacked(item, *i))
								{
									ok = true;
								}
							}
						}
					}
					if (ok)
					{
						item->setSlotX(x);
						item->setSlotY(y);
						// only increase the stack level if the item is actually visible.
						if (item->getRules()->getInventoryWidth())
						{
							_stackLevel[x][y] += 1;
						}
						xMax = std::max(xMax, x + item->getRules()->getInventoryWidth());
					}
					else
					{
						y++;
						if (y > slotsY - item->getRules()->getInventoryHeight())
						{
							y = 0;
							x++;
						}
					}
				}
			}
		}
		if (alterOffset)
		{
			if(!offset) { offset = slotsX - 1; }
			if (xMax >= _groundOffset + offset)
			{
				_groundOffset += offset;
			}
			else
			{
				_groundOffset = 0;
			}
		}
	}
	drawItems();
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
	return (itemA != 0 && itemB != 0 &&
		//both items have the same ruleset
		itemA->getRules() == itemB->getRules() &&
		//both items have same ammo quantity
		itemA->getAmmoQuantity() == itemB->getAmmoQuantity() &&
		// either they both have no ammo
		((!itemA->getAmmoItem() && !itemB->getAmmoItem()) ||
		// or they both have ammo
		(itemA->getAmmoItem() && itemB->getAmmoItem() &&
		// and the same ammo type
		itemA->getAmmoItem()->getRules() == itemB->getAmmoItem()->getRules() &&
		// and the same ammo quantity
		itemA->getAmmoItem()->getAmmoQuantity() == itemB->getAmmoItem()->getAmmoQuantity())) &&
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
 */
void Inventory::drawPrimers()
{
	const int Pulsate[8] = { 0, 1, 2, 3, 4, 3, 2, 1 };
	if (_animFrame == 8)
	{
		_animFrame = 0;
	}
	Surface *tempSurface = _game->getMod()->getSurfaceSet("SCANG.DAT")->getFrame(6);
	for (std::vector<std::pair<int, int> >::const_iterator i = _grenadeIndicators.begin(); i != _grenadeIndicators.end(); ++i)
	{
		tempSurface->blitNShade(_items, (*i).first, (*i).second, Pulsate[_animFrame]);
	}
	_animFrame++;
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
