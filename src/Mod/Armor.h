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
#include "../Engine/Yaml.h"
#include "MapData.h"
#include "Unit.h"
#include "RuleStatBonus.h"
#include "RuleDamageType.h"
#include "ModScript.h"

namespace OpenXcom
{

enum ForcedTorso : Uint8;
enum UnitSide : Uint8;
enum AIAttackWeight : int;

class BattleUnit;
class RuleItem;
class RuleInventoryLayout;
class RuleResearch;
class RuleSoldier;
class RulesoldierBonus;
class RuleCommendations;

/**
 * Move cost multipler.
 */
struct ArmorMoveCost
{
	int TimePercent;
	int EnergyPercent;

	ArmorMoveCost& operator*=(ArmorMoveCost c)
	{
		TimePercent *= c.TimePercent;
		EnergyPercent *= c.EnergyPercent;
		return *this;
	}

	bool operator==(ArmorMoveCost c) const
	{
		return TimePercent == c.TimePercent && EnergyPercent == c.EnergyPercent;
	}

	bool operator!=(ArmorMoveCost c) const
	{
		return !(*this == c);
	}

	void load(const YAML::YamlNodeReader& reader)
	{
		if (!reader)
			return;
		TimePercent = reader[0].readVal<int>();
		EnergyPercent = reader[1].readVal<int>();
	}
	void save(YAML::YamlNodeWriter writer, const char* name) const
	{
		auto pairWriter = writer[writer.saveString(name)];
		pairWriter.setAsSeq();
		pairWriter.write(TimePercent);
		pairWriter.write(EnergyPercent);
	}
};

/**
 * DX: mod-wide default armor move costs. An armor that omits a given move-cost key falls back to
 * these (which themselves default to the stock values), so a mod can retune movement game-wide - e.g.
 * make sneaking slow - without editing every armor. Loaded from the top-level `moveCostDefaults:`
 * node; the field/key names match an armor's own `moveCost:` block.
 */
struct ArmorMoveCostDefaults
{
	ArmorMoveCost base = { 100, 100 };
	ArmorMoveCost baseFly = { 100, 100 };
	ArmorMoveCost baseClimb = { 100, 100 };
	ArmorMoveCost baseNormal = { 100, 100 };
	ArmorMoveCost walk = { 100, 50 };
	ArmorMoveCost run = { 75, 75 };
	ArmorMoveCost strafe = { 100, 50 };
	ArmorMoveCost sneak = { 100, 50 };
	ArmorMoveCost flyWalk = { 100, 50 };
	ArmorMoveCost flyRun = { 75, 75 };
	ArmorMoveCost flyStrafe = { 100, 50 };
	ArmorMoveCost flyUp = { 100, 0 };
	ArmorMoveCost flyDown = { 100, 0 };
	ArmorMoveCost climbUp = { 100, 50 };
	ArmorMoveCost climbDown = { 100, 50 };
	ArmorMoveCost gravLift = { 100, 0 };

	void load(const YAML::YamlNodeReader& reader)
	{
		base.load(reader["basePercent"]);
		baseFly.load(reader["baseFlyPercent"]);
		baseClimb.load(reader["baseClimbPercent"]);
		baseNormal.load(reader["baseNormalPercent"]);
		walk.load(reader["walkPercent"]);
		run.load(reader["runPercent"]);
		strafe.load(reader["strafePercent"]);
		sneak.load(reader["sneakPercent"]);
		flyWalk.load(reader["flyWalkPercent"]);
		flyRun.load(reader["flyRunPercent"]);
		flyStrafe.load(reader["flyStrafePercent"]);
		flyUp.load(reader["flyUpPercent"]);
		flyDown.load(reader["flyDownPercent"]);
		climbUp.load(reader["climbUpPercent"]);
		climbDown.load(reader["climbDownPercent"]);
		gravLift.load(reader["gravLiftPercent"]);
	}
};

/**
 * DX: how a movement mode reshapes the moving unit's defensive reaction/evasion score. The base
 * reaction score is `reactions * (currentTU / maxTU)`; this config tweaks its two parts independently:
 *  - statPercent: percent of the reactions stat used (100 = full).
 *  - tuPenaltyPercent: how strongly low TU penalises the score. The effective TU factor is
 *    `1 - (1 - currentTU/maxTU) * tuPenaltyPercent/100`, so 100 = the vanilla TU/maxTU term, 0 = no
 *    penalty (full score regardless of spent TU).
 * e.g. Sprint = {statPercent:50, tuPenaltyPercent:50} (half reactions, only half the TU penalty);
 *      Sneak  = {statPercent:100, tuPenaltyPercent:0}  (full reactions at all times).
 */
struct EvasionModeConfig
{
	int statPercent = 100;
	int tuPenaltyPercent = 100;
	// DX momentum evasion: when evasionPercentPerTile > 0, this mode's evasion is a momentum ramp -
	// each tile moved in the current run adds this percent of the unit's evasion score
	// (min(tiles, maxMomentumTiles) * evasionPercentPerTile% of the base score), instead of the
	// statPercent/tuPenaltyPercent formula. 0 = disabled.
	int evasionPercentPerTile = 0;
	int maxMomentumTiles = 0; // cap on counted momentum tiles (0 = uncapped)

	void load(const YAML::YamlNodeReader& reader)
	{
		if (!reader)
			return;
		reader.tryRead("statPercent", statPercent);
		reader.tryRead("tuPenaltyPercent", tuPenaltyPercent);
		reader.tryRead("evasionPercentPerTile", evasionPercentPerTile);
		reader.tryRead("maxMomentumTiles", maxMomentumTiles);
	}
};

/**
 * DX: mod-wide defaults for the sprint/sneak evasion configs. Loaded from the top-level
 * `evasionDefaults:` node (`sprint:`/`sneak:` sub-maps); an armor's own `evasionSprint`/`evasionSneak`
 * override per field. Default {100,100} for both ⇒ evasion unchanged from the plain reaction score.
 */
struct ArmorEvasionDefaults
{
	EvasionModeConfig sprint;
	EvasionModeConfig sneak;

	void load(const YAML::YamlNodeReader& reader)
	{
		sprint.load(reader["sprint"]);
		sneak.load(reader["sneak"]);
	}
};

/**
 * DX: mod-wide sneak-mode parameters. Loaded from the top-level `sneakDefaults:` node.
 * `maxLight` is the "no creeping while glowing" gate: a unit emitting more light than this
 * (armor personal light with the toggle on, carried glow items, being on fire) cannot enter
 * sneak mode. Default 5 - small glows tolerated; the stock personal light (15) or a lit
 * flare/torch blocks sneaking until the light is toggled off / dropped.
 */
struct ArmorSneakDefaults
{
	int maxLight = 5;

	void load(const YAML::YamlNodeReader& reader)
	{
		if (reader)
		{
			reader.tryRead("maxLight", maxLight);
		}
	}
};

/**
 * DX: per-armor dynamic-cloak config, loaded from the armor's `cloak:` node.
 *
 * Without the node an armor's `camouflageAtDay`/`camouflageAtDark` behave exactly as in OXCE: always
 * on. With `dynamic: true` those same values become a *cloak* that is up at the start of the unit's
 * turn and drops to no camouflage as soon as the unit does something listed in `breaksOn` - it
 * recovers on the unit's next turn. Sneaking and turning never break a cloak (that is the point:
 * creep while cloaked, expose yourself when you act).
 */
struct ArmorCloak
{
	bool dynamic = false;
	/// The set of unit actions that break the cloak, as a mask of UnitAction bits (the `breaksOn:` list).
	/// Sneaking and turning are out of the default set - creeping and looking around are how you move
	/// while cloaked.
	int breaksOn = UA_MOVE_WALK | UA_MOVE_RUN | UA_ATTACK | UA_USE_ITEM;

	/// Does this cloak break when the unit does this?
	bool breaksOnAction(UnitAction action) const { return (breaksOn & action) != 0; }

	void load(const YAML::YamlNodeReader& reader)
	{
		if (!reader)
		{
			return;
		}
		reader.tryRead("dynamic", dynamic);
		if (const auto& breaks = reader["breaksOn"])
		{
			std::vector<std::string> actions;
			breaks.tryReadVal(actions);
			breaksOn = 0;
			for (const auto& action : actions)
			{
				breaksOn |= unitActionFromName(action);
			}
		}
	}
};

/**
 * DX: mod-wide defaults for the bleedout mechanic. Loaded from the top-level `bleedoutDefaults:` node.
 * `deathHealthPercent` sets the death threshold as a percent of max health below zero (50 ⇒ a unit
 * bleeding out dies at −maxHealth/2); `bufferWounds` is the number of fatal torso wounds added when a
 * unit first enters bleedout (the drain that keeps it sinking toward the threshold if untreated).
 */
struct ArmorBleedoutDefaults
{
	int deathHealthPercent = 50;
	int bufferWounds = 5;
	// DX: when true, a unit that ever entered bleedout (dropped into negative health) is out for the rest
	// of the mission - it can be healed to survive but will not revive/rejoin the fight (recovered at
	// debriefing). false = a bled-out unit heals and revives normally.
	bool lockoutForMission = true;

	void load(const YAML::YamlNodeReader& reader)
	{
		reader.tryRead("deathHealthPercent", deathHealthPercent);
		reader.tryRead("bufferWounds", bufferWounds);
		reader.tryRead("lockoutForMission", lockoutForMission);
	}
};


/**
 * Represents a specific type of armor.
 * Not only soldier armor, but also alien armor - some alien races wear
 * Soldier Armor, Leader Armor or Commander Armor depending on their rank.
 */
class Armor
{
public:

	/// Name of class used in script.
	static constexpr const char *ScriptName = "RuleArmor";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);

	static const std::string NONE;
private:
	std::string _ufopediaType;
	std::string _type, _spriteSheet, _spriteInv, _corpseGeoName, _storeItemName, _selfDestructItemName, _specWeaponName;
	std::string _inventoryLayoutName;
	std::string _requiresName;
	std::string _requiresAwardName;
	std::string _requiresBonusName;
	std::string _layersDefaultPrefix;
	std::map<int, std::string> _layersSpecificPrefix;
	std::map<std::string, std::vector<std::string> > _layersDefinition;
	std::vector<std::string> _corpseBattleNames;
	std::vector<std::string> _builtInWeaponsNames;
	std::vector<std::string> _unitsNames;

	std::vector<const RuleItem*> _corpseBattle;
	std::vector<const RuleItem*> _builtInWeapons;
	std::vector<const RuleSoldier*> _units;
	std::vector<int> _ranks;
	const RuleResearch* _requires = nullptr;
	const RuleCommendations* _requiresAward = nullptr;
	const RuleSoldierBonus* _requiresBonus = nullptr;
	const RuleItem* _corpseGeo = nullptr;
	const RuleItem* _storeItem = nullptr;
	const RuleItem* _selfDestructItem = nullptr;
	const RuleItem* _specWeapon = nullptr;
	const RuleInventoryLayout* _inventoryLayout = nullptr;

	bool _infiniteSupply;
	int _frontArmor, _sideArmor, _leftArmorDiff, _rearArmor, _underArmor, _drawingRoutine;
	bool _drawBubbles;
	MovementType _movementType;
	SpecialAbility _specab;

	NullableValue<AIAttackWeight> _aiTargetWeightAsHostile = { };
	NullableValue<AIAttackWeight> _aiTargetWeightAsHostileCivilians  = { };
	NullableValue<AIAttackWeight> _aiTargetWeightAsFriendly  = { };
	NullableValue<AIAttackWeight> _aiTargetWeightAsNeutral = { };

	bool _turnBeforeFirstStep;
	int _turnCost;

	// DX: { -1, -1 } is the "not set by this armor" sentinel; afterLoad() resolves any unset field
	// from Armor::moveCostDefaults (the mod-wide defaults, which default to the stock values below).
	ArmorMoveCost _moveCostBase = { -1, -1 };
	ArmorMoveCost _moveCostBaseFly = { -1, -1 };
	ArmorMoveCost _moveCostBaseClimb = { -1, -1 };
	ArmorMoveCost _moveCostBaseNormal = { -1, -1 };

	ArmorMoveCost _moveCostWalk = { -1, -1 };
	ArmorMoveCost _moveCostRun = { -1, -1 };
	ArmorMoveCost _moveCostStrafe = { -1, -1 };
	ArmorMoveCost _moveCostSneak = { -1, -1 };
	ArmorMoveCost _moveCostFlyWalk = { -1, -1 };
	ArmorMoveCost _moveCostFlyRun = { -1, -1 };
	ArmorMoveCost _moveCostFlyStrafe = { -1, -1 };
	ArmorMoveCost _moveCostFlyUp = { -1, -1 };
	ArmorMoveCost _moveCostFlyDown = { -1, -1 };
	ArmorMoveCost _moveCostClimbUp = { -1, -1 };
	ArmorMoveCost _moveCostClimbDown = { -1, -1 };
	ArmorMoveCost _moveCostGravLift = { -1, -1 };

	int _moveSound;
	std::vector<int> _deathSoundMale, _deathSoundFemale;
	std::vector<int> _selectUnitSoundMale, _selectUnitSoundFemale;
	std::vector<int> _startMovingSoundMale, _startMovingSoundFemale;
	std::vector<int> _selectWeaponSoundMale, _selectWeaponSoundFemale;
	std::vector<int> _annoyedSoundMale, _annoyedSoundFemale;

	int _size, _weight, _visibilityAtDark, _visibilityAtDay;

	int _personalLightFriend = 15;
	int _personalLightHostile = 0;
	int _personalLightNeutral = 0;

	int _camouflageAtDay, _camouflageAtDark, _antiCamouflageAtDay, _antiCamouflageAtDark;
	ArmorCloak _cloak; // DX
	int _visibilityThroughSmoke, _visibilityThroughFire;
	int _psiVision, _psiCamouflage;
	float _damageModifier[DAMAGE_TYPES];
	std::vector<int> _loftempsSet;
	UnitStats _stats;
	UnitStats _statModifiers;
	bool _hasStatModifiers;
	int _deathFrames;
	bool _constantAnimation, _hasInventory;
	/// DX: derive the unit's turret sprite from the weapon it has mounted (modular vehicle chassis).
	bool _turretFromWeapon = false;
	/// DX: skip the carried-weight-vs-strength TU reduction at the start of each turn.
	bool _ignoresEncumbrance = false;
	ForcedTorso _forcedTorso;
	int _faceColorGroup, _hairColorGroup, _utileColorGroup, _rankColorGroup;
	std::vector<int> _faceColor, _hairColor, _utileColor, _rankColor;
	Sint8  _fearImmune, _bleedImmune, _painImmune, _zombiImmune;
	Sint8 _ignoresMeleeThreat, _createsMeleeThreat;
	float _overKill, _meleeDodgeBackPenalty;
	int _evasion = 100; // DX: reaction-fire evasion (percent of the defensive reaction/evasion score)
	// DX: bleedout eligibility for units in this armor. -1 = auto (legacy rule: an original-player,
	// non-vehicle geoscape soldier); 0 = never; 1 = always (any unit wearing this armor).
	int _allowBleedOut = -1;
	// DX: how sprint/sneak reshape the mover's evasion. -1 in a field = "use the mod-wide default"
	// (resolved per field in afterLoad).
	// -1 = "unset" sentinel per field, resolved from Armor::evasionDefaults in afterLoad (all four fields).
	EvasionModeConfig _evasionSprint{ -1, -1, -1, -1 };
	EvasionModeConfig _evasionSneak{ -1, -1, -1, -1 };
	RuleStatBonus _psiDefence, _meleeDodge;
	RuleStatBonus _timeRecovery, _energyRecovery, _moraleRecovery, _healthRecovery, _stunRecovery, _manaRecovery;
	ModScript::BattleUnitScripts::Container _battleUnitScripts;

	ScriptValues<Armor> _scriptValues;
	std::vector<int> _customArmorPreviewIndex;
	Sint8 _allowsRunning, _allowsStrafing, _allowsSneaking, _allowsKneeling, _allowsMoving;
	bool _isPilotArmor;
	bool _allowTwoMainWeapons;
	bool _instantWoundRecovery;
	bool _isAlwaysVisible = false;
	int _standHeight, _kneelHeight, _floatHeight;
	int _meleeOriginVoxelVerticalOffset;
	int _group;
	int _listOrder;
public:
	/// Creates a blank armor ruleset.
	Armor(const std::string &type, int listOrder);
	/// Cleans up the armor ruleset.
	~Armor();

	/// Loads the armor data from YAML.
	void load(const YAML::YamlNodeReader& reader, Mod *mod, const ModScript& parsers);
	/// Cross link with other rules.
	void afterLoad(const Mod* mod);
	/// Gets whether or not there is an infinite supply of this armor.
	bool hasInfiniteSupply() const { return _infiniteSupply; }

	/// Gets the custom name of the Ufopedia article related to this armor.
	const std::string& getUfopediaType() const;

	/// Gets the armor's type.
	const std::string& getType() const;
	/// Gets the unit's sprite sheet.
	std::string getSpriteSheet() const;
	/// Gets the unit's inventory sprite.
	std::string getSpriteInventory() const;
	/// Gets the front armor level.
	int getFrontArmor() const;
	/// Gets the left side armor level.
	int getLeftSideArmor() const;
	/// Gets the right side armor level.
	int getRightSideArmor() const;
	/// Gets the rear armor level.
	int getRearArmor() const;
	/// Gets the under armor level.
	int getUnderArmor() const;
	/// Gets the armor level of armor side.
	int getArmor(UnitSide side) const;
	/// Gets the Geoscape corpse item.
	const RuleItem* getCorpseGeoscape() const;
	/// Gets the Battlescape corpse item.
	const std::vector<const RuleItem*> &getCorpseBattlescape() const;
	/// Gets the Geoscape corpse item.
	const RuleItem* getSelfDestructItem() const { return _selfDestructItem ? _selfDestructItem : _corpseGeo && _corpseGeo->getPower() > 0 ? _corpseGeo : nullptr; }
	/// Gets the stores item.
	const RuleItem* getStoreItem() const;
	/// Gets the special weapon type.
	const RuleItem* getSpecialWeapon() const;
	/// Gets the research required to be able to equip this armor.
	const RuleResearch* getRequiredResearch() const;
	/// Gets the commendation required to be able to equip this armor.
	const RuleCommendations* getRequiredAward() const { return _requiresAward; }
	/// Gets the soldier bonus required to be able to equip this armor.
	const RuleSoldierBonus* getRequiredBonus() const { return _requiresBonus; }

	/// Armor have layered armor definition. Check by Prefix.
	bool hasLayersDefinition() const { return !_layersDefaultPrefix.empty(); }
	/// Gets the default prefix for layered armor sprite names.
	const std::string &getLayersDefaultPrefix() const { return _layersDefaultPrefix; }
	/// Gets the overrides for layered armor sprite name prefix, per layer.
	const std::map<int, std::string> &getLayersSpecificPrefix() const { return _layersSpecificPrefix; }
	/// Gets the layered armor definition.
	const std::map<std::string, std::vector<std::string> > &getLayersDefinition() const { return _layersDefinition; }

	/// Gets the battlescape drawing routine ID.
	int getDrawingRoutine() const;
	/// Gets whether or not to draw bubbles (breathing animation).
	bool drawBubbles() const;
	/// DO NOT USE THIS FUNCTION OUTSIDE THE BATTLEUNIT CONSTRUCTOR OR I WILL HUNT YOU DOWN.
	MovementType getMovementType() const;
	/// Get MovementType based on depth of battle.
	MovementType getMovementTypeByDepth(int depth) const;
	/// Gets the armor's special ability.
	int getSpecialAbility() const;

	/// Gets weight value as hostile unit.
	NullableValue<AIAttackWeight> getAITargetWeightAsHostile() const { return _aiTargetWeightAsHostile; }
	/// Gets weight value as civilian unit when consider by aliens.
	NullableValue<AIAttackWeight> getAITargetWeightAsHostileCivilians() const { return _aiTargetWeightAsHostileCivilians; }
	/// Gets weight value as same faction unit.
	NullableValue<AIAttackWeight> getAITargetWeightAsFriendly() const { return _aiTargetWeightAsFriendly; }
	/// Gets weight value as neutral unit (xcom to civ or vice versa).
	NullableValue<AIAttackWeight> getAITargetWeightAsNeutral() const { return _aiTargetWeightAsNeutral; }

	/// Should turning before first step cost TU or not?
	bool getTurnBeforeFirstStep() const { return _turnBeforeFirstStep; }
	/// Gets the turn cost.
	int getTurnCost() const { return _turnCost; }

	/// DX: mod-wide default move costs, seeding armors that omit a move-cost key (reset + loaded at mod load).
	static ArmorMoveCostDefaults moveCostDefaults;

	/// Multiplier of all move costs.
	ArmorMoveCost getMoveCostBase() const { return _moveCostBase; }
	/// Multiplier of normal base move cost.
	ArmorMoveCost getMoveCostBaseNormal() const { return _moveCostBaseNormal; }
	/// Multiplier of fly base move cost.
	ArmorMoveCost getMoveCostBaseFly() const { return _moveCostBaseFly; }
	/// Multiplier of climb base move cost.
	ArmorMoveCost getMoveCostBaseClimb() const { return _moveCostBaseClimb; }

	/// Multiplier of walk move cost.
	ArmorMoveCost getMoveCostWalk() const { return _moveCostWalk; }
	/// Multiplier of run move cost.
	ArmorMoveCost getMoveCostRun() const { return _moveCostRun; }
	/// Multiplier of strafe move cost.
	ArmorMoveCost getMoveCostStrafe() const { return _moveCostStrafe; }
	/// Multiplier of sneak move cost.
	ArmorMoveCost getMoveCostSneak() const { return _moveCostSneak; }
	/// Multiplier of fly "walk" move cost.
	ArmorMoveCost getMoveCostFlyWalk() const { return _moveCostFlyWalk; }
	/// Multiplier of fly "run" move cost.
	ArmorMoveCost getMoveCostFlyRun() const { return _moveCostFlyRun; }
	/// Multiplier of fly "strafe" cost.
	ArmorMoveCost getMoveCostFlyStrafe() const { return _moveCostFlyStrafe; }
	/// Multiplier of fly up move cost.
	ArmorMoveCost getMoveCostFlyUp() const { return _moveCostFlyUp; }
	/// Multiplier of fly down move cost.
	ArmorMoveCost getMoveCostFlyDown() const { return _moveCostFlyDown; }
	/// Multiplier of climb up move cost.
	ArmorMoveCost getMoveCostClimbUp() const { return _moveCostClimbUp; }
	/// Multiplier of climb down move cost.
	ArmorMoveCost getMoveCostClimbDown() const { return _moveCostClimbDown; }
	/// Multiplier of moving using GravLift cost.
	ArmorMoveCost getMoveCostGravLift() const { return _moveCostGravLift; }

	/// Gets the move sound id. Overrides default/unit's move sound. To be used in BattleUnit constructors only too!
	int getMoveSound() const;
	/// Gets the male death sounds.
	const std::vector<int> &getMaleDeathSounds() const { return _deathSoundMale; }
	/// Gets the female death sounds.
	const std::vector<int> &getFemaleDeathSounds() const { return _deathSoundFemale; }
	/// Gets the male "select unit" sounds.
	const std::vector<int> &getMaleSelectUnitSounds() const { return _selectUnitSoundMale; }
	/// Gets the female "select unit" sounds.
	const std::vector<int> &getFemaleSelectUnitSounds() const { return _selectUnitSoundFemale; }
	/// Gets the male "start moving" sounds.
	const std::vector<int> &getMaleStartMovingSounds() const { return _startMovingSoundMale; }
	/// Gets the female "start moving" sounds.
	const std::vector<int> &getFemaleStartMovingSounds() const { return _startMovingSoundFemale; }
	/// Gets the male "select weapon" sounds.
	const std::vector<int> &getMaleSelectWeaponSounds() const { return _selectWeaponSoundMale; }
	/// Gets the female "select weapon" sounds.
	const std::vector<int> &getFemaleSelectWeaponSounds() const { return _selectWeaponSoundFemale; }
	/// Gets the male "annoyed" sounds.
	const std::vector<int> &getMaleAnnoyedSounds() const { return _annoyedSoundMale; }
	/// Gets the female "annoyed" sounds.
	const std::vector<int> &getFemaleAnnoyedSounds() const { return _annoyedSoundFemale; }
	/// Gets whether this is a normal or big unit.
	int getSize() const;
	/// Gets how much space the armor occupies in a craft.
	int getTotalSize() const;
	/// Gets damage modifier.
	float getDamageModifier(ItemDamageType dt) const;
	const std::vector<float> getDamageModifiersRaw() const;
	/// Gets loftempSet
	const std::vector<int> &getLoftempsSet() const;
	/// Gets the armor's stats.
	const UnitStats *getStats() const;
	/// Gets percent-based armor stat modifiers (100 = no change).
	const UnitStats *getStatModifiers() const { return &_statModifiers; }
	/// Does this armor define any percent stat modifiers?
	bool hasStatModifiers() const { return _hasStatModifiers; }
	/// Gets unit psi defense.
	int getPsiDefence(const BattleUnit* unit) const;
	const RuleStatBonus *getPsiDefenceRaw() const { return &_psiDefence; }
	/// DX: gets the armor's reaction-fire evasion (percent of the defensive score; 100 = no change).
	int getEvasion() const { return _evasion; }
	/// DX: gets the sprint (running) evasion config.
	const EvasionModeConfig& getEvasionSprint() const { return _evasionSprint; }
	/// DX: gets the sneak evasion config.
	const EvasionModeConfig& getEvasionSneak() const { return _evasionSneak; }
	/// DX: mod-wide defaults for sprint/sneak evasion (reset + loaded at mod load).
	static ArmorEvasionDefaults evasionDefaults;
	/// DX: bleedout eligibility for this armor (-1 auto / 0 never / 1 always). See _allowBleedOut.
	int getAllowBleedOut() const { return _allowBleedOut; }
	/// DX: mod-wide defaults for the bleedout mechanic (reset + loaded at mod load).
	static ArmorBleedoutDefaults bleedoutDefaults;
	/// DX: mod-wide sneak-mode parameters (light gate; reset + loaded at mod load).
	static ArmorSneakDefaults sneakDefaults;
	/// Gets unit melee dodge chance.
	int getMeleeDodge(const BattleUnit* unit) const;
	const RuleStatBonus *getMeleeDodgeRaw() const { return &_meleeDodge; }
	/// Gets unit dodge penalty if hit from behind.
	float getMeleeDodgeBackPenalty() const;

	/// Gets unit TU recovery.
	int getTimeRecovery(const BattleUnit* unit, int externalBonuses) const;
	const RuleStatBonus *getTimeRecoveryRaw() const { return &_timeRecovery; }
	/// Gets unit Energy recovery.
	int getEnergyRecovery(const BattleUnit* unit, int externalBonuses) const;
	const RuleStatBonus *getEnergyRecoveryRaw() const { return &_energyRecovery; }
	/// Gets unit Morale recovery.
	int getMoraleRecovery(const BattleUnit* unit, int externalBonuses) const;
	const RuleStatBonus *getMoraleRecoveryRaw() const { return &_moraleRecovery; }
	/// Gets unit Health recovery.
	int getHealthRecovery(const BattleUnit* unit, int externalBonuses) const;
	const RuleStatBonus *getHealthRecoveryRaw() const { return &_healthRecovery; }
	/// Gets unit Mana recovery.
	int getManaRecovery(const BattleUnit* unit, int externalBonuses) const;
	const RuleStatBonus* getManaRecoveryRaw() const { return &_manaRecovery; }
	/// Gets unit Stun recovery.
	int getStunRegeneration(const BattleUnit* unit, int externalBonuses) const;
	const RuleStatBonus *getStunRegenerationRaw() const { return &_stunRecovery; }

	/// Gets the armor's weight.
	int getWeight() const;
	/// Gets number of death frames.
	int getDeathFrames() const;
	/// Gets if armor uses constant animation.
	bool getConstantAnimation() const;
	/// Checks if this armor ignores gender (power suit/flying suit).
	ForcedTorso getForcedTorso() const;
	/// Gets built-in weapons of armor.
	const std::vector<const RuleItem*> &getBuiltInWeapons() const;

	/// Gets max view distance at dark in BattleScape.
	int getVisibilityAtDark() const;
	/// Gets max view distance at day in BattleScape.
	int getVisibilityAtDay() const;
	/// Gets info about camouflage at day.
	int getCamouflageAtDay() const;
	/// Gets info about camouflage at dark.
	int getCamouflageAtDark() const;
	/// Gets info about anti camouflage at day.
	int getAntiCamouflageAtDay() const;
	/// Gets info about anti camouflage at dark.
	int getAntiCamouflageAtDark() const;
	/// DX: gets the dynamic-cloak config (camouflage that breaks when the unit acts).
	const ArmorCloak &getCloak() const { return _cloak; }
	/// Gets info about heat vision.
	int getVisibilityThroughSmoke() const { return _visibilityThroughSmoke; }
	/// Gets info about visibility through fire.
	int getVisibilityThroughFire() const { return _visibilityThroughFire; }
	/// Gets info about psi vision.
	int getPsiVision() const;
	/// Gets info about psi camouflage.
	int getPsiCamouflage() const;
	/// Gets personal soldier light radius;
	int getPersonalLightFriend() const;
	/// Gets personal alien light radius;
	int getPersonalLightHostile() const;
	/// Gets personal civilian light radius;
	int getPersonalLightNeutral() const;
	/// Gets if unit should be always visible.
	bool isAlwaysVisible() const { return _isAlwaysVisible; }

	/// Gets how armor react to fear.
	bool getFearImmune(bool def = false) const;
	/// Gets how armor react to bleeding.
	bool getBleedImmune(bool def = false) const;
	/// Gets how armor react to inflicted pain.
	bool getPainImmune(bool def = false) const;
	/// Gets how armor react to zombification.
	bool getZombiImmune(bool def = false) const;
	/// Gets whether or not this unit ignores close quarters threats.
	bool getIgnoresMeleeThreat(bool def = false) const;
	/// Gets whether or not this unit is a close quarters threat.
	bool getCreatesMeleeThreat(bool def = true) const;
	/// Gets how much damage (over the maximum HP) is needed to vaporize/disintegrate a unit.
	float getOverKill() const;
	/// Get face base color
	int getFaceColorGroup() const;
	/// Get hair base color
	int getHairColorGroup() const;
	/// Get utile base color
	int getUtileColorGroup() const;
	/// Get rank base color
	int getRankColorGroup() const;
	/// Get face base color
	int getFaceColor(int i) const;
	const std::vector<int> &getFaceColorRaw() const { return _faceColor; }
	/// Get hair base color
	int getHairColor(int i) const;
	const std::vector<int> &getHairColorRaw() const { return _hairColor; }
	/// Get utile base color
	int getUtileColor(int i) const;
	const std::vector<int> &getUtileColorRaw() const { return _utileColor; }
	/// Get rank base color
	int getRankColor(int i) const;
	const std::vector<int> &getRankColorRaw() const { return _rankColor; }
	/// Can we access this unit's inventory?
	bool hasInventory() const;
	/// DX: does this armor take its turret sprite from the mounted weapon rather than a fixed index?
	bool getTurretFromWeapon() const { return _turretFromWeapon; }
	/// DX: is this unit exempt from the start-of-turn encumbrance TU reduction?
	bool getIgnoresEncumbrance() const { return _ignoresEncumbrance; }
	/// Gets the inventory layout this armor assigns (nullptr => use the default layout).
	const RuleInventoryLayout* getInventoryLayout() const { return _inventoryLayout; }
	/// Gets script.
	template<typename Script>
	const typename Script::Container &getScript() const { return _battleUnitScripts.get<Script>(); }
	/// Get all script values.
	const ScriptValues<Armor> &getScriptValuesRaw() const { return _scriptValues; }

	/// Gets the armor's units.
	const std::vector<const RuleSoldier*> &getUnitsRaw() const;
	/// Gets the armor's supported soldier ranks.
	const std::vector<int>& getRanksRaw() const { return _ranks; }
	/// Check if a soldier can use this armor.
	bool getCanBeUsedBy(const Soldier* soldier) const;


	/// Gets the index of the sprite in the CustomArmorPreview sprite set
	const std::vector<int> &getCustomArmorPreviewIndex() const;
	/// Can you run while wearing this armor?
	bool allowsRunning(bool def = true) const;
	/// Can you strafe while wearing this armor?
	bool allowsStrafing(bool def = true) const;
	/// Can you sneak while wearing this armor?
	bool allowsSneaking(bool def = true) const;
	/// Can you kneel while wearing this armor?
	bool allowsKneeling(bool def = true) const;
	/// Can you move while wearing this armor?
	bool allowsMoving() const;
	/// Does this armor count for pilot bonuses?
	bool isPilotArmor() const { return _isPilotArmor; }
	/// Does this armor allow two main weapons during autoequip?
	bool getAllowTwoMainWeapons() const { return _allowTwoMainWeapons; }
	/// Does this armor instantly recover any wounds after the battle?
	bool getInstantWoundRecovery() const;
	/// Gets a unit's height when standing while wearing this armor.
	int getStandHeight() const;
	/// Gets a unit's height when kneeling while wearing this armor.
	int getKneelHeight() const;
	/// Gets a unit's float elevation while wearing this armor.
	int getFloatHeight() const;
	/// Gets a unit's offset for melee attacks.
	int getMeleeOriginVoxelVerticalOffset() const { return _meleeOriginVoxelVerticalOffset; }

	/// Gets the armor type group.
	int getGroup() const { return _group; }
	/// Get the list weight for this armor.
	int getListOrder() const { return _listOrder; }
};

}
