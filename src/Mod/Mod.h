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
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <SDL.h>
#include <yaml-cpp/yaml.h>
#include "../Engine/Options.h"
#include "../Savegame/GameTime.h"
#include "RuleDamageType.h"
#include "Unit.h"
#include "RuleAlienMission.h"

// https://stackoverflow.com/questions/7110301/generic-hash-for-tuples-in-unordered-map-unordered-set
namespace hash_tuple
{
template <typename TT>
struct hash
{
	size_t
		operator()(TT const& tt) const
	{
		return std::hash<TT>()(tt);
	}
};

template <class T>
inline void hash_combine(std::size_t& seed, T const& v)
{
	seed ^= hash_tuple::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace
{
// Recursive template code derived from Matthieu M.
template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
struct HashValueImpl
{
	static void apply(size_t& seed, Tuple const& tuple)
	{
		HashValueImpl<Tuple, Index - 1>::apply(seed, tuple);
		hash_combine(seed, std::get<Index>(tuple));
	}
};

template <class Tuple>
struct HashValueImpl<Tuple, 0>
{
	static void apply(size_t& seed, Tuple const& tuple)
	{
		hash_combine(seed, std::get<0>(tuple));
	}
};
}

template <typename ... TT>
struct hash<std::tuple<TT...>>
{
	size_t
		operator()(std::tuple<TT...> const& tt) const
	{
		size_t seed = 0;
		HashValueImpl<std::tuple<TT...> >::apply(seed, tt);
		return seed;
	}
};

}

namespace OpenXcom
{

class Surface;
class SurfaceSet;
class Font;
class Palette;
class Music;
class SoundSet;
class Sound;
class CatFile;
class GMCatFile;
class Music;
class Palette;
class SavedGame;
class Soldier;
class RuleCountry;
class RuleRegion;
class RuleBaseFacility;
class RuleCraft;
class RuleCraftWeapon;
class RuleItemCategory;
class RuleItem;
class RuleDamageType;
class RuleUfo;
class RuleTerrain;
class MapDataSet;
class RuleRole;
class RuleSoldier;
class Unit;
class Armor;
class ArticleDefinition;
class RuleInventory;
class RuleInventoryLayout;
class RuleResearch;
class RuleManufacture;
class AlienRace;
class RuleStartingCondition;
class AlienDeployment;
class UfoTrajectory;
class RuleAlienMission;
class Base;
class MCDPatch;
class ExtraSprites;
class ExtraSounds;
class CustomPalettes;
class ExtraStrings;
class RuleCommendations;
class StatString;
class RuleInterface;
class RuleGlobe;
class RuleConverter;
class SoundDefinition;
class MapScript;
class RuleVideo;
class RuleMusic;
class RuleMissionScript;
class ModScript;
class ModScriptGlobal;
class ScriptParserBase;
class ScriptGlobal;
struct StatAdjustment;
class Language;
class RuleEffect;

/**
 * Contains all the game-specific static data that never changes
 * throughout the game, like rulesets and resources.
 */
class Mod
{
private:
	Music *_muteMusic;
	Sound *_muteSound;
	std::string _playingMusic;

	std::map<std::string, Palette*> _palettes;
	std::map<std::string, Font*> _fonts;
	std::map<std::string, Surface*> _surfaces;
	std::map<std::string, SurfaceSet*> _sets;
	std::unordered_map<std::tuple<std::string, Uint8, Uint8>, SurfaceSet*, hash_tuple::hash<std::tuple<std::string, Uint8, Uint8> > > _recoloredSets;
	std::map<std::string, SoundSet*> _sounds;
	std::map<std::string, Music*> _musics;
	std::vector<Uint16> _voxelData;
	std::vector<std::vector<Uint8> > _transparencyLUTs;

	std::map<std::string, RuleCountry*> _countries, _extraGlobeLabels;
	std::map<std::string, RuleRegion*> _regions;
	std::map<std::string, RuleBaseFacility*> _facilities;
	std::map<std::string, RuleCraft*> _crafts;
	std::map<std::string, RuleCraftWeapon*> _craftWeapons;
	std::map<std::string, RuleItemCategory*> _itemCategories;
	std::map<std::string, RuleItem*> _items;
	std::map<std::string, RuleRole*> _roles;
	std::map<std::string, RuleUfo*> _ufos;
	std::map<std::string, RuleTerrain*> _terrains;
	std::map<std::string, MapDataSet*> _mapDataSets;
	std::map<std::string, RuleSoldier*> _soldiers;
	std::map<std::string, Unit*> _units;
	std::map<std::string, AlienRace*> _alienRaces;
	std::map<std::string, RuleStartingCondition*> _startingConditions;
	std::map<std::string, AlienDeployment*> _alienDeployments;
	std::map<std::string, Armor*> _armors;
	std::map<std::string, ArticleDefinition*> _ufopaediaArticles;
	std::map<std::string, RuleInventory*> _invs;
	std::map<std::string, RuleInventoryLayout*> _inventoryLayouts;
	std::map<std::string, RuleResearch *> _research;
	std::map<std::string, RuleManufacture *> _manufacture;
	std::map<std::string, UfoTrajectory *> _ufoTrajectories;
	std::map<std::string, RuleAlienMission *> _alienMissions;
	std::map<std::string, RuleInterface *> _interfaces;
	std::map<std::string, SoundDefinition *> _soundDefs;
	std::map<std::string, RuleVideo *>_videos;
	std::map<std::string, MCDPatch *> _MCDPatches;
	std::map<std::string, std::vector<MapScript *> > _mapScripts;
	std::map<std::string, RuleCommendations *> _commendations;
	std::map<std::string, RuleMissionScript*> _missionScripts;
	std::vector<std::pair<std::string, ExtraSprites *> > _extraSprites;
	std::map<std::string, CustomPalettes *> _customPalettes;
	std::vector<std::pair<std::string, ExtraSounds *> > _extraSounds;
	std::map<std::string, ExtraStrings *> _extraStrings;
	std::vector<StatString*> _statStrings;
	std::vector<RuleDamageType*> _damageTypes;
	std::map<std::string, RuleMusic *> _musicDefs;
	std::map<std::string, RuleEffect*> _effects;
	RuleGlobe *_globe;
	RuleConverter *_converter;
	ModScriptGlobal *_scriptGlobal;
	int _maxViewDistance, _maxDarknessToSeeUnits;
	int _maxStaticLightDistance, _maxDynamicLightDistance, _enhancedLighting;
	int _costHireEngineer, _costHireScientist;
	int _costEngineer, _costScientist, _timePersonnel, _initialFunding;
	int _aiUseDelayBlaster, _aiUseDelayFirearm, _aiUseDelayGrenade, _aiUseDelayMelee, _aiUseDelayPsionic;
	int _maxLookVariant, _tooMuchSmokeThreshold, _customTrainingFactor, _minReactionAccuracy;
	int _chanceToStopRetaliation;
	int _kneelBonusGlobal, _oneHandedPenaltyGlobal;
	int _enableCloseQuartersCombat, _closeQuartersAccuracyGlobal, _closeQuartersTuCostGlobal, _closeQuartersEnergyCostGlobal;
	int _surrenderMode;
	int _bughuntMinTurn, _bughuntMaxEnemies, _bughuntRank, _bughuntLowMorale, _bughuntTimeUnitsLeft;
	int _ufoGlancingHitThreshold, _ufoBeamWidthParameter;
	int _ufoTractorBeamSizeModifiers[5];
	int _soldiersPerSergeant, _soldiersPerCaptain, _soldiersPerColonel, _soldiersPerCommander;
	int _pilotAccuracyZeroPoint, _pilotAccuracyRange, _pilotReactionsZeroPoint, _pilotReactionsRange;
	int _pilotBraveryThresholds[3];
	int _performanceBonusFactor;
	bool _useCustomCategories, _showDogfightDistanceInKm;
	int _theMostUselessOptionEver, _theBiggestRipOffEver;
	int _defeatScore, _defeatFunds;
	int _maxTUDebt;
	std::pair<std::string, int> _alienFuel;
	int _baseMissionExperience;
	std::string _fontName, _finalResearch, _psiUnlockResearch;
	YAML::Node _startingBase;
	GameTime _startingTime;
	int _baseDefenseMapFromLocation;
	std::map<int, std::string> _missionRatings, _monthlyRatings;
	std::map<std::string, std::string> _fixedUserOptions;
	std::vector<std::string> _hiddenMovementBackgrounds;
	StatAdjustment _statAdjustment[5];

	std::vector<std::string> _countriesIndex, _extraGlobeLabelsIndex, _regionsIndex, _facilitiesIndex, _craftsIndex, _craftWeaponsIndex, _itemCategoriesIndex, _itemsIndex, _invsIndex, _ufosIndex;
	std::vector<std::string> _soldiersIndex, _aliensIndex, _startingConditionsIndex, _deploymentsIndex, _armorsIndex, _ufopaediaIndex, _ufopaediaCatIndex, _researchIndex, _manufactureIndex, _MCDPatchesIndex;
	std::vector<std::string> _alienMissionsIndex, _terrainIndex, _extraSpritesIndex, _customPalettesIndex, _extraSoundsIndex, _extraStringsIndex, _missionScriptIndex;
	std::vector<std::string> _rolesIndex, _inventoryLayoutsIndex;
	std::vector<std::vector<int> > _alienItemLevels;
	std::vector<SDL_Color> _transparencies;
	int _facilityListOrder, _craftListOrder, _itemCategoryListOrder, _itemListOrder, _researchListOrder, _manufactureListOrder, _ufopaediaListOrder, _invListOrder, _roleListOrder;
	size_t _modOffset;
	std::vector<std::string> _psiRequirements; // it's a cache for psiStrengthEval
	size_t _surfaceOffsetBigobs = 0;
	size_t _surfaceOffsetFloorob = 0;
	size_t _surfaceOffsetHandob = 0;
	size_t _surfaceOffsetSmoke = 0;
	size_t _surfaceOffsetHit = 0;
	size_t _surfaceOffsetBasebits = 0;
	size_t _soundOffsetBattle = 0;
	size_t _soundOffsetGeo = 0;
	std::map<std::string, Uint8> _soldierArmorBaseColors;
	std::vector<std::string> _soldierArmorBaseColorIndex;
	std::map<std::string, int> _actionExperience;
	//std::map<int, float> _damageDropoff;

	/// Loads a ruleset from a YAML file.
	void loadFile(const std::string &filename, ModScript &parsers);
	/// Loads a ruleset element.
	template <typename T>
	T *loadRule(const YAML::Node &node, std::map<std::string, T*> *map, std::vector<std::string> *index = 0, const std::string &key = "type") const;
	/// Gets a ruleset element.
	template <typename T>
	T *getRule(const std::string &id, const std::string &name, const std::map<std::string, T*> &map, bool error) const;
	/// Gets a random music. This is private to prevent access, use playMusic(name, true) instead.
	Music *getRandomMusic(const std::string &name) const;
	/// Gets a particular sound set. This is private to prevent access, use getSound(name, id) instead.
	SoundSet *getSoundSet(const std::string &name, bool error = true) const;
	/// Loads battlescape specific resources.
	void loadBattlescapeResources();
	/// Checks if an extension is a valid image file.
	bool isImageFile(std::string extension) const;
	/// Loads a specified music file.
	Music *loadMusic(MusicFormat fmt, const std::string &file, int track, float volume, CatFile *adlibcat, CatFile *aintrocat, GMCatFile *gmcat) const;
	/// Creates a transparency lookup table for a given palette.
	void createTransparencyLUT(Palette *pal);
	/// Loads a specified mod content.
	void loadMod(const std::vector<std::string> &rulesetFiles, size_t modIdx, ModScript &parsers);
	/// Loads resources from vanilla.
	void loadVanillaResources();
	/// Loads resources from extra rulesets.
	void loadExtraResources();
	/// Applies mods to vanilla resources.
	void modResources();
	/// Sorts all our lists according to their weight.
	void sortLists();
public:
	static int DOOR_OPEN;
	static int SLIDING_DOOR_OPEN;
	static int SLIDING_DOOR_CLOSE;
	static int SMALL_EXPLOSION;
	static int LARGE_EXPLOSION;
	static int EXPLOSION_OFFSET;
	static int SMOKE_OFFSET;
	static int UNDERWATER_SMOKE_OFFSET;
	static int ITEM_DROP;
	static int ITEM_THROW;
	static int ITEM_RELOAD;
	static int WALK_OFFSET;
	static int FLYING_SOUND;
	static int BUTTON_PRESS;
	static int WINDOW_POPUP[3];
	static int UFO_FIRE;
	static int UFO_HIT;
	static int UFO_CRASH;
	static int UFO_EXPLODE;
	static int INTERCEPTOR_HIT;
	static int INTERCEPTOR_EXPLODE;
	static int GEOSCAPE_CURSOR;
	static int BASESCAPE_CURSOR;
	static int BATTLESCAPE_CURSOR;
	static int UFOPAEDIA_CURSOR;
	static int GRAPHS_CURSOR;
	static int DAMAGE_RANGE;
	static int EXPLOSIVE_DAMAGE_RANGE;
	static int FIRE_DAMAGE_RANGE[2];
	static std::string DEBRIEF_MUSIC_GOOD;
	static std::string DEBRIEF_MUSIC_BAD;
	static int DIFFICULTY_COEFFICIENT[5];
	static int DIFFICULTY_BASED_RETAL_DELAY[5];
	static float ENCUMBRANCE_MULTIPLIER;
	static int SPRINTING_SHOT_ACCURACY;
	// reset all the statics in all classes to default values
	static void resetGlobalStatics();

	/// Name of class used in script.
	static constexpr const char *ScriptName = "RuleMod";
	/// Register all useful function used by script.
	static void ScriptRegister(ScriptParserBase* parser);

	/// Creates a blank mod.
	Mod();
	/// Cleans up the mod.
	~Mod();

	/// Gets a particular font.
	Font *getFont(const std::string &name, bool error = true) const;
	/// Gets a particular surface.
	Surface *getSurface(const std::string &name, bool error = true) const;
	/// Gets a particular surface with a recolor.
	Surface *getSurface(const std::string &name, Uint8 recolorGroup, Uint8 recolorColor, bool error = true) const;
	/// Gets a particular surface set.
	SurfaceSet *getSurfaceSet(const std::string &name, bool error = true) const;
	/// Gets a particular surface set with a recolor.
	SurfaceSet *getSurfaceSet(const std::string &name, Uint8 recolorGroup, Uint8 recolorColor, bool error = true) const;
	/// Gets a particular music.
	Music *getMusic(const std::string &name, bool error = true) const;
	/// Gets the available music tracks.
	const std::map<std::string, Music*> &getMusicTrackList() const;
	/// Plays a particular music.
	void playMusic(const std::string &name, int id = 0);
	/// Gets a particular sound.
	Sound *getSound(const std::string &set, unsigned int sound, bool error = true) const;
	/// Gets a particular palette.
	Palette *getPalette(const std::string &name, bool error = true) const;
	/// Sets a new palette.
	void setPalette(SDL_Color *colors, int firstcolor = 0, int ncolors = 256);
	/// Gets list of voxel data.
	std::vector<Uint16> *getVoxelData();
	/// Returns a specific sound from either the land or underwater sound set.
	Sound *getSoundByDepth(unsigned int depth, unsigned int sound, bool error = true) const;
	/// Gets list of LUT data.
	const std::vector<std::vector<Uint8> > *getLUTs() const;
	/// Gets the mod offset.
	int getModOffset() const;
	/// Gets the mod offset for a certain sprite.
	int getSpriteOffset(int sprite, const std::string &set) const;
	/// Gets the mod offset for a certain sound.
	int getSoundOffset(int sound, const std::string &set) const;

	/// Loads a list of mods.
	void loadAll(const std::vector< std::pair< std::string, std::vector<std::string> > > &mods);
	/// Generates the starting saved game.
	SavedGame *newSave() const;
	/// Gets the ruleset for a country type.
	RuleCountry *getCountry(const std::string &id, bool error = false) const;
	/// Gets the available countries.
	const std::vector<std::string> &getCountriesList() const;
	/// Gets the ruleset for an extra globe label type.
	RuleCountry *getExtraGlobeLabel(const std::string &id, bool error = false) const;
	/// Gets the available extra globe labels.
	const std::vector<std::string> &getExtraGlobeLabelsList() const;
	/// Gets the ruleset for a region type.
	RuleRegion *getRegion(const std::string &id, bool error = false) const;
	/// Gets the available regions.
	const std::vector<std::string> &getRegionsList() const;
	/// Gets the ruleset for a facility type.
	RuleBaseFacility *getBaseFacility(const std::string &id, bool error = false) const;
	/// Gets the available facilities.
	const std::vector<std::string> &getBaseFacilitiesList() const;
	/// Gets the ruleset for a craft type.
	RuleCraft *getCraft(const std::string &id, bool error = false) const;
	/// Gets the available crafts.
	const std::vector<std::string> &getCraftsList() const;
	/// Gets the ruleset for a craft weapon type.
	RuleCraftWeapon *getCraftWeapon(const std::string &id, bool error = false) const;
	/// Gets the available craft weapons.
	const std::vector<std::string> &getCraftWeaponsList() const;
	/// Gets the ruleset for an item category type.
	RuleItemCategory *getItemCategory(const std::string &id, bool error = false) const;
	/// Gets the available item categories.
	const std::vector<std::string> &getItemCategoriesList() const;
	/// Gets the ruleset for an item type.
	RuleItem *getItem(const std::string &id, bool error = false) const;
	/// Gets the available items.
	const std::vector<std::string> &getItemsList() const;
	/// Gets the ruleset for a UFO type.
	RuleUfo *getUfo(const std::string &id, bool error = false) const;
	/// Gets the available UFOs.
	const std::vector<std::string> &getUfosList() const;
	/// Gets terrains for battlescape games.
	RuleTerrain *getTerrain(const std::string &name, bool error = false) const;
	/// Gets the available terrains.
	const std::vector<std::string> &getTerrainList() const;
	/// Gets mapdatafile for battlescape games.
	MapDataSet *getMapDataSet(const std::string &name);
	/// Gets soldier unit rules.
	RuleSoldier *getSoldier(const std::string &name, bool error = false) const;
	/// Gets the available soldiers.
	const std::vector<std::string> &getSoldiersList() const;
	/// Gets commendation rules.
	std::map<std::string, RuleCommendations *> getCommendation() const;
	/// Gets role rules.
	RuleRole *getRole(const std::string &name) const;
	// Gets role list.
	const std::vector<std::string> &getRolesList() const;
	/// Gets generated unit rules.
	Unit *getUnit(const std::string &name, bool error = false) const;
	/// Gets alien race rules.
	AlienRace *getAlienRace(const std::string &name, bool error = false) const;
	/// Gets the available alien races.
	const std::vector<std::string> &getAlienRacesList() const;
	/// Gets a starting condition.
	RuleStartingCondition *getStartingCondition(const std::string &name) const;
	/// Gets the available starting conditions.
	const std::vector<std::string> &getStartingConditionsList() const;
	/// Gets deployment rules.
	AlienDeployment *getDeployment(const std::string &name, bool error = false) const;
	/// Gets the available alien deployments.
	const std::vector<std::string> &getDeploymentsList() const;
	/// Gets armor rules.
	Armor *getArmor(const std::string &name, bool error = false) const;
	/// Gets the available armors.
	const std::vector<std::string> &getArmorsList() const;
	/// Gets Ufopaedia article definition.
	ArticleDefinition *getUfopaediaArticle(const std::string &name, bool error = false) const;
	/// Gets the available articles.
	const std::vector<std::string> &getUfopaediaList() const;
	/// Gets the available article categories.
	const std::vector<std::string> &getUfopaediaCategoryList() const;
	/// Gets the inventory list.
	std::map<std::string, RuleInventory*> *getInventories();
	/// Gets the ruleset for a specific inventory.
	RuleInventory *getInventory(const std::string &id, bool error = false) const;
	/// Gets max view distance in BattleScape.
	int getMaxViewDistance() const { return _maxViewDistance; }
	/// Gets threshold of darkness for LoS calculation.
	int getMaxDarknessToSeeUnits() const { return _maxDarknessToSeeUnits; }
	/// Gets max static (tiles & fire) ligth distance in BattleScape.
	int getMaxStaticLightDistance() const { return _maxStaticLightDistance; }
	/// Gets max dynamic (items & units) ligth distance in BattleScape.
	int getMaxDynamicLightDistance() const { return _maxDynamicLightDistance; }
	/// Get flags for enhanced lighting, 0x1 - tiles and fire, 0x2 - items, 0x4 - units.
	int getEnhancedLighting() const { return _enhancedLighting; }
	/// Get basic damage type
	const RuleDamageType *getDamageType(ItemDamageType type) const;
	/// Gets the cost of a soldier.
	int getSoldierCost() const;
	/// Gets the cost of a vehicle.
	int getVehicleCost() const;
	/// Gets the cost of hiring an engineer.
	int getHireEngineerCost() const;
	/// Gets the cost of hiring a scientist.
	int getHireScientistCost() const;
	/// Gets the monthly cost of an engineer.
	int getEngineerCost() const;
	/// Gets the monthly cost of a scientist.
	int getScientistCost() const;
	/// Gets the transfer time of personnel.
	int getPersonnelTime() const;
	/// Gets first turn when AI can use Blaster launcher.
	int getAIUseDelayBlaster() const { return _aiUseDelayBlaster; }
	/// Gets first turn when AI can use firearms.
	int getAIUseDelayFirearm() const { return _aiUseDelayFirearm; }
	/// Gets first turn when AI can use grenades.
	int getAIUseDelayGrenade() const { return _aiUseDelayGrenade; }
	/// Gets first turn when AI can use martial arts.
	int getAIUseDelayMelee() const { return _aiUseDelayMelee; }
	/// Gets first turn when AI can use psionic abilities.
	int getAIUseDelayPsionic() const { return _aiUseDelayPsionic; }
	/// Gets maximum supported lookVariant (0-15)
	int getMaxLookVariant() const { return abs(_maxLookVariant) % 16; }
	/// Gets the threshold for too much smoke (vanilla default = 10).
	int getTooMuchSmokeThreshold() const { return _tooMuchSmokeThreshold; }
	/// Gets the custom physical training factor in percent (default = 100).
	int getCustomTrainingFactor() const { return _customTrainingFactor; }
	/// Gets the minimum firing accuracy for reaction fire (default = 0).
	int getMinReactionAccuracy() const { return _minReactionAccuracy; }
	/// Gets the chance to stop retaliation after unsuccessful xcom base attack (default = 0).
	int getChanceToStopRetaliation() const { return _chanceToStopRetaliation; }
	/// Gets the global kneel bonus (default = 115).
	int getKneelBonusGlobal() const { return _kneelBonusGlobal; }
	/// Gets the global one-handed penalty (default = 80).
	int getOneHandedPenaltyGlobal() const { return _oneHandedPenaltyGlobal; }
	/// Gets whether close quarters combat is enabled (default = 0 is off).
	int getEnableCloseQuartersCombat() const { return _enableCloseQuartersCombat; }
	/// Gets the default close quarters combat accuracy (default = 100).
	int getCloseQuartersAccuracyGlobal() const { return _closeQuartersAccuracyGlobal; }
	/// Gets the default close quarters combat TU cost (default = 12).
	int getCloseQuartersTuCostGlobal() const { return _closeQuartersTuCostGlobal; }
	/// Gets the default close quarters combat energy cost (default = 8).
	int getCloseQuartersEnergyCostGlobal() const { return _closeQuartersEnergyCostGlobal; }
	/// Gets the surrender mode (default = 0).
	int getSurrenderMode() const { return _surrenderMode; }
	/// Gets the bug hunt mode minimum turn requirement (default = 20).
	int getBughuntMinTurn() const { return _bughuntMinTurn; }
	/// Gets the bug hunt mode maximum remaining enemies requirement (default = 2).
	int getBughuntMaxEnemies() const { return _bughuntMaxEnemies; }
	/// Gets the bug hunt mode "VIP rank" parameter (default = 0).
	int getBughuntRank() const { return _bughuntRank; }
	/// Gets the bug hunt mode low morale threshold parameter (default = 40).
	int getBughuntLowMorale() const { return _bughuntLowMorale; }
	/// Gets the bug hunt mode time units % parameter (default = 60).
	int getBughuntTimeUnitsLeft() const { return _bughuntTimeUnitsLeft; }
	/// Gets the threshold for defining a glancing hit on a ufo during interception
	int getUfoGlancingHitThreshold() const { return _ufoGlancingHitThreshold; }
	/// Gets the parameter for drawing the width of a ufo's beam weapon based on power
	int getUfoBeamWidthParameter() const { return _ufoBeamWidthParameter; }
	/// Gets the modifier to a tractor beam's power based on a ufo's size
	int getUfoTractorBeamSizeModifier(int ufoSize) const { return _ufoTractorBeamSizeModifiers[ufoSize]; }
	/// Gets how many soldiers are needed for one sergeant promotion
	int getSoldiersPerSergeant() const { return _soldiersPerSergeant; }
	/// Gets how many soldiers are needed for one captain promotion
	int getSoldiersPerCaptain() const { return _soldiersPerCaptain; }
	/// Gets how many soldiers are needed for one colonel promotion
	int getSoldiersPerColonel() const { return _soldiersPerColonel; }
	/// Gets how many soldiers are needed for one commander promotion
	int getSoldiersPerCommander() const { return _soldiersPerCommander; }
	/// Gets the firing accuracy needed for no bonus/penalty
	int getPilotAccuracyZeroPoint() const { return _pilotAccuracyZeroPoint; }
	/// Gets the firing accuracy impact (as percentage of distance to zero point) on pilot's aim in dogfight
	int getPilotAccuracyRange() const { return _pilotAccuracyRange; }
	/// Gets the reactions needed for no bonus/penalty
	int getPilotReactionsZeroPoint() const { return _pilotReactionsZeroPoint; }
	/// Gets the reactions impact (as percentage of distance to zero point) on pilot's dodge ability in dogfight
	int getPilotReactionsRange() const { return _pilotReactionsRange; }
	/// Gets the pilot's bravery needed for very bold approach speed
	int getPilotBraveryThresholdVeryBold() const { return _pilotBraveryThresholds[0]; }
	/// Gets the pilot's bravery needed for bold approach speed
	int getPilotBraveryThresholdBold() const { return _pilotBraveryThresholds[1]; }
	/// Gets the pilot's bravery needed for normal approach speed
	int getPilotBraveryThresholdNormal() const { return _pilotBraveryThresholds[2]; }
	/// Gets a performance bonus factor
	int getPerformanceBonusFactor() const { return _performanceBonusFactor; }
	/// Should custom categories be used in Buy/Sell/Transfer GUIs?
	bool getUseCustomCategories() const { return _useCustomCategories; }
	/// Should distance in dogfight GUI be shown in kilometers?
	bool getShowDogfightDistanceInKm() const { return _showDogfightDistanceInKm; }
	/// Self-explanatory
	int getTheMostUselessOptionEver() const { return _theMostUselessOptionEver; }
	/// Shame on you!
	int getTheBiggestRipOffEver() const { return _theBiggestRipOffEver; }
	/// Gets whether or not to load base defense terrain from globe texture
	int getBaseDefenseMapFromLocation() const { return _baseDefenseMapFromLocation; }
	/// Gets the ruleset for a specific research project.
	RuleResearch *getResearch(const std::string &id, bool error = false) const;
	/// Gets the list of all research projects.
	const std::vector<std::string> &getResearchList() const;
	/// Gets the ruleset for a specific manufacture project.
	RuleManufacture *getManufacture(const std::string &id, bool error = false) const;
	/// Gets the list of all manufacture projects.
	const std::vector<std::string> &getManufactureList() const;
	/// Gets facilities for custom bases.
	std::vector<RuleBaseFacility*> getCustomBaseFacilities() const;
	/// Gets a specific UfoTrajectory.
	const UfoTrajectory *getUfoTrajectory(const std::string &id, bool error = false) const;
	/// Gets the ruleset for a specific alien mission.
	const RuleAlienMission *getAlienMission(const std::string &id, bool error = false) const;
	/// Gets the ruleset for a random alien mission.
	const RuleAlienMission *getRandomMission(MissionObjective objective, size_t monthsPassed) const;
	/// Gets the list of all alien missions.
	const std::vector<std::string> &getAlienMissionList() const;
	/// Gets the alien item level table.
	const std::vector<std::vector<int> > &getAlienItemLevels() const;
	/// Gets the player starting base.
	const YAML::Node &getStartingBase() const;
	/// Gets the game starting time.
	const GameTime &getStartingTime() const;
	/// Gets an MCDPatch.
	MCDPatch *getMCDPatch(const std::string &name) const;
	/// Gets the list of external Sprites.
	std::vector<std::pair<std::string, ExtraSprites *> > getExtraSprites() const;
	/// Gets the list of custom palettes.
	const std::vector<std::string> &getCustomPalettes() const;
	/// Gets the list of external Sounds.
	std::vector<std::pair<std::string, ExtraSounds *> > getExtraSounds() const;
	/// Gets the list of external Strings.
	std::map<std::string, ExtraStrings *> getExtraStrings() const;
	/// Gets the list of StatStrings.
	std::vector<StatString *> getStatStrings() const;
	/// Gets the research-requirements for Psi-Lab (it's a cache for psiStrengthEval)
	std::vector<std::string> getPsiRequirements() const;
	/// Returns the sorted list of inventories.
	const std::vector<std::string> &getInvsList() const;
	/// Generates a new soldier.
	Soldier *genSoldier(SavedGame *save, std::string type = "") const;
	/// Generates a new vehicle.
	//Soldier *genVehicle(SavedGame *save, Language* lang, const std::string& soldierType) const;
	/// Gets the item to be used as fuel for ships.
	std::string getAlienFuelName() const;
	/// Gets the amount of alien fuel to recover
	int getAlienFuelQuantity() const;
	/// Gets the font name.
	std::string getFontName() const;
	/// Gets the amount of base experience soldiers earn for going on a mission.
	int getBaseMissionExperience() const { return _baseMissionExperience; }
	/// Gets the maximum percentage of TU debt.
	int getMaxTUDebt() const { return _maxTUDebt; }
	/// Gets the number of user selectable colors for armor coloration.
	const std::map<std::string, Uint8> &getSoldierArmorBaseColors() const { return _soldierArmorBaseColors; }
	const std::vector<std::string> &getSoldierArmorBaseColorIndex() const { return _soldierArmorBaseColorIndex; }
	/// Gets the minimum radar's range.
	int getMinRadarRange() const;
	/// Gets information on an interface element.
	RuleInterface *getInterface(const std::string &id, bool error = true) const;
	/// Gets the ruleset for the globe.
	RuleGlobe *getGlobe() const;
	/// Gets the ruleset for the converter.
	RuleConverter *getConverter() const;
	/// Gets the list of selective files for insertion into our cat files.
	const std::map<std::string, SoundDefinition *> *getSoundDefinitions() const;
	/// Gets the list of transparency colors,
	const std::vector<SDL_Color> *getTransparencies() const;
	const std::vector<MapScript*> *getMapScript(const std::string& id) const;
	/// Gets a video for intro/outro etc.
	RuleVideo *getVideo(const std::string &id, bool error = false) const;
	const std::map<std::string, RuleMusic *> *getMusic() const;
	const std::vector<std::string> *getMissionScriptList() const;
	RuleMissionScript *getMissionScript(const std::string &name, bool error = false) const;
	/// Get global script data.
	ScriptGlobal *getScriptGlobal() const;
	std::string getFinalResearch() const;
	const std::map<int, std::string> *getMissionRatings() const;
	const std::map<int, std::string> *getMonthlyRatings() const;
	const std::map<std::string, std::string> &getFixedUserOptions() const;
	const std::vector<std::string> &getHiddenMovementBackgrounds() const;
	StatAdjustment *getStatAdjustment(int difficulty);
	int getDefeatScore() const;
	int getDefeatFunds() const;
	const std::map<std::string, RuleEffect*> *getEffects() const;
	RuleEffect *getEffect(const std::string &type) const;
	std::map<std::string, RuleInventoryLayout*> *getInventoryLayouts();
	RuleInventoryLayout *getInventoryLayout(const std::string &id) const;
	const std::vector<std::string> &getInventoryLayoutsList() const;
	int getActionExperience(const std::string &action) const;
	float getDamageDropoff(int damageType) const;
};

}
