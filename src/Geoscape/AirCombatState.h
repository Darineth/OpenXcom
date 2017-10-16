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

#include "../Engine/State.h"
#include "../Mod/RuleCraft.h"
#include "../Mod/RuleItem.h"
#include "AirActionMenuState.h"
#include <vector>
#include <queue>

namespace OpenXcom
{

class GeoscapeState;
class ImageButton;
class Text;
class Surface;
class InteractiveSurface;
class Window;
class Bar;
class Timer;
class CombatLog;
class TextButton;
class WarningMessage;

class Ufo;
class RuleUfo;
class Craft;
class RuleCraft;
class BattleUnit;
class CraftWeapon;
class AirCombatAI;

enum CombatLogColor;

struct AirCombatUnit
{
	AirCombatUnit(Game *game, Ufo *ufo_);
	AirCombatUnit(Game *game, Craft *craft_, Ufo *primaryUfo);
	~AirCombatUnit();

	void getActionCost(AirCombatAction *action) const;

	std::wstring getDisplayName() const;

	int getHealth() const;
	int getMaxHealth() const;
	int getHealthPercent() const;

	int getCombatFuel() const;
	int getMaxCombatFuel() const;

	bool spendCombatFuel(int fuel);
	bool spendTime(int time);

	bool canTargetPosition(int position) const;

	bool isDone(Ufo *target) const;

	Ufo *ufo;
	std::wstring ufoName;
	const RuleUfo *ufoRule;
	Craft *craft;
	const RuleCraft *craftRule;
	bool enemy;
	bool researched;
	bool destroyed;
	int index;
	int position;
	int weapons;
	int turnDelay;
	int turnOrder;
	bool evading;
	Surface *sprite;
	double combatSpeedCost;
	bool tooSlow;
	AirCombatAI *ai;
};

/*struct AirCombatProjectile
{
	AirCombatProjectile() : surface(nullptr), weapon(nullptr) {}
	AirCombatProjectile(CraftWeapon *weapon);

	Surface *surface;
	CraftWeapon *weapon;
};*/

struct XY
{
	int x;
	int y;

	XY() : x(0), y(0) {}
	XY(int x, int y) : x(x), y(y) {}

	XY& operator =(const XY &other)
	{
		x = other.x;
		y = other.y;
	}
};

struct AirCombatAnimation
{
	typedef void (AirCombatState::* FinishedCallback)(AirCombatAnimation*);

protected:
	FinishedCallback _callback;
	AirCombatAnimation(FinishedCallback callback) : _callback(callback) {};

public:
	virtual ~AirCombatAnimation() {}

	virtual bool animate(int elapsed) = 0;

	void onFinished(AirCombatState* state);
};

struct AirCombatMovementAnimation : public AirCombatAnimation
{
private:
	int _duration, _elapsed;
public:
	int x0, x1, y0, y1;
	Surface *target;

	AirCombatMovementAnimation(int x0, int y0, int x1, int y1, Surface *target, int duration, FinishedCallback callback);
	virtual ~AirCombatMovementAnimation();

	virtual bool animate(int elapsed) override;
};

struct AirCombatProjectileAnimation : public AirCombatAnimation
{
private:
	int _position, _speed;
public:
	int x0, x1, y0, y1;
	std::vector<XY> trajectory;
	int bulletSprite;
	int particleCount;
	bool reverseParticles;
	AirCombatAction *action;

	AirCombatProjectileAnimation(AirCombatAction *action, int x0, int y0, int x1, int y1, int bulletSprite, int particleCount, int speed, FinishedCallback callback);
	virtual ~AirCombatProjectileAnimation() {}

	virtual bool animate(int elapsed) override;

	XY getPosition(int offset) const;
};

struct AirCombatMultiShotAnimation : public AirCombatAnimation
{
private:
	int _delay, _elapsed;

public:
	AirCombatAction *action;
	int shot;

	AirCombatMultiShotAnimation(AirCombatAction *action, int delay, int shot, FinishedCallback callback);
	virtual ~AirCombatMultiShotAnimation() {};

	virtual bool animate(int elapsed) override;
};

struct AirCombatExplosionAnimation : public AirCombatAnimation
{
private:
	static const int HIT_FRAMES = 4;
	static const int EXPLODE_FRAMES = 8;

	int _endFrame;
	int _frameElapsed;

public:
	int currentFrame;
	XY position;
	AirCombatUnit *unit;

	AirCombatExplosionAnimation(int sprite, XY position, AirCombatUnit *unit, FinishedCallback callback);
	virtual ~AirCombatExplosionAnimation();

	virtual bool animate(int elapsed) override;
};

struct AirCombatDelayCallback : public AirCombatAnimation
{
private:
	int _delay, _elapsed;

public:
	AirCombatDelayCallback(FinishedCallback callback, int delay) : AirCombatAnimation(callback), _delay(delay), _elapsed(0) {}
	virtual ~AirCombatDelayCallback() {}

	virtual bool animate(int elapsed) override
	{
		_elapsed += elapsed;
		return _elapsed >= _delay;
	}
};

struct AirCombatInterpolationAnimation : public AirCombatAnimation
{
public:
	typedef void (AirCombatState::* InterpolateCallback)(AirCombatInterpolationAnimation*);

private:
	InterpolateCallback _onAnimate;
	AirCombatState *_state;
	int _value0;
	int _value1;
	int _duration;
	int _elapsed;

public:
	int value;

	AirCombatInterpolationAnimation(AirCombatState *state, int value0, int value1, int duration, InterpolateCallback onAnimate, FinishedCallback callback);
	virtual ~AirCombatInterpolationAnimation();

	virtual bool animate(int elapsed) override;
};

class AirCombatState : public State
{
	friend class AirCombatAI;

private:
	static const int SPRITE_HEIGHT = 32;
	static const int SPRITE_WIDTH = 32;
	static const int COLUMN_WIDTH = 40;
	static constexpr int SPRITE_OFFSET = (COLUMN_WIDTH - SPRITE_WIDTH) / 2;
	static const int BULLET_PARTICLES = 35;
	static const int VISIBLE_COLUMNS = 9;

	GeoscapeState *_parent;

	int _interceptionNumber;
	int _interceptionCount;
	bool _minimized;
	bool _waitForAltitude;
	bool _waitForPoly;
	bool _ended;
	bool _targeting;
	bool _checkEnd;
	int _animationFrame;
	bool _update;
	bool _init;
	int _basePosition;
	int _positionUnitOffset;
	int _backgroundOffset;

	SurfaceSet *_projectileSet;
	SurfaceSet *_hitSet;
	SurfaceSet *_explosionSet;

	bool _redraw;

	Window *_window;
	Surface *_background;
	InteractiveSurface *_battle;

	CombatLog *_combatLog;
	WarningMessage *_warning;

	InteractiveSurface *_btnMinimizedIcon, *_btnMinimize;
	int _minimizedIconX, _minimizedIconY;
	Text *_txtInterceptionNumber;

	std::vector<AirCombatAnimation*> _animations;
	Timer *_animationTimer;
	int _animationTime;

	Timer *_turnTimer;

	Ufo *_ufo;

	TextButton *_moveButton;
	TextButton *_attackButton;
	TextButton *_specialButton;
	TextButton *_waitButton;

	static const int MAX_UNITS = 4;
	//static const int MAX_POSITION = 7;

	std::vector<AirCombatUnit*> _units;

	std::array<AirCombatUnit*, MAX_UNITS> _enemies;
	InteractiveSurface *_enemyButtons[MAX_UNITS];
	int _enemyCount;
	InteractiveSurface *_hoverEnemy;

	std::array<AirCombatUnit*, MAX_UNITS> _craft;
	InteractiveSurface *_craftButtons[MAX_UNITS];
	int _craftCount;
	Text *_craftName[MAX_UNITS];
	Bar *_craftHealth[MAX_UNITS];
	Text *_craftHealthLabel[MAX_UNITS];
	Bar *_craftEnergy[MAX_UNITS];
	Text *_craftEnergyLabel[MAX_UNITS];

	static const int MAX_TURN_UNITS = 7;
	std::vector<AirCombatUnit*> _turnQueue;
	Text *_turnDelay[MAX_TURN_UNITS];
	Text *_turnOrder[MAX_TURN_UNITS];

	std::vector<AirCombatProjectileAnimation*> _projectiles;
	std::vector<AirCombatExplosionAnimation*> _explosions;

	AirCombatAction _currentAction;

	void updateUnitPositions();
	void draw();

	void setupEnemies();

	void addUfo(Ufo* ufo);
	bool removeUfo(AirCombatUnit* ufo);

	void removeUnit(AirCombatUnit *unit);

	void animateUnitMovement(AirCombatAction *action);

	void animateAttack(AirCombatAction *action, int shot = 1);

	std::tuple<int, int> getUnitDrawPosition(AirCombatUnit *unit) const;
	std::tuple<int, int> getUnitDrawPosition(int position, int index) const;

	AirCombatUnit *getCurrentUnit() const;

	void setupCombat();

	void invalidate();

	void addTurnUnit(AirCombatUnit *unit);
	void removeTurnUnit(AirCombatUnit *unit);
	void updateTurnQueue();

	void setupAction();

	bool allowInteraction() const;

	void animate();

	void showActionMenu(AirCombatActionType type);

	void onEnemyClick(Action*);
	void onCraftClick(Action*);

	void onUnitMouseOver(Action*);
	void onUnitMouseOut(Action*);

	void battleClick(Action* action);

	void onMoveClick(Action*);
	void onAttackClick(Action*);
	void onSpecialClick(Action*);
	void onWaitClick(Action*);

	void onProjectileMiss(AirCombatProjectileAnimation *animation);
	void onProjectileHit(AirCombatProjectileAnimation *animation);
	void onNextShot(AirCombatMultiShotAnimation *animation);
	void onExplosionDone(AirCombatExplosionAnimation *animation);
	void onUnitDeathExplosionDone(AirCombatExplosionAnimation *animation);
	void performAIAction(AirCombatDelayCallback *delay);
	void onPositionOffsetFinished(AirCombatDelayCallback *delay);
	void onAnimateBackground(AirCombatInterpolationAnimation *animation);
	void onAnimateBackgroundFinished(AirCombatInterpolationAnimation *animation);

public:
	AirCombatState(GeoscapeState *parent, Ufo *ufo);
	virtual ~AirCombatState();

	void init() override;
	void blit() override;
	void think() override;

	Ufo* getUfo() const;
	AirCombatUnit *getUfoUnit() const;

	bool addCraft(Craft *craft);
	bool removeCraft(Craft *craft, bool ending = false);
	bool hasCraft(Craft *craft);

	void setInterceptionNumber(int number);
	int getInterceptionNumber() const;
	void setInterceptionsCount(int count);
	int getInterceptionsCount() const;
	bool dogfightEnded() const;

	void setMinimized(bool minimized, bool init = false);
	bool isMinimized() const;
	void setWaitForAltitude(bool wait);
	bool getWaitForAltitude() const;
	void setWaitForPoly(bool wait);
	bool getWaitForPoly() const;

	void updatePosition();

	void requestUpdate();
	void update();
	bool updatePositionOffsets(bool animate);

	void executeAction();
	void cancelAction();

	void showWarning(const std::wstring warning);
	void showWarning(AirCombatUnit *actor, const std::wstring warning);

	void showMessage(const std::wstring warning);

	void log(const std::wstring &warning, CombatLogColor color);
	void log(AirCombatUnit *actor, const std::wstring &warning, CombatLogColor color);

	void setTargeting(bool targeting);

	void btnMinimizeClick(Action*);
	void btnMinimizedIconClick(Action*);
};

}
