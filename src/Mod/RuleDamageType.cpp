/*
 * Copyright 2010-2015 OpenXcom Developers.
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
#include <cmath>
#include "RuleDamageType.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "Mod.h"

namespace OpenXcom
{

/**
 * Default constructor
 */
RuleDamageType::RuleDamageType() :
	FixRadius(0), RandomType(DRT_DEFAULT), ResistType(DT_NONE), FireBlastCalc(false),
	IgnoreDirection(false), IgnoreSelfDestruct(false), IgnorePainImmunity(false), IgnoreNormalMoraleLose(false), IgnoreOverKill(false),
	ArmorEffectiveness(1.0f), RadiusEffectiveness(0.0f), RadiusReduction(10.0f),
	FireThreshold(1000), SmokeThreshold(1000),
	ToHealth(1.0f), ToArmor(0.1f), ToArmorPre(0.0f), ToWound(1.0f), ToItem(0.0f), ToTile(0.5f), ToStun(0.25f), ToEnergy(0.0f), ToTime(0.0f), ToMorale(0.0f),
	RandomHealth(false), RandomArmor(false), RandomArmorPre(false), RandomWound(true), RandomItem(false), RandomTile(false), RandomStun(true), RandomEnergy(false), RandomTime(false), RandomMorale(false)
{

}
/**
 * Function converting power to damage.
 * @param power Input power.
 * @param mode Calc mode (0 = dmg, 1 = min dmg, 2 = max dmg)
 * @return Random damage based on power.
 */
int RuleDamageType::getRandomDamage(int power, int mode) const
{
	int dmgRng = 0;
	ItemDamageRandomType randType = RandomType;
	if (randType == DRT_UFO_WITH_TWO_DICE)
	{
		if (mode == 0)
		{
			int firstThrow = RNG::generate(0, power);
			int secondThrow = RNG::generate(0, power);
			return firstThrow + secondThrow;
		}
		else if (mode == 1)
		{
			return 0;
		}
		else
		{
			return power * 2;
		}
	}
	const bool def = randType == DRT_DEFAULT;
	if (def)
	{
		switch (ResistType)
		{
		case DT_NONE: randType = DRT_NONE; break;
		case DT_IN: randType = DRT_FIRE; break;
		case DT_HE: randType = DRT_TFTD; break;
		case DT_SMOKE: randType = DRT_NONE; break;
		default: randType = DRT_UFO; break;
		}
	}

	switch (randType)
	{
	case DRT_UFO: dmgRng = (def ? Mod::DAMAGE_RANGE : 100); break;
	case DRT_TFTD: dmgRng = (def ? Mod::EXPLOSIVE_DAMAGE_RANGE : 50); break;
	case DRT_FLAT: dmgRng = 0; break;
	case DRT_FIRE:
		if (mode == 0)
		{
			return RNG::generate(Mod::FIRE_DAMAGE_RANGE[0], Mod::FIRE_DAMAGE_RANGE[1]);
		}
		else if (mode == 1)
		{
			return Mod::FIRE_DAMAGE_RANGE[0];
		}
		else
		{
			return Mod::FIRE_DAMAGE_RANGE[1];
		}
	case DRT_NONE: return 0;
	default: return 0;
	}

	int min = power * (100 - dmgRng) / 100;
	int max = power * (100 + dmgRng) / 100;
	if (mode == 0)
	{
		return RNG::generate(min, max);
	}
	else if (mode == 1)
	{
		return min;
	}
	else
	{
		return max;
	}
}

/**
 * Is this damage single target.
 * @return True if it can only hit one target.
 */
bool RuleDamageType::isDirect() const
{
	return FixRadius == 0;
}

/**
 * Load rule from YAML.
 * @param node Node with data.
 */
void RuleDamageType::load(const YAML::Node& node)
{
	FixRadius = node["FixRadius"].as<int>(FixRadius);
	RandomType = (ItemDamageRandomType)node["RandomType"].as<int>(RandomType);
	ResistType = (ItemDamageType)node["ResistType"].as<int>(ResistType);
	FireBlastCalc = node["FireBlastCalc"].as<bool>(FireBlastCalc);
	IgnoreDirection = node["IgnoreDirection"].as<bool>(IgnoreDirection);
	IgnoreSelfDestruct = node["IgnoreSelfDestruct"].as<bool>(IgnoreSelfDestruct);
	IgnorePainImmunity = node["IgnorePainImmunity"].as<bool>(IgnorePainImmunity);
	IgnoreNormalMoraleLose = node["IgnoreNormalMoraleLose"].as<bool>(IgnoreNormalMoraleLose);
	IgnoreOverKill = node["IgnoreOverKill"].as<bool>(IgnoreOverKill);
	ArmorEffectiveness = node["ArmorEffectiveness"].as<float>(ArmorEffectiveness);
	RadiusEffectiveness = node["RadiusEffectiveness"].as<float>(RadiusEffectiveness);
	RadiusReduction = node["RadiusReduction"].as<float>(RadiusReduction);

	FireThreshold = node["FireThreshold"].as<float>(FireThreshold);
	SmokeThreshold = node["SmokeThreshold"].as<float>(SmokeThreshold);

	ToHealth = node["ToHealth"].as<float>(ToHealth);
	ToArmor = node["ToArmor"].as<float>(ToArmor);
	ToArmorPre = node["ToArmorPre"].as<float>(ToArmorPre);
	ToWound = node["ToWound"].as<float>(ToWound);
	ToItem = node["ToItem"].as<float>(ToItem);
	ToTile = node["ToTile"].as<float>(ToTile);
	ToStun = node["ToStun"].as<float>(ToStun);
	ToEnergy = node["ToEnergy"].as<float>(ToEnergy);
	ToTime = node["ToTime"].as<float>(ToTime);
	ToMorale = node["ToMorale"].as<float>(ToMorale);

	RandomHealth = node["RandomHealth"].as<bool>(RandomHealth);
	RandomArmor = node["RandomArmor"].as<bool>(RandomArmor);
	RandomArmorPre = node["RandomArmorPre"].as<bool>(RandomArmorPre);
	RandomWound = node["RandomWound"].as<bool>(RandomWound);
	RandomItem = node["RandomItem"].as<bool>(RandomItem);
	RandomTile = node["RandomTile"].as<bool>(RandomTile);
	RandomStun = node["RandomStun"].as<bool>(RandomStun);
	RandomEnergy = node["RandomEnergy"].as<bool>(RandomEnergy);
	RandomTime = node["RandomTime"].as<bool>(RandomTime);
	RandomMorale = node["RandomMorale"].as<bool>(RandomMorale);
}

namespace
{
/**
 * Helper function for calculating damage from power.
 * @param random
 * @param multipler
 * @param power
 * @return Damage to somthing.
 */
int getDamageHelper(bool random, float multipler, int power)
{
	if (power > 0)
	{
		if (random)
		{
			return (int)std::round(RNG::generate(0, power) * multipler);
		}
		else
		{
			return (int)std::round(power * multipler);
		}
	}
	return 0;
}

}

/**
 * Get damage value to health based on power.
 */
int RuleDamageType::getHealthDamage(int power) const
{
	return getDamageHelper(RandomHealth, ToHealth, power);
}

/**
 * Get damage value to armor based on power.
 */
int RuleDamageType::getArmorDamage(int power) const
{
	return getDamageHelper(RandomArmor, ToArmor, power);
}

/**
 * Get damage value to armor based on power before armor reduction.
 */
int RuleDamageType::getArmorPreDamage(int power) const
{
	return getDamageHelper(RandomArmorPre, ToArmorPre, power);
}

/**
 * Get numbers of wound based on power.
 */
int RuleDamageType::getWoundDamage(int power) const
{
	if (power > 0)
	{
		if (RandomWound)
		{
			if (RNG::generate(0, 10) < int(power * ToWound))
			{
				return RNG::generate(1,3);
			}
		}
		else
		{
			return (int)std::round(power * ToWound);
		}
	}
	return 0;
}

/**
 * Get damage value to item based on power.
 */
int RuleDamageType::getItemDamage(int power) const
{
	return getDamageHelper(RandomItem, ToItem, power);
}

/**
 * Get damage value to tile based on power.
 */
int RuleDamageType::getTileDamage(int power) const
{
	return getDamageHelper(RandomTile, ToTile, power);
}

/**
 * Get stun level change based on power.
 */
int RuleDamageType::getStunDamage(int power) const
{
	return getDamageHelper(RandomStun, ToStun, power);
}

/**
 * Get energy change based on power.
 */
int RuleDamageType::getEnergyDamage(int power) const
{
	return getDamageHelper(RandomEnergy, ToEnergy, power);
}

/**
 * Get time units change based on power.
 */
int RuleDamageType::getTimeDamage(int power) const
{
	return getDamageHelper(RandomTime, ToTime, power);
}

/**
 * Get morale change based on power.
 */
int RuleDamageType::getMoraleDamage(int power) const
{
	return getDamageHelper(RandomMorale, ToMorale, power);
}

} //namespace OpenXcom
