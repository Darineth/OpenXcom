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
#include "RuleStatBonus.h"
#include "RuleDamageType.h"
#include "Unit.h"
#include "ModScript.h"

namespace OpenXcom
{

enum BattleType { BT_NONE, BT_FIREARM, BT_AMMO, BT_MELEE, BT_GRENADE, BT_PROXIMITYGRENADE, BT_MEDIKIT, BT_SCANNER, BT_MINDPROBE, BT_PSIAMP, BT_FLARE, BT_CORPSE, BT_ARMOR, BT_EQUIPMENT };
enum BattleFuseType { BFT_NONE = -3, BFT_INSTANT = -2, BFT_SET = -1, BFT_FIX_MIN = 0, BFT_FIX_MAX = 24 };
enum BattleMediKitType { BMT_NORMAL = 0, BMT_HEAL = 1, BMT_STIMULANT = 2, BMT_PAINKILLER = 3 };
enum ExperienceTrainingMode {
	ETM_DEFAULT,
	ETM_MELEE_100, ETM_MELEE_50, ETM_MELEE_33,
	ETM_FIRING_100, ETM_FIRING_50, ETM_FIRING_33,
	ETM_THROWING_100, ETM_THROWING_50, ETM_THROWING_33,
	ETM_FIRING_AND_THROWING,
	ETM_FIRING_OR_THROWING,
	ETM_REACTIONS,
	ETM_REACTIONS_AND_MELEE, ETM_REACTIONS_AND_FIRING, ETM_REACTIONS_AND_THROWING,
	ETM_REACTIONS_OR_MELEE, ETM_REACTIONS_OR_FIRING, ETM_REACTIONS_OR_THROWING,
	ETM_BRAVERY, ETM_BRAVERY_2X,
	ETM_BRAVERY_AND_REACTIONS,
	ETM_BRAVERY_OR_REACTIONS, ETM_BRAVERY_OR_REACTIONS_2X,
	ETM_PSI_STRENGTH, ETM_PSI_STRENGTH_2X,
	ETM_PSI_SKILL, ETM_PSI_SKILL_2X,
	ETM_PSI_STRENGTH_AND_SKILL, ETM_PSI_STRENGTH_AND_SKILL_2X,
	ETM_PSI_STRENGTH_OR_SKILL, ETM_PSI_STRENGTH_OR_SKILL_2X,
	ETM_NOTHING
};

class BattleItem;
class SurfaceSet;
class Surface;
class Mod;
class RuleInventory;

struct RuleItemUseCost
{
	int Time;
	int Energy;
	int Morale;
	int Health;
	int Stun;

	/// Default constructor.
	RuleItemUseCost() : Time(0), Energy(0), Morale(0), Health(0), Stun(0)
	{

	}
	/// Create new cost with one value for time units and another for rest.
	RuleItemUseCost(int tu, int rest = 0) : Time(tu), Energy(rest), Morale(rest), Health(rest), Stun(rest)
	{

	}

	/// Add cost.
	RuleItemUseCost& operator+=(const RuleItemUseCost& cost)
	{
		Time += cost.Time;
		Energy += cost.Energy;
		Morale += cost.Morale;
		Health += cost.Health;
		Stun += cost.Stun;
		return *this;
	}
};

/**
 * Common configuration of item action.
 */
struct RuleItemAction
{
	int accuracy = 0;
	int range = 0;
	int shots = 1;
	int ammoSlot = 0;
	RuleItemUseCost cost;
	RuleItemUseCost flat;
	std::string name;
};

/**
 * Represents a specific type of item.
 * Contains constant info about an item like
 * storage size, sell price, etc.
 * @sa Item
 */
class RuleItem
{
public:
	/// Maximum number of ammo slots on weapon.
	static const int AmmoSlotMax = 4;

private:
	std::string _type, _name, _nameAsAmmo; // two types of objects can have the same name
	std::vector<std::string> _requires;
	std::vector<std::string> _requiresBuy;
	std::vector<std::string> _categories;
	double _size;
	int _costBuy, _costSell, _transferTime, _weight;
	int _bigSprite;
	int _floorSprite;
	int _handSprite, _bulletSprite;
	std::vector<int> _fireSound, _hitSound; 
	int _hitAnimation;
	std::vector<int> _hitMissSound;
	int _hitMissAnimation;
	std::vector<int> _meleeSound;
	int _meleeAnimation;
	std::vector<int> _meleeMissSound;
	int _meleeMissAnimation;
	std::vector<int> _meleeHitSound, _explosionHitSound, _psiSound;
	int _psiAnimation;
	std::vector<int> _psiMissSound;
	int _psiMissAnimation;
	int _power;
	float _powerRangeReduction;
	float _powerRangeThreshold;
	std::vector<std::string> _compatibleAmmo[AmmoSlotMax];
	RuleDamageType _damageType, _meleeType;
	int _baseAccuracy;
	RuleItemAction _confAimed, _confAuto, _confBurst, _confSnap, _confMelee;
	int _accuracyUse, _accuracyMind, _accuracyPanic, _accuracyThrow, _accuracyCloseQuarters;
	RuleItemUseCost _costUse, _costMind, _costPanic, _costThrow, _costPrime, _costUnprime;
	int _clipSize, _battleClipSize, _specialChance, _tuLoad[AmmoSlotMax], _tuUnload[AmmoSlotMax];
	int _accuracyShotgunSpread, _autoDelay;
	int _aiRangeClose, _aiRangeMid, _aiRangeLong, _aiRangeMax;
	std::vector<std::string> _aiAttackPriorityClose, _aiAttackPriorityMid, _aiAttackPriorityLong, _aiAttackPriorityMax;
	int _overwatchModifier, _overwatchRadius, _overwatchRange;
	std::string _overwatchShot;
	BattleType _battleType;
	BattleFuseType _fuseType;
	bool _hiddenOnMinimap;
	std::string _psiAttackName, _primeActionName, _unprimeActionName, _primeActionMessage, _unprimeActionMessage;
	bool _twoHanded, _blockBothHands, _fixedWeapon, _fixedWeaponShow, _allowSelfHeal, _isConsumable, _isFireExtinguisher, _isExplodingInHands;
	std::string _defaultInventorySlot;
	std::vector<std::string> _supportedInventorySections;
	int _waypoints, _invWidth, _invHeight;
	int _painKiller, _heal, _stimulant;
	BattleMediKitType _medikitType;
	int _woundRecovery, _healthRecovery, _stunRecovery, _energyRecovery, _moraleRecovery, _painKillerRecovery;
	int _recoveryPoints;
	int _armor;
	int _turretType;
	int _aiUseDelay, _aiMeleeHitCount;
	bool _recover, _ignoreInBaseDefense, _liveAlien;
	int _liveAlienPrisonType;
	int _attraction;
	RuleItemUseCost _flatUse, _flatThrow, _flatPrime, _flatUnprime;
	bool _arcingShot;
	ExperienceTrainingMode _experienceTrainingMode;
	int _listOrder, _maxRange, _minRange, _dropoff, _bulletSpeed, _explosionSpeed, _shotgunPellets, _aimRange, _snapRange, _autoRange, _burstRange;
	int _shotgunBehaviorType, _shotgunSpread, _shotgunChoke;
	std::string _zombieUnit;
	bool _LOSRequired, _underwaterOnly, _landOnly, _psiRequired;
	int _meleePower, _specialType, _vaporColor, _vaporDensity, _vaporProbability;
	int _customItemPreviewIndex;
	int _kneelBonus, _oneHandedPenalty, _reactionsModifier, _blastDropoff;
	int _monthlySalary, _monthlyMaintenance;
	RuleStatBonus _damageBonus, _meleeBonus, _accuracyMulti, _meleeMulti, _throwMulti, _closeQuartersMulti;
	//int _psiCostPanic, _psiCostMindControl, _psiCostClairvoyance, _psiCostMindBlast;
	ModScript::BattleItemScripts::Container _battleItemScripts;
	ScriptValues<RuleItem> _scriptValues;

	/// Get final value of cost.
	RuleItemUseCost getDefault(const RuleItemUseCost& a, const RuleItemUseCost& b) const;
	/// Load bool as int from yaml.
	void loadBool(int& a, const YAML::Node& node) const;
	/// Load int from yaml.
	void loadInt(int& a, const YAML::Node& node) const;
	/// Load RuleItemUseCost from yaml.
	void loadCost(RuleItemUseCost& a, const YAML::Node& node, const std::string& name) const;
	/// Load RuleItemUseCost as bool from yaml.
	void loadPercent(RuleItemUseCost& a, const YAML::Node& node, const std::string& name) const;
	/// Load RuleItemAction from yaml.
	void loadConfAction(RuleItemAction& a, const YAML::Node& node, const std::string& name) const;
	/// Load sound vector from YAML.
	void loadSoundVector(const YAML::Node &node, Mod *mod, std::vector<int> &vector);
	/// Gets a random sound from a given vector.
	int getRandomSound(const std::vector<int> &vector, int defaultValue = -1) const;

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
	/// Name of class used in script.
	static constexpr const char *ScriptName = "RuleItem";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);

	/// Creates a blank item ruleset.
	RuleItem(const std::string &type);
	/// Cleans up the item ruleset.
	~RuleItem();
	/// Updates item categories based on replacement rules.
	void updateCategories(std::map<std::string, std::string> *replacementRules);
	/// Loads item data from YAML.
	void load(const YAML::Node& node, Mod *mod, int listIndex, const ModScript& parsers);
	/// Gets the item's type.
	const std::string &getType() const;
	/// Gets the item's name.
	const std::string &getName() const;
	/// Gets the item's name when loaded in weapon.
	const std::string &getNameAsAmmo() const;
	/// Gets the item's requirements.
	const std::vector<std::string> &getRequirements() const;
	/// Gets the item's buy requirements.
	const std::vector<std::string> &getBuyRequirements() const;
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
	/// Gets the item's reference in FLOOROB.PCK for use in battlescape.
	int getFloorSprite() const;
	/// Gets the item's reference in HANDOB.PCK for use in inventory.
	int getHandSprite() const;
	/// Gets if the item is two-handed.
	bool isTwoHanded() const;
	/// Gets the item's two-handed modifier.
	int getTwoHandedModifier() const;
	/// Gets if the item can only be used by both hands.
	bool isBlockingBothHands() const;
	/// Gets if the item is fixed.
	bool isFixed() const;
	/// Do show fixed weapon on unit.
	bool getFixedShow() const;
	/// Get name of the default inventory slot.
	const std::string &getDefaultInventorySlot() const;
	/// Gets the item's supported inventory sections.
	const std::vector<std::string> &getSupportedInventorySections() const;
	/// Checks if the item can be placed into a given inventory section.
	bool canBePlacedIntoInventorySection(const std::string &inventorySection) const;
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
	/// Gets the item's hit sound.
	int getHitMissSound() const;
	/// Gets the item's hit animation.
	int getHitMissAnimation() const;

	/// What sound does this weapon make when you swing this at someone?
	int getMeleeSound() const;
	/// Get the melee animation starting frame (comes from hit.pck).
	int getMeleeAnimation() const;
	/// What sound does this weapon make when you miss a swing?
	int getMeleeMissSound() const;
	/// Get the melee miss animation starting frame (comes from hit.pck).
	int getMeleeMissAnimation() const;
	/// What sound does this weapon make when you punch someone in the face with it?
	int getMeleeHitSound() const;
	/// What sound does explosion sound?
	int getExplosionHitSound() const;

	/// Gets the item's psi hit sound.
	int getPsiSound() const;
	/// Get the psi animation starting frame (comes from hit.pck).
	int getPsiAnimation() const;
	/// Gets the item's psi miss sound.
	int getPsiMissSound() const;
	/// Get the psi miss animation starting frame (comes from hit.pck).
	int getPsiMissAnimation() const;

	/// Gets the item's power.
	int getPower() const;
	/// Gets the item's base accuracy.
	int getBaseAccuracy() const;
	/// Get additional power from unit statistics
	int getPowerBonus(const BattleUnit *unit) const;
	/// Ok, so this isn't a melee type weapon but we're using it for melee... how much damage should it do?
	int getMeleePower() const;
	/// Get additional power for melee attack in range weapon from unit statistics.
	int getMeleeBonus(const BattleUnit *unit) const;
	/// Gets amount of power dropped for range in voxels.
	float getPowerRangeReduction(float range) const;
	/// Gets amount of psi accuracy dropped for range in voxels.
	float getPsiAccuracyRangeReduction(float range) const;
	/// Get multiplier of accuracy form unit statistics
	int getAccuracyMultiplier(const BattleUnit *unit) const;
	/// Get multiplier of melee hit chance form unit statistics
	int getMeleeMultiplier(const BattleUnit *unit) const;
	/// Get multiplier of throwing form unit statistics
	int getThrowMultiplier(const BattleUnit *unit) const;
	/// Get multiplier of close quarters combat from unit statistics
	int getCloseQuartersMultiplier(const BattleUnit *unit) const;

	/// Get configuration of aimed shot action.
	const RuleItemAction *getConfigAimed() const;
	/// Get configuration of autoshot action.
	const RuleItemAction *getConfigAuto() const;
	/// Get configuration of snapshot action.
	const RuleItemAction *getConfigSnap() const;
	/// Get configuration of melee action.
	const RuleItemAction *getConfigMelee() const;

	/// Gets the item's aimed shot accuracy.
	int getAccuracyAimed() const;
	/// Gets the item's autoshot accuracy.
	int getAccuracyAuto() const;
	/// Gets the item's snapshot accuracy.
	int getAccuracySnap() const;
	/// Gets the item's burstshot accuracy.
	int getAccuracyBurst() const;
	/// Gets the item's melee accuracy.
	int getAccuracyMelee() const;
	/// Gets the item's use accuracy.
	int getAccuracyUse() const;
	/// Gets the item's mind control accuracy.
	int getAccuracyMind() const;
	/// Gets the item's panic accuracy.
	int getAccuracyPanic() const;
	/// Gets the item's throw accuracy.
	int getAccuracyThrow() const;
	/// Gets the item's close quarters combat accuracy.
	int getAccuracyCloseQuarters(Mod *mod) const;
	/// Gets the item's shotgun spread accuracy.
	int getAccuracyShotgunSpread() const;

	/// Gets the item's aimed shot cost.
	RuleItemUseCost getCostAimed() const;
	/// Gets the item's autoshot cost.
	RuleItemUseCost getCostAuto() const;
	/// Gets the item's snapshot cost.
	RuleItemUseCost getCostSnap() const;
	/// Gets the item's melee cost.
	RuleItemUseCost getCostMelee() const;
	/// Gets the item's use cost.
	RuleItemUseCost getCostUse() const;
	/// Gets the item's mind control cost.
	RuleItemUseCost getCostMind() const;
	/// Gets the item's panic cost.
	RuleItemUseCost getCostPanic() const;
	/// Gets the item's throw cost.
	RuleItemUseCost getCostThrow() const;
	/// Gets the item's prime cost.
	RuleItemUseCost getCostPrime() const;
	/// Gets the item's unprime cost.
	RuleItemUseCost getCostUnprime() const;

	/// Should we charge a flat rate of costAimed?
	RuleItemUseCost getFlatAimed() const;
	/// Should we charge a flat rate of costAuto?
	RuleItemUseCost getFlatAuto() const;
	/// Should we charge a flat rate of costSnap?
	RuleItemUseCost getFlatSnap() const;
	/// Should we charge a flat rate of costMelee?
	RuleItemUseCost getFlatMelee() const;
	/// Should we charge a flat rate?
	RuleItemUseCost getFlatUse() const;
	/// Should we charge a flat rate of costThrow?
	RuleItemUseCost getFlatThrow() const;
	/// Should we charge a flat rate of costPrime?
	RuleItemUseCost getFlatPrime() const;
	/// Should we charge a flat rate of costPrime?
	RuleItemUseCost getFlatUnprime() const;

	/// Gets the item's load TU cost.
	int getTULoad(int slot) const;
	/// Gets the item's unload TU cost.
	int getTUUnload(int slot) const;
	/// Gets list of compatible ammo.
	const std::vector<std::string> *getPrimaryCompatibleAmmo() const;
	/// Get slot position for ammo type.
	int getSlotForAmmo(const std::string &type) const;
	/// Get slot position for ammo type.
	const std::vector<std::string> *getCompatibleAmmoForSlot(int slot) const;

	/// Gets the item's damage type.
	const RuleDamageType *getDamageType() const;
	/// Gets the item's melee damage type for range weapons.
	const RuleDamageType *getMeleeType() const;
	/// Gets the item's type.
	BattleType getBattleType() const;
	/// Gets the item's fuse type.
	BattleFuseType getFuseTimerType() const;
	/// Gets the item's default fuse value.
	int getFuseTimerDefault() const;
	/// Is this item (e.g. a mine) hidden on the minimap?
	bool isHiddenOnMinimap() const;
	/// Gets the item's inventory width.
	int getInventoryWidth() const;
	/// Gets the item's inventory height.
	int getInventoryHeight() const;
	/// Gets the ammo amount.
	int getClipSize() const;
	/// Gets the battlescape ammo amount.
	int getBattleClipSize() const;
	/// Gets the chance of special effect like zombify or corpse explosion or mine triggering.
	int getSpecialChance() const;
	/// Draws the item's hand sprite onto a surface.
	void drawHandSprite(SurfaceSet *texture, Surface *surface, int maxW = 0, int maxH = 0, bool utilitySlot = false) const;
	/// item's hand spite x offset
	int getHandSpriteOffX() const;
	/// item's hand spite y offset
	int getHandSpriteOffY() const;
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
	/// Gets the medikit morale recovered per shot.
	int getMoraleRecovery() const;
	/// Gets the medikit morale recovered based on missing health.
	float getPainKillerRecovery() const;
	/// Gets the medikit ability to self heal.
	bool getAllowSelfHeal() const;
	/// Is this (medikit-type & items with prime) item consumable?
	bool isConsumable() const;
	/// Does this item extinguish fire?
	bool isFireExtinguisher() const;
	/// Is this item explode in hands?
	bool isExplodingInHands() const;
	/// Gets the medikit use type.
	BattleMediKitType getMediKitType() const;
	/// Gets the max explosion radius.
	int getExplosionRadius(const BattleUnit *unit) const;
	/// Gets the recovery points score
	int getRecoveryPoints() const;
	/// Gets the item's armor.
	int getArmor() const;
	/// Gets the item's recoverability.
	bool isRecoverable() const;
	/// Checks if the item can be equipped in base defense mission.
	bool canBeEquippedBeforeBaseDefense() const;
	/// Gets the item's turret type.
	int getTurretType() const;
	/// Gets first turn when AI can use item.
	int getAIUseDelay(const Mod *mod = 0) const;
	/// Gets how many melee hits AI should do.
	int getAIMeleeHitCount() const;
	/// Checks if this a live alien.
	bool isAlien() const;
	/// Returns to which type of prison does the live alien belong.
	int getPrisonType() const;

	/// Should this weapon arc?
	bool getArcingShot() const;
	/// Which experience training mode to use for this weapon?
	ExperienceTrainingMode getExperienceTrainingMode() const;
	/// How much do aliens want this thing?
	int getAttraction() const;
	/// Get the list weight for this item.
	int getListOrder() const;
	/// How fast does a projectile fired from this weapon travel?
	int getBulletSpeed() const;
	/// How fast does the explosion animation play?
	int getExplosionSpeed() const;
	/// Get name of psi attack for action menu.
	const std::string &getPsiAttackName() const { return _psiAttackName; }
	/// Get name of prime action for action menu.
	const std::string &getPrimeActionName() const { return _primeActionName; }
	/// Get message for prime action.
	const std::string &getPrimeActionMessage() const { return _primeActionMessage; }
	/// Get name of unprime action for action menu.
	const std::string &getUnprimeActionName() const { return _unprimeActionName; }
	/// Get message for unprime action.
	const std::string &getUnprimeActionMessage() const { return _unprimeActionMessage; }
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
	/// Get the shotgun behavior type.
	int getShotgunBehaviorType() const;
	/// Get the spread of shotgun projectiles.
	int getShotgunSpread() const;
	/// Get the shotgun choke value.
	int getShotgunChoke() const;
	/// Gets the weapon's zombie unit.
	const std::string &getZombieUnit() const;
	/// Check if LOS is required to use this item (only applies to psionic type items)
	bool isLOSRequired() const;
	/// Is this item restricted to use underwater?
	bool isWaterOnly() const;
	/// Is this item restricted to land use?
	bool isLandOnly() const;
	/// Is this item require unit with psi skill to use it?
	bool isPsiRequired() const;
	/// Get the associated special type of this item.
	int getSpecialType() const;
	/// Get the color offset to use for the vapor trail.
	int getVaporColor() const;
	/// Gets the vapor cloud density.
	int getVaporDensity() const;
	/// Gets the vapor cloud probability.
	int getVaporProbability() const;
	/// Gets the index of the sprite in the CustomItemPreview sprite set
	int getCustomItemPreviewIndex() const;
	/// Gets the kneel bonus.
	int getKneelBonus(Mod *mod) const;
	/// Gets the one-handed penalty.
	int getOneHandedPenalty(Mod *mod) const;
	/// Get the reactions modifier for this weapon.
	int getReactionsModifier() const;
	/// Get the delay between auto fire shots.
	int getAutoDelay() const;
	/// Get the drop-off rate per tile for explosions from this ammo item.
	int getExplosionDropoff() const;
	/// Gets the monthly salary.
	int getMonthlySalary() const;
	/// Gets the monthly maintenance.
	int getMonthlyMaintenance() const;

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

	/// Gets script.
	template<typename Script>
	const typename Script::Container &getScript() const { return _battleItemScripts.get<Script>(); }
};

}
