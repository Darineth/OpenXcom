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
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Options.h"
#include "../Engine/Collections.h"
#include "../Mod/MapData.h"
#include "Position.h"
#include "Particle.h"
#include "Projectile.h" // SpreadSample (aim-cone/throw spread visualization)
#include <vector>

namespace OpenXcom
{

class SavedBattleGame;
class Surface;
class SurfaceSet;
class BattleUnit;
class Projectile;
class Explosion;
class BattlescapeMessage;
class Camera;
class Timer;
class Text;
class Tile;
class UnitSprite;

enum CursorType { CT_NONE, CT_NORMAL, CT_AIM, CT_PSI, CT_WAYPOINT, CT_THROW };
enum TilePart : int;

/**
 * Helper class that returns all important data about the unit movement
 */
struct UnitWalkingOffset
{
	Position ScreenOffset;
	int NormalizedMovePhase;
	int TerrainLevelOffset;
};

/**
 * Interactive map of the battlescape.
 */
class Map : public InteractiveSurface
{
private:
	static const int SCROLL_INTERVAL = 15;
	static const int FADE_INTERVAL = 23;
	static const int NIGHT_VISION_SHADE = 4;
	static const int NIGHT_VISION_MAX_SHADE = 8;
	static const int BULLET_SPRITES = 35;
	Timer *_scrollMouseTimer, *_scrollKeyTimer, *_obstacleTimer;
	Timer *_fadeTimer;
	int _fadeShade;
	bool _nightVisionOn;
	int _debugVisionMode;
	int _nvColor;
	Game *_game;
	SavedBattleGame *_save;
	bool _isTFTD;
	Surface *_arrow;
	Surface *_grenadeIndicator;
	static const int PROXY_PING_FRAMES = 2;
	Surface *_proxyPing[PROXY_PING_FRAMES];
	Surface *_stunIndicator, *_woundIndicator, *_burnIndicator, *_shockIndicator;
	// DX: procedural fallback status icons, built in init() and owned by Map, used when the mod
	// supplies no matching Floor*Indicator surface so the on-map status glyphs work out of the box.
	Surface *_stunIndicatorFallback, *_woundIndicatorFallback, *_burnIndicatorFallback, *_shockIndicatorFallback;
	bool _anyIndicator, _isAltPressed, _isCtrlPressed;
	int _spriteWidth, _spriteHeight;
	int _selectorX, _selectorY;
	int _mouseX, _mouseY;
	CursorType _cursorType;
	int _cursorSize;
	int _cacheActiveWeaponUfopediaArticleUnlocked; // -1 = unknown, 0 = locked, 1 = unlocked
	bool _cacheIsCtrlPressed;
	Position _cacheCursorPosition;
	int _cacheHasLOS; // -1 = unknown, 0 = no LOS, 1 = has LOS
	// Cache for the DX aim-cone hit-chance crosshair readout (expensive: voxel-traced Monte-Carlo);
	// recomputed only when the aim (cursor tile / ctrl / weapon / action type) changes, not every frame.
	int _cacheHitChance; // -1 = unknown, else 0-100 (right/primary hand under dual-fire)
	int _cacheHitChance2; // dual-fire: the left/off hand's paired value (-1 if n/a)
	int _cacheHitChanceCover; // cover-reduction term (percentage points) paired with _cacheHitChance
	Position _cacheHitChancePosition;
	int _cacheHitChanceCtrl; // -1 = unknown, else 0/1
	const void* _cacheHitChanceWeapon;
	int _cacheHitChanceActionType;
	int _cacheHitChanceKneeled; // -1 = unknown, else 0/1 (stance affects muzzle height + accuracy)
	bool _cursorAccuracyShown; // per-frame: was the crosshair accuracy readout drawn this pass?
	                          // (lets the hovered-unit name stack above it)
	// Deferred crosshair text: the accuracy readout and hovered-unit name are prepared during the
	// tile pass but blitted AFTER the targeting tracers/dots, so the text stays legible on top.
	bool _pendingAccuracyText, _pendingUnitName;
	int _accuracyTextX, _accuracyTextY, _unitNameX, _unitNameY;
	int _animFrame;
	std::vector<Projectile*> _projectiles;
	bool _followProjectile;
	bool _projectileInFOV;
	std::list<Explosion *> _explosions;
	std::vector<std::vector<Particle>> _vaporParticlesInit;
	std::vector<std::vector<Particle>> _vaporParticles;
	bool _explosionInFOV, _launch;
	BattlescapeMessage *_message;
	Camera *_camera;
	int _visibleMapHeight;
	std::vector<Position> _waypoints;
	bool _unitDying, _smoothCamera, _smoothingEngaged, _flashScreen;
	int _bgColor;
	bool _previewSettingArrows, _previewSettingTu, _previewSettingEnergy;
	Text *_txtAccuracy;
	Text *_txtUnitName;
	SurfaceSet *_projectileSet;
	// DX: live aiming trajectory preview - a dedicated projectile (kept out of the in-flight
	// _projectiles collection) whose stored ideal trajectory is drawn as tracer sprites while a
	// fire/throw action is being targeted. Rebuilt only when the aim target/action changes.
	Projectile *_targetingProjectile;
	Position _previewTarget;
	int _previewActionType;
	void *_previewActor;
	// DX aim-cone / throw spread visualization: Alt-held sampled impacts/landings (voxel + hit flag)
	// to draw as a dot cloud (in place of the single tracer line). Rebuilt with the preview.
	std::vector<SpreadSample> _targetingDots;
	bool _previewAlt;

	void drawUnit(UnitSprite &unitSprite, Tile *unitTile, Tile *currTile, Position tileScreenPosition, bool topLayer, BattleUnit* movingUnit = nullptr);
	/// Rebuilds the live aiming trajectory preview for the current cursor/action (or clears it).
	void updateTargetingPreview();
	/// Discards the live aiming trajectory preview.
	void clearTargetingPreview();
	/// Draws the live aiming trajectory preview (tracer dots + impact marker).
	void drawTargetingPreview(Surface *surface);
	/// DX: draws markers over the tiles inside the active overwatch cone (while aiming, or when a unit
	/// already on overwatch is selected).
	void drawOverwatchCone(Surface *surface);
	void drawTerrain(Surface *surface);
	int getTerrainLevel(const Position& pos, int size) const;
	int getWallShade(TilePart part, Tile* tileFrot);
	int _iconHeight, _iconWidth, _messageColor;
	int _hostileBarColor, _neutralBarColor, _borderBarColor;
	const std::vector<Uint8> *_transparencies;
	bool _showObstacles;
	bool _showInfoOnCursor;
public:
	/// Creates a new map at the specified position and size.
	Map(Game* game, int width, int height, int x, int y, int visibleMapHeight);
	/// Cleans up the map.
	~Map();
	/// Initializes the map.
	void init();
	/// Handles timers.
	void think() override;
	/// Draws the surface.
	void draw() override;
	void refreshAIProgress(int progress);
	/// Sets the palette.
	void setPalette(const SDL_Color *colors, int firstcolor = 0, int ncolors = 256) override;
	void refreshHiddenMovementBackground();
	/// Special handling for mouse press.
	void mousePress(Action *action, State *state) override;
	/// Special handling for mouse release.
	void mouseRelease(Action *action, State *state) override;
	/// Special handling for mouse over
	void mouseOver(Action *action, State *state) override;
	/// Special handling for key presses.
	void keyboardPress(Action *action, State *state) override;
	/// Special handling for key releases.
	void keyboardRelease(Action *action, State *state) override;
	/// Rotates the tile frames 0-7
	void animate(bool redraw);
	/// Sets the battlescape selector position relative to mouse position.
	void setSelectorPosition(int mx, int my);
	/// Gets the currently selected position.
	void getSelectorPosition(Position *pos) const;
	/// Calculates the offset of a soldier, when it is walking in the middle of 2 tiles.
	UnitWalkingOffset calculateWalkingOffset(const BattleUnit *unit) const;
	/// Sets the 3D cursor type.
	void setCursorType(CursorType type, int size = 1);
	/// Gets the 3D cursor type.
	CursorType getCursorType() const;

	/// Adds a projectile to the in-flight collection.
	void addProjectile(Projectile *p);
	/// Removes a projectile from the collection and deletes it.
	void removeProjectile(Projectile *p);
	/// Returns true if any projectiles are currently in flight.
	bool hasProjectiles() const;
	/// Gets all in-flight projectiles.
	const std::vector<Projectile*>& getProjectiles() const;
	/// Sets follow projectile flag.
	void setFollowProjectile(bool followProjectile) { _followProjectile = followProjectile; }
	/// Gets follow projectile flag.
	bool getFollowProjectile() const { return _followProjectile; }
	/// Gets alt pressed flag.
	bool isAltPressed() const { return _isAltPressed; }
	/// Gets ctrl pressed flag.
	bool isCtrlPressed() const { return _isCtrlPressed; }
	/// Add new vapor particle.
	void addVaporParticle(Position pos, Particle particle);
	/// Get all vapor for tile.
	Collections::Range<const Particle*> getVaporParticle(const Tile* tile, int topLayer) const;
	/// Gets explosion set.
	std::list<Explosion*> *getExplosions();

	/// Gets the pointer to the camera.
	Camera *getCamera();
	/// Mouse-scrolls the camera.
	void scrollMouse();
	/// Keyboard-scrolls the camera.
	void scrollKey();
	/// fades in/out
	void fadeShade();
	/// Get waypoints vector.
	std::vector<Position> *getWaypoints();
	/// Set mouse-buttons' pressed state.
	void setButtonsPressed(Uint8 button, bool pressed);
	/// Sets the unitDying flag.
	void setUnitDying(bool flag);
	/// Refreshes the battlescape selector after scrolling.
	void refreshSelectorPosition();
	/// Special handling for updating map height.
	void setHeight(int height) override;
	/// Special handling for updating map width.
	void setWidth(int width) override;
	/// Get the vertical position of the hidden movement screen.
	int getMessageY() const;
	/// Get the icon height.
	int getIconHeight() const;
	/// Get the icon width.
	int getIconWidth() const;
	/// Convert a map position to a sound angle.
	int getSoundAngle(const Position& pos) const;
	/// Reset the camera smoothing bool.
	void resetCameraSmoothing();
	/// Set whether the screen should "flash" or not.
	void setBlastFlash(bool flash);
	/// Check if the screen is flashing this.
	bool getBlastFlash() const;
	/// Modify shade for fading
	int reShade(Tile *tile);
	/// toggle the night-vision mode
	void enableNightVision();
	void toggleNightVision();
	void toggleDebugVisionMode();
	void persistToggles();
	/// Resets obstacle markers.
	void resetObstacles();
	/// Enables obstacle markers.
	void enableObstacles();
	/// Disables obstacle markers.
	void disableObstacles();
};

}
