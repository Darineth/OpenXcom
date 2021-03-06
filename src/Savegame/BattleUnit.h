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
#include <vector>
#include <string>
#include <unordered_set>
#include <yaml-cpp/yaml.h>
#include "../Battlescape/Position.h"
#include "../Battlescape/BattlescapeGame.h"
#include "../Mod/RuleItem.h"
#include "../Mod/Armor.h"
#include "../Mod/Unit.h"
#include "../Mod/MapData.h"
#include "Soldier.h"
#include "BattleItem.h"
#include "../Mod/RuleEffect.h"

namespace OpenXcom
{

class Tile;
class BattleItem;
class Unit;
class BattleAIState;
class SavedBattleGame;
class Node;
class Surface;
class RuleInventory;
class Soldier;
class SavedGame;
class Language;
class AIModule;
template<typename, typename...> class ScriptContainer;
template<typename, typename...> class ScriptParser;
class ScriptWorkerBlit;
struct BattleUnitStatistics;
struct StatAdjustment;
class SavedBattleGame;
class TileEngine;
class CombatLog;
class BattleEffect;

enum UnitStatus { STATUS_STANDING, STATUS_WALKING, STATUS_FLYING, STATUS_TURNING, STATUS_AIMING, STATUS_COLLAPSING, STATUS_DEAD, STATUS_UNCONSCIOUS, STATUS_PANICKING, STATUS_BERSERK, STATUS_IGNORE_ME, STATUS_BLEEDOUT };
enum UnitFaction { FACTION_PLAYER, FACTION_HOSTILE, FACTION_NEUTRAL };
enum UnitBodyPart { BODYPART_HEAD, BODYPART_TORSO, BODYPART_RIGHTARM, BODYPART_LEFTARM, BODYPART_RIGHTLEG, BODYPART_LEFTLEG, BODYPART_MAX };
enum UnitBodyPartEx { BODYPART_LEGS = BODYPART_MAX, BODYPART_COLLAPSING, BODYPART_ITEM_RIGHTHAND, BODYPART_ITEM_LEFTHAND, BODYPART_ITEM_FLOOR, BODYPART_ITEM_INVENTORY, BODYPART_LARGE_TORSO, BODYPART_LARGE_PROPULSION = BODYPART_LARGE_TORSO + 4, BODYPART_LARGE_TURRET = BODYPART_LARGE_PROPULSION + 4 };

/**
 * Placeholder class for future functionality.
 */
class BattleUnitVisibility
{
public:

	/// Name of class used in script.
	static constexpr const char *ScriptName = "BattleUnitVisibility";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);
};

/**
 * Represents a moving unit in the battlescape, player controlled or AI controlled
 * it holds info about it's position, items carrying, stats, etc
 */
class BattleUnit
{
private:
	static const int SPEC_WEAPON_MAX = 3;

	UnitFaction _faction, _originalFaction;
	UnitFaction _killedBy;
	int _id;
	Position _pos;
	Tile *_tile;
	Position _lastPos;
	int _direction, _toDirection;
	int _directionTurret, _toDirectionTurret;
	int _verticalDirection;
	Position _destination;
	UnitStatus _status;
	bool _wantsToSurrender;
	int _walkPhase, _fallPhase;
	std::vector<BattleUnit *> _visibleUnits, _unitsSpottedThisTurn;
	std::vector<BattleEffect*> _activeEffects;
	std::vector<Tile *> _visibleTiles;
	std::unordered_set<Tile *> _visibleTilesLookup;
	bool _hadNightVision;
	int _tu, _energy, _health, _morale, _stunlevel;
	bool _kneeled, _floating, _dontReselect;
	int _currentArmor[SIDE_MAX], _maxArmor[SIDE_MAX];
	int _fatalWounds[BODYPART_MAX];
	int _fire;
	std::vector<BattleItem*> _inventory;
	BattleItem* _specWeapon[SPEC_WEAPON_MAX];
	AIModule *_currentAIState;
	bool _visible;
	int _expBravery, _expReactions, _expFiring, _expThrowing, _expPsiSkill, _expPsiStrength, _expMelee;
	int _battleExperience;
	int improveStat(int exp) const;
	int _turnsAwake;
	int _motionPoints;
	int _kills;
	int _faceDirection; // used only during strafeing moves
	bool _hitByFire, _hitByAnything;
	int _fireMaxHit;
	int _smokeMaxHit;
	int _moraleRestored;
	int _coverReserve;
	BattleUnit *_charging;
	int _turnsSinceSpotted;
	std::string _spawnUnit;
	std::string _activeHand;
	BattleUnitStatistics* _statistics;
	int _murdererId;	// used to credit the murderer with the kills that this unit got by blowing up on death
	int _mindControllerID;	// used to credit the mind controller with the kills of the mind controllee
	UnitSide _fatalShotSide;
	UnitBodyPart _fatalShotBodyPart;
	std::string _murdererWeapon, _murdererWeaponAmmo;
	bool _overwatch;
	Position _overwatchTarget;
	std::string _overwatchWeaponSlot;
	int _overwatchShotsAvailable;
	bool _justKilled;
	bool _bleedingOut;
	std::string _inventoryLayout;
	bool _inCombat;
	BattleActionMove _movementAction;

	// static data
	std::string _type;
	std::string _rank;
	std::string _race;
	std::wstring _name;
	UnitStats _baseStats, _stats;
	int _standHeight, _kneelHeight, _floatHeight;
	std::vector<int> _deathSound;
	int _value, _aggroSound, _moveSound;
	int _intelligence, _aggression;
	int _maxViewDistanceAtDark, _maxViewDistanceAtDay;
	SpecialAbility _specab;
	Armor *_armor;
	SoldierGender _gender;
	Soldier *_geoscapeSoldier;
	std::vector<int> _loftempsSet;
	Unit *_unitRules;
	int _rankInt;
	int _turretType;
	int _breathFrame;
	bool _breathing;
	int _depth;
	bool _hidingForTurn, _floorAbove, _respawn, _alreadyRespawned;
	bool _isLeeroyJenkins;	// always charges enemy, never retreats.
	MovementType _movementType;
	std::vector<std::pair<Uint8, Uint8> > _recolor;
	int _armorColor;
	ScriptValues<BattleUnit> _scriptValues;

	/// Helper function initing recolor vector.
	void setupSoldierRecolor();
	void setRecolor(int basicLook, int utileLook, int rankLook, int baseArmorColor);
	/// Helper function preparing Time Units recovery at beginning of turn.
	void prepareTimeUnits(int tu);
	/// Helper function preparing Energy recovery at beginning of turn.
	void prepareEnergy(int energy);
	/// Helper function preparing Health recovery at beginning of turn.
	void prepareHealth(int helath);
	/// Helper function preparing Stun recovery at beginning of turn.
	void prepareStun(int strun);
	/// Helper function preparing Morale recovery at beginning of turn.
	void prepareMorale(int morale);

	BattleActionType _ongoingAction;
	BattleUnit *_controlling;
	BattleUnit *_controller;

	int _loadedControllingId;
	bool _isVehicle;

	std::string _weaponSlot1, _weaponSlot2, _utilitySlot;

	bool _aboutToFall;

	bool checkStartBleedout();
	void determineWeaponSlots();

	/// Attempts to place an item in an inventory slot.
	bool fitItem(BattleItem *item, RuleInventory *newSlot, int &x, int &y);

public:
	static const int MAX_SOLDIER_ID = 1000000;
	/// Name of class used in script.
	static constexpr const char *ScriptName = "BattleUnit";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);
	/// Init all required data in script using object data.
	static void ScriptFill(ScriptWorkerBlit* w, BattleUnit* unit, int body_part, int anim_frame, int shade, int burn);

	/// Creates a BattleUnit from solder.
	BattleUnit(Soldier *soldier, int depth, int maxViewDistance);
	/// Creates a BattleUnit from unit.
	BattleUnit(Unit *unit, UnitFaction faction, int id, Armor *armor, StatAdjustment *adjustment, int depth, int maxViewDistance);
	/// Updates a BattleUnit from a Soldier (after a change of armor).
	void updateArmorFromSoldier(Soldier *soldier, int depth, int maxViewDistance);
	/// Cleans up the BattleUnit.
	~BattleUnit();
	/// Loads the unit from YAML.
	void load(const YAML::Node &node, const ScriptGlobal *shared);
	/// Does post-load unit initialization.
	void initLoaded(SavedBattleGame *save);
	/// Saves the unit to YAML.
	YAML::Node save(const ScriptGlobal *shared) const;
	/// Does post-equipment battle initialization.
	void initBattle();
	/// Gets the BattleUnit's ID.
	int getId() const;
	/// Sets the unit's position
	void setPosition(Position pos, bool updateLastPos = true);
	/// Gets the unit's position.
	Position getPosition() const;
	/// Gets the unit's position.
	Position getLastPosition() const;
	/// Gets the unit's position of center in vexels.
	Position getPositionVexels() const;
	/// Sets the unit's direction 0-7.
	void setDirection(int direction);
	/// Sets the unit's face direction (only used by strafing moves)
	void setFaceDirection(int direction);
	/// Gets the unit's direction.
	int getDirection() const;
	/// Gets the unit's face direction (only used by strafing moves)
	int getFaceDirection() const;
	/// Gets the unit's turret direction.
	int getTurretDirection() const;
	/// Gets the unit's turret To direction.
	int getTurretToDirection() const;
	/// Gets the unit's vertical direction.
	int getVerticalDirection() const;
	/// Gets the unit's status.
	UnitStatus getStatus() const;
	/// Does the unit want to surrender?
	bool wantsToSurrender() const;
	/// Start the walkingPhase
	void startWalking(int direction, Position destination, Tile *tileBelowMe, bool cache);
	/// Increase the walkingPhase
	void keepWalking(Tile *tileBelowMe, bool cache);
	/// Gets the walking phase for animation and sound
	int getWalkingPhase() const;
	/// Gets the walking phase for diagonal walking
	int getDiagonalWalkingPhase() const;
	/// Gets the unit's destination when walking
	Position getDestination() const;
	/// Look at a certain point.
	void lookAt(Position point, bool turret = false);
	/// Look at a certain direction.
	void lookAt(int direction, bool force = false);
	/// Turn to the destination direction.
	void turn(bool turret = false);
	/// Abort turning.
	void abortTurn();
	/// Gets the soldier's gender.
	SoldierGender getGender() const;
	/// Gets the unit's faction.
	UnitFaction getFaction() const;
	/// Gets unit sprite recolors values.
	const std::vector<std::pair<Uint8, Uint8> > &getRecolor() const;
	/// Get the unit armor color.
	int getArmorColor() const;
	/// Kneel down.
	void kneel(bool kneeled);
	/// Is kneeled?
	bool isKneeled() const;
	/// Is floating?
	bool isFloating() const;
	/// Aim.
	void aim(bool aiming);
	/// Get direction to a certain point
	int directionTo(Position point) const;
	/// Gets the unit's time units.
	int getTimeUnits() const;
	/// Gets the unit's spendable time units.
	int getAvailableTimeUnits(bool movement = false) const;
	/// Gets the unit's time unit debt.
	int getTimeUnitDebt() const;
	/// Gets the unit's stamina.
	int getEnergy() const;
	/// Gets the unit's health.
	int getHealth() const;
	/// Gets the unit's bravery.
	int getMorale() const;
	/// Get overkill damage to unit.
	int getOverKillDamage() const;
	/// Do damage to the unit.
	int damage(TileEngine* tiles, BattleUnit *source, Position relative, int power, const RuleDamageType *type, SavedBattleGame *save, BattleActionAttack attack, UnitSide sideOverride = SIDE_MAX, UnitBodyPart bodypartOverride = BODYPART_MAX, bool ignoreArmor = false, int *wounds = 0, const std::string &displaySource = "");
	/// Heal stun level of the unit.
	void healStun(int power);
	/// Gets the unit's stun level.
	int getStunlevel() const;
	/// Knocks the unit out instantly.
	void knockOut(BattlescapeGame *battle);
	/// Start falling sequence.
	void startFalling();
	/// Increment the falling sequence.
	void keepFalling();
	/// Get falling sequence.
	int getFallingPhase() const;
	/// The unit is out - either dead or unconscious.
	bool isOut() const;
	/// Get the number of time units a certain action takes.
	RuleItemUseCost getActionTUs(BattleActionType actionType, BattleItem *item) const;
	/// Get the number of time units a certain action takes.
	//RuleItemUseCost getActionTUs(BattleActionType actionType, const RuleItem *item) const;
	/// Spend time units if it can.
	bool spendTimeUnits(int tu, bool allowDebt = false, bool forOverwatch = false);
	/// Checks if the unit could spend the specified time units.
	bool canSpendTimeUnits(int tu, bool allowDebt = false, bool forOverwatch = false);
	/// Spend energy if it can.
	bool spendEnergy(int tu);
	/// Force to spend resources cost without checking.
	void spendCost(const RuleItemUseCost& cost);
	/// Set time units.
	void setTimeUnits(int tu);
	/// Add unit to visible units.
	bool addToVisibleUnits(BattleUnit *unit);
	/// Remove a unit from the list of visible units.
	bool removeFromVisibleUnits(BattleUnit *unit);
	/// Is the given unit among this unit's visible units?
	bool hasVisibleUnit(BattleUnit *unit);
	/// Get the list of visible units.
	std::vector<BattleUnit*> *getVisibleUnits();
	/// Clear visible units.
	void clearVisibleUnits();
	/// Add unit to visible tiles.
	bool addToVisibleTiles(Tile *tile, bool nightVision);
	/// Has this unit marked this tile as within its view?
	bool hasVisibleTile(Tile *tile) const;
	/// Get the list of visible tiles.
	const std::vector<Tile*> *getVisibleTiles();
	/// Clear visible tiles.
	void clearVisibleTiles();
	/// Get if the unit can see the tile.
	/* Necessary?  Replaced by hasVisibleTile? bool canSeeTile(Tile *tile) const;*/
	/// Calculate psi attack accuracy.
	int getPsiAccuracy(BattleActionType actionType, BattleItem *item);
	/// Calculate firing accuracy.
	int getFiringAccuracy(BattleActionType actionType, BattleItem *item, const Mod *mod, bool useShotgun, bool dualFiring);
	/// Calculate accuracy modifier.
	int getAccuracyModifier(BattleItem *item = 0);
	/// Calculate throwing accuracy.
	double getThrowingAccuracy();
	/// Set armor value.
	void setArmor(int armor, UnitSide side);
	/// Get armor value.
	int getArmor(UnitSide side) const;
	/// Get max armor value.
	int getMaxArmor(UnitSide side) const;
	/// Get total number of fatal wounds.
	int getFatalWounds() const;
	/// Get the current reaction score.
	double getReactionScore(bool checkOverwatch) const;
	/// Get the current evasion score.
	double getEvasionScore();
	/// Get the mainhand weapon's reactions modifier.
	double getWeaponReactionsModifier(bool overwatch) const;
	/// Prepare for a new turn.
	void prepareNewTurn(TileEngine *tiles = 0, bool fullProcess = true, bool reset = false);
	/// Calculate change in unit stats.
	void recoverStats(bool tuAndEnergy, bool rest);
	/// Morale change
	int moraleChange(int change);
	/// Calculate value of morale change based on breavy.
	int reduceByBravery(int moraleChange) const;
	/// Calculate power reduction by resistances.
	int reduceByResistance(int power, ItemDamageType resistType) const;
	/// Don't reselect this unit
	void dontReselect();
	/// Reselect this unit
	void allowReselect();
	/// Check whether reselecting this unit is allowed.
	bool reselectAllowed() const;
	/// Set fire.
	void setFire(int fire);
	/// Get fire.
	int getFire() const;

	/// Get the list of items in the inventory.
	std::vector<BattleItem*> *getInventory();
	/// Fit item into inventory slot.
	bool fitItemToInventory(RuleInventory *slot, BattleItem *item);
	/// Add item to unit.
	bool addItem(BattleItem *item, const Mod *mod, bool allowSecondClip = false, bool allowAutoLoadout = false, bool allowUnloadedWeapons = false);

	/// Let AI do their thing.
	void think(BattleAction *action);
	/// Get AI Module.
	AIModule *getAIModule() const;
	/// Set AI Module.
	void setAIModule(AIModule *ai);
	/// Set whether this unit is visible
	void setVisible(bool flag);
	/// Get whether this unit is visible
	bool getVisible() const;
	/// Sets the unit's tile it's standing on
	void setTile(Tile *tile, Tile *tileBelow = 0);
	/// Gets the unit's tile.
	Tile *getTile() const;
	/// Gets the item in the specified slot.
	BattleItem *getItem(RuleInventory *slot, int x = 0, int y = 0) const;
	/// Gets the item in the specified slot.
	BattleItem *getItem(const std::string &slot, int x = 0, int y = 0) const;
	/// Gets the item in the main hand.
	BattleItem *getMainHandWeapon(bool quickest = true) const;
	/// Gets a grenade from the belt, if any.
	BattleItem *getGrenadeFromBelt() const;
	/// Gets the item from right hand.
	BattleItem *findQuickItem(const std::string &item, RuleInventory* destSlot, int *moveCost = 0) const;
	/// Finds the quickest ammo to reload a weapon.
	BattleItem *findQuickAmmo(BattleItem *weapon, int *reloadCost = 0) const;
	/// Reloads righthand weapon if needed.
	bool reloadAmmo();
	/// Check if this unit is in the exit area
	bool isInExitArea(SpecialTileType stt = START_POINT) const;
	/// Gets the unit height taking into account kneeling/standing.
	int getHeight() const;
	/// Gets the unit floating elevation.
	int getFloatHeight() const;
	/// Adds one to the bravery exp counter.
	void addBraveryExp();
	/// Adds one to the reaction exp counter.
	void addReactionExp();
	/// Adds one to the firing exp counter.
	void addFiringExp();
	/// Adds one to the throwing exp counter.
	void addThrowingExp();
	/// Adds one to the psi skill exp counter.
	void addPsiSkillExp();
	/// Adds one to the psi strength exp counter.
	void addPsiStrengthExp();
	/// Adds one to the melee exp counter.
	void addMeleeExp();
	/// Updates the stats of a Geoscape soldier.
	void updateGeoscapeStats(Soldier *soldier) const;
	/// Check if unit eligible for squaddie promotion.
	bool postMissionProcedures(SavedGame *geoscape, UnitStats &statsDiff, int turns, int teamExperience, int &gainedExperience, bool &gainedLevel, int &kills);
	/// Get the sprite index for the minimap
	int getMiniMapSpriteIndex() const;
	/// Set the turret type. -1 is no turret.
	void setTurretType(int turretType);
	/// Get the turret type. -1 is no turret.
	int getTurretType() const;
	/// Get fatal wound amount of a body part
	int getFatalWound(int part) const;
	/// Heal one fatal wound
	bool heal(int part, int woundAmount, int healthAmount);
	/// Give pain killers to this unit
	void painKillers(int moraleAmount, float painKillersStrength);
	/// Give stimulant to this unit
	void stimulant(int energy, int stun);
	/// Get motion points for the motion scanner.
	int getMotionPoints() const;
	/// Gets the unit's armor.
	const Armor *getArmor() const;
	/// Sets the unit's name.
	void setName(const std::wstring &name);
	/// Gets the unit's name.
	std::wstring getName(Language *lang, bool debugAppendId = false) const;
	/// Gets the unit's display name for combat logging.
	std::wstring getLogName(Language *lang) const;
	/// Gets the unit's stats.
	UnitStats *getBaseStats();
	/// Gets the unit's stats.
	const UnitStats *getBaseStats() const;
	/// Get the unit's stand height.
	int getStandHeight() const;
	/// Get the unit's kneel height.
	int getKneelHeight() const;
	/// Get the unit's loft ID.
	int getLoftemps(int entry = 0) const;
	/// Get the unit's value.
	int getValue() const;
	/// Get the unit's death sounds.
	const std::vector<int> &getDeathSounds() const;
	/// Get the unit's move sound.
	int getMoveSound() const;
	/// Get whether the unit is affected by fatal wounds.
	bool isWoundable() const;
	/// Get whether the unit is affected by fear.
	bool isFearable() const;
	/// Get the unit's intelligence.
	int getIntelligence() const;
	/// Get the unit's aggression.
	int getAggression() const;
	/// Helper method.
	int getMaxViewDistance(int baseVisibility, int nerf, int buff) const;
	/// Get maximum view distance at dark.
	int getMaxViewDistanceAtDark(const Armor *otherUnitArmor) const;
	/// Get maximum view distance at day.
	int getMaxViewDistanceAtDay(const Armor *otherUnitArmor) const;
	/// Get the units's special ability.
	int getSpecialAbility() const;
	/// Set the units's respawn flag.
	void setRespawn(bool respawn);
	/// Get the units's respawn flag.
	bool getRespawn() const;
	/// Set the units's alreadyRespawned flag.
	void setAlreadyRespawned(bool alreadyRespawned);
	/// Get the units's alreadyRespawned flag.
	bool getAlreadyRespawned() const;
	/// Get the units's rank string.
	std::string getRankString() const;
	/// Get the geoscape-soldier object.
	Soldier *getGeoscapeSoldier() const;
	/// Add a kill to the counter.
	void addKillCount();
	/// Get unit type.
	std::string getType() const;
	/// Set the hand this unit is using;
	void setActiveHand(const std::string &slot);
	/// Get unit's active hand.
	std::string getActiveHand() const;
	/// Convert's unit to a faction
	void convertToFaction(UnitFaction f);
	/// Set health to 0
	void kill();
	/// Set health to 0 and set status dead
	void instaKill();
	/// Marks the unit dead and triggers related events
	void die(TileEngine *tiles = 0);
	/// Cause backlash to the controlling unit
	void backlashController(TileEngine *tiles = 0, CombatLog *log = 0);
	/// Gets the unit's spawn unit.
	std::string getSpawnUnit() const;
	/// Sets the unit's spawn unit.
	void setSpawnUnit(const std::string &spawnUnit);
	/// Gets the unit's aggro sound.
	int getAggroSound() const;
	/// Sets the unit's energy level.
	void setEnergy(int energy);
	/// Get the faction that killed this unit.
	UnitFaction killedBy() const;
	/// Set the faction that killed this unit.
	void killedBy(UnitFaction f);
	/// Set the units we are charging towards.
	void setCharging(BattleUnit *chargeTarget);
	/// Get the units we are charging towards.
	BattleUnit *getCharging();
	/// Get the carried weight in strength units.
	int getCarriedWeight(BattleItem *draggingItem = 0) const;
	/// Set how many turns this unit will be exposed for.
	void setTurnsSinceSpotted(int turns);
	/// Set how many turns this unit will be exposed for.
	int getTurnsSinceSpotted() const;
	/// Get this unit's original faction
	UnitFaction getOriginalFaction() const;
	/// Calculates the effective range for a shot with the given soldier and weapon accuracy.
	double calculateEffectiveRange(double soldierAcc, double weaponAcc);
	/// Calculates the effective range for a shot from the given action.
	int calculateEffectiveRangeForAction(BattleActionType actionType, BattleItem *item, bool dualFiring);
	/// Calculates the chance to hit for a shot with the given effective range and distance to target.
	double calculateChanceToHit(double effectiveRange, double distance);

	/// Get alien/HWP unit.
	Unit *getUnitRules() const { return _unitRules; }

	Position lastCover;
	/// get the vector of units we've seen this turn.
	std::vector<BattleUnit *> &getUnitsSpottedThisTurn();
	/// set the rank integer
	void setRankInt(int rank);
	/// get the rank integer
	int getRankInt() const;
	/// derive a rank integer based on rank string (for xcom soldiers ONLY)
	void deriveRank();
	/// this function checks if a tile is visible, using maths.
	bool checkViewSector(Position pos, bool useTurretDirection = false) const;
	/// adjust this unit's stats according to difficulty.
	void adjustStats(const StatAdjustment &adjustment);
	/// did this unit already take fire damage this turn? (used to avoid damaging large units multiple times.)
	bool tookFireDamage() const;
	/// switch the state of the fire damage tracker.
	void toggleFireDamage();
	void setCoverReserve(int reserve);
	int getCoverReserve() const;
	/// Is this unit selectable?
	bool isSelectable(UnitFaction faction, bool checkReselect, bool checkInventory) const;
	/// Does this unit have an inventory?
	bool hasInventory() const;
	/// Is this unit breathing and if so what frame?
	int getBreathFrame() const;
	/// Start breathing and/or update the breathing frame.
	void breathe();
	/// Set the flag for "floor above me" meaning stop rendering bubbles.
	void setFloorAbove(bool floor);
	/// Get the flag for "floor above me".
	bool getFloorAbove() const;
	/// Get any utility weapon we may be carrying, or a built in one.
	BattleItem *getUtilityWeapon(BattleType type);
	/// Set fire damage form environment.
	void setEnviFire(int damage);
	/// Set smoke damage form environment.
	void setEnviSmoke(int damage);
	/// Calculate smoke and fire damage form environment.
	void calculateEnviDamage(Mod *mod, SavedBattleGame *save);
	/// Use this function to check the unit's movement type.
	MovementType getMovementType() const;
	/// Create special weapon for unit.
	void setSpecialWeapon(SavedBattleGame *save);
	/// Get special weapon.
	BattleItem *getSpecialWeapon(BattleType type) const;
	/// Checks if this unit is in hiding for a turn.
	bool isHiding() const { return _hidingForTurn; };
	/// Sets this unit is in hiding for a turn (or not).
	void setHiding(bool hiding) { _hidingForTurn = hiding; };
	/// Puts the unit in the corner to think about what he's done.
	void goToTimeOut();
	/// Recovers the unit's time units and energy.
	void recoverTimeUnits();
	/// Get the unit's mission statistics.
	BattleUnitStatistics* getStatistics();
	/// Set the unit murderer's id.
	void setMurdererId(int id);
	/// Get the unit murderer's id.
	int getMurdererId() const;
	/// Set information on the unit's fatal shot.
	void setFatalShotInfo(UnitSide side, UnitBodyPart bodypart);
	/// Get information on the unit's fatal shot's side.
	UnitSide getFatalShotSide() const;
	/// Get information on the unit's fatal shot's body part.
	UnitBodyPart getFatalShotBodyPart() const;
	/// Get the unit murderer's weapon.
	std::string getMurdererWeapon() const;
	/// Set the unit murderer's weapon.
	void setMurdererWeapon(const std::string& weapon);
	/// Get the unit murderer's weapon's ammo.
	std::string getMurdererWeaponAmmo() const;
	/// Set the unit murderer's weapon's ammo.
	void setMurdererWeaponAmmo(const std::string& weaponAmmo);
	/// Set the unit mind controller's id.
	void setMindControllerId(int id);
	/// Get the unit mind controller's id.
	int getMindControllerId() const;
	/// Was this unit just hit?
	bool getHitState();
	/// reset the unit hit state.
	void resetHitState();
	/// Get the unit leeroyJenkins flag
	bool isLeeroyJenkins() const { return _isLeeroyJenkins; };

	bool onOverwatch() const;
	void activateOverwatch(SavedBattleGame *save, BattleItem *weapon, const Position& target);
	void clearOverwatch();
	bool tryFireOverwatch();
	void cancelOverwatchShot();
	const Position& getOverwatchTarget() const;
	BattleItem *getOverwatchWeapon() const;
	BattleActionType getOverwatchShotAction(const RuleItem *weapon = 0) const;

	void setJustKilled(bool justKilled);
	bool getJustKilled() const;
	bool getCanBleedOut() const;
	bool getBleedingOut() const;
	int getDeathHealth() const;

	bool checkSquadSight(SavedBattleGame *save, BattleUnit* target, bool visibleOnly) const;

	BattleActionType getOngoingAction() const;
	void setOngoingAction(BattleActionType type);
	void clearOngoingAction(BattleActionType typeCheck = BA_NONE);

	BattleUnit* getControlledBy() const;
	void setControlledBy(BattleUnit *controller);
	void clearControlledBy();
	BattleUnit* getControlling() const;
	void setControlling(BattleUnit *controlling);
	void clearControlling();

	/// Returns if the unit is a vehicle.
	bool isVehicle() const;
	bool findTurretType();

	void updateStats(BattleItem *draggingItem = 0);
	UnitStats calculateStats(BattleItem *draggingItem = 0) const;
	/// Gets the inventory layout for the unit.
	const std::string &getInventoryLayout() const;

	void calculateArmor(int *armor, BattleItem *draggingItem = 0);
	void updateArmor(BattleItem *draggingItem = 0);

	bool canDualFire() const;
	bool canDualFire(BattleItem *&weapon1, BattleItem *&weapon2) const;
	bool canDualFire(BattleItem *&weapon1, BattleActionType &action1, BattleItem *&weapon2, BattleActionType &action2) const;
	bool canDualFire(BattleItem *&weapon1, BattleActionType &action1, int &tu1, BattleItem *&weapon2, BattleActionType &action2, int &tu2) const;

	double getDualFireAccuracy() const;

	const std::string &getWeaponSlot1() const;
	BattleItem *getWeapon1() const;
	const std::string &getWeaponSlot2() const;
	BattleItem *getWeapon2() const;
	const std::string &getUtilitySlot() const;
	BattleItem *getUtilityItem() const;

	/// Moves an item to a specified slot.
	bool moveItem(BattleItem *item, RuleInventory *slot, int x, int y, std::string &warning, bool checkTu = true, bool testing = false, int *returnCost = 0);

	/// Checks for item overlap.
	bool overlapItems(BattleItem *item, RuleInventory *slot, int x = 0, int y = 0);

	bool unloadWeapon(BattleItem *weapon, std::string &warning, bool checkTu = true, bool testing = false, int *returnCost = 0);

	bool loadWeapon(BattleItem *weapon, BattleItem *ammo, std::string &warning, bool checkTu = true, bool testing = false, int *returnCost = 0, bool ignoreLoaded = false);

	RuleItemUseCost getReloadCost(BattleItem *weapon) const;

	bool reloadWeapon(BattleItem *weapon, std::string &warning, bool checkTu = true, bool testing = false, int *returnCost = 0, BattleItem **reloadedAmmo = 0);

	int getLoadCost(BattleItem *weapon, BattleItem *ammo) const;

	void setMovementAction(BattleActionMove action);
	BattleActionMove getMovementAction() const;

	std::vector<BattleEffect*> &getActiveEffects();

	bool cancelEffects(EffectTrigger trigger);
	const EffectComponent* getEffectComponent(EffectComponentType type) const;

	void addBattleExperience(const std::string &action);
	void addBattleExperience(int battleExperience);
	int getBattleExperience() const;

	int getTurnsAwake() const;
	void setAboutToFall(bool aboutToFall);
	bool getAboutToFall() const;
};

}
