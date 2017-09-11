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
#include "RuleEffect.h"

namespace OpenXcom
{
/// Creates a blank effect ruleset.
RuleEffect::RuleEffect(const std::string &type) : _type(type), _icon(), _initialComponents(), _ongoingComponents(), _finalComponents(), _cancelTriggers(), _duration(0), _maxStack(0), _isNegative(false), _effectClass()
{
}

/// Cleans up the effect ruleset.
RuleEffect::~RuleEffect()
{
	for (auto &ii = _initialComponents.begin(); ii != _initialComponents.end(); ++ii)
	{
		delete *ii;
	}
	for (auto &ii = _ongoingComponents.begin(); ii != _ongoingComponents.end(); ++ii)
	{
		delete *ii;
	}
	for (auto &ii = _finalComponents.begin(); ii != _finalComponents.end(); ++ii)
	{
		delete *ii;
	}
}

/// Load the effect rule from YAML.
void RuleEffect::load(const YAML::Node& node, Mod *mod)
{
	if (const YAML::Node &parent = node["refNode"])
	{
		load(parent, mod);
	}
	_icon = node["icon"].as<std::string>(_icon);
	_effectClass = node["effectClass"].as<std::string>(_effectClass);
	_initialComponents = node["initialComponents"].as< std::vector<EffectComponent*> >(_initialComponents);
	_ongoingComponents = node["ongoingComponents"].as< std::vector<EffectComponent*> >(_ongoingComponents);
	_finalComponents = node["finalComponents"].as< std::vector<EffectComponent*> >(_finalComponents);
	_cancelTriggers = node["cancelTriggers"].as< std::hash_set<EffectTrigger> >(_cancelTriggers);
	_duration = node["duration"].as<int>(_duration);
	_maxStack = node["maxStack"].as<int>(_maxStack);
	_isNegative = node["isNegative"].as<bool>(_isNegative);
}

/// Gets the effect's type.
const std::string &RuleEffect::getType() const
{
	return _type;
}

/// Gets the effect's icon.
const std::string &RuleEffect::getIcon() const
{
	return _icon;
}

const std::string &RuleEffect::getEffectClass() const
{
	return _effectClass;
}

/// Gets the effect's duration.
int RuleEffect::getDuration() const
{
	return _duration;
}

int RuleEffect::getMaxStack() const
{
	return _maxStack;
}

/// Gets the components of the effect.
const std::vector<EffectComponent*> &RuleEffect::getInitialComponents() const
{
	return _initialComponents;
}

/// Gets the components of the effect.
const std::vector<EffectComponent*> &RuleEffect::getOngoingComponents() const
{
	return _ongoingComponents;
}

/// Gets the components of the effect.
const std::vector<EffectComponent*> &RuleEffect::getFinalComponents() const
{
	return _finalComponents;
}

bool RuleEffect::cancelledByTrigger(EffectTrigger trigger) const
{
	return _cancelTriggers.size() > 0 && _cancelTriggers.find(trigger) != _cancelTriggers.cend();
}

bool RuleEffect::getIsNegative() const
{
	return _isNegative;
}
}
