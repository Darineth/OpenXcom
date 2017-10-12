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
 * along with OpenXcom.  If not, see <http:///www.gnu.org/licenses/>.
 */
#include "../Engine/State.h"
#include "../Engine/InteractiveSurface.h"
#include "../Mod/RuleItem.h"
#include <unordered_set>

namespace OpenXcom
{

class AirCombatState;
class AirActionMenuItem;
class Text;
class AirCombatAction;
class Frame;
class CraftWeapon;
class AirCombatUnit;

enum AirCombatActionType : Uint8 { AA_NONE, AA_MOVE, AA_MOVE_FORWARD, AA_MOVE_BACKWARD, AA_MOVE_PURSUE, AA_MOVE_RETREAT, AA_WAIT, AA_HOLD, AA_EVADE, AA_ATTACK, AA_FIRE_WEAPON, AA_SPECIAL, AA_DISENGAGE };

struct AirCombatAction : public RuleItemUseCost
{
	AirCombatUnit *unit;
	AirCombatActionType action;
	CraftWeapon *weapon;
	std::wstring description;
	bool targeting;
	AirCombatUnit *target;

	AirCombatAction() : unit(nullptr), action(AA_NONE), weapon(nullptr), targeting(false), target(nullptr) {}
	AirCombatAction(AirCombatUnit *unit_, AirCombatActionType action_, CraftWeapon *weapon_) : unit(unit_), action(action_), weapon(weapon_), targeting(false), target(nullptr) {}

	AirCombatAction &operator=(const AirCombatAction &other)
	{
		*(RuleItemUseCost*)this = other;
		unit = other.unit;
		action = other.action;
		weapon = other.weapon;
		description = other.description;
		targeting = other.targeting;
		target = other.target;

		return *this;
	}

	void updateCost();
	bool canSpendCost(std::string &message) const;
	bool spendCost(std::string &message);
};

/**
 * Window that allows the player
 * to select a battlescape action.
 */
class AirActionMenuState : public State
{
private:

	/**
	* A class that represents a single box in the action popup menu on the air combat state.
	* It shows the possible actions of an item, their TU cost and accuracy.
	* Mouse over highlights the action, when clicked the action is sent to the parent state.
	*/
	class AirActionMenuItem : public InteractiveSurface
	{
	private:
		bool _highlighted;
		AirCombatAction _action;
		int _highlightModifier;
		std::wstring _description;
		CraftWeapon *_weapon;
		Frame *_frame;
		Text *_txtDescription, *_txtAcc, *_txtRange, *_txtTU, *_txtAmmo, *_txtFuel;
	public:
		/// Creates a new ActionMenuItem.
		AirActionMenuItem(int id, Game *game, int x, int y);
		/// Cleans up the ActionMenuItem.
		~AirActionMenuItem();
		/// Assigns an action to it.
		void setAction(AirCombatAction *action, CraftWeapon *weapon, const std::wstring &description, const std::wstring &ammo, const std::wstring &accuracy, const std::wstring &range, const std::wstring &time, const std::wstring &fuel, bool ammoError);
		/// Gets the assigned action.
		const AirCombatAction &getAction() const;
		/// Gets the assigned action TUs.
		int getTUs() const;
		/// Sets the palettes.
		void setPalette(SDL_Color *colors, int firstcolor, int ncolors);
		/// Redraws it.
		void draw();
		/// Processes a mouse hover in event.
		void mouseIn(Action *action, State *state);
		/// Processes a mouse hover out event.
		void mouseOut(Action *action, State *state);
		/// Gets the action description.
		std::wstring getDescription() const;
		/// Gets the action weapon.
		CraftWeapon *getWeapon() const;
	};

	const static int MAX_ACTIONS = 10;

	AirCombatState *_parent;
	AirCombatAction *_action;
	AirActionMenuItem *_actionMenu[MAX_ACTIONS];
	Text *_selectedItem;
	std::unordered_set<SDLKey> _usedHotkeys;
	/// Adds a new menu item for an action.
	void addItem(AirCombatActionType action, CraftWeapon *weapon, const std::string &name, int *id, SDLKey key);

public:
	/// Creates the Action Menu state.
	AirActionMenuState(AirCombatState *parent, AirCombatAction *action, AirCombatActionType options, int x, int y);
	/// Cleans up the Action Menu state.
	~AirActionMenuState();
	/// Init function.
	void init() override;
	/// Handler for right-clicking anything.
	void handle(Action *action);
	/// Handler for clicking a action menu item.
	void btnAirActionMenuItemClick(Action *action);
	/// Update the resolution settings, we just resized the window.
	void resize(int &dX, int &dY);
};

}
