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
#include "BattleUnit.h"
#include "BattleItem.h"
#include <sstream>
#include "../Engine/Surface.h"
#include "../Engine/Script.h"
#include "../Engine/ScriptBind.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Battlescape/Pathfinding.h"
#include "../Battlescape/BattlescapeGame.h"
#include "../Battlescape/AIModule.h"
#include "../Battlescape/Inventory.h"
#include "../Mod/Mod.h"
#include "../Mod/Armor.h"
#include "../Mod/Unit.h"
#include "../Mod/RuleInventory.h"
#include "../Mod/RuleSoldier.h"
#include "../Mod/Mod.h"
#include "Soldier.h"
#include "Tile.h"
#include "SavedGame.h"
#include "SavedBattleGame.h"
#include "../Engine/ShaderDraw.h"
#include "../Engine/ShaderMove.h"
#include "../Engine/Options.h"
#include "BattleUnitStatistics.h"
#include "../fmath.h"
#include "../Battlescape/TileEngine.h"
#include "../Mod/RuleInventoryLayout.h"
#include "../Engine/Game.h"
#include "../Engine/Sound.h"
#include "../Savegame/Role.h"
#include "../Battlescape/CombatLog.h"
#include "BattleEffect.h"

namespace OpenXcom
{

/**
 * Initializes a BattleUnit from a Soldier
 * @param soldier Pointer to the Soldier.
 * @param depth the depth of the battlefield (used to determine movement type in case of MT_FLOAT).
 */
BattleUnit::BattleUnit(Soldier *soldier, int depth, int maxViewDistance) :
	_faction(FACTION_PLAYER), _originalFaction(FACTION_PLAYER), _killedBy(FACTION_PLAYER), _id(0), _tile(0),
	_lastPos(Position()), _direction(0), _toDirection(0), _directionTurret(0), _toDirectionTurret(0),
	_verticalDirection(0), _status(STATUS_STANDING), _wantsToSurrender(false), _walkPhase(0), _fallPhase(0), _kneeled(false), _floating(false),
	_dontReselect(false), _fire(0), _currentAIState(0), _visible(false),
	_expBravery(0), _expReactions(0), _expFiring(0), _expThrowing(0), _expPsiSkill(0), _expPsiStrength(0), _expMelee(0),
	_motionPoints(0), _kills(0), _hitByFire(false), _fireMaxHit(0), _smokeMaxHit(0), _moraleRestored(0), _coverReserve(0), _charging(0), _turnsSinceSpotted(255),
	_statistics(), _murdererId(0), _mindControllerID(0), _fatalShotSide(SIDE_FRONT), _fatalShotBodyPart(BODYPART_HEAD), _armor(0),
	_geoscapeSoldier(soldier), _unitRules(0), _rankInt(0), _turretType(-1), _hidingForTurn(false), _floorAbove(false), _respawn(false), _alreadyRespawned(false), _isLeeroyJenkins(false),
	_overwatch(false), _overwatchTarget(-1,-1,-1), _overwatchWeaponSlot(), _justKilled(false), _bleedingOut(false), _overwatchShotsAvailable(0),
	_ongoingAction(BA_NONE), _controlling(0), _controller(0), _loadedControllingId(-1), _isVehicle(false), _inventoryLayout("STR_STANDARD_INV"), _inCombat(false), _depth(depth), _movementAction(BAM_NORMAL),
	_hadNightVision(false), _battleExperience(0), _turnsAwake(0), _aboutToFall(false)
{
	_name = soldier->getName(true);
	_id = soldier->getId();
	_type = "SOLDIER";
	_rank = soldier->getRankString();
	_baseStats = *soldier->getCurrentStats();
	_standHeight = soldier->getRules()->getStandHeight();
	_kneelHeight = soldier->getRules()->getKneelHeight();
	_floatHeight = soldier->getRules()->getFloatHeight();
	_deathSound = std::vector<int>(); // this one is hardcoded
	_aggroSound = -1;
	_armor = soldier->getArmor();
	_moveSound = _armor->getMoveSound() != -1 ? _armor->getMoveSound() : -1; // there's no unit move sound, thus hardcoded -1
	_intelligence = 2;
	_aggression = 1;
	_specab = SPECAB_NONE;
	_movementType = _armor->getMovementType();
	_isVehicle = soldier->isVehicle();
	_inventoryLayout = soldier->getInventoryLayout();
	if (_movementType == MT_FLOAT)
	{
		if (depth > 0)
		{
			_movementType = MT_FLY;
		}
		else
		{
			_movementType = MT_WALK;
		}
	}
	else if (_movementType == MT_SINK)
	{
		if (depth == 0)
		{
			_movementType = MT_FLY;
		}
		else
		{
			_movementType = MT_WALK;
		}
	}

	/*if(soldier->isVehicle() && _armor->getIsVehicleItem())
	{
		_stats = *_armor->getStats();	// armors may modify effective stats
	}
	else
	{*/
	_baseStats += *_armor->getStats();	// armors may modify effective stats
	updateStats();
	//}
	_maxViewDistanceAtDark = _armor->getVisibilityAtDark() ? _armor->getVisibilityAtDark() : 9;
	_maxViewDistanceAtDay = _armor->getVisibilityAtDay() ? _armor->getVisibilityAtDay() : maxViewDistance;
	_loftempsSet = _armor->getLoftempsSet();
	_gender = soldier->getGender();
	_faceDirection = -1;
	_breathFrame = 0;
	_floorAbove = false;
	_breathing = false;

	int rankbonus = 0;

	switch (soldier->getRank())
	{
	case RANK_SERGEANT:  rankbonus =  1; break;
	case RANK_CAPTAIN:   rankbonus =  3; break;
	case RANK_COLONEL:   rankbonus =  6; break;
	case RANK_COMMANDER: rankbonus = 10; break;
	default:             rankbonus =  0; break;
	}

	_value = 20 + soldier->getMissions() + rankbonus;

	_tu = _stats.tu;
	_energy = _stats.stamina;
	_health = _stats.health;
	_morale = 100;
	if (Options::everyoneFightsNobodyQuits)
	{
		// wounded soldiers start with half the energy and lowered morale
		if (soldier->isWounded())
		{
			_energy = _stats.stamina / 2;
			_morale = 75;
		}
		// statistically worse than average
		_health = _health - ((soldier->getWoundRecovery(0.0f, 0.0f) * 3) / 2);
		if (_health < 1)
		{
			// this is actually a punishment, strategically it is better to leave them behind :)
			_health = 1;
		}
	}
	_stunlevel = 0;
	_maxArmor[SIDE_FRONT] = _armor->getFrontArmor();
	_maxArmor[SIDE_LEFT] = _armor->getLeftSideArmor();
	_maxArmor[SIDE_RIGHT] = _armor->getRightSideArmor();
	_maxArmor[SIDE_REAR] = _armor->getRearArmor();
	_maxArmor[SIDE_UNDER] = _armor->getUnderArmor();
	_currentArmor[SIDE_FRONT] = _maxArmor[SIDE_FRONT];
	_currentArmor[SIDE_LEFT] = _maxArmor[SIDE_LEFT];
	_currentArmor[SIDE_RIGHT] = _maxArmor[SIDE_RIGHT];
	_currentArmor[SIDE_REAR] = _maxArmor[SIDE_REAR];
	_currentArmor[SIDE_UNDER] = _maxArmor[SIDE_UNDER];
	updateArmor();
	for (int i = 0; i < BODYPART_MAX; ++i)
		_fatalWounds[i] = 0;
	for (int i = 0; i < SPEC_WEAPON_MAX; ++i)
		_specWeapon[i] = 0;

	_activeHand = "STR_RIGHT_HAND";

	lastCover = Position(-1, -1, -1);

	_statistics = new BattleUnitStatistics();

	deriveRank();

	int look = soldier->getGender() + 2 * soldier->getLook() + 8 * soldier->getLookVariant();
	setupSoldierRecolor();
	determineWeaponSlots();
	// TODO: Necessary? Handled by setupSoldierRecolor? setRecolor(look, look, _rankInt);
}

/**
 * Updates a BattleUnit from a Soldier (after a change of armor)
 * @param soldier Pointer to the Soldier.
 * @param depth the depth of the battlefield (used to determine movement type in case of MT_FLOAT).
 */
void BattleUnit::updateArmorFromSoldier(Soldier *soldier, int depth, int maxViewDistance)
{
	_stats = *soldier->getCurrentStats();
	_armor = soldier->getArmor();

	_movementType = _armor->getMovementType();
	if (_movementType == MT_FLOAT) {
		if (depth > 0) { _movementType = MT_FLY; } else { _movementType = MT_WALK; }
	} else if (_movementType == MT_SINK) {
		if (depth == 0) { _movementType = MT_FLY; } else { _movementType = MT_WALK; }
	}

	_stats += *_armor->getStats();	// armors may modify effective stats
	_maxViewDistanceAtDark = _armor->getVisibilityAtDark() ? _armor->getVisibilityAtDark() : 9;
	_maxViewDistanceAtDay = _armor->getVisibilityAtDay() ? _armor->getVisibilityAtDay() : maxViewDistance;
	_loftempsSet = _armor->getLoftempsSet();

	_tu = _stats.tu;
	_energy = _stats.stamina;
	_health = _stats.health;
	_maxArmor[SIDE_FRONT] = _armor->getFrontArmor();
	_maxArmor[SIDE_LEFT] = _armor->getLeftSideArmor();
	_maxArmor[SIDE_RIGHT] = _armor->getRightSideArmor();
	_maxArmor[SIDE_REAR] = _armor->getRearArmor();
	_maxArmor[SIDE_UNDER] = _armor->getUnderArmor();
	_currentArmor[SIDE_FRONT] = _maxArmor[SIDE_FRONT];
	_currentArmor[SIDE_LEFT] = _maxArmor[SIDE_LEFT];
	_currentArmor[SIDE_RIGHT] = _maxArmor[SIDE_RIGHT];
	_currentArmor[SIDE_REAR] = _maxArmor[SIDE_REAR];
	_currentArmor[SIDE_UNDER] = _maxArmor[SIDE_UNDER];
}

/**
 * Initializes a BattleUnit from a Unit (non-player) object.
 * @param unit Pointer to Unit object.
 * @param faction Which faction the units belongs to.
 * @param id Unique unit ID.
 * @param armor Pointer to unit Armor.
 * @param diff difficulty level (for stat adjustment).
 * @param depth the depth of the battlefield (used to determine movement type in case of MT_FLOAT).
 */
BattleUnit::BattleUnit(Unit *unit, UnitFaction faction, int id, Armor *armor, StatAdjustment *adjustment, int depth, int maxViewDistance) :
	_faction(faction), _originalFaction(faction), _killedBy(faction), _id(id),
	_tile(0), _lastPos(Position()), _direction(0), _toDirection(0), _directionTurret(0),
	_toDirectionTurret(0), _verticalDirection(0), _status(STATUS_STANDING), _wantsToSurrender(false), _walkPhase(0),
	_fallPhase(0), _kneeled(false), _floating(false), _dontReselect(false), _fire(0), _currentAIState(0),
	_visible(false), _expBravery(0), _expReactions(0), _expFiring(0),
	_expThrowing(0), _expPsiSkill(0), _expPsiStrength(0), _expMelee(0), _motionPoints(0), _kills(0), _hitByFire(false), _fireMaxHit(0), _smokeMaxHit(0),
	_moraleRestored(0), _coverReserve(0), _charging(0), _turnsSinceSpotted(255),
	_statistics(), _murdererId(0), _mindControllerID(0), _fatalShotSide(SIDE_FRONT),
	_fatalShotBodyPart(BODYPART_HEAD), _armor(armor), _geoscapeSoldier(0), _unitRules(unit),
	_rankInt(0), _turretType(-1), _hidingForTurn(false), _respawn(false), _alreadyRespawned(false), _isLeeroyJenkins(false),
	_overwatch(false), _overwatchTarget(-1,-1,-1), _overwatchWeaponSlot(), _justKilled(false), _bleedingOut(false),
	_overwatchShotsAvailable(0), _ongoingAction(BA_NONE), _controlling(0), _controller(0), _loadedControllingId(-1), _isVehicle(false), _inventoryLayout("STR_STANDARD_INV"), _inCombat(false), _depth(depth), _movementAction(BAM_STRAFE),
	_hadNightVision(false), _battleExperience(0), _turnsAwake(0), _aboutToFall(false)
{
	_type = unit->getType();
	_rank = unit->getRank();
	_race = unit->getRace();
	_baseStats = *unit->getStats();
	_standHeight = unit->getStandHeight();
	_kneelHeight = unit->getKneelHeight();
	_floatHeight = unit->getFloatHeight();
	_loftempsSet = _armor->getLoftempsSet();
	_deathSound = unit->getDeathSounds();
	_aggroSound = unit->getAggroSound();
	_moveSound = _armor->getMoveSound() != -1 ? _armor->getMoveSound() : unit->getMoveSound();
	_intelligence = unit->getIntelligence();
	_aggression = unit->getAggression();
	_specab = (SpecialAbility) unit->getSpecialAbility();
	_spawnUnit = unit->getSpawnUnit();
	_value = unit->getValue();
	_faceDirection = -1;
	_isLeeroyJenkins = unit->isLeeroyJenkins();

	_movementType = _armor->getMovementType();
	if (_movementType == MT_FLOAT)
	{
		if (depth > 0)
		{
			_movementType = MT_FLY;
		}
		else
		{
			_movementType = MT_WALK;
		}
	}
	else if (_movementType == MT_SINK)
	{
		if (depth == 0)
		{
			_movementType = MT_FLY;
		}
		else
		{
			_movementType = MT_WALK;
		}
	}

	_baseStats += *_armor->getStats();	// armors may modify effective stats
	updateStats();
	_maxViewDistanceAtDark = _armor->getVisibilityAtDark() ? _armor->getVisibilityAtDark() : faction==FACTION_HOSTILE ? maxViewDistance : 9;
	_maxViewDistanceAtDay = _armor->getVisibilityAtDay() ? _armor->getVisibilityAtDay() : maxViewDistance;

	_breathFrame = -1; // most aliens don't breathe per-se, that's exclusive to humanoids
	if (armor->getDrawingRoutine() == 14)
	{
		_breathFrame = 0;
	}
	_floorAbove = false;
	_breathing = false;

	_maxArmor[SIDE_FRONT] = _armor->getFrontArmor();
	_maxArmor[SIDE_LEFT] = _armor->getLeftSideArmor();
	_maxArmor[SIDE_RIGHT] = _armor->getRightSideArmor();
	_maxArmor[SIDE_REAR] = _armor->getRearArmor();
	_maxArmor[SIDE_UNDER] = _armor->getUnderArmor();

	if (faction == FACTION_HOSTILE)
	{
		adjustStats(*adjustment);
	}

	_tu = _stats.tu;
	_energy = _stats.stamina;
	_health = _stats.health;
	_morale = 100;
	_stunlevel = 0;
	_currentArmor[SIDE_FRONT] = _maxArmor[SIDE_FRONT];
	_currentArmor[SIDE_LEFT] = _maxArmor[SIDE_LEFT];
	_currentArmor[SIDE_RIGHT] = _maxArmor[SIDE_RIGHT];
	_currentArmor[SIDE_REAR] = _maxArmor[SIDE_REAR];
	_currentArmor[SIDE_UNDER] = _maxArmor[SIDE_UNDER];
	for (int i = 0; i < BODYPART_MAX; ++i)
		_fatalWounds[i] = 0;
	for (int i = 0; i < SPEC_WEAPON_MAX; ++i)
		_specWeapon[i] = 0;

	_activeHand = "STR_RIGHT_HAND";
	_gender = GENDER_MALE;

	lastCover = Position(-1, -1, -1);

	_statistics = new BattleUnitStatistics();

	int generalRank = 0;
	if (faction == FACTION_HOSTILE)
	{
		const int max = 7;
		const char* rankList[max] =
		{
			"STR_LIVE_SOLDIER",
			"STR_LIVE_ENGINEER",
			"STR_LIVE_MEDIC",
			"STR_LIVE_NAVIGATOR",
			"STR_LIVE_LEADER",
			"STR_LIVE_COMMANDER",
			"STR_LIVE_TERRORIST",
		};
		for (int i = 0; i < max; ++i)
		{
			if (_rank.compare(rankList[i]) == 0)
			{
				generalRank = i;
				break;
			}
		}
	}
	else if (faction == FACTION_NEUTRAL)
	{
		generalRank = RNG::seedless(0, 7);
	}

	setRecolor(RNG::seedless(0, 127), RNG::seedless(0, 127), generalRank, _armor->getBaseColorDefault());
	// TODO: Check if the above handles this.  setRecolor(std::rand() % 8, (_armor->getUtileColorCount() > 0) ? std::rand() % _armor->getUtileColorCount() : -1, generalRank);

	determineWeaponSlots();
}

/**
 *
 */
BattleUnit::~BattleUnit()
{
	clearControlledBy();
	clearControlling();
	clearOngoingAction();
	for (std::vector<BattleUnitKills*>::const_iterator i = _statistics->kills.begin(); i != _statistics->kills.end(); ++i)
	{
		delete *i;
	}
	delete _statistics;
	delete _currentAIState;
	for (auto ii = _activeEffects.cbegin(); ii != _activeEffects.cend(); ++ii)
	{
		delete *ii;
	}
}

/**
 * Loads the unit from a YAML file.
 * @param node YAML node.
 */
void BattleUnit::load(const YAML::Node &node, const ScriptGlobal *shared)
{
	_id = node["id"].as<int>(_id);
	_faction = _originalFaction = (UnitFaction)node["faction"].as<int>(_faction);
	_status = (UnitStatus)node["status"].as<int>(_status);
	_wantsToSurrender = node["wantsToSurrender"].as<bool>(_wantsToSurrender);
	_pos = node["position"].as<Position>(_pos);
	_direction = _toDirection = node["direction"].as<int>(_direction);
	_directionTurret = _toDirectionTurret = node["directionTurret"].as<int>(_directionTurret);
	_tu = node["tu"].as<int>(_tu);
	_health = node["health"].as<int>(_health);
	_stunlevel = node["stunlevel"].as<int>(_stunlevel);
	_energy = node["energy"].as<int>(_energy);
	_morale = node["morale"].as<int>(_morale);
	_kneeled = node["kneeled"].as<bool>(_kneeled);
	_floating = node["floating"].as<bool>(_floating);
	for (int i=0; i < SIDE_MAX; i++)
		_currentArmor[i] = node["armor"][i].as<int>(_currentArmor[i]);
	for (int i=0; i < BODYPART_MAX; i++)
		_fatalWounds[i] = node["fatalWounds"][i].as<int>(_fatalWounds[i]);
	_fire = node["fire"].as<int>(_fire);
	_expBravery = node["expBravery"].as<int>(_expBravery);
	_expReactions = node["expReactions"].as<int>(_expReactions);
	_expFiring = node["expFiring"].as<int>(_expFiring);
	_expThrowing = node["expThrowing"].as<int>(_expThrowing);
	_expPsiSkill = node["expPsiSkill"].as<int>(_expPsiSkill);
	_expPsiStrength = node["expPsiStrength"].as<int>(_expPsiStrength);
	_expMelee = node["expMelee"].as<int>(_expMelee);
	_turretType = node["turretType"].as<int>(_turretType);
	_visible = node["visible"].as<bool>(_visible);
	_turnsSinceSpotted = node["turnsSinceSpotted"].as<int>(_turnsSinceSpotted);
	_killedBy = (UnitFaction)node["killedBy"].as<int>(_killedBy);
	_moraleRestored = node["moraleRestored"].as<int>(_moraleRestored);
	_rankInt = node["rankInt"].as<int>(_rankInt);
	_originalFaction = (UnitFaction)node["originalFaction"].as<int>(_originalFaction);
	_kills = node["kills"].as<int>(_kills);
	_dontReselect = node["dontReselect"].as<bool>(_dontReselect);
	_charging = 0;
	_spawnUnit = node["spawnUnit"].as<std::string>(_spawnUnit);
	_motionPoints = node["motionPoints"].as<int>(0);
	_respawn = node["respawn"].as<bool>(_respawn);
	_alreadyRespawned = node["alreadyRespawned"].as<bool>(_alreadyRespawned);
	_activeHand = node["activeHand"].as<std::string>(_activeHand);
	if (node["tempUnitStatistics"])
	{
		_statistics->load(node["tempUnitStatistics"]);
	}
	_murdererId = node["murdererId"].as<int>(_murdererId);
	_fatalShotSide = (UnitSide)node["fatalShotSide"].as<int>(_fatalShotSide);
	_fatalShotBodyPart = (UnitBodyPart)node["fatalShotBodyPart"].as<int>(_fatalShotBodyPart);
	_murdererWeapon = node["murdererWeapon"].as<std::string>(_murdererWeapon);
	_murdererWeaponAmmo = node["murdererWeaponAmmo"].as<std::string>(_murdererWeaponAmmo);

	const YAML::Node& effects = node["activeEffects"];
	if (effects && effects.IsSequence())
	{
		if (_activeEffects.size())
		{
			for (auto ii = _activeEffects.begin(); ii != _activeEffects.end();)
			{
				delete (*ii);
			}
			_activeEffects.clear();
		}
		for (size_t ii = 0; ii < effects.size(); ++ii)
		{
			const YAML::Node &effectNode = effects[ii];
			BattleEffect *effect = new BattleEffect(Game::getMod()->getEffect(effectNode["type"].as<std::string>()), this, 0);
			effect->load(effectNode);
			_activeEffects.push_back(effect);
		}
	}

	if (const YAML::Node& p = node["recolor"])
	{
		_recolor.clear();
		for (size_t i = 0; i < p.size(); ++i)
		{
			_recolor.push_back(std::make_pair(p[i][0].as<int>(), p[i][1].as<int>()));
		}
	}
	_armorColor = node["armorColor"].as<int>(_armorColor);
	_mindControllerID = node["mindControllerID"].as<int>(_mindControllerID);
	_scriptValues.load(node, shared);
	_bleedingOut = node["bleedingOut"].as<bool>(_bleedingOut);
	_overwatch = node["overwatch"].as<bool>(false);
	_ongoingAction = (BattleActionType)node["ongoingAction"].as<int>(_ongoingAction);
	_loadedControllingId = node["controllingId"].as<int>(_loadedControllingId);
	if(_overwatch)
	{
		_overwatchTarget = node["overwatchTarget"].as<Position>(_overwatchTarget);
		_overwatchWeaponSlot = node["overwatchWeaponSlot"].as<std::string>(_overwatchWeaponSlot);
		_overwatchShotsAvailable = node["overwatchShotsAvailable"].as<int>(_overwatchShotsAvailable);
	}

	_battleExperience = node["battleExperience"].as<int>(_battleExperience);
	_turnsAwake = node["turnsAwake"].as<int>(_turnsAwake);

	_inCombat = true;

	updateArmor();
	updateStats();
	determineWeaponSlots();
}

void BattleUnit::initLoaded(SavedBattleGame *save)
{
	if(_loadedControllingId >= 0)
	{
		std::vector<BattleUnit*> *units = save->getUnits();
		for(std::vector<BattleUnit*>::const_iterator ii = units->begin(); ii != units->end(); ++ii)
		{
			if((*ii)->getId() == _loadedControllingId)
			{
				setControlling(*ii);
				break;
			}
		}
		_loadedControllingId = -1;
	}
}

/**
 * Saves the soldier to a YAML file.
 * @return YAML node.
 */
YAML::Node BattleUnit::save(const ScriptGlobal *shared) const
{
	YAML::Node node;

	node["id"] = _id;
	node["genUnitType"] = _type;
	node["genUnitArmor"] = _armor->getType();
	node["faction"] = (int)_faction;
	node["status"] = (int)_status;
	node["wantsToSurrender"] = _wantsToSurrender;
	node["position"] = _pos;
	node["direction"] = _direction;
	node["directionTurret"] = _directionTurret;
	node["tu"] = _tu;
	node["health"] = _health;
	node["stunlevel"] = _stunlevel;
	node["energy"] = _energy;
	node["morale"] = _morale;
	node["kneeled"] = _kneeled;
	node["floating"] = _floating;
	for (int i=0; i < SIDE_MAX; i++) node["armor"].push_back(_currentArmor[i]);
	for (int i=0; i < BODYPART_MAX; i++) node["fatalWounds"].push_back(_fatalWounds[i]);
	node["fire"] = _fire;
	node["expBravery"] = _expBravery;
	node["expReactions"] = _expReactions;
	node["expFiring"] = _expFiring;
	node["expThrowing"] = _expThrowing;
	node["expPsiSkill"] = _expPsiSkill;
	node["expPsiStrength"] = _expPsiStrength;
	node["expMelee"] = _expMelee;
	node["turretType"] = _turretType;
	node["visible"] = _visible;
	node["turnsSinceSpotted"] = _turnsSinceSpotted;
	node["rankInt"] = _rankInt;
	node["moraleRestored"] = _moraleRestored;
	if (getAIModule())
	{
		node["AI"] = getAIModule()->save();
	}
	node["killedBy"] = (int)_killedBy;
	if (_originalFaction != _faction)
		node["originalFaction"] = (int)_originalFaction;
	if (_kills)
		node["kills"] = _kills;
	if (_faction == FACTION_PLAYER && _dontReselect)
		node["dontReselect"] = _dontReselect;
	if (!_spawnUnit.empty())
		node["spawnUnit"] = _spawnUnit;
	node["motionPoints"] = _motionPoints;
	node["respawn"] = _respawn;
	node["alreadyRespawned"] = _alreadyRespawned;
	node["activeHand"] = _activeHand;
	node["tempUnitStatistics"] = _statistics->save();
	node["murdererId"] = _murdererId;
	node["fatalShotSide"] = (int)_fatalShotSide;
	node["fatalShotBodyPart"] = (int)_fatalShotBodyPart;
	node["murdererWeapon"] = _murdererWeapon;
	node["murdererWeaponAmmo"] = _murdererWeaponAmmo;
	node["bleedingOut"] = _bleedingOut;

	for (size_t i = 0; i < _recolor.size(); ++i)
	{
		YAML::Node p;
		p.push_back((int)_recolor[i].first);
		p.push_back((int)_recolor[i].second);
		node["recolor"].push_back(p);
	}
	node["armorColor"] = _armorColor;
	node["mindControllerID"] = _mindControllerID;

	node["overwatch"] = _overwatch;
	node["ongoingAction"] = (int)_ongoingAction;
	if(_controlling)
	{
		node["controllingId"] = _controlling->getId();
	}

	if(_overwatch)
	{
		node["overwatchTarget"] = _overwatchTarget;
		node["overwatchWeaponSlot"] = _overwatchWeaponSlot;
		node["overwatchShotsAvailable"] = _overwatchShotsAvailable;
	}

	if (_activeEffects.size())
	{
		for (auto ii = _activeEffects.cbegin(); ii != _activeEffects.cend(); ++ii)
		{
			node["activeEffects"].push_back((*ii)->save());
		}
	}

	node["battleExperience"] = _battleExperience;
	node["turnsAwake"] = _turnsAwake;

	_scriptValues.save(node, shared);

	return node;
}

void BattleUnit::initBattle()
{
	if (_geoscapeSoldier)
	{
		setupSoldierRecolor();
	}
}

void BattleUnit::setupSoldierRecolor()
{
	int look = _geoscapeSoldier->getGender() + 2 * _geoscapeSoldier->getLook();

	auto &modColorList = Game::getMod()->getSoldierArmorBaseColors();

	//int utileColor = _armor->getDefaultUtileColor();
	//int armorUtileColor = ;
	/*for (auto ii = modColorList.cbegin(); ii != modColorList.cend(); ++ii)
	{
		if (_geoscapeSoldier->getArmorColor() == (*ii))
		{
			utileColor = armorUtileColor;
			break;
		}

		++armorUtileColor;
	}*/

	int armorColor = _armor->getBaseColorDefault();
	auto armorColorFind = modColorList.find(_geoscapeSoldier->getArmorColor());
	if (armorColorFind != modColorList.end())
	{
		armorColor = armorColorFind->second;
	}

	// TODO: Utile color?
	setRecolor(look, 0, _rankInt, armorColor);
}

/**
 * Prepare vector values for recolor.
 * @param basicLook select index for hair and face color.
 * @param utileLook select index for utile color.
 * @param rankLook select index for rank color.
 */
void BattleUnit::setRecolor(int basicLook, int utileLook, int rankLook, int baseArmorColor)
{
	const int colorsMax = 4;
	std::pair<int, int> colors[colorsMax] =
	{
		std::make_pair(_armor->getFaceColorGroup(), _armor->getFaceColor(basicLook)),
		std::make_pair(_armor->getHairColorGroup(), _armor->getHairColor(basicLook)),
		std::make_pair(_armor->getUtileColorGroup(), _armor->getUtileColor(utileLook)),
		std::make_pair(_armor->getRankColorGroup(), _armor->getRankColor(rankLook))
	};

	_recolor.clear();
	for (int i = 0; i < colorsMax; ++i)
	{
		if (colors[i].first > 0 && colors[i].second >= 0)
		{
			_recolor.push_back(std::make_pair(colors[i].first << 4, colors[i].second));
		}
	}

	_armorColor = baseArmorColor;
}

/**
 * Returns the BattleUnit's unique ID.
 * @return Unique ID.
 */
int BattleUnit::getId() const
{
	return _id;
}

/**
 * Changes the BattleUnit's position.
 * @param pos position
 * @param updateLastPos refresh last stored position
 */
void BattleUnit::setPosition(Position pos, bool updateLastPos)
{
	if (updateLastPos) { _lastPos = _pos; }
	_pos = pos;
}

/**
 * Gets the BattleUnit's position.
 * @return position
 */
Position BattleUnit::getPosition() const
{
	return _pos;
}

/**
 * Gets the BattleUnit's position.
 * @return position
 */
Position BattleUnit::getLastPosition() const
{
	return _lastPos;
}

/**
 * Gets position of unit center in vexels.
 * @return position in vexels
 */
Position BattleUnit::getPositionVexels() const
{
	Position center = _pos.toVexel();
	center += Position(8, 8, 0) * _armor->getSize();
	return center;
}

/**
 * Gets the BattleUnit's destination.
 * @return destination
 */
Position BattleUnit::getDestination() const
{
	return _destination;
}

/**
 * Changes the BattleUnit's (horizontal) direction.
 * Only used for initial unit placement.
 * @param direction new horizontal direction
 */
void BattleUnit::setDirection(int direction)
{
	_direction = direction;
	_toDirection = direction;
	_directionTurret = direction;
	_toDirectionTurret = direction;
}

/**
 * Changes the BattleUnit's (horizontal) face direction.
 * Only used for strafing moves.
 * @param direction new face direction
 */
void BattleUnit::setFaceDirection(int direction)
{
	_faceDirection = direction;
}

/**
 * Gets the BattleUnit's (horizontal) direction.
 * @return horizontal direction
 */
int BattleUnit::getDirection() const
{
	return _direction;
}

/**
 * Gets the BattleUnit's (horizontal) face direction.
 * Used only during strafing moves.
 * @return face direction
 */
int BattleUnit::getFaceDirection() const
{
	return _faceDirection;
}

/**
 * Gets the BattleUnit's turret direction.
 * @return direction
 */
int BattleUnit::getTurretDirection() const
{
	return _directionTurret;
}

/**
 * Gets the BattleUnit's turret To direction.
 * @return toDirectionTurret
 */
int BattleUnit::getTurretToDirection() const
{
	return _toDirectionTurret;
}

/**
 * Gets the BattleUnit's vertical direction. This is when going up or down.
 * @return direction
 */
int BattleUnit::getVerticalDirection() const
{
	return _verticalDirection;
}

/**
 * Gets the unit's status.
 * @return the unit's status
 */
UnitStatus BattleUnit::getStatus() const
{
	return _status;
}

/**
* Does the unit want to surrender?
* @return True if the unit wants to surrender
*/
bool BattleUnit::wantsToSurrender() const
{
	return _wantsToSurrender;
}

/**
 * Initialises variables to start walking.
 * @param direction Which way to walk.
 * @param destination The position we should end up on.
 * @param tileBelowMe Which tile is currently below the unit.
 * @param cache Update cache?
 */
void BattleUnit::startWalking(int direction, Position destination, Tile *tileBelowMe, bool cache)
{
	if (direction >= Pathfinding::DIR_UP)
	{
		_verticalDirection = direction;
		_status = STATUS_FLYING;
	}
	else
	{
		_direction = direction;
		_status = STATUS_WALKING;
	}
	bool floorFound = false;
	if (!_tile->hasNoFloor(tileBelowMe))
	{
		floorFound = true;
	}
	if (!floorFound || direction >= Pathfinding::DIR_UP)
	{
		_status = STATUS_FLYING;
		_floating = true;
	}
	else
	{
		_floating = false;
	}

	_walkPhase = 0;
	_destination = destination;
	_lastPos = _pos;
	_kneeled = false;
	if (_breathFrame >= 0)
	{
		_breathing = false;
		_breathFrame = 0;
	}
}

/**
 * This will increment the walking phase.
 * @param tileBelowMe Pointer to tile currently below the unit.
 * @param cache Refresh the unit cache.
 */
void BattleUnit::keepWalking(Tile *tileBelowMe, bool cache)
{
	int middle, end;
	if (_verticalDirection)
	{
		middle = 4;
		end = 8;
	}
	else
	{
		// diagonal walking takes double the steps
		middle = 4 + 4 * (_direction % 2);
		end = 8 + 8 * (_direction % 2);
		if (_armor->getSize() > 1)
		{
			if (_direction < 1 || _direction > 5)
				middle = end;
			else if (_direction == 5)
				middle = 12;
			else if (_direction == 1)
				middle = 5;
			else
				middle = 1;
		}
	}
	if (!cache)
	{
		_pos = _destination;
		end = 2;
	}

	_walkPhase++;

	if (_walkPhase == middle)
	{
		// we assume we reached our destination tile
		// this is actually a drawing hack, so soldiers are not overlapped by floortiles
		_pos = _destination;
	}

	if (_walkPhase >= end)
	{
		if (_floating && !_tile->hasNoFloor(tileBelowMe))
		{
			_floating = false;
		}
		// we officially reached our destination tile
		_status = STATUS_STANDING;
		_walkPhase = 0;
		_verticalDirection = 0;
		if (_faceDirection >= 0) {
			// Finish strafing move facing the correct way.
			_direction = _faceDirection;
			_faceDirection = -1;
		}

		// motion points calculation for the motion scanner blips
		if (_armor->getSize() > 1)
		{
			_motionPoints += 30;
		}
		else
		{
			// sectoids actually have less motion points
			// but instead of create yet another variable,
			// I used the height of the unit instead (logical)
			if (getStandHeight() > 16)
				_motionPoints += 4;
			else
				_motionPoints += 3;
		}
	}
}

/**
 * Gets the walking phase for animation and sound.
 * @return phase will always go from 0-7
 */
int BattleUnit::getWalkingPhase() const
{
	return _walkPhase % 8;
}

/**
 * Gets the walking phase for diagonal walking.
 * @return phase this will be 0 or 8
 */
int BattleUnit::getDiagonalWalkingPhase() const
{
	return (_walkPhase / 8) * 8;
}

/**
 * Look at a point.
 * @param point Position to look at.
 * @param turret True to turn the turret, false to turn the unit.
 */
void BattleUnit::lookAt(Position point, bool turret)
{
	int dir = directionTo (point);

	if (turret)
	{
		_toDirectionTurret = dir;
		if (_toDirectionTurret != _directionTurret)
		{
			_status = STATUS_TURNING;
		}
	}
	else
	{
		_toDirection = dir;
		if (_toDirection != _direction
			&& _toDirection < 8
			&& _toDirection > -1)
		{
			_status = STATUS_TURNING;
		}
	}
}

/**
 * Look at a direction.
 * @param direction Direction to look at.
 * @param force True to reset the direction, false to animate to it.
 */
void BattleUnit::lookAt(int direction, bool force)
{
	if (!force)
	{
		if (direction < 0 || direction >= 8) return;
		_toDirection = direction;
		if (_toDirection != _direction)
		{
			_status = STATUS_TURNING;
		}
	}
	else
	{
		_toDirection = direction;
		_direction = direction;
	}
}

/**
 * Advances the turning towards the target direction.
 * @param turret True to turn the turret, false to turn the unit.
 */
void BattleUnit::turn(bool turret)
{
	int a = 0;

	if (turret)
	{
		if (_directionTurret == _toDirectionTurret)
		{
			abortTurn();
			return;
		}
		a = _toDirectionTurret - _directionTurret;
	}
	else
	{
		if (_direction == _toDirection)
		{
			abortTurn();
			return;
		}
		a = _toDirection - _direction;
	}

	if (a != 0) {
		if (a > 0) {
			if (a <= 4) {
				if (!turret) {
					if (_turretType > -1)
						_directionTurret++;
					_direction++;
				} else _directionTurret++;
			} else {
				if (!turret) {
					if (_turretType > -1)
						_directionTurret--;
					_direction--;
				} else _directionTurret--;
			}
		} else {
			if (a > -4) {
				if (!turret) {
					if (_turretType > -1)
						_directionTurret--;
					_direction--;
				} else _directionTurret--;
			} else {
				if (!turret) {
					if (_turretType > -1)
						_directionTurret++;
					_direction++;
				} else _directionTurret++;
			}
		}
		if (_direction < 0) _direction = 7;
		if (_direction > 7) _direction = 0;
		if (_directionTurret < 0) _directionTurret = 7;
		if (_directionTurret > 7) _directionTurret = 0;
	}

	if (turret)
	{
		 if (_toDirectionTurret == _directionTurret)
		 {
			// we officially reached our destination
			_status = STATUS_STANDING;
		 }
	}
	else if (_toDirection == _direction || _status == STATUS_UNCONSCIOUS)
	{
		// we officially reached our destination
		_status = STATUS_STANDING;
	}
}

/**
 * Stops the turning towards the target direction.
 */
void BattleUnit::abortTurn()
{
	_status = STATUS_STANDING;
}


/**
 * Gets the soldier's gender.
 */
SoldierGender BattleUnit::getGender() const
{
	return _gender;
}

/**
 * Returns the unit's faction.
 * @return Faction. (player, hostile or neutral)
 */
UnitFaction BattleUnit::getFaction() const
{
	return _faction;
}

/**
 * Gets values used for recoloring sprites.
 * @param i what value choose.
 * @return Pairs of value, where first is color group to replace and second is new color group with shade.
 */
const std::vector<std::pair<Uint8, Uint8> > &BattleUnit::getRecolor() const
{
	return _recolor;
}

/**
 * Gets the unit's armor color.
 * @return The armor color.
 */
int BattleUnit::getArmorColor() const
{
	return _armorColor;
}

/**
 * Kneel down.
 * @param kneeled to kneel or to stand up
 */
void BattleUnit::kneel(bool kneeled)
{
	_kneeled = kneeled;
}

/**
 * Is kneeled down?
 * @return true/false
 */
bool BattleUnit::isKneeled() const
{
	return _kneeled;
}

/**
 * Is floating? A unit is floating when there is no ground under him/her.
 * @return true/false
 */
bool BattleUnit::isFloating() const
{
	return _floating;
}

/**
 * Aim. (shows the right hand sprite and weapon holding)
 * @param aiming true/false
 */
void BattleUnit::aim(bool aiming)
{
	if (aiming)
		_status = STATUS_AIMING;
	else
		_status = STATUS_STANDING;
}

/**
 * Returns the direction from this unit to a given point.
 * @param point given position.
 * @return direction.
 */
int BattleUnit::directionTo(Position point) const
{
	double ox = point.x - _pos.x;
	double oy = point.y - _pos.y;
	double angle = atan2(ox, -oy);
	// divide the pie in 4 angles each at 1/8th before each quarter
	double pie[4] = {(M_PI_4 * 4.0) - M_PI_4 / 2.0, (M_PI_4 * 3.0) - M_PI_4 / 2.0, (M_PI_4 * 2.0) - M_PI_4 / 2.0, (M_PI_4 * 1.0) - M_PI_4 / 2.0};
	int dir = 0;

	if (angle > pie[0] || angle < -pie[0])
	{
		dir = 4;
	}
	else if (angle > pie[1])
	{
		dir = 3;
	}
	else if (angle > pie[2])
	{
		dir = 2;
	}
	else if (angle > pie[3])
	{
		dir = 1;
	}
	else if (angle < -pie[1])
	{
		dir = 5;
	}
	else if (angle < -pie[2])
	{
		dir = 6;
	}
	else if (angle < -pie[3])
	{
		dir = 7;
	}
	else if (angle < pie[0])
	{
		dir = 0;
	}
	return dir;
}

/**
 * Returns the soldier's amount of time units.
 * @return Time units.
 */
int BattleUnit::getTimeUnits() const
{
	return _tu;
}

/**
* Returns the soldier's amount of spendable time units.
* @return Available time units.
*/
int BattleUnit::getAvailableTimeUnits(bool movement) const
{
	return movement ? _tu : (_tu > 0 ? (_tu + (_stats.tu / 5)) : 0);
}

/**
* Returns the soldier's amount of time unit debt.
* @return Time unit debt.
*/
int BattleUnit::getTimeUnitDebt() const
{
	return _tu < 0 ? _tu : 0;
}

/**
 * Returns the soldier's amount of energy.
 * @return Energy.
 */
int BattleUnit::getEnergy() const
{
	return _energy;
}

/**
 * Returns the soldier's amount of health.
 * @return Health.
 */
int BattleUnit::getHealth() const
{
	return _health;
}

/**
 * Returns the soldier's amount of morale.
 * @return Morale.
 */
int BattleUnit::getMorale() const
{
	return _morale;
}

/**
 * Get overkill damage to unit.
 * @return Damage over normal health.
 */
int BattleUnit::getOverKillDamage() const
{
	return std::max(-_health - (int)(_stats.health * _armor->getOverKill()), 0);
}

/**
* Helper function for setting value with max bound.
* @return The actual applied delta.
*/
static inline int setValueMax(int& value, int diff, int min, int max)
{
	int newValue = Clamp(value + diff, min, max);
	int delta = newValue - value;
	value = newValue;
	return delta;
}

/**
 * Do an amount of damage.
 * @param relative The relative position of which part of armor and/or bodypart is hit.
 * @param power The amount of damage to inflict.
 * @param type The type of damage being inflicted.
 * @param ignoreArmor Should the damage ignore armor resistance?
 * @return damage done after adjustment
 */
int BattleUnit::damage(TileEngine* tiles, BattleUnit *source, Position relative, int power, const RuleDamageType *type, SavedBattleGame *save, BattleActionAttack attack, UnitSide sideOverride, UnitBodyPart bodypartOverride, bool ignoreArmor, int *wounds, const std::string &displaySource)
{
	UnitSide side = SIDE_FRONT;
	UnitBodyPart bodypart = BODYPART_TORSO;

	if(wounds)
	{
		(*wounds) = 0;
	}

	if (power <= 0)
	{
		return 0;
	}

	power = reduceByResistance(power, type->ResistType);

	if (!ignoreArmor && !type->IgnoreDirection)
	{
		if (relative == Position(0, 0, 0))
		{
			side = SIDE_UNDER;
		}
		else
		{
			int relativeDirection;
			const int abs_x = abs(relative.x);
			const int abs_y = abs(relative.y);
			if (abs_y > abs_x * 2)
				relativeDirection = 8 + 4 * (relative.y > 0);
			else if (abs_x > abs_y * 2)
				relativeDirection = 10 + 4 * (relative.x < 0);
			else
			{
				if (relative.x < 0)
				{
					if (relative.y > 0)
						relativeDirection = 13;
					else
						relativeDirection = 15;
				}
				else
				{
					if (relative.y > 0)
						relativeDirection = 11;
					else
						relativeDirection = 9;
				}
			}

			switch((relativeDirection - _direction) % 8)
			{
			case 0:	side = SIDE_FRONT; 										break;
			case 1:	side = RNG::generate(0,2) < 2 ? SIDE_FRONT:SIDE_RIGHT; 	break;
			case 2:	side = SIDE_RIGHT; 										break;
			case 3:	side = RNG::generate(0,2) < 2 ? SIDE_REAR:SIDE_RIGHT; 	break;
			case 4:	side = SIDE_REAR; 										break;
			case 5:	side = RNG::generate(0,2) < 2 ? SIDE_REAR:SIDE_LEFT; 	break;
			case 6:	side = SIDE_LEFT; 										break;
			case 7:	side = RNG::generate(0,2) < 2 ? SIDE_FRONT:SIDE_LEFT; 	break;
			}
			if (relative.z >= getHeight())
			{
				bodypart = BODYPART_HEAD;
			}
			else if (relative.z > 4)
			{
				switch(side)
				{
				case SIDE_LEFT:		bodypart = BODYPART_LEFTARM; break;
				case SIDE_RIGHT:	bodypart = BODYPART_RIGHTARM; break;
				default:			bodypart = BODYPART_TORSO;
				}
			}
			else
			{
				switch(side)
				{
				case SIDE_LEFT: 	bodypart = BODYPART_LEFTLEG; 	break;
				case SIDE_RIGHT:	bodypart = BODYPART_RIGHTLEG; 	break;
				default:
					bodypart = (UnitBodyPart) RNG::generate(BODYPART_RIGHTLEG,BODYPART_LEFTLEG);
				}
			}
		}
	}

	const int orgPower = power;
	const int overKillMinimum = type->IgnoreOverKill ? getDeathHealth() : -4 * _stats.health;

	{
		ModScript::HitUnit::Output args { power, bodypart, side, };
		ModScript::HitUnit::Worker work { this, attack.damage_item, attack.weapon_item, attack.attacker, save, orgPower, type->ResistType, attack.type, };

		work.execute(this->getArmor()->getScript<ModScript::HitUnit>(), args);

		power = args.getFirst();
		bodypart = (UnitBodyPart)args.getSecond();
		side = (UnitSide)args.getThird();
		if (bodypart >= BODYPART_MAX)
		{
			bodypart = {};
		}
		if (side >= SIDE_MAX)
		{
			side = {};
		}
	}

	// side and bodypart overrides (used by environmental conditions only)
	if (sideOverride != SIDE_MAX)
	{
		side = sideOverride;
	}
	if (bodypartOverride != BODYPART_MAX)
	{
		bodypart = bodypartOverride;
	}

	// TODO: Reimplement armor smashed/penetrated.
	// TODO: Reimplement psionic percentage-based damage.
	// TODO: Capture wounds in *wounds parameter.

	/*int armor = getArmor(side);
	int originalPower = power;
	power -= armor;

	// armor damage
	if(type != DT_PSYCHIC && type != DT_STUN)
	{
		if(power > 0)
		{
			// Damage was done
			if(power > armor)
			{
				// Armor was smashed through. Armor takes heavy damage for every point of original damage past armor*2.
				setArmor(armor - (power - armor) - power * 0.2 - (originalPower - armor * 0.5) * 0.05 - 1, side);
			}
			else
			{
				// Armor was penetrated cleanly. Armor takes significant extra damage for every point of original damage past armor.
				setArmor(armor - power * 0.2 - (originalPower - armor * 0.5) * 0.05 - 1, side);
			}
		}
		else if(originalPower >= (armor * 0.5))
		{
			// Damage was prevented, but armor is still reduced slightly for every point of original damage past armor/2.
			setArmor(armor - (originalPower - armor * 0.5) * 0.05 - 1, side);
		}
	}
	}

	if (power > 0)
	{
		if (type == DT_STUN)
		{
			int effectivePower = std::min(_stats.health, power);
			_stunlevel += power;

			if(tiles)
			{
				tiles->displayDamage(source, this, BA_NONE, type, 0, 0, power);
			}

			//if(BattleUnit *controller = getControlledBy())
			//{
			//	effectivePower = controller->getBaseStats()->health * (effectivePower / _stats.health);
			//	controller->damage(tiles, this, Position(), RNG::generate(effectivePower / 4, effectivePower / 2), DT_STUN, true);
			//}
		}
		else
		{
			if (type == DT_PSYCHIC)
			{
				// Psychic damage is percentage-based.
				int moraleDamage = power * (1.0 - _stats.bravery / 110.0) * 2.0;
				if (moraleDamage > 0)
				{
					moraleChange(-moraleDamage);
				}
				power = (int)(_stats.health * power / 100.0);
			}
			//if(type == DT_PSYCHIC)
			//{
			//	double damageRatio = (_stats.psiStrength / 200.0) + RNG::generate(0.0, 0.25);
			//	int stun = power * (1.0 - damageRatio);

			//	_stunlevel += stun;

			//	power *= damageRatio;
			//}
			
			int newWounds = 0;
			int additionalStun = 0;
			//int effectivePower = std::min(_stats.health, power);
			_health -= power;

			if (type != DT_IN && type != DT_PSYCHIC)
			{
				if (_armor->getDamageModifier(DT_STUN) > 0.0)
				{
					// conventional weapons can cause additional stun damage
					_stunlevel += (additionalStun = RNG::generate(0, power / 4));
				}
				// fatal wounds
				if (isWoundable())
				{
					if (RNG::generate(0, 10) < power)
					{
						newWounds = RNG::generate(1,3);
						if(wounds)
						{
							(*wounds) = newWounds;
						}
						_fatalWounds[bodypart] += newWounds;
					}

					if (_fatalWounds[bodypart])
						moraleChange(-_fatalWounds[bodypart]);
				}
			}
			//else if (type == DT_PSYCHIC)
			//{
			//	// Apply morale loss when receiving psychic damage.
			//	int moraleDamage = (int)((1 - (getBaseStats()->bravery / 110.0)) * effectivePower * 2);
			//	moraleChange(-moraleDamage);
			//}*/

	{
		constexpr int toHealth = 0;
		constexpr int toArmor = 1;
		constexpr int toStun = 2;
		constexpr int toTime = 3;
		constexpr int toEnergy = 4;
		constexpr int toMorale = 5;
		constexpr int toWound = 6;

		ModScript::DamageUnit::Output args { };

		std::get<toArmor>(args.data) += type->getArmorPreDamage(power);

		if (type->ArmorEffectiveness > 0.0f)
		{
			power -= getArmor(side) * type->ArmorEffectiveness;
		}

		if (power > 0)
		{
			// stun level change
			std::get<toStun>(args.data) += type->getStunDamage(power);

			// morale change
			std::get<toMorale>(args.data) += type->getMoraleDamage(power);

			// time units change
			std::get<toTime>(args.data) += type->getTimeDamage(power);

			// health change
			std::get<toHealth>(args.data) += type->getHealthDamage(power);

			// energy change
			std::get<toEnergy>(args.data) += type->getEnergyDamage(power);

			// fatal wounds change
			std::get<toWound>(args.data) += type->getWoundDamage(power);

			// armor value change
			std::get<toArmor>(args.data) += type->getArmorDamage(power);
		}

		ModScript::DamageUnit::Worker work { this, attack.damage_item, attack.weapon_item, attack.attacker, save, power, orgPower, bodypart, side, type->ResistType, attack.type, };

		work.execute(this->getArmor()->getScript<ModScript::DamageUnit>(), args);

		int stunDamage = 0;
		if (!_armor->getPainImmune() || type->IgnorePainImmunity)
		{
			setValueMax(_stunlevel, stunDamage = std::get<toStun>(args.data), 0, 4 * _stats.health);
		}

		int moraleDamage = moraleChange(-reduceByBravery(std::get<toMorale>(args.data)));

		int tuDamage = setValueMax(_tu, -std::get<toTime>(args.data), 0, _stats.tu);

		int fullHealthDamage = -std::get<toHealth>(args.data);
		int healthDamage = setValueMax(_health, -std::get<toHealth>(args.data), overKillMinimum, _stats.health);

		int energyDamage = setValueMax(_energy, -std::get<toEnergy>(args.data), 0, _stats.stamina);

		int newWounds = 0;
		if (isWoundable())
		{
			_fatalWounds[bodypart] += (newWounds = std::get<toWound>(args.data));
			moraleDamage += moraleChange(-std::get<toWound>(args.data));
		}

		setValueMax(_currentArmor[side], -std::get<toArmor>(args.data), 0, _armor->getArmor(side));

		if (_health <= 0 && !checkStartBleedout())
		{
			_health = 0;
		}

		if (tiles)
		{
			// TODO: Rework displaying damage based on multiple damage bits done.
			tiles->displayDamage(source, this, BA_NONE, type, fullHealthDamage, newWounds, displaySource);
		}

		setFatalShotInfo(side, bodypart);
		return std::get<toHealth>(args.data);
	}

	/*if(BattleUnit *controller = getControlledBy())
	{
		//effectivePower = controller->getBaseStats()->health * (effectivePower / _stats.health);
		//if(int damageBacklash = RNG::generate((int)(effectivePower * 0.5), effectivePower))
		if (int damageBacklash = RNG::generate())
		{
			controller->damage(tiles, this, Position(), damageBacklash, DT_PSYCHIC, true);
		}
	}*/

	//setFatalShotInfo(side, bodypart);

	//return power < 0 ? 0:power;
}

bool BattleUnit::checkStartBleedout()
{
	if(getCanBleedOut())
	{
		if(!_bleedingOut)
		{
			_bleedingOut = true;
			_fatalWounds[BODYPART_TORSO] += 5;
		}
		return true;
	}

	return false;
}

/**
 * Do an amount of stun recovery.
 * @param power
 */
void BattleUnit::healStun(int power)
{
	_stunlevel -= power;
	if (_stunlevel < 0) _stunlevel = 0;
}

int BattleUnit::getStunlevel() const
{
	return _stunlevel;
}

/**
 * Raises a unit's stun level sufficiently so that the unit is ready to become unconscious.
 * Used when another unit falls on top of this unit.
 * Zombified units first convert to their spawn unit.
 * @param battle Pointer to the battlescape game.
 */
void BattleUnit::knockOut(BattlescapeGame *battle)
{
	if (!_spawnUnit.empty())
	{
		setRespawn(false);
		BattleUnit *newUnit = battle->convertUnit(this);
		newUnit->knockOut(battle);
	}
	else
	{
		_stunlevel = std::max(_health, 1);
	}
}

/**
 * Initialises the falling sequence. Occurs after death or stunned.
 */
void BattleUnit::startFalling()
{
	clearOngoingAction();
	_status = STATUS_COLLAPSING;
	_fallPhase = 0;
}

/**
 * Advances the phase of falling sequence.
 */
void BattleUnit::keepFalling()
{
	_fallPhase++;
	if (_fallPhase == _armor->getDeathFrames())
	{
		_fallPhase--;
		if (_health <= getDeathHealth())
		{
			die();
		}
		else
			_status = STATUS_UNCONSCIOUS;
	}
}


/**
 * Returns the phase of the falling sequence.
 * @return phase
 */
int BattleUnit::getFallingPhase() const
{
	return _fallPhase;
}

/**
 * Returns whether the soldier is out of combat, dead or unconscious.
 * A soldier that is out, cannot perform any actions, cannot be selected, but it's still a unit.
 * @return flag if out or not.
 */
bool BattleUnit::isOut() const
{
	return _status == STATUS_DEAD || _status == STATUS_UNCONSCIOUS || _status == STATUS_IGNORE_ME;
}

/**
 * Get the number of time units a certain action takes.
 * @param actionType
 * @param item
 * @return TUs
 */
RuleItemUseCost BattleUnit::getActionTUs(BattleActionType actionType, BattleItem *item) const
{
	if (item == 0)
	{
		return 0;
	}

	const RuleItem *rule = item->getRules();

	RuleItemUseCost cost;
	RuleItemUseCost flat;
	switch (actionType)
	{
		case BA_PRIME:
			flat = rule->getFlatPrime();
			cost = rule->getCostPrime();
			break;
		case BA_UNPRIME:
			flat = rule->getFlatUnprime();
			cost = rule->getCostUnprime();
			break;
		case BA_THROW:
			flat = rule->getFlatThrow();
			cost = rule->getCostThrow();
			break;
		case BA_AUTOSHOT:
			flat = rule->getFlatAuto();
			cost = rule->getCostAuto();
			break;
		case BA_BURSTSHOT:
			flat = rule->getFlatBurst();
			cost = rule->getCostBurst();
			break;
		case BA_SNAPSHOT:
			flat = rule->getFlatSnap();
			cost = rule->getCostSnap();
			break;
		case BA_HIT:
		case BA_EXECUTE:
			flat = rule->getFlatMelee();
			cost = rule->getCostMelee();
			break;
		case BA_LAUNCH:
		case BA_AIMEDSHOT:
			flat = rule->getFlatAimed();
			cost = rule->getCostAimed();
			break;
		case BA_USE:
			cost = rule->getCostUse();
			break;
		case BA_MINDCONTROL:
			cost = rule->getCostMind();
			break;
		case BA_PANIC:
			cost = rule->getCostPanic();
			break;
		case BA_CLAIRVOYANCE:
			// TODO: Implement custom cost for clairvoyance.
			cost = rule->getCostUse();
		case BA_MINDBLAST:
			// TODO: Implement custom cost for mind blast.
			cost = rule->getCostUse();
			break;
		case BA_RELOAD:
			cost = getReloadCost(item);
			break;
		case BA_OVERWATCH:
			//int overwatchCost = std::max(getActionTUs(getOverwatchShotAction(rule), item), (int)(floor(getBaseStats()->tu * 0.9f)));
			//return onOverwatch() ? 0: overwatchCost;
			if (onOverwatch())
			{
				cost.Time = 0;
			}
			else
			{
				cost.Time = 90;
			}
			break;

		case BA_DUALFIRE:
			BattleItem *weapon1;
			BattleActionType action1;
			int tu1;
			BattleItem *weapon2;
			BattleActionType action2;
			int tu2;
			// TODO: Fix dualfire cost calculations.
			if(canDualFire(weapon1, action1, tu1, weapon2, action2, tu2))
			{
				cost = std::min((int)(std::max(tu1, tu2) * 1.1f), 96);
			}
			else
			{
				return 0;
			}
			break;
		default:
			cost = 0;
	}

	// if it's a percentage, apply it to unit TUs
	if (!flat.Time && cost.Time)
	{
		cost.Time = std::max(1, (int)floor(getBaseStats()->tu * cost.Time / 100.0f));
	}
	// if it's a percentage, apply it to unit Energy
	if (!flat.Energy && cost.Energy)
	{
		cost.Energy = std::max(1, (int)floor(getBaseStats()->stamina * cost.Energy / 100.0f));
	}
	// if it's a percentage, apply it to unit Morale
	if (!flat.Morale && cost.Morale)
	{
		cost.Morale = std::max(1, (int)floor((110 - getBaseStats()->bravery) * cost.Morale / 100.0f));
	}
	// if it's a percentage, apply it to unit Health
	if (!flat.Health && cost.Health)
	{
		cost.Health = std::max(1, (int)floor(getBaseStats()->health * cost.Health / 100.0f));
	}
	// if it's a percentage, apply it to unit Health
	if (!flat.Stun && cost.Stun)
	{
		cost.Stun = std::max(1, (int)floor(getBaseStats()->health * cost.Stun / 100.0f));
	}

	return cost;
}


/**
 * Spend time units if it can. Return false if it can't.
 * @param tu
 * @return flag if it could spend the time units or not.
 */
bool BattleUnit::spendTimeUnits(int tu, bool allowDebt, bool forOverwatch)
{
	if (tu == 0 || (_tu > 0 && (_tu - tu) >= (-_stats.tu * (allowDebt ? Game::getMod()->getMaxTUDebt() : 0) / 100)))
	{
		_tu -= tu;
		if(!forOverwatch && tu)
		{
			clearOverwatch();
		}
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Checks if the unit could spend the specified time units.
 * @param tu
 * @return flag if it could spend the time units or not.
 */
bool BattleUnit::canSpendTimeUnits(int tu, bool allowDebt, bool forOverwatch)
{
	return tu == 0 || (_tu > 0 && (_tu - tu) >= (-_stats.tu * (allowDebt ? Game::getMod()->getMaxTUDebt() : 0) / 100));
}

/**
 * Spend energy  if it can. Return false if it can't.
 * @param energy
 * @return flag if it could spend the time units or not.
 */
bool BattleUnit::spendEnergy(int energy)
{
	if (energy <= _energy)
	{
		_energy -= energy;
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Spend resources cost without checking.
 * @param cost
 */
void BattleUnit::spendCost(const RuleItemUseCost& cost)
{
	_tu -= cost.Time;
	_energy -= cost.Energy;
	_morale -= cost.Morale;
	_health -= cost.Health;
	_stunlevel += cost.Stun;
}

/**
 * Set a specific number of timeunits.
 * @param tu
 */
void BattleUnit::setTimeUnits(int tu)
{
	_tu = std::max(std::min(tu, _stats.tu), 0);
}

/**
 * Add this unit to the list of visible units. Returns true if this is a new one.
 * @param unit
 * @return
 */
bool BattleUnit::addToVisibleUnits(BattleUnit *unit)
{
	bool add = true;
	for (std::vector<BattleUnit*>::iterator i = _unitsSpottedThisTurn.begin(); i != _unitsSpottedThisTurn.end();++i)
	{
		if ((BattleUnit*)(*i) == unit)
		{
			add = false;
			break;
		}
	}
	if (add)
	{
		_unitsSpottedThisTurn.push_back(unit);
	}
	for (std::vector<BattleUnit*>::iterator i = _visibleUnits.begin(); i != _visibleUnits.end(); ++i)
	{
		if ((BattleUnit*)(*i) == unit)
		{
			return false;
		}
	}
	_visibleUnits.push_back(unit);
	return true;
}

/**
* Removes the given unit from the list of visible units.
* @param unit The unit to add to our visibility cache.
* @return true if unit found and removed
*/
bool BattleUnit::removeFromVisibleUnits(BattleUnit *unit)
{
	if (!_visibleUnits.size()) {
		return false;
	}
	std::vector<BattleUnit*>::iterator i = std::find(_visibleUnits.begin(), _visibleUnits.end(), unit);
	if (i == _visibleUnits.end())
	{
		return false;
	}
	//Slow to remove stuff from vector as it shuffles all the following items. Swap in rearmost element before removal.
	(*i) = *(_visibleUnits.end() - 1);
	_visibleUnits.pop_back();
	return true;
}

/**
* Checks if the given unit is on the list of visible units.
* @param unit The unit to check whether we have in our visibility cache.
* @return true if on the visible list or of the same faction
*/
bool BattleUnit::hasVisibleUnit(BattleUnit *unit)
{
	if (getFaction() == unit->getFaction())
	{
		//Units of same faction are always visible, but not stored in the visible unit list
		return true;
	}
	return std::find(_visibleUnits.begin(), _visibleUnits.end(), unit) != _visibleUnits.end();
}

/**
 * Get the pointer to the vector of visible units.
 * @return pointer to vector.
 */
std::vector<BattleUnit*> *BattleUnit::getVisibleUnits()
{
	return &_visibleUnits;
}

/**
 * Clear visible units.
 */
void BattleUnit::clearVisibleUnits()
{
	_visibleUnits.clear();
}

/**
 * Add this unit to the list of visible tiles. Returns true if this is a new one.
 * @param tile
 * @return
 */
bool BattleUnit::addToVisibleTiles(Tile *tile, bool nightVision)
{
	_hadNightVision = _hadNightVision || nightVision;
	if (_visibleTilesLookup.insert(tile).second)
	{
		if (_faction == FACTION_PLAYER)
		{
			tile->changeVisibleCounts(+1, _hadNightVision ? +1 : 0);
		}
		_visibleTiles.push_back(tile);
		return true;
	}
	return false;
}

bool BattleUnit::hasVisibleTile(Tile *tile) const
{
	return _visibleTilesLookup.find(tile) != _visibleTilesLookup.end(); //find?
}

/**
 * Get the pointer to the vector of visible tiles.
 * @return pointer to vector.
 */
const std::vector<Tile*> *BattleUnit::getVisibleTiles()
{
	return &_visibleTiles;
}

/**
 * Clears visible tiles. Also reduces the associated visibility counter used by the AI.
 */
void BattleUnit::clearVisibleTiles()
{
	if (_faction == FACTION_PLAYER)
	{
		for (std::vector<Tile*>::iterator j = _visibleTiles.begin(); j != _visibleTiles.end(); ++j)
		{
			(*j)->changeVisibleCounts(-1, _hadNightVision ? -1 : 0);
		}
	}

	_visibleTilesLookup.clear();
	_visibleTiles.clear();
	_hadNightVision = false;
}

/**
 * Get accuracy of different types of psi attack.
 * @param actionType Psi attack type.
 * @param item Psi-Amp.
 * @return Attack bonus.
 */
int BattleUnit::getPsiAccuracy(BattleActionType actionType, BattleItem *item)
{
	int psiAcc = 0;
	switch (actionType)
	{
	case BA_MINDCONTROL:
		psiAcc = item->getRules()->getAccuracyMind();
		break;
	case BA_PANIC:
		psiAcc = item->getRules()->getAccuracyPanic();
		break;
	case BA_CLAIRVOYANCE:
		return 100;
		break;
	case BA_MINDBLAST:
		// TODO: Accuracy for Mind Blast?
		return 100;
		break;
	case BA_USE:
		psiAcc = item->getRules()->getAccuracyUse();
	}

	psiAcc += item->getRules()->getAccuracyMultiplier(this);

	return psiAcc;
}

/**
 * Calculate firing accuracy.
 * Formula = accuracyStat * weaponAccuracy * kneelingbonus(1.15) * one-handPenalty(0.8) * woundsPenalty(% health) * critWoundsPenalty (-10%/wound)
 * @param actionType
 * @param item
 * @return firing Accuracy
 */
int BattleUnit::getFiringAccuracy(BattleActionType actionType, BattleItem *item, const Mod *mod, bool useShotgun, bool dualFiring)
{
	int weaponAcc = item->getRules()->getAccuracySnap();
	int shotgunAcc = item->getRules()->getShotgunSpread();

	if(actionType == BA_OVERWATCH)
	{
		actionType = getOverwatchShotAction(item->getRules());
	}

	bool kneeled = _kneeled;

	if(useShotgun && shotgunAcc)
	{
		weaponAcc = shotgunAcc;
	}
	else if (actionType == BA_AIMEDSHOT || actionType == BA_LAUNCH)
		weaponAcc = item->getRules()->getAccuracyAimed();
	else if(actionType == BA_AUTOSHOT)
		weaponAcc = item->getRules()->getAccuracyAuto();
	else if(actionType == BA_BURSTSHOT)
		weaponAcc = item->getRules()->getAccuracyBurst();
	else if (actionType == BA_HIT)
	{
		return item->getRules()->getMeleeMultiplier(this) * item->getRules()->getAccuracyMelee() * getAccuracyModifier(item) / 100;
	}
	else if(actionType == BA_DUALFIRE)
	{
		BattleItem *weapon1;
		BattleItem *weapon2;
		BattleActionType action1;
		BattleActionType action2;
		if (canDualFire(weapon1, action1, weapon2, action2))
		{
			return (getFiringAccuracy(action1, weapon1, mod, false, true) + getFiringAccuracy(action2, weapon2, mod, false, true)) / 2.0;
		}
		else
		{
			return 0;
		}
	}
	else if (actionType == BA_CQB)
	{
		kneeled = false;
		return item->getRules()->getCloseQuartersMultiplier(this) * item->getRules()->getAccuracyCloseQuarters(mod) / 100;
	}

	if(dualFiring)
	{
		weaponAcc *= getDualFireAccuracy();
	}

	int result = getBaseStats()->firing * weaponAcc / 100;

	int kneelMod = item->getRules()->getKneelBonus(mod);
	// Grant aliens special kneel bonuses since they can't kneel properly themselves.
	if (kneeled || (getFaction() == FACTION_HOSTILE && kneelMod > 115))
	{
		result = kneelMod ? result * kneelMod / 100 : result * 115 / 100;
	}

	if (item->getRules()->isTwoHanded())
	{
		// two handed weapon, means one hand should be empty
		if (getItem(_weaponSlot1) != 0 && getItem(_weaponSlot2) != 0)
		{
			result = result * item->getRules()->getOneHandedPenalty(mod) / 100;
		}
	}

	return result * getAccuracyModifier(item) / 100;
}

/**
 * To calculate firing accuracy. Takes health and fatal wounds into account.
 * Formula = accuracyStat * woundsPenalty(% health) * critWoundsPenalty (-10%/wound)
 * @param item the item we are shooting right now.
 * @return modifier
 */
int BattleUnit::getAccuracyModifier(BattleItem *item)
{
	int wounds = _fatalWounds[BODYPART_HEAD];

	if (item)
	{
		if (item->getRules()->isTwoHanded())
		{
			wounds += _fatalWounds[BODYPART_RIGHTARM] + _fatalWounds[BODYPART_LEFTARM];
		}
		else
		{
			if (getItem("STR_RIGHT_HAND") == item)
			{
				wounds += _fatalWounds[BODYPART_RIGHTARM];
			}
			else
			{
				wounds += _fatalWounds[BODYPART_LEFTARM];
			}
		}
	}
	return std::max(10, 25 * _health / getBaseStats()->health + 75 + -10 * wounds);
}

/**
 * Calculate throwing accuracy.
 * @return throwing Accuracy
 */
double BattleUnit::getThrowingAccuracy()
{
	return (double)(getBaseStats()->throwing * getAccuracyModifier()) / 100.0;
}

/**
 * Set the armor value of a certain armor side.
 * @param armor Amount of armor.
 * @param side The side of the armor.
 */
void BattleUnit::setArmor(int armor, UnitSide side)
{
	if (armor < 0)
	{
		armor = 0;
	}
	_currentArmor[side] = armor;
}

/**
 * Get the armor value of a certain armor side.
 * @param side The side of the armor.
 * @return Amount of armor.
 */
int BattleUnit::getArmor(UnitSide side) const
{
	return _currentArmor[side];
}

/**
 * Get the max armor value of a certain armor side.
 * @param side The side of the armor.
 * @return Amount of armor.
 */
int BattleUnit::getMaxArmor(UnitSide side) const
{
	return _maxArmor[side];
}

/**
 * Get total amount of fatal wounds this unit has.
 * @return Number of fatal wounds.
 */
int BattleUnit::getFatalWounds() const
{
	int sum = 0;
	for (int i = 0; i < BODYPART_MAX; ++i)
		sum += _fatalWounds[i];
	return sum;
}

/**
 * Little formula to calculate reaction score.
 * @return Reaction score.
 */
double BattleUnit::getReactionScore(bool overwatch) const
{
	//(Reactions Stat) x (Current Time Units / Max TUs)
	//double tuRatio = (overwatch || _movementAction == BAM_SNEAK) ? 1.0 : (double)getTimeUnits() / (double)getBaseStats()->tu;

	double reactions = getBaseStats()->reactions * getWeaponReactionsModifier(overwatch);

	if (!overwatch)
	{
		reactions *= (double)getTimeUnits() / (double)getBaseStats()->tu;
	}

	return reactions;

	//return (double)getBaseStats()->reactions * tuRatio * (defending ? (_movementAction == BAM_RUN ? 0.2 : 1.0) : getWeaponReactionsModifier(overwatch));
}

/**
* Get the current evasion score.
* @return Evasion score.
*/
double BattleUnit::getEvasionScore()
{
	double reactions = getBaseStats()->reactions;
	double tuRatio = (double)getTimeUnits() / (double)getBaseStats()->tu;

	switch (_movementAction)
	{
	case BAM_RUN:
		// Half reaction score * TU ratio
		reactions = reactions * 0.5 * tuRatio;
		break;
	case BAM_SNEAK:
		// Always use full reaction score
		//reactions *= 1.25;
		break;
	default:
		// Reaction score * TU ratio
		reactions *= tuRatio;
	}

	return reactions;
}

/**
 * Returns this unit's main hand weapon's reactions modifier as a multiplier.
 * @return Reactions modifier.
 */
double BattleUnit::getWeaponReactionsModifier(bool overwatch) const
{
	BattleItem *weapon = overwatch ? getOverwatchWeapon() : getMainHandWeapon();

	if(weapon)
	{
		const RuleItem *rules = weapon->getRules();
		if(rules)
		{
			double modifier = overwatch ? rules->getOverwatchModifier() : rules->getReactionsModifier();
			return modifier / 100.0;
		}
	}

	return 1.0;
}

/**
* Helper function preparing Time Units recovery at beginning of turn.
* @param tu New time units for this turn.
*/
void BattleUnit::prepareTimeUnits(int tu)
{
	// recover TUs
	int TURecovery = tu;
	float encumbrance = (float)getBaseStats()->strength / (float)getCarriedWeight();
	if (!isVehicle() && encumbrance < 1)
	{
		TURecovery = int(encumbrance * TURecovery);
	}
	// Each fatal wound to the left or right leg reduces the soldier's TUs by 10%.
	int legWounds = _fatalWounds[BODYPART_LEFTLEG] + _fatalWounds[BODYPART_RIGHTLEG];
	if (legWounds)
	{
		TURecovery *= std::min((1.0f - (0.1f * (float)legWounds)), 0.5f);
	}
	//TURecovery -= (TURecovery * ((_fatalWounds[BODYPART_LEFTLEG]+_fatalWounds[BODYPART_RIGHTLEG]) * 10))/100;

	switch (getOngoingAction())
	{
	case BA_OVERWATCH:
	{
		if (BattleItem *weapon = getOverwatchWeapon())
		{
			RuleItemUseCost overwatchShotCost = getActionTUs(getOverwatchShotAction(weapon->getRules()), weapon);
			int overwatchCost = std::max(overwatchShotCost.Time, (int)(floor(getBaseStats()->tu * 0.9f)));
			_overwatchShotsAvailable = TURecovery / overwatchShotCost.Time;

			if (overwatchShotCost.Time <= TURecovery && _overwatchShotsAvailable > 0)
			{
				TURecovery -= overwatchCost;
			}
			else
			{
				clearOverwatch();
			}
		}
		else
		{
			clearOverwatch();
		}
	}
	break;

	case BA_MINDCONTROL:
		BattleItem *weapon;
		if (!(((weapon = getItem(_weaponSlot1)) && weapon->getRules()->getBattleType() == BT_PSIAMP)
			|| ((weapon = getItem(_weaponSlot2)) && weapon->getRules()->getBattleType() == BT_PSIAMP)))
		{
			clearControlling();
		}
		else
		{
			TURecovery *= 0.10;
		}

		break;
	}

	setTimeUnits(_tu + TURecovery);

	// recover energy
	if (!isOut())
	{
		int ENRecovery;
		if (_geoscapeSoldier != 0)
		{
			if (!_geoscapeSoldier->isVehicle())
			{
				//ENRecovery = _geoscapeSoldier->getInitStats()->tu / 3;
				ENRecovery = getBaseStats()->stamina / 3;
			}
			else
			{
				// TODO: Improve vehicle energy recovery.
				ENRecovery = getBaseStats()->stamina;
			}
		}
		else
		{
			ENRecovery = _unitRules->getEnergyRecovery();
		}
		// Each fatal wound to the body reduces the soldier's energy recovery by 10%.
		//ENRecovery -= (_energy * (_fatalWounds[BODYPART_TORSO] * 10))/100;
		ENRecovery *= (1 - ((_fatalWounds[BODYPART_TORSO] * 10) / 100));
		_energy += ENRecovery;
		if (_energy > getBaseStats()->stamina)
			_energy = getBaseStats()->stamina;
	}
}

/**
 * Helper function preparing Energy recovery at beginning of turn.
 * @param energy Energy grain this turn.
 */
void BattleUnit::prepareEnergy(int energy)
{
	if (!isOut())
	{
		// Each fatal wound to the body reduces the soldier's energy recovery by 10%.
		energy -= (_energy * (_fatalWounds[BODYPART_TORSO] * 10)) / 100;
		_energy += energy;
		if (_energy > getBaseStats()->stamina)
			_energy = getBaseStats()->stamina;
		else if (_energy < 0)
			_energy = 0;
	}
}

/**
 * Helper function preparing Health recovery at beginning of turn.
 * @param health Health gain this turn.
 */
void BattleUnit::prepareHealth(int health)
{
	// suffer from fatal wounds
	health -= getFatalWounds();

	// suffer from fire
	if (!_hitByFire && _fire > 0)
	{
		health -= reduceByResistance(RNG::generate(Mod::FIRE_DAMAGE_RANGE[0], Mod::FIRE_DAMAGE_RANGE[1]), DT_IN);
		_fire--;
	}

	setValueMax(_health, health, -4 * _stats.health, _stats.health);

	if (_health <= 0 && !checkStartBleedout())
	{
		_health = 0;
	}

	// if unit is dead, AI state should be gone
	if (_health <= 0 && _currentAIState)
	{
		delete _currentAIState;
		_currentAIState = 0;
	}
}

/**
 * Helper function preparing Stun recovery at beginning of turn.
 * @param stun Stun damage reduction this turn.
 */
void BattleUnit::prepareStun(int stun)
{
	if (!_isVehicle && (_armor->getSize() == 1 || !isOut()))
	{
		healStun(stun);
	}
}

/**
 * Helper function preparing Morale recovery at beginning of turn.
 * @param morale Morale grain this turn.
 */
void BattleUnit::prepareMorale(int morale)
{
	if (!isOut() && !getControlledBy())
	{
		_turnsAwake++;

		moraleChange(morale);
		int chance = 100 - (2 * getMorale());
		if (RNG::generate(1, 100) <= chance)
		{
			int type = RNG::generate(0, 100);
			_status = (type <= 33 ? STATUS_BERSERK : STATUS_PANICKING); // 33% chance of berserk, panic can mean freeze or flee, but that is determined later
			_wantsToSurrender = true;
		}
		else
		{
			// successfully avoided panic
			// increase bravery experience counter
			if (chance > 1)
				addBraveryExp();
		}
	}
	else
	{
		// knocked out units are willing to surrender if they wake up
		if (_status == STATUS_UNCONSCIOUS)
			_wantsToSurrender = true;
	}
}

/**
 * Prepare for a new turn.
 */
void BattleUnit::prepareNewTurn(TileEngine *tiles, bool fullProcess, bool reset)
{
	if (_status == STATUS_IGNORE_ME)
	{
		return;
	}

	_inCombat = true;

	_unitsSpottedThisTurn.clear();

	_hitByFire = false;
	_dontReselect = false;
	_motionPoints = 0;

	if(reset)
	{
		clearControlledBy();
		clearControlling();
		clearOngoingAction();
	}
	/*else
	{
		recoverTimeUnits();
	}*/

	updateStats();

	// transition between stages, don't do damage or panic
	if (!fullProcess)
	{
		return;
	}

	recoverStats(true, true);

	for (auto ii = _activeEffects.begin(); ii != _activeEffects.end();)
	{
		if ((*ii)->prepareNewTurn(tiles))
		{
			++ii;
		}
		else
		{
			ii = _activeEffects.erase(ii);
		}
	}

	if (_armor)
	{
		auto armorEffects = _armor->getEquippedEffects();

		for (auto ii = armorEffects.begin(); ii != armorEffects.end(); ++ii)
		{
			BattleEffect *effect = new BattleEffect(Game::getMod()->getEffect(*ii), this, this);
			effect->apply(tiles);
		}
	}

	for (auto ii = _inventory.cbegin(); ii != _inventory.cend(); ++ii)
	{
		const std::string &itemEffect = (*ii)->getRules()->getEquippedEffect();
		if (itemEffect.size())
		{
			BattleEffect *effect = new BattleEffect(Game::getMod()->getEffect(itemEffect), this, this);
			effect->apply(tiles);
		}
	}
}

/**
 * Update stats of unit.
 * @param tuAndEnergy
 * @param rest
 */
void BattleUnit::recoverStats(bool tuAndEnergy, bool rest)
{
	// snapshot of current stats
	int TURecovery = _armor->getTimeRecovery(this);
	int ENRecovery = _armor->getEnergyRecovery(this);
	int HPRecovery = _armor->getHealthRecovery(this);
	int MRRecovery = _armor->getMoraleRecovery(this);
	int STRecovery = _armor->getStunRegeneration(this);

	// update stats
	if (tuAndEnergy)
	{
		prepareTimeUnits(TURecovery);
		prepareEnergy(ENRecovery);
	}
	if (rest)
	{
		prepareHealth(HPRecovery);
		prepareStun(STRecovery);
		prepareMorale(MRRecovery);
	}
}

/**
 * Morale change with bounds check.
 * @param change can be positive or negative
 * @return final change in morale
 */
int BattleUnit::moraleChange(int change)
{
	if (!isFearable()) return 0;

	int oldMorale = _morale;

	_morale += change;
	if (_morale > 100)
		_morale = 100;
	else if (_morale < 0)
		_morale = 0;

	return _morale - oldMorale;
}

/**
 * Get reduced morale change value by bravery.
 */
int BattleUnit::reduceByBravery(int moraleChange) const
{
	return (110 - _stats.bravery) * moraleChange / 100;
}

/**
 * Calculate power reduction by resistances.
 */
int BattleUnit::reduceByResistance(int power, ItemDamageType resistType) const
{
	return (int)floor(power * _armor->getDamageModifier(resistType));
}

/**
 * Mark this unit as not reselectable.
 */
void BattleUnit::dontReselect()
{
	_dontReselect = true;
}

/**
 * Mark this unit as reselectable.
 */
void BattleUnit::allowReselect()
{
	_dontReselect = false;
}


/**
 * Check whether reselecting this unit is allowed.
 * @return bool
 */
bool BattleUnit::reselectAllowed() const
{
	return !_dontReselect;
}

/**
 * Set the amount of turns this unit is on fire. 0 = no fire.
 * @param fire : amount of turns this tile is on fire.
 */
void BattleUnit::setFire(int fire)
{
	if (_specab != SPECAB_BURNFLOOR && _specab != SPECAB_BURN_AND_EXPLODE)
		_fire = fire;
}

/**
 * Get the amount of turns this unit is on fire. 0 = no fire.
 * @return fire : amount of turns this tile is on fire.
 */
int BattleUnit::getFire() const
{
	return _fire;
}

/**
 * Get the pointer to the vector of inventory items.
 * @return pointer to vector.
 */
std::vector<BattleItem*> *BattleUnit::getInventory()
{
	return &_inventory;
}

/**
 * Fit item into inventory slot.
 * @param slot Slot to fit.
 * @param item Item to fit.
 * @return True if succeded, false otherwise.
 */
bool BattleUnit::fitItemToInventory(RuleInventory *slot, BattleItem *item)
{
	auto rule = item->getRules();
	if (slot->getType() == INV_HAND)
	{
		if (!Inventory::overlapItems(this, item, slot))
		{
			item->moveToOwner(this);
			item->setSlot(slot);
			return true;
		}
	}
	else if (slot->getType() == INV_SLOT)
	{
		for (const RuleSlot &rs : *slot->getSlots())
		{
			if (!Inventory::overlapItems(this, item, slot, rs.x, rs.y) && slot->fitItemInSlot(rule, rs.x, rs.y))
			{
				item->moveToOwner(this);
				item->setSlot(slot);
				item->setSlotX(rs.x);
				item->setSlotY(rs.y);
				return true;
			}
		}
	}
	return false;
}

/**
 * Adds an item to an XCom soldier (auto-equip).
 * @param item Pointer to the Item.
 * @param mod Pointer to the Mod.
 * @param save Pointer to the saved battle game for storing items.
 * @param allowSecondClip allow the unit to take a second clip or not. (only applies to xcom soldiers, aliens are allowed regardless of this flag)
 * @param allowAutoLoadout allow auto equip of weapons for solders.
 * @param allowUnloadedWeapons allow equip of weapons without ammo.
 * @return if the item was placed or not.
 */
bool BattleUnit::addItem(BattleItem *item, const Mod *mod, bool allowSecondClip, bool allowAutoLoadout, bool allowUnloadedWeapons)
{
	RuleInventory *rightHand = mod->getInventory("STR_RIGHT_HAND");
	RuleInventory *leftHand = mod->getInventory("STR_LEFT_HAND");
	bool placed = false;
	bool loaded = false;
	const RuleItem *rule = item->getRules();
	int weight = 0;

	// tanks and aliens don't care about weight or multiple items,
	// their loadouts are defined in the rulesets and more or less set in stone.
	if (getFaction() == FACTION_PLAYER && hasInventory())
	{
		weight = getCarriedWeight() + item->getTotalWeight();
		// allow all weapons to be loaded by avoiding this check,
		// they'll return false later anyway if the unit has something in his hand.
		if (rule->getBattleType() != BT_FIREARM && rule->getBattleType() != BT_MELEE)
		{
			int tally = 0;
			for (BattleItem *i : *getInventory())
			{
				if (rule->getType() == i->getRules()->getType())
				{
					if (allowSecondClip && rule->getBattleType() == BT_AMMO)
					{
						tally++;
						if (tally == 2)
						{
							return false;
						}
					}
					else
					{
						// we already have one, thanks.
						return false;
					}
				}
			}
		}
	}

	// place fixed weapon
	if (rule->isFixed())
	{
		// either in the default slot provided in the ruleset
		if (!rule->getDefaultInventorySlot().empty())
		{
			RuleInventory *defaultSlot = mod->getInventory(rule->getDefaultInventorySlot());
			BattleItem *defaultSlotWeapon = getItem(rule->getDefaultInventorySlot());
			if (!defaultSlotWeapon)
			{
				item->moveToOwner(this);
				item->setSlot(defaultSlot);
				placed = true;
				item->setXCOMProperty(getFaction() == FACTION_PLAYER);
				if (item->getRules()->getTurretType() > -1)
				{
					setTurretType(item->getRules()->getTurretType());
				}
			}
		}
		// or in the left/right hand
		if (!placed && (fitItemToInventory(rightHand, item) || fitItemToInventory(leftHand, item)))
		{
			placed = true;
			item->setXCOMProperty(getFaction() == FACTION_PLAYER);
			if (item->getRules()->getTurretType() > -1)
			{
				setTurretType(item->getRules()->getTurretType());
			}
		}
		return placed;
	}

	// we equip item only if we have skill to use it.
	if (getBaseStats()->psiSkill <= 0 && rule->isPsiRequired())
	{
		return false;
	}

	bool keep = true;
	switch (rule->getBattleType())
	{
	case BT_FIREARM:
	case BT_MELEE:
		if (item->haveAnyAmmo() || getFaction() != FACTION_PLAYER || !hasInventory() || allowUnloadedWeapons)
		{
			loaded = true;
		}

		if (loaded && (getGeoscapeSoldier() == 0 || allowAutoLoadout))
		{
			if (getBaseStats()->strength * 0.66 >= weight) // weight is always considered 0 for aliens
			{
				if (fitItemToInventory(rightHand, item))
				{
					placed = true;
				}
				if (!placed && getFaction() != FACTION_PLAYER && fitItemToInventory(leftHand, item))
				{
					placed = true;
				}
			}
		}
		break;
	case BT_AMMO:
		{
			BattleItem *rightWeapon = getItem(getWeaponSlot1());
			BattleItem *leftWeapon = getItem(getWeaponSlot2());
			// xcom weapons will already be loaded, aliens and tanks, however, get their ammo added afterwards.
			// so let's try to load them here.
			if (rightWeapon && (rightWeapon->getRules()->isFixed() || getFaction() != FACTION_PLAYER || allowUnloadedWeapons) &&
				rightWeapon->isWeaponWithAmmo() && rightWeapon->setAmmoPreMission(item))
			{
				placed = true;
				break;
			}
			if (leftWeapon && (leftWeapon->getRules()->isFixed() || getFaction() != FACTION_PLAYER || allowUnloadedWeapons) &&
				leftWeapon->isWeaponWithAmmo() && leftWeapon->setAmmoPreMission(item))
			{
				placed = true;
				break;
			}
			// don't take ammo for weapons we don't have.
			keep = (getFaction() != FACTION_PLAYER);
			if (rightWeapon)
			{
				if (rightWeapon->getRules()->getSlotForAmmo(rule->getType()) != -1)
				{
					keep = true;
				}
			}
			if (leftWeapon)
			{
				if (leftWeapon->getRules()->getSlotForAmmo(rule->getType()) != -1)
				{
					keep = true;
				}
			}
			if (!keep)
			{
				break;
			}
		}
	default:
		if (rule->getBattleType() == BT_PSIAMP && getFaction() == FACTION_HOSTILE)
		{
			if (fitItemToInventory(rightHand, item) || fitItemToInventory(leftHand, item))
			{
				placed = true;
			}
		}
		else if ((getGeoscapeSoldier() == 0 || allowAutoLoadout))
		{
			if (getBaseStats()->strength >= weight) // weight is always considered 0 for aliens
			{
				for (const std::string &s : mod->getInvsList())
				{
					RuleInventory *slot = mod->getInventory(s);
					if (slot->getType() == INV_SLOT)
					{
						placed = fitItemToInventory(slot, item);
						if (placed)
						{
							break;
						}
					}
				}
			}
		}
		break;
	}

	item->setXCOMProperty(getFaction() == FACTION_PLAYER);

	return placed;
}

/**
 * Let AI do their thing.
 * @param action AI action.
 */
void BattleUnit::think(BattleAction *action)
{
	reloadAmmo();
	_currentAIState->think(action);
}

/**
 * Changes the current AI state.
 * @param aiState Pointer to AI state.
 */
void BattleUnit::setAIModule(AIModule *ai)
{
	if (_currentAIState)
	{
		delete _currentAIState;
	}
	_currentAIState = ai;
}

/**
 * Returns the current AI state.
 * @return Pointer to AI state.
 */
AIModule *BattleUnit::getAIModule() const
{
	return _currentAIState;
}

/**
 * Set whether this unit is visible.
 * @param flag
 */
void BattleUnit::setVisible(bool flag)
{
	_visible = flag;
}


/**
 * Get whether this unit is visible.
 * @return flag
 */
bool BattleUnit::getVisible() const
{
	if (getFaction() == FACTION_PLAYER)
	{
		return true;
	}
	else
	{
		return _visible;
	}
}

/**
 * Sets the unit's tile it's standing on
 * @param tile Pointer to tile.
 * @param tileBelow Pointer to tile below.
 */
void BattleUnit::setTile(Tile *tile, Tile *tileBelow)
{
	_tile = tile;
	if (!_tile)
	{
		_floating = false;
		return;
	}
	// unit could have changed from flying to walking or vice versa
	if (_status == STATUS_WALKING && _tile->hasNoFloor(tileBelow) && _movementType == MT_FLY)
	{
		_status = STATUS_FLYING;
		_floating = true;
	}
	else if (_status == STATUS_FLYING && !_tile->hasNoFloor(tileBelow) && _verticalDirection == 0)
	{
		_status = STATUS_WALKING;
		_floating = false;
	}
	else if (_status == STATUS_UNCONSCIOUS)
	{
		_floating = _movementType == MT_FLY && _tile->hasNoFloor(tileBelow);
	}
}

/**
 * Gets the unit's tile.
 * @return Tile
 */
Tile *BattleUnit::getTile() const
{
	return _tile;
}

/**
 * Checks if there's an inventory item in
 * the specified inventory position.
 * @param slot Inventory slot.
 * @param x X position in slot.
 * @param y Y position in slot.
 * @return Item in the slot, or NULL if none.
 */
BattleItem *BattleUnit::getItem(RuleInventory *slot, int x, int y) const
{
	// Soldier items
	if (slot->getType() != INV_GROUND)
	{
		for (std::vector<BattleItem*>::const_iterator i = _inventory.begin(); i != _inventory.end(); ++i)
		{
			if ((*i)->getSlot() == slot && (*i)->occupiesSlot(x, y))
			{
				return *i;
			}
		}
	}
	// Ground items
	else if (_tile != 0)
	{
		for (std::vector<BattleItem*>::const_iterator i = _tile->getInventory()->begin(); i != _tile->getInventory()->end(); ++i)
		{
			if ((*i)->occupiesSlot(x, y))
			{
				return *i;
			}
		}
	}
	return 0;
}

/**
 * Checks if there's an inventory item in
 * the specified inventory position.
 * @param slot Inventory slot.
 * @param x X position in slot.
 * @param y Y position in slot.
 * @return Item in the slot, or NULL if none.
 */
BattleItem *BattleUnit::getItem(const std::string &slot, int x, int y) const
{
	// Soldier items
	if (slot != "STR_GROUND")
	{
		for (std::vector<BattleItem*>::const_iterator i = _inventory.begin(); i != _inventory.end(); ++i)
		{
			if ((*i)->getSlot() != 0 && (*i)->getSlot()->getId() == slot && (*i)->occupiesSlot(x, y))
			{
				return *i;
			}
		}
	}
	// Ground items
	else if (_tile != 0)
	{
		for (std::vector<BattleItem*>::const_iterator i = _tile->getInventory()->begin(); i != _tile->getInventory()->end(); ++i)
		{
			if ((*i)->getSlot() != 0 && (*i)->occupiesSlot(x, y))
			{
				return *i;
			}
		}
	}
	return 0;
}

/**
 * Get the "main hand weapon" from the unit.
 * @param quickest Whether to get the quickest weapon, default true
 * @return Pointer to item.
 */
BattleItem *BattleUnit::getMainHandWeapon(bool quickest) const
{
	BattleItem *weaponRightHand = getItem(_weaponSlot1);
	BattleItem *weaponLeftHand = getItem(_weaponSlot2);

	// ignore weapons without ammo (rules out grenades)
	if (!weaponRightHand || !weaponRightHand->haveAnyAmmo())
		weaponRightHand = 0;
	if (!weaponLeftHand || !weaponLeftHand->haveAnyAmmo())
		weaponLeftHand = 0;

	// if there is only one weapon, it's easy:
	if (weaponRightHand && !weaponLeftHand)
		return weaponRightHand;
	else if (!weaponRightHand && weaponLeftHand)
		return weaponLeftHand;
	else if (!weaponRightHand && !weaponLeftHand)
		return 0;

	// otherwise pick the one with the least snapshot TUs
	int tuRightHand = getActionTUs(BA_SNAPSHOT, weaponRightHand).Time;
	int tuLeftHand = getActionTUs(BA_SNAPSHOT, weaponLeftHand).Time;
	BattleItem *weaponCurrentHand = getItem(getActiveHand());
	//prioritize blasters
	if (!quickest && _faction != FACTION_PLAYER)
	{
		if (weaponRightHand->getRules()->getWaypoints() > 0 || weaponRightHand->getAmmoForAction(BA_LAUNCH)->getRules()->getWaypoints() > 0)
		{
			return weaponRightHand;
		}
		if (weaponLeftHand->getRules()->getWaypoints() != 0 || weaponLeftHand->getAmmoForAction(BA_LAUNCH)->getRules()->getWaypoints() != 0)
		{
			return weaponLeftHand;
		}
	}
	// if only one weapon has snapshot, pick that one
	if (tuLeftHand <= 0 && tuRightHand > 0)
		return weaponRightHand;
	else if (tuRightHand <= 0 && tuLeftHand > 0)
		return weaponLeftHand;
	// else pick the better one
	else
	{
		if (tuLeftHand >= tuRightHand)
		{
			if (quickest)
			{
				return weaponRightHand;
			}
			else if (_faction == FACTION_PLAYER)
			{
				return weaponCurrentHand;
			}
			else
			{
				return weaponLeftHand;
			}
		}
		else
		{
			if (quickest)
			{
				return weaponLeftHand;
			}
			else if (_faction == FACTION_PLAYER)
			{
				return weaponCurrentHand;
			}
			else
			{
				return weaponRightHand;
			}
		}
	}
}

/**
 * Get a grenade from the belt (used for AI)
 * @return Pointer to item.
 */
BattleItem *BattleUnit::getGrenadeFromBelt() const
{
	for (std::vector<BattleItem*>::const_iterator i = _inventory.begin(); i != _inventory.end(); ++i)
	{
		if ((*i)->getRules()->getBattleType() == BT_GRENADE)
		{
			return *i;
		}
	}
	return 0;
}

/**
 * Find the cheapest to grab item from the inventory
 * @return Pointer to item.
 */
BattleItem *BattleUnit::findQuickItem(const std::string &item, RuleInventory* destSlot, int* moveCost) const
{
	BattleItem *quickItem = 0;
	int quickCost = 1000000;
	for (std::vector<BattleItem*>::const_iterator ii = _inventory.begin(); ii != _inventory.end(); ++ii)
	{
		BattleItem *bi = (*ii);

		int cost;
		if(bi->getRules()->getType() == item && bi->getSlot() != destSlot && ((cost = bi->getSlot()->getCost(destSlot)) < quickCost))
		{
			quickItem = bi;
			quickCost = cost; // ??
		}
	}

	Tile* tile = getTile();

	if(!isVehicle() && tile && tile->getInventory())
	{
		for(auto ii = tile->getInventory()->begin(); ii != tile->getInventory()->end(); ++ii)
		{
			BattleItem *bi = (*ii);

			int cost;
			if(bi->getRules()->getType() == item && ((cost = bi->getSlot()->getCost(destSlot)) < quickCost))
			{
				quickItem = bi;
				quickCost = cost;
			}
		}
	}

	if(quickItem && moveCost) { (*moveCost) = quickCost; }

	return quickItem;
}

/**
 * Find the cheapest to ammo item for the specified weapon.
 * @param weapon The weapon to find ammo for.
 * @return Pointer to item.
 */
BattleItem *BattleUnit::findQuickAmmo(BattleItem *weapon, int* reloadCost) const
{
	BattleItem *bestAmmo = 0;
	int bestCost = 100000;

	const std::vector<std::string> *ammoItems = weapon->getRules()->getCompatibleAmmoForSlot(0);

	for(std::vector<std::string>::const_iterator ii = ammoItems->begin(); ii != ammoItems->end(); ++ii)
	{
		int cost = 100000;
		BattleItem *ammo = findQuickItem(*ii, weapon->getSlot(), &cost);
		if(ammo && ((cost = getLoadCost(weapon, ammo)) < bestCost))
		{
			bestAmmo = ammo;
			bestCost = cost;
		}
	}

	if(bestAmmo && reloadCost)
	{
		*reloadCost = bestCost;
	}

	return bestAmmo;
}

/**
 * Check if we have ammo and reload if needed (used for AI).
 * @return Do we have ammo?
 */
bool BattleUnit::reloadAmmo()
{
	BattleItem *list[2] =
	{
		getWeapon1(),
		getWeapon2(),
	};

	for (int i = 0; i < 2; ++i)
	{
		BattleItem *weapon = list[i];
		if (!weapon || !weapon->isWeaponWithAmmo() || weapon->haveAllAmmo())
		{
			continue;
		}

		// we have a non-melee weapon with no ammo and 15 or more TUs - we might need to look for ammo then
		BattleItem *ammo = 0;
		auto ruleWeapon = weapon->getRules();
		auto tuCost = getTimeUnits();
		auto slotAmmo = 0;

		for (BattleItem* i : *getInventory())
		{
			int slot = ruleWeapon->getSlotForAmmo(i->getRules()->getType());
			if (slot != -1)
			{
				int tuTemp = i->getSlot()->getType() != INV_HAND ? i->getSlot()->getCost(weapon->getSlot()) : 0;
				tuTemp += ruleWeapon->getTULoad(slot);
				if (tuTemp < tuCost)
				{
					tuCost = tuTemp;
					ammo = i;
					slotAmmo = slot;
				}
			}
		}

		if (ammo && spendTimeUnits(tuCost))
		{
			weapon->setAmmoForSlot(slotAmmo, ammo);
			ammo->moveToOwner(0);

			return true;
		}
	}
	return false;
}

/**
 * Check if this unit is in the exit area.
 * @param stt Type of exit tile to check for.
 * @return Is in the exit area?
 */
bool BattleUnit::isInExitArea(SpecialTileType stt) const
{
	return _tile && _tile->getMapData(O_FLOOR) && (_tile->getMapData(O_FLOOR)->getSpecialType() == stt);
}

/**
 * Gets the unit height taking into account kneeling/standing.
 * @return Unit's height.
 */
int BattleUnit::getHeight() const
{
	return isKneeled()?getKneelHeight():getStandHeight();
}

/**
 * Adds one to the bravery exp counter.
 */
void BattleUnit::addBraveryExp()
{
	_expBravery++;
}

/**
 * Adds one to the reaction exp counter.
 */
void BattleUnit::addReactionExp()
{
	_expReactions++;
}

/**
 * Adds one to the firing exp counter.
 */
void BattleUnit::addFiringExp()
{
	_expFiring++;
}

/**
 * Adds one to the throwing exp counter.
 */
void BattleUnit::addThrowingExp()
{
	_expThrowing++;
}

/**
 * Adds one to the psi skill exp counter.
 */
void BattleUnit::addPsiSkillExp()
{
	_expPsiSkill++;
}

/**
 * Adds one to the psi strength exp counter.
 */
void BattleUnit::addPsiStrengthExp()
{
	_expPsiStrength++;
}

/**
 * Adds one to the melee exp counter.
 */
void BattleUnit::addMeleeExp()
{
	_expMelee++;
}

void BattleUnit::updateGeoscapeStats(Soldier *soldier) const
{
	soldier->addMissionCount();
	soldier->addKillCount(_kills);
}

/**
 * Check if unit eligible for squaddie promotion. If yes, promote the unit.
 * Increase the mission counter. Calculate the experience increases.
 * @param geoscape Pointer to geoscape save.
 * @param statsDiff (out) The passed UnitStats struct will be filled with the stats differences.
 * @return True if the soldier was eligible for squaddie promotion.
 */
bool BattleUnit::postMissionProcedures(SavedGame *geoscape, UnitStats &statsDiff, int turns, int teamExperience, int &gainedExperience, bool &gainedLevel, int &kills)
{
	Soldier *s = geoscape->getSoldier(_id);
	if (s == 0)
	{
		return false;
	}

	updateGeoscapeStats(s);

	kills = _kills;

	if (s->isVehicle())
	{
		gainedLevel = false;
		gainedExperience = 0;
		return false;
	}

	UnitStats *stats = s->getCurrentStats();
	statsDiff -= *stats;        // subtract old stats
	const UnitStats caps = s->getRules()->getStatCaps();
	int healthLoss = _stats.health - _health;

	if (healthLoss < 0)
	{
		healthLoss = 0;
	}

	if (healthLoss > 0)
	{
		// TODO: Revamp to standardize un-upgraded 1 recovery per day time
		if (geoscape->isResearched("STR_FIELD_SURGERY_UNIT"))
		{
			s->setWoundRecovery((healthLoss * RNG::generate(15, 25)) / stats->health);
		}
		else
		{
			s->setWoundRecovery((healthLoss * RNG::generate(20, 30)) / stats->health);
		}
	}

	gainedLevel = false;
	if (teamExperience)
	{
		gainedExperience = ((teamExperience * (_battleExperience + 100)) / 200) * (getTurnsAwake() / turns);
		if (gainedExperience)
		{
			gainedLevel = s->addExperience(gainedExperience);
		}
	}
	else
	{
		gainedExperience = 0;
	}

	if (_expBravery && stats->bravery < caps.bravery)
	{
		if (_expBravery > RNG::generate(0, 10)) stats->bravery += 10;
	}
	if (_expReactions && stats->reactions < caps.reactions)
	{
		stats->reactions += improveStat(_expReactions);
	}
	if (_expFiring && stats->firing < caps.firing)
	{
		stats->firing += improveStat(_expFiring);
	}
	if (_expMelee && stats->melee < caps.melee)
	{
		stats->melee += improveStat(_expMelee);
	}
	if (_expThrowing && stats->throwing < caps.throwing)
	{
		stats->throwing += improveStat(_expThrowing);
	}
	if (_expPsiSkill && stats->psiSkill < caps.psiSkill)
	{
		stats->psiSkill += improveStat(_expPsiSkill);
	}
	if (_expPsiStrength && stats->psiStrength < caps.psiStrength)
	{
		stats->psiStrength += improveStat(_expPsiStrength);
	}

	bool hasImproved = false;
	if (_expBravery || _expReactions || _expFiring || _expPsiSkill || _expPsiStrength || _expMelee || _expThrowing)
	{
		hasImproved = true;
		if (s->getRank() == RANK_ROOKIE)
			s->promoteRank();
		int v;
		v = caps.tu - stats->tu;
		if (v > 0) stats->tu += RNG::generate(0, v / 10 + 2);
		v = caps.health - stats->health;
		if (v > 0) stats->health += RNG::generate(0, v / 10 + 2);
		v = caps.strength - stats->strength;
		if (v > 0) stats->strength += RNG::generate(0, v / 10 + 2);
		v = caps.stamina - stats->stamina;
		if (v > 0) stats->stamina += RNG::generate(0, v / 10 + 2);
	}

	statsDiff += *stats; // add new stats

	return hasImproved;
}

/**
 * Converts the number of experience to the stat increase.
 * @param Experience counter.
 * @return Stat increase.
 */
int BattleUnit::improveStat(int exp) const
{
	if      (exp > 10) return RNG::generate(2, 6);
	else if (exp > 5)  return RNG::generate(1, 4);
	else if (exp > 2)  return RNG::generate(1, 3);
	else if (exp > 0)  return RNG::generate(0, 1);
	else               return 0;
}

/**
 * Get the unit's minimap sprite index. Used to display the unit on the minimap
 * @return the unit minimap index
 */
int BattleUnit::getMiniMapSpriteIndex() const
{
	//minimap sprite index:
	// * 0-2   : Xcom soldier
	// * 3-5   : Alien
	// * 6-8   : Civilian
	// * 9-11  : Item
	// * 12-23 : Xcom HWP
	// * 24-35 : Alien big terror unit(cyberdisk, ...)
	if (isOut())
	{
		return 9;
	}
	switch (getFaction())
	{
	case FACTION_HOSTILE:
		if (_armor->getSize() == 1)
			return 3;
		else
			return 24;
	case FACTION_NEUTRAL:
		if (_armor->getSize() == 1)
			return 6;
		else
			return 12;
	default:
		if (_armor->getSize() == 1)
			return 0;
		else
			return 12;
	}
}

/**
  * Set the turret type. -1 is no turret.
  * @param turretType
  */
void BattleUnit::setTurretType(int turretType)
{
	_turretType = turretType;
}

/**
  * Get the turret type. -1 is no turret.
  * @return type
  */
int BattleUnit::getTurretType() const
{
	return _turretType;
}

/**
 * Get the amount of fatal wound for a body part
 * @param part The body part (in the range 0-5)
 * @return The amount of fatal wound of a body part
 */
int BattleUnit::getFatalWound(int part) const
{
	if (part < 0 || part > 5)
		return 0;
	return _fatalWounds[part];
}

/**
 * Heal a fatal wound of the soldier
 * @param part the body part to heal
 * @param woundAmount the amount of fatal wound healed
 * @param healthAmount The amount of health to add to soldier health
 */
bool BattleUnit::heal(int part, int woundAmount, int healthAmount)
{
	if(isVehicle())
		return false;
	if (part < 0 || part > 5)
		return false;
	if (!_fatalWounds[part])
		return false;

	bool bleeding = getBleedingOut();
	if(bleeding)
	{
		// Apply a heavy stun to prevent the unit from returning to combat.
		_stunlevel = std::max(1000, _stunlevel);
		//_health = (int)(getStats()->health * 0.1);

		// Boost wound healing, and prevent health restore when bleeding out.
		//woundAmount *= 3;
		healthAmount = 0;
	}

	setValueMax(_fatalWounds[part], -woundAmount, 0, 100);
	setValueMax(_health, healthAmount, 1, getBaseStats()->health); //Hippocratic Oath: First do no harm

	return true;
}

/**
 * Restore soldier morale
 * @param moraleAmount additional morale boost.
 * @param painKillersStrength how much of damage convert to morale.
 */
void BattleUnit::painKillers(int moraleAmount, float painKillersStrength)
{
	int lostHealth = (getBaseStats()->health - _health) * painKillersStrength;
	if (lostHealth > _moraleRestored)
	{
		_morale = std::min(100, (lostHealth - _moraleRestored + _morale));
		_moraleRestored = lostHealth;
	}
	moraleChange(moraleAmount);
}

/**
 * Restore soldier energy and reduce stun level
 * @param energy The amount of energy to add
 * @param s The amount of stun level to reduce
 */
void BattleUnit::stimulant (int energy, int s)
{
	_energy += energy;
	if (_energy > getBaseStats()->stamina)
		_energy = getBaseStats()->stamina;
	healStun (s);
}

/**
 * Get motion points for the motion scanner. More points
 * is a larger blip on the scanner.
 * @return points.
 */
int BattleUnit::getMotionPoints() const
{
	return _motionPoints;
}

/**
 * Gets the unit's armor.
 * @return Pointer to armor.
 */
const Armor *BattleUnit::getArmor() const
{
	return _armor;
}

/**
 * Set the unit's name.
 * @param name Name
 */
void BattleUnit::setName(const std::wstring &name)
{
	_name = name;
}

/**
 * Get unit's name.
 * An aliens name is the translation of it's race and rank.
 * hence the language pointer needed.
 * @param lang Pointer to language.
 * @param debugAppendId Append unit ID to name for debug purposes.
 * @return name Widecharstring of the unit's name.
 */
std::wstring BattleUnit::getName(Language *lang, bool debugAppendId) const
{
	if (_type != "SOLDIER" && lang != 0)
	{
		std::wstring ret;

		if (_type.find("STR_") != std::string::npos)
			ret = lang->getString(_type);
		else
			ret = lang->getString(_race);

		if (debugAppendId)
		{
			std::wostringstream ss;
			ss << ret << L" " << _id;
			ret = ss.str();
		}
		return ret;
	}

	return _name;
}

std::wstring BattleUnit::getLogName(Language *lang) const
{
	Role* role = _geoscapeSoldier ? _geoscapeSoldier->getRole() : 0;
	if (role && !role->isBlank())
	{
		return lang->getString("STR_LOG_ROLE_ACTOR").arg(lang->getString(role->getName() + "_SHORT")).arg(getName(lang));
	}
	else
	{
		return getName(lang);
	}
}

/**
  * Gets pointer to the unit's stats.
  * @return stats Pointer to the unit's stats.
  */
UnitStats *BattleUnit::getBaseStats()
{
	return &_stats;
}

/**
  * Gets pointer to the unit's stats.
  * @return stats Pointer to the unit's stats.
  */
const UnitStats *BattleUnit::getBaseStats() const
{
	return &_stats;
}

/**
  * Get the unit's stand height.
  * @return The unit's height in voxels, when standing up.
  */
int BattleUnit::getStandHeight() const
{
	return _standHeight;
}

/**
  * Get the unit's kneel height.
  * @return The unit's height in voxels, when kneeling.
  */
int BattleUnit::getKneelHeight() const
{
	return _kneelHeight;
}

/**
  * Get the unit's floating elevation.
  * @return The unit's elevation over the ground in voxels, when flying.
  */
int BattleUnit::getFloatHeight() const
{
	return _floatHeight;
}

/**
  * Get the unit's loft ID, one per unit tile.
  * Each tile only has one loft, as it is repeated over the entire height of the unit.
  * @param entry Unit tile
  * @return The unit's line of fire template ID.
  */
int BattleUnit::getLoftemps(int entry) const
{
	return _loftempsSet.at(entry);
}

/**
  * Get the unit's value. Used for score at debriefing.
  * @return value score
  */
int BattleUnit::getValue() const
{
	return _value;
}

/**
 * Get the unit's death sounds.
 * @return List of sound IDs.
 */
const std::vector<int> &BattleUnit::getDeathSounds() const
{
	if (_deathSound.empty() && _geoscapeSoldier != 0)
	{
		if (_gender == GENDER_MALE)
			return _geoscapeSoldier->getRules()->getMaleDeathSounds();
		else
			return _geoscapeSoldier->getRules()->getFemaleDeathSounds();
	}
	return _deathSound;
}

/**
 * Get the unit's move sound.
 * @return id.
 */
int BattleUnit::getMoveSound() const
{
	return _moveSound;
}


/**
 * Get whether the unit is affected by fatal wounds.
 * Normally only soldiers are affected by fatal wounds.
 * @return Is the unit affected by wounds?
 */
bool BattleUnit::isWoundable() const
{
	return !_armor->getBleedImmune(!(_type=="SOLDIER" || (Options::alienBleeding && _originalFaction != FACTION_PLAYER)));
	// TODO: Ensure vehicle armor forces woundable to false.
	//return ((_type=="SOLDIER" && !_isVehicle) || (Options::alienBleeding && _originalFaction != FACTION_PLAYER && _armor->getSize() == 1));
}

/**
 * Get whether the unit is affected by morale loss.
 * Normally only small units are affected by morale loss.
 * @return Is the unit affected by morale?
 */
bool BattleUnit::isFearable() const
{
	// TODO: Ensure vehicle armor fear immune to false.
	return !_armor->getFearImmune();
}

/**
 * Get the number of turns an AI unit remembers a soldier's position.
 * @return intelligence.
 */
int BattleUnit::getIntelligence() const
{
	return _intelligence;
}

/**
 * Get the unit's aggression.
 * @return aggression.
 */
int BattleUnit::getAggression() const
{
	return _aggression;
}

int BattleUnit::getMaxViewDistance(int baseVisibility, int nerf, int buff) const
{
	int result = baseVisibility;
	if (nerf > 0)
	{
		result = nerf; // fixed distance nerf
	}
	else
	{
		result += nerf; // relative distance nerf
	}
	if (result < 1)
	{
		result = 1;  // can't go under melee distance
	}
	result += buff; // relative distance buff
	if (result > baseVisibility)
	{
		result = baseVisibility; // don't overbuff (buff is only supposed to counter the nerf)
	}
	return result;
}

int BattleUnit::getMaxViewDistanceAtDark(const Armor *otherUnitArmor) const
{
	if (otherUnitArmor)
	{
		return getMaxViewDistance(_maxViewDistanceAtDark, otherUnitArmor->getCamouflageAtDark(), _armor->getAntiCamouflageAtDark());
	}
	else
	{
		return _maxViewDistanceAtDark;
	}
}

int BattleUnit::getMaxViewDistanceAtDay(const Armor *otherUnitArmor) const
{
	if (otherUnitArmor)
	{
		return getMaxViewDistance(_maxViewDistanceAtDay, otherUnitArmor->getCamouflageAtDay(), _armor->getAntiCamouflageAtDay());
	}
	else
	{
		return _maxViewDistanceAtDay;
	}
}

/**
 * Returns the unit's special ability.
 * @return special ability.
 */
int BattleUnit::getSpecialAbility() const
{
	return _specab;
}

/**
 * Sets this unit to respawn (or not).
 * @param respawn whether it should respawn.
 */
void BattleUnit::setRespawn(bool respawn)
{
	_respawn = respawn;
}

/**
 * Gets this unit's respawn flag.
 */
bool BattleUnit::getRespawn() const
{
	return _respawn;
}

/**
 * Marks this unit as already respawned (or not).
 * @param alreadyRespawned whether it already respawned.
 */
void BattleUnit::setAlreadyRespawned(bool alreadyRespawned)
{
	_alreadyRespawned = alreadyRespawned;
}

/**
 * Gets this unit's alreadyRespawned flag.
 */
bool BattleUnit::getAlreadyRespawned() const
{
	return _alreadyRespawned;
}

/**
 * Get the unit that is spawned when this one dies.
 * @return unit.
 */
std::string BattleUnit::getSpawnUnit() const
{
	return _spawnUnit;
}

/**
 * Set the unit that is spawned when this one dies.
 * @param spawnUnit unit.
 */
void BattleUnit::setSpawnUnit(const std::string &spawnUnit)
{
	_spawnUnit = spawnUnit;
}

/**
 * Get the units's rank string.
 * @return rank.
 */
std::string BattleUnit::getRankString() const
{
	return _rank;
}

/**
 * Get the geoscape-soldier object.
 * @return soldier.
 */
Soldier *BattleUnit::getGeoscapeSoldier() const
{
	return _geoscapeSoldier;
}

/**
 * Add a kill to the counter.
 */
void BattleUnit::addKillCount()
{
	_kills++;
}

/**
 * Get unit type.
 * @return unit type.
 */
std::string BattleUnit::getType() const
{
	return _type;
}

/**
 * Set unit's active hand.
 * @param hand active hand.
 */
void BattleUnit::setActiveHand(const std::string &hand)
{
	_activeHand = hand;
}

/**
 * Get unit's active hand.
 * @return active hand.
 */
std::string BattleUnit::getActiveHand() const
{
	if (getItem(_activeHand)) return _activeHand;
	if (getItem(_weaponSlot2)) return _weaponSlot2;
	return _weaponSlot1;
}

/**
 * Converts unit to another faction (original faction is still stored).
 * @param f faction.
 */
void BattleUnit::convertToFaction(UnitFaction f)
{
	_faction = f;
}

/**
* Set health to 0 - used when getting killed unconscious.
*/
void BattleUnit::kill()
{
	_health = 0;
}

/**
 * Set health to 0 and set status dead - used when getting zombified.
 */
void BattleUnit::instaKill()
{
	_health = 0;
	die();
}

/**
 * Marks the unit dead and triggers related events
 */
void BattleUnit::die(TileEngine *tiles)
{
	_status = STATUS_DEAD;

	clearControlling();
	clearOngoingAction();

	backlashController(tiles);
}

void BattleUnit::backlashController(TileEngine *tiles, CombatLog *log)
{
	if (BattleUnit *controller = getControlledBy())
	{
		clearControlledBy();

		controller->damage(tiles, this, Position(), RNG::generate(20, 40), Game::getMod()->getDamageType(DT_PSYCHIC), Game::getSavedGame()->getSavedBattle(), BattleActionAttack(BA_NONE, this), SIDE_MAX, BODYPART_MAX, false, 0, "STR_BACKLASH");

		if (Soldier *soldier = controller->getGeoscapeSoldier())
		{
			const int psiSkillLoss = 15;
			soldier->getCurrentStats()->psiStrength = std::max(soldier->getCurrentStats()->psiSkill - psiSkillLoss, 0);
			controller->getBaseStats()->psiStrength = std::max(controller->getBaseStats()->psiSkill - psiSkillLoss, 0);

			if (log && controller->getOriginalFaction() == FACTION_PLAYER)
			{
				std::wostringstream ss;
				ss << Game::getLanguage()->getString("STR_LOG_ACTOR").arg(controller->getLogName(Game::getLanguage()));
				ss << Game::getLanguage()->getString("STR_BACKLASH_PSI_LOSS").arg(psiSkillLoss);
				log->log(ss.str(), COMBAT_LOG_RED);
			}
		}
	}
}

/**
 * Get sound to play when unit aggros.
 * @return sound
 */
int BattleUnit::getAggroSound() const
{
	return _aggroSound;
}

/**
 * Set a specific number of energy.
 * @param energy energy.
 */
void BattleUnit::setEnergy(int energy)
{
	_energy = energy;
}

/**
 * Get the faction the unit was killed by.
 * @return faction
 */
UnitFaction BattleUnit::killedBy() const
{
	return _killedBy;
}

/**
 * Set the faction the unit was killed by.
 * @param f faction
 */
void BattleUnit::killedBy(UnitFaction f)
{
	_killedBy = f;
}

/**
 * Set the units we are charging towards.
 * @param chargeTarget Charge Target
 */
void BattleUnit::setCharging(BattleUnit *chargeTarget)
{
	_charging = chargeTarget;
}

/**
 * Get the units we are charging towards.
 * @return Charge Target
 */
BattleUnit *BattleUnit::getCharging()
{
	return _charging;
}

/**
 * Get the units carried weight in strength units.
 * @param draggingItem item to ignore
 * @return weight
 */
int BattleUnit::getCarriedWeight(BattleItem *draggingItem) const
{
	int weight = _armor->getWeight();
	for (std::vector<BattleItem*>::const_iterator i = _inventory.begin(); i != _inventory.end(); ++i)
	{
		if ((*i) == draggingItem) continue;
		weight += (*i)->getTotalWeight();
	}
	return std::max(0,weight);
}

/**
 * Set how long since this unit was last exposed.
 * @param turns number of turns
 */
void BattleUnit::setTurnsSinceSpotted (int turns)
{
	_turnsSinceSpotted = turns;
}

/**
 * Get how long since this unit was exposed.
 * @return number of turns
 */
int BattleUnit::getTurnsSinceSpotted() const
{
	return _turnsSinceSpotted;
}

/**
 * Get this unit's original Faction.
 * @return original faction
 */
UnitFaction BattleUnit::getOriginalFaction() const
{
	return _originalFaction;
}

/**
 * Calculate effective range for the given soldier accuracy and weapon accuracy.
 * @return Effective range.
 */
double BattleUnit::calculateEffectiveRange(double soldierAcc, double weaponAcc)
{
	if(soldierAcc < 20)
		soldierAcc = 20;
	if(weaponAcc < 1)
		weaponAcc = 1;

    soldierAcc = 0.437 / (soldierAcc * soldierAcc / 50) * 1.4826 * 2;
    weaponAcc = 0.437 / weaponAcc * 1.4826;

	double accMul = soldierAcc;
	double acc = weaponAcc / accMul;

	//Approximation curve. TODO: Better fit.
	double medianAngle = accMul * (0.33724 * acc + 0.33724 * sqrt((acc - 2) * acc + 2) + 0.30384);

    return 0.437 / medianAngle;
}

/**
 * Calculate effective range for the given action.
 * @return Effective range.
 */
int BattleUnit::calculateEffectiveRangeForAction(BattleActionType actionType, BattleItem *item, bool dualFiring)
{
	if (actionType == BA_DUALFIRE)
	{
		BattleItem *weapon1;
		BattleItem *weapon2;
		BattleActionType action1;
		BattleActionType action2;
		if (canDualFire(weapon1, action1, weapon2, action2))
		{
			return (calculateEffectiveRangeForAction(action1, weapon1, true) + calculateEffectiveRangeForAction(action2, weapon2, true)) / 2.0;
		}
		else
		{
			return 0;
		}
	}

	double soldierAcc = 1;
	double weaponAcc = 1;

	if (isKneeled())
	{
		int kneelMod = item->getRules()->getKneelBonus(Game::getMod());
		if (kneelMod)
		{
			soldierAcc = double(getBaseStats()->firing * kneelMod) / 100.0;
		}
		else
		{
			soldierAcc = double(getBaseStats()->firing * 115) / 100.0;
		}
	}
	else
	{
		soldierAcc = double(getBaseStats()->firing);
	}

	if (actionType == BA_OVERWATCH)
	{
		actionType = getOverwatchShotAction(item->getRules());
	}

	//Specific shot modes now provide an accuracy modifier to the soldier, rather than the weapon.
	switch(actionType)
	{
	case BA_SNAPSHOT:
		soldierAcc *= double(item->getRules()->getAccuracySnap()) / 100.0;
		break;
	case BA_AIMEDSHOT:
		soldierAcc *= double(item->getRules()->getAccuracyAimed()) / 100.0;
		break;
	case BA_AUTOSHOT:
		soldierAcc *= double(item->getRules()->getAccuracyAuto()) / 100.0;
		break;
	case BA_BURSTSHOT:
		soldierAcc *= double(item->getRules()->getAccuracyBurst()) / 100.0;
		break;
	}

	soldierAcc *= double(getAccuracyModifier(item)) / 100.0;

	double energyRatio = std::max((double)getEnergy() / (double)getBaseStats()->stamina, 0.0);
	if (energyRatio < 0.5)
	{
		soldierAcc *= (0.5 + energyRatio);
	}

	if (item->getRules()->isTwoHanded()
		&& getItem(_weaponSlot1)
		&& getItem(_weaponSlot2))
	{
		soldierAcc *= item->getRules()->getOneHandedPenalty(Game::getMod()) / 100.0;
	}

	if (dualFiring)
	{
		soldierAcc *= getDualFireAccuracy();
	}
	
	//Weapon accuracy comes straight off the weapon, and isn't influenced by anything.
	weaponAcc = item->getRules()->getBaseAccuracy();
	if(weaponAcc <= 1)
	{
		weaponAcc = 1;
	}

	int acc = calculateEffectiveRange(soldierAcc, weaponAcc);

    return acc;
}

/**
 * Calculate chance to hit for the given effective range and distance.
 * @return Chance to hit.
 */
double BattleUnit::calculateChanceToHit(double effectiveRange, double distance)
{
	if(distance < 0)
		distance = 0;
	if(effectiveRange < 1)
		effectiveRange = 1;

	//Approximation curve. TODO: Better fit.
    return effectiveRange / (distance + effectiveRange);
}

/**
 * Get the list of units spotted this turn.
 * @return List of units.
 */
std::vector<BattleUnit *> &BattleUnit::getUnitsSpottedThisTurn()
{
	return _unitsSpottedThisTurn;
}

/**
 * Change the numeric version of the unit's rank.
 * @param rank unit rank, 0 = lowest
 */
void BattleUnit::setRankInt(int rank)
{
	_rankInt = rank;
}

/**
 * Return the numeric version of the unit's rank.
 * @return unit rank, 0 = lowest
 */
int BattleUnit::getRankInt() const
{
	return _rankInt;
}

/**
 * Derive the numeric unit rank from the string rank
 * (for soldier units).
 */
void BattleUnit::deriveRank()
{
	if(_isVehicle) return;
		
	if (_geoscapeSoldier)
	{
		switch (_geoscapeSoldier->getRank())
		{
		case RANK_ROOKIE:    _rankInt = 0; break;
		case RANK_SQUADDIE:  _rankInt = 1; break;
		case RANK_SERGEANT:  _rankInt = 2; break;
		case RANK_CAPTAIN:   _rankInt = 3; break;
		case RANK_COLONEL:   _rankInt = 4; break;
		case RANK_COMMANDER: _rankInt = 5; break;
		default:             _rankInt = 0; break;
		}
	}
}

/**
* this function checks if a tile is visible from either of the unit's tiles, using maths.
* @param pos the position to check against
* @param useTurretDirection use turret facing (true) or body facing (false) for sector calculation
* @return what the maths decide
*/
bool BattleUnit::checkViewSector (Position pos, bool useTurretDirection /* = false */) const
{
	int unitSize = getArmor()->getSize();
	//Check view cone from each of the unit's tiles
	for (int x = 0; x < unitSize; ++x)
	{
		for (int y = 0; y < unitSize; ++y)
		{
			int deltaX = pos.x + x - _pos.x;
			int deltaY = _pos.y - pos.y - y;
			switch (useTurretDirection ? _directionTurret : _direction)
			{
			case 0:
				if ((deltaX + deltaY >= 0) && (deltaY - deltaX >= 0))
					return true;
				break;
			case 1:
				if ((deltaX >= 0) && (deltaY >= 0))
					return true;
				break;
			case 2:
				if ((deltaX + deltaY >= 0) && (deltaY - deltaX <= 0))
					return true;
				break;
			case 3:
				if ((deltaY <= 0) && (deltaX >= 0))
					return true;
				break;
			case 4:
				if ((deltaX + deltaY <= 0) && (deltaY - deltaX <= 0))
					return true;
				break;
			case 5:
				if ((deltaX <= 0) && (deltaY <= 0))
					return true;
				break;
			case 6:
				if ((deltaX + deltaY <= 0) && (deltaY - deltaX >= 0))
					return true;
				break;
			case 7:
				if ((deltaY >= 0) && (deltaX <= 0))
					return true;
				break;
			default:
				break;
			}
		}
	}
	return false;
}

/**
 * common function to adjust a unit's stats according to difficulty setting.
 * @param statAdjustment the stat adjustment variables coefficient value.
 */
void BattleUnit::adjustStats(const StatAdjustment &adjustment)
{
	_stats.tu += adjustment.statGrowth.tu * adjustment.growthMultiplier * _stats.tu / 100;
	_stats.stamina += adjustment.statGrowth.stamina * adjustment.growthMultiplier * _stats.stamina / 100;
	_stats.health += adjustment.statGrowth.health * adjustment.growthMultiplier * _stats.health / 100;
	_stats.bravery += adjustment.statGrowth.bravery * adjustment.growthMultiplier * _stats.bravery / 100;
	_stats.reactions += adjustment.statGrowth.reactions * adjustment.growthMultiplier * _stats.reactions / 100;
	_stats.firing += adjustment.statGrowth.firing * adjustment.growthMultiplier * _stats.firing / 100;
	_stats.throwing += adjustment.statGrowth.throwing * adjustment.growthMultiplier * _stats.throwing / 100;
	_stats.strength += adjustment.statGrowth.strength * adjustment.growthMultiplier * _stats.strength / 100;
	_stats.psiStrength += adjustment.statGrowth.psiStrength * adjustment.growthMultiplier * _stats.psiStrength / 100;
	_stats.psiSkill += adjustment.statGrowth.psiSkill * adjustment.growthMultiplier * _stats.psiSkill / 100;
	_stats.melee += adjustment.statGrowth.melee * adjustment.growthMultiplier * _stats.melee / 100;
	
	_stats.firing *= adjustment.aimAndArmorMultiplier;
	_maxArmor[0] *= adjustment.aimAndArmorMultiplier;
	_maxArmor[1] *= adjustment.aimAndArmorMultiplier;
	_maxArmor[2] *= adjustment.aimAndArmorMultiplier;
	_maxArmor[3] *= adjustment.aimAndArmorMultiplier;
	_maxArmor[4] *= adjustment.aimAndArmorMultiplier;
}

/**
 * did this unit already take fire damage this turn?
 * (used to avoid damaging large units multiple times.)
 * @return ow it burns
 */
bool BattleUnit::tookFireDamage() const
{
	return _hitByFire;
}

/**
 * toggle the state of the fire damage tracking boolean.
 */
void BattleUnit::toggleFireDamage()
{
	_hitByFire = !_hitByFire;
}

/**
 * Changes the amount of TUs reserved for cover.
 * @param reserve time units.
 */
void BattleUnit::setCoverReserve(int reserve)
{
	_coverReserve = reserve;
}

/**
 * Returns the amount of TUs reserved for cover.
 * @return time units.
 */
int BattleUnit::getCoverReserve() const
{
	return _coverReserve;
}

/**
 * Checks if this unit can be selected. Only alive units
 * belonging to the faction can be selected.
 * @param faction The faction to compare with.
 * @param checkReselect Check if the unit is reselectable.
 * @param checkInventory Check if the unit has an inventory.
 * @return True if the unit can be selected, false otherwise.
 */
bool BattleUnit::isSelectable(UnitFaction faction, bool checkReselect, bool checkInventory) const
{
	return (_faction == faction && !isOut() && (!checkReselect || reselectAllowed()) && (!checkInventory || hasInventory()));
}

/**
 * Checks if this unit has an inventory. Large units and/or
 * terror units generally don't have inventories.
 * @return True if an inventory is available, false otherwise.
 */
bool BattleUnit::hasInventory() const
{
	return (_armor->hasInventory());
	//return (_geoscapeSoldier || (_armor->getSize() == 1 && _rank != "STR_LIVE_TERRORIST"));
}

/**
 * If this unit is breathing, what frame should be displayed?
 * @return frame number.
 */
int BattleUnit::getBreathFrame() const
{
	if (_floorAbove)
		return 0;
	return _breathFrame;
}

bool BattleUnit::onOverwatch() const
{
	return getOngoingAction() == BA_OVERWATCH;
}

/**
 * Decides if we should start producing bubbles, and/or updates which bubble frame we are on.
 */
void BattleUnit::breathe()
{
	// _breathFrame of -1 means this unit doesn't produce bubbles
	if (_breathFrame < 0 || isOut())
	{
		_breathing = false;
		return;
	}

	if (!_breathing || _status == STATUS_WALKING)
	{
		// deviation from original: TFTD used a static 10% chance for every animation frame,
		// instead let's use 5%, but allow morale to affect it.
		_breathing = (_status != STATUS_WALKING && RNG::seedless(0, 99) < (105 - _morale));
		_breathFrame = 0;
	}

	if (_breathing)
	{
		// advance the bubble frame
		_breathFrame++;

		// we've reached the end of the cycle, get rid of the bubbles
		if (_breathFrame >= 17)
		{
			_breathFrame = 0;
			_breathing = false;
		}
	}
}

/**
 * Sets the flag for "this unit is under cover" meaning don't draw bubbles.
 * @param floor is there a floor.
 */
void BattleUnit::setFloorAbove(bool floor)
{
	_floorAbove = floor;
}

/**
 * Checks if the floor above flag has been set.
 * @return if we're under cover.
 */
bool BattleUnit::getFloorAbove() const
{
	return _floorAbove;
}

/**
 * Get the name of any utility weapon we may be carrying, or a built in one.
 * @return the name .
 */
BattleItem *BattleUnit::getUtilityWeapon(BattleType type)
{
	BattleItem *melee = getItem(_weaponSlot1);
	if (melee && melee->getRules()->getBattleType() == type)
	{
		return melee;
	}
	melee = getItem(_weaponSlot2);
	if (melee && melee->getRules()->getBattleType() == type)
	{
		return melee;
	}
	melee = getSpecialWeapon(type);
	if (melee)
	{
		return melee;
	}
	return 0;
}

/**
 * Set fire damage from environment.
 * @param damage
 */
void BattleUnit::setEnviFire(int damage)
{
	if (_fireMaxHit < damage) _fireMaxHit = damage;
}

/**
 * Set smoke damage from environment.
 * @param damage
 */
void BattleUnit::setEnviSmoke(int damage)
{
	if (_smokeMaxHit < damage) _smokeMaxHit = damage;
}

/**
 * Calculate smoke and fire damage from environment.
 */
void BattleUnit::calculateEnviDamage(Mod *mod, SavedBattleGame *save)
{
	if (_fireMaxHit)
	{
		_hitByFire = true;
		damage(0, 0, Position(0, 0, 0), _fireMaxHit, mod->getDamageType(DT_IN), save, { });
		// try to set the unit on fire.
		if (RNG::percent(40 * getArmor()->getDamageModifier(DT_IN)))
		{
			int burnTime = RNG::generate(0, int(5.0f * getArmor()->getDamageModifier(DT_IN)));
			if (getFire() < burnTime)
			{
				setFire(burnTime);
			}
		}
	}

	if (_smokeMaxHit)
	{
		damage(0, 0, Position(0,0,0), _smokeMaxHit, mod->getDamageType(DT_SMOKE), save, { });
	}

	_fireMaxHit = 0;
	_smokeMaxHit = 0;
}

/**
 * use this instead of checking the rules of the armor.
 */
MovementType BattleUnit::getMovementType() const
{
	return _movementType;
}

/**
 * Elevates the unit to grand galactic inquisitor status,
 * meaning they will NOT take part in the current battle.
 */
void BattleUnit::goToTimeOut()
{
	_status = STATUS_IGNORE_ME;
}

/**
 * Set special weapon that is handled outside inventory.
 * @param save
 */
void BattleUnit::setSpecialWeapon(SavedBattleGame *save)
{
	const Mod *mod = save->getMod();
	RuleItem *item = 0;
	int i = 0;

	if (getUnitRules())
	{
		item = mod->getItem(getUnitRules()->getMeleeWeapon());
		if (item && i < SPEC_WEAPON_MAX)
		{
			_specWeapon[i++] = save->createItemForUnitBuildin(item, this);
		}
	}
	item = mod->getItem(getArmor()->getSpecialWeapon());
	if (item && i < SPEC_WEAPON_MAX)
	{
		_specWeapon[i++] = save->createItemForUnitBuildin(item, this);
	}
	if (getBaseStats()->psiSkill > 0 && getOriginalFaction() == FACTION_HOSTILE)
	{
		item = mod->getItem(getUnitRules()->getPsiWeapon());
		if (item && i < SPEC_WEAPON_MAX)
		{
			_specWeapon[i++] = save->createItemForUnitBuildin(item, this);
		}
	}
}

/**
 * Get special weapon.
 */
BattleItem *BattleUnit::getSpecialWeapon(BattleType type) const
{
	for (int i = 0; i < SPEC_WEAPON_MAX; ++i)
	{
		if (!_specWeapon[i])
		{
			break;
		}
		if (_specWeapon[i]->getRules()->getBattleType() == type)
		{
			return _specWeapon[i];
		}
	}
	return 0;
}

/**
 * Recovers a unit's TUs and energy, taking a number of factors into consideration.
 */
void BattleUnit::recoverTimeUnits()
{
	recoverStats(true, false);
}

/**
 * Get the unit's statistics.
 * @return BattleUnitStatistics statistics.
 */
BattleUnitStatistics* BattleUnit::getStatistics()
{
	return _statistics;
}

/**
 * Sets the unit murderer's id.
 * @param int murderer id.
 */
void BattleUnit::setMurdererId(int id)
{
	_murdererId = id;
}

/**
 * Gets the unit murderer's id.
 * @return int murderer id.
 */
int BattleUnit::getMurdererId() const
{
	return _murdererId;
}

/**
 * Set information on the unit's fatal blow.
 * @param UnitSide unit's side that was shot.
 * @param UnitBodyPart unit's body part that was shot.
 */
void BattleUnit::setFatalShotInfo(UnitSide side, UnitBodyPart bodypart)
{
	_fatalShotSide = side;
	_fatalShotBodyPart = bodypart;
}

/**
 * Get information on the unit's fatal shot's side.
 * @return UnitSide fatal shot's side.
 */
UnitSide BattleUnit::getFatalShotSide() const
{
	return _fatalShotSide;
}

/**
 * Get information on the unit's fatal shot's body part.
 * @return UnitBodyPart fatal shot's body part.
 */
UnitBodyPart BattleUnit::getFatalShotBodyPart() const
{
	return _fatalShotBodyPart;
}

/**
 * Gets the unit murderer's weapon.
 * @return int murderer weapon.
 */
std::string BattleUnit::getMurdererWeapon() const
{
	return _murdererWeapon;
}

/**
 * Set the murderer's weapon.
 * @param string murderer's weapon.
 */
void BattleUnit::setMurdererWeapon(const std::string& weapon)
{
	_murdererWeapon = weapon;
}

/**
 * Gets the unit murderer's weapon's ammo.
 * @return int murderer weapon ammo.
 */
std::string BattleUnit::getMurdererWeaponAmmo() const
{
	return _murdererWeaponAmmo;
}

/**
 * Set the murderer's weapon's ammo.
 * @param string murderer weapon ammo.
 */
void BattleUnit::setMurdererWeaponAmmo(const std::string& weaponAmmo)
{
	_murdererWeaponAmmo = weaponAmmo;
}

/**
 * Sets the unit mind controller's id.
 * @param int mind controller id.
 */
void BattleUnit::setMindControllerId(int id)
{
	_mindControllerID = id;
}

/**
 * Gets the unit mind controller's id.
 * @return int mind controller id.
 */
int BattleUnit::getMindControllerId() const
{
	return _mindControllerID;
}

void BattleUnit::activateOverwatch(SavedBattleGame *save, BattleItem *weapon, const Position& target)
{
	_overwatch = true;
	setOngoingAction(BA_OVERWATCH);
	_overwatchWeaponSlot = getItem(_weaponSlot2) == weapon ? _weaponSlot2 : _weaponSlot1;
	_overwatchTarget = target;
	_tu = 0;

	int overwatchShotCost = getActionTUs(getOverwatchShotAction(weapon->getRules()), weapon).Time; //std::max(getActionTUs(getOverwatchShotAction(weapon), weapon), (int)(floor(getStats()->tu * 0.9f)));
	_overwatchShotsAvailable = 1 + (_tu / overwatchShotCost);
}


void BattleUnit::clearOverwatch()
{
	_overwatchShotsAvailable = 0;
	_overwatch = false;
	clearOngoingAction(BA_OVERWATCH);
}

const Position& BattleUnit::getOverwatchTarget() const
{
	return _overwatchTarget;
}

BattleItem *BattleUnit::getOverwatchWeapon() const
{
	return _overwatch ? getItem(_overwatchWeaponSlot) : 0;
}

bool BattleUnit::tryFireOverwatch()
{
	if(_overwatch && _overwatchShotsAvailable)
	{
		--_overwatchShotsAvailable;
		return true;
	}
	return false;
}

void BattleUnit::cancelOverwatchShot()
{
	if (_overwatch)
	{
		++_overwatchShotsAvailable;
	}
}

BattleActionType BattleUnit::getOverwatchShotAction(const RuleItem *weapon) const
{
	if(!weapon)
	{
		BattleItem *weaponItem = getOverwatchWeapon();
		if(weaponItem)
		{
			weapon = weaponItem->getRules();
		}
	}

	if(weapon)
	{
		return weapon->getOverwatchShotAction();
	}

	return BA_NONE;
}

void BattleUnit::setJustKilled(bool justKilled)
{
	_justKilled = justKilled;
}

bool BattleUnit::getJustKilled() const
{
	return _justKilled;
}

bool BattleUnit::getCanBleedOut() const
{
	return (_originalFaction == FACTION_PLAYER) && _geoscapeSoldier && !_geoscapeSoldier->isVehicle();
}

bool BattleUnit::getBleedingOut() const
{
	return _bleedingOut && getCanBleedOut() && (getHealth() <= 0) && (getHealth() > getDeathHealth()) && getFatalWounds();
}

int BattleUnit::getDeathHealth() const
{
	return getCanBleedOut() ? _stats.health / -2 : 0;
}

bool BattleUnit::checkSquadSight(SavedBattleGame *save, BattleUnit* target, bool visibleOnly) const
{
	for (std::vector<BattleUnit*>::const_iterator ii = save->getUnits()->begin(); ii != save->getUnits()->end(); ++ii)
	{
		BattleUnit* unit = *ii;

		if(unit->getFaction() == getFaction())
		{
			const std::vector<BattleUnit*> &visibleUnits = visibleOnly ? (*unit->getVisibleUnits()) : unit->getUnitsSpottedThisTurn();
			if(std::find(visibleUnits.begin(), visibleUnits.end(), target) != visibleUnits.end())
			{
				return true;
			}
		}
	}

	return false;
}

BattleActionType BattleUnit::getOngoingAction() const
{
	return _ongoingAction;
}

void BattleUnit::setOngoingAction(BattleActionType action)
{
	_ongoingAction = action;
}

void BattleUnit::clearOngoingAction(BattleActionType actionCheck)
{
	if(actionCheck == BA_NONE || actionCheck == _ongoingAction)
	{
		_ongoingAction = BA_NONE;
	}
}

BattleUnit* BattleUnit::getControlledBy() const
{
	return _controller;
}

void BattleUnit::setControlledBy(BattleUnit *controller)
{
	if(_controller == controller) return;

	if(_controller)
	{
		BattleUnit *old = _controller;
		_controller = 0;
		_faction = _originalFaction;
		old->clearControlling();
	}

	if(controller && _originalFaction != controller->getFaction())
	{
		clearControlling();
		_controller = controller;
		convertToFaction(_controller->getFaction());
	}

	clearOngoingAction(BA_OVERWATCH);
}

BattleUnit* BattleUnit::getControlling() const
{
	return _controlling;
}

void BattleUnit::clearControlledBy()
{
	setControlledBy(0);
}

void BattleUnit::setControlling(BattleUnit *controlling)
{
	if(_controlling == controlling) return;

	if(_controlling)
	{
		BattleUnit *old = _controlling;
		_controlling = 0;
		old->setControlledBy(0);
		clearOngoingAction(BA_MINDCONTROL);
	}

	if(controlling)
	{
		if(controlling->getOriginalFaction() != getFaction())
		{
			_controlling = controlling;
			_controlling->setControlledBy(this);
			setOngoingAction(BA_MINDCONTROL);
		}
		else
		{
			controlling->setControlledBy(0);
		}
	}
}

void BattleUnit::clearControlling()
{
	setControlling(0);
}

/**
 * Returns if the unit is a vehicle.
 * @return True if the unit is a vehicle soldier.
 */
bool BattleUnit::isVehicle() const
{
	return _isVehicle;
}

bool BattleUnit::findTurretType()
{
	BattleItem *weapon1 = getItem(_weaponSlot1);
	BattleItem *weapon2 = getItem(_weaponSlot2);

	int turretType = -1;
	if (weapon1 && weapon1->getRules() && ((turretType = weapon1->getRules()->getTurretType()) > -1))
	{
		setTurretType(turretType);
		return true;
	}
	else if (weapon2 && weapon2->getRules() && ((turretType = weapon2->getRules()->getTurretType()) > -1))
	{
		setTurretType(turretType);
		return true;
	}

	return false;
}

void BattleUnit::updateStats(BattleItem *draggingItem)
{
	_stats = calculateStats(draggingItem);
	if(!_inCombat)
	{
		_tu = _stats.tu;
		_energy = _stats.stamina;
		_health = _stats.health;
	}
	/*else
	{
		if(_tu > _stats.tu) _tu = _stats.tu;
		if(_energy > _stats.stamina) _energy = _stats.stamina;
		if(_health > _stats.health) _health = _stats.health;
	}*/
	updateArmor(draggingItem);
}

UnitStats BattleUnit::calculateStats(BattleItem *draggingItem) const
{
	UnitStats stats = _baseStats;

	UnitStats modifiers(*_armor->getStatModifiers());

	for(std::vector<BattleItem*>::const_iterator ii = _inventory.begin(); ii != _inventory.end(); ++ii)
	{
		BattleItem *item = *ii;
		RuleInventory *slot = item->getSlot();
		if(item != draggingItem && slot && slot->getCountStats())
		{
			const RuleItem *rule = item->getRules();
			if(rule->hasStats())
			{
				 stats += *rule->getStats();
				 modifiers += *rule->getStatModifiers();
			}
		}
	}

	stats *= modifiers;

	int weight = getCarriedWeight(draggingItem);

	if(stats.bravery < 0) { stats.bravery = 0; }
	if(stats.firing < 0) { stats.firing = 0; }
	if(stats.health < 0) { stats.health = 0; }
	if(stats.melee < 0) { stats.melee = 0; }
	if(stats.psiSkill < 0) { stats.psiSkill = 0; }
	if(stats.psiStrength < 0) { stats.psiStrength = 0; }
	if(stats.reactions < 0) { stats.reactions = 0; }
	if(stats.stamina < 0) { stats.stamina = 0; }
	if(stats.strength < 0) { stats.strength = 0; }
	if(stats.throwing < 0) { stats.throwing = 0; }
	if(stats.tu < 0) { stats.tu = 0; }

	if(isVehicle())
	{
		// Power to Weight Ratio Formula
		float pwr = weight > 0 ? (float)stats.strength / (float)weight : (stats.strength ? 1.0f : 0.0f);
		stats.tu = (int)(pwr * (float)stats.tu);

		// Linear Power
		/*float pwr = (float)(stats.strength - weight) / (float)stats.strength;
		stats.tu = (int)(std::max(std::min(1.0f, pwr), 0.0f) * stats.tu);*/
	}
	else
	{
		stats.tu *= std::min((double)stats.strength / (weight * Mod::ENCUMBRANCE_MULTIPLIER), 1.0);
	}

	return stats;
}

const std::string &BattleUnit::getInventoryLayout() const
{
	return (_armor && _armor->getInventoryLayout().size()) ? _armor->getInventoryLayout() : _inventoryLayout;
}

void BattleUnit::updateArmor(BattleItem *draggingItem)
{
	calculateArmor(_maxArmor, draggingItem);
	if(!_inCombat)
	{
		_currentArmor[SIDE_FRONT] = _maxArmor[SIDE_FRONT];
		_currentArmor[SIDE_LEFT] = _maxArmor[SIDE_LEFT];
		_currentArmor[SIDE_RIGHT] = _maxArmor[SIDE_RIGHT];
		_currentArmor[SIDE_REAR] = _maxArmor[SIDE_REAR];
		_currentArmor[SIDE_UNDER] = _maxArmor[SIDE_UNDER];
	}
}

void BattleUnit::calculateArmor(int *armor, BattleItem *draggingItem)
{
	armor[SIDE_FRONT] = _armor->getFrontArmor();
	armor[SIDE_LEFT] = _armor->getLeftSideArmor();
	armor[SIDE_RIGHT] = _armor->getRightSideArmor();
	armor[SIDE_REAR] = _armor->getRearArmor();
	armor[SIDE_UNDER] = _armor->getUnderArmor();

	for(std::vector<BattleItem*>::const_iterator ii = _inventory.begin(); ii != _inventory.end(); ++ii)
	{
		BattleItem *item = *ii;
		RuleInventory *slot = item->getSlot();
		if(item != draggingItem && slot && slot->getBattleType() == BT_ARMOR)
		{
			const RuleItem *rule = item->getRules();
			switch(slot->getArmorSide())
			{
			case -1:
				armor[SIDE_FRONT] += rule->getFrontArmor();
				armor[SIDE_LEFT] += rule->getLeftArmor();
				armor[SIDE_RIGHT] += rule->getRightArmor();
				armor[SIDE_REAR] += rule->getRearArmor();
				armor[SIDE_UNDER] += rule->getUnderArmor();
				break;
			case SIDE_FRONT:
				armor[SIDE_FRONT] += rule->getFrontArmor();
				break;
			case SIDE_LEFT:
				armor[SIDE_LEFT] += rule->getLeftArmor();
				break;
			case SIDE_RIGHT:
				armor[SIDE_RIGHT] += rule->getRightArmor();
				break;
			case SIDE_REAR:
				armor[SIDE_REAR] += rule->getRearArmor();
				break;
			case SIDE_UNDER:
				armor[SIDE_UNDER] += rule->getUnderArmor();
				break;
			}
		}
	}
}

bool BattleUnit::canDualFire() const
{
	BattleItem *weapon1;
	BattleItem *weapon2;
	BattleActionType action1;
	BattleActionType action2;
	int tu1;
	int tu2;
	return canDualFire(weapon1, action1, tu1, weapon2, action2, tu2);
}

bool BattleUnit::canDualFire(BattleItem *&weapon1, BattleItem *&weapon2) const
{
	BattleActionType action1;
	BattleActionType action2;
	int tu1;
	int tu2;
	return canDualFire(weapon1, action1, tu1, weapon2, action2, tu2);
}

bool BattleUnit::canDualFire(BattleItem *&weapon1, BattleActionType &action1, BattleItem *&weapon2, BattleActionType &action2) const
{
	int tu1;
	int tu2;
	return canDualFire(weapon1, action1, tu1, weapon2, action2, tu2);
}

bool BattleUnit::canDualFire(BattleItem *&weapon1, BattleActionType &action1, int &tu1, BattleItem *&weapon2, BattleActionType &action2, int &tu2) const
{
	int firearms = 0;
	weapon1 = 0;
	weapon2 = 0;
	action1 = BA_NONE;
	action2 = BA_NONE;
	tu1 = 0;
	tu2 = 0;

	for (std::vector<BattleItem*>::const_iterator ii = _inventory.begin(); ii != _inventory.end(); ++ii)
	{
		BattleItem *item = *ii;
		RuleInventory *slot = item->getSlot();

		if (item->getRules()->getBattleType() == BT_FIREARM
			&& item->getRules()->getWaypoints() < 2
			&& slot->getType() == INV_HAND
			&& (!item->haveAnyAmmo() || item->haveAnyAmmo()))
		{
			switch (++firearms)
			{
			case 1:
				if (tu1 = item->getRules()->getCostAuto().Time)
				{
					weapon1 = item;
					action1 = BA_AUTOSHOT;
				}
				else if(tu1 = item->getRules()->getCostBurst().Time)
				{
					weapon1 = item;
					action1 = BA_BURSTSHOT;
				}
				else if (tu1 = item->getRules()->getCostAimed().Time)
				{
					weapon1 = item;
					action1 = BA_AIMEDSHOT;
				}
				else if (tu1 = item->getRules()->getCostSnap().Time)
				{
					weapon1 = item;
					action1 = BA_SNAPSHOT;
				}
				else
				{
					weapon1 = 0;
					tu1 = 0;
					action1 = BA_NONE;
				}
				break;
			case 2:
				if (tu2 = item->getRules()->getCostAuto().Time)
				{
					weapon2 = item;
					action2 = BA_AUTOSHOT;
				}
				else if (tu2 = item->getRules()->getCostBurst().Time)
				{
					weapon2 = item;
					action2 = BA_BURSTSHOT;
				}
				else if (tu2 = item->getRules()->getCostAimed().Time)
				{
					weapon2 = item;
					action2 = BA_AIMEDSHOT;
				}
				else if (tu2 = item->getRules()->getCostSnap().Time)
				{
					weapon2 = item;
					action2 = BA_SNAPSHOT;
				}
				else
				{
					weapon2 = 0;
					tu2 = 0;
					action2 = BA_NONE;
				}

				return weapon1 && weapon2;
			}
		}
	}

	return false;
}

double BattleUnit::getDualFireAccuracy() const
{
	return isVehicle() ? 0.9 : 0.4;
}

void BattleUnit::determineWeaponSlots()
{
	RuleInventoryLayout *inventoryLayout = Game::getMod()->getInventoryLayout(getInventoryLayout());
	if(inventoryLayout)
	{
		_weaponSlot1 = "";
		_weaponSlot2 = "";
		_utilitySlot = "";
		int handSlotsFound = 0;

		for(std::vector<RuleInventory*>::const_iterator ii = inventoryLayout->getSlots()->begin(); ii != inventoryLayout->getSlots()->end(); ++ii)
		{
			RuleInventory *inv = *ii;
			if(inv->getType() == INV_HAND)
			{
				switch(handSlotsFound)
				{
				case 0:
					_weaponSlot1 = inv->getId();
					break;
				case 1:
					_weaponSlot2 = inv->getId();
					break;
				}

				++handSlotsFound;
			}
			else if (inv->getType() == INV_UTILITY && !_utilitySlot.size())
			{
				_utilitySlot = inv->getId();
			}
		}
	}
	else
	{
		_weaponSlot1 = "STR_RIGHT_HAND";
		_weaponSlot2 = "STR_LEFT_HAND";
		_utilitySlot = "";
	}
}

const std::string &BattleUnit::getWeaponSlot1() const
{
	return _weaponSlot1;
}

BattleItem *BattleUnit::getWeapon1() const
{
	return getItem(_weaponSlot1);
}

const std::string &BattleUnit::getWeaponSlot2() const
{
	return _weaponSlot2;
}

BattleItem *BattleUnit::getWeapon2() const
{
	return getItem(_weaponSlot2);
}

const std::string &BattleUnit::getUtilitySlot() const
{
	return _utilitySlot;
}

BattleItem *BattleUnit::getUtilityItem() const
{
	return getItem(_utilitySlot);
}

/**
 * Moves an item to a specified slot in the
 * selected player's inventory.
 * @param item Pointer to battle item.
 * @param slot Inventory slot, or NULL if none.
 * @param x X position in slot.
 * @param y Y position in slot.
 * @param warning Warning message to return if move fails.
 * @param checkTu Sets whether to test/spend TUs or not.
 * @param testing If true, no actual changes will be executed.
 * @param cost The expected/spent cost of the move.
 * @return Returns true if the move was (or would be) successful.
 */
bool BattleUnit::moveItem(BattleItem *item, RuleInventory *slot, int x, int y, std::string &warning, bool checkTu, bool testing, int *returnCost)
{
	// Make items vanish (eg. ammo in weapons)
	if (slot == 0)
	{
		if(returnCost)
		{
			*returnCost = 0;
		}

		if(!testing)
		{
			if (item->getSlot()->getType() == INV_GROUND)
			{
				getTile()->removeItem(item);
			}
			else
			{
				item->moveToOwner(0);
			}
		}
		return true;
	}
	/*else if(item->getSlot() == slot)
	{
		if(returnCost)
		{
			*returnCost = 0;
		}
		return true;
	}*/
	else
	{
		RuleInventory *currentSlot = item->getSlot();
		int cost = currentSlot ? currentSlot->getCost(slot) : 0;

		BattleItem *weapon = (slot->getType() == INV_HAND || slot->getType() == INV_UTILITY || slot->getType() == INV_EQUIP) ? this->getItem(slot->getId()) : 0;
		if(weapon && weapon != item && loadWeapon(weapon, item, warning, false, true, 0, true))
		{
			return loadWeapon(weapon, item, warning, checkTu, testing, returnCost);
		}

		if (returnCost)
		{
			*returnCost = std::max(cost, 0);
		}

		// Target slot or current slot invalid for combat swap.
		if (_inCombat && !item->isAmmo() && (!slot->getAllowCombatSwap() || (currentSlot && !currentSlot->getAllowCombatSwap())))
		{
			warning = "STR_NOT_COMBAT_SWAPPABLE";
			return false;
		}
		// -1 cost = invalid slot-slot transfer.
		else if (_inCombat && cost < 0 && !item->isAmmo())
		{
			warning = "STR_INVALID_TRANSFER";
			return false;
		}

		// Clear no-transfer cost after checking it.
		if(cost < 0) cost = 0;

		// Failed to fit into slot.
		if (slot->getType() != INV_GROUND && ((x < 0 || y < 0) ? !fitItem(item, slot, x, y) : (overlapItems(item, slot, x, y) || !slot->fitItemInSlot(item->getRules(), x, y))))
		{
			warning = "STR_NOT_ENOUGH_SPACE";
			return false;
		}
		// Target slot is not valid for selected item.
		else if (slot->getType() != INV_GROUND && !item->getRules()->canBePlacedIntoInventorySection(slot))
		{
			warning = "STR_INVALID_ITEM_SLOT";
			return false;
		}
		// Insufficient TUs.
		else if (_inCombat && checkTu && (testing ? (cost > getAvailableTimeUnits()) : !spendTimeUnits(cost)))
		{
			warning = "STR_NOT_ENOUGH_TIME_UNITS";
			return false;
		}
		// All checks passed, early exit for test mode.
		else if (testing)
		{
			return true;
		}
		else if (slot != currentSlot || item->getOwner() != this)
		{
			if (slot->getType() == INV_GROUND)
			{
				if (currentSlot == 0 || currentSlot->getType() != INV_GROUND)
				{
					item->moveToOwner(0);
					getTile()->addItem(item, slot);
					if (item->getUnit() && item->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
					{
						item->getUnit()->setPosition(getPosition());
					}
				}

				item->setSlot(slot);
				item->setSlotX(x);
				item->setSlotY(y);
				updateStats();
				return true;
			}
			else if (currentSlot == 0 || item->getOwner() != this || currentSlot->getType() == INV_GROUND)
			{
				// Call setSlot early to ensure moveToOwner can update stats properly.
				item->setSlot(slot);
				item->setSlotX(x);
				item->setSlotY(y);
				item->moveToOwner(this);
				getTile()->removeItem(item);
				item->setTurnFlag(false);
				if (item->getUnit() && item->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
				{
					item->getUnit()->setPosition(Position(-1,-1,-1));
				}

				// Return early to skip later setSlot calls.
				return true;
			}
		}

		item->setSlot(slot);
		item->setSlotX(x);
		item->setSlotY(y);
		updateStats();
		return true;
	}
}

/**
 * Attempts to place the item in the inventory slot.w
 * @param newSlot Where to place the item.
 * @param item Item to be placed.
 * @param warning Warning message if item could not be placed.
 * @return True, if the item was successfully placed in the inventory.
 */
bool BattleUnit::fitItem(BattleItem *item, RuleInventory *newSlot, int &x, int &y)
{
	if (newSlot->getType() == INV_HAND || newSlot->getType() == INV_UTILITY || newSlot->getType() == INV_EQUIP)
	{
		if (!overlapItems(item, newSlot, 0, 0) && newSlot->fitItemInSlot(item->getRules(), 0, 0))
		{
			x = 0;
			y = 0;
			return true;
		}
	}
	else
	{
		std::vector<RuleSlot> *slots = newSlot->getSlots();

		for(std::vector<RuleSlot>::const_iterator ii = slots->begin(); ii != slots->end(); ++ii)
		{
			if (!overlapItems(item, newSlot, ii->x, ii->y) && newSlot->fitItemInSlot(item->getRules(), ii->x, ii->y))
			{
				x = ii->x;
				y = ii->y;
				return true;
			}
		}
	}
	return false;
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
bool BattleUnit::overlapItems(BattleItem *item, RuleInventory *slot, int x, int y)
{
	if (slot->getType() != INV_GROUND)
	{
		for (std::vector<BattleItem*>::const_iterator i = getInventory()->begin(); i != getInventory()->end(); ++i)
		{
			if (item != (*i) && (*i)->getSlot() == slot && (*i)->occupiesSlot(x, y, item))
			{
				return true;
			}
		}
	}
	else if (getTile() != 0)
	{
		for (std::vector<BattleItem*>::const_iterator i = getTile()->getInventory()->begin(); i != getTile()->getInventory()->end(); ++i)
		{
			if ((*i)->occupiesSlot(x, y, item))
			{
				return true;
			}
		}
	}
	return false;
}

bool BattleUnit::unloadWeapon(BattleItem *weapon, std::string &warning, bool checkTu, bool testing, int *returnCost)
{
	BattleItem *ammo = weapon->getAmmoForSlot(0);
	if(!ammo)
	{
		if(returnCost)
		{
			*returnCost = 0;
		}
		return true;
	}

	if(ammo == weapon)
	{
		return false;
	}

	ammo->setSlot(weapon->getSlot());

	const std::vector<RuleInventory*> *slots = Game::getMod()->getInventoryLayout(this->getInventoryLayout())->getSlots();

	RuleInventory *ground = Game::getMod()->getInventory("STR_GROUND");

	RuleInventory *sourceSlot = weapon->getSlot();

	if(sourceSlot != ground)
	{
		if(isVehicle())
		{
			for(std::vector<RuleInventory*>::const_iterator ii = slots->begin(); ii != slots->end(); ++ii)
			{
				RuleInventory *inv = *ii;
				if(inv != sourceSlot && inv->getBattleType() == BT_AMMO && moveItem(ammo, inv, -1, -1, warning, checkTu, testing, returnCost))
				{
					warning = "";
					if(!testing)
					{
						weapon->setAmmoForSlot(0, nullptr);
					}
					return true;
				}
			}
		}

		for(std::vector<RuleInventory*>::const_iterator ii = slots->begin(); ii != slots->end(); ++ii)
		{
			RuleInventory *inv = *ii;
			int cost = 0;
			if (inv != sourceSlot && (inv->getType() == INV_HAND || inv->getType() == INV_UTILITY || inv->getType() == INV_EQUIP) && moveItem(ammo, inv, -1, -1, warning, checkTu, testing, returnCost))
			{
				warning = "";
				if(!testing)
				{
					weapon->setAmmoForSlot(0, nullptr);
				}
				return true;
			}
		}

		if(!isVehicle())
		{
			warning = "STR_BOTH_HANDS_MUST_BE_EMPTY";
			return false;
		}
	}

	if(sourceSlot == ground)
	{
		ammo->setSlot(0);
	}

	if(moveItem(ammo, ground, -1, -1, warning, checkTu, testing, returnCost))
	{
		warning = "";
		if(!testing)
		{
			weapon->setAmmoForSlot(0, nullptr);
		}
		return true;
	}

	return false;
}

bool BattleUnit::loadWeapon(BattleItem *weapon, BattleItem *ammo, std::string &warning, bool checkTu, bool testing, int *returnCost, bool ignoreLoaded)
{
	int cost = getLoadCost(weapon, ammo);

	if (_inCombat && cost < 0)
	{
		if(returnCost)
		{
			*returnCost = 0;
		}
		warning = "STR_INVALID_TRANSFER";
		return false;
	}

	if(cost < 0) cost = 0;

	if(returnCost)
	{
		*returnCost = cost;
	}

	bool validAmmo = false;
	const std::vector<std::string> *ammos = weapon->getRules()->getPrimaryCompatibleAmmo();
	for(std::vector<std::string>::const_iterator ii = ammos->begin(); ii != ammos->end(); ++ii)
	{
		if((*ii) == ammo->getRules()->getType())
		{
			validAmmo = true;
			break;
		}
	}
	
	if(!validAmmo)
	{
		warning = "STR_WRONG_AMMUNITION_FOR_THIS_WEAPON";
		return false;
	}

	if (weapon->getAmmoForSlot(0) && !ignoreLoaded)
	{
		warning = "STR_WEAPON_IS_ALREADY_LOADED";
		return false;
	}

	/*if(!moveItem(ammo, weapon->getSlot(), -1, -1, warning, false, true))
	{
		return false;
	}*/

	if (checkTu && (testing ? cost > getAvailableTimeUnits() : !spendTimeUnits(cost)))
	{
		warning = "STR_NOT_ENOUGH_TIME_UNITS";
		return false;
	}

	if(!testing)
	{
		if (ammo->getSlot()->getType() == INV_GROUND)
		{
			getTile()->removeItem(ammo);
		}

		ammo->moveToOwner(0);
		ammo->setSlot(weapon->getSlot());
		weapon->setAmmoForSlot(0, ammo);
	}

	return true;
}

RuleItemUseCost BattleUnit::getReloadCost(BattleItem *weapon) const
{
	RuleItemUseCost cost;
	std::string warning;
	const_cast<BattleUnit*>(this)->reloadWeapon(weapon, warning, true, true, &cost.Time);
	return cost;
}

bool BattleUnit::reloadWeapon(BattleItem *weapon, std::string &warning, bool checkTu, bool testing, int *returnCost, BattleItem **reloadedAmmo)
{
	BattleItem *ammo = findQuickAmmo(weapon);
	int unloadCost = 0;
	int loadCost = 0;
	if(!ammo)
	{
		warning = "STR_NO_ROUNDS_LEFT";
		return false;
	}

	bool canUnload = unloadWeapon(weapon, warning, checkTu, true, &unloadCost);
	bool canLoad = loadWeapon(weapon, ammo, warning, checkTu, true, &loadCost, true);
	if (returnCost)
	{
		*returnCost = unloadCost + loadCost;
	}

	bool haveTus = canSpendTimeUnits(unloadCost + loadCost);

	if(checkTu && !haveTus)
	{
		warning = "STR_NOT_ENOUGH_TIME_UNITS";
		return false;
	}

	if(canUnload && canLoad && (!checkTu || haveTus))
	{
		if (reloadedAmmo)
		{
			*reloadedAmmo = ammo;
		}
		if(!testing)
		{
			unloadWeapon(weapon, warning, checkTu, false);
			loadWeapon(weapon, ammo, warning, checkTu, false);
		}
		return true;
	}

	if(returnCost)
	{
		*returnCost = unloadCost + loadCost;
	}

	return false;
}

int BattleUnit::getLoadCost(BattleItem *weapon, BattleItem *ammo) const
{
	int slotCost = ammo->getSlot()->getCost(weapon->getSlot());
	return isVehicle() ? (slotCost < 0 ? -1 : 0) : (Options::battleAdjustReloadCost ? (slotCost + ammo->getAmmoReloadCost()) : 15);
}

void BattleUnit::setMovementAction(BattleActionMove action)
{
	_movementAction = action;
}

BattleActionMove BattleUnit::getMovementAction() const
{
	return _movementAction;
}

std::vector<BattleEffect*> &BattleUnit::getActiveEffects()
{
	return _activeEffects;
}

bool BattleUnit::cancelEffects(EffectTrigger trigger)
{
	if (_activeEffects.size())
	{
		bool cancelled = false;
		for (std::vector<BattleEffect*>::iterator ii = _activeEffects.begin(); ii != _activeEffects.end();)
		{
			if ((*ii)->cancelByTrigger(trigger))
			{
				cancelled = true;
				ii = _activeEffects.erase(ii);
			}
			else
			{
				++ii;
			}
		}

		return cancelled;
	}

	return false;
}

const EffectComponent* BattleUnit::getEffectComponent(EffectComponentType type) const
{
	if (_activeEffects.size())
	{
		for (auto ii = _activeEffects.cbegin(); ii != _activeEffects.cend(); ++ii)
		{
			if(const EffectComponent *match = (*ii)->getComponent(type))
			{
				return match; 
			}
		}
	}

	return 0;
}

void BattleUnit::addBattleExperience(const std::string &action)
{
	addBattleExperience(Game::getMod()->getActionExperience(action));
}

void BattleUnit::addBattleExperience(int battleExperience)
{
	_battleExperience = std::min(_battleExperience + battleExperience, 100);
}

int BattleUnit::getBattleExperience() const
{
	return _battleExperience;
}

int BattleUnit::getTurnsAwake() const
{
	return _turnsAwake;
}

void BattleUnit::setAboutToFall(bool aboutToFall)
{
	_aboutToFall = aboutToFall;
}

bool BattleUnit::getAboutToFall() const
{
	return _aboutToFall;
}


namespace
{

void getArmorValueScript(const BattleUnit *bu, int &ret, int side)
{
	if (bu && 0 <= side && side < SIDE_MAX)
	{
		ret = bu->getArmor((UnitSide)side);
		return;
	}
	ret = 0;
}
void getArmorMaxScript(const BattleUnit *bu, int &ret, int side)
{
	if (bu && 0 <= side && side < SIDE_MAX)
	{
		ret = bu->getMaxArmor((UnitSide)side);
		return;
	}
	ret = 0;
}
void getGenderScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		ret = bu->getGender();
		return;
	}
	ret = 0;
}
void getLookScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		auto g = bu->getGeoscapeSoldier();
		if (g)
		{
			ret = g->getLook();
			return;
		}
	}
	ret = 0;
}
void getLookVariantScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		auto g = bu->getGeoscapeSoldier();
		if (g)
		{
			ret = g->getLookVariant();
			return;
		}
	}
	ret = 0;
}
void geReactionScoreScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		ret = (int)bu->getReactionScore(false);
	}
	ret = 0;
}
void getRecolorScript(const BattleUnit *bu, int &pixel)
{
	// TODO: Add stealth effect to this function.
	if (bu)
	{
		const int g = pixel & helper::ColorGroup;
		const int s = pixel & helper::ColorShade;

		const int ab = bu->getArmor()->getBaseColorGroup() << 4;
		const int ac = bu->getArmorColor();
		if (ab > 0 && ac >= 0 && g == ab)
		{
			switch (ac & helper::ColorShade)
			{
				case 1:
					switch (pixel & helper::ColorShade)
					{
						case 0:
						case 1:
							pixel = 0;
							break;
						case 2:
						case 3:
							pixel = 1;
							break;
						case 4:
						case 5:
							pixel = 2;
							break;
						case 6:
						case 7:
							pixel = 3;
							break;
						default:
							pixel = (pixel & helper::ColorShade) - 4;
					}

					pixel = (ac & helper::ColorShade) ? (pixel + (ac & helper::ColorGroup)) : pixel + 1;
					return;
				case 15:
					pixel = (ac & helper::ColorGroup) + ((pixel & helper::ColorShade) >> 1) + 8;
					return;
				default:
					pixel = ac + (pixel & helper::ColorShade);
					pixel = pixel > 0 ? pixel : 1;
					return;
			}
		}

		const auto& vec = bu->getRecolor();
		for (auto& p : vec)
		{
			if (g == p.first)
			{
				pixel = s + p.second;
				return;
			}
		}
	}
}
void getTileShade(const BattleUnit *bu, int &shade)
{
	if (bu)
	{
		auto tile = bu->getTile();
		if (tile)
		{
			shade = tile->getShade();
			return;
		}
	}
	shade = 0;
}

struct getRightHandWeaponScript
{
	static RetEnum func(BattleUnit *bu, BattleItem *&bi)
	{
		if (bu)
		{
			bi = bu->getWeapon1();
		}
		else
		{
			bi = nullptr;
		}
		return RetContinue;
	}
};
struct getRightHandWeaponConstScript
{
	static RetEnum func(const BattleUnit *bu, const BattleItem *&bi)
	{
		if (bu)
		{
			bi = bu->getWeapon2();
		}
		else
		{
			bi = nullptr;
		}
		return RetContinue;
	}
};
struct getLeftHandWeaponScript
{
	static RetEnum func(BattleUnit *bu, BattleItem *&bi)
	{
		if (bu)
		{
			bi = bu->getWeapon1();
		}
		else
		{
			bi = nullptr;
		}
		return RetContinue;
	}
};
struct getLeftHandWeaponConstScript
{
	static RetEnum func(const BattleUnit *bu, const BattleItem *&bi)
	{
		if (bu)
		{
			bi = bu->getWeapon2();
		}
		else
		{
			bi = nullptr;
		}
		return RetContinue;
	}
};

struct reduceByBraveryScript
{
	static RetEnum func(const BattleUnit *bu, int &ret)
	{
		if (bu)
		{
			ret = bu->reduceByBravery(ret);
		}
		return RetContinue;
	}
};

struct reduceByResistanceScript
{
	static RetEnum func(const BattleUnit *bu, int &ret, int resistType)
	{
		if (bu)
		{
			if (resistType >= 0 && resistType < DAMAGE_TYPES)
			{
				ret = bu->reduceByResistance(ret, (ItemDamageType)resistType);
			}
		}
		return RetContinue;
	}
};

void isWalkingScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		ret = bu->getStatus() == STATUS_WALKING;
		return;
	}
	ret = 0;
}
void isFlyingScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		ret = bu->getStatus() == STATUS_FLYING;
		return;
	}
	ret = 0;
}
void isCollapsingScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		ret = bu->getStatus() == STATUS_COLLAPSING;
		return;
	}
	ret = 0;
}
void isStandingScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		ret = bu->getStatus() == STATUS_STANDING;
		return;
	}
	ret = 0;
}
void isAimingScript(const BattleUnit *bu, int &ret)
{
	if (bu)
	{
		ret = bu->getStatus() == STATUS_AIMING;
		return;
	}
	ret = 0;
}

struct burnShadeScript
{
	static RetEnum func(int &curr, int burn, int shade)
	{
		Uint8 d = curr;
		Uint8 s = curr;
		helper::BurnShade::func(d, s, burn, shade);
		curr = d;
		return RetContinue;
	}
};

template<int BattleUnit::*StatCurr, int UnitStats::*StatMax>
void setBaseStatScript(BattleUnit *bu, int val)
{
	if (bu)
	{
		(bu->*StatCurr) = Clamp(val, 0, (bu->getBaseStats()->*StatMax));
	}
}

template<int BattleUnit::*StatCurr, int BattleUnit::*StatMax>
void setBaseStatScript(BattleUnit *bu, int val)
{
	if (bu)
	{
		(bu->*StatCurr) = Clamp(val, 0, (bu->*StatMax));
	}
}

template<int BattleUnit::*StatCurr, int UnitStats::*StatMax>
void setMaxStatScript(BattleUnit *bu, int val)
{
	if (bu)
	{
		val = Clamp(val, 1, 1000);
		(bu->getBaseStats()->*StatMax) = val;
		if ((bu->*StatCurr) > val)
		{
			(bu->*StatCurr) = val;
		}
	}
}

template<int UnitStats::*StatMax>
void setMaxStatScript(BattleUnit *bu, int val)
{
	if (bu)
	{
		val = Clamp(val, 1, 1000);
	}
}

template<int BattleUnit::*StatCurr, int Min, int Max>
void setBaseStatRangeScript(BattleUnit *bu, int val)
{
	if (bu)
	{
		(bu->*StatCurr) = Clamp(val, Min, Max);
	}
}

std::string debugDisplayScript(const BattleUnit* bu)
{
	if (bu)
	{
		std::string s;
		s += BattleUnit::ScriptName;
		s += "(type: \"";
		s += bu->getType();
		auto unit = bu->getUnitRules();
		if (unit)
		{
			s += "\" race: \"";
			s += unit->getRace();
		}
		s += "\" id: ";
		s += std::to_string(bu->getId());
		s += " faction: ";
		switch (bu->getFaction())
		{
		case FACTION_HOSTILE: s += "Hostile"; break;
		case FACTION_NEUTRAL: s += "Neutral"; break;
		case FACTION_PLAYER: s += "Player"; break;
		}
		s += " hp: ";
		s += std::to_string(bu->getHealth());
		s += "/";
		s += std::to_string(bu->getBaseStats()->health);
		s += ")";
		return s;
	}
	else
	{
		return "null";
	}
}

} // namespace

  /**
  * Register BattleUnit in script parser.
  * @param parser Script parser.
  */
void BattleUnit::ScriptRegister(ScriptParserBase* parser)
{
	parser->registerPointerType<Mod>();
	parser->registerPointerType<Armor>();
	parser->registerPointerType<BattleItem>();

	Bind<BattleUnit> bu = { parser };
	BindNested<BattleUnit, UnitStats, &BattleUnit::_stats> us = { bu };


	bu.addField<&BattleUnit::_id>("getId");
	bu.addField<&BattleUnit::_rankInt>("getRank");
	bu.add<&getGenderScript>("getGender");
	bu.add<&getLookScript>("getLook");
	bu.add<&getLookVariantScript>("getLookVariant");
	bu.add<&getRecolorScript>("getRecolor");
	bu.add<&BattleUnit::isFloating>("isFloating");
	bu.add<&BattleUnit::isKneeled>("isKneeled");
	bu.add<&isStandingScript>("isStanding");
	bu.add<&isWalkingScript>("isWalking");
	bu.add<&isFlyingScript>("isFlying");
	bu.add<&isCollapsingScript>("isCollapsing");
	bu.add<&isAimingScript>("isAiming");
	bu.add<&geReactionScoreScript>("geReactionScore");
	bu.add<&BattleUnit::getDirection>("getDirection");
	bu.add<&BattleUnit::getTurretDirection>("getTurretDirection");
	bu.add<&BattleUnit::getWalkingPhase>("getWalkingPhase");


	bu.addField<&BattleUnit::_tu>("getTimeUnits");
	us.addField<&UnitStats::tu>("getTimeUnitsMax");
	bu.add<&setBaseStatScript<&BattleUnit::_tu, &UnitStats::tu>>("setTimeUnits");

	bu.addField<&BattleUnit::_health>("getHealth");
	us.addField<&UnitStats::health>("getHealthMax");
	bu.add<&setBaseStatScript<&BattleUnit::_health, &UnitStats::health>>("setHealth");

	bu.addField<&BattleUnit::_energy>("getEnergy");
	us.addField<&UnitStats::stamina>("getEnergyMax");
	bu.add<&setBaseStatScript<&BattleUnit::_energy, &UnitStats::stamina>>("setEnergy");

	bu.addField<&BattleUnit::_stunlevel>("getStun");
	bu.addField<&BattleUnit::_health>("getStunMax");
	bu.add<&setBaseStatScript<&BattleUnit::_stunlevel, &BattleUnit::_health>>("setStun");

	bu.addField<&BattleUnit::_morale>("getMorale");
	bu.addFake<100>("getMoraleMax");
	bu.add<&setBaseStatRangeScript<&BattleUnit::_morale, 0, 100>>("setMorale");


	bu.add<&getArmorValueScript>("getArmor");
	bu.add<&getArmorMaxScript>("getArmorMax");


	us.addField<&UnitStats::tu>("Stats.getTimeUnits");
	bu.add<&setMaxStatScript<&BattleUnit::_tu, &UnitStats::tu>>("Stats.setTimeUnits");

	us.addField<&UnitStats::stamina>("Stats.getStamina");
	bu.add<&setMaxStatScript<&BattleUnit::_energy, &UnitStats::stamina>>("Stats.setStamina");

	us.addField<&UnitStats::health>("Stats.getHealth");
	bu.add<&setMaxStatScript<&BattleUnit::_health, &UnitStats::health>>("Stats.setHealth");

	us.addField<&UnitStats::bravery>("Stats.getBravery");
	bu.add<&setMaxStatScript<&UnitStats::reactions>>("Stats.setBravery");

	us.addField<&UnitStats::reactions>("Stats.getReactions");
	bu.add<&setMaxStatScript<&UnitStats::reactions>>("Stats.setReactions");

	us.addField<&UnitStats::firing>("Stats.getFiring");
	bu.add<&setMaxStatScript<&UnitStats::firing>>("Stats.setFiring");

	us.addField<&UnitStats::throwing>("Stats.getThrowing");
	bu.add<&setMaxStatScript<&UnitStats::throwing>>("Stats.setThrowing");

	us.addField<&UnitStats::strength>("Stats.getStrength");
	bu.add<&setMaxStatScript<&UnitStats::strength>>("Stats.setStrength");

	us.addField<&UnitStats::psiStrength>("Stats.getPsiStrength");
	bu.add<&setMaxStatScript<&UnitStats::psiStrength>>("Stats.setPsiStrength");

	us.addField<&UnitStats::psiSkill>("Stats.getPsiSkill");
	bu.add<&setMaxStatScript<&UnitStats::psiSkill>>("Stats.setPsiSkill");

	us.addField<&UnitStats::melee>("Stats.getMelee");
	bu.add<&setMaxStatScript<&UnitStats::melee>>("Stats.setMelee");


	bu.add<&BattleUnit::getFatalWounds>("getFatalwoundsTotal");
	bu.add<&BattleUnit::getFatalWound>("getFatalwounds");
	bu.add<&BattleUnit::getOverKillDamage>("getOverKillDamage");
	bu.addRules<Armor, &BattleUnit::getArmor>("getRuleArmor");
	bu.addFunc<getRightHandWeaponScript>("getRightHandWeapon");
	bu.addFunc<getRightHandWeaponConstScript>("getRightHandWeapon");
	bu.addFunc<getLeftHandWeaponScript>("getLeftHandWeapon");
	bu.addFunc<getLeftHandWeaponConstScript>("getLeftHandWeapon");
	bu.addFunc<reduceByBraveryScript>("reduceByBravery");
	bu.addFunc<reduceByResistanceScript>("reduceByResistance");


	bu.addScriptValue<&BattleUnit::_scriptValues>();
	bu.addDebugDisplay<&debugDisplayScript>();


	bu.add<&getTileShade>("getTileShade");


	bu.addCustomConst("BODYPART_HEAD", BODYPART_HEAD);
	bu.addCustomConst("BODYPART_TORSO", BODYPART_TORSO);
	bu.addCustomConst("BODYPART_LEFTARM", BODYPART_LEFTARM);
	bu.addCustomConst("BODYPART_RIGHTARM", BODYPART_RIGHTARM);
	bu.addCustomConst("BODYPART_LEFTLEG", BODYPART_LEFTLEG);
	bu.addCustomConst("BODYPART_RIGHTLEG", BODYPART_RIGHTLEG);

	bu.addCustomConst("UNIT_RANK_ROOKIE", 0);
	bu.addCustomConst("UNIT_RANK_SQUADDIE", 1);
	bu.addCustomConst("UNIT_RANK_SERGEANT", 2);
	bu.addCustomConst("UNIT_RANK_CAPTAIN", 3);
	bu.addCustomConst("UNIT_RANK_COLONEL", 4);
	bu.addCustomConst("UNIT_RANK_COMMANDER", 5);

	bu.addCustomConst("COLOR_X1_HAIR", 6);
	bu.addCustomConst("COLOR_X1_FACE", 9);

	bu.addCustomConst("COLOR_X1_NULL", 0);
	bu.addCustomConst("COLOR_X1_YELLOW", 1);
	bu.addCustomConst("COLOR_X1_RED", 2);
	bu.addCustomConst("COLOR_X1_GREEN0", 3);
	bu.addCustomConst("COLOR_X1_GREEN1", 4);
	bu.addCustomConst("COLOR_X1_GRAY", 5);
	bu.addCustomConst("COLOR_X1_BROWN0", 6);
	bu.addCustomConst("COLOR_X1_BLUE0", 7);
	bu.addCustomConst("COLOR_X1_BLUE1", 8);
	bu.addCustomConst("COLOR_X1_BROWN1", 9);
	bu.addCustomConst("COLOR_X1_BROWN2", 10);
	bu.addCustomConst("COLOR_X1_PURPLE0", 11);
	bu.addCustomConst("COLOR_X1_PURPLE1", 12);
	bu.addCustomConst("COLOR_X1_BLUE2", 13);
	bu.addCustomConst("COLOR_X1_SILVER", 14);
	bu.addCustomConst("COLOR_X1_SPECIAL", 15);


	bu.addCustomConst("LOOK_BLONDE", LOOK_BLONDE);
	bu.addCustomConst("LOOK_BROWNHAIR", LOOK_BROWNHAIR);
	bu.addCustomConst("LOOK_ORIENTAL", LOOK_ORIENTAL);
	bu.addCustomConst("LOOK_AFRICAN", LOOK_AFRICAN);

	bu.addCustomConst("GENDER_MALE", GENDER_MALE);
	bu.addCustomConst("GENDER_FEMALE", GENDER_FEMALE);
}

/**
* Register BattleUnitVisibility in script parser.
* @param parser Script parser.
*/
void BattleUnitVisibility::ScriptRegister(ScriptParserBase* parser)
{
	Bind<BattleUnitVisibility> uv = { parser };

	uv.addScriptTag();
}


namespace
{

void commonImpl(BindBase& b, Mod* mod)
{
	b.addCustomPtr<const Mod>("rules", mod);

	b.addCustomConst("blit_torso", BODYPART_TORSO);
	b.addCustomConst("blit_leftarm", BODYPART_LEFTARM);
	b.addCustomConst("blit_rightarm", BODYPART_RIGHTARM);
	b.addCustomConst("blit_legs", BODYPART_LEGS);
	b.addCustomConst("blit_collapse", BODYPART_COLLAPSING);

	b.addCustomConst("blit_large_torso_0", BODYPART_LARGE_TORSO + 0);
	b.addCustomConst("blit_large_torso_1", BODYPART_LARGE_TORSO + 1);
	b.addCustomConst("blit_large_torso_2", BODYPART_LARGE_TORSO + 2);
	b.addCustomConst("blit_large_torso_3", BODYPART_LARGE_TORSO + 3);
	b.addCustomConst("blit_large_propulsion_0", BODYPART_LARGE_PROPULSION + 0);
	b.addCustomConst("blit_large_propulsion_1", BODYPART_LARGE_PROPULSION + 1);
	b.addCustomConst("blit_large_propulsion_2", BODYPART_LARGE_PROPULSION + 2);
	b.addCustomConst("blit_large_propulsion_3", BODYPART_LARGE_PROPULSION + 3);
	b.addCustomConst("blit_large_turret", BODYPART_LARGE_TURRET);
}

}

/**
* Constructor of recolor script parser.
*/
ModScript::RecolorUnitParser::RecolorUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name, "new_pixel", "old_pixel", "unit", "blit_part", "anim_frame", "shade", "burn" }
{
	BindBase b{ this };

	b.addCustomFunc<burnShadeScript>("add_burn_shade");

	commonImpl(b, mod);

	b.addCustomConst("blit_item_righthand", BODYPART_ITEM_RIGHTHAND);
	b.addCustomConst("blit_item_lefthand", BODYPART_ITEM_LEFTHAND);
	b.addCustomConst("blit_item_floor", BODYPART_ITEM_FLOOR);
	b.addCustomConst("blit_item_big", BODYPART_ITEM_INVENTORY);

	setDefault("unit.getRecolor new_pixel; add_burn_shade new_pixel burn shade; return new_pixel;");
}

/**
* Constructor of select sprite script parser.
*/
ModScript::SelectUnitParser::SelectUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name, "sprite_index", "sprite_offset", "unit", "blit_part", "anim_frame", "shade" }
{
	BindBase b{ this };

	commonImpl(b, mod);

	setDefault("add sprite_index sprite_offset; return sprite_index;");
}

/**
* Constructor of reaction chance script parser.
*/
ModScript::ReactionUnitParser::ReactionUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name, "reaction_chance", "distance", "action_unit", "reaction_unit", "weapon", "action", "action_target", "move" }
{
	BindBase b{ this };

	b.addCustomPtr<const Mod>("rules", mod);

	b.addCustomConst("action_aimshoot", BA_AIMEDSHOT);
	b.addCustomConst("action_autoshoot", BA_AUTOSHOT);
	b.addCustomConst("action_snapshot", BA_SNAPSHOT);
	b.addCustomConst("action_walk", BA_WALK);
	b.addCustomConst("action_hit", BA_HIT);
	b.addCustomConst("action_throw", BA_THROW);

	b.addCustomConst("move_normal", BAM_NORMAL);
	b.addCustomConst("move_run", BAM_RUN);
	b.addCustomConst("move_strafe", BAM_STRAFE);
}

/**
* Constructor of visibility script parser.
*/
ModScript::VisibilityUnitParser::VisibilityUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name, "current_visibility", "default_visibility", "visibility_mode", "observer_unit", "target_unit", "distance", "distance_max", "smoke_density", "fire_density", }
{
	BindBase b{ this };

	b.addCustomPtr<const Mod>("rules", mod);
}

/**
* Init all required data in script using object data.
*/
void BattleUnit::ScriptFill(ScriptWorkerBlit* w, BattleUnit* unit, int body_part, int anim_frame, int shade, int burn)
{
	w->clear();
	if (unit)
	{
		w->update(unit->getArmor()->getScript<ModScript::RecolorUnitSprite>(), unit, body_part, anim_frame, shade, burn);
	}
}

ModScript::DamageUnitParser::DamageUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
"to_health",
"to_armor",
"to_stun",
"to_time",
"to_energy",
"to_morale",
"to_wound",
"unit", "damaging_item", "weapon_item", "attacker",
"battle_game", "currPower", "orig_power", "part", "side", "damaging_type", "battle_action", }
{
	BindBase b{ this };

	b.addCustomPtr<const Mod>("rules", mod);

	setEmptyReturn();
}

ModScript::HitUnitParser::HitUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name,
"power",
"part",
"side",
"unit", "damaging_item", "weapon_item", "attacker",
"battle_game", "orig_power", "damaging_type", "battle_action", }
{
	BindBase b{ this };

	b.addCustomPtr<const Mod>("rules", mod);
}

ModScript::CreateUnitParser::CreateUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name, "unit", "battle_game", "turn", }
{
	BindBase b{ this };

	b.addCustomPtr<const Mod>("rules", mod);
}

ModScript::NewTurnUnitParser::NewTurnUnitParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name, "unit", "battle_game", "turn", "side", }
{
	BindBase b{ this };

	b.addCustomPtr<const Mod>("rules", mod);
}

ModScript::AwardExperienceParser::AwardExperienceParser(ScriptGlobal* shared, const std::string& name, Mod* mod) : ScriptParserEvents{ shared, name, "experience_multipler", "experience_type", "attacker", "unit", "weapon", }
{
	BindBase b{ this };

	b.addCustomPtr<const Mod>("rules", mod);
}
}
