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
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include "Unit.h"

namespace OpenXcom
{

enum ItemDamageType { DT_NONE, DT_AP, DT_IN, DT_HE, DT_LASER, DT_PLASMA, DT_STUN, DT_MELEE, DT_ACID, DT_SMOKE, DT_PSYCHIC, DT_TELEPORT };
enum BattleType { BT_NONE, BT_FIREARM, BT_AMMO, BT_MELEE, BT_GRENADE, BT_PROXIMITYGRENADE, BT_MEDIKIT, BT_SCANNER, BT_MINDPROBE, BT_PSIAMP, BT_FLARE, BT_CORPSE, BT_ARMOR, BT_EQUIPMENT };

class SurfaceSet;
class Surface;
class Mod;
class RuleInventory;

/**
 * Represents a specific type of item.
 * Contains constant info about an item like
 * storage size, sell price, etc.
 * @sa Item
 */
class RuleItem
{
private:
	std::string _type, _name; // two types of objects can have the same name
	std::vector<std::string> _requires;
	std::vector<std::string> _categories;
	double _size;
	int _costBuy, _costSell, _transferTime, _weight;
	int _bigSprite, _floorSprite, _handSprite, _bulletSprite;
	int _fireSound, _hitSound, _hitAnimation;
	int _power;
	std::vector<std::string> _compatibleAmmo;
	ItemDamageType _damageType;
	int _baseAccuracy, _accuracyAuto, _accuracySnap, _accuracyAimed, _accuracyBurst, _tuAuto, _tuSnap, _tuAimed, _tuBurst;
	int _clipSize, _accuracyMelee, _tuMelee;
	int _accuracyShotgunSpread, _autoDelay;
	int _aiRangeClose, _aiRangeMid, _aiRangeLong, _aiRangeMax;
	std::vector<std::string> _aiAttackPriorityClose, _aiAttackPriorityMid, _aiAttackPriorityLong, _aiAttackPriorityMax;
	int _overwatchModifier, _overwatchRadius, _overwatchRange;
	std::string _overwatchShot;
	BattleType _battleType;
	bool _twoHanded, _fixedWeapon;
	int _waypoints, _invWidth, _invHeight;
	int _painKiller, _heal, _stimulant;
	int _woundRecovery, _healthRecovery, _stunRecovery, _energyRecovery;
	int _tuUse;
	int _recoveryPoints;
	int _armor;
	int _turretType;
	bool _recover, _liveAlien;
	int _blastRadius, _attraction;
	bool _flatRate, _arcingShot;
	int _listOrder, _maxRange, _aimRange, _snapRange, _autoRange, _burstRange, _minRange, _dropoff, _bulletSpeed, _explosionSpeed, _autoShots, _shotgunPellets, _burstShots;
	std::string _zombieUnit;
	bool _strengthApplied, _skillApplied, _LOSRequired, _underwaterOnly, _landOnly;
	int _meleeSound, _meleePower, _meleeAnimation, _meleeHitSound, _specialType, _vaporColor, _vaporDensity, _vaporProbability;
	int _kneelModifier, _reactionsModifier, _blastDropoff;
	int _psiCostPanic, _psiCostMindControl, _psiCostClairvoyance, _psiCostMindBlast;
	int _twoHandedModifier, _battleClipSize;
	bool _vehicleItem;
	UnitStats _stats, _statModifiers;
	bool _hasStats;
	int _frontArmor, _sideArmor, _rearArmor, _underArmor, _armorSide;
	std::vector<std::string> _validSlots;
	bool _checkValidSlots;
	std::string _hitEffect;
	std::string _equippedEffect;
	int _ammoRegen;
public:
	/// Creates a blank item ruleset.
	RuleItem(const std::string &type);
	/// Cleans up the item ruleset.
	~RuleItem();
	/// Loads item data from YAML.
	void load(const YAML::Node& node, Mod *mod, int listIndex);
	/// Gets the item's type.
	std::string getType() const;
	/// Gets the item's name.
	std::string getName() const;
	/// Gets the item's requirements.
	const std::vector<std::string> &getRequirements() const;
	/// Gets the item's categories.
	const std::vector<std::string> &getCategories() const;
	/// Checks if the item belongs to a category.
	bool belongsToCategory(const std::string &category) const;
	/// Gets the item's size.
	double getSize() const;
	/// Gets the item's purchase cost.
	int getBuyCost() const;
	/// Gets the item's sale cost.
	int getSellCost() const;
	/// Gets the item's transfer time.
	int getTransferTime() const;
	/// Gets the item's weight.
	int getWeight() const;
	/// Gets the item's reference in BIGOBS.PCK for use in inventory.
	int getBigSprite() const;
	/// Gets the item's reference in FLOOROB.PCK for use in inventory.
	int getFloorSprite() const;
	/// Gets the item's reference in HANDOB.PCK for use in inventory.
	int getHandSprite() const;
	/// Gets if the item is two-handed.
	bool isTwoHanded() const;
	/// Gets the item's two-handed modifier.
	int getTwoHandedModifier() const;
	/// Gets if the item is fixed.
	bool isFixed() const;
	/// Gets if the item is a launcher.
	int getWaypoints() const;
	/// Gets the item's bullet sprite reference.
	int getBulletSprite() const;
	/// Gets the item's fire sound.
	int getFireSound() const;
	/// Gets the item's hit sound.
	int getHitSound() const;
	/// Gets the item's hit animation.
	int getHitAnimation() const;
	/// Gets the item's power.
	int getPower() const;
	/// Gets the item's base accuracy.
	int getBaseAccuracy() const;
	/// Gets the item's snapshot accuracy.
	int getAccuracySnap() const;
	/// Gets the item's autoshot accuracy.
	int getAccuracyAuto() const;
	/// Gets the item's aimed shot accuracy.
	int getAccuracyAimed() const;
	/// Gets the item's burstshot accuracy.
	int getAccuracyBurst() const;
	/// Gets the item's melee accuracy.
	int getAccuracyMelee() const;
	/// Gets the item's shotgun spread accuracy.
	int getAccuracyShotgunSpread() const;
	/// Gets the item's snapshot TU cost.
	int getTUSnap() const;
	/// Gets the item's autoshot TU cost.
	int getTUAuto() const;
	/// Gets the item's aimed shot TU cost.
	int getTUAimed() const;
	/// Gets the item's burstshot TU cost.
	int getTUBurst() const;
	/// Gets the item's melee TU cost.
	int getTUMelee() const;
	/// Gets list of compatible ammo.
	std::vector<std::string> *getCompatibleAmmo();
	/// Gets the item's damage type.
	ItemDamageType getDamageType() const;
	/// Gets the item's type.
	BattleType getBattleType() const;
	/// Gets the item's inventory width.
	int getInventoryWidth() const;
	/// Gets the item's inventory height.
	int getInventoryHeight() const;
	/// Gets the ammo amount.
	int getClipSize() const;
	/// Gets the battlescape ammo amount.
	int getBattleClipSize() const;
	/// Draws the item's hand sprite onto a surface.
	void drawHandSprite(SurfaceSet *texture, Surface *surface, int maxW = 0, int maxH = 0, bool utilitySlot = false) const;
	/// Gets the medikit heal quantity.
	int getHealQuantity() const;
	/// Gets the medikit pain killer quantity.
	int getPainKillerQuantity() const;
	/// Gets the medikit stimulant quantity.
	int getStimulantQuantity() const;
	/// Gets the medikit wound healed per shot.
	int getWoundRecovery() const;
	/// Gets the medikit health recovered per shot.
	int getHealthRecovery() const;
	/// Gets the medikit energy recovered per shot.
	int getEnergyRecovery() const;
	/// Gets the medikit stun recovered per shot.
	int getStunRecovery() const;
	/// Gets the Time Unit use.
	int getTUUse() const;
	/// Gets the max explosion radius.
	int getExplosionRadius() const;
	/// Gets the recovery points score
	int getRecoveryPoints() const;
	/// Gets the item's armor.
	int getArmor() const;
	/// Gets the item's recoverability.
	bool isRecoverable() const;
	/// Gets the item's turret type.
	int getTurretType() const;
	/// Checks if this a live alien.
	bool isAlien() const;
	/// Should we charge a flat rate?
	bool getFlatRate() const;
	/// Should this weapon arc?
	bool getArcingShot() const;
	/// How much do aliens want this thing?
	int getAttraction() const;
	/// Get the list weight for this item.
	int getListOrder() const;
	/// How fast does a projectile fired from this weapon travel?
	int getBulletSpeed() const;
	/// How fast does the explosion animation play?
	int getExplosionSpeed() const;
	/// How many auto shots does this weapon fire.
	int getAutoShots() const;
	/// How many burst shots does this weapon fire.
	int getBurstShots() const;
	/// is this item a 2 handed weapon?
	bool isRifle() const;
	/// is this item a single handed weapon?
	bool isPistol() const;
	/// Get the max range of this weapon.
	int getMaxRange() const;
	/// Get the max range of aimed shots with this weapon.
	int getAimRange() const;
	/// Get the max range of snap shots with this weapon.
	int getSnapRange() const;
	/// Get the max range of auto shots with this weapon.
	int getAutoRange() const;
	/// Get the max range of burst shots with this weapon.
	int getBurstRange() const;
	/// Get the minimum effective range of this weapon.
	int getMinRange() const;
	/// Get the accuracy dropoff of this weapon.
	int getDropoff() const;
	/// Get the number of projectiles to trace.
	int getShotgunPellets() const;
	/// Gets the weapon's zombie unit.
	std::string getZombieUnit() const;
	/// Is strength applied to the damage of this weapon?
	bool isStrengthApplied() const;
	/// Is skill applied to the accuracy of this weapon?
	bool isSkillApplied() const;
	/// What sound does this weapon make when you swing this at someone?
	int getMeleeAttackSound() const;
	/// What sound does this weapon make when you punch someone in the face with it?
	int getMeleeHitSound() const;
	/// Ok, so this isn't a melee type weapon but we're using it for melee... how much damage should it do?
	int getMeleePower() const;
	/// Get the melee animation starting frame (comes from hit.pck).
	int getMeleeAnimation() const;
	/// Check if LOS is required to use this item (only applies to psionic type items)
	bool isLOSRequired() const;
	/// Is this item restricted to underwater use?
	bool isWaterOnly() const;
	/// Is this item restricted to land use?
	bool isLandOnly() const;
	/// Get the associated special type of this item.
	int getSpecialType() const;
	/// Get the color offset to use for the vapor trail.
	int getVaporColor() const;
	/// Gets the vapor cloud density.
	int getVaporDensity() const;
	/// Gets the vapor cloud probability.
	int getVaporProbability() const;

	/// Get the kneeling accuracy modifier for this weapon.
	int getKneelModifier() const;
	/// Get the reactions modifier for this weapon.
	int getReactionsModifier() const;
	/// Get the delay between auto fire shots.
	int getAutoDelay() const;
	/// Get the drop-off rate per tile for explosions from this ammo item.
	int getExplosionDropoff() const;

	int getAiRangeClose() const;
	int getAiRangeMid() const;
	int getAiRangeLong() const;
	int getAiRangeMax() const;

	const std::vector<std::string> getAiAttackPriorityClose() const;
	const std::vector<std::string> getAiAttackPriorityMid() const;
	const std::vector<std::string> getAiAttackPriorityLong() const;
	const std::vector<std::string> getAiAttackPriorityMax() const;

	int getOverwatchModifier() const;
	int getOverwatchRadius() const;
	int getOverwatchRange() const;
	const std::string &getOverwatchShot() const;
	bool isAmmo() const;

	int getPsiCostPanic() const;
	int getPsiCostMindControl() const;
	int getPsiCostClairvoyance() const;
	int getPsiCostMindBlast() const;

	bool isVehicleItem() const;

	bool hasStats() const;
	UnitStats *getStats();
	UnitStats *getStatModifiers();

	/// Gets the front armor level.
	int getFrontArmor() const;
	/// Gets the side armor level.
	int getSideArmor() const;
	/// Gets the rear armor level.
	int getRearArmor() const;
	/// Gets the under armor level.
	int getUnderArmor() const;
	/// Gets the ammo regen per turn.
	int getAmmoRegen() const;

	bool isValidSlot(const RuleInventory* slot) const;

	const std::string &getHitEffect() const;
	const std::string &getEquippedEffect() const;
};

}
