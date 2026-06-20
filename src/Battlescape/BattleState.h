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
#include "BattlescapeGame.h"

namespace OpenXcom
{

/**
 * This class sets the battlescape in a certain sub-state.
 * These states can be triggered by the player or the AI.
 */
class BattleState
{
protected:
	BattlescapeGame *_parent;
	BattleAction _action;
	/// Whether this state runs concurrently (non-blocking) alongside the main queue.
	/// Set by BattlescapeGame::statePushConcurrent(), not by the state's creator.
	bool _concurrent = false;
public:
	/// Creates a new BattleState linked to the game.
	BattleState(BattlescapeGame *parent, BattleAction action);
	/// Creates a new BattleState linked to the game.
	BattleState(BattlescapeGame *parent);
	/// Cleans up the BattleState.
	virtual ~BattleState();
	/// Initializes the state.
	virtual void init();
	/// Called when the state gets popped out.
	virtual void deinit();
	/// Handles a cancel request.
	virtual void cancel();
	/// Runs state functionality every cycle.
	virtual void think();
	/// Gets the action.
	const BattleAction& getAction() const;
	/// Marks whether this state runs concurrently (called by statePushConcurrent()).
	void setConcurrent(bool concurrent) { _concurrent = concurrent; }
	/// Is this state running concurrently (non-blocking)?
	bool isConcurrent() const { return _concurrent; }
	/// Ends this state through the correct queue (main vs concurrent).
	void finishState();
};

}
