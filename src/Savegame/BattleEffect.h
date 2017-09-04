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
#include <string>
#include "../Mod/RuleEffect.h"

namespace OpenXcom
{
	class BattleUnit;
	class SavedBattleGame;
	class TileEngine;

/**
 * Represents an effect applied to a unit.
 */
class BattleEffect
{
private:
	std::string _type;
	RuleEffect *_rules;
	BattleUnit *_unit;
	BattleUnit *_source;
	int _durationRemaining;
	int _loadSourceId;

	void applyComponents(const std::vector<EffectComponent*> &effects, TileEngine *tiles) const;
	void applyComponent(const EffectComponent *effect, TileEngine *tiles) const;

	void finish();

public:
	/// Creates a new BattleEffect for a unit and ruleset.
	BattleEffect(RuleEffect *rules, BattleUnit *unit, BattleUnit *source);

	/// Cleans up the BattleEffect.
	~BattleEffect();

	/// Loads the unit from YAML.
	void load(const YAML::Node& node);

	/// Saves the unit to YAML.
	YAML::Node save() const;

	/// Finishes initializing the effect after a save is loaded.
	void initLoaded(SavedBattleGame *save);

	/// Gets the rules for the effect.
	RuleEffect *getRules() const;

	/// Gets the remaining duration.
	int getDurationRemaining() const;

	/// Gets if this represents the source unit effects.
	bool getSourceEffects() const;

	/// Gets the unit the effect is applied to.
	BattleUnit *getUnit() const;

	/// Applies the initial effects
	bool apply(TileEngine *tiles);

	/// Update the effect for a new turn and apply its effects.
	bool prepareNewTurn(TileEngine *tiles);

	/// Resets the duration on the effect.
	void refresh();

	/// Removes the effect from the unit.
	void cancel(bool removeFromUnit = true);

	/// Check if the effect has finished.
	bool getFinished();

	/// Check if this effect blocks the indicated class.
	bool prevents(const std::string &effectClass) const;

	bool cancelByTrigger(EffectTrigger trigger);

	const EffectComponent* getComponent(EffectComponentType type) const;
};

}
