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
#include "Position.h"
#include "BattlescapeGame.h"
#include "../Mod/RuleItem.h"
#include "../Mod/MapData.h"
#include "../Mod/Unit.h" // UnitSide / UnitBodyPart - needed as hitUnit's default arguments

namespace OpenXcom
{

class SavedBattleGame;
class BattleUnit;
class BattleItem;
class Tile;
class RuleSkill;
struct BattleAction;
template<typename Tag, typename DataType> struct AreaSubset;

enum UnitBodyPart : int;

/**
 * Define some part of map
 */
using MapSubset = AreaSubset<Position, Sint16>;
enum BattleActionType : Uint8;
enum BattleActionMove : char; // DX: mover's movement mode, for mode-aware evasion in reaction fire
enum LightLayers : Uint8;


/**
 * A utility class that modifies tile properties on a battlescape map. This includes lighting, destruction, smoke, fire, fog of war.
 * Note that this function does not handle any sounds or animations.
 */
class TileEngine
{
public:
	/// Value representing non-existing position.
	static constexpr Position invalid = { -1, -1, -1 };

	/// Size of tile in voxels
	static constexpr Position voxelTileSize = { Position::TileXY, Position::TileXY, Position::TileZ };
	/// Half of size of tile in voxels
	static constexpr Position voxelTileCenter = { Position::TileXY / 2, Position::TileXY / 2, Position::TileZ / 2 };

	/// Calculate distance of each step of trajectory.
	static float trajectoryStepSize(const std::vector<Position>& voxelPath, size_t pos)
	{
		if (pos < voxelPath.size())
		{
			if (pos > 2)
			{
				return 0.5f * Position::distance(voxelPath[pos], voxelPath[pos - 2]);
			}
			else if (pos)
			{
				return Position::distance(voxelPath[1], voxelPath[0]);
			}
		}
		return 0.0f;
	}

private:
	/**
	 * Helper class storing cached visibility blockage data.
	 */
	struct VisibilityBlockCache
	{
		Uint32 blockDir;

		Uint8 bigWall;

		Uint8 height;
	};

	/**
	 * Helper class storing reaction data.
	 */
	struct ReactionScore
	{
		BattleUnit *unit;
		BattleItem *weapon;
		BattleActionType attackType;
		double reactionScore;
		double reactionReduction;
		int count;
		bool overwatch = false; // DX: this is a pre-paid overwatch shot (free TU, decrements shots)
	};

	SavedBattleGame *_save;
	const std::vector<Uint16> *_voxelData;

	/// Cache for tile visibility and light propagation.
	std::vector<VisibilityBlockCache> _blockVisibility;
	/// Cache dedicated for speedup for light propagation calculation.
	std::vector<Uint32> _lightPropagationTerrainBlocking;
	/// Cache for marking tiles that need light updated.
	std::vector<Uint32> _lightPropagationTempNeedUpdate;

	const RuleInventory *_inventorySlotGround;
	constexpr static int heightFromCenter[11] = {0,-2,+2,-4,+4,-6,+6,-8,+8,-12,+12};
	bool _personalLighting;
	Tile *_cacheTile;
	Tile *_cacheTileBelow;
	Position _cacheTilePos;
	const int _maxViewDistance;        // 20 tiles by default
	const int _maxViewDistanceSq;      // 20 * 20
	const int _maxVoxelViewDistance;   // maxViewDistance * 16
	const int _maxDarknessToSeeUnits;  // 9 by default
	const int _maxStaticLightDistance;
	const int _maxDynamicLightDistance;
	const int _enhancedLighting;
	Position _eventVisibilitySectorL, _eventVisibilitySectorR, _eventVisibilityObserverPos;
	std::vector<BattleUnit*> _movingUnitPrev;
	BattleUnit* _movingUnit = nullptr;

	/// Add light source. DX: coneDirection (0-7) + coneAngleDeg > 0 restrict the light to a
	/// facing cone (directional/flashlight light); coneDirection -1 = circular (default).
	void addLight(MapSubset gs, Position center, int power, LightLayers layer, int coneDirection = -1, int coneAngleDeg = 0);
	/// Calculate blockage amount.
	int blockage(Tile *tile, const TilePart part, ItemDamageType type, int direction = -1, bool checkingFromOrigin = false);

	bool setupEventVisibilitySector(const Position &observerPos, const Position &eventPos, const int &eventRadius);
	inline bool inEventVisibilitySector(const Position &toCheck) const;

	/// Calculates sun shading of the whole map.
	void calculateSunShading(MapSubset gs);
	/// Recalculates lighting of the battlescape for terrain.
	void calculateTerrainBackground(MapSubset gs);
	/// Recalculates lighting of the battlescape for terrain.
	void calculateTerrainItems(MapSubset gs);
	/// Recalculates lighting of the battlescape for units.
	void calculateUnitLighting(MapSubset gs);

	/// Checks validity of a snap shot to this position.
	ReactionScore determineReactionType(BattleUnit *unit, BattleUnit *target);
	/// Creates a vector of units that can spot this unit.
	std::vector<ReactionScore> getSpottingUnits(BattleUnit* unit, BattleActionMove moverMove);
	/// Given a vector of spotters, and a unit, picks the spotter with the highest reaction score.
	ReactionScore *getReactor(std::vector<ReactionScore> &spotters, BattleUnit *unit, BattleActionMove moverMove);
	/// Tries to perform a reaction snap shot to this location.
	bool tryReaction(ReactionScore *reaction, BattleUnit *target, const BattleAction &originalAction);
public:
	/// Creates a new TileEngine class.
	TileEngine(SavedBattleGame *save, Mod *mod);
	/// Cleans up the TileEngine.
	~TileEngine();

	/// Get max distance that fire light can reach.
	int getMaxStaticLightDistance() const { return _maxStaticLightDistance; }
	/// Get max distance that light can reach.
	int getMaxDynamicLightDistance() const { return _maxDynamicLightDistance; }
	/// Get flags for enhanced lighting.
	int getEnhancedLighting() const { return _enhancedLighting; }
	/// Get max view distance.
	int getMaxViewDistance() const { return _maxViewDistance; }
	/// Get square of max view distance.
	int getMaxViewDistanceSq() const { return _maxViewDistanceSq; }
	/// Get max view distance in voxel space.
	int getMaxVoxelViewDistance() const { return _maxVoxelViewDistance; }
	/// Get threshold of darkness for LoS calculation.
	int getMaxDarknessToSeeUnits() const { return _maxDarknessToSeeUnits; }

	/// Calculates visible tiles within the field of view. Supply an eventPosition to do an update limited to a small slice of the view sector.
	void calculateTilesInFOV(BattleUnit *unit, const Position eventPos = invalid, const int eventRadius = 0);
	/// Calculates visible units within the field of view. Supply an eventPosition to do an update limited to a small slice of the view sector.
	bool calculateUnitsInFOV(BattleUnit* unit, const Position eventPos = invalid, const int eventRadius = 0);
	/// Calculates the field of view from a units view point.
	bool calculateFOV(BattleUnit *unit, bool doTileRecalc = true, bool doUnitRecalc = true);
	/// Calculates the field of view within range of a certain position.
	void calculateFOV(Position position, int eventRadius = -1, const bool updateTiles = true, const bool appendToTileVisibility = false);
	/// Checks reaction fire.
	bool checkReactionFire(BattleUnit *unit, const BattleAction &originalAction);
	/// DX: tests whether a tile is inside an overwatch cone (min/max range + full cone angle from origin→aim).
	bool isInOverwatchCone(Position origin, Position aimTarget, Position tile, int minRange, int maxRange, int coneAngleDeg) const;
	/// Recalculate all lighting in some area.
	void calculateLighting(LightLayers layer, Position position = invalid, int eventRadius = 0, bool terrianChanged = false);
	/// DX: what light a unit emits (armor personal light honoring the toggle, carried glow items, fire).
	struct UnitLightEmission
	{
		int circular = 0;  ///< brightest circular emission (personal light, non-cone glow items, fire)
		int conePower = 0; ///< brightest directional glow item's power (0 = none)
		int coneAngle = 0; ///< that item's full cone angle in degrees
		/// The unit's strongest emission (for the sneak light gate).
		int total() const { return circular > conePower ? circular : conePower; }
	};
	/// DX: computes the unit's light emission (shared by the lighting pass and the sneak gate).
	UnitLightEmission getUnitLightEmission(const BattleUnit *unit) const;
	/// Handles tile hit.
	int hitTile(Tile *tile, int damage, const RuleDamageType* type);
	/// Handles experience training.
	bool awardExperience(BattleActionAttack attack, BattleUnit *target, bool rangeAtack);
	/// Handles unit hit. `sideOverride`/`bodypartOverride` force where the hit lands instead of deriving
	/// it from `relative` (DX: a mind blast always strikes the head). `awardExp` lets a caller that awards
	/// its own experience opt out of the standard weapon-based award.
	bool hitUnit(BattleActionAttack attack, BattleUnit *target, const Position &relative, int damage, const RuleDamageType *type, bool rangeAtack = true, UnitSide sideOverride = SIDE_MAX, UnitBodyPart bodypartOverride = BODYPART_MAX, bool awardExp = true);
	/// Handles bullet/weapon hits.
	void hit(BattleActionAttack attack, Position center, int power, const RuleDamageType *type, bool rangeAtack = true, int terrainMeleeTilePart = 0);
	/// Handles explosions.
	void explode(BattleActionAttack attack, Position center, int power, const RuleDamageType *type, int maxRadius, bool rangeAtack = true);
	/// Checks if a destroyed tile starts an explosion.
	Tile *checkForTerrainExplosions();
	/// Unit opens door?
	int unitOpensDoor(BattleUnit *unit, bool rClick = false, int dir = -1);
	/// Closes ufo doors.
	int closeUfoDoors();
	/// Calculates a line trajectory in tile space.
	int calculateLineTile(Position origin, Position target, std::vector<Position> &trajectory);
	/// Aim-cone: accuracy multiplier (0-1) from smoke along the shooter->target line of fire.
	double getSmokeAccuracyFactor(Position originVoxel, Position targetVoxel);
	/// Calculates a line trajectory in voxel space.
	VoxelType calculateLineVoxel(Position origin, Position target, bool storeTrajectory, std::vector<Position> *trajectory, BattleUnit *excludeUnit, BattleUnit *excludeAllBut = 0, bool onlyVisible = false);
	/// Calculates a parabola trajectory.
	int calculateParabolaVoxel(Position origin, Position target, bool storeTrajectory, std::vector<Position> *trajectory, BattleUnit *excludeUnit, double curvature, const Position delta);
	/// Gets the origin voxel of a unit's eyesight.
	Position getSightOriginVoxel(BattleUnit *currentUnit);
	/// Checks visibility of a unit on this tile.
	bool visible(BattleUnit *currentUnit, Tile *tile);
	/// Checks visibility of a tile.
	bool isTileInLOS(BattleAction *action, Tile *tile, bool drawing);
	/// Turn XCom soldiers' personal lighting on or off (squad-wide master toggle).
	void togglePersonalLighting();
	/// DX: toggle one unit's personal light on or off (per-unit switch).
	void toggleUnitPersonalLight(BattleUnit *unit);
	/// Checks the horizontal blockage of a tile.
	int horizontalBlockage(Tile *startTile, Tile *endTile, ItemDamageType type, bool skipObject = false);
	/// Checks the vertical blockage of a tile.
	int verticalBlockage(Tile *startTile, Tile *endTile, ItemDamageType type, bool skipObject = false);

	/// Calculate success rate of psi attack. The optional out-params report the contest inputs (attack
	/// score, defence score, tile distance) for DX's verbose combat log, so a caller that actually
	/// resolves an attack can show the maths without recomputing it; speculative callers (the AI's
	/// target scoring) simply pass nothing and log nothing.
	int psiAttackCalculate(BattleActionAttack::ReadOnly attack, const BattleUnit *victim, int *attackStrengthOut = nullptr, int *defenseStrengthOut = nullptr, int *distanceOut = nullptr);
	/// DX: a mind blast - a psi contest that deals damage scaled by the margin, or backlashes on a miss.
	bool mindBlast(BattleUnit *actor, BattleUnit *victim, BattleItem *ampItem);
	/// DX: a clairvoyant sweep - reveals the map around a target tile, and marks the units in it.
	bool clairvoyance(BattleUnit *actor, Position target, const RuleItem *amp);
	/// DX: makes a psi controller suffer for a mind control that went wrong (failed attempt / thrall death).
	void applyPsiBacklash(BattleUnit *controller, const RuleItem *amp, const RulePsiBacklash &backlash, int damageTypeId = -1);
	/// Attempts a panic or mind control action.
	bool psiAttack(BattleActionAttack attack, BattleUnit *victim);
	/// Calculate success rate of melee attack action.
	int meleeAttackCalculate(BattleActionAttack::ReadOnly attack, const BattleUnit *victim);
	/// Attempts a melee attack action.
	bool meleeAttack(BattleActionAttack attack, BattleUnit *victim, int terrainMeleeTilePart = 0);

	/// Remove the medikit from the game if consumable and empty.
	void medikitRemoveIfEmpty(BattleAction *action);
	/// Try using medikit heal ability.
	bool medikitUse(BattleAction *action, BattleUnit *target, BattleMediKitAction medikitAction, UnitBodyPart bodyPart);
	/// Try using a skill.
	bool skillUse(BattleAction *action, const RuleSkill *skill);
	/// Try to conceal a unit.
	bool tryConcealUnit(BattleUnit* unit);
	/// Applies gravity to anything that occupy this tile.
	Tile *applyGravity(Tile *t);

	/// Drop item on ground.
	void itemDrop(Tile *t, BattleItem *item, bool updateLight);
	/// Drop all unit items on ground.
	void itemDropInventory(Tile *t, BattleUnit *unit, bool unprimeItems = false, bool deleteFixedItems = false);
	/// Move item to other place in inventory or ground.
	void itemMoveInventory(Tile *t, BattleUnit *unit, BattleItem *item, const RuleInventory *slot, int x, int y);

	/// Add moving unit.
	void addMovingUnit(BattleUnit* unit);
	/// Add moving unit.
	void removeMovingUnit(BattleUnit* unit);
	/// Get current moving unit.
	BattleUnit* getMovingUnit();

	/// Returns melee validity between two units.
	bool validMeleeRange(BattleUnit *attacker, BattleUnit *target, int dir);
	/// Returns validity of a melee attack from a given position.
	bool validMeleeRange(Position pos, int direction, BattleUnit *attacker, BattleUnit *target, Position *dest, bool preferEnemy = true);
	bool validTerrainMeleeRange(BattleAction* action);
	/// Gets the AI to look through a window.
	int faceWindow(Position position);
	/// Checks a unit's % exposure on a tile.
	int checkVoxelExposure(Position *originVoxel, Tile *tile, BattleUnit *excludeUnit, BattleUnit *excludeAllBut);
	/// Checks validity for targetting a unit.
	bool canTargetUnit(Position *originVoxel, Tile *tile, Position *scanVoxel, BattleUnit *excludeUnit, bool rememberObstacles, BattleUnit *potentialUnit = 0);
	/// Check validity for targetting a tile.
	bool canTargetTile(Position *originVoxel, Tile *tile, int part, Position *scanVoxel, BattleUnit *excludeUnit, bool rememberObstacles);
	/// Resolves the aim voxel for a direct (non-arcing) shot (unit->object->walls->floor priority); returns false if no line of fire.
	bool resolveFireTargetVoxel(BattleAction &action, Position origin, bool rememberObstacles, Position *targetVoxel);
	/// Calculates the z voxel for shadows.
	int castedShade(Position voxel);
	/// Checks the visibility of a given voxel.
	bool isVoxelVisible(Position voxel);
	/// Checks what type of voxel occupies this space.
	VoxelType voxelCheck(Position voxel, BattleUnit *excludeUnit, bool excludeAllUnits = false, bool onlyVisible = false, BattleUnit *excludeAllBut = 0);
	/// Flushes cache of voxel check
	void voxelCheckFlush();
	/// Blows this tile up.
	bool detonate(Tile* tile, int power);
	/// Validates a throwing action.
	bool validateThrow(BattleAction &action, Position originVoxel, Position targetVoxel, int depth, double *curve = 0, int *voxelType = 0, bool forced = false);
	/// Opens any doors this door is connected to.
	std::pair<int, Position> checkAdjacentDoors(Position pos, TilePart part);
	/// Recalculates FOV of all units in-game.
	void recalculateFOV();
	/// Get direction to a certain point
	int getDirectionTo(Position origin, Position target) const;
	/// Get arc between two direction.
	int getArcDirection(int directionA, int directionB) const;
	/// determine the origin voxel of a given action.
	Position getOriginVoxel(BattleAction &action, Tile *tile);
	/// mark a region of the map as "dangerous" for a turn.
	void setDangerZone(Position pos, int radius, BattleUnit *unit);
	/// Checks if a position is valid for a unit, used for spawning and forced movement.
	bool isPositionValidForUnit(Position &position, BattleUnit *unit, bool checkSurrounding = false, int startSurroundingCheckDirection = 0);
	/// Update game state after script hook execution.
	void updateGameStateAfterScript(BattleActionAttack battleActionAttack, Position pos);

};

}
