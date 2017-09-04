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

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include <hash_set>
#include "Mod.h"
#include "Unit.h"
#include "RuleItem.h"

namespace OpenXcom
{
enum EffectComponentType { EC_STAT_MODIFIER = 1, EC_MAX_HEALTH = 2, EC_MAX_ENERGY = 3, EC_MAX_TU = 4, EC_HEALTH = 11, EC_ENERGY = 12, EC_TU = 13, EC_REMOVE_EFFECT = 21, EC_PREVENT_EFFECT = 22, EC_STEALTH = 30, EC_CIRCULAR_LIGHT = 40, EC_DIRECTIONAL_LIGHT = 41, EC_NIGHT_VISION = 42 };
enum EffectTrigger { ECT_MOVE = 1, ECT_WALK = 2, ECT_SNEAK = 3, ECT_SPRINT = 4, ECT_TURN = 5, ECT_ACTIVATE = 10, ECT_ATTACK = 11, ECT_MELEE = 12, ECT_SHOOT = 13, ECT_THROW = 14 };

struct EffectComponent
{
private:
	EffectComponent(const EffectComponent& copy);

public:
	EffectComponentType type;
	bool magnitudePercent;
	double magnitude;
	UnitStats stats;
	bool hasStats;
	std::string affectsEffectClass;
	ItemDamageType damageType;
	bool ignoreArmor;

	EffectComponent() : type(EC_STAT_MODIFIER), magnitude(0.0), stats(), hasStats(false), magnitudePercent(false), affectsEffectClass(), damageType(DT_NONE), ignoreArmor(false){};
	~EffectComponent() {}
};

/**
	* Represents an effect that can be applied to units by weapons, abilities, etc.
	*/
class RuleEffect
{
private:
	std::string _type;
	std::string _icon;
	std::string _effectClass;
	std::vector<EffectComponent*> _initialComponents;
	std::vector<EffectComponent*> _ongoingComponents;
	std::vector<EffectComponent*> _finalComponents;
	std::hash_set<EffectTrigger> _cancelTriggers;
	int _duration;
	int _maxStack;
	bool _isNegative;

public:
	/// Creates a blank effect ruleset.
	RuleEffect(const std::string &type);

	/// Cleans up the effect ruleset.
	~RuleEffect();

	/// Load the effect rule from YAML.
	void load(const YAML::Node& node, Mod *mod);

	/// Gets the effect's type.
	const std::string &getType() const;

	/// Gets the effect's icon.
	const std::string &getIcon() const;

	/// Gets the effect's class.  Affects stacking and removal.
	const std::string &getEffectClass() const;

	/// Gets the effect's duration.
	int getDuration() const;

	/// Gets the maximum stack level.
	int getMaxStack() const;

	/// Gets the initial components of the effect.
	const std::vector<EffectComponent*> &getInitialComponents() const;

	/// Gets the ongoing components of the effect.
	const std::vector<EffectComponent*> &getOngoingComponents() const;

	/// Gets the final components of the effect.
	const std::vector<EffectComponent*> &getFinalComponents() const;

	bool cancelledByTrigger(EffectTrigger trigger) const;

	bool getIsNegative() const;
};
}

namespace YAML
{
template<>
struct convert < OpenXcom::EffectComponent* >
{
	static Node encode(const OpenXcom::EffectComponent*& rhs)
	{
		Node node;
		node["type"] = (int)rhs->type;
		node["magnitude"] = rhs->magnitude;
		node["hasStats"] = rhs->hasStats;
		node["stats"] = rhs->stats;
		node["magnitudePercent"] = rhs->magnitudePercent;
		node["affectsEffectClass"] = rhs->affectsEffectClass;
		node["damageType"] = (int)rhs->damageType;
		node["ignoreArmor"] = rhs->ignoreArmor;
		return node;
	}

	static bool decode(const Node& node, OpenXcom::EffectComponent*& rhs)
	{
		rhs = new OpenXcom::EffectComponent();
		rhs->type = (OpenXcom::EffectComponentType)node["type"].as<int>(rhs->type);
		rhs->magnitude = node["magnitude"].as<double>(rhs->magnitude);
		rhs->hasStats = node["hasStats"].as<bool>(rhs->hasStats);
		rhs->stats = node["stats"].as<OpenXcom::UnitStats>(rhs->stats);
		rhs->magnitudePercent = node["statPercentages"].as<bool>(rhs->magnitudePercent);
		rhs->affectsEffectClass = node["affectsEffectClass"].as<std::string>(rhs->affectsEffectClass);
		rhs->damageType = (OpenXcom::ItemDamageType)node["damageType"].as<int>(rhs->damageType);
		rhs->ignoreArmor = node["ignoreArmor"].as<bool>(rhs->ignoreArmor);
		return true;
	}
};

template<>
struct convert < std::hash_set<OpenXcom::EffectTrigger> >
{
	static bool decode(const Node &node, std::hash_set<OpenXcom::EffectTrigger> &rhs)
	{
		if (node.IsSequence())
		{
			rhs.clear();

			for (Node::const_iterator ii = node.begin(); ii != node.end(); ++ii)
			{
				rhs.insert(static_cast<OpenXcom::EffectTrigger>(ii->as<int>()));
			}
			return true;
		}

		return false;
	}
};
}
