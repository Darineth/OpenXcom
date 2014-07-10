/*
 * Copyright 2010-2014 OpenXcom Developers.
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
#define _USE_MATH_DEFINES
#include <cmath>
#include <fstream>
#include "Map.h"
#include "Camera.h"
#include "UnitSprite.h"
#include "Position.h"
#include "Pathfinding.h"
#include "TileEngine.h"
#include "Projectile.h"
#include "Explosion.h"
#include "BattlescapeState.h"
#include "../Resource/ResourcePack.h"
#include "../Engine/Action.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Engine/RNG.h"
#include "../Engine/Game.h"
#include "../Engine/Screen.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/MapDataSet.h"
#include "../Ruleset/MapData.h"
#include "../Ruleset/Armor.h"
#include "BattlescapeMessage.h"
#include "../Savegame/SavedGame.h"
#include "../Interface/Cursor.h"
#include "../Interface/NumberText.h"
#include "../Interface/Text.h"
#include "../Savegame/Role.h"
#include "../Ruleset/RuleRole.h"


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
Map::Map(Game *game, int width, int height, int x, int y, int visibleMapHeight) : InteractiveSurface(width, height, x, y), _game(game), _arrow(0), _selectorX(0), _selectorY(0), _mouseX(0), _mouseY(0), _cursorType(CT_NORMAL), _cursorSize(1), _animFrame(0), _projectiles(10), _projectileInFOV(false), _explosionInFOV(false), _launch(false), _visibleMapHeight(visibleMapHeight), _unitDying(false), _smoothingEngaged(false), _targetingTarget(-1,-1,-1), _targetingProjectile(0)
{
	_previewSetting = Options::battleNewPreviewPath;
	_smoothCamera = Options::battleSmoothCamera;
	if (Options::traceAI)
	{
		// turn everything on because we want to see the markers.
		_previewSetting = PATH_FULL;
	}
	_res = _game->getResourcePack();
	_spriteWidth = _res->getSurfaceSet("BLANKS.PCK")->getFrame(0)->getWidth();
	_spriteHeight = _res->getSurfaceSet("BLANKS.PCK")->getFrame(0)->getHeight();
	_save = _game->getSavedGame()->getSavedBattle();
	_message = new BattlescapeMessage(320, (visibleMapHeight < 200)? visibleMapHeight : 200, 0, 0);
	_message->setX(_game->getScreen()->getDX());
	_message->setY((visibleMapHeight - _message->getHeight()) / 2);
	_camera = new Camera(_spriteWidth, _spriteHeight, _save->getMapSizeX(), _save->getMapSizeY(), _save->getMapSizeZ(), this, visibleMapHeight);
	_scrollMouseTimer = new Timer(SCROLL_INTERVAL);
	_scrollMouseTimer->onTimer((SurfaceHandler)&Map::scrollMouse);
	_scrollKeyTimer = new Timer(SCROLL_INTERVAL);
	_scrollKeyTimer->onTimer((SurfaceHandler)&Map::scrollKey);
	_camera->setScrollTimer(_scrollMouseTimer, _scrollKeyTimer);
	
	_txtAccuracy = new Text(24, 9, 0, 0);
	_txtAccuracy->setSmall();
	_txtAccuracy->setPalette(_game->getScreen()->getPalette());
	_txtAccuracy->setHighContrast(true);
	_txtAccuracy->initText(_res->getFont("FONT_BIG"), _res->getFont("FONT_SMALL"), _game->getLanguage());
}

/**
 * Deletes the map.
 */
Map::~Map()
{
	deleteProjectiles();
	delete _scrollMouseTimer;
	delete _scrollKeyTimer;
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
	for (int y = 0; y < 9;++y)
		for (int x = 0; x < 9; ++x)
			_arrow->setPixel(x, y, pixels[x+(y*9)]);
	_arrow->unlock();

	_projectiles.clear();
}

/**
 * Keeps the animation timers running.
 */
void Map::think()
{
	_scrollMouseTimer->think(0, this);
	_scrollKeyTimer->think(0, this);
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
	_redraw = false;
	clear(Palette::blockOffset(0)+15);

	Tile *t;

	_projectileInFOV = _save->getDebugMode();
	if (_projectiles.size())
	{
		for(std::vector<Projectile*>::const_iterator ii = _projectiles.begin(); ii != _projectiles.end(); ++ii)
		{
			Projectile *p = *ii;
			t = _save->getTile(Position(p->getPosition(0).x/16, p->getPosition(0).y/16, p->getPosition(0).z/24));
			if (_save->getSide() == FACTION_PLAYER || (t && t->getVisible()))
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
			t = _save->getTile(Position((*i)->getPosition().x/16, (*i)->getPosition().y/16, (*i)->getPosition().z/24));
			if (t && ((*i)->isBig() || t->getVisible()))
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
	_message->setBackground(_res->getSurface("TAC00.SCR"));
	_message->initText(_res->getFont("FONT_BIG"), _res->getFont("FONT_SMALL"), _game->getLanguage());
	_message->setText(_game->getLanguage()->getString("STR_HIDDEN_MOVEMENT"));
}

/**
 * Draw the terrain.
 * Keep this function as optimised as possible. It's big to minimise overhead of function calls.
 * @param surface The surface to draw on.
 */
void Map::drawTerrain(Surface *surface)
{
	static const int arrowBob[8] = {0,1,2,1,0,1,2,1};

	int frameNumber = 0;
	Surface *tmpSurface;
	Tile *tile;
	int beginX = 0, endX = _save->getMapSizeX() - 1;
	int beginY = 0, endY = _save->getMapSizeY() - 1;
	int beginZ = 0, endZ = _camera->getShowAllLayers()?_save->getMapSizeZ() - 1:_camera->getViewLevel();
	Position mapPosition, screenPosition, bulletPositionScreen;
	int bulletLowX=16000, bulletLowY=16000, bulletLowZ=16000, bulletHighX=0, bulletHighY=0, bulletHighZ=0;
	int dummy;
	BattleUnit *unit = 0;
	bool invalid;
	int tileShade, wallShade, tileColor;
	NumberText *_numWaypid = 0;
	BattleAction *currentAction = _save->getBattleGame() ? _save->getBattleGame()->getCurrentAction() : 0;

	double calculatedAccuracy = -1.0;

	// if we got bullet, get the highest x and y tiles to draw it on
	if (_projectiles.size() /*&& _explosions.empty()*/)
	{
		Position posAvg;
		int avgCount = 0;
		for(std::vector<Projectile*>::const_iterator ii = _projectiles.begin(); ii != _projectiles.end(); ++ii)
		{
			Projectile *pp = *ii;
			//int part = pp->getItem() ? 0 : BULLET_SPRITES-1;
			int begin = 0;
			int end = pp->getItem() ? 1 : BULLET_SPRITES;
			int direction = 1;
			if (pp->isReversed())
			{
				begin = BULLET_SPRITES - 1;
				end = -1;
				direction = -1;
			}

			posAvg += pp->getPosition();
			avgCount += 1;

			for (int i = begin; i != end; i += direction)
			{
				Projectile *pp = *ii;
				int part = pp->getItem() ? 0 : BULLET_SPRITES-1;

				posAvg += pp->getPosition();
				avgCount += 1;

				for (int i = 0; i <= part; ++i)
				{
					Position pos = pp->getPosition(1-i);
					if (pos.x < bulletLowX) bulletLowX = pos.x;
					if (pos.y < bulletLowY) bulletLowY = pos.y;
					if (pos.z < bulletLowZ) bulletLowZ = pos.z;
					if (pos.x > bulletHighX) bulletHighX = pos.x;
					if (pos.y > bulletHighY) bulletHighY = pos.y;
					if (pos.z > bulletHighZ) bulletHighZ = pos.z;
				}
			}
		}
		
		if(avgCount > 0)
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
				}
				while (!enough);
			}
		}
	}
	else
	{
		_smoothingEngaged = false;
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

	if (!_waypoints.empty() || (pathfinderTurnedOn && (_previewSetting & PATH_TU_COST)))
	{
		_numWaypid = new NumberText(15, 15, 20, 30);
		_numWaypid->setPalette(getPalette());
		_numWaypid->setColor(Palette::blockOffset(pathfinderTurnedOn ? 0 : 1));
	}

	if (_projectiles.size() /*&& _explosions.empty()*/)
	{
		for(std::vector<Projectile*>::const_iterator ii = _projectiles.begin(); ii != _projectiles.end(); ++ii)
		{
			Projectile *pp = *ii;
			if(pp->getItem())
			{
				tmpSurface = pp->getSprite();

				Position pos = pp->getPosition();

				int posX = pos.x / 16;
				int posY = pos.y / 16;

				if (posX >= beginX && posX <= endX &&
					posY >= beginY && posY <= endY)
				{
					Position shadowPos = pos;
					// draw shadow on the floor
					shadowPos.z = _save->getTileEngine()->castedShade(shadowPos);

					if(_save->getTileEngine()->isVoxelVisible(shadowPos))
					{
						Position tilePos(posX, posY, shadowPos.z / 24);
						if(tile = _save->getTile(tilePos))
						{
							_camera->convertVoxelToScreen(shadowPos, &bulletPositionScreen);
							tile->getDrawables().push_back(new TileDrawable(tmpSurface, bulletPositionScreen.x - 16, bulletPositionScreen.y - 26, 16));
						}
					}

					if(_save->getTileEngine()->isVoxelVisible(pos))
					{
						Position tilePos(posX, posY, pos.z / 24);
						if(tile = _save->getTile(tilePos))
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
				int end = BULLET_SPRITES;
				int direction = 1;
				if (pp->isReversed())
				{
					begin = BULLET_SPRITES - 1;
					end = -1;
					direction = -1;
				}

				for (int i = begin; i != end; i += direction)
				{
					tmpSurface = _res->getSurfaceSet("Projectiles")->getFrame(pp->getParticle(i));
					if (tmpSurface)
					{
						Position pos = pp->getPosition(1-i);

						int posX = pos.x / 16;
						int posY = pos.y / 16;

						// TODO: Different height stuff for thrown thing.

						if (posX >= beginX && posX <= endX &&
							posY >= beginY && posY <= endY)
						{
							Position shadowPos = pos;
							// draw shadow on the floor
							shadowPos.z = _save->getTileEngine()->castedShade(shadowPos);
							if(_save->getTileEngine()->isVoxelVisible(shadowPos))
							{
								Position tilePos(posX, posY, shadowPos.z / 24);
								if(tile = _save->getTile(tilePos))
								{
									_camera->convertVoxelToScreen(shadowPos, &bulletPositionScreen);
									bulletPositionScreen.x -= tmpSurface->getWidth() / 2;
									bulletPositionScreen.y -= tmpSurface->getHeight() / 2;
									//tmpSurface->blitNShade(surface, bulletPositionScreen.x, bulletPositionScreen.y, 16);
									tile->getDrawables().push_back(new TileDrawable(tmpSurface, bulletPositionScreen.x, bulletPositionScreen.y, 16));
								}
							}

							// draw bullet itself
							if(_save->getTileEngine()->isVoxelVisible(pos))
							{
								Position tilePos(posX, posY, pos.z / 24);
								if(tile = _save->getTile(tilePos))
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
	else if(currentAction && (_cursorType == CT_AIM || _cursorType == CT_THROW))
	{
		Position cursorPos;
		getSelectorPosition(&cursorPos);

		if(_targetingTarget != cursorPos)
		{
			_targetingTarget = cursorPos;
			Tile *targetTile = _save->getTile(_targetingTarget);
			currentAction->target = cursorPos;

			if(targetTile)
			{
				Position targetVoxel;
				_save->getTileEngine()->getTargetVoxel(currentAction, _targetingTarget, targetVoxel);

				switch(currentAction->type)
				{
				case BA_THROW:
				case BA_SNAPSHOT:
				case BA_AIMEDSHOT:
				case BA_AUTOSHOT:
					if(_targetingProjectile)
					{
						delete _targetingProjectile;
					}
					_targetingProjectile = new Projectile(_res, _save, *currentAction, currentAction->actor->getPosition(), targetVoxel, 0, false);
					break;
				default:
					if(_targetingProjectile)
					{
						_targetingTarget = Position(-1,-1,-1);
						_targetingProjectile = 0;
						delete _targetingProjectile;
					}
				}

				if(_targetingProjectile)
				{
					int hit = currentAction->type != BA_THROW ? _targetingProjectile->calculateTrajectory(1.0, true, true) : _targetingProjectile->calculateThrow(100, true);
					if(hit == V_OUTOFBOUNDS)
					{
						delete _targetingProjectile;
						_targetingProjectile = 0;
					}
				}
			}
		}

		if(_targetingProjectile)
		{
			calculatedAccuracy = _targetingProjectile->getCalculatedAccuracy();

			Surface *surfaceTracer = _res->getSurfaceSet("Projectiles")->getFrame(35);
			Surface *surfaceImpact = _res->getSurfaceSet("Projectiles")->getFrame(280);

			/*int offsetX = 0;
			int offsetY = 0;

			if(_targetingProjectile->getItem())
			{
				Surface *floorSprite = _res->getSurfaceSet("FLOOROB.PCK")->getFrame(_targetingProjectile->getItem()->getRules()->getFloorSprite());
				if(floorSprite)
				{
					offsetX = floorSprite->getWidth() / 2;
					offsetY = floorSprite->getHeight() / 2;
				}
			}*/

			if(surface)
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
						&& _save->getTileEngine()->isVoxelVisible(pos))
					{
						int offsetY = 0;
						if(posZ > endZ)
						{
							offsetY -= 26 * (posZ - endZ);
							posZ = endZ;
						}

						Position tilePos(posX, posY, posZ);

						Tile *displayTile = _save->getTile(tilePos);

						if(displayTile)
						{				
							_camera->convertVoxelToScreen(pos, &bulletPositionScreen);
							displayTile->getDrawables().push_back(new TileDrawable(impact ? surfaceImpact : surfaceTracer, bulletPositionScreen.x, bulletPositionScreen.y, 0, 0, true));
						}
					}
				}
				while(_targetingProjectile->move() || ((!impact) && (impact = true)));
			}
		}
	}
	else
	{
		if(_targetingProjectile)
		{
			_targetingTarget = Position(-1,-1,-1);
			_targetingProjectile = 0;
			delete _targetingProjectile;
		}
	}

	Position selectorPosition;
	getSelectorPosition(&selectorPosition);

	int overwatchRadius = -1;
	if(currentAction->type == BA_OVERWATCH && currentAction->weapon)
	{
		overwatchRadius = currentAction->weapon->getRules()->getOverwatchRadius();
	}

	int unitOverwatchRadius = -1;
	Position unitOverwatchPosition;

	BattleUnit *selectedUnit = _save->getSelectedUnit();
	if(selectedUnit && selectedUnit->onOverwatch())
	{
		unitOverwatchRadius = selectedUnit->getOverwatchWeapon()->getRules()->getOverwatchRadius();
		unitOverwatchPosition = selectedUnit->getOverwatchTarget();
	}

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
					screenPosition.y > -_spriteHeight && screenPosition.y < surface->getHeight() + _spriteHeight )
				{
					tile = _save->getTile(mapPosition);

					if (!tile) continue;

					if (tile->isDiscovered(2))
					{
						tileShade = tile->getShade();
					}
					else
					{
						tileShade = 16;
						unit = 0;
					}

					tileColor = tile->getMarkerColor();

					// Draw floor
					tmpSurface = tile->getSprite(MapData::O_FLOOR);
					if (tmpSurface)
						tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y - tile->getMapData(MapData::O_FLOOR)->getYOffset(), tileShade, false);
					unit = tile->getUnit();

					// Draw cursor back
					if (_cursorType != CT_NONE && _selectorX > itX - _cursorSize && _selectorY > itY - _cursorSize && _selectorX < itX+1 && _selectorY < itY+1 && !_save->getBattleState()->getMouseOverIcons())
					{
						if (_camera->getViewLevel() == itZ)
						{
							if (_cursorType != CT_AIM)
							{
								if (unit && (unit->getVisible() || _save->getDebugMode()))
									frameNumber = (_animFrame % 2); // yellow box
								else
									frameNumber = 0; // red box
							}
							else
							{
								if (unit && (unit->getVisible() || _save->getDebugMode()))
									frameNumber = 7 + (_animFrame / 2); // yellow animated crosshairs
								else
									frameNumber = 6; // red static crosshairs
							}
							tmpSurface = _res->getSurfaceSet("CURSOR.PCK")->getFrame(frameNumber);
							tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y, 0);
						}
						else if (_camera->getViewLevel() > itZ)
						{
							frameNumber = 2; // blue box
							tmpSurface = _res->getSurfaceSet("CURSOR.PCK")->getFrame(frameNumber);
							tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y, 0);
						}
					}

					// special handling for a moving unit.
					if (mapPosition.y > 0)
					{
						Tile *tileNorth = _save->getTile(mapPosition - Position(0,1,0));
						BattleUnit *bu = tileNorth->getUnit();
						int tileNorthShade, tileTwoNorthShade, tileWestShade, tileNorthWestShade, tileSouthWestShade;
						if (tileNorth->isDiscovered(2))
						{
							tileNorthShade = tileNorth->getShade();
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
							Position tileOffset = Position(16,-8,0);
							// the part is 0 for small units, large units have parts 1,2 & 3 depending on the relative x/y position of this tile vs the actual unit position.
							int part = 0;
							part += tileNorth->getPosition().x - bu->getPosition().x;
							part += (tileNorth->getPosition().y - bu->getPosition().y)*2;
							tmpSurface = bu->getCache(&invalid, part);
							if (tmpSurface)
							{
								// draw unit
								Position offset;
								calculateWalkingOffset(bu, &offset);
								tmpSurface->blitNShade(surface, screenPosition.x + offset.x + tileOffset.x, screenPosition.y + offset.y  + tileOffset.y, tileNorthShade);
								// draw fire
								if (bu->getFire() > 0)
								{
									frameNumber = 4 + (_animFrame / 2);
									tmpSurface = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frameNumber);
									tmpSurface->blitNShade(surface, screenPosition.x + offset.x + tileOffset.x, screenPosition.y + offset.y + tileOffset.y, 0);
								}
							}

							/*
							 * Phase II: rerender any east wall type objects in the tile to the north of the unit
							 * only applies to movement in the north/south direction.
							 */
							if ((bu->getDirection() == 0 || bu->getDirection() == 4) && mapPosition.y >= 2)
							{
								Tile *tileTwoNorth = _save->getTile(mapPosition - Position(0,2,0));
								if (tileTwoNorth->isDiscovered(2))
								{
									tileTwoNorthShade = tileTwoNorth->getShade();
								}
								else
								{
									tileTwoNorthShade = 16;
								}
								tmpSurface = tileTwoNorth->getSprite(MapData::O_OBJECT);
								if (tmpSurface && tileTwoNorth->getMapData(MapData::O_OBJECT)->getBigWall() == 6)
								{
									tmpSurface->blitNShade(surface, screenPosition.x + tileOffset.x*2, screenPosition.y - tileTwoNorth->getMapData(MapData::O_OBJECT)->getYOffset() + tileOffset.y*2, tileTwoNorthShade);
								}
							}

							/*
							 * Phase III: render any south wall type objects in the tile to the northWest
							 */
							if (mapPosition.x > 0)
							{
								Tile *tileNorthWest = _save->getTile(mapPosition - Position(1,1,0));
								if (tileNorthWest->isDiscovered(2))
								{
									tileNorthWestShade = tileNorthWest->getShade();
								}
								else
								{
									tileNorthWestShade = 16;
								}
								tmpSurface = tileNorthWest->getSprite(MapData::O_OBJECT);
								if (tmpSurface && tileNorthWest->getMapData(MapData::O_OBJECT)->getBigWall() == 7)
								{
									tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y - tileNorthWest->getMapData(MapData::O_OBJECT)->getYOffset() + tileOffset.y*2, tileNorthWestShade);
								}
							}

							/*
							 * Phase IV: render any south or east wall type objects in the tile to the north
							 */
							if (tileNorth->getMapData(MapData::O_OBJECT) && tileNorth->getMapData(MapData::O_OBJECT)->getBigWall() >= 6)
							{
								tmpSurface = tileNorth->getSprite(MapData::O_OBJECT);
								if (tmpSurface)
									tmpSurface->blitNShade(surface, screenPosition.x + tileOffset.x, screenPosition.y - tileNorth->getMapData(MapData::O_OBJECT)->getYOffset() + tileOffset.y, tileNorthShade);
							}
							if (mapPosition.x > 0)
							{
								/*
								 * Phase V: re-render objects in the tile to the south west
								 * only render half so it won't overlap other areas that are already drawn
								 * and only apply this to movement in a north easterly or south westerly direction.
								 */
								if ( (bu->getDirection() == 1 || bu->getDirection() == 5) && mapPosition.y < endY-1)
								{
									Tile *tileSouthWest = _save->getTile(mapPosition + Position(-1, 1, 0));
									if (tileSouthWest->isDiscovered(2))
									{
										tileSouthWestShade = tileSouthWest->getShade();
									}
									else
									{
										tileSouthWestShade = 16;
									}
									tmpSurface = tileSouthWest->getSprite(MapData::O_OBJECT);
									if (tmpSurface)
									{
										tmpSurface->blitNShade(surface, screenPosition.x - tileOffset.x * 2, screenPosition.y - tileSouthWest->getMapData(MapData::O_OBJECT)->getYOffset(), tileSouthWestShade, true);
									}
								}

								/*
								 * Phase VI: we need to re-render everything in the tile to the west.
								 */
								Tile *tileWest = _save->getTile(mapPosition - Position(1,0,0));
								BattleUnit *westUnit = tileWest->getUnit();
								if (tileWest->isDiscovered(2))
								{
									tileWestShade = tileWest->getShade();
								}
								else
								{
									tileWestShade = 16;
									westUnit = 0;
								}
								tmpSurface = tileWest->getSprite(MapData::O_WESTWALL);
								if (tmpSurface && bu != unit)
								{
									if ((tileWest->getMapData(MapData::O_WESTWALL)->isDoor() || tileWest->getMapData(MapData::O_WESTWALL)->isUFODoor())
											&& tileWest->isDiscovered(0))
										wallShade = tileWest->getShade();
									else
										wallShade = tileWestShade;
									tmpSurface->blitNShade(surface, screenPosition.x - tileOffset.x, screenPosition.y - tileWest->getMapData(MapData::O_WESTWALL)->getYOffset() + tileOffset.y, wallShade, true);
								}
								tmpSurface = tileWest->getSprite(MapData::O_NORTHWALL);
								if (tmpSurface)
								{
									if ((tileWest->getMapData(MapData::O_NORTHWALL)->isDoor() || tileWest->getMapData(MapData::O_NORTHWALL)->isUFODoor())
											&& tileWest->isDiscovered(1))
										wallShade = tileWest->getShade();
									else
										wallShade = tileWestShade;
									tmpSurface->blitNShade(surface, screenPosition.x - tileOffset.x, screenPosition.y - tileWest->getMapData(MapData::O_NORTHWALL)->getYOffset() + tileOffset.y, wallShade, true);
								}
								tmpSurface = tileWest->getSprite(MapData::O_OBJECT);
								if (tmpSurface && tileWest->getMapData(MapData::O_OBJECT)->getBigWall() < 6 && tileWest->getMapData(MapData::O_OBJECT)->getBigWall() != 3)
								{
									tmpSurface->blitNShade(surface, screenPosition.x - tileOffset.x, screenPosition.y - tileWest->getMapData(MapData::O_OBJECT)->getYOffset() + tileOffset.y, tileWestShade, true);
									// if the object in the tile to the west is a diagonal big wall, we need to cover up the black triangle at the bottom
									if (tileWest->getMapData(MapData::O_OBJECT)->getBigWall() == 2)
									{
										tmpSurface = tile->getSprite(MapData::O_FLOOR);
										if (tmpSurface)
											tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y - tile->getMapData(MapData::O_FLOOR)->getYOffset(), tileShade);
									}
								}
								// draw an item on top of the floor (if any)
								int sprite = tileWest->getTopItemSprite();
								if (sprite != -1)
								{
									tmpSurface = _res->getSurfaceSet("FLOOROB.PCK")->getFrame(sprite);
									tmpSurface->blitNShade(surface, screenPosition.x - tileOffset.x, screenPosition.y + tileWest->getTerrainLevel() + tileOffset.y, tileWestShade, true);
								}
								// Draw soldier
								if (westUnit && westUnit->getStatus() != STATUS_WALKING && (!tileWest->getMapData(MapData::O_OBJECT) || tileWest->getMapData(MapData::O_OBJECT)->getBigWall() < 6) && (westUnit->getVisible() || _save->getDebugMode()))
								{
									// the part is 0 for small units, large units have parts 1,2 & 3 depending on the relative x/y position of this tile vs the actual unit position.
									int part = 0;
									part += tileWest->getPosition().x - westUnit->getPosition().x;
									part += (tileWest->getPosition().y - westUnit->getPosition().y)*2;
									tmpSurface = westUnit->getCache(&invalid, part);
									if (tmpSurface)
									{
										tmpSurface->blitNShade(surface, screenPosition.x - tileOffset.x, screenPosition.y + tileOffset.y + getTerrainLevel(westUnit->getPosition(), westUnit->getArmor()->getSize()), tileWestShade, true);
										if (westUnit->getFire() > 0)
										{
											frameNumber = 4 + (_animFrame / 2);
											tmpSurface = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frameNumber);
											tmpSurface->blitNShade(surface, screenPosition.x - tileOffset.x, screenPosition.y + tileOffset.y + getTerrainLevel(westUnit->getPosition(), westUnit->getArmor()->getSize()), 0, true);
										}
									}
								}

								// Draw smoke/fire
								if (tileWest->getSmoke() && tileWest->isDiscovered(2))
								{
									frameNumber = 0;
									if (!tileWest->getFire())
									{
										frameNumber = 8 + int(floor((tileWest->getSmoke() / 6.0) - 0.1)); // see http://www.ufopaedia.org/images/c/cb/Smoke.gif
									}

									if ((_animFrame / 2) + tileWest->getAnimationOffset() > 3)
									{
										frameNumber += ((_animFrame / 2) + tileWest->getAnimationOffset() - 4);
									}
									else
									{
										frameNumber += (_animFrame / 2) + tileWest->getAnimationOffset();
									}
									tmpSurface = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frameNumber);
									tmpSurface->blitNShade(surface, screenPosition.x - tileOffset.x, screenPosition.y + tileOffset.y, 0, true);
								}
								// Draw object
								if (tileWest->getMapData(MapData::O_OBJECT) && tileWest->getMapData(MapData::O_OBJECT)->getBigWall() >= 6)
								{
									tmpSurface = tileWest->getSprite(MapData::O_OBJECT);
									tmpSurface->blitNShade(surface, screenPosition.x - tileOffset.x, screenPosition.y - tileWest->getMapData(MapData::O_OBJECT)->getYOffset() + tileOffset.y, tileWestShade, true);
								}
							}
						}
					}

					if(unitOverwatchRadius >= 0
						&& mapPosition.z == unitOverwatchPosition.z
						&& _save->getTileEngine()->distance(mapPosition, unitOverwatchPosition) <= unitOverwatchRadius)
					{
						tmpSurface = _res->getSurfaceSet("Pathfinding")->getFrame(22);
						if(tmpSurface)
							tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y+2, 0, false, 3);
					}

					// Draw target area for Overwatch.
					if(_cursorType != CT_NONE
						&& overwatchRadius >= 0
						&& mapPosition.z == selectorPosition.z
						&& _save->getTileEngine()->distance(mapPosition, selectorPosition) <= overwatchRadius)
					{
						tmpSurface = _res->getSurfaceSet("Pathfinding")->getFrame(22);
						if(tmpSurface)
							tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y+2, 0, false, 2);
					}


					// Draw walls
					if (!tile->isVoid())
					{
						// Draw west wall
						tmpSurface = tile->getSprite(MapData::O_WESTWALL);
						if (tmpSurface)
						{
							if ((tile->getMapData(MapData::O_WESTWALL)->isDoor() || tile->getMapData(MapData::O_WESTWALL)->isUFODoor())
								 && tile->isDiscovered(0))
								wallShade = tile->getShade();
							else
								wallShade = tileShade;
							tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y - tile->getMapData(MapData::O_WESTWALL)->getYOffset(), wallShade, false);
						}
						// Draw north wall
						tmpSurface = tile->getSprite(MapData::O_NORTHWALL);
						if (tmpSurface)
						{
							if ((tile->getMapData(MapData::O_NORTHWALL)->isDoor() || tile->getMapData(MapData::O_NORTHWALL)->isUFODoor())
								 && tile->isDiscovered(1))
								wallShade = tile->getShade();
							else
								wallShade = tileShade;
							if (tile->getMapData(MapData::O_WESTWALL))
							{
								tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y - tile->getMapData(MapData::O_NORTHWALL)->getYOffset(), wallShade, true);
							}
							else
							{
								tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y - tile->getMapData(MapData::O_NORTHWALL)->getYOffset(), wallShade, false);
							}
						}
						// Draw object
						if (tile->getMapData(MapData::O_OBJECT) && tile->getMapData(MapData::O_OBJECT)->getBigWall() < 6)
						{
							tmpSurface = tile->getSprite(MapData::O_OBJECT);
							if (tmpSurface)
								tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y - tile->getMapData(MapData::O_OBJECT)->getYOffset(), tileShade, false);
						}
						// draw an item on top of the floor (if any)
						int sprite = tile->getTopItemSprite();
						if (sprite != -1)
						{
							tmpSurface = _res->getSurfaceSet("FLOOROB.PCK")->getFrame(sprite);
							tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y + tile->getTerrainLevel(), tileShade, false);
						}

					}

					bool topMostDrawables = false;
					std::vector<TileDrawable*> &drawables = tile->getDrawables();

					if(drawables.size())
					{
						for(std::vector<TileDrawable*>::const_iterator ii = drawables.begin(); ii != drawables.end(); ++ii)
						{
							TileDrawable *td = *ii;
							if(td->topMost)
							{
								topMostDrawables = true;
							}
							else
							{
								td->surface->blitNShade(surface, td->x, td->y, td->off, false, td->color);
							}
						}

						if(!topMostDrawables)
							tile->clearDrawables();
					}

			        unit = tile->getUnit();
					// Draw soldier
					if (unit && (unit->getVisible() || _save->getDebugMode()))
					{
						// the part is 0 for small units, large units have parts 1,2 & 3 depending on the relative x/y position of this tile vs the actual unit position.
						int part = 0;
						part += tile->getPosition().x - unit->getPosition().x;
						part += (tile->getPosition().y - unit->getPosition().y)*2;
						tmpSurface = unit->getCache(&invalid, part);
						if (tmpSurface)
						{
							Position offset;
							calculateWalkingOffset(unit, &offset);
							tmpSurface->blitNShade(surface, screenPosition.x + offset.x, screenPosition.y + offset.y, tileShade);
							if (unit->getFire() > 0)
							{
								frameNumber = 4 + (_animFrame / 2);
								tmpSurface = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frameNumber);
								tmpSurface->blitNShade(surface, screenPosition.x + offset.x, screenPosition.y + offset.y, 0);
							}
						}
					}
					// if we can see through the floor, draw the soldier below it if it is on stairs
					Tile *tileBelow = _save->getTile(mapPosition + Position(0, 0, -1));
					if (itZ > 0 && tile->hasNoFloor(tileBelow))
					{
						BattleUnit *tunit = _save->selectUnit(Position(itX, itY, itZ-1));
						Tile *ttile = _save->getTile(Position(itX, itY, itZ-1));
						if (tunit && tunit->getVisible() && ttile->getTerrainLevel() < 0 && ttile->isDiscovered(2))
						{
							// the part is 0 for small units, large units have parts 1,2 & 3 depending on the relative x/y position of this tile vs the actual unit position.
							int part = 0;
							part += ttile->getPosition().x - tunit->getPosition().x;
							part += (ttile->getPosition().y - tunit->getPosition().y)*2;
							tmpSurface = tunit->getCache(&invalid, part);
							if (tmpSurface)
							{
								Position offset;
								calculateWalkingOffset(tunit, &offset);
								offset.y += 24;
								tmpSurface->blitNShade(surface, screenPosition.x + offset.x, screenPosition.y + offset.y, ttile->getShade());
								if (tunit->getArmor()->getSize() > 1)
								{
									offset.y += 4;
								}
								if (tunit->getFire() > 0)
								{
									frameNumber = 4 + (_animFrame / 2);
									tmpSurface = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frameNumber);
									tmpSurface->blitNShade(surface, screenPosition.x + offset.x, screenPosition.y + offset.y, 0);
								}
							}
						}
					}

					// Draw smoke/fire
					if (tile->getSmoke() && tile->isDiscovered(2))
					{
						frameNumber = 0;
						if (!tile->getFire())
						{
							frameNumber = 8 + int(floor((tile->getSmoke() / 6.0) - 0.1)); // see http://www.ufopaedia.org/images/c/cb/Smoke.gif
						}

						if ((_animFrame / 2) + tile->getAnimationOffset() > 3)
						{
							frameNumber += ((_animFrame / 2) + tile->getAnimationOffset() - 4);
						}
						else
						{
							frameNumber += (_animFrame / 2) + tile->getAnimationOffset();
						}
						tmpSurface = _res->getSurfaceSet("SMOKE.PCK")->getFrame(frameNumber);
						tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y, 0);
					}

					// Draw Path Preview
					if (tile->getPreview() != -1 && tile->isDiscovered(0) && (_previewSetting & PATH_ARROWS))
					{
						if (itZ > 0 && tile->hasNoFloor(tileBelow))
						{
							tmpSurface = _res->getSurfaceSet("Pathfinding")->getFrame(11);
							if (tmpSurface)
							{
								tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y+2, 0, false, tile->getMarkerColor());
							}
						}
						tmpSurface = _res->getSurfaceSet("Pathfinding")->getFrame(tile->getPreview());
						if (tmpSurface)
						{
							tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y + tile->getTerrainLevel(), 0, false, tileColor);
						}
					}
					if (!tile->isVoid())
					{
						// Draw object
						if (tile->getMapData(MapData::O_OBJECT) && tile->getMapData(MapData::O_OBJECT)->getBigWall() >= 6)
						{
							tmpSurface = tile->getSprite(MapData::O_OBJECT);
							if (tmpSurface)
								tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y - tile->getMapData(MapData::O_OBJECT)->getYOffset(), tileShade, false);
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
									frameNumber = 3 + (_animFrame % 2); // yellow box
								else
									frameNumber = 3; // red box
							}
							else
							{
								if(currentAction->actor && currentAction->TU && currentAction->actor->getTimeUnits() < currentAction->TU)
									frameNumber = _animFrame % 2 ? 6 : 7;
								else if (unit && (unit->getVisible() || _save->getDebugMode()))
									frameNumber = 7 + (_animFrame / 2); // yellow animated crosshairs
								else
									frameNumber = 6; // red static crosshairs
							}
							tmpSurface = _res->getSurfaceSet("CURSOR.PCK")->getFrame(frameNumber);
							tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y, 0);

							// UFO extender accuracy: display adjusted accuracy value on crosshair in real-time.
							if (_cursorType == CT_AIM)
							{
								RuleItem *weapon = currentAction->weapon->getRules();
								int distance = _save->getTileEngine()->distance(Position (itX, itY,itZ), currentAction->actor->getPosition());

								std::ostringstream ss;
								int accuracy = currentAction->actor->getFiringAccuracy(currentAction->type, currentAction->weapon);
								int upperLimit = 200;
								int lowerLimit = weapon->getMinRange();

								if(Options::battleUFOExtenderAccuracy)
								{
									switch (currentAction->type)
									{
									case BA_AIMEDSHOT:
										upperLimit = weapon->getAimRange();
										break;
									case BA_SNAPSHOT:
										upperLimit = weapon->getSnapRange();
										break;
									case BA_AUTOSHOT:
										upperLimit = weapon->getAutoRange();
										break;
									default:
										break;
									}
									// at this point, let's assume the shot is adjusted and set the text amber.
									_txtAccuracy->setColor(Palette::blockOffset(1)-1);

									if (distance > upperLimit)
									{
										accuracy -= (distance - upperLimit) * weapon->getDropoff();
									}
									else if (distance < lowerLimit)
									{
										accuracy -= (lowerLimit - distance) * weapon->getDropoff();
									}
									else
									{
										// no adjustment made? set it to green.
										_txtAccuracy->setColor(Palette::blockOffset(4)-1);
									}

									// zero accuracy or out of range: set it red.
									if (accuracy <= 0 || distance > weapon->getMaxRange())
									{
										accuracy = 0;
										_txtAccuracy->setColor(Palette::blockOffset(2)-1);
									}
									else
									{
										if(calculatedAccuracy == 0.0)
										{
											accuracy = 0;
											_txtAccuracy->setColor(Palette::blockOffset(2)-1);
										}
										else if(calculatedAccuracy > 0.0 && calculatedAccuracy < 1.0)
										{
											accuracy = (int)((double)accuracy * calculatedAccuracy);
											_txtAccuracy->setColor(Palette::blockOffset(1)-1);
										}
									}

									ss << accuracy;
									ss << "%";
									_txtAccuracy->setText(Language::utf8ToWstr(ss.str().c_str()).c_str());
									_txtAccuracy->draw();
									_txtAccuracy->blitNShade(surface, screenPosition.x, screenPosition.y, 0);
								}
								else if (currentAction->type != BA_THROW && currentAction->type != BA_HIT)
								{
									double modifier = 0.0;
									int upperLimit = weapon->getAimRange();
									int lowerLimit = weapon->getMinRange();

									if (distance < lowerLimit)
									{
										modifier = (weapon->getDropoff() * (lowerLimit - distance));
										_txtAccuracy->setColor(Palette::blockOffset(1)-1);
									}
									else if (upperLimit < distance)
									{
										modifier = (weapon->getDropoff() * (distance - upperLimit));
										_txtAccuracy->setColor(Palette::blockOffset(1)-1);
									}
									else
									{
										_txtAccuracy->setColor(Palette::blockOffset(4)-1);
									}
									accuracy = std::max(0.0, accuracy - modifier);


									if(calculatedAccuracy == 0.0)
									{
										accuracy = 0;
										_txtAccuracy->setColor(Palette::blockOffset(2)-1);
									}
									else if(calculatedAccuracy > 0.0 && calculatedAccuracy < 1.0)
									{
										accuracy = (int)((double)accuracy * calculatedAccuracy);
										_txtAccuracy->setColor(Palette::blockOffset(1)-1);
									}

									ss << accuracy << "%";
									_txtAccuracy->setText(Language::utf8ToWstr(ss.str().c_str()).c_str());
									_txtAccuracy->draw();
									_txtAccuracy->blitNShade(surface, screenPosition.x, screenPosition.y, 0);
								}
								else
								{
									_txtAccuracy->setColor(Palette::blockOffset(4)-1);
									ss << accuracy;
									ss << "%";
									_txtAccuracy->setText(Language::utf8ToWstr(ss.str().c_str()).c_str());
									_txtAccuracy->draw();
									_txtAccuracy->blitNShade(surface, screenPosition.x, screenPosition.y, 0);
								}
							}
						}
						else if (_camera->getViewLevel() > itZ)
						{
							frameNumber = 5; // blue box
							tmpSurface = _res->getSurfaceSet("CURSOR.PCK")->getFrame(frameNumber);
							tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y, 0);
						}
						if (_cursorType > 2 && _camera->getViewLevel() == itZ)
						{
							int frame[6] = {0, 0, 0, 11, 13, 15};
							tmpSurface = _res->getSurfaceSet("CURSOR.PCK")->getFrame(frame[_cursorType] + (_animFrame / 4));
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
								tmpSurface = _res->getSurfaceSet("CURSOR.PCK")->getFrame(7);
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

					if(topMostDrawables)
					{
						for(std::vector<TileDrawable*>::const_iterator ii = drawables.begin(); ii != drawables.end(); ++ii)
						{
							TileDrawable *td = *ii;
							if(td->topMost)
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
						Tile *tileBelow = _save->getTile(mapPosition - Position(0,0,1));
						if (!tile || !tile->isDiscovered(0) || tile->getPreview() == -1)
							continue;
						int adjustment = -tile->getTerrainLevel();
						if (_previewSetting & PATH_ARROWS)
						{
							if (itZ > 0 && tile->hasNoFloor(tileBelow))
							{
								tmpSurface = _res->getSurfaceSet("Pathfinding")->getFrame(23);
								if (tmpSurface)
								{
									tmpSurface->blitNShade(surface, screenPosition.x, screenPosition.y+2, 0, false, tile->getMarkerColor());
								}
							}
							int overlay = tile->getPreview() + 12;
							tmpSurface = _res->getSurfaceSet("Pathfinding")->getFrame(overlay);
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
							if ( !(_previewSetting & PATH_ARROWS) )
							{
								_numWaypid->blitNShade(surface, screenPosition.x + 16 - off, screenPosition.y + (29-adjustment), 0, false, tile->getMarkerColor() );
							}
							else
							{
								_numWaypid->blitNShade(surface, screenPosition.x + 16 - off, screenPosition.y + (22-adjustment), 0);
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

	unit = _save->getSelectedUnit();
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
			if(soldier && (role = soldier->getRole()) && !role->isBlank() && (roleIcon = _game->getResourcePack()->getSurface(role->getRules()->getSmallIconSprite())))
			{
				roleIcon->blitNShade(surface, screenPosition.x + offset.x + (_spriteWidth / 2) - (roleIcon->getWidth() / 2), screenPosition.y + offset.y - roleIcon->getHeight() + arrowBob[_animFrame], 0);
			}
			else
			{
				_arrow->blitNShade(surface, screenPosition.x + offset.x + (_spriteWidth / 2) - (_arrow->getWidth() / 2), screenPosition.y + offset.y - _arrow->getHeight() + arrowBob[_animFrame], 0);
			}
		}
	}
	delete _numWaypid;

	// check if we got big explosions
	if (_explosionInFOV)
	{
		for (std::list<Explosion*>::const_iterator i = _explosions.begin(); i != _explosions.end(); ++i)
		{
			_camera->convertVoxelToScreen((*i)->getPosition(), &bulletPositionScreen);
			if ((*i)->isBig())
			{
				if ((*i)->getCurrentFrame() >= 0)
				{
					tmpSurface = _res->getSurfaceSet("X1.PCK")->getFrame((*i)->getCurrentFrame());
					tmpSurface->blitNShade(surface, bulletPositionScreen.x - 64, bulletPositionScreen.y - 64, 0);
				}
			}
			else if ((*i)->isHit())
			{
				tmpSurface = _res->getSurfaceSet("HIT.PCK")->getFrame((*i)->getCurrentFrame());
				tmpSurface->blitNShade(surface, bulletPositionScreen.x - 15, bulletPositionScreen.y - 25, 0);
			}
			else
			{
				tmpSurface = _res->getSurfaceSet("SMOKE.PCK")->getFrame((*i)->getCurrentFrame());
				tmpSurface->blitNShade(surface, bulletPositionScreen.x - 15, bulletPositionScreen.y - 15, 0);
			}
		}
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
	_animFrame++;
	if (_animFrame == 8) _animFrame = 0;

	// animate tiles
	for (int i = 0; i < _save->getMapSizeXYZ(); ++i)
	{
		_save->getTiles()[i]->animate();
	}

	// animate certain units (large flying units have a propultion animation)
	for (std::vector<BattleUnit*>::iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end(); ++i)
	{
		if ((*i)->getArmor()->getConstantAnimation())
		{
			(*i)->setCache(0);
			cacheUnit(*i);
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
	else
	if ((unit->getStatus() == STATUS_WALKING || unit->getStatus() == STATUS_FLYING))
	{
		if (phase < midphase)
		{
			offset->x = phase * 2 * offsetX[dir];
			offset->y = - phase * offsetY[dir];
		}
		else
		{
			offset->x = (phase - endphase) * 2 * offsetX[dir];
			offset->y = - (phase - endphase) * offsetY[dir];
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
				toLevel += 24*(unit->getPosition().z - unit->getDestination().z);
			}else if (unit->getPosition().z < unit->getDestination().z)
			{
				// going up a level, so toLevel 0 becomes -24, -8 becomes -16
				toLevel = -24*(unit->getDestination().z - unit->getPosition().z) + abs(toLevel);
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
				fromLevel -= 24*(unit->getLastPosition().z - unit->getDestination().z);
			}else if (unit->getLastPosition().z < unit->getDestination().z)
			{
				// going up a level, so fromLevel 0 becomes +24, -8 becomes 16
				fromLevel = 24*(unit->getDestination().z - unit->getLastPosition().z) - abs(fromLevel);
			}
			offset->y += ((fromLevel * (endphase - phase)) / endphase) + ((toLevel * (phase)) / endphase);
		}
	}
	else
	{
		offset->y += getTerrainLevel(unit->getPosition(), size);

		if (unit->getArmor()->getCanHoldWeapon())
		{
			if (unit->getStatus() == STATUS_AIMING)
			{
				offset->x = -16;
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
int Map::getTerrainLevel(Position pos, int size)
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
 * Checks all units for if they need to be redrawn.
 */
void Map::cacheUnits()
{
	for (std::vector<BattleUnit*>::iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end(); ++i)
	{
		cacheUnit(*i);
	}
}

/**
 * Check if a certain unit needs to be redrawn.
 * @param unit Pointer to battleUnit.
 */
void Map::cacheUnit(BattleUnit *unit)
{
	UnitSprite *unitSprite = new UnitSprite(unit->getStatus() == STATUS_AIMING ? _spriteWidth * 2: _spriteWidth, _spriteHeight, 0, 0, _save->getDepth() != 0);
	unitSprite->setPalette(this->getPalette());
	bool invalid, dummy;
	int numOfParts = unit->getArmor()->getSize() == 1?1:unit->getArmor()->getSize()*2;

	unit->getCache(&invalid);
	if (invalid)
	{
		if(_targetingProjectile)
		{
			delete _targetingProjectile;
			_targetingProjectile = 0;
			_targetingTarget.x = -1;
			_targetingTarget.y = -1;
			_targetingTarget.z = -1;
		}

		// 1 or 4 iterations, depending on unit size
		for (int i = 0; i < numOfParts; i++)
		{
			Surface *cache = unit->getCache(&dummy, i);
			if (!cache) // no cache created yet
			{
				cache = new Surface(_spriteWidth, _spriteHeight);
				cache->setPalette(this->getPalette());
			}

			cache->setWidth(unitSprite->getWidth());

			unitSprite->setBattleUnit(unit, i);

			BattleItem *rhandItem = unit->getItem("STR_RIGHT_HAND");
			BattleItem *lhandItem = unit->getItem("STR_LEFT_HAND");
			if (rhandItem)
			{
				unitSprite->setBattleItem(rhandItem);
			}
			if (lhandItem)
			{
				unitSprite->setBattleItem(lhandItem);
			}

			if(!lhandItem && !rhandItem)
			{
				unitSprite->setBattleItem(0);
			}
			unitSprite->setSurfaces(_res->getSurfaceSet(unit->getArmor()->getSpriteSheet()),
									_res->getSurfaceSet("HANDOB.PCK"),
									_res->getSurfaceSet("HANDOB2.PCK"));
			unitSprite->setAnimationFrame(_animFrame);
			cache->clear();
			unitSprite->blit(cache);
			unit->setCache(cache, i);
		}
	}
	delete unitSprite;
}

///**
// * Puts a projectile sprite on the map.
// * @param projectile Projectile to place.
// */
//void Map::setProjectile(Projectile *projectile)
//{
//	_projectile = projectile;
//	if (projectile && Options::battleSmoothCamera)
//	{
//		_launch = true;
//	}
//}
//
///**
// * Gets the current projectile sprite on the map.
// * @return Projectile or 0 if there is no projectile sprite on the map.
// */
//Projectile *Map::getProjectile() const
//{
//	return _projectile;
//}

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
	for(std::vector<Projectile*>::const_iterator ii = _projectiles.begin(); ii != _projectiles.end(); ++ii)
	{
		if((*ii) == projectile)
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
const std::vector<Projectile*>& Map::getProjectiles() const
{
	return _projectiles;
}

/**
 * Deletes all projectiles and clears the collection.
 */
const void Map::deleteProjectiles()
{
	for(std::vector<Projectile*>::const_iterator ii = _projectiles.begin(); ii != _projectiles.end(); ++ii)
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

/*
 * Special handling for setting the height of the map viewport.
 * @param height the new base screen height.
 */
void Map::setHeight(int height)
{
	Surface::setHeight(height);
	_visibleMapHeight = height - ICON_HEIGHT;
	_message->setHeight((_visibleMapHeight < 200)? _visibleMapHeight : 200);
	_message->setY((_visibleMapHeight - _message->getHeight()) / 2);
}
/*
 * Special handling for setting the width of the map viewport.
 * @param width the new base screen width.
 */
void Map::setWidth(int width)
{
	int dX = width - getWidth();
	Surface::setWidth(width);
	_message->setX(_message->getX() + dX / 2);
}

/*
 * Get the hidden movement screen's vertical position.
 * @return the vertical position of the hidden movement window.
 */
int Map::getMessageY()
{
	return _message->getY();
}
}
