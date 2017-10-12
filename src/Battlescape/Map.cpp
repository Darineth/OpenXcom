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
#include <fstream>
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
#include "../Mod/RuleStartingCondition.h"
#include "BattlescapeMessage.h"
#include "../Savegame/SavedGame.h"
#include "../Interface/NumberText.h"
#include "../Interface/Text.h"
#include "../fmath.h"
#include "../Savegame/Role.h"
#include "../Mod/RuleRole.h"

 /*
   1) Map origin is top corner.
   2) X axis goes downright. (width of the map)
   3) Y axis goes downleft. (length of the map
   4) Z axis goes up (height of the map)

			0,0
			 /\
		 y+ /  \ x+
			\  /
			 \/
  */

  //#define DEBUG_FOV

namespace OpenXcom
{

/**
 * Sets up a map with the specified size and position.
 * @param game Pointer to the core game.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 * @param visibleMapHeight Current visible map height.
 */
Map::Map(Game *game, int width, int height, int x, int y, int visibleMapHeight) : InteractiveSurface(width, height, x, y), _game(game), _arrow(0), _selectorX(0), _selectorY(0), _mouseX(0), _mouseY(0), _cursorType(CT_NORMAL), _cursorSize(1), _animFrame(0), _projectiles(10), _projectileInFOV(false), _explosionInFOV(false), _launch(false), _visibleMapHeight(visibleMapHeight), _unitDying(false), _smoothingEngaged(false), _flashScreen(false), _bgColor(15), _targetingTarget(-1, -1, -1), _targetingProjectile(0)
{
	_iconHeight = _game->getMod()->getInterface("battlescape")->getElement("icons")->h;
	_iconWidth = _game->getMod()->getInterface("battlescape")->getElement("icons")->w;
	_messageColor = _game->getMod()->getInterface("battlescape")->getElement("messageWindows")->color;

	_namePlayerColor = _game->getMod()->getInterface("battlescape")->getElement("unitName")->color;
	_nameNeutralColor = _game->getMod()->getInterface("battlescape")->getElement("unitName")->color2;
	_nameHostileColor = _game->getMod()->getInterface("battlescape")->getElement("unitName")->color3;

	_previewSetting = Options::battleNewPreviewPath;
	_smoothCamera = Options::battleSmoothCamera;
	if (Options::traceAI)
	{
		// turn everything on because we want to see the markers.
		_previewSetting = PATH_FULL;
	}
	_save = _game->getSavedGame()->getSavedBattle();
	if ((int)(_game->getMod()->getLUTs()->size()) > _save->getDepth())
	{
		_transparencies = &_game->getMod()->getLUTs()->at(_save->getDepth());
	}

	_spriteWidth = _game->getMod()->getSurfaceSet("BLANKS.PCK")->getFrame(0)->getWidth();
	_spriteHeight = _game->getMod()->getSurfaceSet("BLANKS.PCK")->getFrame(0)->getHeight();
	_message = new BattlescapeMessage(320, (visibleMapHeight < 200) ? visibleMapHeight : 200, 0, 0);
	_message->setX(_game->getScreen()->getDX());
	_message->setY((visibleMapHeight - _message->getHeight()) / 2);
	_message->setTextColor(_messageColor);
	_camera = new Camera(_spriteWidth, _spriteHeight, _save->getMapSizeX(), _save->getMapSizeY(), _save->getMapSizeZ(), this, visibleMapHeight);
	_scrollMouseTimer = new Timer(SCROLL_INTERVAL);
	_scrollMouseTimer->onTimer((SurfaceHandler)&Map::scrollMouse);
	_scrollKeyTimer = new Timer(SCROLL_INTERVAL);
	_scrollKeyTimer->onTimer((SurfaceHandler)&Map::scrollKey);
	_camera->setScrollTimer(_scrollMouseTimer, _scrollKeyTimer);

	_txtAccuracy = new Text(150, 20, 0, 0);
	_txtAccuracy->setSmall();
	_txtAccuracy->setPalette(_game->getScreen()->getPalette());
	_txtAccuracy->setHighContrast(true);
	_txtAccuracy->initText(_game->getMod()->getFont("FONT_BIG"), _game->getMod()->getFont("FONT_SMALL"), _game->getLanguage());
	_txtAccuracy->setAlign(ALIGN_CENTER);
	_activeWeaponUfopediaArticleUnlocked = -1;

	_nightVisionOn = false;
	_fadeShade = 16;
	_nvColor = 0;
	_fadeTimer = new Timer(FADE_INTERVAL);
	if ((_save->getGlobalShade() > NIGHT_VISION_THRESHOLD))
	{
		_fadeTimer->onTimer((SurfaceHandler)&Map::fadeShade);
		_fadeTimer->start();
	}

	const RuleStartingCondition *startingCondition = _game->getMod()->getStartingCondition(_save->getStartingConditionType());
	if (startingCondition != 0)
	{
		_bgColor = startingCondition->getMapBackgroundColor();
	}

	_stunIndicator = _game->getMod()->getSurface("FloorStunIndicator", false);
	_woundIndicator = _game->getMod()->getSurface("FloorWoundIndicator", false);
	_burnIndicator = _game->getMod()->getSurface("FloorBurnIndicator", false);
}

/**
 * Deletes the map.
 */
Map::~Map()
{
	deleteProjectiles();
	delete _scrollMouseTimer;
	delete _scrollKeyTimer;
	delete _fadeTimer;
	delete _arrow;
	delete _message;
	delete _camera;
	delete _txtAccuracy;
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
	for (int y = 0; y < 9; ++y)
		for (int x = 0; x < 9; ++x)
			_arrow->setPixel(x, y, pixels[x + (y * 9)]);
	_arrow->unlock();

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
	clear(Palette::blockOffset(0) + _bgColor);

	Tile *t;

	_projectileInFOV = _save->getDebugMode();
	if (_projectiles.size())
	{
		for (std::vector<Projectile*>::const_iterator ii = _projectiles.begin(); ii != _projectiles.end(); ++ii)
		{
			Projectile *p = *ii;
			t = _save->getTile(Position(p->getPosition(0).x / 16, p->getPosition(0).y / 16, p->getPosition(0).z / 24));
			if (_save->getSide() == FACTION_PLAYER || (t && t->getVisibleCount()))
			{
				_projectileInFOV = true;
				break;
			}
		}
	}
	_explosionInFOV = _save->getDebugMode();
	if (!_explosions.empty())
	{
		for (std::list<Explosion*>::iterator i = _explosions.begin(); i != _explosions.end(); ++i)
		{
			t = _save->getTile(Position((*i)->getPosition().x / 16, (*i)->getPosition().y / 16, (*i)->getPosition().z / 24));
			if (t && ((*i)->isBig() || t->getVisibleCount()))
			{
				_explosionInFOV = true;
				break;
			}
		}
	}

	if ((_save->getSelectedUnit() && _save->getSelectedUnit()->getVisible()) || _unitDying || _save->getSelectedUnit() == 0 || _save->getDebugMode() || _projectileInFOV || _explosionInFOV)
	{
		drawTerrain(this);
	}
	else
	{
		_message->blit(this);
	}
}

/**
 * Replaces a certain amount of colors in the surface's palette.
 * @param colors Pointer to the set of colors.
 * @param firstcolor Offset of the first color to replace.
 * @param ncolors Amount of colors to replace.
 */
void Map::setPalette(SDL_Color *colors, int firstcolor, int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	for (std::vector<MapDataSet*>::const_iterator i = _save->getMapDataSets()->begin(); i != _save->getMapDataSets()->end(); ++i)
	{
		(*i)->getSurfaceset()->setPalette(colors, firstcolor, ncolors);
	}
	_message->setPalette(colors, firstcolor, ncolors);
	_message->setBackground(_game->getMod()->getSurface(_save->getHiddenMovementBackground()));
	_message->initText(_game->getMod()->getFont("FONT_BIG"), _game->getMod()->getFont("FONT_SMALL"), _game->getLanguage());
	_message->setText(_game->getLanguage()->getString("STR_HIDDEN_MOVEMENT"));
}

/**
 * Get shade of wall.
 * @param part For what wall do calculations.
 * @param tileFrot Tile of wall.
 * @param tileBehind Tile behind wall.
 * @return Current shade of wall.
 */
int Map::getWallShade(MapDataType part, Tile* tileFrot, Tile* tileBehind)
{
	int shade;
	if (tileFrot->isDiscovered(2))
	{
		shade = reShade(tileFrot);
	}
	else
	{
		shade = 16;
	}
	if (part)
	{
		auto data = tileFrot->getMapData(part);
		if ((data->isDoor() || data->isUFODoor()) && tileFrot->isDiscovered(part - 1))
		{
			shade = std::min(reShade(tileFrot), tileBehind ? tileBehind->getShade() + 5 : 16);
		}
	}
	return shade;
}

/**
 * Draw the terrain.
 * Keep this function as optimised as possible. It's big to minimise overhead of function calls.
 * @param surface The surface to draw on.
 */
void Map::drawTerrain(Surface *surface)
{
	bool isAltPressed = (SDL_GetModState() & KMOD_ALT) != 0;
	int frameNumber = 0;
	Surface *tmpSurface;
	Tile *tile;
	int beginX = 0, endX = _save->getMapSizeX() - 1;
	int beginY = 0, endY = _save->getMapSizeY() - 1;
	int beginZ = 0, endZ = _camera->getShowAllLayers() ? _save->getMapSizeZ() - 1 : _camera->getViewLevel();
	Position mapPosition, screenPosition, bulletPositionScreen;
	int bulletLowX = 16000, bulletLowY = 16000, bulletLowZ = 16000, bulletHighX = 0, bulletHighY = 0, bulletHighZ = 0;
	int dummy;
	BattleUnit *unit = 0;
	int tileShade, wallShade, tileColor;
	UnitSprite unitSprite(surface, _game->getMod(), _animFrame, _save->getDepth() != 0);
	ItemSprite itemSprite(surface, _game->getMod(), _animFrame);

	const int halfAnimFrame = (_animFrame / 2) % 4;
	const int halfAnimFrameRest = (_animFrame % 2);

	static const int arrowBob[8] = { 0,1,2,1,0,1,2,1 };
	static const int Pulsate[8] = { 0, 1, 2, 3, 4, 3, 2, 1 };

	NumberText *_numWaypid = 0;
	BattleAction *currentAction = _save->getBattleGame() ? _save->getBattleGame()->getCurrentAction() : 0;
	TileEngine *tileEngine = _save->getTileEngine();
	//const int stealthShade = 8;

	double calculatedAccuracy = -1.0;

	Position cursorPos;
	getSelectorPosition(&cursorPos);
	// if we got bullet, get the highest x and y tiles to draw it on
	if (_projectiles.size() /*&& _explosions.empty()*/)
	{
		Position posAvg;
		int avgCount = 0;
		for (std::vector<Projectile*>::const_iterator ii = _projectiles.begin(); ii != _projectiles.end(); ++ii)
		{
			Projectile *pp = *ii;
			//int part = pp->getItem() ? 0 : BULLET_SPRITES-1;

			int begin = 0;
			int sprites = std::min(pp->getSprites(), MAX_CAMERA_BULLET_SPRITES);
			if (sprites < 0) { sprites = BULLET_SPRITES; }
			int end = sprites;
			int direction = 1;

			if (pp->isReversed())
			{
				begin = end - 1;
				end = -1;
				direction = -1;
			}

			posAvg += pp->getPosition();
			avgCount += 1;

			for (int i = begin; i != end; i += direction)
			{
				Projectile *pp = *ii;
				int part = pp->getItem() ? 0 : sprites - 1;

				posAvg += pp->getPosition();
				avgCount += 1;

				for (int i = 0; i <= part; ++i)
				{
					Position pos = pp->getPosition(1 - i);
					if (pos.x < bulletLowX) bulletLowX = pos.x;
					if (pos.y < bulletLowY) bulletLowY = pos.y;
					if (pos.z < bulletLowZ) bulletLowZ = pos.z;
					if (pos.x > bulletHighX) bulletHighX = pos.x;
					if (pos.y > bulletHighY) bulletHighY = pos.y;
					if (pos.z > bulletHighZ) bulletHighZ = pos.z;
				}
			}
		}

		if (avgCount > 0)
		{
			posAvg = posAvg / avgCount;
		}

		// divide by 16 to go from voxel to tile position
		bulletLowX = bulletLowX / 16;
		bulletLowY = bulletLowY / 16;
		bulletLowZ = bulletLowZ / 24;
		bulletHighX = bulletHighX / 16;
		bulletHighY = bulletHighY / 16;
		bulletHighZ = bulletHighZ / 24;

		// if the projectile is outside the viewport - center it back on it
		_camera->convertVoxelToScreen(posAvg, &bulletPositionScreen);

		if (_projectileInFOV)
		{
			Position newCam = _camera->getMapOffset();
			if (newCam.z != bulletHighZ) //switch level
			{
				newCam.z = bulletHighZ;
				if (_projectileInFOV)
				{
					_camera->setMapOffset(newCam);
					_camera->convertVoxelToScreen(posAvg, &bulletPositionScreen);
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
						_camera->centerOnPosition(Position(bulletLowX, bulletLowY, bulletHighZ), false);
						_camera->convertVoxelToScreen(posAvg, &bulletPositionScreen);
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
					_camera->convertVoxelToScreen(posAvg, &bulletPositionScreen);
				} while (!enough);
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

	bool pathfinderTurnedOn = _save->getPathfinding()->isPathPreviewed();
	bool hiddenUnitsKnown = false;

	if (!_waypoints.empty() || (pathfinderTurnedOn && (_previewSetting & PATH_TU_COST)))
	{
		_numWaypid = new NumberText(15, 15, 20, 30);
		_numWaypid->setPalette(getPalette());
		_numWaypid->setColor(pathfinderTurnedOn ? _messageColor + 1 : Palette::blockOffset(1));
	}

	if (_projectiles.size())
	{
		for (std::vector<Projectile*>::const_iterator ii = _projectiles.begin(); ii != _projectiles.end(); ++ii)
		{
			Projectile *pp = *ii;
			if (pp->getItem())
			{
				//tmpSurface = pp->getSprite();
				//tmpSuface = 0;
				BattleItem *item = pp->getItem();

				tmpSurface = item->getFloorSprite(_game->getMod()->getSurfaceSet("FLOOROB.PCK"));

				Position pos = pp->getPosition();

				int posX = pos.x / 16;
				int posY = pos.y / 16;

				if (posX >= beginX && posX <= endX &&
					posY >= beginY && posY <= endY)
				{
					Position shadowPos = pos;
					// draw shadow on the floor
					shadowPos.z = tileEngine->castedShade(shadowPos);

					if (tileEngine->isVoxelVisible(shadowPos))
					{
						Position tilePos(posX, posY, shadowPos.z / 24);
						if (tile = _save->getTile(tilePos))
						{
							_camera->convertVoxelToScreen(shadowPos, &bulletPositionScreen);
							tile->getDrawables().push_back(new TileDrawable(tmpSurface, bulletPositionScreen.x - 16, bulletPositionScreen.y - 26, 16));
						}
					}

					if (tileEngine->isVoxelVisible(pos))
					{
						Position tilePos(posX, posY, pos.z / 24);
						if (tile = _save->getTile(tilePos))
						{
							_camera->convertVoxelToScreen(pos, &bulletPositionScreen);
							tile->getDrawables().push_back(new TileDrawable(tmpSurface, bulletPositionScreen.x - 16, bulletPositionScreen.y - 26, 0));
						}
					}
				}
			}
			else
			{
				int begin = 0;
				int end = pp->getSprites();
				if (end < 0) { end = BULLET_SPRITES; }
				int direction = 1;
				double particleScale = 1.0;

				if (end > BULLET_SPRITES)
				{
					particleScale = (double)BULLET_SPRITES / end;
				}

				if (pp->isReversed())
				{
					begin = end - 1;
					end = -1;
					direction = -1;
				}

				for (int i = begin; i != end; i += direction)
				{
					tmpSurface = _projectileSet->getFrame(pp->getParticle((int)floor(i * particleScale)));
					if (tmpSurface)
					{
						Position pos = pp->getPosition(1 - i);

						int posX = pos.x / 16;
						int posY = pos.y / 16;

						// TODO: Different height stuff for thrown thing.

						if (posX >= beginX && posX <= endX &&
							posY >= beginY && posY <= endY)
						{
							Position shadowPos = pos;
							// draw shadow on the floor
							shadowPos.z = tileEngine->castedShade(shadowPos);
							if (tileEngine->isVoxelVisible(shadowPos))
							{
								Position tilePos(posX, posY, shadowPos.z / 24);
								if (tile = _save->getTile(tilePos))
								{
									_camera->convertVoxelToScreen(shadowPos, &bulletPositionScreen);
									bulletPositionScreen.x -= tmpSurface->getWidth() / 2;
									bulletPositionScreen.y -= tmpSurface->getHeight() / 2;
									//tmpSurface->blitNShade(surface, bulletPositionScreen.x, bulletPositionScreen.y, 16);
									tile->getDrawables().push_back(new TileDrawable(tmpSurface, bulletPositionScreen.x, bulletPositionScreen.y, 16));
								}
							}

							// draw bullet itself
							if (tileEngine->isVoxelVisible(pos))
							{
								Position tilePos(posX, posY, pos.z / 24);
								if (tile = _save->getTile(tilePos))
								{
									_camera->convertVoxelToScreen(pos, &bulletPositionScreen);
									bulletPositionScreen.x -= tmpSurface->getWidth() / 2;
									bulletPositionScreen.y -= tmpSurface->getHeight() / 2;
									//tmpSurface->blitNShade(surface, bulletPositionScreen.x, bulletPositionScreen.y, 0);
									tile->getDrawables().push_back(new TileDrawable(tmpSurface, bulletPositionScreen.x, bulletPositionScreen.y, 0));
								}
							}
						}
					}
				}
			}
		}
	}
	else if (currentAction && currentAction->actor && (_cursorType == CT_AIM || _cursorType == CT_THROW))
	{
		if (_targetingTarget != cursorPos)
		{
			_targetingTarget = cursorPos;
			Tile *targetTile = _save->getTile(_targetingTarget);
			currentAction->target = cursorPos;

			if (targetTile)
			{
				Position targetVoxel;
				tileEngine->getTargetVoxel(currentAction, _targetingTarget, targetVoxel);

				switch (currentAction->type)
				{
					case BA_THROW:
					case BA_SNAPSHOT:
					case BA_AIMEDSHOT:
					case BA_AUTOSHOT:
					case BA_BURSTSHOT:
					case BA_MINDBLAST:
					case BA_DUALFIRE:
						if (_targetingProjectile)
						{
							delete _targetingProjectile;
						}
						_targetingProjectile = new Projectile(_game->getMod(), _save, *currentAction, currentAction->actor->getPosition(), targetVoxel, 0, false);
						break;
					default:
						if (_targetingProjectile)
						{
							_targetingTarget = Position(-1, -1, -1);
							_targetingProjectile = 0;
							delete _targetingProjectile;
						}
				}

				if (_targetingProjectile)
				{
					int hit = V_OUTOFBOUNDS;
					if (currentAction->type == BA_THROW || (currentAction->weapon && currentAction->weapon->getRules()->getArcingShot()))
					{
						hit = _targetingProjectile->calculateThrow(1.0, true);
					}
					else
					{
						hit = _targetingProjectile->calculateTrajectoryFromVector(true, true);
					}
					if (hit == V_OUTOFBOUNDS)
					{
						delete _targetingProjectile;
						_targetingProjectile = 0;
					}
				}
			}
		}

		if (_targetingProjectile)
		{
			calculatedAccuracy = _targetingProjectile->getCalculatedAccuracy();

			Surface *surfaceTracer = _game->getMod()->getSurfaceSet("Projectiles")->getFrame(35);
			Surface *surfaceImpact = _game->getMod()->getSurfaceSet("Projectiles")->getFrame(280);

			/*int offsetX = 0;
			int offsetY = 0;

			if(_targetingProjectile->getItem())
			{
				Surface *floorSprite = _game->getMod()->getSurfaceSet("FLOOROB.PCK")->getFrame(_targetingProjectile->getItem()->getRules()->getFloorSprite());
				if(floorSprite)
				{
					offsetX = floorSprite->getWidth() / 2;
					offsetY = floorSprite->getHeight() / 2;
				}
			}*/

			if (surface)
			{
				_targetingProjectile->resetTrajectory();
				bool impact = false;
				do
				{
					Position pos = _targetingProjectile->getPosition();
					int posX = pos.x / 16;
					int posY = pos.y / 16;
					int posZ = pos.z / 24;

					if (posX >= beginX && posX <= endX
						&& posY >= beginY && posY <= endY
						&& tileEngine->isVoxelVisible(pos))
					{
						int offsetY = 0;
						if (posZ > endZ)
						{
							offsetY -= 26 * (posZ - endZ);
							posZ = endZ;
						}

						Position tilePos(posX, posY, posZ);

						Tile *displayTile = _save->getTile(tilePos);

						if (displayTile)
						{
							_camera->convertVoxelToScreen(pos, &bulletPositionScreen);
							displayTile->getDrawables().push_back(new TileDrawable(impact ? surfaceImpact : surfaceTracer, bulletPositionScreen.x, bulletPositionScreen.y, 0, 0, true));
						}
					}
				} while (_targetingProjectile->move() || ((!impact) && (impact = true)));
			}
		}
	}
	else
	{
		if (_targetingProjectile)
		{
			_targetingTarget = Position(-1, -1, -1);
			_targetingProjectile = 0;
			delete _targetingProjectile;
		}
	}

	Position selectorPosition;
	getSelectorPosition(&selectorPosition);

	int overwatchRadius = -1;
	if (currentAction->type == BA_OVERWATCH && currentAction->weapon)
	{
		overwatchRadius = currentAction->weapon->getRules()->getOverwatchRadius();
	}

	int unitOverwatchRadius = -1;
	Position unitOverwatchPosition;

	BattleUnit *selectedUnit = _save->getSelectedUnit();
	if (selectedUnit && selectedUnit->onOverwatch())
	{
		unitOverwatchRadius = selectedUnit->getOverwatchWeapon()->getRules()->getOverwatchRadius();
		unitOverwatchPosition = selectedUnit->getOverwatchTarget();
	}

	std::vector<BattleUnit*> *allUnits = _save->getUnits();

	Surface *groundBox = _game->getMod()->getSurfaceSet("Pathfinding")->getFrame(11);
	Surface *groundX = _game->getMod()->getSurfaceSet("Pathfinding")->getFrame(10);
	Surface *groundXHidden = _game->getMod()->getSurfaceSet("Pathfinding")->getFrame(22);
	Surface *mindControl = _game->getMod()->getSurface("IconMindControl");
	BattleUnit *selected = _save->getSelectedUnit();

	static Position tileOffset = Position(16, -8, 0);

	std::vector<BattleUnit*> *units = _save->getUnits();

	Tile *cursorTile = _save->getTile(cursorPos);
	BattleUnit *cursorUnit = cursorTile ? cursorTile->getUnit() : 0;

	surface->lock();
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
					screenPosition.y > -_spriteHeight && screenPosition.y < surface->getHeight() + _spriteHeight)
				{
					tile = _save->getTile(mapPosition);
					Tile *tileNorth = _save->getTile(mapPosition - Position(0, 1, 0));
					Tile *tileWest = _save->getTile(mapPosition - Position(1, 0, 0));

					if (!tile) continue;

					hiddenUnitsKnown = hiddenUnitsKnown || tile->getKnownHiddenUnit();

					bool isMouseTile = mapPosition == cursorPos;

					bool visible = tile->getVisibleCount();
					if (tile->isDiscovered(2))
					{
						tileShade = reShade(tile);
						if (!visible)
						{
							tileShade = 5 * tileShade / 8 + 6;
							//tileShade = 15;
						}
					}
					else
					{
						tileShade = 16;
						unit = 0;
					}

					tileColor = tile->getMarkerColor();

#ifdef DEBUG_FOV
					bool debugTileVisible = _save->getSelectedUnit() && _save->getSelectedUnit()->canSeeTile(tile);
					bool debugHoverUnitTileVisible = cursorUnit && cursorUnit->canSeeTile(tile);
					UnitFaction debugTileHiddenUnit = tile->getUnit() ? tile->getUnit()->getFaction() : FACTION_PLAYER;
#endif

					// Draw floor
					tmpSurface = tile->getSprite(O_FLOOR);
					if (tmpSurface)
						tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y - tile->getMapData(O_FLOOR)->getYOffset(), tileShade, false, _nvColor);
					unit = tile->getUnit();

					/*if(_save->getDebugMode())
					{
						if(unit && unit->getFaction() != FACTION_PLAYER)
						{
							if(unit->getFaction() == FACTION_HOSTILE)
							{
								tile->getDrawables().push_back(new TileDrawable(groundX, screenPosition.x, screenPosition.y+2, 0, 3, true));
							}
							else
							{
								tile->getDrawables().push_back(new TileDrawable(groundX, screenPosition.x, screenPosition.y+2, 0, 5, true));
							}
						}

						for(auto ii = allUnits->begin(); ii != allUnits->end(); ++ii)
						{
							BattleUnit *uu = *ii;

							if(uu->getFaction() == FACTION_HOSTILE && uu->tileInFOV(tile))
							{
								tile->getDrawables().push_back(new TileDrawable(groundBox, screenPosition.x, screenPosition.y+2, 0, 3, true));
								break;
							}
						}

						if(unit && unit->getFaction() == FACTION_PLAYER)
						{
							for(auto ii = allUnits->begin(); ii != allUnits->end(); ++ii)
							{
								BattleUnit *uu = *ii;

								if(uu->getFaction() == FACTION_HOSTILE && uu->checkSquadSight(_save, unit, false))
								{
									tile->getDrawables().push_back(new TileDrawable(groundBox, screenPosition.x, screenPosition.y+2, 0, 2, true));
									break;
								}
							}
						}
					}*/

					// Draw cursor back
					if (!_projectiles.size() && _cursorType != CT_NONE && _selectorX > itX - _cursorSize && _selectorY > itY - _cursorSize && _selectorX < itX + 1 && _selectorY < itY + 1 && !_save->getBattleState()->getMouseOverIcons())
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
							tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y, 0);
						}
						else if (_camera->getViewLevel() > itZ)
						{
							frameNumber = 2; // blue box
							tmpSurface = _game->getMod()->getSurfaceSet("CURSOR.PCK")->getFrame(frameNumber);
							tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y, 0);
						}
					}

					// special handling for a moving unit.
					if (tileNorth)
					{
						BattleUnit *bu = tileNorth->getUnit();
						int tileNorthShade, tileTwoNorthShade, tileWestShade, tileNorthWestShade, tileSouthWestShade;
						if (tileNorth->isDiscovered(2))
						{
							tileNorthShade = reShade(tileNorth);
						}
						else
						{
							tileNorthShade = 16;
							bu = 0;
						}

						/*
						 * Phase I: rerender the unit to make sure they don't get drawn over any walls or under any tiles
						 */
						if (bu && bu->getVisible() && bu->getStatus() == STATUS_WALKING && tile->getTerrainLevel() >= tileNorth->getTerrainLevel())
						{
							Position tileOffset = Position(16, -8, 0);
							// the part is 0 for small units, large units have parts 1,2 & 3 depending on the relative x/y position of this tile vs the actual unit position.
							int part = 0;
							part += tileNorth->getPosition().x - bu->getPosition().x;
							part += (tileNorth->getPosition().y - bu->getPosition().y) * 2;
							Position offset;
							calculateWalkingOffset(bu, &offset);
							offset += screenPosition;
							offset += tileOffset;

							unitSprite.draw(
								bu, part,
								offset.x,
								offset.y,
								tileNorthShade
							);

							/*
							 * Phase II: rerender any east wall type objects in the tile to the north of the unit
							 * only applies to movement in the north/south direction.
							 */
							if ((bu->getDirection() == 0 || bu->getDirection() == 4) && mapPosition.y >= 2)
							{
								Tile *tileTwoNorth = _save->getTile(mapPosition - Position(0, 2, 0));
								if (tileTwoNorth->isDiscovered(2))
								{
									tileTwoNorthShade = reShade(tileTwoNorth);
								}
								else
								{
									tileTwoNorthShade = 16;
								}
								tmpSurface = tileTwoNorth->getSprite(O_OBJECT);
								if (tmpSurface && tileTwoNorth->getMapData(O_OBJECT)->getBigWall() == 6)
								{
									tmpSurface->blitNShade(surface, screenPosition.x + tileOffset.x * 2, screenPosition.y - tileTwoNorth->getMapData(O_OBJECT)->getYOffset() + tileOffset.y * 2, tileTwoNorthShade, false, _nvColor);
								}
							}

							/*
							 * Phase III: render any south wall type objects in the tile to the northWest
							 */
							if (mapPosition.x > 0)
							{
								Tile *tileNorthWest = _save->getTile(mapPosition - Position(1, 1, 0));
								if (tileNorthWest->isDiscovered(2))
								{
									tileNorthWestShade = reShade(tileNorthWest);
								}
								else
								{
									tileNorthWestShade = 16;
								}
								tmpSurface = tileNorthWest->getSprite(O_OBJECT);
								if (tmpSurface && tileNorthWest->getMapData(O_OBJECT)->getBigWall() == 7)
								{
									tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y - tileNorthWest->getMapData(O_OBJECT)->getYOffset() + tileOffset.y * 2, tileNorthWestShade, false, _nvColor);
								}
							}

							/*
							 * Phase IV: render any south or east wall type objects in the tile to the north
							 */
							if (tileNorth->getMapData(O_OBJECT) && tileNorth->getMapData(O_OBJECT)->getBigWall() >= 6 && tileNorth->getMapData(O_OBJECT)->getBigWall() != 9)
							{
								tmpSurface = tileNorth->getSprite(O_OBJECT);
								if (tmpSurface)
									tmpSurface->blitNShade(surface, screenPosition.x + tileOffset.x, screenPosition.y - tileNorth->getMapData(O_OBJECT)->getYOffset() + tileOffset.y, tileNorthShade, false, _nvColor);
							}
							if (tileWest)
							{
								/*
								 * Phase V: re-render objects in the tile to the south west
								 * only render half so it won't overlap other areas that are already drawn
								 * and only apply this to movement in a north easterly or south westerly direction.
								 */
								if ((bu->getDirection() == 1 || bu->getDirection() == 5) && mapPosition.y < endY - 1)
								{
									Tile *tileSouthWest = _save->getTile(mapPosition + Position(-1, 1, 0));
									if (tileSouthWest->isDiscovered(2))
									{
										tileSouthWestShade = reShade(tileSouthWest);
									}
									else
									{
										tileSouthWestShade = 16;
									}
									tmpSurface = tileSouthWest->getSprite(O_OBJECT);
									if (tmpSurface)
									{
										tmpSurface->blitNShade(surface, screenPosition.x - tileOffset.x * 2, screenPosition.y - tileSouthWest->getMapData(O_OBJECT)->getYOffset(), tileSouthWestShade, true, _nvColor);
									}
								}

								/*
								 * Phase VI: we need to re-render everything in the tile to the west.
								 */
								BattleUnit *westUnit = tileWest->getUnit();
								if (tileWest->isDiscovered(2))
								{
									tileWestShade = reShade(tileWest);
								}
								else
								{
									tileWestShade = 16;
									westUnit = 0;
								}
								tmpSurface = tileWest->getSprite(O_WESTWALL);
								if (tmpSurface && bu != unit)
								{
									Tile *tileWestWest = _save->getTile(mapPosition - Position(2, 0, 0));
									wallShade = getWallShade(O_WESTWALL, tileWest, tileWestWest);
									tmpSurface->blitNShade(surface, screenPosition.x - tileOffset.x, screenPosition.y - tileWest->getMapData(O_WESTWALL)->getYOffset() + tileOffset.y, wallShade, true, _nvColor);
								}
								tmpSurface = tileWest->getSprite(O_NORTHWALL);
								if (tmpSurface)
								{
									Tile *tileWestNorth = _save->getTile(mapPosition - Position(1, 1, 0));
									wallShade = getWallShade(O_NORTHWALL, tileWest, tileWestNorth);
									tmpSurface->blitNShade(surface, screenPosition.x - tileOffset.x, screenPosition.y - tileWest->getMapData(O_NORTHWALL)->getYOffset() + tileOffset.y, wallShade, true, _nvColor);
								}
								tmpSurface = tileWest->getSprite(O_OBJECT);
								if (tmpSurface && (tileWest->getMapData(O_OBJECT)->getBigWall() < 6 || tileWest->getMapData(O_OBJECT)->getBigWall() == 9) && tileWest->getMapData(O_OBJECT)->getBigWall() != 3)
								{
									tmpSurface->blitNShade(surface, screenPosition.x - tileOffset.x, screenPosition.y - tileWest->getMapData(O_OBJECT)->getYOffset() + tileOffset.y, tileWestShade, true, _nvColor);
									// if the object in the tile to the west is a diagonal big wall, we need to cover up the black triangle at the bottom
									if (tileWest->getMapData(O_OBJECT)->getBigWall() == 2)
									{
										tmpSurface = tile->getSprite(O_FLOOR);
										if (tmpSurface)
											tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y - tile->getMapData(O_FLOOR)->getYOffset(), tileShade, false, _nvColor);
									}
								}
								// draw an item on top of the floor (if any)
								BattleItem* item = tileWest->getTopItem();
								if (item)
								{
									itemSprite.draw(item,
										screenPosition.x - tileOffset.x,
										screenPosition.y + tileWest->getTerrainLevel() + tileOffset.y,
										tileWestShade,
										true
									);
									if (_stunIndicator || _woundIndicator || _burnIndicator)
									{
										BattleUnit *itemUnit = item->getUnit();
										if (itemUnit && itemUnit->getStatus() == STATUS_UNCONSCIOUS)
										{
											if (_burnIndicator && itemUnit->getFire() > 0)
											{
												_burnIndicator->blitNShade(surface,
													screenPosition.x - tileOffset.x,
													screenPosition.y + tileWest->getTerrainLevel() + tileOffset.y,
													Pulsate[_animFrame % 8],
													true);
											}
											else if (_woundIndicator && itemUnit->getFatalWounds() > 0)
											{
												_woundIndicator->blitNShade(surface,
													screenPosition.x - tileOffset.x,
													screenPosition.y + tileWest->getTerrainLevel() + tileOffset.y,
													Pulsate[_animFrame % 8],
													true);
											}
											else if (_stunIndicator)
											{
												_stunIndicator->blitNShade(surface,
													screenPosition.x - tileOffset.x,
													screenPosition.y + tileWest->getTerrainLevel() + tileOffset.y,
													Pulsate[_animFrame % 8],
													true);
											}
										}
									}
								}
								// Draw soldier
								if (westUnit && (!tileWest->getMapData(O_OBJECT) || tileWest->getMapData(O_OBJECT)->getBigWall() < 6 || tileWest->getMapData(O_OBJECT)->getBigWall() == 9) && (westUnit->getVisible() || _save->getDebugMode()))
								{
									// the part is 0 for small units, large units have parts 1,2 & 3 depending on the relative x/y position of this tile vs the actual unit position.
									int part = 0;
									part += tileWest->getPosition().x - westUnit->getPosition().x;
									part += (tileWest->getPosition().y - westUnit->getPosition().y) * 2;
									Position offset;
									calculateWalkingOffset(westUnit, &offset);
									unitSprite.draw(
										westUnit, part,
										screenPosition.x - tileOffset.x + offset.x,
										screenPosition.y + tileOffset.y + offset.y + getTerrainLevel(westUnit->getPosition(), westUnit->getArmor()->getSize()),
										tileWestShade,
										true
									);
								}
								// Draw smoke/fire
								if (tileWest->getSmoke() && tileWest->isDiscovered(2))
								{
									frameNumber = 0;
									int shade = 0;
									if (!tileWest->getFire())
									{
										if (_save->getDepth() > 0)
										{
											frameNumber += Mod::UNDERWATER_SMOKE_OFFSET;
										}
										else
										{
											frameNumber += Mod::SMOKE_OFFSET;
										}
										frameNumber += int(floor((tileWest->getSmoke() / 6.0) - 0.1)); // see http://www.ufopaedia.org/images/c/cb/Smoke.gif
										shade = tileWestShade;
									}

									if (halfAnimFrame + tileWest->getAnimationOffset() > 3)
									{
										frameNumber += (halfAnimFrame + tileWest->getAnimationOffset() - 4);
									}
									else
									{
										frameNumber += halfAnimFrame + tileWest->getAnimationOffset();
									}
									tmpSurface = _game->getMod()->getSurfaceSet("SMOKE.PCK")->getFrame(frameNumber);
									tmpSurface->blitNShade(surface, screenPosition.x - tileOffset.x, screenPosition.y + tileOffset.y, shade, true, _nvColor);
								}
								// Draw object
								if (tileWest->getMapData(O_OBJECT) && tileWest->getMapData(O_OBJECT)->getBigWall() >= 6 && tileWest->getMapData(O_OBJECT)->getBigWall() != 9)
								{
									tmpSurface = tileWest->getSprite(O_OBJECT);
									tmpSurface->blitNShade(surface, screenPosition.x - tileOffset.x, screenPosition.y - tileWest->getMapData(O_OBJECT)->getYOffset() + tileOffset.y, tileWestShade, true, _nvColor);
								}
							}
						}
					}

					if (unitOverwatchRadius >= 0
						&& mapPosition.z == unitOverwatchPosition.z
						&& tileEngine->distance(mapPosition, unitOverwatchPosition) <= unitOverwatchRadius)
					{
						tmpSurface = _game->getMod()->getSurfaceSet("Pathfinding")->getFrame(22);
						if (tmpSurface)
							tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y + 2, 0, false, 3);
					}

					// Draw target area for Overwatch.
					if (_cursorType != CT_NONE
						&& overwatchRadius >= 0
						&& mapPosition.z == selectorPosition.z
						&& tileEngine->distance(mapPosition, selectorPosition) <= overwatchRadius)
					{
						tmpSurface = _game->getMod()->getSurfaceSet("Pathfinding")->getFrame(22);
						if (tmpSurface)
							tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y + 2, 0, false, 2);
					}


					// Draw walls
					if (!tile->isVoid())
					{
						// Draw west wall
						tmpSurface = tile->getSprite(O_WESTWALL);
						if (tmpSurface)
						{
							wallShade = getWallShade(O_WESTWALL, tile, tileWest);
							tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y - tile->getMapData(O_WESTWALL)->getYOffset(), wallShade, false, _nvColor);
						}
						// Draw north wall
						tmpSurface = tile->getSprite(O_NORTHWALL);
						if (tmpSurface)
						{
							wallShade = getWallShade(O_NORTHWALL, tile, tileNorth);
							tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y - tile->getMapData(O_NORTHWALL)->getYOffset(), wallShade, tile->getMapData(O_WESTWALL), _nvColor);
						}
						// Draw object
						if (tile->getMapData(O_OBJECT) && (tile->getMapData(O_OBJECT)->getBigWall() < 6 || tile->getMapData(O_OBJECT)->getBigWall() == 9))
						{
							tmpSurface = tile->getSprite(O_OBJECT);
							if (tmpSurface)
								tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y - tile->getMapData(O_OBJECT)->getYOffset(), tileShade, false, _nvColor);
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
							if (_stunIndicator || _woundIndicator || _burnIndicator)
							{
								BattleUnit *itemUnit = item->getUnit();
								if (itemUnit && itemUnit->getStatus() == STATUS_UNCONSCIOUS)
								{
									if (_burnIndicator && itemUnit->getFire() > 0)
									{
										_burnIndicator->blitNShade(surface,
											screenPosition.x,
											screenPosition.y + tile->getTerrainLevel(),
											Pulsate[_animFrame % 8]);
									}
									else if (_woundIndicator && itemUnit->getFatalWounds() > 0)
									{
										_woundIndicator->blitNShade(surface,
											screenPosition.x,
											screenPosition.y + tile->getTerrainLevel(),
											Pulsate[_animFrame % 8]);
									}
									else if (_stunIndicator)
									{
										_stunIndicator->blitNShade(surface,
											screenPosition.x,
											screenPosition.y + tile->getTerrainLevel(),
											Pulsate[_animFrame % 8]);
									}
								}
							}
						}
					}

					// check if we got bullet && it is in Field Of View
					/*if (_projectiles.size())
					{
						for (Projectile *pp : _projectiles)
						{
							tmpSurface = 0;
							BattleItem* item = pp->getItem();
							if (item)
							{
								Position voxelPos = pp->getPosition();
								// draw shadow on the floor
								voxelPos.z = _save->getTileEngine()->castedShade(voxelPos);
								if (voxelPos.x / 16 >= itX &&
									voxelPos.y / 16 >= itY &&
									voxelPos.x / 16 <= itX + 1 &&
									voxelPos.y / 16 <= itY + 1 &&
									voxelPos.z / 24 == itZ &&
									_save->getTileEngine()->isVoxelVisible(voxelPos))
								{
									_camera->convertVoxelToScreen(voxelPos, &bulletPositionScreen);

									itemSprite.drawShadow(item,
										bulletPositionScreen.x - 16,
										bulletPositionScreen.y - 26
									);
								}

								voxelPos = pp->getPosition();
								// draw thrown object
								if (voxelPos.x / 16 >= itX &&
									voxelPos.y / 16 >= itY &&
									voxelPos.x / 16 <= itX + 1 &&
									voxelPos.y / 16 <= itY + 1 &&
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
									int end = pp->getSprites();
									if (end < 0) { end = BULLET_SPRITES; }
									int direction = 1;
									double particleScale = 1.0;

									if (end > BULLET_SPRITES)
									{
										particleScale = (double)BULLET_SPRITES / end;
									}

									if (pp->isReversed())
									{
										begin = end - 1;
										end = -1;
										direction = -1;
									}

									for (int i = begin; i != end; i += direction)
									{
										int particle = i;

										tmpSurface = _projectileSet->getFrame(pp->getParticle((int)floor(i * particleScale)));
										if (tmpSurface)
										{
											Position voxelPos = pp->getPosition(1 - i);
											// draw shadow on the floor
											voxelPos.z = _save->getTileEngine()->castedShade(voxelPos);
											if (voxelPos.x / 16 == itX &&
												voxelPos.y / 16 == itY &&
												voxelPos.z / 24 == itZ &&
												_save->getTileEngine()->isVoxelVisible(voxelPos))
											{
												_camera->convertVoxelToScreen(voxelPos, &bulletPositionScreen);
												bulletPositionScreen.x -= tmpSurface->getWidth() / 2;
												bulletPositionScreen.y -= tmpSurface->getHeight() / 2;
												tmpSurface->blitNShade(surface, bulletPositionScreen.x, bulletPositionScreen.y, 16, false, _nvColor);
											}

											// draw bullet itself
											voxelPos = pp->getPosition(1 - i);
											if (voxelPos.x / 16 == itX &&
												voxelPos.y / 16 == itY &&
												voxelPos.z / 24 == itZ &&
												_save->getTileEngine()->isVoxelVisible(voxelPos))
											{
												_camera->convertVoxelToScreen(voxelPos, &bulletPositionScreen);
												bulletPositionScreen.x -= tmpSurface->getWidth() / 2;
												bulletPositionScreen.y -= tmpSurface->getHeight() / 2;
												tmpSurface->blitNShade(surface, bulletPositionScreen.x, bulletPositionScreen.y, 0, false, _nvColor);
											}
										}
									}
								}
							}
						}
					}*/

					/*if(_save->getDebugMode())
					{
						for(auto ii = allUnits->begin(); ii != allUnits->end(); ++ii)
						{
							BattleUnit *unit = *ii;
							if(unit->getFaction() == FACTION_HOSTILE && unit->tileInFOV(tile))
							{
								tmpSurface = _game->getMod()->getSurfaceSet("Pathfinding")->getFrame(22);
								if(tmpSurface)
									tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y+2, 0, false, 2);
								break;
							}
						}
					}*/

					bool topMostDrawables = false;
					std::vector<TileDrawable*> &drawables = tile->getDrawables();

					if (drawables.size())
					{
						for (std::vector<TileDrawable*>::const_iterator ii = drawables.begin(); ii != drawables.end(); ++ii)
						{
							TileDrawable *td = *ii;
							if (td->topMost)
							{
								topMostDrawables = true;
							}
							else
							{
								td->surface->blitNShade(surface, td->x, td->y, td->off, false, td->color);
							}
						}

						if (!topMostDrawables)
							tile->clearDrawables();
					}

					unit = tile->getUnit();
					// Draw soldier
					if (unit && (unit->getVisible() || _save->getDebugMode()))
					{
						if (unit->onOverwatch() && (unit->getFaction() == FACTION_PLAYER || _save->getDebugMode()))
						{
							tmpSurface = _game->getMod()->getSurfaceSet("Pathfinding")->getFrame(22);
							if (tmpSurface)
								tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y + 2, 0, false, 13);
						}

						// the part is 0 for small units, large units have parts 1,2 & 3 depending on the relative x/y position of this tile vs the actual unit position.
						int part = 0;
						part += tile->getPosition().x - unit->getPosition().x;
						part += (tile->getPosition().y - unit->getPosition().y) * 2;
						Position offset;
						calculateWalkingOffset(unit, &offset);
						offset += screenPosition;

						unitSprite.draw(
							unit, part,
							offset.x,
							offset.y,
							tileShade
						);
						if (unit->getBreathFrame() > 0)
						{
							SurfaceSet *breath = _game->getMod()->getSurfaceSet("BREATH-1.PCK", false);
							if (breath)
							{
								tmpSurface = breath->getFrame(unit->getBreathFrame() - 1);
								// we enlarge the unit sprite when aiming to accommodate the weapon. so adjust as necessary.
								if (unit->getStatus() == STATUS_AIMING)
								{
									offset.x = 0;
								}

								// lower the bubbles for shorter or kneeling units.
								offset.y += (22 - unit->getHeight());
								if (tmpSurface)
								{
									tmpSurface->blitNShade(surface, offset.x, offset.y - 30, tileShade, false, _nvColor);
								}
							}
						}
						if (isAltPressed)
						{
							// draw unit facing indicator
							tmpSurface = _game->getMod()->getSurfaceSet("DETBLOB.DAT")->getFrame(7 + ((unit->getDirection() + 1) % 8));
							tmpSurface->blitNShade(surface, offset.x, offset.y, 0);
						}

						if ((unit->getFaction() == FACTION_PLAYER || _save->getDebugMode()) && unit->getBleedingOut() && _woundIndicator)
						{
							topMostDrawables = true;
							tile->getDrawables().push_back(new TileDrawable(_woundIndicator, screenPosition.x + (_spriteWidth / 2) - (_woundIndicator->getWidth() / 2), screenPosition.y + _woundIndicator->getHeight() + arrowBob[_animFrame % 8], 0, 0, true));
							/*tmpSurface = _game->getMod()->getSurface("IconBleeding");
							if(tmpSurface)
							{
								tmpSurface->blitNShade(surface, screenPosition.x + (_spriteWidth / 2) - (tmpSurface->getWidth() / 2), screenPosition.y + tmpSurface->getHeight() + arrowBob[_animFrame % 8], 0);
							}*/
						}

						/*if(unit->getTurnsSinceSpotted() == 0)
						{
							tmpSurface = _game->getMod()->getSurfaceSet("Pathfinding")->getFrame(11);
							if(tmpSurface)
								tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y-24, 0, false, 1);
						}*/

						/*std::wostringstream ss;

						ss << Text::formatNumber(unit->getHealth()) << "/" << Text::formatNumber(-unit->getDeathHealth());

						_txtAccuracy->setText(ss.str());
						_txtAccuracy->draw();
						_txtAccuracy->blitNShade(surface, screenPosition.x, screenPosition.y, 0);*/
					}
					else if (unit && tile->getKnownHiddenUnit())
					{
						/*switch(unit->getFaction())
						{
						case FACTION_PLAYER:
							groundX->blitNShade(surface, screenPosition.x, screenPosition.y+2, 0, false, 5);
							break;
						case FACTION_NEUTRAL:
							groundX->blitNShade(surface, screenPosition.x, screenPosition.y+2, 0, false, 10);
							break;
						case FACTION_HOSTILE:
							groundX->blitNShade(surface, screenPosition.x, screenPosition.y+2, 0, false, 3);
							break;
						}*/
						groundX->blitNShade(surface, screenPosition.x, screenPosition.y + 2, 0, false, _camera->getViewLevel() == itZ ? 10 : 0);
					}
					// if we can see through the floor, draw the soldier below it if it is on stairs
					Tile *tileBelow = _save->getTile(mapPosition + Position(0, 0, -1));
					if (itZ > 0 && tile->hasNoFloor(tileBelow))
					{
						BattleUnit *tunit = _save->selectUnit(Position(itX, itY, itZ - 1));
						Tile *ttile = _save->getTile(Position(itX, itY, itZ - 1));
						if (tunit && tunit->getVisible() && ttile->getTerrainLevel() < 0 && ttile->isDiscovered(2))
						{
							// the part is 0 for small units, large units have parts 1,2 & 3 depending on the relative x/y position of this tile vs the actual unit position.
							int part = 0;
							part += ttile->getPosition().x - tunit->getPosition().x;
							part += (ttile->getPosition().y - tunit->getPosition().y) * 2;
							Position offset;
							calculateWalkingOffset(tunit, &offset);
							offset += screenPosition;
							offset += Position(0, 24, 0);

							unitSprite.draw(
								tunit, part,
								offset.x,
								offset.y,
								reShade(ttile)
							);
						}
					}

					// Draw smoke/fire
					if (tile->getSmoke() && tile->isDiscovered(2))
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
							frameNumber += int(floor((tile->getSmoke() / 6.0) - 0.1)); // see http://www.ufopaedia.org/images/c/cb/Smoke.gif
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
						tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y, shade, false, _nvColor);
					}

					//draw particle clouds
					for (std::list<Particle*>::const_iterator i = tile->getParticleCloud()->begin(); i != tile->getParticleCloud()->end(); ++i)
					{
						int vaporX = screenPosition.x + (*i)->getX();
						int vaporY = screenPosition.y + (*i)->getY();
						if ((int)(_transparencies->size()) >= ((*i)->getColor() + 1) * 1024)
						{
							switch ((*i)->getSize())
							{
								case 3:
									surface->setPixel(vaporX + 1, vaporY + 1, (*_transparencies)[((*i)->getColor() * 1024) + ((*i)->getOpacity() * 256) + surface->getPixel(vaporX + 1, vaporY + 1)]);
								case 2:
									surface->setPixel(vaporX + 1, vaporY, (*_transparencies)[((*i)->getColor() * 1024) + ((*i)->getOpacity() * 256) + surface->getPixel(vaporX + 1, vaporY)]);
								case 1:
									surface->setPixel(vaporX, vaporY + 1, (*_transparencies)[((*i)->getColor() * 1024) + ((*i)->getOpacity() * 256) + surface->getPixel(vaporX, vaporY + 1)]);
								default:
									surface->setPixel(vaporX, vaporY, (*_transparencies)[((*i)->getColor() * 1024) + ((*i)->getOpacity() * 256) + surface->getPixel(vaporX, vaporY)]);
									break;
							}
						}
					}

					// Draw Path Preview
					if (tile->getPreview() != -1 && tile->isDiscovered(0) && (_previewSetting & PATH_ARROWS))
					{
						if (itZ > 0 && tile->hasNoFloor(tileBelow))
						{
							tmpSurface = _game->getMod()->getSurfaceSet("Pathfinding")->getFrame(11);
							if (tmpSurface)
							{
								tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y + 2, 0, false, tile->getMarkerColor());
							}
						}
						tmpSurface = _game->getMod()->getSurfaceSet("Pathfinding")->getFrame(tile->getPreview());
						if (tmpSurface)
						{
							tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y + tile->getTerrainLevel(), 0, false, tileColor);
						}
					}
#ifdef DEBUG_FOV
					if (debugTileVisible && debugHoverUnitTileVisible)
					{
						groundXHidden->blitNShade(surface, screenPosition.x, screenPosition.y + 2, 0, false, 10);
					}
					else if (debugTileVisible)
					{
						groundXHidden->blitNShade(surface, screenPosition.x, screenPosition.y + 2, 0, false, 5);
					}
					else if (debugHoverUnitTileVisible)
					{
						groundXHidden->blitNShade(surface, screenPosition.x, screenPosition.y + 2, 0, false, 3);
					}

					if (debugTileHiddenUnit)
					{
						switch (debugTileHiddenUnit)
						{
							case FACTION_NEUTRAL:
								groundX->blitNShade(surface, screenPosition.x, screenPosition.y + 2, 0, false, 10);
								break;
							case FACTION_HOSTILE:
								groundX->blitNShade(surface, screenPosition.x, screenPosition.y + 2, 0, false, 3);
								break;
						}
					}
#endif
					if (!tile->isVoid())
					{
						// Draw object
						if (tile->getMapData(O_OBJECT) && tile->getMapData(O_OBJECT)->getBigWall() >= 6 && tile->getMapData(O_OBJECT)->getBigWall() != 9)
						{
							tmpSurface = tile->getSprite(O_OBJECT);
							if (tmpSurface)
								tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y - tile->getMapData(O_OBJECT)->getYOffset(), tileShade, false, _nvColor);
						}
					}
					// Draw cursor front
					if (_cursorType != CT_NONE && _selectorX > itX - _cursorSize && _selectorY > itY - _cursorSize && _selectorX < itX + 1 && _selectorY < itY + 1 && !_save->getBattleState()->getMouseOverIcons())
					{
						if (_camera->getViewLevel() == itZ)
						{
							bool showTip = false;
							std::wostringstream ss;
							Uint8 tipSecondaryColor = 0;
							if (unit && unit->getVisible() && _selectorX == itX && _selectorY == itY)
							{
								showTip = true;
								ss << L"\x01";
								switch (unit->getFaction())
								{
									case FACTION_PLAYER:
										tipSecondaryColor = _namePlayerColor;
										ss << unit->getName(_game->getLanguage());
										break;
									case FACTION_HOSTILE:
										tipSecondaryColor = _nameHostileColor;
										if (unit->getOriginalFaction() == FACTION_PLAYER || !unit->getUnitRules() || _save->getGeoscapeSave()->isResearched(unit->getUnitRules()->getType()))
										{
											ss << unit->getName(_game->getLanguage());
										}
										else if (_save->getGeoscapeSave()->isResearched(unit->getUnitRules()->getRace() + "_AUTOPSY") || _save->getGeoscapeSave()->isResearched(unit->getUnitRules()->getRace()))
										{
											ss << _game->getLanguage()->getString(unit->getUnitRules()->getRace());
										}
										else
										{
											ss << _game->getLanguage()->getString("STR_HOSTILE");
										}

										break;
									case FACTION_NEUTRAL:
										tipSecondaryColor = _nameNeutralColor;
										ss << _game->getLanguage()->getString("STR_NEUTRAL");
										break;
								}
								ss << L"\x01";
							}

							if (_cursorType != CT_AIM)
							{
								if (unit && (unit->getVisible() || _save->getDebugMode()))
									frameNumber = 3 + halfAnimFrameRest; // yellow box
								else
									frameNumber = 3; // red box
							}
							else
							{
								if (currentAction->actor && currentAction->Time && currentAction->actor->getTimeUnits() < currentAction->Time)
									frameNumber = _animFrame % 2 ? 6 : 7;
								else if (unit && (unit->getVisible() || _save->getDebugMode()))
									frameNumber = 7 + halfAnimFrame; // yellow animated crosshairs
								else
									frameNumber = 6; // red static crosshairs
							}
							tmpSurface = _game->getMod()->getSurfaceSet("CURSOR.PCK")->getFrame(frameNumber);
							tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y, 0);

							// UFO extender accuracy: display adjusted accuracy value on crosshair in real-time.
							//if ((_cursorType == CT_AIM || _cursorType == CT_PSI || _cursorType == CT_WAYPOINT) && Options::battleUFOExtenderAccuracy) // TODO: Evaluate PSI/WAYPOINT options.
							if (calculatedAccuracy >= 0 && (_cursorType == CT_AIM || _cursorType == CT_THROW) && currentAction->type != BA_MINDBLAST)
							{
								const RuleItem *weapon = currentAction->weapon->getRules();
								int distance = tileEngine->distance(mapPosition, currentAction->actor->getPosition());

								int accuracy = 0;
								int accuracyCover = 0;
								if (currentAction->type == BA_THROW)
								{
									accuracy = currentAction->actor->getThrowingAccuracy();
								}
								else if (currentAction->type == BA_HIT)
								{
									accuracy = currentAction->actor->getFiringAccuracy(currentAction->type, currentAction->weapon, _game->getMod(), false, currentAction->dualFiring);
								}
								else
								{
									double chance = 0, coverChance = 0;
									_targetingProjectile->calculateChanceToHit(chance, coverChance);
									calculatedAccuracy = chance;
									chance *= 100.0;
									coverChance *= 100.0;

									accuracy = (int)chance;
									accuracyCover = (int)coverChance;
								}

								/* REBASE: Integrate short range logic?  Old UFO extender stuff.
								bool outOfRange = distance > weapon->getMaxRange();
								// special handling for short ranges and diagonals
								if (outOfRange && action->actor->directionTo(action->target) % 2 == 1)
								{
									// special handling for maxRange 1: allow it to target diagonally adjacent tiles, even though they are technically 2 tiles away.
									if (weapon->getMaxRange() == 1
										&& distance == 2)
									{
										outOfRange = false;
									}
									// special handling for maxRange 2: allow it to target diagonally adjacent tiles on a level above/below, even though they are technically 3 tiles away.
									else if (weapon->getMaxRange() == 2
										&& distance == 3
										&& itZ != action->actor->getPosition().z)
									{
										outOfRange = false;
									}
								}
								// zero accuracy or out of range: set it red.
								if (accuracy <= 0 || outOfRange)*/
								if (currentAction->type != BA_THROW && currentAction->type != BA_HIT)
								{
									// TODO: Get accuracy colors from Interface rules.
									if (accuracy == 0)
									{
										_txtAccuracy->setColor(Palette::blockOffset(2) - 1);
									}
									else if (accuracy > 0 && accuracy < 33)
									{
										_txtAccuracy->setColor(Palette::blockOffset(1) - 1);
									}
									else if (accuracy > 33 && accuracy < 66)
									{
										_txtAccuracy->setColor(Palette::blockOffset(9) - 1);
									}
									else if (accuracy > 66 && accuracy < 95)
									{
										_txtAccuracy->setColor(Palette::blockOffset(4) - 1);
									}
									else
									{
										_txtAccuracy->setColor(Palette::blockOffset(3) - 1);
									}
								}
								else
								{
									if (calculatedAccuracy == 0.0)
									{
										accuracy = 0;
										_txtAccuracy->setColor(Palette::blockOffset(2) - 1);
									}
									else if (calculatedAccuracy > 0.0 && calculatedAccuracy < 1.0)
									{
										accuracy = (int)((double)accuracy * calculatedAccuracy);
										_txtAccuracy->setColor(Palette::blockOffset(1) - 1);
									}
									else
									{
										_txtAccuracy->setColor(Palette::blockOffset(4) - 1);
									}
								}

								showTip = true;
								ss << "\n" << accuracy << "% (-" << accuracyCover << "%) @ " << distance << "m";
							}

							if (showTip)
							{
								_txtAccuracy->setSecondaryColor(tipSecondaryColor);
								_txtAccuracy->setText(ss.str());
								_txtAccuracy->draw();
								_txtAccuracy->blitNShade(surface, screenPosition.x - ((_txtAccuracy->getWidth() - 32) / 2), screenPosition.y - _txtAccuracy->getHeight(), 0);
							}
						}
						else if (_camera->getViewLevel() > itZ)
						{
							frameNumber = 5; // blue box
							tmpSurface = _game->getMod()->getSurfaceSet("CURSOR.PCK")->getFrame(frameNumber);
							tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y, 0);
						}
						if (!isAltPressed && _cursorType > 2 && _camera->getViewLevel() == itZ)
						{
							int frame[6] = { 0, 0, 0, 11, 13, 15 };
							tmpSurface = _game->getMod()->getSurfaceSet("CURSOR.PCK")->getFrame(frame[_cursorType] + (_animFrame / 4) % 2);
							tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y, 0);
						}
					}

					// Draw waypoints if any on this tile
					int waypid = 1;
					int waypXOff = 2;
					int waypYOff = 2;

					for (std::vector<Position>::const_iterator i = _waypoints.begin(); i != _waypoints.end(); ++i)
					{
						if ((*i) == mapPosition)
						{
							if (waypXOff == 2 && waypYOff == 2)
							{
								tmpSurface = _game->getMod()->getSurfaceSet("CURSOR.PCK")->getFrame(7);
								tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y, 0);
							}
							if (_save->getBattleGame()->getCurrentAction()->type == BA_LAUNCH)
							{
								_numWaypid->setValue(waypid);
								_numWaypid->draw();
								_numWaypid->blitNShade(surface, screenPosition.x + waypXOff, screenPosition.y + waypYOff, 0);

								waypXOff += waypid > 9 ? 8 : 6;
								if (waypXOff >= 26)
								{
									waypXOff = 2;
									waypYOff += 8;
								}
							}
						}
						waypid++;
					}

					if (isMouseTile
						&& unit
						&& unit->getVisible()
						&& (selected != unit)
						&& (_save->getSide() == FACTION_PLAYER || _save->getDebugMode())
						&& (unit->getFaction() == _save->getSide())
						&& unit->getPosition().z <= _camera->getViewLevel())
					{
						_camera->convertMapToScreen(unit->getPosition(), &screenPosition);
						screenPosition += _camera->getMapOffset();
						Position offset;
						/*calculateWalkingOffset(unit, &offset);
						if (unit->getArmor()->getSize() > 1)
						{
							offset.y += 4;
						}
						offset.y += 24 - unit->getHeight();
						if (unit->isKneeled())
						{
							offset.y -= 2;
						}*/
						if (this->getCursorType() != CT_NONE)
						{
							Soldier *soldier = _game->getSavedGame()->getSoldier(unit->getId());
							Role *role;
							Surface *roleIcon;
							if (soldier && (role = soldier->getRole()) && !role->isBlank() && (roleIcon = _game->getMod()->getSurface(role->getRules()->getSmallIconSprite())))
							{
								roleIcon->blitNShade(surface,
									screenPosition.x + offset.x + (_spriteWidth / 2) - (roleIcon->getWidth() / 2),
									screenPosition.y + offset.y - roleIcon->getHeight() + -_txtAccuracy->getHeight(),
									0);
							}
							/*else
							{
								_arrow->blitNShade(surface, screenPosition.x + offset.x + (_spriteWidth / 2) - (_arrow->getWidth() / 2), screenPosition.y + offset.y - _arrow->getHeight() + arrowBob[_animFrame % 8], 0);
							}*/
						}
					}
					else if (unit
						&& unit->getVisible()
						&& selected
						&& unit != selected
						&& unit->getPosition().z <= _camera->getViewLevel()
						&& (unit->getControlledBy() == selected
							|| unit->getControlling() == selected))
					{
						_camera->convertMapToScreen(unit->getPosition(), &screenPosition);
						screenPosition += _camera->getMapOffset();
						Position offset;
						calculateWalkingOffset(unit, &offset);
						if (unit->getArmor()->getSize() > 1)
						{
							offset.y += 4;
						}
						offset.y += 24 - unit->getHeight();
						if (unit->isKneeled())
						{
							offset.y -= 2;
						}
						mindControl->blitNShade(surface, screenPosition.x + offset.x + (_spriteWidth / 2) - (_arrow->getWidth() / 2), screenPosition.y + offset.y - _arrow->getHeight() + arrowBob[_animFrame % 8], 0, false, unit->getControlledBy() == selected ? 3 : 4);
					}

					if (topMostDrawables)
					{
						for (std::vector<TileDrawable*>::const_iterator ii = drawables.begin(); ii != drawables.end(); ++ii)
						{
							TileDrawable *td = *ii;
							if (td->topMost)
							{
								td->surface->blitNShade(surface, td->x, td->y, td->off, false, td->color);
							}
						}

						tile->clearDrawables();
					}
				}
			}
		}
	}
	if (pathfinderTurnedOn || hiddenUnitsKnown)
	{
		if (pathfinderTurnedOn && _numWaypid)
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
						screenPosition.y > -_spriteHeight && screenPosition.y < surface->getHeight() + _spriteHeight)
					{
						tile = _save->getTile(mapPosition);

						BattleUnit *unit;
						/*if(tile && tile->getKnownHiddenUnit() && (unit = tile->getUnit()))
						{
							switch(unit->getFaction())
							{
							case FACTION_PLAYER:
								groundXHidden->blitNShade(surface, screenPosition.x, screenPosition.y+2, 0, false, 5);
								break;
							case FACTION_NEUTRAL:
								groundXHidden->blitNShade(surface, screenPosition.x, screenPosition.y+2, 0, false, 10);
								break;
							case FACTION_HOSTILE:
								groundXHidden->blitNShade(surface, screenPosition.x, screenPosition.y+2, 0, false, 3);
								break;
							}
						}*/
						if (tile && tile->getKnownHiddenUnit())
						{
							groundXHidden->blitNShade(surface, screenPosition.x, screenPosition.y + 2, 0, false, _camera->getViewLevel() == itZ ? 10 : 0);
						}

						if (!pathfinderTurnedOn || !tile || !tile->isDiscovered(0) || tile->getPreview() == -1)
							continue;

						Tile *tileBelow = _save->getTile(mapPosition - Position(0, 0, 1));

						int adjustment = -tile->getTerrainLevel();
						if (_previewSetting & PATH_ARROWS)
						{
							if (itZ > 0 && tile->hasNoFloor(tileBelow))
							{
								tmpSurface = _game->getMod()->getSurfaceSet("Pathfinding")->getFrame(23);
								if (tmpSurface)
								{
									tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y + 2, 0, false, tile->getMarkerColor());
								}
							}
							int overlay = tile->getPreview() + 12;
							tmpSurface = _game->getMod()->getSurfaceSet("Pathfinding")->getFrame(overlay);
							if (tmpSurface)
							{
								tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y - adjustment, 0, false, tile->getMarkerColor());
							}
						}

						if (_previewSetting & PATH_TU_COST && tile->getTUMarker() > -1)
						{
							int off = tile->getTUMarker() > 9 ? 5 : 3;
							if (_save->getSelectedUnit() && _save->getSelectedUnit()->getArmor()->getSize() > 1)
							{
								adjustment += 1;
								if (!(_previewSetting & PATH_ARROWS))
								{
									adjustment += 7;
								}
							}
							_numWaypid->setValue(tile->getTUMarker());
							_numWaypid->draw();
							if (!(_previewSetting & PATH_ARROWS))
							{
								_numWaypid->blitNShade(surface, screenPosition.x + 16 - off, screenPosition.y + (29 - adjustment), 0, false, tile->getMarkerColor());
							}
							else
							{
								_numWaypid->blitNShade(surface, screenPosition.x + 16 - off, screenPosition.y + (22 - adjustment), 0);
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

	unit = selected;
	if (unit && (_save->getSide() == FACTION_PLAYER || _save->getDebugMode()) && unit->getPosition().z <= _camera->getViewLevel())
	{
		_camera->convertMapToScreen(unit->getPosition(), &screenPosition);
		screenPosition += _camera->getMapOffset();
		Position offset;
		calculateWalkingOffset(unit, &offset);
		if (unit->getArmor()->getSize() > 1)
		{
			offset.y += 4;
		}
		offset.y += 24 - unit->getHeight();
		if (unit->isKneeled())
		{
			offset.y -= 2;
		}
		if (this->getCursorType() != CT_NONE)
		{
			Soldier *soldier = _game->getSavedGame()->getSoldier(unit->getId());
			Role *role;
			Surface *roleIcon;
			if (soldier && (role = soldier->getRole()) && !role->isBlank() && (roleIcon = _game->getMod()->getSurface(role->getRules()->getSmallIconSprite())))
			{
				roleIcon->blitNShade(surface, screenPosition.x + offset.x + (_spriteWidth / 2) - (roleIcon->getWidth() / 2), screenPosition.y + offset.y - roleIcon->getHeight() + arrowBob[_animFrame % 8], 0);
			}
			else
			{
				_arrow->blitNShade(surface, screenPosition.x + offset.x + (_spriteWidth / 2) - (_arrow->getWidth() / 2), screenPosition.y + offset.y - _arrow->getHeight() + arrowBob[_animFrame % 8], 0);
			}
		}
	}
	delete _numWaypid;

	// check if we got big explosions
	if (_explosionInFOV)
	{
		// big explosions cause the screen to flash as bright as possible before any explosions are actually drawn.
		// this causes everything to look like EGA for a single frame.
		// Meridian: no frikin flashing!!
		_flashScreen = false;
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
			for (std::list<Explosion*>::const_iterator i = _explosions.begin(); i != _explosions.end(); ++i)
			{
				_camera->convertVoxelToScreen((*i)->getPosition(), &bulletPositionScreen);
				if ((*i)->isBig())
				{
					if ((*i)->getCurrentFrame() >= 0)
					{
						tmpSurface = _game->getMod()->getSurfaceSet("X1.PCK")->getFrame((*i)->getCurrentFrame());
						tmpSurface->blitNShade(surface, bulletPositionScreen.x - (tmpSurface->getWidth() / 2), bulletPositionScreen.y - (tmpSurface->getHeight() / 2), 0, false, _nvColor);
					}
				}
				else if ((*i)->isHit())
				{
					tmpSurface = _game->getMod()->getSurfaceSet("HIT.PCK")->getFrame((*i)->getCurrentFrame());
					tmpSurface->blitNShade(surface, bulletPositionScreen.x - 15, bulletPositionScreen.y - 25, 0, false, _nvColor);
				}
				else
				{
					tmpSurface = _game->getMod()->getSurfaceSet("SMOKE.PCK")->getFrame((*i)->getCurrentFrame());
					tmpSurface->blitNShade(surface, bulletPositionScreen.x - 15, bulletPositionScreen.y - 15, 0, false, _nvColor);
				}
			}
		}
	}

	// draw map borders
	if (_camera->getShowAllLayers())
	{
		Position a, b, c, d;
		_camera->convertMapToScreen(Position(1, 0, 0), &a); // top left
		_camera->convertMapToScreen(Position(_camera->getMapSizeX() + 1, 0, 0), &b); // top right
		_camera->convertMapToScreen(Position(_camera->getMapSizeX() + 1, _camera->getMapSizeY(), 0), &c); // bottom right
		_camera->convertMapToScreen(Position(1, _camera->getMapSizeY(), 0), &d); // bottom left

		a += _camera->getMapOffset() + Position(0, 16, 0); // magicNumber = 16 (additional y offset)
		b += _camera->getMapOffset() + Position(0, 16, 0);
		c += _camera->getMapOffset() + Position(0, 16, 0);
		d += _camera->getMapOffset() + Position(0, 16, 0);

		surface->drawLine(a.x, a.y, b.x, b.y, 56); // magicNumber = 56 (UFO green/TFTD blue)
		surface->drawLine(b.x, b.y, c.x, c.y, 56);
		surface->drawLine(c.x, c.y, d.x, d.y, 56);
		surface->drawLine(d.x, d.y, a.x, a.y, 56);
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

void Map::toggleNightVision()
{
	_nightVisionOn = !_nightVisionOn;
}

/**
 * Handles fade-in and fade-out shade modification
 * @param original tile/item/unit shade
 */

int Map::reShade(Tile *tile)
{
	// no night vision
	if ((_save->getGlobalShade() <= NIGHT_VISION_THRESHOLD))
	{
		return tile->getShade();
	}

	// full night vision
	if (Options::fullNightVision)
	{
		return tile->getShade() > _fadeShade ? _fadeShade : tile->getShade();
	}

	// local night vision
	for (std::vector<BattleUnit*>::iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end(); ++i)
	{
		if ((*i)->getFaction() == FACTION_PLAYER && !(*i)->isOut())
		{
			if (_save->getTileEngine()->distanceSq(tile->getPosition(), (*i)->getPosition(), false) <= (*i)->getMaxViewDistanceAtDark(0) * (*i)->getMaxViewDistanceAtDark(0))
			{
				return tile->getShade() > _fadeShade ? _fadeShade : tile->getShade();
			}
		}
	}

	return tile->getShade();
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

	_camera->convertScreenToMap(mx, my + _spriteHeight / 4, &_selectorX, &_selectorY);

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

	// animate tiles
	for (int i = 0; i < _save->getMapSizeXYZ(); ++i)
	{
		_save->getTile(i)->animate();
	}

	// animate certain units (large flying units have a propulsion animation)
	for (std::vector<BattleUnit*>::iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end(); ++i)
	{
		if (_save->getDepth() > 0 && !(*i)->getFloorAbove())
		{
			(*i)->breathe();
		}
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
void Map::calculateWalkingOffset(BattleUnit *unit, Position *offset)
{
	int offsetX[8] = { 1, 1, 1, 0, -1, -1, -1, 0 };
	int offsetY[8] = { 1, 0, -1, -1, -1, 0, 1, 1 };
	int phase = unit->getWalkingPhase() + unit->getDiagonalWalkingPhase();
	int dir = unit->getDirection();
	int midphase = 4 + 4 * (dir % 2);
	int endphase = 8 + 8 * (dir % 2);
	int size = unit->getArmor()->getSize();

	offset->x = 0;
	offset->y = 0;

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
			offset->x = phase * 2 * offsetX[dir];
			offset->y = -phase * offsetY[dir];
		}
		else
		{
			offset->x = (phase - endphase) * 2 * offsetX[dir];
			offset->y = -(phase - endphase) * offsetY[dir];
		}
	}

	// If we are walking in between tiles, interpolate it's terrain level.
	if (unit->getStatus() == STATUS_WALKING || unit->getStatus() == STATUS_FLYING)
	{
		if (phase < midphase)
		{
			int fromLevel = getTerrainLevel(unit->getPosition(), size);
			int toLevel = getTerrainLevel(unit->getDestination(), size);
			if (unit->getPosition().z > unit->getDestination().z)
			{
				// going down a level, so toLevel 0 becomes +24, -8 becomes  16
				toLevel += 24 * (unit->getPosition().z - unit->getDestination().z);
			}
			else if (unit->getPosition().z < unit->getDestination().z)
			{
				// going up a level, so toLevel 0 becomes -24, -8 becomes -16
				toLevel = -24 * (unit->getDestination().z - unit->getPosition().z) + abs(toLevel);
			}
			offset->y += ((fromLevel * (endphase - phase)) / endphase) + ((toLevel * (phase)) / endphase);
		}
		else
		{
			// from phase 4 onwards the unit behind the scenes already is on the destination tile
			// we have to get it's last position to calculate the correct offset
			int fromLevel = getTerrainLevel(unit->getLastPosition(), size);
			int toLevel = getTerrainLevel(unit->getDestination(), size);
			if (unit->getLastPosition().z > unit->getDestination().z)
			{
				// going down a level, so fromLevel 0 becomes -24, -8 becomes -32
				fromLevel -= 24 * (unit->getLastPosition().z - unit->getDestination().z);
			}
			else if (unit->getLastPosition().z < unit->getDestination().z)
			{
				// going up a level, so fromLevel 0 becomes +24, -8 becomes 16
				fromLevel = 24 * (unit->getDestination().z - unit->getLastPosition().z) - abs(fromLevel);
			}
			offset->y += ((fromLevel * (endphase - phase)) / endphase) + ((toLevel * (phase)) / endphase);
		}
	}
	else
	{
		offset->y += getTerrainLevel(unit->getPosition(), size);

		if (_save->getDepth() > 0)
		{
			unit->setFloorAbove(false);

			// make sure this unit isn't obscured by the floor above him, otherwise it looks weird.
			if (_camera->getViewLevel() > unit->getPosition().z)
			{
				for (int z = std::min(_camera->getViewLevel(), _save->getMapSizeZ() - 1); z != unit->getPosition().z; --z)
				{
					if (!_save->getTile(Position(unit->getPosition().x, unit->getPosition().y, z))->hasNoFloor(0))
					{
						unit->setFloorAbove(true);
						break;
					}
				}
			}
		}
	}
}


/**
  * Terrainlevel goes from 0 to -24. For a larger sized unit, we need to pick the heighest terrain level, which is the lowest number...
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
			int l = _save->getTile(pos + Position(x, y, 0))->getTerrainLevel();
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
	_activeWeaponUfopediaArticleUnlocked = -1; // reset cache
	_cursorType = type;
	if (_cursorType == CT_NORMAL)
		_cursorSize = size;
	else
		_cursorSize = 1;
}

/**
 * Gets the cursor type.
 * @return cursortype.
 */
CursorType Map::getCursorType() const
{
	return _cursorType;
}

/**
 * Returns if there are any projectiles on the map.
 * @return True if there  are any projectiles.
 */
bool Map::hasProjectile() const
{
	return _projectiles.size() > 0;
}

/**
 * Adds a projectile to the map.
 * @param projectile The projectile to add to the map.
 */
void Map::addProjectile(Projectile *projectile)
{
	_projectiles.push_back(projectile);
}

/**
 * Removes a projectile from the map.
 * @param projectile The projectile to remove from the map.
 */
void Map::removeProjectile(Projectile *projectile)
{
	for (std::vector<Projectile*>::const_iterator ii = _projectiles.begin(); ii != _projectiles.end(); ++ii)
	{
		if ((*ii) == projectile)
		{
			_projectiles.erase(ii);
			return;
		}
	}
}

/**
 * Gets the projectiles on the map.
 * @return A vector containing the current projectiles.
 */
std::vector<Projectile*>& Map::getProjectiles()
{
	return _projectiles;
}

/**
 * Deletes all projectiles and clears the collection.
 */
const void Map::deleteProjectiles()
{
	for (std::vector<Projectile*>::const_iterator ii = _projectiles.begin(); ii != _projectiles.end(); ++ii)
	{
		delete *ii;
	}

	_projectiles.clear();
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
		_nvColor = Options::nightVisionColor;
		if (_fadeShade > NIGHT_VISION_SHADE) // 0 = max brightness
		{
			--_fadeShade;
		}
	}
	else
	{
		_nvColor = 0;
		if (_fadeShade < _save->getGlobalShade())
		{
			++_fadeShade;
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
	_message->setHeight((_visibleMapHeight < 200) ? _visibleMapHeight : 200);
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
	relativePosition.x = std::max(-midPoint, std::min(midPoint, (relativePosition.x + _camera->getMapOffset().x) - midPoint));

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
}

/**
 * Checks if the screen is still being rendered in EGA.
 * @return if we are still in EGA mode.
 */
bool Map::getBlastFlash() const
{
	return _flashScreen;
}

}
