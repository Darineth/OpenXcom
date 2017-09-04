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
#define _USE_MATH_DEFINES
#include "BattleEffect.h"
#include "BattleUnit.h"
#include "../Engine/Game.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Battlescape/TileEngine.h"

namespace OpenXcom
{
	/// Creates a new BattleEffect for a unit and ruleset.
	BattleEffect::BattleEffect(RuleEffect *rules, BattleUnit *unit, BattleUnit *source) : _rules(rules), _type(rules->getType()), _unit(unit), _source(source), _durationRemaining(rules->getDuration()), _loadSourceId(-1)
	{
	}

	/// Cleans up the BattleEffect.
	BattleEffect::~BattleEffect()
	{
	}

	/// Loads the unit from YAML.
	void BattleEffect::load(const YAML::Node& node)
	{
		_durationRemaining = node["durationRemaining"].as<int>(_durationRemaining);
		_loadSourceId = node["sourceId"].as<int>(_loadSourceId);
	}

	/// Saves the unit to YAML.
	YAML::Node BattleEffect::save() const
	{
		YAML::Node n;
		n["type"] = _type;
		n["durationRemaining"] = _durationRemaining;
		n["loadSourceId"] = _source ? _source->getId() : -1;
		return n;
	}

	void BattleEffect::initLoaded(SavedBattleGame *save)
	{
		if (_loadSourceId > -1)
		{
			std::vector<BattleUnit*> *units = save->getUnits();
			for (std::vector<BattleUnit*>::const_iterator ii = units->begin(); ii != units->end(); ++ii)
			{
				if ((*ii)->getId() == _loadSourceId)
				{
					_source = *ii;
					break;
				}
			}
			_loadSourceId = -1;
		}
	}

	/// Gets the rules for the effect.
	RuleEffect *BattleEffect::getRules() const
	{
		return _rules;
	}

	/// Gets the remaining duration.
	int BattleEffect::getDurationRemaining() const
	{
		return _durationRemaining;
	}

	/// Gets the unit the effect is applied to.
	BattleUnit *BattleEffect::getUnit() const
	{
		return _unit;
	}

	/// Applies the initial effects
	bool BattleEffect::apply(TileEngine *tiles)
	{
		if (_unit->getStatus() == STATUS_DEAD)
		{
			delete this;
			return false;
		}

		std::vector<BattleEffect*> &unitEffects = _unit->getActiveEffects();

		if (unitEffects.size())
		{
			if (_rules->getEffectClass().size())
			{
				for (auto &ii = unitEffects.cbegin(); ii != unitEffects.cend(); ++ii)
				{
					if ((*ii)->prevents(_rules->getEffectClass()))
					{
						delete this;
						return false;
					}
				}
			}

			for (auto &ii = unitEffects.cbegin(); ii != unitEffects.cend();)
			{
				BattleEffect *effect = *ii;
				if (prevents(effect->getRules()->getEffectClass()))
				{
					effect->cancel(false);
					delete effect;
					ii = unitEffects.erase(ii);
				}
				else
				{
					++ii;
				}
			}

			auto firstStack = unitEffects.cend();
			int stackCount = 0;
			for (auto &ii = unitEffects.cbegin(); ii != unitEffects.cend();)
			{
				BattleEffect *effect = *ii;
				if (_rules == effect->_rules)
				{
					if (_rules->getMaxStack() <= 1)
					{
						if (_rules->getInitialComponents().size() || _rules->getFinalComponents().size())
						{
							effect->cancel(false);
							delete effect;
							ii = unitEffects.erase(ii);
						}
						else
						{
							effect->refresh();
							delete this;
							return false;
						}
					}
					else
					{
						firstStack = ii;
						++stackCount;
						++ii;
					}
				}
				else
				{
					++ii;
				}
			}

			if (stackCount > _rules->getMaxStack())
			{
				(*firstStack)->cancel();
			}
		}

		if (tiles)
		{
			tiles->displayDamage(_source, _unit, _rules->getIsNegative() ? BA_EFFECT_NEGATIVE : BA_EFFECT_POSITIVE, DT_NONE, 0, 0, 0, _type);
		}

		applyComponents(_rules->getInitialComponents(), tiles);
		if (_durationRemaining != 0)
		{
			_unit->getActiveEffects().push_back(this);
			_unit->updateStats();
			if (getComponent(EC_STEALTH))
			{
				_unit->invalidateCache();
			}
		}
		else
		{
			delete this;
		}

		return true;
	}

	/// Update the effect for a new turn and apply its effects.
	bool BattleEffect::prepareNewTurn(TileEngine *tiles)
	{
		if (_durationRemaining == 0)
		{
			applyComponents(_rules->getFinalComponents(), tiles);
			delete this;
			return false;
		}
		else
		{
			applyComponents(_rules->getOngoingComponents(), tiles);

			if (_durationRemaining > 0)
			{
				--_durationRemaining;
			}

			return true;
		}
	}

	/// Resets the duration on the effect.
	void BattleEffect::refresh()
	{
		if (_durationRemaining > 0)
		{
			_durationRemaining = _rules->getDuration();
		}
	}

	/// Removes the effect from the unit.
	void BattleEffect::cancel(bool removeFromUnit)
	{
		if (getComponent(EC_CIRCULAR_LIGHT) || getComponent(EC_DIRECTIONAL_LIGHT) || getComponent(EC_STEALTH))
		{
			_unit->invalidateCache();
			TileEngine *tiles = Game::getGame()->getSavedGame()->getSavedBattle()->getTileEngine();
			if (tiles) { tiles->calculateUnitLighting(); }
		}

		if (removeFromUnit)
		{
			std::vector<BattleEffect*> &unitEffects = _unit->getActiveEffects();
			std::vector<BattleEffect*>::const_iterator ii = std::find(unitEffects.cbegin(), unitEffects.cend(), this);
			if (ii != unitEffects.cend())
			{
				unitEffects.erase(ii);
			}

			delete this;
		}
	}

	/// Check if the effect has finished.
	bool BattleEffect::getFinished()
	{
		return _durationRemaining == 0;
	}

	void BattleEffect::applyComponents(const std::vector<EffectComponent*> &components, TileEngine *tiles) const
	{
		if (_unit->getStatus() == STATUS_DEAD)
		{
			return;
		}

		for (std::vector<EffectComponent*>::const_iterator ii = components.begin(); ii != components.end(); ++ii)
		{
			applyComponent(*ii, tiles);
		}
	}

	void BattleEffect::applyComponent(const EffectComponent *component, TileEngine *tiles) const
	{
		if (_unit->getStatus() == STATUS_DEAD)
		{
			return;
		}

		switch (component->type)
		{
		case EC_HEALTH:
			if (component->magnitude > 0)
			{
				_unit->damage(tiles, _source, Position(), (int)component->magnitude, component->damageType, component->ignoreArmor, 0, _type);
			}
			else
			{
				_unit->heal(0, 0, -(int)component->magnitude);
			}

			break;

		case EC_STEALTH:
		case EC_CIRCULAR_LIGHT:
		case EC_DIRECTIONAL_LIGHT:
			{
				TileEngine *tiles = Game::getGame()->getSavedGame()->getSavedBattle()->getTileEngine();
				if (tiles) { tiles->calculateUnitLighting(); }
			}
			break;
		}
	}

	/// Check if this effect blocks the indicated class.
	bool BattleEffect::prevents(const std::string &effectClass) const
	{
		if (effectClass.size())
		{
			auto components = _rules->getOngoingComponents();

			for (auto &ii = components.cbegin(); ii != components.cend(); ++ii)
			{
				EffectComponent *effect = *ii;

				if (effect->type == EC_PREVENT_EFFECT && effect->affectsEffectClass == effectClass)
				{
					return true;
				}
			}
		}

		return false;
	}

	const EffectComponent* BattleEffect::getComponent(EffectComponentType type) const
	{
		const std::vector<EffectComponent*> &components = _rules->getOngoingComponents();
		for (std::vector<EffectComponent*>::const_iterator ii = components.begin(); ii != components.end(); ++ii)
		{
			EffectComponent *ec = *ii;
			if (ec->type == type) { return ec; }
		}

		return 0;
	}

	bool BattleEffect::cancelByTrigger(EffectTrigger trigger)
	{
		if (_rules->cancelledByTrigger(trigger))
		{
			cancel(false);
			delete this;
			return true;
		}

		return false;
	}
}