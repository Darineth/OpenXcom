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
#include "Map.h"
#include "Camera.h"
#include "UnitSprite.h"
#include "ItemSprite.h"
#include "Pathfinding.h"
#include "TileEngine.h"
#include "Projectile.h"
#include "Explosion.h"
#include "BattlescapeState.h"
#include "Particle.h"
#include "../Mod/Mod.h"
#include "../Engine/Action.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Engine/Game.h"
#include "../Engine/Screen.h"
#include "../Engine/ShaderDraw.h"
#include "../Engine/ShaderMove.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Ufopaedia/Ufopaedia.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleInterface.h"
#include "../Mod/MapDataSet.h"
#include "../Mod/MapData.h"
#include "../Mod/Armor.h"
#include "../Mod/RuleEnviroEffects.h"
#include "BattlescapeMessage.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Role.h"
#include "../Mod/RuleRoleIcon.h"
#include "../Interface/NumberText.h"
#include "../Interface/Text.h"
#include "../fmath.h"


/*
  1) Map origin is top corner.
  2) X axis goes downright. (width of the map)
  3) Y axis goes downleft. (length of the map
  4) Z axis goes up (height of the map)

           0,0
            /\
           /  \
        y+ \  / x+
            \/

  Compass directions

         W  /\  N
           /  \
           \  /
         S  \/  E

  Unit directions

         6  /\  0
           /  \
           \  /
         4  \/  2

  Big units parts

            /\
           /0 \
          /\  /\
         /2 \/1 \
         \  /\  /
          \/3 \/
           \  /
            \/
 */

namespace OpenXcom
{

/**
 * DX: layout of the "Pathfinding2" tile-marker sprite sheet (bin/common/Resources/Pathfinding/
 * Pathfinding2.png). Each COLUMN is one marker type; row 0 holds its solid variant and row 1 its
 * dithered (translucent-reading) variant. SurfaceSet frames number row-major, so the frame index
 * of a variant depends on the sheet's column count - always resolve frames through solidFrame()/
 * ditheredFrame() and keep Columns in sync when the sheet grows a new marker column.
 */
namespace Pathfinding2Markers
{
	/// Number of marker columns in the sheet - UPDATE when new columns are added.
	const int Columns = 4;
	/// Column 0: the full-tile outline marker.
	const int FullTile = 0;
	/// Column 1: the crossed-out tile marker (no consumer yet).
	const int Cross = 1;
	/// Column 2: the target marker.
	const int Target = 2;
	/// Column 3: the X "no" marker (overwatch dead zones).
	const int No = 3;
	/// Frame index of a marker's solid variant (row 0).
	inline int solidFrame(int column) { return column; }
	/// Frame index of a marker's dithered variant (row 1).
	inline int ditheredFrame(int column) { return Columns + column; }
}

/**
 * Sets up a map with the specified size and position.
 * @param game Pointer to the core game.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 * @param visibleMapHeight Current visible map height.
 */
Map::Map(Game *game, int width, int height, int x, int y, int visibleMapHeight) : InteractiveSurface(width, height, x, y),
	_game(game), _isTFTD(false), _arrow(0), _grenadeIndicator(0), _proxyPing{},
	_stunIndicatorFallback(0), _woundIndicatorFallback(0), _burnIndicatorFallback(0), _shockIndicatorFallback(0),
	_bleedoutIndicator(0), _bleedoutIndicatorFallback(0),
	_channelingIndicator(0), _enthralledIndicator(0), _channelingIndicatorFallback(0), _enthralledIndicatorFallback(0),
	_anyIndicator(false), _isAltPressed(false), _isCtrlPressed(false),
	_selectorX(0), _selectorY(0), _mouseX(0), _mouseY(0), _cursorType(CT_NORMAL), _cursorSize(1), _animFrame(0),
	_followProjectile(true), _projectileInFOV(false), _explosionInFOV(false), _launch(false), _visibleMapHeight(visibleMapHeight),
	_unitDying(false), _deathFocus(false), _smoothingEngaged(false), _flashScreen(false), _bgColor(15), _projectileSet(0),
	_targetingProjectile(0), _previewTarget(-1, -1, -1), _previewActionType(-1), _previewActor(0), _previewAlt(false),
	_showObstacles(false), _showInfoOnCursor(false), _owLosCacheOrigin(-1, -1, -1)
{
	// TODO: extract to a better place later
	for (const auto& pair : Options::mods)
	{
		if (pair.second)
		{
			if (pair.first == "xcom2")
			{
				_isTFTD = true;
				break;
			}
		}
	}

	_iconHeight = _game->getMod()->getInterface("battlescape")->getElement("icons")->h;
	_iconWidth = _game->getMod()->getInterface("battlescape")->getElement("icons")->w;
	_messageColor = _game->getMod()->getInterface("battlescape")->getElement("messageWindows")->color;

	auto* itf = _game->getMod()->getInterface("battlescape")->getElement("thinkingProgressBar");
	_hostileBarColor = itf->color;
	_neutralBarColor = itf->color2;
	_borderBarColor = itf->border;

	PathPreview previewSetting = Options::battleNewPreviewPath;
	_smoothCamera = Options::battleSmoothCamera;
	if (Options::traceAI)
	{
		// turn everything on because we want to see the markers.
		previewSetting = PATH_ARROW_TU;
	}
	_previewSettingArrows = previewSetting & PATH_ARROWS;
	_previewSettingTu     = previewSetting & PATH_TU_COST;
	_previewSettingEnergy = previewSetting & PATH_ENERGY_COST;

	_save = _game->getSavedGame()->getSavedBattle();
	if ((int)(_game->getMod()->getLUTs()->size()) > _save->getDepth())
	{
		_transparencies = &_game->getMod()->getLUTs()->at(_save->getDepth());
	}
	else
	{
		const static std::vector<Uint8> dummy;
		_transparencies = &dummy;
	}

	_spriteWidth = _game->getMod()->getSurfaceSet("BLANKS.PCK")->getFrame(0)->getWidth();
	_spriteHeight = _game->getMod()->getSurfaceSet("BLANKS.PCK")->getFrame(0)->getHeight();
	_message = new BattlescapeMessage(320, (visibleMapHeight < 200)? visibleMapHeight : 200, 0, 0);
	_message->setX(_game->getScreen()->getDX());
	_message->setY((visibleMapHeight - _message->getHeight()) / 2);
	_message->setTextColor(_messageColor);
	_camera = new Camera(_spriteWidth, _spriteHeight, _save->getMapSizeX(), _save->getMapSizeY(), _save->getMapSizeZ(), this, visibleMapHeight);
	_scrollMouseTimer = new Timer(SCROLL_INTERVAL);
	_scrollMouseTimer->onTimer((SurfaceHandler)&Map::scrollMouse);
	_scrollKeyTimer = new Timer(SCROLL_INTERVAL);
	_scrollKeyTimer->onTimer((SurfaceHandler)&Map::scrollKey);
	_camera->setScrollTimer(_scrollMouseTimer, _scrollKeyTimer);
	_obstacleTimer = new Timer(2500);
	_obstacleTimer->stop();
	_obstacleTimer->onTimer((SurfaceHandler)&Map::disableObstacles);

	_showInfoOnCursor = (Options::oxceShowAccuracyOnCrosshair == 1 && Options::battleUFOExtenderAccuracy) || Options::oxceShowAccuracyOnCrosshair == 2;
	// Wide and centered so the one-line "X% (-Y%) @ Zm" readout fits and sits centered over the tile.
	_txtAccuracy = new Text(90, 18, 0, 0);
	_txtAccuracy->setSmall();
	_txtAccuracy->setAlign(ALIGN_CENTER);
	_txtAccuracy->setPalette(_game->getScreen()->getPalette());
	_txtAccuracy->setHighContrast(true);
	_txtAccuracy->initText(_game->getMod()->getFont("FONT_BIG"), _game->getMod()->getFont("FONT_SMALL"), _game->getLanguage());
	// Floating label for the hovered unit's name (DX on-map overlay). Wide and centered so the
	// name balances over the tile regardless of length.
	_txtUnitName = new Text(120, 9, 0, 0);
	_txtUnitName->setSmall();
	_txtUnitName->setAlign(ALIGN_CENTER);
	_txtUnitName->setPalette(_game->getScreen()->getPalette());
	_txtUnitName->setHighContrast(true);
	_txtUnitName->initText(_game->getMod()->getFont("FONT_BIG"), _game->getMod()->getFont("FONT_SMALL"), _game->getLanguage());
	_cacheActiveWeaponUfopediaArticleUnlocked = -1;
	_cacheIsCtrlPressed = false;
	_cacheCursorPosition = TileEngine::invalid;
	_cacheHasLOS = -1;
	_cacheHitChance = -1;
	_cacheHitChance2 = -1;
	_cacheHitChanceCover = 0;
	_cacheHitChancePosition = TileEngine::invalid;
	_cacheHitChanceCtrl = -1;
	_cacheHitChanceWeapon = nullptr;
	_cacheHitChanceActionType = -1;
	_cacheHitChanceKneeled = -1;

	_nightVisionOn = false;
	if (Options::oxceToggleNightVisionType == 2)
	{
		// persisted per campaign
		_nightVisionOn = _game->getSavedGame()->getToggleNightVision();
	}
	else if (Options::oxceToggleNightVisionType == 1)
	{
		// persisted per battle
		_nightVisionOn = _save->getToggleNightVision();
	}

	_debugVisionMode = 0;
	if (Options::oxceToggleBrightnessType == 2)
	{
		// persisted per campaign
		_debugVisionMode = _game->getSavedGame()->getToggleBrightness();
	}
	else if (Options::oxceToggleBrightnessType == 1)
	{
		// persisted per battle
		_debugVisionMode = _save->getToggleBrightness();
	}

	_save->setToggleNightVisionTemp(false);
	_save->setToggleNightVisionColorTemp(0);
	_save->setToggleBrightnessTemp(_debugVisionMode);

	_fadeShade = 16;
	_nvColor = 0;
	_fadeTimer = new Timer(FADE_INTERVAL);
	_fadeTimer->onTimer((SurfaceHandler)&Map::fadeShade);
	_fadeTimer->start();

	auto* enviro = _save->getEnviroEffects();
	if (enviro)
	{
		_bgColor = enviro->getMapBackgroundColor();
	}

	_stunIndicator = _game->getMod()->getSurface("FloorStunIndicator", false);
	_woundIndicator = _game->getMod()->getSurface("FloorWoundIndicator", false);
	_burnIndicator = _game->getMod()->getSurface("FloorBurnIndicator", false);
	_shockIndicator = _game->getMod()->getSurface("FloorShockIndicator", false);
	_bleedoutIndicator = _game->getMod()->getSurface("FloorBleedoutIndicator", false);
	// DX: the two ends of a channeled mind-control link.
	_channelingIndicator = _game->getMod()->getSurface("FloorChannelingIndicator", false);
	_enthralledIndicator = _game->getMod()->getSurface("FloorEnthralledIndicator", false);
	_anyIndicator = _stunIndicator || _woundIndicator || _burnIndicator || _shockIndicator || _bleedoutIndicator;

	if (enviro)
	{
		if (!enviro->getMapShockIndicator().empty())
		{
			_shockIndicator = _game->getMod()->getSurface(enviro->getMapShockIndicator(), false);
		}
	}

	_vaporParticlesInit.resize(_camera->getMapSizeY() * _camera->getMapSizeX());
	_vaporParticles.resize(_camera->getMapSizeY() * _camera->getMapSizeX());
}

/**
 * Deletes the map.
 */
Map::~Map()
{
	delete _scrollMouseTimer;
	delete _scrollKeyTimer;
	delete _fadeTimer;
	delete _obstacleTimer;
	delete _arrow;
	delete _grenadeIndicator;
	for (Surface* frame : _proxyPing)
		delete frame;
	delete _stunIndicatorFallback;
	delete _woundIndicatorFallback;
	delete _burnIndicatorFallback;
	delete _shockIndicatorFallback;
	delete _bleedoutIndicatorFallback;
	delete _message;
	delete _camera;
	delete _txtAccuracy;
	delete _txtUnitName;
	delete _targetingProjectile;
}

/**
 * Initializes the map.
 */
void Map::init()
{
	// load the tiny arrow into a surface
	int f = Palette::blockOffset(1); // yellow
	int b = 15; // black
	int pixels[81] = { 0, 0, b, b, b, b, b, 0, 0,
					   0, 0, b, f, f, f, b, 0, 0,
					   0, 0, b, f, f, f, b, 0, 0,
					   b, b, b, f, f, f, b, b, b,
					   b, f, f, f, f, f, f, f, b,
					   0, b, f, f, f, f, f, b, 0,
					   0, 0, b, f, f, f, b, 0, 0,
					   0, 0, 0, b, f, b, 0, 0, 0,
					   0, 0, 0, 0, b, 0, 0, 0, 0 };

	_arrow = new Surface(9, 9);
	_arrow->setPalette(this->getPalette());
	_arrow->lock();
	for (int y = 0; y < 9;++y)
		for (int x = 0; x < 9; ++x)
			_arrow->setPixel(x, y, pixels[x+(y*9)]);
	_arrow->unlock();

	// DX on-map overlay: hovering markers for primed grenades lying on the ground (built once,
	// procedurally, in the battlescape palette - same technique as the selection arrow above). A
	// filled red disc marks a normal primed grenade; a cyan target-ring marks a proximity grenade,
	// so the two read as clearly different icons. Both can be overridden by a mod-supplied surface
	// (see drawTerrain). Palette indices: block 2 = red ramp, block 13 = cyan/blue ramp, 0 = clear.
	if (!_grenadeIndicator)
	{
		const int R = 0, F = 34, H = 32, B = 15; // clear / red fill / red highlight / black outline
		int disc[81] = {
			R, R, R, B, B, B, R, R, R,
			R, R, B, F, F, F, B, R, R,
			R, B, F, H, F, F, F, B, R,
			B, F, H, F, F, F, F, F, B,
			B, F, F, F, F, F, F, F, B,
			B, F, F, F, F, F, F, F, B,
			R, B, F, F, F, F, F, B, R,
			R, R, B, F, F, F, B, R, R,
			R, R, R, B, B, B, R, R, R };
		_grenadeIndicator = new Surface(9, 9);
		_grenadeIndicator->setPalette(this->getPalette());
		_grenadeIndicator->lock();
		for (int y = 0; y < 9; ++y)
			for (int x = 0; x < 9; ++x)
				_grenadeIndicator->setPixel(x, y, disc[x + (y * 9)]);
		_grenadeIndicator->unlock();
	}
	// The proximity marker is a two-frame "wifi" ping: a red core (same red as the grenade disc)
	// flanked by a ( . ) bracket-arc pair that jumps from near to far, so it reads as a wave
	// broadcasting outward. drawTerrain applies the disc's brightness pulse on top. Each lit pixel
	// gets a black halo (like the selection arrow) so the thin arcs stay readable on any terrain.
	// Grid cells: 0 = transparent, 1 = arc, 2 = core. Red ramp indices kept <= 43 so the +0..+4
	// pulse never spills out of the red block; the black halo (15) clamps back to black under pulse.
	if (!_proxyPing[0])
	{
		const int pingNear[121] = {
			0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,1,0,1,0,0,0,0,
			0,0,0,1,0,0,0,1,0,0,0,
			0,0,0,1,0,2,0,1,0,0,0,
			0,0,0,1,0,0,0,1,0,0,0,
			0,0,0,0,1,0,1,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0 };
		const int pingFar[121] = {
			0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,
			0,0,1,0,0,0,0,0,1,0,0,
			0,1,0,0,0,0,0,0,0,1,0,
			0,1,0,0,0,2,0,0,0,1,0,
			0,1,0,0,0,0,0,0,0,1,0,
			0,0,1,0,0,0,0,0,1,0,0,
			0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0 };
		const int* grids[PROXY_PING_FRAMES] = { pingNear, pingFar };
		const int arcColor[PROXY_PING_FRAMES] = { 34, 38 }; // near arc = disc's red fill; far arc a touch darker
		const int coreColor = 32; // brightest red (matches the grenade disc highlight)
		const int black = 15;
		for (int f = 0; f < PROXY_PING_FRAMES; ++f)
		{
			const int* grid = grids[f];
			Surface* frame = new Surface(11, 11);
			frame->setPalette(this->getPalette());
			frame->lock();
			for (int y = 0; y < 11; ++y)
			{
				for (int x = 0; x < 11; ++x)
				{
					const int cell = grid[x + (y * 11)];
					if (cell == 2)
					{
						frame->setPixel(x, y, coreColor);
					}
					else if (cell == 1)
					{
						frame->setPixel(x, y, arcColor[f]);
					}
					else
					{
						// transparent: paint black where it borders a lit pixel (8-neighbourhood)
						bool border = false;
						for (int dy = -1; dy <= 1 && !border; ++dy)
							for (int dx = -1; dx <= 1; ++dx)
							{
								const int nx = x + dx, ny = y + dy;
								if (nx >= 0 && nx < 11 && ny >= 0 && ny < 11 && grid[nx + (ny * 11)] != 0)
								{
									border = true;
									break;
								}
							}
						if (border)
							frame->setPixel(x, y, black);
					}
				}
			}
			frame->unlock();
			_proxyPing[f] = frame;
		}
	}

	// DX: procedural fallback status icons (built once, in the battlescape palette) shown over
	// unconscious bodies - and, for the wound icon, over conscious bleeding units - when a mod
	// supplies no Floor*Indicator art, so the status overlays work out of the box. Each glyph is a
	// small grid (0 = clear, 1..3 = colour-ramp slots) with an auto-painted black halo on every
	// transparent cell bordering a lit one (same readable-outline technique as the proxy ping).
	// Colours use only the red (block 2) and yellow (block 1) ramps + black, all palette-safe in
	// both the UFO and TFTD palettes (same blocks the grenade disc and selection arrow rely on);
	// the four are distinguished by shape, not colour.
	if (!_woundIndicatorFallback)
	{
		auto buildIcon = [this](const int* grid, int w, int h, const int* colors) -> Surface*
		{
			Surface* s = new Surface(w, h);
			s->setPalette(this->getPalette());
			s->lock();
			for (int y = 0; y < h; ++y)
			{
				for (int x = 0; x < w; ++x)
				{
					const int cell = grid[x + (y * w)];
					if (cell > 0)
					{
						s->setPixel(x, y, colors[cell]);
					}
					else
					{
						// transparent: paint black where it borders a lit cell (8-neighbourhood)
						bool border = false;
						for (int dy = -1; dy <= 1 && !border; ++dy)
							for (int dx = -1; dx <= 1; ++dx)
							{
								const int nx = x + dx, ny = y + dy;
								if (nx >= 0 && nx < w && ny >= 0 && ny < h && grid[nx + (ny * w)] > 0)
								{
									border = true;
									break;
								}
							}
						if (border)
							s->setPixel(x, y, 15); // black halo
					}
				}
			}
			s->unlock();
			return s;
		};

		// Wound: a red blood drop (pointed top, round bottom).
		const int woundColors[3] = { 0, 34, 32 }; // red fill / brighter red highlight
		const int wound[121] = {
			0,0,0,0,0,1,0,0,0,0,0,
			0,0,0,0,0,1,0,0,0,0,0,
			0,0,0,0,1,1,1,0,0,0,0,
			0,0,0,0,1,1,1,0,0,0,0,
			0,0,0,1,1,1,1,1,0,0,0,
			0,0,0,1,1,2,1,1,0,0,0,
			0,0,1,1,1,1,1,1,1,0,0,
			0,0,1,1,1,1,1,1,1,0,0,
			0,0,0,1,1,1,1,1,0,0,0,
			0,0,0,0,1,1,1,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0 };

		// Burn: a flame - red body, yellow inner, bright-yellow tip.
		const int burnColors[4] = { 0, 34, 18, 16 }; // red / yellow / bright yellow
		const int burn[121] = {
			0,0,0,0,0,0,3,0,0,0,0,
			0,0,0,0,0,3,3,0,0,0,0,
			0,0,0,0,0,2,3,0,0,0,0,
			0,0,0,0,2,2,2,0,0,0,0,
			0,0,0,1,2,2,3,0,0,0,0,
			0,0,0,1,2,3,2,1,0,0,0,
			0,0,1,1,2,2,2,1,0,0,0,
			0,0,1,1,1,2,1,1,1,0,0,
			0,0,1,1,1,1,1,1,1,0,0,
			0,0,0,1,1,1,1,1,0,0,0,
			0,0,0,0,1,1,1,0,0,0,0 };

		// Shock: a yellow lightning bolt.
		const int shockColors[2] = { 0, 16 }; // bright yellow
		const int shock[121] = {
			0,0,0,0,0,0,1,1,0,0,0,
			0,0,0,0,0,1,1,0,0,0,0,
			0,0,0,0,1,1,0,0,0,0,0,
			0,0,0,1,1,1,1,0,0,0,0,
			0,0,0,0,1,1,1,1,0,0,0,
			0,0,0,0,0,0,1,1,0,0,0,
			0,0,0,0,0,1,1,0,0,0,0,
			0,0,0,0,1,1,0,0,0,0,0,
			0,0,0,1,1,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0 };

		// Stun: a "Zzz" sleep glyph (asleep / about-to-drop cue). The lit cells are transcribed
		// pixel-for-pixel from reference/Zzz.png (its bright pixels -> lit; its own outline/colours
		// discarded), then buildIcon paints the fill + the shared black halo. 14x14. Each Z is a
		// step darker down the greyscale ramp than the next-bigger one (1 = big/white, 2 = medium,
		// 3 = small), so the trio reads as fading away as it drifts up.
		const int stunColors[4] = { 0, 1, 3, 5 }; // white -> light grey -> mid grey (greyscale ramp)
		const int stun[14 * 14] = {
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,1,1,1,1,1,1,0,0,0,0,0,0,0,
			0,0,0,0,0,0,1,0,0,0,0,0,0,0,
			0,0,0,0,0,1,0,0,0,0,0,0,0,0,
			0,0,0,0,1,0,2,2,2,2,0,0,0,0,
			0,0,0,1,0,0,0,0,0,2,0,0,0,0,
			0,0,1,0,0,0,0,0,2,0,0,0,0,0,
			0,1,0,0,0,0,0,2,0,0,3,3,3,0,
			0,1,1,1,1,1,1,0,0,0,0,0,3,0,
			0,0,0,0,0,0,0,0,0,0,0,3,0,0,
			0,0,0,0,0,0,2,2,2,2,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,3,3,3,3,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

		// Bleedout: a bold red medical cross - "this soldier is dying, get a medic here". Distinct in
		// shape from the wound blood-drop so a bleeding-out unit reads differently from a merely wounded one.
		const int bleedoutColors[3] = { 0, 34, 32 }; // red fill / brighter red centre
		// Inset 1px on every side so buildIcon's black halo has transparent cells to paint into (the arms
		// must not touch the grid edge, or the outline can't be drawn on that side).
		const int bleedout[121] = {
			0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,1,1,1,0,0,0,0,
			0,0,0,0,1,1,1,0,0,0,0,
			0,0,0,0,1,1,1,0,0,0,0,
			0,1,1,1,1,1,1,1,1,1,0,
			0,1,1,1,1,2,1,1,1,1,0,
			0,1,1,1,1,1,1,1,1,1,0,
			0,0,0,0,1,1,1,0,0,0,0,
			0,0,0,0,1,1,1,0,0,0,0,
			0,0,0,0,1,1,1,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0 };

		// DX channeled mind control. Two glyphs so the two ends of a link read differently at a glance:
		// the CONTROLLER gets a radiating "grip" (a dot with waves coming off it - he is projecting), the
		// THRALL gets a ring closed around a dot (he is the one being held).
		const int psiColors[3] = { 0, 208, 210 }; // purple ramp, palette-safe in both games
		const int channeling[121] = {
			0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,1,0,0,0,1,0,0,0,
			0,0,1,0,1,0,1,0,1,0,0,
			0,0,0,0,0,1,0,0,0,0,0,
			0,0,1,0,1,2,1,0,1,0,0,
			0,0,0,1,2,2,2,1,0,0,0,
			0,0,1,0,1,2,1,0,1,0,0,
			0,0,0,0,0,1,0,0,0,0,0,
			0,0,1,0,1,0,1,0,1,0,0,
			0,0,0,1,0,0,0,1,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0 };
		const int enthralled[121] = {
			0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,1,1,1,1,1,0,0,0,
			0,0,1,0,0,0,0,0,1,0,0,
			0,1,0,0,0,0,0,0,0,1,0,
			0,1,0,0,2,2,2,0,0,1,0,
			0,1,0,0,2,2,2,0,0,1,0,
			0,1,0,0,2,2,2,0,0,1,0,
			0,1,0,0,0,0,0,0,0,1,0,
			0,0,1,0,0,0,0,0,1,0,0,
			0,0,0,1,1,1,1,1,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0 };

		_channelingIndicatorFallback = buildIcon(channeling, 11, 11, psiColors);
		_enthralledIndicatorFallback = buildIcon(enthralled, 11, 11, psiColors);

		_woundIndicatorFallback = buildIcon(wound, 11, 11, woundColors);
		_burnIndicatorFallback = buildIcon(burn, 11, 11, burnColors);
		_shockIndicatorFallback = buildIcon(shock, 11, 11, shockColors);
		_stunIndicatorFallback = buildIcon(stun, 14, 14, stunColors);
		_bleedoutIndicatorFallback = buildIcon(bleedout, 11, 11, bleedoutColors);
	}

	for (Projectile* p : _projectiles) delete p;
	_projectiles.clear();
	if (_save->getDepth() == 0)
	{
		_projectileSet = _game->getMod()->getSurfaceSet("Projectiles");
	}
	else
	{
		_projectileSet = _game->getMod()->getSurfaceSet("UnderwaterProjectiles");
	}
}

/**
 * Keeps the animation timers running.
 */
void Map::think()
{
	_scrollMouseTimer->think(0, this);
	_scrollKeyTimer->think(0, this);
	_fadeTimer->think(0, this);
	_obstacleTimer->think(0, this);
}

/**
 * Draws the whole map, part by part.
 */
void Map::draw()
{
	if (!_redraw)
	{
		return;
	}

	// normally we'd call for a Surface::draw();
	// but we don't want to clear the background with colour 0, which is transparent (aka black)
	// we use colour 15 because that actually corresponds to the colour we DO want in all variations of the xcom and tftd palettes.
	// Note: un-hardcoded the color from 15 to ruleset value, default 15
	_redraw = false;

	// DX: keep the live aiming trajectory preview in sync with the current cursor/action.
	updateTargetingPreview();

	ShaderDrawFunc(
		[](Uint8& dest, Uint8 color)
		{
			dest = color;
		},
		ShaderSurface(this),
		ShaderScalar<Uint8>(Palette::blockOffset(0) + _bgColor)
	);

	Tile *t;

	_projectileInFOV = _save->getDebugMode();
	for (Projectile* proj : _projectiles)
	{
		t = _save->getTile(proj->getPosition(0).toTile());
		if (_save->getSide() == FACTION_PLAYER || (t && t->getVisible()))
		{
			_projectileInFOV = true;
			break;
		}
	}
	_explosionInFOV = _save->getDebugMode();
	if (!_explosions.empty())
	{
		for (auto* explosion : _explosions)
		{
			if (explosion->isBig())
			{
				_explosionInFOV = true;
				break;
			}
			t = _save->getTile(explosion->getPosition().toTile());
			if (t && t->getVisible())
			{
				_explosionInFOV = true;
				break;
			}
		}
	}

	if ((_save->getSelectedUnit() && _save->getSelectedUnit()->getVisible()) || _unitDying || _save->getSide() == FACTION_PLAYER || _save->getDebugMode() || _projectileInFOV || _explosionInFOV)
	{
		drawTerrain(this);
	}
	else
	{
		_message->blit(this->getSurface());
	}
}

void Map::refreshAIProgress(int progress)
{
	if (_save->getSide() == FACTION_NEUTRAL)
	{
		_message->setProgressBarColor(_neutralBarColor, _borderBarColor);
	}
	else
	{
		_message->setProgressBarColor(_hostileBarColor, _borderBarColor);
	}
	_message->setProgressValue(progress);
}

/**
 * Replaces a certain amount of colors in the surface's palette.
 * @param colors Pointer to the set of colors.
 * @param firstcolor Offset of the first color to replace.
 * @param ncolors Amount of colors to replace.
 */
void Map::setPalette(const SDL_Color *colors, int firstcolor, int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	for (auto* mds : *_save->getMapDataSets())
	{
		mds->getSurfaceset()->setPalette(colors, firstcolor, ncolors);
	}
	_message->setPalette(colors, firstcolor, ncolors);
	refreshHiddenMovementBackground();
	_message->initText(_game->getMod()->getFont("FONT_BIG"), _game->getMod()->getFont("FONT_SMALL"), _game->getLanguage());
	_message->setText(_game->getLanguage()->getString("STR_HIDDEN_MOVEMENT"), _game->getLanguage()->getString("STR_THINKING"));
}

void Map::refreshHiddenMovementBackground()
{
	_message->setBackground(_game->getMod()->getSurface(_save->getHiddenMovementBackground()));
}

/**
 * Get shade of wall.
 * @param part For what wall do calculations.
 * @param tileFrot Tile of wall.
 * @return Current shade of wall.
 */
int Map::getWallShade(TilePart part, Tile* tileFrot)
{
	int shade;
	if (tileFrot->isDiscovered(O_FLOOR))
	{
		shade = reShade(tileFrot);
	}
	else
	{
		shade = 16;
	}
	if (part)
	{
		if ((tileFrot->isDoor(part) || tileFrot->isUfoDoor(part)) && tileFrot->isDiscovered(part))
		{
			Position offset =
				part == O_NORTHWALL ? Position(1,0,0) :
				part == O_WESTWALL ? Position(0,1,0) :
					throw Exception("Unsupported tile part for wall shade");

			Tile *tileBehind = _save->getTile(tileFrot->getPosition() - offset);

			shade = std::min(reShade(tileFrot), tileBehind ? tileBehind->getShade() + 5 : 16);
		}
	}
	return shade;
}

/**
 * Check two positions if have same XY cords
 */
static bool positionHaveSameXY(Position a, Position b)
{
	return a.x == b.x && a.y == b.y;
}

/**
 * Check two positions if have same XY cords
 */
static bool positionInRangeXY(Position a, Position b, int diff)
{
	return std::abs(a.x - b.x) <= diff && std::abs(a.y - b.y) <= diff;
}

namespace
{

static const int ArrowBobOffsets[8] = {0,1,2,1,0,1,2,1};

static const int ArrowColorsUFO[4]  = { 6,  3, 14, 4 }; // white,    red, blue, green
static const int ArrowColorsTFTD[4] = { 4, 11, 16, 6 }; // white, orange, blue, green

int getArrowBobForFrame(int frame)
{
	return ArrowBobOffsets[frame % 8];
}

int getShadePulseForFrame(int shade, int frame)
{
	if (shade > 7) shade = 7;
	if (shade < 2) shade = 2;
	shade += (ArrowBobOffsets[frame % 8] * 2 - 2);
	return shade;
}

}

/**
 * Draw part of unit graphic that overlap current tile.
 * @param surface
 * @param unitTile
 * @param currTile
 * @param currTileScreenPosition
 * @param shade
 * @param obstacleShade
 * @param topLayer
 */
void Map::drawUnit(UnitSprite &unitSprite, Tile *unitTile, Tile *currTile, Position currTileScreenPosition, bool topLayer, BattleUnit* movingUnit)
{
	const int tileFoorWidth = 32;
	const int tileFoorHeight = 16;
	const int tileHeight = 40;

	if (!unitTile)
	{
		return;
	}
	BattleUnit* bu = unitTile->getOverlappingUnit(_save, TUO_ALWAYS);
	Position unitOffset;
	bool unitFromBelow = false;
	bool unitFromAbove = false;
	if (bu)
	{
		if (bu != unitTile->getUnit())
		{
			unitFromBelow = true;
		}
	}
	else if (movingUnit && unitTile == currTile)
	{
		auto* upperTile = _save->getAboveTile(unitTile);
		if (upperTile && upperTile->hasNoFloor(_save))
		{
			bu = upperTile->getUnit();
		}
		if (bu != movingUnit)
		{
			return;
		}
		unitFromAbove = true;
	}
	else
	{
		return;
	}

	if (!(bu->getVisible() || _save->getDebugMode()))
	{
		return;
	}

	unitOffset.x = unitTile->getPosition().x - bu->getPosition().x;
	unitOffset.y = unitTile->getPosition().y - bu->getPosition().y;
	int part = unitOffset.x + unitOffset.y*2;

	bool moving = bu->getStatus() == STATUS_WALKING || bu->getStatus() == STATUS_FLYING;
	int bonusWidth = moving ? 0 : tileFoorWidth;
	int topMargin = 0;
	int bottomMargin = 0;

	//if unit is from below then we draw only part that in in tile
	if (unitFromBelow)
	{
		bottomMargin = -tileFoorHeight / 2;
		topMargin = tileFoorHeight;
	}
	else if (topLayer)
	{
		topMargin = 2 * tileFoorHeight;
	}
	else
	{
		const Tile *top = _save->getAboveTile(unitTile);
		if (top && top->getOverlappingUnit(_save, TUO_ALWAYS) == bu)
		{
			topMargin = -tileFoorHeight / 2;
		}
		else
		{
			topMargin = tileFoorHeight;
		}
	}

	GraphSubset mask = GraphSubset(tileFoorWidth + bonusWidth, tileHeight + topMargin + bottomMargin).offset(currTileScreenPosition.x - bonusWidth / 2, currTileScreenPosition.y - topMargin);

	if (moving)
	{
		GraphSubset leftMask = mask.offset(-tileFoorWidth/2, 0);
		GraphSubset rightMask = mask.offset(+tileFoorWidth/2, 0);
		int direction = bu->getDirection();
		Position partCurr = currTile->getPosition();
		Position partDest = bu->getDestination() + unitOffset;
		Position partLast = bu->getLastPosition() + unitOffset;
		bool isTileDestPos = positionHaveSameXY(partDest, partCurr);
		bool isTileLastPos = positionHaveSameXY(partLast, partCurr);

		if (unitFromAbove && partLast != unitTile->getPosition())
		{
			//this tile is below moving unit and it do not change levels, nothing to draw
			return;
		}

		//adjusting mask
		if (positionHaveSameXY(partLast, partDest))
		{
			if (currTile == unitTile)
			{
				//no change
			}
			else
			{
				//nothing to draw
				return;
			}
		}
		else if (isTileDestPos)
		{
			//unit is moving to this tile
			switch (direction)
			{
			case 0:
			case 1:
				mask = GraphSubset::intersection(mask, rightMask);
				break;
			case 2:
				//no change
				break;
			case 3:
				//no change
				break;
			case 4:
				//no change
				break;
			case 5:
			case 6:
				mask = GraphSubset::intersection(mask, leftMask);
				break;
			case 7:
				//nothing to draw
				return;
			}
		}
		else if (isTileLastPos)
		{
			//unit is exiting this tile
			switch (direction)
			{
			case 0:
				//no change
				break;
			case 1:
			case 2:
				mask = GraphSubset::intersection(mask, leftMask);
				break;
			case 3:
				//nothing to draw
				return;
			case 4:
			case 5:
				mask = GraphSubset::intersection(mask, rightMask);
				break;
			case 6:
				//no change
				break;
			case 7:
				//no change
				break;
			}
		}
		else
		{
			Position leftPos = partCurr + Position(-1, 0, 0);
			Position rightPos = partCurr + Position(0, -1, 0);
			if (!topLayer && (partDest.z > partCurr.z || partLast.z > partCurr.z))
			{
				//unit change layers, it will be drawn by upper layer not lower.
				return;
			}
			else if (
				(direction == 1 && (partDest == rightPos || partLast == leftPos)) ||
				(direction == 5 && (partDest == leftPos || partLast == rightPos)))
			{
				mask = GraphSubset(tileFoorWidth, tileHeight + 2 * tileFoorHeight).offset(currTileScreenPosition.x, currTileScreenPosition.y - 2 * tileFoorHeight);
			}
			else
			{
				//unit is not moving close to tile
				return;
			}
		}
	}
	else if (unitTile != currTile || unitFromAbove)
	{
		return;
	}

	Position tileScreenPosition;
	_camera->convertMapToScreen(unitTile->getPosition() + Position(0, 0, static_cast<int>(unitFromAbove) - static_cast<int>(unitFromBelow)), &tileScreenPosition);
	tileScreenPosition += _camera->getMapOffset();

	//get shade helpers
	auto getTileShade = [&](Tile* tile)
	{
		return tile ? (tile->isDiscovered(O_FLOOR) ? reShade(tile) : 16) : 16;
	};
	auto getMixedTileShade = [&](Tile* tile, int heightOffset, bool below)
	{
		int shadeLower = 0;
		int shadeUpper = 0;
		if (below)
		{
			shadeLower = getTileShade(_save->getBelowTile(tile));
			shadeUpper = getTileShade(tile);
		}
		else
		{
			shadeLower = getTileShade(tile);
			shadeUpper = getTileShade(_save->getAboveTile(tile));
		}

		return Interpolate(shadeLower, shadeUpper, -heightOffset, Position::TileZ);
	};

	// draw unit
	int shade = 0;
	UnitWalkingOffset offsets = calculateWalkingOffset(bu);
	if (moving)
	{
		const Position start = bu->getPosition();
		const Position end = bu->getDestination();
		const auto minLevel = std::min(start.z, end.z); // Sint16
		const int startShade = getMixedTileShade(_save->getTile(start), start.z == minLevel ? offsets.TerrainLevelOffset : 0, false);
		const int endShade = getMixedTileShade(_save->getTile(end), end.z == minLevel ? offsets.TerrainLevelOffset : 0, false);
		shade = Interpolate(startShade, endShade, offsets.NormalizedMovePhase, 16);
	}
	else
	{
		shade = getMixedTileShade(currTile, offsets.TerrainLevelOffset, unitFromBelow);
		if (_showObstacles && unitTile->getObstacle(4))
		{
			shade = getShadePulseForFrame(shade, _animFrame);
		}
	}
	if (_debugVisionMode == 1)
	{
		shade = std::min(+NIGHT_VISION_SHADE, shade);
	}
	unitSprite.draw(bu, part, tileScreenPosition.x + offsets.ScreenOffset.x, tileScreenPosition.y + offsets.ScreenOffset.y, shade, mask, _isAltPressed && !_isCtrlPressed);
}

/**
 * Draw the terrain.
 * Keep this function as optimised as possible. It's big to minimise overhead of function calls.
 * @param surface The surface to draw on.
 */
void Map::drawTerrain(Surface *surface)
{
	_isAltPressed = _game->isAltPressed(true);
	_isCtrlPressed = _game->isCtrlPressed(true);
	_cursorAccuracyShown = false; // set when the crosshair accuracy readout is drawn (below)
	_pendingAccuracyText = _pendingUnitName = false; // deferred crosshair text (blitted after tracers)
	int frameNumber = 0;
	SurfaceRaw<const Uint8> tmpSurface;
	Tile *tile;
	int beginX = 0, endX = _save->getMapSizeX() - 1;
	int beginY = 0, endY = _save->getMapSizeY() - 1;
	int beginZ = 0, endZ = _save->getMapSizeZ() - 1;
	Position mapPosition, screenPosition, bulletPositionScreen, movingUnitPosition;
	int bulletLowX=16000, bulletLowY=16000, bulletLowZ=16000, bulletHighX=0, bulletHighY=0, bulletHighZ=0;
	int dummy;
	BattleUnit *movingUnit = _save->getTileEngine()->getMovingUnit();
	int tileShade, tileColor, obstacleShade;
	UnitSprite unitSprite(surface, _game->getMod(), _save, _animFrame, _save->getDepth() != 0,
		_isTFTD ? ArrowColorsTFTD[1] : ArrowColorsUFO[1], _isTFTD ? ArrowColorsTFTD[2] : ArrowColorsUFO[2]);
	ItemSprite itemSprite(surface, _game->getMod(), _save, _animFrame);

	const int halfAnimFrame = (_animFrame / 2) % 4;
	const int halfAnimFrameRest = (_animFrame % 2);

	NumberText *_numWaypid = 0;

	// if we got bullets, get the highest x and y tiles to draw them on.
	// NOTE: this runs even while an explosion is resolving - otherwise the bullet
	// bounding box below would stay at its default and any still-airborne projectiles
	// would stop being drawn until the explosion finished.
	if (!_projectiles.empty())
	{
		Position avgProjectileVoxel(0, 0, 0);
		for (Projectile* proj : _projectiles)
		{
			Position pos = proj->getPosition();
			avgProjectileVoxel.x += pos.x;
			avgProjectileVoxel.y += pos.y;
			avgProjectileVoxel.z += pos.z;
			int part = proj->getItem() ? 0 : BULLET_SPRITES-1;
			for (int i = 0; i <= part; ++i)
			{
				if (proj->getPosition(1-i).x < bulletLowX)
					bulletLowX = proj->getPosition(1-i).x;
				if (proj->getPosition(1-i).y < bulletLowY)
					bulletLowY = proj->getPosition(1-i).y;
				if (proj->getPosition(1-i).z < bulletLowZ)
					bulletLowZ = proj->getPosition(1-i).z;
				if (proj->getPosition(1-i).x > bulletHighX)
					bulletHighX = proj->getPosition(1-i).x;
				if (proj->getPosition(1-i).y > bulletHighY)
					bulletHighY = proj->getPosition(1-i).y;
				if (proj->getPosition(1-i).z > bulletHighZ)
					bulletHighZ = proj->getPosition(1-i).z;
			}
		}
		int projCount = (int)_projectiles.size();
		avgProjectileVoxel.x /= projCount;
		avgProjectileVoxel.y /= projCount;
		avgProjectileVoxel.z /= projCount;
		// divide by 16 to go from voxel to tile position
		bulletLowX = bulletLowX / 16;
		bulletLowY = bulletLowY / 16;
		bulletLowZ = bulletLowZ / 24;
		bulletHighX = bulletHighX / 16;
		bulletHighY = bulletHighY / 16;
		bulletHighZ = bulletHighZ / 24;

		// if the projectile is outside the viewport - center it back on it
		_camera->convertVoxelToScreen(avgProjectileVoxel, &bulletPositionScreen);

		// Only actively chase the bullets when nothing is exploding and nobody is dying on camera,
		// so the camera doesn't fight the explosion's or the death's own framing.
		if (_explosions.empty() && !_deathFocus && _projectileInFOV && _followProjectile)
		{
			Position newCam = _camera->getMapOffset();
			if (newCam.z != bulletHighZ) //switch level
			{
				newCam.z = bulletHighZ;
				if (_projectileInFOV)
				{
					_camera->setMapOffset(newCam);
					_camera->convertVoxelToScreen(avgProjectileVoxel, &bulletPositionScreen);
				}
			}
			if (_smoothCamera)
			{
				if (_launch)
				{
					_launch = false;
					if ((bulletPositionScreen.x < 1 || bulletPositionScreen.x > surface->getWidth() - 1 ||
						bulletPositionScreen.y < 1 || bulletPositionScreen.y > _visibleMapHeight - 1))
					{
						_camera->centerOnPosition(Position(avgProjectileVoxel.x / 16, avgProjectileVoxel.y / 16, bulletHighZ), false);
						_camera->convertVoxelToScreen(avgProjectileVoxel, &bulletPositionScreen);
					}
				}
				if (!_smoothingEngaged)
				{
					if (bulletPositionScreen.x < 1 || bulletPositionScreen.x > surface->getWidth() - 1 ||
						bulletPositionScreen.y < 1 || bulletPositionScreen.y > _visibleMapHeight - 1)
					{
						_smoothingEngaged = true;
					}
				}
				else
				{
					_camera->jumpXY(surface->getWidth() / 2 - bulletPositionScreen.x, _visibleMapHeight / 2 - bulletPositionScreen.y);
				}
			}
			else
			{
				bool enough;
				do
				{
					enough = true;
					if (bulletPositionScreen.x < 0)
					{
						_camera->jumpXY(+surface->getWidth(), 0);
						enough = false;
					}
					else if (bulletPositionScreen.x > surface->getWidth())
					{
						_camera->jumpXY(-surface->getWidth(), 0);
						enough = false;
					}
					else if (bulletPositionScreen.y < 0)
					{
						_camera->jumpXY(0, +_visibleMapHeight);
						enough = false;
					}
					else if (bulletPositionScreen.y > _visibleMapHeight)
					{
						_camera->jumpXY(0, -_visibleMapHeight);
						enough = false;
					}
					_camera->convertVoxelToScreen(avgProjectileVoxel, &bulletPositionScreen);
				}
				while (!enough);
			}
		}
	}

	// get corner map coordinates to give rough boundaries in which tiles to redraw are
	_camera->convertScreenToMap(0, 0, &beginX, &dummy);
	_camera->convertScreenToMap(surface->getWidth(), 0, &dummy, &beginY);
	_camera->convertScreenToMap(surface->getWidth() + _spriteWidth, surface->getHeight() + _spriteHeight, &endX, &dummy);
	_camera->convertScreenToMap(0, surface->getHeight() + _spriteHeight, &dummy, &endY);
	beginY -= (_camera->getViewLevel() * 2);
	beginX -= (_camera->getViewLevel() * 2);
	if (beginX < 0)
		beginX = 0;
	if (beginY < 0)
		beginY = 0;

	if (!_camera->getShowAllLayers())
	{
		endZ = std::min(endZ, _camera->getViewLevel());
	}
	if (_camera->getShowSingleLayer())
	{
		beginZ = _camera->getViewLevel();
		endZ = _camera->getViewLevel();
	}


	bool pathfinderTurnedOn = _save->getPathfinding()->isPathPreviewed();

	if (!_waypoints.empty() || (pathfinderTurnedOn && (_previewSettingTu || _previewSettingEnergy)))
	{
		_numWaypid = new NumberText(15, 15, 20, 30);
		_numWaypid->setPalette(getPalette());
		_numWaypid->setColor(pathfinderTurnedOn ? _messageColor + 1 : Palette::blockOffset(1));
	}

	if (movingUnit)
	{
		movingUnitPosition = movingUnit->getPosition();
	}

	surface->lock();
	const Position cameraPos = _camera->getMapOffset();
	for (int itZ = beginZ; itZ <= endZ; itZ++)
	{
		bool topLayer = itZ == endZ;
		for (int itY = beginY; itY < endY; itY++)
		{
			mapPosition = Position(beginX, itY, itZ);
			tile = _save->getTile(mapPosition);
			for (int itX = beginX; itX < endX; itX++, mapPosition.x++, tile++)
			{
				_camera->convertMapToScreen(mapPosition, &screenPosition);
				screenPosition += cameraPos;

				// only render cells that are inside the surface
				if (screenPosition.x > -_spriteWidth && screenPosition.x < surface->getWidth() + _spriteWidth &&
					screenPosition.y > -_spriteHeight && screenPosition.y < surface->getHeight() + _spriteHeight )
				{
					bool isUnitMovingNearby = movingUnit && positionInRangeXY(movingUnitPosition, mapPosition, 2);

					if (tile->isDiscovered(O_FLOOR))
					{
						tileShade = reShade(tile);
						// DX fog-of-war: dim a discovered tile that no player unit currently sees, so
						// remembered terrain reads as out of live sight. Tile::getVisible() is the
						// player-only LOS count maintained by TileEngine::calculateTilesInFOV. Maps
						// shade [0..16] -> [4..16] (bright tiles dim a little, dark tiles stay dark);
						// layers on top of the light/night-vision shade reShade already returned.
						if (Options::fogOfWarEnabled && tile->getVisible() == 0)
							tileShade = (3 * tileShade) / 4 + 4;
						obstacleShade = tileShade;
						if (_showObstacles)
						{
							if (tile->isObstacle())
							{
								obstacleShade = getShadePulseForFrame(tileShade, _animFrame);
							}
						}
					}
					else
					{
						tileShade = 16;
						obstacleShade = 16;
					}

					tileColor = tile->getMarkerColor();

					// Draw floor
					tmpSurface = tile->getSprite(O_FLOOR);
					if (tmpSurface)
					{
						if (tile->getObstacle(O_FLOOR))
							Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y - tile->getYOffset(O_FLOOR), obstacleShade, false, _nvColor);
						else
							Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y - tile->getYOffset(O_FLOOR), tileShade, false, _nvColor);
					}

					auto* unit = tile->getUnit();

					// DX on-map overlay: motion-detector reading, ground-decal layer. Drawn before
					// this tile's walls/unit so it reads as ground; drawMotionMarkersOverlay()
					// re-draws the dithered variant on top of the finished scene so the reading
					// stays visible through terrain. Shared checks/pulse in isMotionMarkerActive/
					// blitMotionMarker. Detection gating is unchanged - only already-scanned units.
					if (isMotionMarkerActive(unit)
						&& unit->getPosition() == tile->getPosition()) // anchor tile only (big units)
					{
						SurfaceSet* markerSet = _game->getMod()->getSurfaceSet("Pathfinding2", false);
						Surface* motionMarker = markerSet ? markerSet->getFrame(Pathfinding2Markers::solidFrame(Pathfinding2Markers::Target)) : nullptr;
						if (motionMarker)
						{
							blitMotionMarker(surface, unit, motionMarker,
								screenPosition.x, screenPosition.y + tile->getTerrainLevel());
						}
					}

					// Draw cursor back
					if (_cursorType != CT_NONE && _selectorX > itX - _cursorSize && _selectorY > itY - _cursorSize && _selectorX < itX+1 && _selectorY < itY+1 && !_save->getBattleState()->getMouseOverIcons())
					{
						if (_camera->getViewLevel() == itZ)
						{
							if (_cursorType != CT_AIM)
							{
								if (unit && (unit->getVisible() || _save->getDebugMode()))
									frameNumber = halfAnimFrameRest; // yellow box
								else
									frameNumber = 0; // red box
							}
							else
							{
								if (unit && (unit->getVisible() || _save->getDebugMode()))
									frameNumber = 7 + halfAnimFrame; // yellow animated crosshairs
								else
									frameNumber = 6; // red static crosshairs
							}
							tmpSurface = _game->getMod()->getSurfaceSet("CURSOR.PCK")->getFrame(frameNumber);
							Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y, 0);
						}
						else if (_camera->getViewLevel() > itZ)
						{
							frameNumber = 2; // blue box
							tmpSurface = _game->getMod()->getSurfaceSet("CURSOR.PCK")->getFrame(frameNumber);
							Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y, 0);
						}
					}

					if (isUnitMovingNearby)
					{
						// special handling for a moving unit in background of tile.
						constexpr static Position backPos[] =
						{
							Position(0, -1, 0),
							Position(-1, -1, 0),
							Position(-1, 0, 0),
						};

						for (size_t b = 0; b < std::size(backPos); ++b)
						{
							drawUnit(unitSprite, _save->getTile(mapPosition + backPos[b]), tile, screenPosition, topLayer);
						}
					}

					// Draw walls
					{
						// Draw west wall
						tmpSurface = tile->getSprite(O_WESTWALL);
						if (tmpSurface)
						{
							int wallShade = getWallShade(O_WESTWALL, tile);
							if (tile->getObstacle(O_WESTWALL))
								Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y - tile->getYOffset(O_WESTWALL), obstacleShade, false, _nvColor);
							else
								Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y - tile->getYOffset(O_WESTWALL), wallShade, false, _nvColor);
						}
						// Draw north wall
						tmpSurface = tile->getSprite(O_NORTHWALL);
						if (tmpSurface)
						{
							int wallShade = getWallShade(O_NORTHWALL, tile);
							if (tile->getObstacle(O_NORTHWALL))
								Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y - tile->getYOffset(O_NORTHWALL), obstacleShade, bool(tile->getSprite(O_WESTWALL)), _nvColor);
							else
								Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y - tile->getYOffset(O_NORTHWALL), wallShade, bool(tile->getSprite(O_WESTWALL)), _nvColor);
						}
						// Draw object
						tmpSurface = tile->getSprite(O_OBJECT);
						if (tmpSurface)
						{
							if (tile->isBackTileObject(O_OBJECT))
							{
								if (tile->getObstacle(O_OBJECT))
									Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y - tile->getYOffset(O_OBJECT), obstacleShade, false, _nvColor);
								else
									Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y - tile->getYOffset(O_OBJECT), tileShade, false, _nvColor);
							}
						}
						// draw an item on top of the floor (if any)
						BattleItem* item = tile->getTopItem();
						if (item)
						{
							itemSprite.draw(item,
								screenPosition.x,
								screenPosition.y + tile->getTerrainLevel(),
								tileShade
							);
							{
								BattleUnit *itemUnit = item->getUnit();
								if (itemUnit && itemUnit->getStatus() == STATUS_UNCONSCIOUS && itemUnit->indicatorsAreEnabled())
								{
									// Pick the highest-priority status glyph, preferring mod-supplied
									// Floor*Indicator art and falling back to the DX procedural icon so
									// the overlay always shows (burn > wound > shock > stun).
									Surface *modArt = nullptr, *fallback = nullptr;
									if (itemUnit->getBleedingOut() || (itemUnit->isLockedOutForMission() && itemUnit->getFatalWounds() > 0))
									{
										// DX: the bleedout cross flags "still needs aid" - an actively-dying soldier,
										// or one who bled out and is down for the mission and STILL bleeding (has fatal
										// wounds, so it keeps losing HP even if a partial heal put it back above 0).
										// Once the bleeding is fully stopped (wounds cured) it drops to the ordinary
										// knocked-out glyph, so the cross never lingers on a stabilized unit.
										modArt = _bleedoutIndicator; fallback = _bleedoutIndicatorFallback;
									}
									else if (itemUnit->getFire() > 0)
									{
										modArt = _burnIndicator; fallback = _burnIndicatorFallback;
									}
									else if (itemUnit->getFatalWounds() > 0)
									{
										modArt = _woundIndicator; fallback = _woundIndicatorFallback;
									}
									else if (itemUnit->hasNegativeHealthRegen())
									{
										modArt = _shockIndicator; fallback = _shockIndicatorFallback;
									}
									else
									{
										modArt = _stunIndicator; fallback = _stunIndicatorFallback;
									}
									if (modArt)
									{
										// Mod art is tile-aligned: blit at the tile origin as before.
										modArt->blitNShade(surface,
											screenPosition.x,
											screenPosition.y + tile->getTerrainLevel(),
											tileShade);
									}
									else if (fallback)
									{
										// The small procedural icon is centered over the tile floor.
										fallback->blitNShade(surface,
											screenPosition.x + (_spriteWidth - fallback->getWidth()) / 2,
											screenPosition.y + tile->getTerrainLevel() + 6,
											tileShade);
									}
								}
							}
						}
					}

					// DX on-map overlay: a hovering marker over each player-thrown primed grenade
					// lying on a discovered tile (red disc for a normal grenade, cyan ring for a
					// proximity grenade). Enemy grenades are never marked - only the player's own.
					if (Options::grenadeIndicatorEnabled && tile->isDiscovered(O_FLOOR))
					{
						for (BattleItem* groundItem : *tile->getInventory())
						{
							if (groundItem->getFuseTimer() < 0 || !groundItem->getRules()->isGrenadeOrProxy())
								continue;
							const BattleUnit* thrower = groundItem->getPreviousOwner();
							if (!thrower || thrower->getFaction() != FACTION_PLAYER)
								continue;
							// A proximity grenade gets the animated wifi ping (its frame cycles the
							// broadcasting arc); a normal grenade gets the static red disc. Both then
							// pulse in brightness off the same Pulsate shade.
							const int Pulsate[8] = { 0, 1, 2, 3, 4, 3, 2, 1 };
							Surface* marker = (groundItem->getRules()->getBattleType() == BT_PROXIMITYGRENADE)
								? _proxyPing[(_animFrame / 2) % PROXY_PING_FRAMES]
								: _grenadeIndicator;
							marker->blitNShade(surface,
								screenPosition.x + (_spriteWidth / 2) - (marker->getWidth() / 2),
								screenPosition.y + tile->getTerrainLevel() - marker->getHeight(),
								Pulsate[_animFrame % 8]);
							break; // one marker per tile is enough
						}
					}

					// check if we got bullet && it is in Field Of View
					if (_projectileInFOV)
					{
						for (Projectile* proj : _projectiles)
						{
						tmpSurface = nullptr;
						BattleItem* item = proj->getItem();
						if (item)
						{
							Position voxelPos = proj->getPosition();
							// draw shadow on the floor
							voxelPos.z = _save->getTileEngine()->castedShade(voxelPos);
							if (voxelPos.x / 16 >= itX &&
								voxelPos.y / 16 >= itY &&
								voxelPos.x / 16 <= itX+1 &&
								voxelPos.y / 16 <= itY+1 &&
								voxelPos.z / 24 == itZ &&
								_save->getTileEngine()->isVoxelVisible(voxelPos))
							{
								_camera->convertVoxelToScreen(voxelPos, &bulletPositionScreen);

								itemSprite.drawShadow(item,
									bulletPositionScreen.x - 16,
									bulletPositionScreen.y - 26
								);
							}

							voxelPos = proj->getPosition();
							// draw thrown object
							if (voxelPos.x / 16 >= itX &&
								voxelPos.y / 16 >= itY &&
								voxelPos.x / 16 <= itX+1 &&
								voxelPos.y / 16 <= itY+1 &&
								voxelPos.z / 24 == itZ &&
								_save->getTileEngine()->isVoxelVisible(voxelPos))
							{
								_camera->convertVoxelToScreen(voxelPos, &bulletPositionScreen);

								itemSprite.draw(item,
									bulletPositionScreen.x - 16,
									bulletPositionScreen.y - 26,
									tileShade
								);
							}
						}
						else
						{
							// draw bullet on the correct tile
							if (itX >= bulletLowX && itX <= bulletHighX && itY >= bulletLowY && itY <= bulletHighY)
							{
								int begin = 0;
								int end = BULLET_SPRITES;
								int direction = 1;
								if (proj->isReversed())
								{
									begin = BULLET_SPRITES - 1;
									end = -1;
									direction = -1;
								}

								for (int i = begin; i != end; i += direction)
								{
									tmpSurface = _projectileSet->getFrame(proj->getParticle(i));
									if (tmpSurface)
									{
										Position voxelPos = proj->getPosition(1-i);
										// draw shadow on the floor
										voxelPos.z = _save->getTileEngine()->castedShade(voxelPos);
										if (voxelPos.x / 16 == itX &&
											voxelPos.y / 16 == itY &&
											voxelPos.z / 24 == itZ &&
											_save->getTileEngine()->isVoxelVisible(voxelPos))
										{
											_camera->convertVoxelToScreen(voxelPos, &bulletPositionScreen);
											bulletPositionScreen.x -= tmpSurface.getWidth() / 2;
											bulletPositionScreen.y -= tmpSurface.getHeight() / 2;
											Surface::blitRaw(surface, tmpSurface, bulletPositionScreen.x, bulletPositionScreen.y, 16, false, _nvColor);
										}

										// draw bullet itself
										voxelPos = proj->getPosition(1-i);
										if (voxelPos.x / 16 == itX &&
											voxelPos.y / 16 == itY &&
											voxelPos.z / 24 == itZ &&
											_save->getTileEngine()->isVoxelVisible(voxelPos))
										{
											_camera->convertVoxelToScreen(voxelPos, &bulletPositionScreen);
											bulletPositionScreen.x -= tmpSurface.getWidth() / 2;
											bulletPositionScreen.y -= tmpSurface.getHeight() / 2;
											Surface::blitRaw(surface, tmpSurface, bulletPositionScreen.x, bulletPositionScreen.y, 0, false, _nvColor);
										}
									}
								}
							}
						}
						} // end for proj
					}

					//draw particle clouds
					int pixelMaskArray[] = { 0, 2, 1, 3 };
					SurfaceRaw<int> pixelMask(pixelMaskArray, 2, 2);
					const int vaporScreenOriginX = screenPosition.x + _spriteWidth / 2;
					const int vaporScreenOriginY = screenPosition.y + _spriteHeight - _spriteWidth / 2 + tile->getPosition().toVoxel().z;
					const Uint8* const transparetPtr = _transparencies->data();

					//draw particle clouds behind solder
					for (const Particle& p : getVaporParticle(tile, 0))
					{
						int vaporX = vaporScreenOriginX + p.getOffsetX();
						int vaporY = vaporScreenOriginY + p.getOffsetY();
						auto transparetOffsets = transparetPtr
							+ (p.getColor() * Mod::TransparenciesOpacityLevels * Mod::TransparenciesPaletteColors)
							+ (p.getOpacity() * Mod::TransparenciesPaletteColors);

						ShaderDrawFunc(
							[&](Uint8& dest, int size)
							{
								if (p.getSize() <= size)
								{
									dest = transparetOffsets[dest];
								}
							},
							ShaderSurface(this),
							ShaderMove(pixelMask, vaporX, vaporY)
						);
					}

					unit = tile->getUnit();
					// Draw soldier from this tile, below or above
					drawUnit(unitSprite, tile, tile, screenPosition, topLayer, isUnitMovingNearby ? movingUnit : nullptr);

					if (isUnitMovingNearby)
					{
						// special handling for a moving unit in foreground of tile.
						constexpr static Position frontPos[] =
						{
							Position(-1, +1, 0),
							Position(0, +1, 0),
							Position(+1, +1, 0),
							Position(+1, 0, 0),
							Position(+1, -1, 0),
						};

						for (size_t f = 0; f < std::size(frontPos); ++f)
						{
							drawUnit(unitSprite, _save->getTile(mapPosition + frontPos[f]), tile, screenPosition, topLayer);
						}
					}

					// Draw smoke/fire
					if (tile->getSmoke() && tile->isDiscovered(O_FLOOR))
					{
						frameNumber = 0;
						int shade = 0;
						if (!tile->getFire())
						{
							if (_save->getDepth() > 0)
							{
								frameNumber += Mod::UNDERWATER_SMOKE_OFFSET;
							}
							else
							{
								frameNumber += Mod::SMOKE_OFFSET;
							}
							if (Mod::EXTENDED_SMOKE_OFFSET == 0)
							{
								frameNumber += int(floor((tile->getSmoke() / 6.0) - 0.1)); // see http://www.ufopaedia.org/images/c/cb/Smoke.gif
							}
							else if (Mod::EXTENDED_SMOKE_OFFSET == 1)
							{
								frameNumber += int(floor((tile->getSmoke() / 6.0) - 0.1)) * 4;
							}
							else // if (Mod::EXTENDED_SMOKE_OFFSET == 2)
							{
								frameNumber += (tile->getSmoke() - 1) / 5 * 4;
							}
							shade = tileShade;
						}

						if (halfAnimFrame + tile->getAnimationOffset() > 3)
						{
							frameNumber += halfAnimFrame + tile->getAnimationOffset() - 4;
						}
						else
						{
							frameNumber += halfAnimFrame + tile->getAnimationOffset();
						}
						tmpSurface = _game->getMod()->getSurfaceSet("SMOKE.PCK")->getFrame(frameNumber);
						Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y, shade, false, _nvColor);
					}

					//draw particle clouds on front of solder
					for (const Particle& p : getVaporParticle(tile, topLayer ? 3 : 1))
					{
						int vaporX = vaporScreenOriginX + p.getOffsetX();
						int vaporY = vaporScreenOriginY + p.getOffsetY();
						auto transparetOffsets = transparetPtr
							+ (p.getColor() * Mod::TransparenciesOpacityLevels * Mod::TransparenciesPaletteColors)
							+ (p.getOpacity() * Mod::TransparenciesPaletteColors);

						ShaderDrawFunc(
							[&](Uint8& dest, int size)
							{
								if (p.getSize() <= size)
								{
									dest = transparetOffsets[dest];
								}
							},
							ShaderSurface(this),
							ShaderMove(pixelMask, vaporX, vaporY)
						);
					}

					// Draw Path Preview
					if (_previewSettingArrows && tile->getPreview() != -1 && tile->isDiscovered(O_FLOOR))
					{
						if (itZ > 0 && tile->hasNoFloor(_save))
						{
							tmpSurface = _game->getMod()->getSurfaceSet("Pathfinding")->getFrame(11);
							if (tmpSurface)
							{
								Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y+2, 0, false, tile->getMarkerColor());
							}
						}
						tmpSurface = _game->getMod()->getSurfaceSet("Pathfinding")->getFrame(tile->getPreview());
						if (tmpSurface)
						{
							Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y + tile->getTerrainLevel(), 0, false, tileColor);
						}
					}

					{
						// Draw object
						tmpSurface = tile->getSprite(O_OBJECT);
						if (tmpSurface)
						{
							if (!tile->isBackTileObject(O_OBJECT))
							{
								if (tile->getObstacle(O_OBJECT))
									Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y - tile->getYOffset(O_OBJECT), obstacleShade, false, _nvColor);
								else
									Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y - tile->getYOffset(O_OBJECT), tileShade, false, _nvColor);
							}
						}
					}
					// Draw cursor front
					if (_cursorType != CT_NONE && _selectorX > itX - _cursorSize && _selectorY > itY - _cursorSize && _selectorX < itX+1 && _selectorY < itY+1 && !_save->getBattleState()->getMouseOverIcons())
					{
						if (_camera->getViewLevel() == itZ)
						{
							if (_cursorType != CT_AIM)
							{
								if (unit && (unit->getVisible() || _save->getDebugMode()))
									frameNumber = 3 + halfAnimFrameRest; // yellow box
								else
									frameNumber = 3; // red box
							}
							else
							{
								if (unit && (unit->getVisible() || _save->getDebugMode()))
									frameNumber = 7 + halfAnimFrame; // yellow animated crosshairs
								else
									frameNumber = 6; // red static crosshairs
							}
							tmpSurface = _game->getMod()->getSurfaceSet("CURSOR.PCK")->getFrame(frameNumber);
							Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y, 0);

							// DX aim-cone weapons show a physical hit-chance readout even when UFO Extender
							// accuracy is off (the cone produces range falloff intrinsically, so the number
							// is meaningful regardless) - as long as the player hasn't disabled crosshair info.
							bool coneInfoReadout = false;
							if (_cursorType == CT_AIM && Options::oxceShowAccuracyOnCrosshair != 0)
							{
								BattleAction *ca = _save->getBattleGame()->getCurrentAction();
								// dual-fire has no weapon of its own; gate on the display (cone-preferred) hand
								const BattleItem *caWeapon = (ca && ca->type == BA_DUALFIRE && ca->actor)
									? ca->actor->getDualFireDisplayWeapon() : (ca ? ca->weapon : nullptr);
								coneInfoReadout = caWeapon && caWeapon->getRules()->getBaseAccuracy() > 0;
							}

							// UFO extender accuracy: display adjusted accuracy value on crosshair in real-time.
							if (_cursorType >= CT_AIM && (_showInfoOnCursor || coneInfoReadout) && (_cursorType != CT_THROW || !Options::oxceDisableInfoOnThrowCursor))
							{
								BattleAction *action = _save->getBattleGame()->getCurrentAction();
								const RuleItem *weapon = action->weapon->getRules();
								std::ostringstream ss;
								BattleActionAttack attack = BattleActionAttack::GetBeforeShoot(*action);
								int distanceSq = action->actor->distance3dToPositionSq(Position(itX, itY,itZ));
								int distance = (int)std::ceil(sqrt(float(distanceSq)));

								if (_cursorType == CT_AIM || _cursorType == CT_THROW)
								{
								// DX dual-fire fires both hands at once; show each hand's own chance (R: right,
								// L: left) since the two weapons/modes can differ. Otherwise fall through to
								// the single-shot aim-cone / native readout below.
								bool dualFire = _cursorType == CT_AIM && action->type == BA_DUALFIRE && action->actor;
								// DX aim-cone weapons: show the estimated *physical* hit chance instead of
								// the native folded accuracy (which is only the soldier-cone input, and whose
								// dropoff/range terms don't apply on the cone path).
								bool coneModel = !dualFire && _cursorType == CT_AIM
									&& weapon->getBaseAccuracy() > 0
									&& action->type != BA_THROW
									&& action->type != BA_LAUNCH
									&& action->type != BA_HIT;
								if (dualFire)
								{
									// Line of sight (cached, keyed on cursor tile + ctrl) - shared by both hands.
									bool hasLOS = false;
									if (Position(itX, itY, itZ) == _cacheCursorPosition && _isCtrlPressed == _cacheIsCtrlPressed && _cacheHasLOS != -1)
										hasLOS = (_cacheHasLOS == 1);
									else
									{
										if (unit && (unit->getVisible() || _save->getDebugMode()))
											hasLOS = _save->getTileEngine()->visible(action->actor, tile);
										else
											hasLOS = _save->getTileEngine()->isTileInLOS(action, tile, true);
										_cacheIsCtrlPressed = _isCtrlPressed;
										_cacheCursorPosition = Position(itX, itY, itZ);
										_cacheHasLOS = hasLOS ? 1 : 0;
									}

									// Both hands' hit-chances are voxel-traced, so cache them (R in _cacheHitChance,
									// L in _cacheHitChance2) and recompute only when the aim changes.
									Position cursorPos(itX, itY, itZ);
									int kneeled = action->actor->isKneeled() ? 1 : 0;
									int rChance, lChance;
									if (_cacheHitChance != -1
										&& cursorPos == _cacheHitChancePosition
										&& (_isCtrlPressed ? 1 : 0) == _cacheHitChanceCtrl
										&& action->weapon == _cacheHitChanceWeapon
										&& (int)action->type == _cacheHitChanceActionType
										&& kneeled == _cacheHitChanceKneeled)
									{
										rChance = _cacheHitChance;
										lChance = _cacheHitChance2;
									}
									else
									{
										// Per-hand display percent at the hovered tile: aim-cone hit-chance for a
										// cone weapon, folded accuracy for a vanilla one.
										auto handPercent = [&](BattleItem *hw) -> int
										{
											if (!hw) return 0;
											const RuleItem *hr = hw->getRules();
											BattleActionType mode = hr->getDualFireMode();
											int dSq = action->actor->distance3dToPositionSq(cursorPos);
											if (hr->isOutOfRange(dSq)) return 0;
											BattleAction ha = *action;
											ha.weapon = hw;
											ha.type = mode;
											if (hr->getBaseAccuracy() > 0)
												return Projectile::calculateHitChancePercent(_save, &ha, cursorPos, hw->getAmmoForAction(mode), _game->getMod(), hasLOS);
											return BattleUnit::getFiringAccuracy(BattleActionAttack::GetBeforeShoot(ha), _game->getMod());
										};
										rChance = handPercent(action->actor->getRightHandWeapon());
										lChance = handPercent(action->actor->getLeftHandWeapon());
										_cacheHitChance = rChance;
										_cacheHitChance2 = lChance;
										_cacheHitChanceCover = 0;
										_cacheHitChancePosition = cursorPos;
										_cacheHitChanceCtrl = _isCtrlPressed ? 1 : 0;
										_cacheHitChanceWeapon = action->weapon;
										_cacheHitChanceActionType = (int)action->type;
										_cacheHitChanceKneeled = kneeled;
									}

									// color-grade by the better hand (the player's best shot)
									int best = std::max(rChance, lChance);
									if (best >= 65)
										_txtAccuracy->setColor(Palette::blockOffset(Pathfinding::green - 1) - 1);
									else if (best >= 35)
										_txtAccuracy->setColor(Palette::blockOffset(Pathfinding::yellow - 1) - 1);
									else
										_txtAccuracy->setColor(Palette::blockOffset(Pathfinding::red - 1) - 1);

									ss << "R:" << rChance << "% L:" << lChance << "% @" << distance << "m";
								}
								else if (coneModel)
								{
									// Line of sight (cached, keyed on cursor tile + ctrl) widens the soldier cone.
									bool hasLOS = false;
									if (Position(itX, itY, itZ) == _cacheCursorPosition && _isCtrlPressed == _cacheIsCtrlPressed && _cacheHasLOS != -1)
									{
										hasLOS = (_cacheHasLOS == 1);
									}
									else
									{
										if (unit && (unit->getVisible() || _save->getDebugMode()))
											hasLOS = _save->getTileEngine()->visible(action->actor, tile);
										else
											hasLOS = _save->getTileEngine()->isTileInLOS(action, tile, true);
										_cacheIsCtrlPressed = _isCtrlPressed;
										_cacheCursorPosition = Position(itX, itY, itZ);
										_cacheHasLOS = hasLOS ? 1 : 0;
									}

									// The hit-chance estimate voxel-traces hundreds of rays, so cache it (and its
									// cover term) and recompute only when the aim changes (cursor / ctrl / weapon /
									// action / stance).
									int chance, cover;
									Position cursorPos(itX, itY, itZ);
									int kneeled = (action->actor && action->actor->isKneeled()) ? 1 : 0;
									if (_cacheHitChance != -1
										&& cursorPos == _cacheHitChancePosition
										&& (_isCtrlPressed ? 1 : 0) == _cacheHitChanceCtrl
										&& action->weapon == _cacheHitChanceWeapon
										&& (int)action->type == _cacheHitChanceActionType
										&& kneeled == _cacheHitChanceKneeled)
									{
										chance = _cacheHitChance;
										cover = _cacheHitChanceCover;
									}
									else
									{
										cover = 0;
										chance = weapon->isOutOfRange(distanceSq)
											? 0
											: Projectile::calculateHitChancePercent(_save, action, cursorPos, attack.damage_item, _game->getMod(), hasLOS, &cover);
										_cacheHitChance = chance;
										_cacheHitChanceCover = cover;
										_cacheHitChancePosition = cursorPos;
										_cacheHitChanceCtrl = _isCtrlPressed ? 1 : 0;
										_cacheHitChanceWeapon = action->weapon;
										_cacheHitChanceActionType = (int)action->type;
										_cacheHitChanceKneeled = kneeled;
									}

									// color-grade the readout red -> yellow -> green by hit chance
									if (chance >= 65)
										_txtAccuracy->setColor(Palette::blockOffset(Pathfinding::green - 1) - 1);
									else if (chance >= 35)
										_txtAccuracy->setColor(Palette::blockOffset(Pathfinding::yellow - 1) - 1);
									else
										_txtAccuracy->setColor(Palette::blockOffset(Pathfinding::red - 1) - 1);

									// one line: "X% (-Y%) @ Zm" (cover term omitted when zero): e.g.
									// "45% (-20%) @ 12m" or "72% @ 8m".
									ss << chance << "%";
									if (cover > 0)
										ss << " (-" << cover << "%)";
									ss << " @ " << distance << "m";
								}
								else if (_cursorType == CT_THROW && Options::battleRealisticThrowing && action->type == BA_THROW)
								{
									// DX realistic throwing: show the estimated chance the item lands on the
									// exact target tile (not the abstract throw-accuracy stat). Cached like the
									// hit-chance readout, since it Monte-Carlos the parabola.
									int chance;
									Position cursorPos(itX, itY, itZ);
									int kneeled = (action->actor && action->actor->isKneeled()) ? 1 : 0;
									if (_cacheHitChance != -1
										&& cursorPos == _cacheHitChancePosition
										&& (_isCtrlPressed ? 1 : 0) == _cacheHitChanceCtrl
										&& action->weapon == _cacheHitChanceWeapon
										&& (int)action->type == _cacheHitChanceActionType
										&& kneeled == _cacheHitChanceKneeled)
									{
										chance = _cacheHitChance;
									}
									else
									{
										chance = weapon->isOutOfThrowRange(distanceSq, _save->getDepth())
											? 0
											: Projectile::calculateThrowLandChancePercent(_save, action, cursorPos, _game->getMod());
										_cacheHitChance = chance;
										_cacheHitChanceCover = 0;
										_cacheHitChancePosition = cursorPos;
										_cacheHitChanceCtrl = _isCtrlPressed ? 1 : 0;
										_cacheHitChanceWeapon = action->weapon;
										_cacheHitChanceActionType = (int)action->type;
										_cacheHitChanceKneeled = kneeled;
									}

									if (chance >= 65)
										_txtAccuracy->setColor(Palette::blockOffset(Pathfinding::green - 1) - 1);
									else if (chance >= 35)
										_txtAccuracy->setColor(Palette::blockOffset(Pathfinding::yellow - 1) - 1);
									else
										_txtAccuracy->setColor(Palette::blockOffset(Pathfinding::red - 1) - 1);

									ss << chance << "% @ " << distance << "m";
								}
								else
								{
									int accuracy = BattleUnit::getFiringAccuracy(attack, _game->getMod());

									{
										int upperLimit, lowerLimit;
										int dropoff = weapon->calculateLimits(upperLimit, lowerLimit, _save->getDepth(), action->type);

										// at this point, let's assume the shot is adjusted and set the text amber.
										_txtAccuracy->setColor(Palette::blockOffset(Pathfinding::yellow - 1) - 1);

										if (distance > upperLimit)
										{
											accuracy -= (distance - upperLimit) * dropoff;
										}
										else if (distance < lowerLimit)
										{
											accuracy -= (lowerLimit - distance) * dropoff;
										}
										else
										{
											// no adjustment made? set it to green.
											_txtAccuracy->setColor(Palette::blockOffset(Pathfinding::green - 1) - 1);
										}
									}

									// Include LOS penalty for tiles in the unit's current view range
									// Don't recalculate LOS for outside of the current FOV
									int noLOSAccuracyPenalty = action->weapon->getRules()->getNoLOSAccuracyPenalty(_game->getMod());
									if (noLOSAccuracyPenalty != -1)
									{
										bool hasLOS = false;
										if (Position(itX, itY, itZ) == _cacheCursorPosition && _isCtrlPressed == _cacheIsCtrlPressed && _cacheHasLOS != -1)
										{
											// use cached result
											hasLOS = (_cacheHasLOS == 1);
										}
										else
										{
											// recalculate
											if (unit && (unit->getVisible() || _save->getDebugMode()))
											{
												hasLOS = _save->getTileEngine()->visible(action->actor, tile);
											}
											else
											{
												hasLOS = _save->getTileEngine()->isTileInLOS(action, tile, true);
											}
											// remember
											_cacheIsCtrlPressed = _isCtrlPressed;
											_cacheCursorPosition = Position(itX, itY, itZ);
											_cacheHasLOS = hasLOS ? 1 : 0;
										}

										if (!hasLOS)
										{
											accuracy = accuracy * noLOSAccuracyPenalty / 100;
											_txtAccuracy->setColor(Palette::blockOffset(Pathfinding::yellow - 1) - 1);
										}
									}

									bool outOfRange = action->type == BA_THROW
										? weapon->isOutOfThrowRange(distanceSq, _save->getDepth())
										: weapon->isOutOfRange(distanceSq);

									// zero accuracy or out of range: set it red.
									if (accuracy <= 0 || outOfRange)
									{
										accuracy = 0;
										_txtAccuracy->setColor(Palette::blockOffset(Pathfinding::red - 1) - 1);
									}
									ss << accuracy;
									ss << "%";
								}

								} // end cone-model vs native accuracy readout
								// DX: only the actions that actually resolve through psiAttackCalculate get a chance
								// readout. BA_CLAIRVOYANCE shares the psi cursor but is NOT a contest - it is a flat
								// psiScore vs minPsiScore check against no target at all - so a percentage there would
								// be pure fiction.
								else if (_cursorType == CT_PSI && Options::psiChanceIndicatorEnabled
									&& weapon->getBattleType() == BT_PSIAMP && unit && !unit->isOut()
									// Never over a unit the player cannot see: the readout appearing at all would
									// betray that something is standing there.
									&& (unit->getVisible() || _save->getDebugMode())
									&& (action->type == BA_MINDCONTROL || action->type == BA_PANIC
										|| action->type == BA_MINDBLAST || action->type == BA_USE))
								{
									// DX: psi success chance on hover. Psi used to be the only attack with no accuracy
									// feedback at all - the action menu shows TU only, and the sole readout was the Alt-held
									// margin range below (which this supersedes; see the guard there).
									//
									// The contest is `attack + roll - defence - dropoff > 0`, with the roll a uniform integer
									// over [0, 55] (the default `tryPsiAttackItem` script body). So of the 56 equally likely
									// rolls exactly `margin + 55` win - an exact probability, not a heuristic.
									const int attackStrength = BattleUnit::getPsiAccuracy(attack);

									// Reveal the target's real psi defence only for units the player actually knows: his own
									// (and civilians), or a hostile whose type has been researched. Against an unresearched
									// hostile fall back to the baseline 30 - what the stock indicator has always assumed - and
									// mark the figure as an estimate. A hover must not be a free interrogation: the player
									// cannot read an alien's psiDefence off the percentage.
									bool fullInfo = unit->getOriginalFaction() != FACTION_HOSTILE;
									if (!fullInfo)
									{
										const SavedGame *geo = _game->getSavedGame();
										fullInfo = geo && geo->isResearched(unit->getType());
									}

									int defenseStrength = 30;
									if (fullInfo)
									{
										defenseStrength += unit->getArmor()->getPsiDefence(unit);
										// DX counter-control: prising a unit out of an existing mind control is a contest against
										// its CONTROLLER, so that is whose defence the readout must use.
										if (action->type == BA_MINDCONTROL && unit->isMindControlled())
										{
											if (const BattleUnit *controller = _save->getMindController(unit))
											{
												defenseStrength = 30 + controller->getArmor()->getPsiDefence(controller);
											}
										}
									}

									// Distance falloff is measured in voxel space, exactly as the contest measures it.
									const float dis = Position::distance(action->actor->getPosition().toVoxel(), Position(itX, itY, itZ).toVoxel());
									const int margin = attackStrength - defenseStrength - (int)weapon->getPsiAccuracyRangeReduction(dis);

									// Winning rolls out of the 56 possible.
									int wins = margin + 55;
									if (wins < 0) wins = 0;
									if (wins > 56) wins = 56;
									int chance = weapon->isOutOfRange(distanceSq) ? 0 : wins * 100 / 56;

									if (chance >= 65)
										_txtAccuracy->setColor(Palette::blockOffset(Pathfinding::green - 1) - 1);
									else if (chance >= 35)
										_txtAccuracy->setColor(Palette::blockOffset(Pathfinding::yellow - 1) - 1);
									else
										_txtAccuracy->setColor(Palette::blockOffset(Pathfinding::red - 1) - 1);

									// A leading "~" marks a figure computed against the baseline defence rather than the
									// target's real one.
									if (!fullInfo)
									{
										ss << "~";
									}
									ss << chance << "%" << " @ " << distance << "m";
								}

									//TODO: merge this code with `InventoryState::calculateCurrentDamageTooltip` as 90% is same or should be same
									// display additional damage and psi-effectiveness info
								if (_isAltPressed)
								{
									// step 1: determine rule
									const RuleItem *rule;
									if (weapon->getBattleType() == BT_PSIAMP)
									{
										rule = weapon;
									}
									else if (action->weapon->needsAmmoForAction(action->type))
									{
										auto* ammo = attack.damage_item;
										if (ammo != nullptr)
										{
											rule = ammo->getRules();
										}
										else
										{
											rule = 0; // empty weapon = no rule
										}
									}
									else
									{
										rule = weapon;
									}

									// step 2: check if unlocked
									if (_cacheActiveWeaponUfopediaArticleUnlocked == -1)
									{
										_cacheActiveWeaponUfopediaArticleUnlocked = 0;
										if (_game->getSavedGame()->getMonthsPassed() == -1)
										{
											_cacheActiveWeaponUfopediaArticleUnlocked = 1; // new battle mode
										}
										else if (rule)
										{
											_cacheActiveWeaponUfopediaArticleUnlocked = 1; // assume unlocked
											ArticleDefinition *article = _game->getMod()->getUfopaediaArticle(rule->getType(), false);
											if (article && !Ufopaedia::isArticleAvailable(_game->getSavedGame(), article))
											{
												_cacheActiveWeaponUfopediaArticleUnlocked = 0; // ammo/weapon locked
											}
											if (rule->getType() != weapon->getType())
											{
												article = _game->getMod()->getUfopaediaArticle(weapon->getType(), false);
												if (article && !Ufopaedia::isArticleAvailable(_game->getSavedGame(), article))
												{
													_cacheActiveWeaponUfopediaArticleUnlocked = 0; // weapon locked
												}
											}
										}
									}

									// step 3: calculate and draw
									if (rule && _cacheActiveWeaponUfopediaArticleUnlocked == 1)
									{
										// DX: superseded by the psi chance readout above, which shows an actual
										// probability rather than a margin range. Only fall back to this stock
										// min-max indicator when that one is switched off - two psi numbers that
										// disagree would be worse than the one that used to be hidden.
										if (rule->getBattleType() == BT_PSIAMP && !Options::psiChanceIndicatorEnabled)
										{
											float attackStrength = BattleUnit::getPsiAccuracy(attack);
											float defenseStrength = 30.0f; // indicator ignores: +victim->getArmor()->getPsiDefence(victim);

											float dis = Position::distance(action->actor->getPosition().toVoxel(), Position(itX, itY, itZ).toVoxel());
											int min = attackStrength - defenseStrength - rule->getPsiAccuracyRangeReduction(dis);
											int max = min + 55;
											if (max <= 0)
											{
												ss << "0%";
											}
											else
											{
												ss << min << "-" << max << "%";
											}
										}
										if (rule->getBattleType() != BT_PSIAMP || action->type == BA_USE)
										{
											int totalDamage = 0;
											if (weapon->getIgnoreAmmoPower())
											{
												totalDamage += weapon->getPowerBonus(attack);
												totalDamage -= weapon->getPowerRangeReduction(distance * 16);
											}
											else
											{
												totalDamage += rule->getPowerBonus(attack);
												totalDamage -= rule->getPowerRangeReduction(distance * 16);
											}
											if (totalDamage < 0) totalDamage = 0;
											if (_cursorType != CT_WAYPOINT)
												ss << "\n";
											ss << rule->getDamageType()->getRandomDamage(totalDamage, 1);
											ss << "-";
											ss << rule->getDamageType()->getRandomDamage(totalDamage, 2);
											if (rule->getDamageType()->RandomType == DRT_UFO_WITH_TWO_DICE)
												ss << "*";
										}
									}
									else
									{
										ss << "\n?-?";
									}
								}

								_txtAccuracy->setText(ss.str());
								_txtAccuracy->draw();
								// centered over the tile (90px box) and lifted just above the crosshair;
								// the actual blit is deferred until after the targeting tracers/dots (below).
								_accuracyTextX = screenPosition.x + 16 - 45;
								_accuracyTextY = screenPosition.y - 10;
								_pendingAccuracyText = true;
								_cursorAccuracyShown = true;
							}
						}
						else if (_camera->getViewLevel() > itZ)
						{
							frameNumber = 5; // blue box
							tmpSurface = _game->getMod()->getSurfaceSet("CURSOR.PCK")->getFrame(frameNumber);
							Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y, 0);
						}
						if (!_isAltPressed && _cursorType > CT_AIM && _camera->getViewLevel() == itZ)
						{
							bool ignore = false;
							if (_cursorType == CT_PSI || _cursorType == CT_WAYPOINT)
							{
								BattleAction* action = _save->getBattleGame()->getCurrentAction();
								int distanceSq = action->actor->distance3dToPositionSq(Position(itX, itY, itZ));
								if (action->weapon->getRules()->isOutOfRange(distanceSq))
								{
									// weapon doesn't work at this distance, just draw a normal cursor with a red 0% hint text
									ignore = true;
									_txtAccuracy->setColor(Palette::blockOffset(Pathfinding::red - 1) - 1);
									_txtAccuracy->setText("0%");
									_txtAccuracy->draw();
									_accuracyTextX = screenPosition.x + 16 - 45;
									_accuracyTextY = screenPosition.y - 10;
									_pendingAccuracyText = true;
									_cursorAccuracyShown = true;
								}
							}
							if (!ignore)
							{
								int frame[6] = { 0, 0, 0, 11, 13, 15 };
								tmpSurface = _game->getMod()->getSurfaceSet("CURSOR.PCK")->getFrame(frame[_cursorType] + (_animFrame / 4) % 2);
								Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y, 0);
							}
						}
					}

					// DX: floating name label over the unit under the cursor (knowledge-aware,
					// faction-colored). Only the exact hovered tile on the current view level, and
					// never for a unit the player can't see.
					if (Options::hoveredUnitNameEnabled && unit
						&& _selectorX == itX && _selectorY == itY && _camera->getViewLevel() == itZ
						&& (unit->getVisible() || _save->getDebugMode())
						&& !_save->getBattleState()->getMouseOverIcons())
					{
						int nameColor;
						switch (unit->getFaction())
						{
						case FACTION_PLAYER:  nameColor = Palette::blockOffset(Pathfinding::green - 1) - 1; break;
						case FACTION_HOSTILE: nameColor = Palette::blockOffset(Pathfinding::red - 1) - 1; break;
						default:              nameColor = Palette::blockOffset(Pathfinding::yellow - 1) - 1; break;
						}
						_txtUnitName->setColor(nameColor);
						_txtUnitName->setText(_save->getCombatLogName(unit));
						_txtUnitName->draw();
						// Center the 120px label over the 32px tile and lift it above the unit's head -
						// and higher still (above the accuracy readout) when that readout is being drawn.
						// Blit is deferred until after the targeting tracers/dots (below) so it stays on top.
						_unitNameX = screenPosition.x + 16 - 60;
						_unitNameY = screenPosition.y - (_cursorAccuracyShown ? 20 : 10);
						_pendingUnitName = true;
					}

					// Draw waypoints if any on this tile
					int waypid = 1;
					int waypXOff = 2;
					int waypYOff = 2;

					for (const auto& waypoint : _waypoints)
					{
						if (waypoint == mapPosition)
						{
							if (waypXOff == 2 && waypYOff == 2)
							{
								tmpSurface = _game->getMod()->getSurfaceSet("CURSOR.PCK")->getFrame(7);
								Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y, 0);
							}
							if (_save->getBattleGame()->getCurrentAction()->type == BA_LAUNCH || _save->getBattleGame()->getCurrentAction()->sprayTargeting)
							{
								_numWaypid->setValue(waypid);
								_numWaypid->setBordered(true); // OXCE, not configurable
								_numWaypid->draw();
								_numWaypid->blitNShade(surface, screenPosition.x + waypXOff, screenPosition.y + waypYOff, 0);

								waypXOff += waypid > 9 ? 10 : 6; // OXCE
								if (waypXOff >= 26)
								{
									waypXOff = 2;
									waypYOff += 8;
								}
							}
						}
						waypid++;
					}
				}
			}
		}
	}
	if (pathfinderTurnedOn)
	{
		if (_numWaypid)
		{
			_numWaypid->setBordered(true); // give it a border for the pathfinding display, makes it more visible on snow, etc.
		}
		for (int itZ = beginZ; itZ <= endZ; itZ++)
		{
			for (int itX = beginX; itX <= endX; itX++)
			{
				for (int itY = beginY; itY <= endY; itY++)
				{
					mapPosition = Position(itX, itY, itZ);
					_camera->convertMapToScreen(mapPosition, &screenPosition);
					screenPosition += _camera->getMapOffset();

					// only render cells that are inside the surface
					if (screenPosition.x > -_spriteWidth && screenPosition.x < surface->getWidth() + _spriteWidth &&
						screenPosition.y > -_spriteHeight && screenPosition.y < surface->getHeight() + _spriteHeight )
					{
						tile = _save->getTile(mapPosition);
						if (!tile || !tile->isDiscovered(O_FLOOR) || tile->getPreview() == -1)
							continue;
						int adjustment = -tile->getTerrainLevel();
						if (_previewSettingArrows)
						{
							if (itZ > 0 && tile->hasNoFloor(_save))
							{
								tmpSurface = _game->getMod()->getSurfaceSet("Pathfinding")->getFrame(23);
								if (tmpSurface)
								{
									Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y+2, 0, false, tile->getMarkerColor());
								}
							}
							int overlay = tile->getPreview() + 12;
							tmpSurface = _game->getMod()->getSurfaceSet("Pathfinding")->getFrame(overlay);
							if (tmpSurface)
							{
								Surface::blitRaw(surface, tmpSurface, screenPosition.x, screenPosition.y - adjustment, 0, false, tile->getMarkerColor());
							}
						}

						if ((_previewSettingTu || _previewSettingEnergy) && (tile->getTUMarker() > -1 || tile->getEnergyMarker() > -1))
						{
							int off = tile->getTUMarker() > 9 ? 5 : 3;
							int offE = tile->getEnergyMarker() > 9 ? 5 : 3;
							int mcolor = _previewSettingArrows ? 0 : tile->getMarkerColor();
							if (_previewSettingArrows)
							{
								adjustment += 7;
							}
							if (_save->getSelectedUnit() && _save->getSelectedUnit()->isBigUnit())
							{
								adjustment += 1;
								if (!_previewSettingArrows)
								{
									adjustment += 7;
								}
							}
							if (_previewSettingTu)
							{
								_numWaypid->setValue(tile->getTUMarker());
								_numWaypid->draw();
								if (_previewSettingEnergy)
								{
									// TU
									_numWaypid->blitNShade(surface, screenPosition.x + 16 - off, screenPosition.y + (22 - adjustment), 0, false, mcolor);
									// and Energy
									_numWaypid->setValue(tile->getEnergyMarker());
									_numWaypid->draw();
									_numWaypid->blitNShade(surface, screenPosition.x + 16 - offE, screenPosition.y + (29 - adjustment), 0, false, mcolor);
								}
								else
								{
									// only TU
									_numWaypid->blitNShade(surface, screenPosition.x + 16 - off, screenPosition.y + (29 - adjustment), 0, false, mcolor);
								}
							}
							else if (_previewSettingEnergy)
							{
								// only Energy
								_numWaypid->setValue(tile->getEnergyMarker());
								_numWaypid->draw();
								_numWaypid->blitNShade(surface, screenPosition.x + 16 - offE, screenPosition.y + (29 - adjustment), 0, false, mcolor);
							}
						}
					}
				}
			}
		}
		if (_numWaypid)
		{
			_numWaypid->setBordered(false); // make sure we remove the border in case it's being used for missile waypoints.
		}
	}

	auto* selectedUnit = _save->getSelectedUnit();
	if (selectedUnit && (_save->getSide() == FACTION_PLAYER || _save->getDebugMode()) && selectedUnit->getPosition().z <= _camera->getViewLevel())
	{
		_camera->convertMapToScreen(selectedUnit->getPosition(), &screenPosition);
		screenPosition += _camera->getMapOffset();
		Position offset = calculateWalkingOffset(selectedUnit).ScreenOffset;
		if (selectedUnit->isBigUnit())
		{
			offset.y += 4;
		}
		offset.y += Position::TileZ - (selectedUnit->getHeight() + selectedUnit->getFloatHeight());
		if (selectedUnit->isKneeled())
		{
			offset.y -= 2;
		}
		if (this->getCursorType() != CT_NONE)
		{
			// DX: replace the bobbing selection arrow with the selected unit's role map marker,
			// if its role has one (RoleIcon<Name>Map). Units with no role / no map glyph keep the
			// default arrow. NOTE: the map markers are authored in the UI palette; colours may
			// differ under the battlescape palette (see the role icons in xcom1/roles.rul).
			Surface* marker = _arrow;
			if (Soldier* selectedSoldier = selectedUnit->getGeoscapeSoldier())
			{
				if (selectedSoldier->getRoleId() != 0)
				{
					const Role* role = _game->getSavedGame()->getRole(selectedSoldier->getRoleId());
					const RuleRoleIcon* icon = role ? _game->getMod()->getRoleIcon(role->getIcon(), false) : nullptr;
					if (icon && !icon->getMapSprite().empty())
					{
						if (Surface* mapMarker = _game->getMod()->getSurface(icon->getMapSprite(), false))
							marker = mapMarker;
					}
				}
			}
			marker->blitNShade(surface, screenPosition.x + offset.x + (_spriteWidth / 2) - (marker->getWidth() / 2), screenPosition.y + offset.y - marker->getHeight() + getArrowBobForFrame(_animFrame), 0);
		}
	}

	// DX on-map overlay: status markers hovering over the player's own living units, one per active
	// ongoing-harm condition - on fire, bleeding (fatal wounds), or losing HP each turn (negative
	// health regen) - plus a stun warning when accumulated stun is close to dropping the unit. Uses
	// the same glyphs as the unconscious-body floor overlay (mod art when present, else the DX
	// procedural fallback). All active markers stack side-by-side, centered above the head and
	// lifted clear of the selection arrow, with the arrow's hover bob. Honours the per-unit
	// disableIndicators script flag and the current view level.
	if (Options::unitStatusIndicatorEnabled)
	{
		for (auto* statusUnit : *_save->getUnits())
		{
			if (statusUnit->getFaction() != FACTION_PLAYER || statusUnit->isOut() || !statusUnit->indicatorsAreEnabled())
				continue;
			if (statusUnit->getPosition().z > _camera->getViewLevel())
				continue;

			// Gather the active condition glyphs (mod art preferred, procedural fallback otherwise).
			Surface* markers[6];
			int markerCount = 0;
			auto addMarker = [&](Surface* mod, Surface* fb)
			{
				Surface* s = mod ? mod : fb;
				if (s)
					markers[markerCount++] = s;
			};
			if (statusUnit->getFire() > 0)
				addMarker(_burnIndicator, _burnIndicatorFallback);
			if (statusUnit->getFatalWounds() > 0)
				addMarker(_woundIndicator, _woundIndicatorFallback);
			if (statusUnit->hasNegativeHealthRegen())
				addMarker(_shockIndicator, _shockIndicatorFallback);
			// Stun: warn when the unit is within a quarter of its current health of being knocked out.
			if (statusUnit->getHealth() > 0 && statusUnit->getStunlevel() * 4 >= statusUnit->getHealth() * 3)
				addMarker(_stunIndicator, _stunIndicatorFallback);
			// DX channeled mind control: show both ends of a link - my psi soldier holding a thrall, and
			// the thrall he is holding. The upkeep is otherwise invisible (it just eats his regen), so
			// without this the player has no way to see WHY his psi soldier has no TU.
			if (statusUnit->isChanneling())
				addMarker(_channelingIndicator, _channelingIndicatorFallback);
			if (statusUnit->isMindControlled())
				addMarker(_enthralledIndicator, _enthralledIndicatorFallback);
			if (markerCount == 0)
				continue;

			_camera->convertMapToScreen(statusUnit->getPosition(), &screenPosition);
			screenPosition += _camera->getMapOffset();
			Position offset = calculateWalkingOffset(statusUnit).ScreenOffset;
			if (statusUnit->isBigUnit())
			{
				offset.y += 4;
			}
			offset.y += Position::TileZ - (statusUnit->getHeight() + statusUnit->getFloatHeight());
			if (statusUnit->isKneeled())
			{
				offset.y -= 2;
			}

			// Lay the markers out as a centered row above the head, lifted by the arrow's height so
			// the row clears the selection arrow when this unit is also the selected one.
			const int gap = 1;
			int totalWidth = 0, maxHeight = 0;
			for (int i = 0; i < markerCount; ++i)
			{
				totalWidth += markers[i]->getWidth() + (i ? gap : 0);
				if (markers[i]->getHeight() > maxHeight)
					maxHeight = markers[i]->getHeight();
			}
			int drawX = screenPosition.x + offset.x + (_spriteWidth / 2) - (totalWidth / 2);
			int baseY = screenPosition.y + offset.y - _arrow->getHeight() - maxHeight + getArrowBobForFrame(_animFrame);
			for (int i = 0; i < markerCount; ++i)
			{
				markers[i]->blitNShade(surface, drawX, baseY + (maxHeight - markers[i]->getHeight()), 0);
				drawX += markers[i]->getWidth() + gap;
			}
		}
	}

	// Draw custom unit markers (Alt-held)
	if (_isAltPressed && _save->getSide() == FACTION_PLAYER && this->getCursorType() != CT_NONE)
	{
		for (auto* myUnit : *_save->getUnits())
		{
			if (myUnit->getCustomMarker() <= 0 || myUnit->getFaction() != FACTION_PLAYER || myUnit->isOut())
				continue;
			Position temp = myUnit->getPosition();
			temp.z = _camera->getViewLevel();
			_camera->convertMapToScreen(temp, &screenPosition);
			screenPosition += _camera->getMapOffset();
			Position offset;
			if (myUnit->isBigUnit())
			{
				offset.y += 4;
			}
			offset.y += Position::TileZ - (myUnit->getHeight() + myUnit->getFloatHeight());
			if (myUnit->isKneeled())
			{
				offset.y -= 2;
			}
			Surface::blitRaw(
				surface,
				_arrow,
				screenPosition.x + offset.x + (_spriteWidth / 2) - (_arrow->getWidth() / 2),
				screenPosition.y + offset.y - _arrow->getHeight() + getArrowBobForFrame(_animFrame),
				0,
				false,
				_isTFTD ? ArrowColorsTFTD[myUnit->getCustomMarker() % 4] : ArrowColorsUFO[myUnit->getCustomMarker() % 4]);
		}
	}
	delete _numWaypid;

	// Draw craft deployment preview arrows
	if (_isAltPressed && _save->isPreview() && this->getCursorType() != CT_NONE)
	{
		for (auto& pos : _save->getCraftTiles())
		{
			if (pos.z == _camera->getViewLevel())
			{
				_camera->convertMapToScreen(pos, &screenPosition);
				screenPosition += _camera->getMapOffset();
				screenPosition.y += 2; // based on vanilla soldier standHeight
				_arrow->blitNShade(
					surface,
					screenPosition.x + (_spriteWidth / 2) - (_arrow->getWidth() / 2),
					screenPosition.y - _arrow->getHeight() + getArrowBobForFrame(_animFrame),
					0);
			}
		}
	}

	// check if we got big explosions
	if (_explosionInFOV)
	{
		// big explosions cause the screen to flash as bright as possible before any explosions are actually drawn.
		// this causes everything to look like EGA for a single frame.
		if (_flashScreen)
		{
			for (int x = 0, y = 0; x < surface->getWidth() && y < surface->getHeight();)
			{
				Uint8 pixel = surface->getPixel(x, y);
				if (pixel)
				{
					pixel = (pixel & 0xF0) + 1; //avoid 0 pixel
					surface->setPixelIterative(&x, &y, pixel);
				}
			}
			_flashScreen = false;
		}
		else
		{
			for (const auto* explosion : _explosions)
			{
				_camera->convertVoxelToScreen(explosion->getPosition(), &bulletPositionScreen);
				if (explosion->isBig())
				{
					if (explosion->getCurrentFrame() >= 0)
					{
						tmpSurface = _game->getMod()->getSurfaceSet("X1.PCK")->getFrame(explosion->getCurrentFrame());
						Surface::blitRaw(surface, tmpSurface, bulletPositionScreen.x - (tmpSurface.getWidth() / 2), bulletPositionScreen.y - (tmpSurface.getHeight() / 2), 0, false, _nvColor);
					}
				}
				else if (explosion->isHit())
				{
					tmpSurface = _game->getMod()->getSurfaceSet("HIT.PCK")->getFrame(explosion->getCurrentFrame());
					Surface::blitRaw(surface, tmpSurface, bulletPositionScreen.x - 15, bulletPositionScreen.y - 25, 0, false, _nvColor);
				}
				else
				{
					tmpSurface = _game->getMod()->getSurfaceSet("SMOKE.PCK")->getFrame(explosion->getCurrentFrame());
					Surface::blitRaw(surface, tmpSurface, bulletPositionScreen.x - 15, bulletPositionScreen.y - 15, 0, false, _nvColor);
				}
			}
		}
	}

	// DX: draw the live aiming trajectory preview on top of the scene.
	drawTargetingPreview(surface);

	// DX: draw the overwatch cone markers (while aiming overwatch, or reviewing a unit on overwatch).
	drawOverwatchCone(surface);

	// DX: re-draw motion-detector readings (dithered) on top, so they show through walls/vegetation.
	drawMotionMarkersOverlay(surface);

	// DX: crosshair text (hovered-unit name + accuracy/hit-chance readout) is prepared during the
	// tile pass but blitted here, AFTER the tracers/dots, so it stays legible on top of the cloud.
	if (_pendingUnitName)
	{
		_txtUnitName->blitNShade(surface, _unitNameX, _unitNameY, 0);
	}
	if (_pendingAccuracyText)
	{
		_txtAccuracy->blitNShade(surface, _accuracyTextX, _accuracyTextY, 0);
	}

	surface->unlock();
}

/**
 * Handles mouse presses on the map.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void Map::mousePress(Action *action, State *state)
{
	InteractiveSurface::mousePress(action, state);
	_camera->mousePress(action, state);
}

/**
 * Handles mouse releases on the map.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void Map::mouseRelease(Action *action, State *state)
{
	InteractiveSurface::mouseRelease(action, state);
	_camera->mouseRelease(action, state);
}

/**
 * Handles keyboard presses on the map.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void Map::keyboardPress(Action *action, State *state)
{
	InteractiveSurface::keyboardPress(action, state);
	_camera->keyboardPress(action, state);
}

/**
 * Handles map vision toggle mode.
 */

void Map::enableNightVision()
{
	_nightVisionOn = true;
	_debugVisionMode = 0;
	persistToggles();
}

void Map::toggleNightVision()
{
	_nightVisionOn = !_nightVisionOn;
	_debugVisionMode = 0;
	persistToggles();
}

void Map::toggleDebugVisionMode()
{
	_debugVisionMode = (_debugVisionMode + 1) % 3;
	_nightVisionOn = false;
	persistToggles();
}

void Map::persistToggles()
{
	if (Options::oxceToggleNightVisionType == 2)
	{
		// persisted per campaign
		_game->getSavedGame()->setToggleNightVision(_nightVisionOn);
	}
	else if (Options::oxceToggleNightVisionType == 1)
	{
		// persisted per battle
		_save->setToggleNightVision(_nightVisionOn);
	}

	if (Options::oxceToggleBrightnessType == 2)
	{
		// persisted per campaign
		_game->getSavedGame()->setToggleBrightness(_debugVisionMode);
	}
	else if (Options::oxceToggleBrightnessType == 1)
	{
		// persisted per battle
		_save->setToggleBrightness(_debugVisionMode);
	}

	_save->setToggleBrightnessTemp(_debugVisionMode);
}

/**
 * Handles fade-in and fade-out shade modification
 * @param original tile/item/unit shade
 */

int Map::reShade(Tile *tile)
{
	// when modders just don't know where to stop...
	if (_debugVisionMode > 0)
	{
		if (_debugVisionMode == 1)
		{
			// Reaver's tests
			return tile->getShade() / 2;
		}
		// Meridian's debug helper
		return 0;
	}

	// no night vision
	if (_nvColor == 0)
	{
		return tile->getShade();
	}

	// already bright enough
	if ((tile->getShade() <= NIGHT_VISION_SHADE))
	{
		return tile->getShade();
	}

	// hybrid night vision (local)
	for (const auto* bu : *_save->getUnits())
	{
		if (bu->getFaction() == FACTION_PLAYER && !bu->isOut())
		{
			if (Position::distance2dSq(tile->getPosition(), bu->getPosition()) <= bu->getMaxViewDistanceAtDarkSquared())
			{
				return tile->getShade() > _fadeShade ? _fadeShade : tile->getShade();
			}
		}
	}

	// hybrid night vision (global)
	return std::min(+NIGHT_VISION_MAX_SHADE, tile->getShade());
}

/**
 * Handles keyboard releases on the map.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void Map::keyboardRelease(Action *action, State *state)
{
	InteractiveSurface::keyboardRelease(action, state);
	_camera->keyboardRelease(action, state);
}

/**
 * Handles mouse over events on the map.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void Map::mouseOver(Action *action, State *state)
{
	InteractiveSurface::mouseOver(action, state);
	_camera->mouseOver(action, state);
	_mouseX = (int)action->getAbsoluteXMouse();
	_mouseY = (int)action->getAbsoluteYMouse();
	setSelectorPosition(_mouseX, _mouseY);
}


/**
 * Sets the selector to a certain tile on the map.
 * @param mx mouse x position.
 * @param my mouse y position.
 */
void Map::setSelectorPosition(int mx, int my)
{
	int oldX = _selectorX, oldY = _selectorY;

	_camera->convertScreenToMap(mx, my + _spriteHeight/4, &_selectorX, &_selectorY);

	if (oldX != _selectorX || oldY != _selectorY)
	{
		_redraw = true;
	}
}

/**
 * Handles animating tiles. 8 Frames per animation.
 * @param redraw Redraw the battlescape?
 */
void Map::animate(bool redraw)
{
	_save->nextAnimFrame();
	_animFrame = _save->getAnimFrame();

	// random ambient sounds
	{
		if (!_save->getAmbienceRandom().empty())
		{
			_save->decreaseCurrentAmbienceDelay();
			if (_save->getCurrentAmbienceDelay() <= 0)
			{
				_save->resetCurrentAmbienceDelay();
				_save->playRandomAmbientSound();
			}
		}
	}

	// animate tiles
	for (int i = 0; i < _save->getMapSizeXYZ(); ++i)
	{
		_save->getTile(i)->animate();
	}

	// animate vapor
	for (auto i : Collections::rangeValueLess(_vaporParticles.size()))
	{
		auto& v = _vaporParticles[i];
		int posX = i % _camera->getMapSizeX();
		int posY = i / _camera->getMapSizeX();

		Collections::removeIf(
			v,
			[&](Particle& p)
			{
				if (p.animate())
				{
					Position tileOffset = p.updateScreenPosition();
					if (tileOffset != Position(0,0,0))
					{
						addVaporParticle(Position(posX,posY,0) + tileOffset, p);
						return true;
					}
					return false;
				}
				else
				{
					return true;
				}
			}
		);
	}

	// init vapor vector
	for (auto i : Collections::rangeValueLess(_vaporParticlesInit.size()))
	{
		auto& vi = _vaporParticlesInit[i];
		auto& vDest = _vaporParticles[i];
		if (vi.empty())
		{
			continue;
		}

		if (vDest.empty())
		{
			vi.swap(vDest);
		}
		else
		{
			vDest.insert(std::begin(vDest), std::begin(vi), std::end(vi));
		}


		Collections::removeAll(vi);
	}

	for (auto& tilePar : _vaporParticles)
	{
		if (tilePar.empty())
		{
			Collections::removeAll(tilePar);
		}
		else
		{
			std::sort(std::begin(tilePar), std::end(tilePar), [](const Particle& a, const Particle& b){ return a.getLayerZ() < b.getLayerZ(); });
		}
	}

	// animate certain units (large flying units have a propulsion animation)
	for (auto* bu : *_save->getUnits())
	{
		const Position pos = bu->getPosition();

		// skip units that do not have position
		if (pos == TileEngine::invalid)
		{
			continue;
		}

		if (_save->getDepth() > 0)
		{
			bu->setFloorAbove(false);

			// make sure this unit isn't obscured by the floor above him, otherwise it looks weird.
			if (_camera->getViewLevel() > pos.z)
			{
				for (int z = std::min(_camera->getViewLevel(), _save->getMapSizeZ() - 1); z != pos.z; --z)
				{
					if (!_save->getTile(Position(pos.x, pos.y, z))->hasNoFloor(0))
					{
						bu->setFloorAbove(true);
						break;
					}
				}
			}
		}

		bu->breathe();
	}

	if (redraw) _redraw = true;
}

/**
 * Draws the rectangle selector.
 * @param pos Pointer to a position.
 */
void Map::getSelectorPosition(Position *pos) const
{
	pos->x = _selectorX;
	pos->y = _selectorY;
	pos->z = _camera->getViewLevel();
}

/**
 * Calculates the offset of a soldier, when it is walking in the middle of 2 tiles.
 * @param unit Pointer to BattleUnit.
 * @param offset Pointer to the offset to return the calculation.
 */
UnitWalkingOffset Map::calculateWalkingOffset(const BattleUnit *unit) const
{
	UnitWalkingOffset result = { };

	int offsetX[8] = { 1, 1, 1, 0, -1, -1, -1, 0 };
	int offsetY[8] = { 1, 0, -1, -1, -1, 0, 1, 1 };
	int phase = unit->getWalkingPhase() + unit->getDiagonalWalkingPhase();
	int dir = unit->getDirection();
	int midphase = 4 + 4 * (dir % 2);
	int endphase = 8 + 8 * (dir % 2);
	int size = unit->getArmor()->getSize();

	result.ScreenOffset.x = 0;
	result.ScreenOffset.y = 0;

	if (size > 1)
	{
		if (dir < 1 || dir > 5)
			midphase = endphase;
		else if (dir == 5)
			midphase = 12;
		else if (dir == 1)
			midphase = 5;
		else
			midphase = 1;
	}
	if (unit->getVerticalDirection())
	{
		midphase = 4;
		endphase = 8;
	}
	else if ((unit->getStatus() == STATUS_WALKING || unit->getStatus() == STATUS_FLYING))
	{
		if (phase < midphase)
		{
			result.ScreenOffset.x = phase * 2 * offsetX[dir];
			result.ScreenOffset.y = - phase * offsetY[dir];
		}
		else
		{
			result.ScreenOffset.x = (phase - endphase) * 2 * offsetX[dir];
			result.ScreenOffset.y = - (phase - endphase) * offsetY[dir];
		}
	}

	result.NormalizedMovePhase = endphase == 16 ? phase : phase * 2;

	// If we are walking in between tiles, interpolate it's terrain level.
	if (unit->getStatus() == STATUS_WALKING || unit->getStatus() == STATUS_FLYING)
	{
		const Position posCurr = unit->getPosition();
		const Position posDest = unit->getDestination();
		const Position posLast = unit->getLastPosition();
		if (phase < midphase)
		{
			int fromLevel = getTerrainLevel(posCurr, size);
			int toLevel = getTerrainLevel(posDest, size);
			if (posCurr.z > posDest.z)
			{
				// going down a level, so toLevel 0 becomes +24, -8 becomes  16
				toLevel += Position::TileZ*(posCurr.z - posDest.z);
			}
			else if (posCurr.z < posDest.z)
			{
				// going up a level, so toLevel 0 becomes -24, -8 becomes -16
				toLevel = -Position::TileZ*(posDest.z - posCurr.z) + abs(toLevel);
			}
			result.TerrainLevelOffset = Interpolate(fromLevel, toLevel, phase, endphase);
		}
		else
		{
			// from phase 4 onwards the unit behind the scenes already is on the destination tile
			// we have to get it's last position to calculate the correct offset
			int fromLevel = getTerrainLevel(posLast, size);
			int toLevel = getTerrainLevel(posDest, size);
			if (posLast.z > posDest.z)
			{
				// going down a level, so fromLevel 0 becomes -24, -8 becomes -32
				fromLevel -= Position::TileZ*(posLast.z - posDest.z);
			}
			else if (posLast.z < posDest.z)
			{
				// going up a level, so fromLevel 0 becomes +24, -8 becomes 16
				fromLevel = Position::TileZ*(posDest.z - posLast.z) - abs(fromLevel);
			}
			result.TerrainLevelOffset = Interpolate(fromLevel, toLevel, phase, endphase);
		}
	}
	else
	{
		result.TerrainLevelOffset = getTerrainLevel(unit->getPosition(), size);
	}
	result.ScreenOffset.y += result.TerrainLevelOffset;
	return result;
}


/**
  * Terrainlevel goes from 0 to -24. For a larger sized unit, we need to pick the highest terrain level, which is the lowest number...
  * @param pos Position.
  * @param size Size of the unit we want to get the level from.
  * @return terrainlevel.
  */
int Map::getTerrainLevel(const Position& pos, int size) const
{
	int lowestlevel = 0;

	for (int x = 0; x < size; x++)
	{
		for (int y = 0; y < size; y++)
		{
			int l = _save->getTile(pos + Position(x,y,0))->getTerrainLevel();
			if (l < lowestlevel)
				lowestlevel = l;
		}
	}

	return lowestlevel;
}

/**
 * Sets the 3D cursor to selection/aim mode.
 * @param type Cursor type.
 * @param size Size of cursor.
 */
void Map::setCursorType(CursorType type, int size)
{
	// reset cursor indicator cache
	_cacheActiveWeaponUfopediaArticleUnlocked = -1;
	_cacheIsCtrlPressed = false;
	_cacheCursorPosition = TileEngine::invalid;
	_cacheHasLOS = -1;

	_cursorType = type;
	if (_cursorType == CT_NORMAL)
		_cursorSize = size;
	else
		_cursorSize = 1;

	// DX: leaving an aiming cursor discards any live trajectory preview.
	if (_cursorType != CT_AIM && _cursorType != CT_THROW)
		clearTargetingPreview();
}

/**
 * Gets the cursor type.
 * @return cursor type.
 */
CursorType Map::getCursorType() const
{
	return _cursorType;
}

/**
 * Discards the live aiming trajectory preview projectile and resets its rebuild cache.
 */
void Map::clearTargetingPreview()
{
	delete _targetingProjectile;
	_targetingProjectile = 0;
	_previewTarget = Position(-1, -1, -1);
	_previewActionType = -1;
	_previewActor = 0;
	_targetingDots.clear();
}

/**
 * DX: rebuilds the live aiming trajectory preview for the current fire/throw action and cursor
 * position. Traces the ideal (undeviated) path - a straight line-of-fire for direct fire, a
 * parabola for throws/arcing shots - into a dedicated projectile kept out of the in-flight
 * collection. The trace is only re-run when the aim target, action or actor changes; otherwise the
 * cached preview is kept. Clears the preview when no fire/throw action is being aimed.
 */
void Map::updateTargetingPreview()
{
	// Only while a fire/throw action is being aimed, and only if the player enabled the preview.
	if (!Options::battleTrajectoryPreview
		|| (_cursorType != CT_AIM && _cursorType != CT_THROW)
		|| _save->getBattleState()->getMouseOverIcons())
	{
		clearTargetingPreview();
		return;
	}

	BattleAction *action = _save->getBattleGame()->getCurrentAction();
	if (!action || !action->actor || !action->weapon)
	{
		clearTargetingPreview();
		return;
	}

	Position target(_selectorX, _selectorY, _camera->getViewLevel());
	Tile *targetTile = _save->getTile(target);
	if (!targetTile)
	{
		clearTargetingPreview();
		return;
	}

	// Alt shows the spread as a sampled dot cloud (in place of the tracer line); include it in the
	// rebuild key so toggling Alt re-traces.
	const bool altHeld = _game->isAltPressed(true);

	// Rebuild only when the aim target, action, actor, or Alt state changes (the voxel trace isn't free).
	if (_targetingProjectile
		&& target == _previewTarget
		&& action->type == _previewActionType
		&& (void*)action->actor == _previewActor
		&& altHeld == _previewAlt)
	{
		return;
	}
	clearTargetingPreview();
	_previewTarget = target;
	_previewActionType = action->type;
	_previewActor = (void*)action->actor;
	_previewAlt = altHeld;

	// Work on a copy so off-centre origin resolution never mutates the live action.
	BattleAction previewAction = *action;
	previewAction.target = target;

	// DX dual-fire has no single weapon/mode of its own; preview one hand's shot (prefer the
	// cone-model hand so the previewed line/readout is the meaningful one).
	if (previewAction.type == BA_DUALFIRE)
	{
		BattleItem *dispWeapon = previewAction.actor->getDualFireDisplayWeapon();
		if (dispWeapon)
		{
			previewAction.weapon = dispWeapon;
			previewAction.type = dispWeapon->getRules()->getDualFireMode();
		}
	}

	Position origin = previewAction.actor->getPosition();
	bool isThrow = (previewAction.type == BA_THROW);
	bool isArc = isThrow || previewAction.weapon->getArcingShot(previewAction.type);

	// Direct fire and arcing shots need ammo (the Projectile ctor asserts it); a pure throw doesn't.
	BattleItem *ammo = isThrow ? nullptr : previewAction.weapon->getAmmoForAction(previewAction.type);
	if (!isThrow && !ammo)
	{
		// empty weapon: nothing to preview
		return;
	}

	Position targetVoxel(0, 0, 0);
	if (!isArc)
	{
		// resolve the same aim voxel the real shot would use (no obstacle highlighting for a preview)
		if (!_save->getTileEngine()->resolveFireTargetVoxel(previewAction, origin, false, &targetVoxel))
		{
			// no line of fire to the target: nothing meaningful to draw
			return;
		}
	}

	Projectile *proj = new Projectile(_game->getMod(), _save, previewAction, origin, targetVoxel, ammo);
	int impact = isArc ? proj->calculateThrow(1.0, true) : proj->calculatePreviewTrajectory();

	// A parabola that found no valid arc has nothing to draw. A direct-fire ray always yields a
	// path - it flies to the first obstacle or on to the map edge, exactly like a real shot into
	// empty air - so only discard it when the trace produced no points at all.
	bool valid = isArc ? (impact != V_OUTOFBOUNDS) : true;
	if (!valid || proj->getTrajectory().empty())
	{
		delete proj;
		return;
	}
	_targetingProjectile = proj;

	// DX spread visualization: with Alt held, sample where the shots (aim-cone) or the throw
	// (launch error) would actually land, and stash the voxels as a dot cloud - drawTargetingPreview
	// draws these in place of the single ideal tracer line. Only for models that HAVE a spread:
	// cone-model direct fire, and realistic throwing. (Both reuse their already-cached Monte-Carlos.)
	if (altHeld)
	{
		if (isThrow)
		{
			if (Options::battleRealisticThrowing)
				Projectile::calculateThrowLandChancePercent(_save, &previewAction, target, _game->getMod(), &_targetingDots);
		}
		else if (!isArc && previewAction.weapon->getRules()->getBaseAccuracy() > 0)
		{
			bool hasLOS = _save->getTileEngine()->isTileInLOS(&previewAction, targetTile, false);
			Projectile::calculateHitChancePercent(_save, &previewAction, target, ammo, _game->getMod(), hasLOS, nullptr, &_targetingDots);
		}
	}
}

/**
 * DX: draws the live aiming trajectory preview - tracer dots spaced along the predicted path with a
 * distinct marker at the impact point. Drawn as a top pass so a lobbed throwing arc that rises above
 * the current view level is not hidden under higher floors.
 */
void Map::drawTargetingPreview(Surface *surface)
{
	if (!_targetingProjectile || !_projectileSet)
	{
		return;
	}

	// Use a single fixed standard tracer for every preview (regardless of weapon or throw), so the
	// line always reads the same. The Projectiles frame is mod-configurable via the "constants"
	// ruleset key trajectoryPreviewSprite (default frame 35, the rifle-type base bullet). Comes from
	// _projectileSet so it stays depth-correct for TFTD's underwater projectiles.
	Surface *tracer = _projectileSet->getFrame(Mod::TRAJECTORY_PREVIEW_SPRITE);
	if (!tracer)
	{
		return;
	}

	Position screen;

	// DX spread visualization (Alt held): draw the sampled impact/landing dot cloud INSTEAD of the
	// ideal tracer line. A readable subset of the samples so the cloud doesn't turn to mush; rounds
	// are coloured by outcome - green = landed on the target (unit/wall/tile, or target tile for
	// throws), yellow = aimed on target but stopped by cover, red = genuine miss - so you can see at
	// a glance how much of the spread connects and how much cover is eating.
	if (!_targetingDots.empty())
	{
		// blitRaw's newBaseColor is a palette-block index+1 (it does (v-1)<<4 internally), NOT a raw
		// palette offset - so pass the Pathfinding block constants directly.
		const int stride = std::max<int>(1, (int)_targetingDots.size() / 40);
		for (size_t i = 0; i < _targetingDots.size(); i += (size_t)stride)
		{
			int color;
			switch (_targetingDots[i].outcome)
			{
			case SPREAD_HIT:   color = Pathfinding::green;  break;
			case SPREAD_COVER: color = Pathfinding::yellow; break;
			default:           color = Pathfinding::red;    break;
			}
			_camera->convertVoxelToScreen(_targetingDots[i].pos, &screen);
			Surface::blitRaw(surface, tracer, screen.x - tracer->getWidth() / 2, screen.y - tracer->getHeight() / 2, 0, false, color);
		}
		return;
	}

	const std::vector<Position>& trajectory = _targetingProjectile->getTrajectory();
	if (trajectory.empty())
	{
		return;
	}

	const int stride = 5; // one tracer dot every few voxel steps, so it reads as a dotted line
	const int last = (int)trajectory.size() - 1;
	for (int i = 0; i < last; i += stride)
	{
		_camera->convertVoxelToScreen(trajectory[i], &screen);
		Surface::blitRaw(surface, tracer, screen.x - tracer->getWidth() / 2, screen.y - tracer->getHeight() / 2, 0, false, _nvColor);
	}

	// Impact marker at the end of the path: the engine's hit sprite (HIT.PCK, used for both UFO and
	// TFTD hit rendering), falling back to the tracer if the set is missing.
	_camera->convertVoxelToScreen(trajectory[last], &screen);
	SurfaceSet *hitSet = _game->getMod()->getSurfaceSet("HIT.PCK");
	Surface *impact = hitSet ? hitSet->getFrame(0) : nullptr;
	if (!impact)
	{
		impact = tracer;
	}
	Surface::blitRaw(surface, impact, screen.x - impact->getWidth() / 2, screen.y - impact->getHeight() / 2, 0, false, _nvColor);
}

/**
 * DX: draws markers over the tiles inside the active overwatch cone. The cone is shown while the
 * selected unit is aiming a BA_OVERWATCH action (toward the cursor) and while a unit already on
 * overwatch is selected (toward its committed target), so the player sees exactly what's watched.
 * Geometry only for now (no per-tile line-of-fire test).
 */
/**
 * DX: is this unit an active motion-detector reading? (overlay enabled, non-player unit scanned
 * this turn with recorded motion). Shared by the ground-decal draw and the overlay post-pass;
 * detection gating is unchanged - this only ever displays already-scanned units.
 * @param unit The unit (may be null).
 * @return True when a reading marker should be drawn for it.
 */
bool Map::isMotionMarkerActive(const BattleUnit *unit) const
{
	// Only while it's the player's side of the round: the turn number spans the whole round, and
	// the marker draws at the unit's CURRENT position - during the alien half it would track
	// hidden alien movement live, leaking exactly what the scanner shouldn't know yet.
	return Options::motionDetectorOverlayEnabled && unit
		&& _save->getSide() == FACTION_PLAYER
		&& unit->getFaction() != FACTION_PLAYER && !unit->isOut()
		&& unit->getMotionPoints() > 0
		&& unit->getScannedTurn() == _save->getTurn();
}

/**
 * DX: blits one motion-detector reading marker - amber, alarm-pulsing, brighter (lower shade)
 * the more the unit moved (its motion points). Shared by the ground decal and the overlay pass.
 * @param surface The surface to draw on.
 * @param unit The scanned unit (drives the intensity).
 * @param marker The marker sprite (solid or dithered variant).
 * @param x Screen x.
 * @param y Screen y.
 */
void Map::blitMotionMarker(Surface *surface, const BattleUnit *unit, Surface *marker, int x, int y)
{
	static const int Pulsate[8] = { 0, 1, 2, 3, 4, 3, 2, 1 };
	int intensity = std::min(5, unit->getMotionPoints() / 5);
	int shade = (5 - intensity) + Pulsate[_animFrame % 8];
	Surface::blitRaw(surface, marker, x, y, shade, false, 2 /* newBaseColor: block 1 = amber, palette-safe in UFO+TFTD */);
}

/**
 * DX: post-pass for the motion-detector readings. The solid marker drawn during the tile pass sits
 * under walls/units (a ground decal), which makes it near-invisible in dense terrain - so after the
 * whole scene has rendered, re-draw each reading in its DITHERED variant on top of everything. The
 * dithering keeps the obscuring terrain readable while the reading stays visible through walls,
 * vegetation and units. Same pulse/intensity as the ground decal.
 * @param surface The surface to draw on.
 */
void Map::drawMotionMarkersOverlay(Surface *surface)
{
	if (!Options::motionDetectorOverlayEnabled)
	{
		return;
	}
	SurfaceSet *markerSet = _game->getMod()->getSurfaceSet("Pathfinding2", false);
	Surface *marker = markerSet ? markerSet->getFrame(Pathfinding2Markers::ditheredFrame(Pathfinding2Markers::Target)) : nullptr;
	if (!marker)
	{
		return;
	}

	for (BattleUnit *unit : *_save->getUnits())
	{
		if (!isMotionMarkerActive(unit))
		{
			continue;
		}
		Position pos = unit->getPosition();
		if (pos.z > _camera->getViewLevel())
		{
			continue;
		}
		Tile *tile = _save->getTile(pos);
		if (!tile || !tile->isDiscovered(O_FLOOR))
		{
			continue;
		}
		Position screen;
		_camera->convertMapToScreen(pos, &screen);
		screen += _camera->getMapOffset();
		blitMotionMarker(surface, unit, marker, screen.x, screen.y + tile->getTerrainLevel());
	}
}

void Map::drawOverwatchCone(Surface *surface)
{
	BattleUnit *unit = _save->getSelectedUnit();
	if (!unit)
	{
		return;
	}

	Position origin = unit->getPosition();
	Position target;
	BattleItem *weapon = nullptr;

	BattleAction *action = _save->getBattleGame()->getCurrentAction();
	if (_cursorType == CT_AIM && action && action->type == BA_OVERWATCH && action->actor == unit)
	{
		// While confirming (a target has been clicked once), freeze the cone on that pending tile so
		// it's clear what the second click will commit to; otherwise the cone follows the cursor.
		if (!action->waypoints.empty())
		{
			target = action->waypoints.front();
		}
		else
		{
			Position cursor;
			getSelectorPosition(&cursor);
			target = cursor;
		}
		weapon = action->weapon;
	}
	else if (unit->isOnOverwatch())
	{
		target = unit->getOverwatchTarget();
		weapon = unit->getOverwatchWeapon();
	}
	else
	{
		return;
	}
	if (!weapon || target == origin)
	{
		return;
	}

	const RuleItem *rule = weapon->getRules();
	int range = rule->getOverwatchRange();
	int minRange = rule->getOverwatchMinRange();
	int angle = rule->getOverwatchConeAngle();

	// Use tile-level markers rather than floating dots, so the watched area reads as a filled
	// region (dithered variants keep the map visible underneath - see Pathfinding2Markers).
	// Watched tiles get the full-tile outline; tiles the watcher can't actually hit get the
	// X "no" marker, so dead zones differ by shape as well as colour.
	SurfaceSet *pathSet = _game->getMod()->getSurfaceSet("Pathfinding2", false);
	Surface *marker = pathSet ? pathSet->getFrame(Pathfinding2Markers::ditheredFrame(Pathfinding2Markers::FullTile)) : nullptr;
	Surface *blockedMarker = pathSet ? pathSet->getFrame(Pathfinding2Markers::ditheredFrame(Pathfinding2Markers::No)) : nullptr;
	if (!marker || !blockedMarker)
	{
		return;
	}

	TileEngine *te = _save->getTileEngine();
	// DX: per-tile line-of-fire cache is only valid for a stationary watcher - drop it if the unit moved.
	if (_owLosCacheOrigin != origin)
	{
		_owLosCache.clear();
		_owLosCacheOrigin = origin;
	}
	// Minimal action to build the firing origin voxel for this weapon/unit's line-of-fire test.
	BattleAction losAction;
	losAction.actor = unit;
	losAction.type = rule->getOverwatchShot();
	const int mapW = _save->getMapSizeX(), mapH = _save->getMapSizeY();

	Position screen;
	for (int dx = -range; dx <= range; ++dx)
	{
		for (int dy = -range; dy <= range; ++dy)
		{
			Position t(origin.x + dx, origin.y + dy, origin.z);
			Tile *tile = _save->getTile(t);
			if (!tile || !te->isInOverwatchCone(origin, target, t, minRange, range, angle))
			{
				continue;
			}
			// Tiles the watcher has line of fire to are the real watched area (yellow); tiles blocked by
			// terrain (behind a wall) are dead zones overwatch can't cover - flag them red. We use the pure
			// line-of-fire test (canTargetUnit for a hypothetical target at the tile), NOT isTileInLOS -
			// overwatch fires by line of fire up to its full cone range and is NOT limited by the ~20-tile
			// view distance (nor by the watcher's own sight; a teammate can supply the spotting).
			int key = (t.z * mapH + t.y) * mapW + t.x;
			auto it = _owLosCache.find(key);
			bool inLos;
			if (it != _owLosCache.end())
			{
				inLos = it->second;
			}
			else
			{
				losAction.target = t;
				Position originVoxel = te->getOriginVoxel(losAction, 0);
				// Raw line-of-fire trace from the firing origin to the tile's centre (~standing-unit
				// height), excluding the watcher so it can't block its own shot. Clear if the ray reaches
				// there unobstructed (V_EMPTY) or the first thing it hits is on the target tile itself;
				// blocked if terrain stops it short. No view-distance cap, so it spans the whole cone.
				Position targetVoxel = t.toVoxel() + Position(8, 8, 12);
				std::vector<Position> traj;
				VoxelType test = te->calculateLineVoxel(originVoxel, targetVoxel, false, &traj, unit);
				inLos = (test == V_EMPTY) || (test != V_OUTOFBOUNDS && !traj.empty() && traj.front().toTile() == t);
				_owLosCache[key] = inLos;
			}
			_camera->convertMapToScreen(t, &screen);
			screen += _camera->getMapOffset();
			// Full brightness (shade 0), vibrant colour - the dithered sprite supplies the translucency.
			// Hittable tiles: yellow outline; dead zones: red X marker (shape + colour cues).
			Surface::blitRaw(surface, inLos ? marker : blockedMarker, screen.x, screen.y - tile->getTerrainLevel(), 0, false,
				inLos ? Pathfinding::yellow : Pathfinding::red);
		}
	}
}

/**
 * Adds a projectile to the in-flight collection.
 * @param p Projectile to add.
 */
void Map::addProjectile(Projectile *p)
{
	_projectiles.push_back(p);
	if (Options::battleSmoothCamera)
	{
		_launch = true;
	}
}

/**
 * Removes a projectile from the in-flight collection and deletes it.
 * @param p Projectile to remove.
 */
void Map::removeProjectile(Projectile *p)
{
	auto it = std::find(_projectiles.begin(), _projectiles.end(), p);
	if (it != _projectiles.end())
	{
		_projectiles.erase(it);
	}
	delete p;
}

/**
 * Returns true if any projectiles are currently in flight.
 */
bool Map::hasProjectiles() const
{
	return !_projectiles.empty();
}

/**
 * Gets all in-flight projectiles.
 */
const std::vector<Projectile*>& Map::getProjectiles() const
{
	return _projectiles;
}

/**
 * Add new vapor particle.
 * @param pos Tile position of particle.
 * @param particle Particle to add.
 */
void Map::addVaporParticle(Position pos, Particle particle)
{
	if ((int)(_transparencies->size()) < (particle.getColor() + 1) * Mod::TransparenciesOpacityLevels * Mod::TransparenciesPaletteColors)
	{
		return;
	}
	if (pos.x >= _camera->getMapSizeX() || pos.y >= _camera->getMapSizeY())
	{
		return;
	}
	if (pos.x < 0 || pos.y < 0)
	{
		return;
	}

	auto& v = _vaporParticlesInit[_camera->getMapSizeX() * pos.y + pos.x];

	// as there will usually be more than one Particle, we prepare more space
	if (v.capacity() < 64)
	{
		v.reserve(64);
	}

	v.push_back(particle);
}

/**
 * Get all vapor for tile.
 * @param tile current tile.
 * @param topLayer if tile is top visible layer, if true then will return particles belongs to upper tiles.
 * @return range of particles that should be drawn.
 */
Collections::Range<const Particle*> Map::getVaporParticle(const Tile* tile, int topLayer) const
{
	Position pos = tile->getPosition();
	auto& v = _vaporParticles[_camera->getMapSizeX() * pos.y + pos.x];
	int startZ = pos.z * Particle::LayerAccuracy + (topLayer & 1);
	int endZ = startZ + Particle::LayerAccuracy / 2;
	auto* s = std::partition_point(v.data(), v.data() + v.size(), [&](const Particle& a){ return a.getLayerZ() < startZ; });
	auto* e = (topLayer & 2) ? v.data() + v.size() : std::partition_point(s, v.data() + v.size(), [&](const Particle& a){ return a.getLayerZ() < endZ; });
	return Collections::Range{ s, e };
}

/**
 * Gets a list of explosion sprites on the map.
 * @return A list of explosion sprites.
 */
std::list<Explosion*> *Map::getExplosions()
{
	return &_explosions;
}

/**
 * Gets the pointer to the camera.
 * @return Pointer to camera.
 */
Camera *Map::getCamera()
{
	return _camera;
}

/**
 * Timers only work on surfaces so we have to pass this on to the camera object.
 */
void Map::scrollMouse()
{
	_camera->scrollMouse();
}

/**
 * Timers only work on surfaces so we have to pass this on to the camera object.
 */
void Map::scrollKey()
{
	_camera->scrollKey();
}

/**
 * Modify the fade shade level if fade's in progress.
 */
void Map::fadeShade()
{
	bool hold = SDL_GetKeyState(NULL)[Options::keyNightVisionHold];
	if ((_nightVisionOn && !hold) || (!_nightVisionOn && hold))
	{
		_nvColor = Options::oxceNightVisionColor;
		_save->setToggleNightVisionTemp(true);
		_save->setToggleNightVisionColorTemp(_nvColor);
		if (_fadeShade > NIGHT_VISION_SHADE) // 0 = max brightness
		{
			--_fadeShade;
		}
	}
	else
	{
		if (_nvColor != 0)
		{
			if (_fadeShade < _save->getGlobalShade())
			{
				// gradually fade away
				++_fadeShade;
			}
			else
			{
				// and at the end turn off night vision
				_nvColor = 0;
				_save->setToggleNightVisionTemp(false);
				_save->setToggleNightVisionColorTemp(0);
			}
		}
	}
}

/**
 * Gets a list of waypoints on the map.
 * @return A list of waypoints.
 */
std::vector<Position> *Map::getWaypoints()
{
	return &_waypoints;
}

/**
 * Sets mouse-buttons' pressed state.
 * @param button Index of the button.
 * @param pressed The state of the button.
 */
void Map::setButtonsPressed(Uint8 button, bool pressed)
{
	setButtonPressed(button, pressed);
}

/**
 * Sets the unitDying flag.
 * @param flag True if the unit is dying.
 */
void Map::setUnitDying(bool flag)
{
	_unitDying = flag;
}

/**
 * Updates the selector to the last-known mouse position.
 */
void Map::refreshSelectorPosition()
{
	setSelectorPosition(_mouseX, _mouseY);
}

/**
 * Special handling for setting the height of the map viewport.
 * @param height the new base screen height.
 */
void Map::setHeight(int height)
{
	Surface::setHeight(height);
	_visibleMapHeight = height - _iconHeight;
	_message->setHeight((_visibleMapHeight < 200)? _visibleMapHeight : 200);
	_message->setY((_visibleMapHeight - _message->getHeight()) / 2);
}

/**
 * Special handling for setting the width of the map viewport.
 * @param width the new base screen width.
 */
void Map::setWidth(int width)
{
	int dX = width - getWidth();
	Surface::setWidth(width);
	_message->setX(_message->getX() + dX / 2);
}

/**
 * Get the hidden movement screen's vertical position.
 * @return the vertical position of the hidden movement window.
 */
int Map::getMessageY() const
{
	return _message->getY();
}

/**
 * Get the icon height.
 */
int Map::getIconHeight() const
{
	return _iconHeight;
}

/**
 * Get the icon width.
 */
int Map::getIconWidth() const
{
	return _iconWidth;
}

/**
 * Returns the angle(left/right balance) of a sound effect,
 * based off a map position.
 * @param pos the map position to calculate the sound angle from.
 * @return the angle of the sound (280 to 440).
 */
int Map::getSoundAngle(const Position& pos) const
{
	int midPoint = getWidth() / 2;
	Position relativePosition;

	_camera->convertMapToScreen(pos, &relativePosition);
	// cap the position to the screen edges relative to the center,
	// negative values indicating a left-shift, and positive values shifting to the right.
	relativePosition.x = Clamp((relativePosition.x + _camera->getMapOffset().x) - midPoint, -midPoint, midPoint);

	// convert the relative distance to a relative increment of an 80 degree angle
	// we use +- 80 instead of +- 90, so as not to go ALL the way left or right
	// which would effectively mute the sound out of one speaker.
	// since Mix_SetPosition uses modulo 360, we can't feed it a negative number, so add 360 instead.
	return 360 + (relativePosition.x / (midPoint / 80.0));
}

/**
 * Reset the camera smoothing bool.
 */
void Map::resetCameraSmoothing()
{
	_smoothingEngaged = false;
}

/**
 * Set the "explosion flash" bool.
 * @param flash should the screen be rendered in EGA this frame?
 */
void Map::setBlastFlash(bool flash)
{
	_flashScreen = flash;

	// Meridian: no frikin flashing!!
	_flashScreen = false;
}

/**
 * Checks if the screen is still being rendered in EGA.
 * @return if we are still in EGA mode.
 */
bool Map::getBlastFlash() const
{
	return _flashScreen;
}

/**
 * Resets obstacle markers.
 */
void Map::resetObstacles(void)
{
	for (int z = 0; z < _save->getMapSizeZ(); z++)
		for (int y = 0; y < _save->getMapSizeY(); y++)
			for (int x = 0; x < _save->getMapSizeX(); x++)
			{
				Tile *tile = _save->getTile(Position(x, y, z));
				if (tile) tile->resetObstacle();
			}
	_showObstacles = false;
}

/**
 * Enables obstacle markers.
 */
void Map::enableObstacles(void)
{
	_showObstacles = true;
	if (_obstacleTimer)
	{
		_obstacleTimer->stop();
		_obstacleTimer->start();
	}
}

/**
 * Disables obstacle markers.
 */
void Map::disableObstacles(void)
{
	_showObstacles = false;
	if (_obstacleTimer)
	{
		_obstacleTimer->stop();
	}
}

}
