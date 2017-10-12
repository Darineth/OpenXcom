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

#include "AirCombatState.h"

#include "../Engine/Game.h"
#include "../Engine/Screen.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/InteractiveSurface.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Engine/Action.h"
#include "../Mod/Mod.h"
#include "../Engine/Exception.h"
#include "../Battlescape/CombatLog.h"
#include "../Interface/TextButton.h"
#include "../Engine/Language.h"
#include "../Interface/Bar.h"
#include "AirActionMenuState.h"
#include "../Battlescape/WarningMessage.h"
#include "../Mod/RuleInterface.h"
#include "../Engine/Sound.h"
#include "../Engine/RNG.h"
#include "../Savegame/Ufo.h"
#include "../Mod/RuleUfo.h"
#include "../Savegame/Craft.h"
#include "../Mod/RuleCraft.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Savegame/CraftWeapon.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Country.h"
#include "../Savegame/Region.h"
#include "../Mod/RuleCountry.h"
#include "../Mod/RuleRegion.h"
#include "../Mod/RuleGlobe.h"
#include "../Geoscape/Globe.h"
#include "../Engine/Timer.h"
#include "../Battlescape/TileEngine.h"

#include "GeoscapeState.h"

#include <functional>

namespace OpenXcom
{

namespace
{
/**
* Calculates a line trajectory, using bresenham algorithm in 3D.
* @param origin Origin.
* @param target Target.
* @param posFunc Function call for each step in primary direction of line.
* @param driftFunc Function call for each side step of line.
*/
template<typename FuncNewPosition, typename FuncDrift>
bool calculateLineHitHelper(const XY &xy0, const XY &xy1, FuncNewPosition posFunc, FuncDrift driftFunc)
{
	int x, x0, x1, delta_x, step_x;
	int y, y0, y1, delta_y, step_y;
	int z, z0, z1, delta_z, step_z;
	int swap_xy, swap_xz;
	int drift_xy, drift_xz;
	int cx, cy, cz;

	x0 = xy0.x;
	y0 = xy0.y;

	x1 = xy1.x;
	y1 = xy1.y;

	//start and end points
	z0 = 0;	 z1 = 0;

	//'steep' xy Line, make longest delta x plane
	swap_xy = abs(y1 - y0) > abs(x1 - x0);
	if (swap_xy)
	{
		std::swap(x0, y0);
		std::swap(x1, y1);
	}

	//do same for xz
	swap_xz = abs(z1 - z0) > abs(x1 - x0);
	if (swap_xz)
	{
		std::swap(x0, z0);
		std::swap(x1, z1);
	}

	//delta is Length in each plane
	delta_x = abs(x1 - x0);
	delta_y = abs(y1 - y0);
	delta_z = abs(z1 - z0);

	//drift controls when to step in 'shallow' planes
	//starting value keeps Line centred
	drift_xy = (delta_x / 2);
	drift_xz = (delta_x / 2);

	//direction of line
	step_x = 1;  if (x0 > x1) { step_x = -1; }
	step_y = 1;  if (y0 > y1) { step_y = -1; }
	step_z = 1;  if (z0 > z1) { step_z = -1; }

	//starting point
	y = y0;
	z = z0;

	//step through longest delta (which we have swapped to x)
	for (x = x0; x != (x1 + step_x); x += step_x)
	{
		//copy position
		cx = x;	cy = y;	cz = z;

		//unswap (in reverse)
		if (swap_xz) std::swap(cx, cz);
		if (swap_xy) std::swap(cx, cy);
		if (posFunc(XY(cx, cy)))
		{
			return true;
		}

		//update progress in other planes
		drift_xy = drift_xy - delta_y;
		drift_xz = drift_xz - delta_z;

		//step in y plane
		if (drift_xy < 0)
		{
			y = y + step_y;
			drift_xy = drift_xy + delta_x;

			cx = x;	cz = z; cy = y;
			if (swap_xz) std::swap(cx, cz);
			if (swap_xy) std::swap(cx, cy);
			if (driftFunc(XY(cx, cy)))
			{
				return true;
			}
		}

		//same in z
		if (drift_xz < 0)
		{
			z = z + step_z;
			drift_xz = drift_xz + delta_x;

			cx = x;	cz = z; cy = y;
			if (swap_xz) std::swap(cx, cz);
			if (swap_xy) std::swap(cx, cy);
			if (driftFunc(XY(cx, cy)))
			{
				return true;
			}
		}
	}
	return false;
}
}

typedef std::tuple<int, int> coords;

// TODO: Create unit.
AirCombatUnit::AirCombatUnit(Game *game, Ufo *ufo_) :
	ufo(ufo_), ufoRule(ufo->getRules()), weapons(1),
	craft(nullptr), craftRule(nullptr), enemy(true),
	destroyed(false), position(0), index(-1), turnDelay(-1),
	sprite(nullptr), combatSpeedCost(1.0), tooSlow(false)
{
	int spriteIndex = ufoRule->getCombatSprite();
	if (spriteIndex > -1)
	{
		sprite = Game::getMod()->getSurfaceSet("AirCombatSprites")->getFrame(spriteIndex);
	}

	if (!sprite)
	{
		sprite = Game::getMod()->getSurface("AirCombatUnknownUfo");
	}

	researched = game->getSavedGame()->isResearched(ufoRule->getType());
	ufoName = ufo->getName(game->getLanguage());
}

// TODO: Create unit.
AirCombatUnit::AirCombatUnit(Game *game, Craft *craft_, Ufo *primaryUfo) :
	ufo(nullptr), ufoRule(nullptr),
	craft(craft_), craftRule(craft->getRules()), weapons(craftRule->getWeapons()), enemy(false),
	destroyed(false), position(0), index(-1), turnDelay(-1),
	sprite(nullptr),
	researched(true), combatSpeedCost(1.0), tooSlow(false)
{
	int spriteIndex = craftRule->getCombatSprite();
	if (spriteIndex > -1)
	{
		sprite = Game::getMod()->getSurfaceSet("AirCombatSprites")->getFrame(spriteIndex);
	}

	if (!sprite)
	{
		sprite = Game::getMod()->getSurface("AirCombatUnknownCraft");
	}

	int ufoSpeed = primaryUfo->getCraftStats().speedMax;
	int combatSpeed = craft->getCraftStats().speedMax;
	int maxCombatSpeed = combatSpeed * 1.5;

	// Exceeding max combat speed means we can't engage.
	if (ufoSpeed > maxCombatSpeed)
	{
		tooSlow = true;
	}
	else if (ufoSpeed > combatSpeed)
	{
		// 100%-150% of combat speed linearly scales from 1x to 16x.
		combatSpeedCost = (ufoSpeed - combatSpeed) / combatSpeed * 32;
	}
	else
	{
		// 0-100% of combat speed linearly scales directly.
		combatSpeedCost = (ufoSpeed / combatSpeed);
	}
}

AirCombatUnit::~AirCombatUnit()
{
}

void AirCombatUnit::getActionCost(AirCombatAction *action) const
{
	action->resetCost();

	// TODO: Use Speed to calcualte movement costs.

	switch (action->action)
	{
		case AA_MOVE_FORWARD:
			action->Time = 15;
			action->Energy = 10;
			return;
		case AA_MOVE_BACKWARD:
			action->Time = 10;
			action->Energy = 0;
			return;
		case AA_MOVE_PURSUE:
			action->Time = 25;
			action->Energy = 50;
			return;
		case AA_HOLD:
			action->Time = 15;
			return;
		case AA_EVADE:
			action->Time = 25;
			action->Energy = 25;
			return;
		case AA_FIRE_WEAPON:
			action->Ammo = 1;

			if (action->weapon)
			{
				action->Time = action->weapon->getRules()->getCostAimed();
			}
			else
			{
				action->Time = 20;
			}
			return;
	}
}

std::wstring AirCombatUnit::getDisplayName() const
{
	return ufo ? ufoName : craft->getName(Game::getLanguage());
}

int AirCombatUnit::getHealth() const
{
	if (craft)
	{
		return craft->getDamageMax() - craft->getDamage();
	}
	else if (ufo)
	{
		return ufo->getCraftStats().damageMax - ufo->getDamage();
	}

	return 0;
}

int AirCombatUnit::getMaxHealth() const
{
	if (craft)
	{
		return craft->getDamageMax();
	}
	else if (ufo)
	{
		return ufo->getCraftStats().damageMax;
	}

	return 0;
}

int AirCombatUnit::getCombatFuel() const
{
	if (craft)
	{
		return craft->getCombatFuel();
	}
	else if (ufo)
	{
		return 1000;
	}

	return 0;
}

int AirCombatUnit::getMaxCombatFuel() const
{
	if (craft)
	{
		return craftRule->getMaxCombatFuel();
	}
	else if (ufo)
	{
		return 1000;
	}

	return 0;
}

bool AirCombatUnit::spendCombatFuel(int fuel)
{
	if (ufo)
	{
		return true;
	}

	craft->setCombatFuel(craft->getCombatFuel() - fuel);

	return craft->getCombatFuel() > 0;
}

bool AirCombatUnit::spendTime(int time)
{
	turnDelay += time;
	return spendCombatFuel(time / 2);
}

bool AirCombatUnit::isDone(Ufo *target) const
{
	if (craft)
	{
		return craft->isDestroyed() || craft->getDestination() != target || !craft->isInDogfight();
	}
	else if (ufo)
	{
		return ufo->isCrashed() || ufo->getStatus() == Ufo::LANDED;
	}

	return false;
}

AirCombatState::AirCombatState(GeoscapeState *parent, Ufo *ufo) :
	_parent(parent), _ufo(ufo),
	_waitForAltitude(false), _waitForPoly(false),
	_minimized(false), _ended(false), _checkEnd(false), _interceptionNumber(0), _interceptionCount(0),
	_minimizedIconX(0), _minimizedIconY(0),
	_enemyCount(0), _craftCount(0)
{
	_screen = false;

	_window = new Window(this, 320, 200, 0, 0, POPUP_BOTH);
	_battle = new InteractiveSurface(320, 140, 0, 0);

	_combatLog = new CombatLog(320, 30, 0, 2);
	_warning = new WarningMessage(224, 24, 48, 200 - 24);

	_btnMinimize = new InteractiveSurface(12, 12, 308, 0);

	_btnMinimizedIcon = new InteractiveSurface(32, 20, _minimizedIconX, _minimizedIconY);
	_txtInterceptionNumber = new Text(16, 9, _minimizedIconX + 18, _minimizedIconY + 6);

	_moveButton = new TextButton(68, 14, 162, 140);
	_attackButton = new TextButton(68, 14, 162, 155);
	_specialButton = new TextButton(68, 14, 162, 170);
	_waitButton = new TextButton(68, 14, 162, 185);

	//setPalette("PAL_BATTLESCAPE");
	setPalette("PAL_GEOSCAPE");

	add(_btnMinimizedIcon);
	add(_txtInterceptionNumber, "minimizedNumber", "aircombat");

	_btnMinimizedIcon->drawRect(0, 0, _btnMinimizedIcon->getWidth(), _btnMinimizedIcon->getHeight(), 80);
	_btnMinimizedIcon->onMouseClick((ActionHandler)&AirCombatState::btnMinimizedIconClick);
	_txtInterceptionNumber->setText(Text::formatNumber(ufo->getId()));
	_btnMinimize->onMouseClick((ActionHandler)&AirCombatState::btnMinimizeClick);

	setInterface("aircombat");

	// TODO: Use Depth palette for undersea?
	//setPalette("PAL_BATTLESCAPE");

	add(_window);

	_battle->onMouseClick((ActionHandler)&AirCombatState::battleClick, 0);

	add(_btnMinimize);

	add(_moveButton, "actionButton", "aircombat");
	_moveButton->setHighContrast(true);
	_moveButton->setText(tr("STR_AIR_MOVE"));
	_moveButton->onMouseClick((ActionHandler)&AirCombatState::onMoveClick);
	_moveButton->onKeyboardPress((ActionHandler)&AirCombatState::onMoveClick, Options::keyAirMove);

	add(_attackButton, "actionButton", "aircombat");
	_attackButton->setHighContrast(true);
	_attackButton->setText(tr("STR_AIR_ATTACK"));
	_attackButton->onMouseClick((ActionHandler)&AirCombatState::onAttackClick);
	_attackButton->onKeyboardPress((ActionHandler)&AirCombatState::onAttackClick, Options::keyAirAttack);

	add(_specialButton, "actionButton", "aircombat");
	_specialButton->setHighContrast(true);
	_specialButton->setText(tr("STR_AIR_SPECIAL"));
	_specialButton->onMouseClick((ActionHandler)&AirCombatState::onSpecialClick);
	_specialButton->onKeyboardPress((ActionHandler)&AirCombatState::onSpecialClick, Options::keyAirSpecial);

	add(_waitButton, "actionButton", "aircombat");
	_waitButton->setHighContrast(true);
	_waitButton->setText(tr("STR_AIR_WAIT"));
	_waitButton->onMouseClick((ActionHandler)&AirCombatState::onWaitClick);
	_waitButton->onKeyboardPress((ActionHandler)&AirCombatState::onWaitClick, Options::keyAirWait);

	_window->setBorderColor(0);
	_window->setNoBorder();
	_window->setHighContrast(true);
	_window->setBackground(Game::getMod()->getSurface("AirCombatScreen"));
	_window->draw();

	for (int ii = 0; ii < MAX_UNITS; ++ii)
	{
		_enemies[ii] = nullptr;
		_craft[ii] = nullptr;

		add(_enemyButtons[ii] = new InteractiveSurface(SPRITE_WIDTH + 2, SPRITE_HEIGHT + 2, 0, 0));
		_enemyButtons[ii]->setVisible(false);
		_enemyButtons[ii]->onMouseClick((ActionHandler)&AirCombatState::onEnemyClick);
		_enemyButtons[ii]->onMouseOver((ActionHandler)&AirCombatState::onUnitMouseOver);
		_enemyButtons[ii]->onMouseOut((ActionHandler)&AirCombatState::onUnitMouseOut);

		add(_craftButtons[ii] = new InteractiveSurface(SPRITE_WIDTH + 2, SPRITE_HEIGHT + 2, 0, 0));
		_craftButtons[ii]->setVisible(false);
		_craftButtons[ii]->onMouseClick((ActionHandler)&AirCombatState::onCraftClick);
		_craftButtons[ii]->onMouseOver((ActionHandler)&AirCombatState::onUnitMouseOver);
		_craftButtons[ii]->onMouseOut((ActionHandler)&AirCombatState::onUnitMouseOut);

		add(_craftName[ii] = new Text(80, 8, 0, 140 + 15 * ii), "unitName", "aircombat");
		_craftName[ii]->setHighContrast(true);
		_craftName[ii]->setVisible(false);

		add(_craftHealth[ii] = new Bar(160, 3, 0, 148 + 15 * ii), "barHealth", "aircombat");
		_craftHealth[ii]->setMax(100);
		_craftHealth[ii]->setValue(100);
		_craftHealth[ii]->setAutoScale(true);
		_craftHealth[ii]->setBordered(true);
		_craftHealth[ii]->setFullBordered(true);
		_craftHealth[ii]->setVisible(false);

		add(_craftHealthLabel[ii] = new Text(40, 8, 80, 140 + 15 * ii), "unitHealth", "aircombat");
		_craftHealthLabel[ii]->setHighContrast(true);
		_craftHealthLabel[ii]->setVerticalAlign(ALIGN_MIDDLE);
		_craftHealthLabel[ii]->setVisible(false);
		_craftHealthLabel[ii]->setAlign(ALIGN_RIGHT);

		add(_craftEnergy[ii] = new Bar(160, 3, 0, 151 + 15 * ii), "barEnergy", "aircombat");
		_craftEnergy[ii]->setMax(100);
		_craftEnergy[ii]->setValue(100);
		_craftEnergy[ii]->setAutoScale(true);
		_craftEnergy[ii]->setBordered(true);
		_craftEnergy[ii]->setFullBordered(true);
		_craftEnergy[ii]->setVisible(false);

		add(_craftEnergyLabel[ii] = new Text(40, 8, 120, 140 + 15 * ii), "unitEnergy", "aircombat");
		_craftEnergyLabel[ii]->setHighContrast(true);
		_craftEnergyLabel[ii]->setVerticalAlign(ALIGN_MIDDLE);
		_craftEnergyLabel[ii]->setVisible(false);
		_craftEnergyLabel[ii]->setAlign(ALIGN_RIGHT);
	}

	for (int ii = 0; ii < MAX_TURN_UNITS; ++ii)
	{
		add(_turnDelay[ii] = new Text(11, 8, 232, 140 + 8 * ii), "turnUnit", "aircombat");
		_turnDelay[ii]->setHighContrast(true);
		_turnDelay[ii]->setVisible(false);
		_turnDelay[ii]->setAlign(ALIGN_LEFT);
		_turnDelay[ii]->setVerticalAlign(ALIGN_MIDDLE);

		add(_turnOrder[ii] = new Text(80, 8, 244, 140 + 8 * ii), "turnUnit", "aircombat");
		_turnOrder[ii]->setHighContrast(true);
		_turnOrder[ii]->setVisible(false);
		_turnOrder[ii]->setVerticalAlign(ALIGN_MIDDLE);
	}

	add(_battle);

	add(_combatLog);

	add(_warning, "warning", "battlescape");
	_warning->setColor(Game::getMod()->getInterface("battlescape")->getElement("warning")->color2);
	_warning->setTextColor(Game::getMod()->getInterface("battlescape")->getElement("warning")->color);

	_animationTime = 1000 / 60;
	_animationTimer = new Timer(_animationTime);
	_animationTimer->onTimer((StateHandler)&AirCombatState::animate);

	// TODO: Check depth.
	_projectileSet = _game->getMod()->getSurfaceSet("Projectiles");
	_explosionSet = _game->getMod()->getSurfaceSet("SMOKE.PCK");

	setupEnemies();

	updatePosition();

	setMinimized(true, true);
}

AirCombatState::~AirCombatState()
{
	for (auto ii : _enemies)
	{
		if (ii)
		{
			removeUfo(ii);
		}
	}

	for (auto ii : _craft)
	{
		if (ii)
		{
			removeCraft(ii->craft);
		}
	}
}

void AirCombatState::init()
{
	State::init();
	draw();
}

void AirCombatState::blit()
{
	if (_redraw)
	{
		_redraw = false;
		draw();
	}
	State::blit();
	blitProjectiles();
}

void AirCombatState::blitProjectiles()
{
	_battle->clear();

	for (AirCombatProjectileAnimation *pp : _projectiles)
	{
		int begin = 0;
		int end = pp->particleCount;
		if (end < 0) { end = BULLET_PARTICLES; }
		int direction = 1;
		double particleScale = 1.0;

		if (end > BULLET_PARTICLES)
		{
			particleScale = (double)BULLET_PARTICLES / end;
		}

		if (pp->reverseParticles)
		{
			begin = end - 1;
			end = -1;
			direction = -1;
		}

		for (int i = begin; i != end; i += direction)
		{
			Surface *particle = _projectileSet->getFrame(pp->bulletSprite + (int)floor(i * particleScale));

			XY pos = pp->getPosition(1 - i);

			if (pos.x > -1 && pos.y > -1)
			{
				particle->blitNShade(_battle, pos.x, pos.y, 0);
			}
		}
	}

	for (AirCombatExplosionAnimation *ee : _explosions)
	{
		Surface *hit = _explosionSet->getFrame(ee->currentFrame);

		hit->blitNShade(_battle, ee->position.x - 15, ee->position.y - 15, 0);
	}
}

void AirCombatState::setupEnemies()
{
	//AirCombatUnit *ufoUnit = new AirCombatUnit(_game, _ufo);

	addUfo(_ufo);

	invalidate();
}

void AirCombatState::addUfo(Ufo* ufo)
{
	for (int ii = 0; ii < MAX_UNITS; ++ii)
	{
		int index = (ii + 1) % MAX_UNITS;
		if (!_enemies[index])
		{
			AirCombatUnit *unit = new AirCombatUnit(_game, ufo);
			_enemies[index] = unit;
			unit->index = index;
			unit->sprite->setPalette(getPalette());
			unit->sprite->blit(_enemyButtons[index]);
			_enemyButtons[index]->setVisible(true);
			_enemyCount += 1;

			unit->position = 0;

			addTurnUnit(unit);

			for (Ufo *escort : ufo->getEscorts())
			{
				addUfo(escort);
			}

			update();

			return;
		}
	}
}

bool AirCombatState::removeUfo(AirCombatUnit* ufo)
{
	for (int ii = 0; ii < MAX_UNITS; ++ii)
	{
		if (_enemies[ii] == ufo)
		{
			delete _enemies[ii];
			_enemies[ii] = nullptr;

			_enemyButtons[ii]->setVisible(false);

			_enemyCount -= 1;

			removeTurnUnit(ufo);

			invalidate();

			return true;
		}
	}

	return false;
}

void AirCombatState::removeUnit(AirCombatUnit *unit)
{
	if (unit->craft)
	{
		removeCraft(unit->craft);
	}
	else
	{
		removeUfo(unit);
	}
}

Ufo* AirCombatState::getUfo() const
{
	return _ufo;
}

bool AirCombatState::addCraft(Craft *craft)
{
	if (craft->getCombatFuel() <= 0) { return false; }

	for (int ii = 0; ii < MAX_UNITS; ++ii)
	{
		int index = (ii + 1) % MAX_UNITS;
		if (!_craft[index])
		{
			AirCombatUnit* craftUnit = new AirCombatUnit(_game, craft, _ufo);
			_craft[index] = craftUnit;
			craftUnit->index = index;
			craftUnit->sprite->setPalette(getPalette());
			_craftButtons[index]->setVisible(true);

			_craftName[index]->setText(craft->getName(Game::getLanguage()));
			_craftName[index]->setVisible(true);

			_craftHealth[index]->setMax(craftUnit->getMaxHealth());
			_craftHealth[index]->setValue(craftUnit->getMaxHealth());
			_craftHealth[index]->setVisible(true);

			_craftHealthLabel[index]->setVisible(true);

			_craftEnergy[index]->setMax(craftUnit->getMaxCombatFuel());
			_craftEnergy[index]->setValue(craftUnit->getCombatFuel());
			_craftEnergy[index]->setVisible(true);

			_craftEnergyLabel[index]->setVisible(true);

			_craftCount += 1;

			craftUnit->position = 0;

			craft->setInDogfight(true);

			addTurnUnit(craftUnit);

			setMinimized(false);

			update();

			return true;
		}
	}

	return false;
}

bool AirCombatState::removeCraft(Craft *craft)
{
	for (int ii = 0; ii < MAX_UNITS; ++ii)
	{
		if (_craft[ii] && _craft[ii]->craft == craft)
		{
			// TODO: Award experience;
			_craft[ii]->craft->setInDogfight(false);
			if (!craft->isDestroyed())
			{
				craft->returnToBase();
			}
			removeTurnUnit(_craft[ii]);
			delete _craft[ii];
			_craft[ii] = nullptr;
			_craftName[ii]->setVisible(true);
			_craftHealth[ii]->setVisible(true);
			_craftHealthLabel[ii]->setVisible(true);
			_craftEnergy[ii]->setVisible(true);
			_craftButtons[ii]->setVisible(false);

			_craftCount -= 1;

			return true;
		}
	}

	return false;
}

bool AirCombatState::hasCraft(Craft *craft)
{
	for (AirCombatUnit *uu : _craft)
	{
		if (uu && uu->craft == craft)
		{
			return true;
		}
	}

	return false;
}

void AirCombatState::setInterceptionNumber(int number)
{
	_interceptionNumber = number;
	updatePosition();
}

int AirCombatState::getInterceptionNumber() const
{
	return _interceptionNumber;
}

void AirCombatState::setInterceptionsCount(int count)
{
	_interceptionCount = count;
	updatePosition();
}

int AirCombatState::getInterceptionsCount() const
{
	return _interceptionCount;
}

bool AirCombatState::dogfightEnded() const
{
	return _ended;
}

void AirCombatState::setMinimized(bool minimized, bool init)
{
	if (_minimized != minimized)
	{
		//setPalette(minimized ? "PAL_GEOSCAPE" : "PAL_BATTLESCAPE");

		_minimized = minimized;

		if (_minimized)
		{
			_animationTimer->stop();
			hideAll();
		}
		else
		{
			_animationTimer->start();
			showAll();
		}

		_btnMinimizedIcon->setHidden(!_minimized);
		_txtInterceptionNumber->setHidden(!_minimized);
		_btnMinimizedIcon->setVisible(_minimized);
		_txtInterceptionNumber->setVisible(_minimized);

		if (!init)
		{
			if (_minimized)
			{
				Game::getGame()->getScreen()->popMaximizeInfoScreen();
				if (Game::getGame()->isState(this))
				{
					Game::getGame()->popState(false);
				}
			}
			else
			{
				Game::getGame()->getScreen()->pushMaximizeInfoScreen();
				if (!Game::getGame()->isState(this))
				{
					Game::getGame()->pushState(this);
				}
			}
		}

		invalidate();

		//_window->invalidate(false);
		//_window->draw();
	}
}

bool AirCombatState::isMinimized() const
{
	return _minimized;
}

void AirCombatState::setWaitForAltitude(bool wait)
{
	_waitForAltitude = wait;
}

bool AirCombatState::getWaitForAltitude() const
{
	return _waitForAltitude;
}

void AirCombatState::setWaitForPoly(bool wait)
{
	_waitForPoly = wait;
}

bool AirCombatState::getWaitForPoly() const
{
	return _waitForPoly;
}

void AirCombatState::updatePosition()
{
	_minimizedIconX = 5;
	_minimizedIconY = (5 * _interceptionNumber) + (16 * (_interceptionNumber - 1));

	_btnMinimizedIcon->setX(_minimizedIconX);
	_btnMinimizedIcon->setY(_minimizedIconY);

	_txtInterceptionNumber->setX(_minimizedIconX + 18);
	_txtInterceptionNumber->setY(_minimizedIconY + 6);
}

void AirCombatState::update()
{
	if (_ended) { return; }

	for (int ii = 0; ii < MAX_UNITS; ++ii)
	{
		/*if (_enemies[ii] && _enemies[ii]->isDone(_ufo))
		{
			removeUfo(_enemies[ii]);
		}*/

		if (_craft[ii] && _craft[ii]->isDone(_ufo))
		{
			removeCraft(_craft[ii]->craft);
		}
	}

	updateTurnQueue();

	for (AirCombatUnit *enemy : _enemies)
	{
		if (enemy)
		{
			if (enemy->ufo->isDestroyed())
			{
				for (std::vector<Country*>::iterator country = _game->getSavedGame()->getCountries()->begin(); country != _game->getSavedGame()->getCountries()->end(); ++country)
				{
					if ((*country)->getRules()->insideCountry(enemy->ufo->getLongitude(), enemy->ufo->getLatitude()))
					{
						(*country)->addActivityXcom(enemy->ufo->getRules()->getScore() * 2);
						break;
					}
				}
				for (std::vector<Region*>::iterator region = _game->getSavedGame()->getRegions()->begin(); region != _game->getSavedGame()->getRegions()->end(); ++region)
				{
					if ((*region)->getRules()->insideRegion(enemy->ufo->getLongitude(), enemy->ufo->getLatitude()))
					{
						(*region)->addActivityXcom(enemy->ufo->getRules()->getScore() * 2);
						break;
					}
				}
				_game->getMod()->getSound("GEO.CAT", Mod::UFO_EXPLODE)->play(); //11

				showMessage(tr("STR_UFO_DESTROYED"));

				for (auto ii = enemy->ufo->getFollowers()->begin(); ii != enemy->ufo->getFollowers()->end();)
				{
					Craft *cc = dynamic_cast<Craft*>(*ii);
					if (cc)
					{
						cc->returnToBase();
						ii = enemy->ufo->getFollowers()->begin();
					}
					else
					{
						++ii;
					}
				}

				removeUfo(enemy);
			}
			else if (enemy->ufo->isCrashed())
			{
				for (std::vector<Country*>::iterator country = _game->getSavedGame()->getCountries()->begin(); country != _game->getSavedGame()->getCountries()->end(); ++country)
				{
					if ((*country)->getRules()->insideCountry(enemy->ufo->getLongitude(), enemy->ufo->getLatitude()))
					{
						(*country)->addActivityXcom(enemy->ufo->getRules()->getScore());
						break;
					}
				}
				for (std::vector<Region*>::iterator region = _game->getSavedGame()->getRegions()->begin(); region != _game->getSavedGame()->getRegions()->end(); ++region)
				{
					if ((*region)->getRules()->insideRegion(enemy->ufo->getLongitude(), enemy->ufo->getLatitude()))
					{
						(*region)->addActivityXcom(enemy->ufo->getRules()->getScore());
						break;
					}
				}
				_game->getMod()->getSound("GEO.CAT", Mod::UFO_CRASH)->play(); //10

				showMessage(tr("STR_UFO_CRASH_LANDS"));

				if (!_parent->getGlobe()->insideLand(enemy->ufo->getLongitude(), enemy->ufo->getLatitude()))
				{
					enemy->ufo->setStatus(Ufo::DESTROYED);
				}
				else
				{
					enemy->ufo->setSecondsRemaining(RNG::generate(24, 96) * 3600);
					enemy->ufo->setAltitude("STR_GROUND");
					if (enemy->ufo->getCrashId() == 0)
					{
						enemy->ufo->setCrashId(_game->getSavedGame()->getId("STR_CRASH_SITE"));
					}
				}

				for (auto ii = enemy->ufo->getFollowers()->begin(); ii != enemy->ufo->getFollowers()->end(); ++ii)
				{
					Craft *cc = dynamic_cast<Craft*>(*ii);
					if (cc && cc->getNumSoldiers() == 0 && cc->getNumVehicles() == 0)
					{
						cc->returnToBase();
					}
				}

				removeUfo(enemy);
			}
			/*else
			{
				enemy->ufo->move();
			}*/
		}
	}

	if (_checkEnd && (_enemyCount == 0 || _craftCount == 0))
	{
		if (!_ufo->isDestroyed() && !_ufo->isCrashed())
		{
			_ufo->move();
		}
		_ended = true;
		//setMinimized(true);
	}

	updateUnitPositions();

	invalidate();
}

void AirCombatState::executeAction()
{
	if (_currentAction.action == AA_NONE)
	{
		return;
	}

	_checkEnd = true;

	switch (_currentAction.action)
	{
		case AA_MOVE_FORWARD:
		case AA_MOVE_PURSUE:
		{
			if (_currentAction.unit->tooSlow)
			{
				showWarning(tr("STR_CANT_KEEP_UP").arg(_currentAction.unit->getDisplayName()));
				return;
			}

			int targetPosition = _currentAction.unit->position + (_currentAction.action == AA_MOVE_FORWARD ? 1 : 2);

			if (targetPosition > MAX_POSITION)
			{
				showWarning(tr("STR_CANT_MOVE_FORWARD").arg(_currentAction.unit->getDisplayName()));
				return;
			}

			for (int ii = 0; ii < MAX_UNITS; ++ii)
			{
				if (AirCombatUnit *enemy = (_currentAction.unit->enemy ? _craft[ii] : _enemies[ii]))
				{
					int enemyPosition = MAX_POSITION - enemy->position;
					if (enemyPosition <= targetPosition)
					{
						showWarning(tr("STR_MOVE_BLOCKED_BY_ENEMY").arg(_currentAction.unit->getDisplayName()));
						return;
					}
				}
			}
		}

		break;

		case AA_MOVE_BACKWARD:
		case AA_MOVE_RETREAT:
		{
			int targetPosition = _currentAction.unit->position - (_currentAction.action == AA_MOVE_BACKWARD ? 1 : 2);

			if (targetPosition < 0)
			{
				showWarning(tr("STR_CANT_MOVE_BACKWARD").arg(_currentAction.unit->getDisplayName()));
				return;
			}
		}
		break;

		case AA_FIRE_WEAPON:
			// Players can't fire from standoff.
			if (!_currentAction.unit->enemy && !_currentAction.unit->position)
			{
				showWarning(tr("STR_CANT_ATTACK_FROM_STANDOFF").arg(_currentAction.unit->getDisplayName()));
				return;
			}
			else if (_currentAction.weapon && !_currentAction.weapon->getAmmo())
			{
				showWarning(tr("STR_NO_ROUNDS_LEFT").arg(tr(_currentAction.weapon->getRules()->getType())));
				return;
			}
			break;
	}

	if (_currentAction.targeting)
	{
		return;
	}

	switch (_currentAction.action)
	{
		case AA_FIRE_WEAPON:
			// Enemies can't attack into the standoff row.
			if (_currentAction.unit->enemy && !_currentAction.unit->position && !_currentAction.target->position)
			{
				showWarning(tr("STR_CANT_ATTACK_STANDOFF").arg(_currentAction.unit->getDisplayName()));
				return;
			}

			int range = _currentAction.weapon ? _currentAction.weapon->getRules()->getRange() : _currentAction.unit->ufoRule->getWeaponRange();

			//int distance = (MAX_POSITION - _currentAction.unit->position) + (MAX_POSITION - _currentAction.target->position) + 1;
			int distance = (MAX_POSITION - _currentAction.target->position) - (_currentAction.unit->position);
			if (distance > range)
			{
				showWarning(tr("STR_OUT_OF_RANGE").arg(_currentAction.unit->getDisplayName()));
				return;
			}
			break;
	}

	std::string error;
	if (!_currentAction.spendCost(error))
	{
		_warning->showMessage(tr(error).arg(_currentAction.unit->getDisplayName()));
		return;
	}

	CombatLogColor color = _currentAction.unit->enemy ? COMBAT_LOG_RED : COMBAT_LOG_GREEN;

	std::stringstream ss;

	switch (_currentAction.action)
	{
		case AA_MOVE_FORWARD:
			animateUnitMovement(&_currentAction);
			_currentAction.unit->position = (_currentAction.unit->position + 1);
			ss << "STR_AIR_POSITION_" << _currentAction.unit->position;
			log(_currentAction.unit, tr(ss.str()), color);
			updateUnitPositions();
			break;

		case AA_MOVE_PURSUE:
			animateUnitMovement(&_currentAction);
			_currentAction.unit->position = (_currentAction.unit->position + 2);
			ss << "STR_AIR_POSITION_" << _currentAction.unit->position;
			log(_currentAction.unit, tr(ss.str()), color);
			updateUnitPositions();
			break;

		case AA_MOVE_BACKWARD:
			animateUnitMovement(&_currentAction);
			_currentAction.unit->position = (_currentAction.unit->position - 1);
			ss << "STR_AIR_POSITION_" << _currentAction.unit->position;
			log(_currentAction.unit, tr(ss.str()), color);
			updateUnitPositions();
			break;

		case AA_MOVE_RETREAT:
			animateUnitMovement(&_currentAction);
			_currentAction.unit->position = (_currentAction.unit->position - 2);
			ss << "STR_AIR_POSITION_" << _currentAction.unit->position;
			log(_currentAction.unit, tr(ss.str()), color);
			updateUnitPositions();
			break;

		case AA_HOLD:
			log(_currentAction.unit, tr("STR_AIR_HOLDING"), COMBAT_LOG_GREEN);
			break;

		case AA_EVADE:
			_currentAction.unit->evading = true;
			log(_currentAction.unit, tr("STR_AIR_EVADING"), COMBAT_LOG_GREEN);
			break;

		case AA_FIRE_WEAPON:
			//attack(_currentAction.unit, _currentAction.weapon, _currentAction.target);
			animateAttack(&_currentAction);
			return;
	}

	cancelAction();

	update();
}

void AirCombatState::cancelAction()
{
	setupAction();
	invalidate();
}

void AirCombatState::animateUnitMovement(AirCombatAction *action)
{
	int p0 = action->unit->position;
	int p1;
	switch (action->action)
	{
		case AA_MOVE_FORWARD:
			p1 = p0 + 1;
			break;
		case AA_MOVE_PURSUE:
			p1 = p0 + 2;
			break;
		case AA_MOVE_BACKWARD:
			p1 = p0 - 1;
			break;
		case AA_MOVE_RETREAT:
			p1 = p0 - 2;
			break;
		default:
			std::ostringstream ss;
			ss << "Unhandled movement action: " << action->action;
			throw Exception(ss.str());
	}

	int x0, y0, x1, y1;

	std::tie(x0, y0) = getUnitDrawPosition(p0, action->unit->index, !action->unit->craft);
	std::tie(x1, y1) = getUnitDrawPosition(p1, action->unit->index, !action->unit->craft);

	InteractiveSurface *surface = action->unit->craft ? _craftButtons[action->unit->index] : _enemyButtons[action->unit->index];

	AirCombatMovementAnimation *anim = new AirCombatMovementAnimation(action, x0, y0, x1, y1, surface, 250, nullptr);
	_animations.push_back(anim);
}

void AirCombatState::animateAttack(AirCombatAction *action, int shot)
{
	if (action->weapon)
	{
		if (!action->weapon->getAmmo())
		{
			showWarning(tr("STR_NO_ROUNDS_LEFT").arg(tr(action->weapon->getRules()->getType())));
			return;
		}

		action->weapon->setAmmo(action->weapon->getAmmo() - 1);
	}

	int shots = 1;
	int sprite = 1 * BULLET_PARTICLES;
	int speed = 1;
	int particles = BULLET_PARTICLES;

	if (action->weapon)
	{
		if (action->weapon->getRules()->getShotsAimed() > 1) { shots = action->weapon->getRules()->getShotsAimed(); }
		if (action->weapon->getRules()->getBulletSprite() > -1) { sprite = action->weapon->getRules()->getBulletSprite(); }
		if (action->weapon->getRules()->getBulletSpeed() > 0) { speed = action->weapon->getRules()->getBulletSpeed(); }
		if (action->weapon->getRules()->getBulletParticles() > 0) { particles = action->weapon->getRules()->getBulletParticles(); }

		_game->getMod()->getSound("GEO.CAT", action->weapon->getRules()->getSound())->play();
	}
	else
	{
		// UFO...
		sprite = 8 * BULLET_PARTICLES;
		speed = 5;

		int sound = action->unit->ufoRule->getFireSound();
		if (sound < 0)
		{
			_game->getMod()->getSound("GEO.CAT", Mod::UFO_FIRE)->play();
		}
		else
		{
			_game->getMod()->getSound("GEO.CAT", sound)->play();
		}
	}

	int x0, y0, x1, y1;

	std::tie(x0, y0) = getUnitDrawPosition(action->unit);
	std::tie(x1, y1) = getUnitDrawPosition(action->target);

	if (action->unit->enemy)
	{
		x0 += 3 * SPRITE_WIDTH / 4;
	}
	else
	{
		x0 += SPRITE_WIDTH / 4;
	}
	y0 += SPRITE_HEIGHT / 2;

	x1 += SPRITE_HEIGHT / 2;
	y1 += SPRITE_HEIGHT / 2;

	Craft *targetCraft = action->target->craft;
	Ufo *targetUfo = action->target->ufo;

	// TODO: Make these bonuses multiplicative.
	int accuracy = action->weapon ? action->weapon->getRules()->getAccuracy() : action->unit->ufoRule->getWeaponAccuracy();
	accuracy -= targetCraft ? targetCraft->getCraftStats().avoidBonus : targetUfo->getCraftStats().avoidBonus;
	accuracy += action->unit->craft ? action->unit->craft->getCraftStats().hitBonus : action->unit->ufo->getCraftStats().hitBonus;
	accuracy -= targetCraft ? targetCraft->getPilotDodgeBonus(targetCraft->getPilotList(false), _game->getMod()) : 0;

	x1 += RNG::generate(-5, 5);
	y1 += RNG::generate(-5, 5);

	bool hit = RNG::percent(accuracy);
	if (hit)
	{
		AirCombatProjectileAnimation *anim = new AirCombatProjectileAnimation(action, x0, y0, x1, y1, sprite, particles, speed, (AirCombatAnimation::FinishedCallback)&AirCombatState::onProjectileHit);
		_animations.push_back(anim);
		_projectiles.push_back(anim);
	}
	else
	{
		if (x1 == x0) { x1 = x0 + 1; }

		double m = (double)(y1 - y0) / (x1 - x0);

		int b = y0 - m * x0;

		// Intercept according to left or right edge.
		int xx = action->unit->enemy ? _battle->getWidth() : 0;
		int yx = m * xx + b;

		if (action->unit->enemy)
		{
			// Intercept according to top or bottom edge.
			int yy = m > 0 ? _battle->getHeight() : 0;
			int xy = (yy - b) / m;

			// Side intercept closer.
			if (xy > xx)
			{
				x1 = xx;
				y1 = yx;
			}
			else
			{
				x1 = xy;
				y1 = yy;
			}
		}
		else
		{
			// Intercept according to top or bottom edge.
			int yy = m < 0 ? _battle->getHeight() : 0;
			int xy = (yy - b) / m;

			// Top/bottom intercept closer.
			if (xy > xx)
			{
				x1 = xy;
				y1 = yy;
			}
			else
			{
				x1 = xx;
				y1 = yx;
			}
		}

		AirCombatProjectileAnimation *anim = new AirCombatProjectileAnimation(action, x0, y0, x1, y1, sprite, particles, speed, (AirCombatAnimation::FinishedCallback)&AirCombatState::onProjectileMiss);
		_animations.push_back(anim);
		_projectiles.push_back(anim);
	}

	if (shot < shots)
	{
		AirCombatMultiShotAnimation *nextShot = new AirCombatMultiShotAnimation(action, 100, shot + 1, (AirCombatAnimation::FinishedCallback)&AirCombatState::onNextShot);
		_animations.push_back(nextShot);
	}
}

void AirCombatState::onProjectileMiss(AirCombatProjectileAnimation *animation)
{
	//remove(animation->target);
	//delete animation->target;
	// Should we do anything on a miss?

	auto ii = std::find(_projectiles.begin(), _projectiles.end(), animation);
	if (ii != _projectiles.end())
	{
		_projectiles.erase(ii);
	}
}

void AirCombatState::onProjectileHit(AirCombatProjectileAnimation *animation)
{
	//remove(animation->target);
	//delete animation->target;

	auto ii = std::find(_projectiles.begin(), _projectiles.end(), animation);
	if (ii != _projectiles.end())
	{
		_projectiles.erase(ii);
	}

	CraftWeapon *weapon = animation->action->weapon;

	AirCombatUnit *source = animation->action->unit;
	AirCombatUnit *target = animation->action->target;
	Craft *craft = target->craft;
	Ufo *ufo = target->ufo;

	int rawDamage = weapon ? weapon->getRules()->getDamage() : source->ufo->getRules()->getWeaponPower();

	int damage = RNG::generate(0, rawDamage);

	damage = std::max(0, damage - (craft ? craft->getCraftStats().armor : ufo->getCraftStats().armor));

	// TODO: Shield damage.

	if (craft)
	{
		_game->getMod()->getSound("GEO.CAT", Mod::INTERCEPTOR_HIT)->play();
		craft->setDamage(craft->getDamage() + damage);
	}
	else if (ufo)
	{
		_game->getMod()->getSound("GEO.CAT", Mod::UFO_HIT)->play();
		ufo->setDamage(ufo->getDamage() + damage);

		if (ufo->isCrashed() && source && source->craft)
		{
			ufo->setShotDownByCraftId(source->craft->getUniqueId());
			ufo->setSpeed(0);
		}
	}

	std::wostringstream damageLog;

	damageLog << tr("STR_LOG_ACTOR").arg(source->getDisplayName());

	damageLog << tr("STR_LOG_TARGET").arg(target->getDisplayName());

	if (target->researched)
	{
		damageLog << "[" << target->getHealth() << "/" << target->getMaxHealth() << "] " << tr("STR_LOG_DAMAGE").arg(Text::formatNumber(damage));
	}
	else
	{
		double damageRatio = target->getMaxHealth() < 1 ? 0.0 : ((double)damage / target->getMaxHealth());

		if (damageRatio > 0.5)
		{
			damageLog << tr("STR_LOG_DAMAGE").arg(tr("STR_LOG_HIGH_DAMAGE"));
		}
		else if (damageRatio > 0.0)
		{
			damageLog << tr("STR_LOG_DAMAGE").arg(tr("STR_LOG_LOW_DAMAGE"));
		}
		else
		{
			damageLog << tr("STR_LOG_DAMAGE").arg(tr("STR_LOG_NO_DAMAGE"));
		}
	}

	log(damageLog.str(), craft ? COMBAT_LOG_RED : COMBAT_LOG_GREEN);

	if (craft && craft->isDestroyed())
	{
		_game->getMod()->getSound("GEO.CAT", Mod::INTERCEPTOR_EXPLODE)->play();
		showWarning(tr("STR_CRAFT_DESTROYED").arg(craft->getName(_game->getLanguage())));
	}
	else if (ufo && ufo->isDestroyed())
	{
		_game->getMod()->getSound("GEO.CAT", Mod::UFO_EXPLODE)->play();
		showMessage(tr("STR_UFO_DESTROYED"));
	}

	int hitSprite = 26;

	AirCombatExplosionAnimation *explosion = new AirCombatExplosionAnimation(hitSprite, XY(animation->x1, animation->y1), (AirCombatAnimation::FinishedCallback)&AirCombatState::onExplosionDone);
	_animations.push_back(explosion);
	_explosions.push_back(explosion);
}

void AirCombatState::onNextShot(AirCombatMultiShotAnimation *animation)
{
	animateAttack(animation->action, animation->shot);
}

void AirCombatState::onExplosionDone(AirCombatExplosionAnimation *animation)
{
	auto ii = std::find(_explosions.begin(), _explosions.end(), animation);
	if (ii != _explosions.end())
	{
		_explosions.erase(ii);
	}
}

void AirCombatState::showWarning(const std::wstring warning)
{
	_warning->setColor(WARNING_RED);
	_warning->showMessage(warning);
}

void AirCombatState::showWarning(AirCombatUnit *actor, const std::wstring warning)
{
	std::wostringstream ss;
	ss << tr("STR_LOG_ACTOR").arg(actor->getDisplayName()) << warning;
	_warning->setColor(WARNING_RED);
	showWarning(ss.str());
}

void AirCombatState::showMessage(const std::wstring warning)
{
	_warning->setColor(WARNING_GREEN);
	_warning->showMessage(warning);
}

void AirCombatState::log(const std::wstring &message, CombatLogColor color)
{
	_combatLog->log(message, color);
}

void AirCombatState::log(AirCombatUnit * actor, const std::wstring &message, CombatLogColor color)
{
	std::wostringstream ss;
	ss << tr("STR_LOG_ACTOR").arg(actor->getDisplayName()) << message;
	log(ss.str(), color);
}

void AirCombatState::setTargeting(bool targeting)
{
	_targeting = targeting;
	invalidate();
}

void AirCombatState::btnMinimizeClick(Action*)
{
	for (AirCombatUnit *craft : _craft)
	{
		if (craft && craft->position > 0)
		{
			showWarning(tr("STR_MINIMISE_AT_STANDOFF_RANGE_ONLY"));
			return;
		}
	}

	setMinimized(true);
}

void AirCombatState::btnMinimizedIconClick(Action*)
{
	// TODO: Check craft altitude abilities.
	/*if (_craft->getRules()->isWaterOnly() && _ufo->getAltitudeInt() > _craft->getRules()->getMaxAltitude())
	{
		_state->popup(new DogfightErrorState(_craft, tr("STR_UNABLE_TO_ENGAGE_DEPTH")));
		setWaitForAltitude(true);
	}
	else if (_craft->getRules()->isWaterOnly() && !_state->getGlobe()->insideLand(_craft->getLongitude(), _craft->getLatitude()))
	{
		_state->popup(new DogfightErrorState(_craft, tr("STR_UNABLE_TO_ENGAGE_AIRBORNE")));
		setWaitForPoly(true);
	}
	else*/
	{
		setMinimized(false);
	}
}

void AirCombatState::updateUnitPositions()
{
	for (AirCombatUnit *unit : _enemies)
	{
		if (unit && unit->sprite && !unit->destroyed)
		{
			int x, y;
			std::tie(x, y) = getUnitDrawPosition(unit);
			_enemyButtons[unit->index]->setX(x);
			_enemyButtons[unit->index]->setY(y);
		}
	}

	for (AirCombatUnit *unit : _craft)
	{
		if (unit && unit->sprite && !unit->destroyed)
		{
			int x, y;
			std::tie(x, y) = getUnitDrawPosition(unit);
			_craftButtons[unit->index]->setX(x);
			_craftButtons[unit->index]->setY(y);
		}
	}
}

void AirCombatState::draw()
{
	static Element *hover = Game::getMod()->getInterface("aircombat")->getElement("hoverUnit");
	static Element *current = Game::getMod()->getInterface("aircombat")->getElement("currentUnit");
	//_window->draw();
	//_battle->clear();

	for (int ii = 0; ii < MAX_UNITS; ++ii)
	{
		_enemyButtons[ii]->clear();
		_craftButtons[ii]->clear();

		if (_enemies[ii])
		{
			if (_enemies[ii] == getCurrentUnit())
			{
				_enemyButtons[ii]->drawRect(0, 0, SPRITE_WIDTH + 2, SPRITE_HEIGHT + 2, current->color);
				_enemyButtons[ii]->drawRect(1, 1, SPRITE_WIDTH, SPRITE_HEIGHT, 0);
			}
			else if (_enemyButtons[ii]->isHovered())
			{
				_enemyButtons[ii]->drawRect(0, 0, SPRITE_WIDTH + 2, SPRITE_HEIGHT + 2, hover->color2);
				_enemyButtons[ii]->drawRect(1, 1, SPRITE_WIDTH, SPRITE_HEIGHT, 0);
			}

			_enemies[ii]->sprite->setX(1);
			_enemies[ii]->sprite->setY(1);
			_enemies[ii]->sprite->blit(_enemyButtons[ii]);
		}

		if (_craft[ii])
		{
			_craftHealth[ii]->setValue(_craft[ii]->getHealth());
			_craftEnergy[ii]->setValue(_craft[ii]->getCombatFuel());
			_craftHealthLabel[ii]->setText(Text::formatNumber(_craft[ii]->getHealth()));
			_craftEnergyLabel[ii]->setText(Text::formatNumber(_craft[ii]->getCombatFuel()));
			//_craftHealthLabel[ii]->setText(tr("STR_AIR_UNIT_HEALTH").arg(_craft[ii]->getHealth()).arg(_craft[ii]->getMaxHealth()));
			//_craftEnergyLabel[ii]->setText(tr("STR_AIR_UNIT_COMBAT_FUEL").arg(_craft[ii]->getCombatFuel()).arg(_craft[ii]->getMaxCombatFuel()));

			if (_craft[ii] == getCurrentUnit())
			{
				_craftButtons[ii]->drawRect(0, 0, SPRITE_WIDTH + 2, SPRITE_HEIGHT + 2, current->color);
				_craftButtons[ii]->drawRect(1, 1, SPRITE_WIDTH, SPRITE_HEIGHT, 0);
			}
			else if (_craftButtons[ii]->isHovered())
			{
				_craftButtons[ii]->drawRect(0, 0, SPRITE_WIDTH + 2, SPRITE_HEIGHT + 2, hover->color);
				_craftButtons[ii]->drawRect(1, 1, SPRITE_WIDTH, SPRITE_HEIGHT, 0);
			}

			_craft[ii]->sprite->setX(1);
			_craft[ii]->sprite->setY(1);
			_craft[ii]->sprite->blit(_craftButtons[ii]);
		}
	}
}

std::tuple<int, int> AirCombatState::getUnitDrawPosition(AirCombatUnit *unit) const
{
	return getUnitDrawPosition(unit->position, unit->index, unit->enemy);
}

std::tuple<int, int> AirCombatState::getUnitDrawPosition(int position, int index, bool enemy) const
{
	return {
		enemy ? (0 + SPRITE_OFFSET + position * COLUMN_WIDTH) : (320 + SPRITE_OFFSET - (position + 1) * COLUMN_WIDTH),
		index * (SPRITE_HEIGHT + 2) + 2
	};
}

void AirCombatState::invalidate()
{
	_redraw = true;
}

void AirCombatState::onEnemyClick(Action *action)
{
	if (!allowInteraction()) return;

	if (_currentAction.targeting && _currentAction.unit->craft)
	{
		int index = 0;
		for (auto ss : _enemyButtons)
		{
			if (action->getSender() == ss)
			{
				_currentAction.target = _enemies[index];
				_currentAction.targeting = false;
				executeAction();
				return;
			}

			++index;
		}
	}
}

void AirCombatState::onCraftClick(Action *action)
{
	if (!allowInteraction()) return;

	if (_currentAction.targeting && _currentAction.unit->ufo)
	{
		int index = 0;
		for (auto ss : _craftButtons)
		{
			if (action->getSender() == ss)
			{
				_currentAction.target = _craft[index];
				_currentAction.targeting = false;
				executeAction();
				return;
			}

			++index;
		}
	}
}

void AirCombatState::onUnitMouseOver(Action *)
{
	if (!allowInteraction()) return;

	invalidate();
}

void AirCombatState::onUnitMouseOut(Action *)
{
	if (!allowInteraction()) return;

	invalidate();
}

void AirCombatState::battleClick(Action* action)
{
	if (!allowInteraction()) return;

	if (_currentAction.targeting)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		{
			cancelAction();
			return;
		}
	}
}

void AirCombatState::onMoveClick(Action*)
{
	if (!allowInteraction()) return;

	showActionMenu(AA_MOVE);
}

void AirCombatState::onAttackClick(Action*)
{
	if (!allowInteraction()) return;

	showActionMenu(AA_ATTACK);
}

void AirCombatState::onSpecialClick(Action*)
{
	if (!allowInteraction()) return;

	showActionMenu(AA_SPECIAL);
}

void AirCombatState::onWaitClick(Action*)
{
	if (!allowInteraction()) return;

	showActionMenu(AA_WAIT);
}

void AirCombatState::think()
{
	State::think();

	if (!_ended)
	{
		_animationTimer->think(this, nullptr);
	}

	if (_ended)
	{
		if (!_warning->getVisible() && !_animations.size())
		{
			for (int ii = 0; ii < MAX_UNITS; ++ii)
			{
				if (_craft[ii])
				{
					removeCraft(_craft[ii]->craft);
					_craft[ii] = nullptr;
				}
			}
			setMinimized(true);
		}
	}
}

void AirCombatState::addTurnUnit(AirCombatUnit *unit)
{
	auto ii = std::find(_turnQueue.begin(), _turnQueue.end(), unit);
	if (ii == _turnQueue.end())
	{
		if (_turnQueue.size())
		{
			unit->turnDelay = _turnQueue.back()->turnDelay + 1;
		}
		else
		{
			unit->turnDelay = 0;
		}
		_turnQueue.push_back(unit);
	}
}

void AirCombatState::removeTurnUnit(AirCombatUnit *unit)
{
	auto ii = std::find(_turnQueue.begin(), _turnQueue.end(), unit);
	if (ii != _turnQueue.end())
	{
		_turnQueue.erase(ii);
	}
}

void AirCombatState::updateTurnQueue()
{
	for (auto ii = _turnQueue.begin(); ii != _turnQueue.end();)
	{
		if ((*ii)->getCombatFuel())
		{
			++ii;
		}
		else
		{
			AirCombatUnit *remove = *ii;
			ii = _turnQueue.erase(ii);
			removeUnit(remove);
		}
	}

	std::sort(_turnQueue.begin(), _turnQueue.end(), [](AirCombatUnit *unit1, AirCombatUnit *unit2) -> bool
	{
		return unit1->turnDelay < unit2->turnDelay;
	});

	static Element *turnUnit = _game->getMod()->getInterface("aircombat")->getElement("turnUnit");

	int order = 0;
	for (AirCombatUnit *unit : _turnQueue)
	{
		unit->turnOrder = order++;
	}

	if (_turnQueue.size())
	{
		int firstDelay = _turnQueue.front()->turnDelay;

		_turnQueue.front()->evading = false;

		for (auto ii = _turnQueue.begin(); ii != _turnQueue.end(); ++ii)
		{
			AirCombatUnit *unit = *ii;
			unit->turnDelay -= firstDelay;
			/*if (unit->spendCombatFuel(firstDelay))
			{
				unit->turnDelay -= firstDelay;
				++ii;
			}
			else
			{
				showWarning(tr("STR_LOW_FUEL_RETURNING_TO_BASE"));
				unit->craft->setLowFuel(true);
				unit->craft->returnToBase();
				ii = _turnQueue.erase(ii);
				removeCraft(unit->craft);
			}*/
		}
	}

	for (int ii = 0; ii < MAX_TURN_UNITS; ++ii)
	{
		if (_turnQueue.size() > ii)
		{
			_turnDelay[ii]->setVisible(true);
			_turnDelay[ii]->setColor(_turnQueue[ii]->ufo ? turnUnit->color2 : turnUnit->color);
			if (_turnQueue[ii]->turnDelay)
			{
				_turnDelay[ii]->setText(Text::formatNumber(_turnQueue[ii]->turnDelay));
			}
			else
			{
				_turnDelay[ii]->setText(L"");
			}

			_turnOrder[ii]->setVisible(true);
			_turnOrder[ii]->setText(_turnQueue[ii]->getDisplayName());
			_turnOrder[ii]->setColor(_turnQueue[ii]->ufo ? turnUnit->color2 : turnUnit->color);
		}
		else
		{
			_turnDelay[ii]->setVisible(false);
			_turnOrder[ii]->setVisible(false);
		}
	}
}

AirCombatUnit *AirCombatState::getCurrentUnit() const
{
	return _turnQueue.size() ? _turnQueue.front() : nullptr;
}

void AirCombatState::setupAction()
{
	_currentAction.action = AA_NONE;
	_currentAction.unit = getCurrentUnit();
	_currentAction.description = L"";
	_currentAction.weapon = nullptr;
	_currentAction.targeting = false;
	_currentAction.target = nullptr;
	_currentAction.Time = 0;
	_currentAction.Energy = 0;
}

bool AirCombatState::allowInteraction() const
{
	return !_animations.size() && !_ended;
}

void AirCombatState::showActionMenu(AirCombatActionType type)
{
	setupAction();
	Game::getGame()->pushState(new AirActionMenuState(this, &_currentAction, type, 0, _window->getHeight() - 60));
}

void AirCombatState::animate()
{
	if (_animations.size())
	{
		int lastAnimation = _animations.size();
		for (int ii = 0; ii < lastAnimation; )
		{
			AirCombatAnimation *anim = _animations[ii];

			if (anim->animate(_animationTime))
			{
				anim->onFinished(this);
				delete anim;
				_animations.erase(_animations.begin() + ii);
				--lastAnimation;
			}
			else
			{
				++ii;
			}
		}

		invalidate();

		if (!_animations.size())
		{
			update();
		}
	}
}

////////// Animation Classes //////////

void AirCombatAnimation::onFinished(AirCombatState* state)
{
	if (_callback)
	{
		(state->*_callback)(this);
	}
}

AirCombatMovementAnimation::AirCombatMovementAnimation(AirCombatAction *action, int x0, int y0, int x1, int y1, Surface *target, int duration, FinishedCallback callback)
	: AirCombatAnimation(callback), x0(x0), y0(y0), x1(x1), y1(y1), target(target), _duration(duration), _elapsed(0)
{

}

AirCombatMovementAnimation::~AirCombatMovementAnimation()
{

}

bool AirCombatMovementAnimation::animate(int elapsed)
{
	_elapsed += elapsed;

	double interpolate = std::min((double)(_elapsed) / _duration, 1.0);

	target->setX(x0 + (x1 - x0) * interpolate);
	target->setY(y0 + (y1 - y0) * interpolate);

	return _elapsed >= _duration;
}

// AirCombatProjectileAnimation

AirCombatProjectileAnimation::AirCombatProjectileAnimation(AirCombatAction *action, int x0, int y0, int x1, int y1, int bulletSprite, int particleCount, int speed, FinishedCallback callback)
	: AirCombatAnimation(callback), action(action), x0(x0), y0(y0), x1(x1), y1(y1), bulletSprite(bulletSprite), particleCount(particleCount), _position(0), _speed(speed), reverseParticles(false)
{
	if ((x1 - x0) + (y1 - y0) >= 0)
	{
		reverseParticles = true;
	}

	calculateLineHitHelper(XY(x0, y0), XY(x1, y1),
		[&](XY position)
	{
		trajectory.emplace_back(position);
		return false;
	},
		[](XY position)
	{
		return false;
	});
}

bool AirCombatProjectileAnimation::animate(int elapsed)
{
	_position += _speed;
	if (_position >= trajectory.size())
		_position = trajectory.size() - 1;

	//const XY &xy = trajectory[_position];

	//target->setX(xy.x);
	//target->setY(xy.y);

	return _position >= trajectory.size() - 1;
}

XY AirCombatProjectileAnimation::getPosition(int offset) const
{
	int posOffset = (int)_position + offset;
	if (posOffset >= 0 && posOffset < (int)trajectory.size())
		return trajectory.at(posOffset);
	else if (trajectory.size())
		return trajectory.at(_position);
	else
		return XY(-1, -1);
}

// AirCombatMultiShotAnimation

AirCombatMultiShotAnimation::AirCombatMultiShotAnimation(AirCombatAction *action, int delay, int shot, FinishedCallback callback) : AirCombatAnimation(callback), action(action), _delay(delay), shot(shot), _elapsed(0)
{
}

bool AirCombatMultiShotAnimation::animate(int elapsed)
{
	_elapsed += elapsed;

	return _elapsed >= _delay;
}

// AirCombatExplosionAnimation
AirCombatExplosionAnimation::AirCombatExplosionAnimation(int sprite, XY position, FinishedCallback callback) : AirCombatAnimation(callback), currentFrame(sprite), _endFrame(sprite + HIT_FRAMES), position(position), _frameElapsed(0)
{
}

AirCombatExplosionAnimation::~AirCombatExplosionAnimation()
{
}

bool AirCombatExplosionAnimation::animate(int elapsed)
{
	_frameElapsed += elapsed;

	if (_frameElapsed > 50)
	{
		currentFrame = std::min(currentFrame + 1, _endFrame);
		_frameElapsed = 0;
	}
	return currentFrame >= _endFrame;
}

}
