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

#include "BattleItem.h"
#include "BattleUnit.h"
#include "Tile.h"
#include "SavedGame.h"
#include "SavedBattleGame.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleInventory.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Script.h"
#include "../Engine/ScriptBind.h"
#include "../fmath.h"

namespace OpenXcom
{

/**
 * Initializes a item of the specified type.
 * @param rules Pointer to ruleset.
 * @param id The id of the item.
 */
BattleItem::BattleItem(RuleItem *rules, int *id) : _id(*id), _rules(rules), _owner(0), _previousOwner(0), _unit(0), _tile(0), _inventorySlot(0), _inventoryX(0), _inventoryY(0), _ammoItem{ }, _fuseTimer(GRENADE_INACTIVE), _ammoQuantity(0), _painKiller(0), _heal(0), _stimulant(0), _XCOMProperty(false), _droppedOnAlienTurn(false), _isAmmo(false), _isWeaponWithAmmo(false)
{
	(*id)++;
	if (_rules)
	{
		_confMelee = _rules->getConfigMelee();
		setAmmoQuantity(_rules->getClipSize());
		if (_rules->getBattleType() == BT_MEDIKIT)
		{
			setHealQuantity (_rules->getHealQuantity());
			setPainKillerQuantity (_rules->getPainKillerQuantity());
			setStimulantQuantity (_rules->getStimulantQuantity());
			/* Necessary?
			_ammoItem[0] = this;
			_ammoVisibility[0] = false;*/
		}
		// weapon does not need ammo, ammo item points to weapon
		else if (_rules->getBattleType() == BT_FIREARM || _rules->getBattleType() == BT_MELEE || _rules->getBattleType() == BT_GRENADE || _rules->getBattleType() == BT_PSIAMP)
		{
			_confAimedOrLaunch = _rules->getConfigAimed();
			_confAuto = _rules->getConfigAuto();
			_confBurst = _rules->getConfigBurst();
			_confSnap = _rules->getConfigSnap();
			bool showSelfAmmo = _rules->getClipSize() > 0;
			for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
			{
				bool used = false;
				used |= (_confAimedOrLaunch && _confAimedOrLaunch->ammoSlot == slot);
				used |= (_confAuto && _confAuto->ammoSlot == slot);
				used |= (_confBurst && _confBurst->ammoSlot == slot);
				used |= (_confSnap && _confSnap->ammoSlot == slot);
				used |= (_confMelee && _confMelee->ammoSlot == slot);
				if (_rules->getCompatibleAmmoForSlot(slot)->empty())
				{
					if (used && showSelfAmmo)
					{
						_ammoVisibility[slot] = true;
						showSelfAmmo = false;
					}
					_ammoItem[slot] = this;
				}
				else
				{
					_ammoVisibility[slot] = used;
					_isWeaponWithAmmo = true;
				}
			}
		}
	}
}

/**
 *
 */
BattleItem::~BattleItem()
{
}

/**
 * Loads the item from a YAML file.
 * @param node YAML node.
 */
void BattleItem::load(const YAML::Node &node, const ScriptGlobal *shared)
{
	_inventoryX = node["inventoryX"].as<int>(_inventoryX);
	_inventoryY = node["inventoryY"].as<int>(_inventoryY);
	_ammoQuantity = node["ammoqty"].as<int>(_ammoQuantity);
	_painKiller = node["painKiller"].as<int>(_painKiller);
	_heal = node["heal"].as<int>(_heal);
	_stimulant = node["stimulant"].as<int>(_stimulant);
	_fuseTimer = node["fuseTimer"].as<int>(_fuseTimer);
	_droppedOnAlienTurn = node["droppedOnAlienTurn"].as<bool>(_droppedOnAlienTurn);
	_scriptValues.load(node, shared);
}

/**
 * Saves the item to a YAML file.
 * @return YAML node.
 */
YAML::Node BattleItem::save(const ScriptGlobal *shared) const
{
	YAML::Node node;
	node["id"] = _id;
	node["type"] = _rules->getType();
	if (_owner)
	{
		node["owner"] = _owner->getId();
	}
	else
	{
		node["owner"] = -1;
	}
	if (_previousOwner)
	{
		node["previousOwner"] = _previousOwner->getId();
	}
	if (_unit)
	{
		node["unit"] = _unit->getId();
	}
	else
	{
		node["unit"] = -1;
	}

	if (_inventorySlot)
	{
		node["inventoryslot"] = _inventorySlot->getId();
	}
	else
	{
		node["inventoryslot"] = "NULL";
	}
	node["inventoryX"] = _inventoryX;
	node["inventoryY"] = _inventoryY;

	if (_tile)
	{
		node["position"] = _tile->getPosition();
	}
	else
	{
		node["position"] = Position(-1, -1, -1);
	}
	node["ammoqty"] = _ammoQuantity;
	if (_ammoItem[0])
	{
		node["ammoItem"] = _ammoItem[0]->getId();
	}
	else
	{
		node["ammoItem"] = -1;
	}
	for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
	{
		node["ammoItemSlots"].push_back(_ammoItem[slot] ? _ammoItem[slot]->getId() : -1);
	}

	node["painKiller"] = _painKiller;
	node["heal"] = _heal;
	node["stimulant"] = _stimulant;
	node["fuseTimer"] = _fuseTimer;
	if (_droppedOnAlienTurn)
		node["droppedOnAlienTurn"] = _droppedOnAlienTurn;
	_scriptValues.save(node, shared);

	return node;
}

/**
 * Gets the ruleset for the item's type.
 * @return Pointer to ruleset.
 */
const RuleItem *BattleItem::getRules() const
{
	return _rules;
}

/**
 * Gets the turns until detonation. -1 = unprimed grenade
 * @return turns until detonation.
 */
int BattleItem::getFuseTimer() const
{
	return _fuseTimer;
}
/**
 * Checks if the grenade is currently live (the fuse is active).
 * @return True if the fuse is active.
 */
bool BattleItem::getGrenadeLive() const
{
	return _fuseTimer != GRENADE_INACTIVE;
}

/**
 * Checks if the grenade is ready to explode immediately.
 * @return True if the grenade should explode immediately.
 */
bool BattleItem::getFuseDone() const
{
	return _fuseTimer == 0 || getFuseInstant();
}

/**
 * Checks if the grenade is set to explode instantly.
 * @return True if the grenade fuse is set to explode immediately.
 */
bool BattleItem::getFuseInstant() const
{
	return _fuseTimer == GRENADE_INSTANT_FUSE || _rules->getFuseTimerType() == BFT_INSTANT;
}

/**
 * Sets the turn to explode on.
 * @param turns Turns until detonation (player/alien turns, not game turns).
 */
void BattleItem::setFuseTimer(int turns)
{
	_fuseTimer = turns;
}

/**
 * Gets the quantity of ammo in this item.
 * @return Ammo quantity.
 */
int BattleItem::getAmmoQuantity() const
{
	if (_rules->getClipSize() == -1)
	{
		return 255;
	}
	return _ammoQuantity;
}

/**
 * Changes the quantity of ammo in this item.
 * @param qty Ammo quantity.
 */
void BattleItem::setAmmoQuantity(int qty)
{
	_ammoQuantity = qty;
}

/**
 * Spends a bullet from the ammo in this item.
 * @return True if there are bullets left.
 */
bool BattleItem::spendBullet(int bullets)
{
	if(_ammoQuantity < 0) return true;

	_ammoQuantity -= bullets;
	if (_ammoQuantity <= 0)
		return false;
	else
		return true;
}

/**
 * Gets the item's owner.
 * @return Pointer to Battleunit.
 */
BattleUnit *BattleItem::getOwner()
{
	return _owner;
}

/**
 * Gets the item's owner.
 * @return Pointer to Battleunit.
 */
const BattleUnit *BattleItem::getOwner() const
{
	return _owner;
}

/**
 * Gets the item's previous owner.
 * @return Pointer to Battleunit.
 */
BattleUnit *BattleItem::getPreviousOwner()
{
	return _previousOwner;
}

/**
 * Gets the item's previous owner.
 * @return Pointer to Battleunit.
 */
const BattleUnit *BattleItem::getPreviousOwner() const
{
	return _previousOwner;
}

/**
 * Sets the item's owner.
 * @param owner Pointer to Battleunit.
 */
void BattleItem::setOwner(BattleUnit *owner)
{
	_previousOwner = _owner;
	_owner = owner;
}

/**
 * Sets the item's previous owner.
 * @param owner Pointer to Battleunit.
 */
void BattleItem::setPreviousOwner(BattleUnit *owner)
{
	_previousOwner = owner;
}

/**
 * Removes the item from the previous owner and moves it to the new owner.
 * @param owner Pointer to Battleunit.
 */
void BattleItem::moveToOwner(BattleUnit *owner)
{
	_previousOwner = _owner ? _owner:owner;
	_owner = owner;
	_tile = nullptr;
	if (_previousOwner != 0)
	{
		for (std::vector<BattleItem*>::iterator i = _previousOwner->getInventory()->begin(); i != _previousOwner->getInventory()->end(); ++i)
		{
			if ((*i) == this)
			{
				_previousOwner->getInventory()->erase(i);
				_previousOwner->updateStats();
				break;
			}
		}
	}
	if (_owner != 0)
	{
		_owner->getInventory()->push_back(this);
		_owner->updateStats();
	}
}

/**
 * Gets the item's inventory slot.
 * @return The slot id.
 */
RuleInventory *BattleItem::getSlot() const
{
	return _inventorySlot;
}

/**
 * Sets the item's inventory slot.
 * @param slot The slot id.
 */
void BattleItem::setSlot(RuleInventory *slot)
{
	_inventorySlot = slot;
}

/**
 * Gets the item's inventory X position.
 * @return X position.
 */
int BattleItem::getSlotX() const
{
	return _inventoryX;
}

/**
 * Sets the item's inventory X position.
 * @param x X position.
 */
void BattleItem::setSlotX(int x)
{
	_inventoryX = x;
}

/**
 * Gets the item's inventory Y position.
 * @return Y position.
 */
int BattleItem::getSlotY() const
{
	return _inventoryY;
}

/**
 * Sets the item's inventory Y position.
 * @param y Y position.
 */
void BattleItem::setSlotY(int y)
{
	_inventoryY = y;
}

/**
 * Checks if the item is covering certain inventory slot(s).
 * @param x Slot X position.
 * @param y Slot Y position.
 * @param item Item to check for overlap, or NULL if none.
 * @return True if it is covering.
 */
bool BattleItem::occupiesSlot(int x, int y, BattleItem *item) const
{
	if (item == this || !_inventorySlot)
		return false;
	if (_inventorySlot->getType() == INV_HAND || _inventorySlot->getType() == INV_UTILITY || _inventorySlot->getType() == INV_EQUIP)
		return true;
	if (item == 0)
	{
		return (x >= _inventoryX && x < _inventoryX + _rules->getInventoryWidth() &&
				y >= _inventoryY && y < _inventoryY + _rules->getInventoryHeight());
	}
	else
	{
		return !(x >= _inventoryX + _rules->getInventoryWidth() ||
				x + item->getRules()->getInventoryWidth() <= _inventoryX ||
				y >= _inventoryY + _rules->getInventoryHeight() ||
				y + item->getRules()->getInventoryHeight() <= _inventoryY);
	}
}

/**
 * Gets the item's floor sprite.
 * @return Return current floor sprite.
 */
Surface *BattleItem::getFloorSprite(SurfaceSet *set) const
{
	int i = _rules->getFloorSprite();
	if (i != -1)
	{
		Surface *surf = set->getFrame(i);
		//enforce compatibility with basic version
		if (surf == nullptr)
		{
			throw Exception("Invlid surface set 'FLOOROB.PCK' for item '" + _rules->getType() + "': not enough frames");
		}

		ModScript::SelectItemSprite::Output arg{ i, 0 };
		ModScript::SelectItemSprite::Worker work{ this, BODYPART_ITEM_FLOOR, 0, 0 };
		work.execute(_rules->getScript<ModScript::SelectItemSprite>(), arg);

		surf = set->getFrame(arg.getFirst());
		if (surf == nullptr)
		{
			throw Exception("Invlid surface set 'FLOOROB.PCK' for item '" + _rules->getType() + "': not enough frames");
		}
		return surf;
	}
	else
	{
		return nullptr;
	}
}

/**
 * Gets the item's inventory sprite.
 * @return Return current inventory sprite.
 */
Surface *BattleItem::getBigSprite(SurfaceSet *set) const
{
	int i = _rules->getBigSprite();
	if (i != -1)
	{
		Surface *surf = set->getFrame(i);
		//enforce compatibility with basic version
		if (surf == nullptr)
		{
			throw Exception("Invlid surface set 'BIGOBS.PCK' for item '" + _rules->getType() + "': not enough frames");
		}

		ModScript::SelectItemSprite::Output arg{ i, 0 };
		ModScript::SelectItemSprite::Worker work{ this, BODYPART_ITEM_INVENTORY, 0, 0 };
		work.execute(_rules->getScript<ModScript::SelectItemSprite>(), arg);

		surf = set->getFrame(arg.getFirst());
		if (surf == nullptr)
		{
			throw Exception("Invlid surface set 'BIGOBS.PCK' for item '" + _rules->getType() + "': not enough frames");
		}
		return surf;
	}
	else
	{
		return nullptr;
	}
}

/**
 * Check if item use any ammo.
 * @return True if item accept ammo.
 */
bool BattleItem::isWeaponWithAmmo() const
{
	return _isWeaponWithAmmo;
}

/**
 * Check if weapon have enought ammo to shoot.
 * @return True if have ammo.
 */
bool BattleItem::haveAnyAmmo() const
{
	if (!_isWeaponWithAmmo)
	{
		return true;
	}
	auto type = _rules->getBattleType();
	if (type == BT_MELEE)
	{
		return getAmmoForAction(BA_HIT);
	}
	else
	{
		return getAmmoForAction(BA_AIMEDSHOT) ||
			getAmmoForAction(BA_AUTOSHOT) ||
			getAmmoForAction(BA_SNAPSHOT) ||
			getAmmoForAction(BA_BURSTSHOT);
	}
}

/**
 * Check if weapon have all ammo slot filled.
 * @return True if all ammo slots are fill.
 */
bool BattleItem::haveAllAmmo() const
{
	for (const auto& a : _ammoItem)
	{
		if (a == nullptr)
		{
			return false;
		}
	}
	return true;
}

/**
* Check if weapon have full ammo slot filled and completely loaded.
* @return True if all ammo slots are filled and completely loaded.
*/
bool BattleItem::haveFullAmmo() const
{
	for (const auto& a : _ammoItem)
	{
		if (a == nullptr || a->getAmmoQuantity() < std::max(a->getRules()->getClipSize(), a->getRules()->getBattleClipSize()))
		{
			return false;
		}
	}
	return true;
}

/**
 * Sets the item's ammo item.
 * @param item The ammo item.
 * @return True if item fit to weapon.
 */
bool BattleItem::setAmmoPreMission(BattleItem *item)
{
	int slot = _rules->getSlotForAmmo(item->getRules()->getType());
	if (slot >= 0)
	{
		if (_ammoItem[slot])
			return false;

		setAmmoForSlot(slot, item);
		return true;
	}

	return false;
}

/**
 * Get configuration of action on that item.
 * @param action Action type.
 * @return Return config of item action or nullptr for wrong action type or item.
 */
const RuleItemAction *BattleItem::getActionConf(BattleActionType action) const
{
	switch (action)
	{
	case BA_LAUNCH:
	case BA_AIMEDSHOT: return _confAimedOrLaunch;
	case BA_AUTOSHOT: return _confAuto;
	case BA_BURSTSHOT: return _confBurst;
	case BA_SNAPSHOT: return _confSnap;
	case BA_HIT: return _confMelee;
	case BA_OVERWATCH: return getActionConf(_rules->getOverwatchShotAction());
	default: return nullptr;
	}
}

/**
 * Determines if this item uses ammo.
 */
bool BattleItem::needsAmmoForAction(BattleActionType action) const
{
	auto conf = getActionConf(action);
	if (!conf || conf->ammoSlot == -1)
	{
		return false;
	}

	return needsAmmoForSlot(conf->ammoSlot);
}

/**
 * Get ammo used by action.
 * @param action Battle Action done using this item.
 * @return
 */
const BattleItem *BattleItem::getAmmoForAction(BattleActionType action) const
{
	auto conf = getActionConf(action);
	if (!conf)
	{
		return nullptr;
	}
	if (conf->ammoSlot == -1)
	{
		return this;
	}

	auto ammo = getAmmoForSlot(conf->ammoSlot);
	if (ammo && ammo->getAmmoQuantity() == 0)
	{
		return nullptr;
	}
	return ammo;
}

/**
 * Get ammo used by action.
 * @param action Battle Action done using this item.
 * @param message Error message if weapon don't have ammo.
 * @return
 */
BattleItem *BattleItem::getAmmoForAction(BattleActionType action, std::string* message)
{
	auto conf = getActionConf(action);
	if (!conf)
	{
		return nullptr;
	}
	if (conf->ammoSlot == -1)
	{
		return this;
	}

	auto ammo = getAmmoForSlot(conf->ammoSlot);
	if (ammo == nullptr)
	{
		if (message) *message = "STR_NO_AMMUNITION_LOADED";
		return nullptr;
	}
	if (ammo->getAmmoQuantity() == 0)
	{
		if (message) *message = "STR_NO_ROUNDS_LEFT";
		return nullptr;
	}
	return ammo;
}

/**
 * Spend weapon ammo, if depleded remove clip.
 * @param action Battle Action done using this item.
 * @param save Save game.
 */
void BattleItem::spendAmmoForAction(BattleActionType action, SavedBattleGame* save)
{
	if (save->getDebugMode() || getActionConf(action)->ammoSlot == -1)
	{
		return;
	}

	auto ammo = getAmmoForAction(action);
	if (ammo)
	{
		if ((ammo->getRules()->getClipSize() > 0 || ammo->getRules()->getBattleClipSize() > 0) && ammo->spendBullet() == false)
		{
			save->removeItem(ammo);
			ammo->setIsAmmo(false);
			if (ammo != this)
			{
				for (auto& a : _ammoItem)
				{
					if (a == ammo)
					{
						a = nullptr;
					}
				}
			}
			if(_owner) _owner->updateStats();
		}
	}
}

/**
 * Check how many shoots attack can perform.
 * @param action Attack type.
 * @param shotCount Current shot count.
 * @return True if still can shoot.
 */
bool BattleItem::haveNextShotsForAction(BattleActionType action, int shotCount) const
{
	auto conf = getActionConf(action);
	if (conf)
	{
		return shotCount < conf->shots;
	}
	return false;
}

/**
 * Determines if the item uses ammo.
 * @return True if ammo is used.
 */
bool BattleItem::needsAmmoForSlot(int slot) const
{
	return (_isWeaponWithAmmo && _ammoItem[slot] != this); // no ammo for this weapon is needed
}

/**
 * Set ammo slot with new ammo
 * @param slot Ammo slot position.
 * @param item Ammo item.
 * @return Old item if was set.
 */
BattleItem *BattleItem::setAmmoForSlot(int slot, BattleItem* item)
{
	if (!needsAmmoForSlot(slot))
	{
		return nullptr;
	}

	BattleItem *oldItem = _ammoItem[slot];
	if (oldItem)
	{
		oldItem->setIsAmmo(false);
	}
	_ammoItem[slot] = item;
	if (item)
	{
		item->setSlot(nullptr);
		item->setIsAmmo(true);
	}
	return oldItem;
}

/**
 * Gets the item's ammo item.
 * @return The ammo item.
 */
BattleItem *BattleItem::getAmmoForSlot(int slot)
{
	return _ammoItem[slot];
}

/**
 * Gets the item's ammo item.
 * @return The ammo item.
 */
const BattleItem *BattleItem::getAmmoForSlot(int slot) const
{
	return _ammoItem[slot];
}

/**
 * Get ammo count visibility for slot.
 */
bool BattleItem::isAmmoVisibleForSlot(int slot) const
{
	return _ammoVisibility[slot];
}

/**
 * Get item weight with ammo weight.
 * @return Weight.
 */
int BattleItem::getTotalWeight() const
{
	int weight = _rules->getWeight();
	for (const auto& a : _ammoItem)
	{
		if (a && a != this)
		{
			weight += a->_rules->getWeight();
		}
	}
	return weight;
}

/**
 * Gets the ammo item's reload cost.
 * @return The TU cost of reloading this ammo item.
 */
int BattleItem::getAmmoReloadCost() const
{
	// TODO: Should this use the OXCE+ reload modifier instead?
	return (_rules->getWeight() * 2) + 5;
}

/**
 * Get wayponts count of weapon or from ammo.
 * @return Maximum waypoints count or -1 if unlimited.
 */
int BattleItem::getCurrentWaypoints() const
{
	int waypoints = _rules->getWaypoints();
	auto ammo = getAmmoForAction(BA_LAUNCH);
	if (waypoints == 0 && ammo && ammo != this)
	{
		waypoints = ammo->_rules->getWaypoints();
	}
	return waypoints;
}

/**
 * Gets the item's tile.
 * @return The tile.
 */
Tile *BattleItem::getTile() const
{
	return _tile;
}

/**
 * Sets the item's tile.
 * @param tile The tile.
 */
void BattleItem::setTile(Tile *tile)
{
	_tile = tile;
}

/**
 * Gets the item's id.
 * @return The item's id.
 */
int BattleItem::getId() const
{
	return _id;
}

/**
 * Gets the corpse's unit.
 * @return Pointer to BattleUnit.
 */
BattleUnit *BattleItem::getUnit()
{
	return _unit;
}

/**
 * Gets the corpse's unit.
 * @return Pointer to BattleUnit.
 */
const BattleUnit *BattleItem::getUnit() const
{
	return _unit;
}

/**
 * Sets the corpse's unit.
 * @param unit Pointer to BattleUnit.
 */
void BattleItem::setUnit(BattleUnit *unit)
{
	_unit = unit;
}

/**
 * Sets the heal quantity of the item.
 * @param heal The new heal quantity.
 */
void BattleItem::setHealQuantity (int heal)
{
	_heal = heal;
}

/**
 * Gets the heal quantity of the item.
 * @return The new heal quantity.
 */
int BattleItem::getHealQuantity() const
{
	return _heal;
}

/**
 * Sets the pain killer quantity of the item.
 * @param pk The new pain killer quantity.
 */
void BattleItem::setPainKillerQuantity (int pk)
{
	_painKiller = pk;
}

/**
 * Gets the pain killer quantity of the item.
 * @return The new pain killer quantity.
 */
int BattleItem::getPainKillerQuantity() const
{
	return _painKiller;
}

/**
 * Sets the stimulant quantity of the item.
 * @param stimulant The new stimulant quantity.
 */
void BattleItem::setStimulantQuantity (int stimulant)
{
	_stimulant = stimulant;
}

/**
 * Gets the stimulant quantity of the item.
 * @return The new stimulant quantity.
 */
int BattleItem::getStimulantQuantity() const
{
	return _stimulant;
}

/**
 * Sets the XCom property flag. This is to determine at debriefing what goes into the base/craft.
 * @param flag True if it's XCom property.
 */
void BattleItem::setXCOMProperty (bool flag)
{
	_XCOMProperty = flag;
}

/**
 * Gets the XCom property flag. This is to determine at debriefing what goes into the base/craft.
 * @return True if it's XCom property.
 */
bool BattleItem::getXCOMProperty() const
{
	return _XCOMProperty;
}

/**
 * Gets the "dropped on non-player turn" flag. This is to determine whether or not
 * aliens should attempt to pick this item up, as items dropped by the player may be "honey traps".
 * @return True if the aliens dropped the item.
 */
bool BattleItem::getTurnFlag() const
{
	return _droppedOnAlienTurn;
}

/**
 * Sets the "dropped on non-player turn" flag. This is set when the item is dropped in the battlescape
 * or picked up in the inventory screen.
 * @param flag True if the aliens dropped the item.
 */
void BattleItem::setTurnFlag(bool flag)
{
	_droppedOnAlienTurn = flag;
}

/**
 * Converts an unconscious body into a dead one.
 * @param rules the rules of the corpse item to convert this item into.
 */
void BattleItem::convertToCorpse(RuleItem *rules)
{
	if (_unit && _rules->getBattleType() == BT_CORPSE && rules->getBattleType() == BT_CORPSE)
	{
		_rules = rules;
	}
}

/**
 * Check if item can glow in darkness.
 * @return True if it glow.
 */
bool BattleItem::getGlow() const
{
	return _rules->getBattleType() == BT_FLARE && (_rules->getFuseTimerType() == BFT_NONE || _fuseTimer >= 0);
}

/**
 * Gets range of glow in tiles.
 * @return Range.
 */
int BattleItem::getGlowRange() const
{
	auto owner = _unit ? _unit : _previousOwner;
	return owner ? _rules->getPowerBonus(owner) : _rules->getPower();
}

/**
 * Gets update range needed by this item. For simplicity we always update to max range if it can glow.
 * @return Range.
 */
int BattleItem::getVisibilityUpdateRange() const
{
	return _rules->getBattleType() == BT_FLARE ? getGlowRange() : 1;
}

/**
 * Sets the flag on this item indicating whether or not it is a clip used in a weapon.
 * @param ammo set the ammo flag to this.
 */
void BattleItem::setIsAmmo(bool ammo)
{
	_isAmmo = ammo;
}

/**
 * Checks if this item is loaded into a weapon.
 * @return if this is loaded into a weapon or not.
 */
bool BattleItem::isAmmo() const
{
	return _isAmmo;
}


namespace
{

struct getAmmoForSlotScript
{
	static RetEnum func(BattleItem *weapon, BattleItem *&ammo, int slot)
	{
		if (weapon && slot >= 0 && slot < RuleItem::AmmoSlotMax)
		{
			ammo = weapon->getAmmoForSlot(slot);
		}
		else
		{
			ammo = nullptr;
		}
		return RetContinue;
	}
};

struct getAmmoForSlotConstScript
{
	static RetEnum func(const BattleItem *weapon, const BattleItem *&ammo, int slot)
	{
		if (weapon && slot >= 0 && slot < RuleItem::AmmoSlotMax)
		{
			ammo = weapon->getAmmoForSlot(slot);
		}
		else
		{
			ammo = nullptr;
		}
		return RetContinue;
	}
};

struct getAmmoItemScript
{
	static RetEnum func(BattleItem *weapon, BattleItem *&ammo)
	{
		return getAmmoForSlotScript::func(weapon, ammo, 0);
	}
};

struct getAmmoItemConstScript
{
	static RetEnum func(const BattleItem *weapon, const BattleItem *&ammo)
	{
		return getAmmoForSlotConstScript::func(weapon, ammo, 0);
	}
};

struct getAmmoForActionScript
{
	static RetEnum func(BattleItem *weapon, BattleItem *&ammo, int action)
	{
		BattleActionType bat = (BattleActionType)action;
		if (weapon)
		{
			ammo = weapon->getAmmoForAction(bat);
		}
		else
		{
			ammo = nullptr;
		}
		return RetContinue;
	}
};

struct getAmmoForActionConstScript
{
	static RetEnum func(const BattleItem *weapon, const BattleItem *&ammo, int action)
	{
		BattleActionType bat = (BattleActionType)action;
		if (weapon)
		{
			ammo = weapon->getAmmoForAction(bat);
		}
		else
		{
			ammo = nullptr;
		}
		return RetContinue;
	}
};

std::string debugDisplayScript(const BattleItem* bt)
{
	if (bt)
	{
		auto rule = bt->getRules();
		std::string s;
		s += BattleItem::ScriptName;
		s += "(name: \"";
		s += rule->getName();
		s += "\" id: ";
		s += std::to_string(bt->getId());

		auto clipSize = rule->getClipSize();
		if (clipSize > 0)
		{
			s += " ammo: ";
			s += std::to_string(bt->getAmmoQuantity());
			s += "/";
			s += std::to_string(clipSize);
		}
		s += ")";
		return s;
	}
	else
	{
		return "null";
	}
}

void setFuseTimerScript(BattleItem* bt, int i)
{
	if (bt)
	{
		bt->setFuseTimer(Clamp(i, 1, 100));
	}
}

void setAmmoQuantityScript(BattleItem* bt, int i)
{
	if (bt)
	{
		bt->setAmmoQuantity(Clamp(i, 1, bt->getRules()->getClipSize()));
	}
}

void setHealQuantityScript(BattleItem* bt, int i)
{
	if (bt)
	{
		bt->setHealQuantity(Clamp(i, 0, bt->getRules()->getHealQuantity()));
	}
}

void setPainKillerQuantityScript(BattleItem* bt, int i)
{
	if (bt)
	{
		bt->setPainKillerQuantity(Clamp(i, 0, bt->getRules()->getPainKillerQuantity()));
	}
}

void setStimulantQuantityScript(BattleItem* bt, int i)
{
	if (bt)
	{
		bt->setStimulantQuantity(Clamp(i, 0, bt->getRules()->getStimulantQuantity()));
	}
}

}

/**
 * Register BattleItem in script parser.
 * @param parser Script parser.
 */
void BattleItem::ScriptRegister(ScriptParserBase* parser)
{
	parser->registerPointerType<Mod>();
	parser->registerPointerType<RuleItem>();
	parser->registerPointerType<BattleUnit>();

	Bind<BattleItem> bi = { parser };

	bi.addRules<RuleItem, &BattleItem::getRules>("getRuleItem");
	bi.addPair<BattleUnit, &BattleItem::getUnit, &BattleItem::getUnit>("getBattleUnit");
	bi.addFunc<getAmmoItemScript>("getAmmoItem");
	bi.addFunc<getAmmoItemConstScript>("getAmmoItem");
	bi.addFunc<getAmmoForSlotScript>("getAmmoForSlot");
	bi.addFunc<getAmmoForSlotConstScript>("getAmmoForSlot");
	bi.addFunc<getAmmoForActionScript>("getAmmoForAction");
	bi.addFunc<getAmmoForActionConstScript>("getAmmoForAction");
	bi.addPair<BattleUnit, &BattleItem::getPreviousOwner, &BattleItem::getPreviousOwner>("getPreviousOwner");
	bi.addPair<BattleUnit, &BattleItem::getOwner, &BattleItem::getOwner>("getOwner");
	bi.add<&BattleItem::getId>("getId");
	bi.add<&BattleItem::getGlow>("getGlow");
	bi.add<&BattleItem::getTotalWeight>("getTotalWeight");
	bi.add<&BattleItem::isAmmo>("isAmmo");

	bi.add<&BattleItem::getAmmoQuantity>("getAmmoQuantity");
	bi.add<&setAmmoQuantityScript>("setAmmoQuantity");

	bi.add<&BattleItem::getFuseTimer>("getFuseTimer");
	bi.add<&setFuseTimerScript>("setFuseTimer");

	bi.add<&BattleItem::getHealQuantity>("getHealQuantity");
	bi.add<&setHealQuantityScript>("setHealQuantity");

	bi.add<&BattleItem::getPainKillerQuantity>("getPainKillerQuantity");
	bi.add<&setPainKillerQuantityScript>("setPainKillerQuantity");

	bi.add<&BattleItem::getStimulantQuantity>("getStimulantQuantity");
	bi.add<&setStimulantQuantityScript>("setStimulantQuantity");

	bi.addScriptValue<&BattleItem::_scriptValues>();
	bi.addDebugDisplay<&debugDisplayScript>();

	bi.addCustomConst("BA_AUTOSHOT", BA_AUTOSHOT);
	bi.addCustomConst("BA_SNAPSHOT", BA_SNAPSHOT);
	bi.addCustomConst("BA_AIMEDSHOT", BA_AIMEDSHOT);
	bi.addCustomConst("BA_LAUNCH", BA_LAUNCH);
	bi.addCustomConst("BA_HIT", BA_HIT);
	bi.addCustomConst("BA_NONE", BA_NONE);
}

namespace
{

void commonImpl(BindBase& b, Mod* mod)
{
	b.addCustomPtr<const Mod>("rules", mod);

	b.addCustomConst("blit_item_righthand", BODYPART_ITEM_RIGHTHAND);
	b.addCustomConst("blit_item_lefthand", BODYPART_ITEM_LEFTHAND);
	b.addCustomConst("blit_item_floor", BODYPART_ITEM_FLOOR);
	b.addCustomConst("blit_item_big", BODYPART_ITEM_INVENTORY);
}

}

/**
 * Constructor of recolor script parser.
 */
ModScript::RecolorItemParser::RecolorItemParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name, "new_pixel", "old_pixel", "item", "blit_part", "anim_frame", "shade" }
{
	BindBase b { this };

	commonImpl(b, mod);

	setDefault("add_shade new_pixel shade; return new_pixel;");
}

/**
 * Constructor of select sprite script parser.
 */
ModScript::SelectItemParser::SelectItemParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name, "sprite_index", "sprite_offset", "item", "blit_part", "anim_frame", "shade" }
{
	BindBase b { this };

	commonImpl(b, mod);

	setDefault("add sprite_index sprite_offset; return sprite_index;");
}

ModScript::CreateItemParser::CreateItemParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name, "item", "battle_game", "turn", }
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);
}

ModScript::NewTurnItemParser::NewTurnItemParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name, "item", "battle_game", "turn", "side", }
{
	BindBase b { this };

	b.addCustomPtr<const Mod>("rules", mod);
}

/**
 * Init all required data in script using object data.
 */
void BattleItem::ScriptFill(ScriptWorkerBlit* w, BattleItem* item, int part, int anim_frame, int shade)
{
	w->clear();
	if(item)
	{
		const auto &scr = item->getRules()->getScript<ModScript::RecolorItemSprite>();
		if (scr)
		{
			w->update(scr, item, part, anim_frame, shade);
		}
		else
		{
			BattleUnit::ScriptFill(w, item->getUnit(), part, anim_frame, shade, 0);
		}
	}
}

}
