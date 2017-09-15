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
#include "Unit.h"
#include "RuleStatBonus.h"
#include "../Engine/RNG.h"
#include "../Savegame/BattleUnit.h"
#include "../fmath.h"

namespace OpenXcom
{

namespace
{

typedef float (*BonusStatFunc)(const BattleUnit*);

/**
 * Function returning static value.
 */
template<int I>
float stat0(const BattleUnit *unit)
{
	return I;
}
/**
 * Getter for one basic stat of unit.
 */
template<int UnitStats::* field>
float stat1(const BattleUnit *unit)
{
	const UnitStats *stat = unit->getBaseStats();
	return stat->*field;
}

/**
 * Getter for multiply of two basic stat of unit.
 */
template<int UnitStats::* fieldA, int UnitStats::* fieldB>
float stat2(const BattleUnit *unit)
{
	const UnitStats *stat = unit->getBaseStats();
	return (stat->*fieldA) * (stat->*fieldB);
}

float currentFatalWounds(const BattleUnit *unit)
{
	return unit->getFatalWounds();
}

float currentRank(const BattleUnit *unit)
{
	return unit->getRankInt();
}

float curretTimeUnits(const BattleUnit *unit)
{
	return unit->getTimeUnits();
}

float curretHealth(const BattleUnit *unit)
{
	return unit->getHealth();
}

float curretEnergy(const BattleUnit *unit)
{
	return unit->getEnergy();
}

float curretMorale(const BattleUnit *unit)
{
	return unit->getMorale();
}

float curretStun(const BattleUnit *unit)
{
	return unit->getStunlevel();
}


float normalizedTimeUnits(const BattleUnit *unit)
{
	return 1.0f * unit->getTimeUnits()/ unit->getBaseStats()->tu;
}

float normalizedHealth(const BattleUnit *unit)
{
	return 1.0f * unit->getHealth() / unit->getBaseStats()->health;
}

float normalizedEnergy(const BattleUnit *unit)
{
	return 1.0f * unit->getEnergy() / unit->getBaseStats()->stamina;
}

float normalizedMorale(const BattleUnit *unit)
{
	return 1.0f * unit->getMorale() / 100;
}

float normalizedStun(const BattleUnit *unit)
{
	int health = unit->getHealth();
	if (health > 0)
	{
		return 1.0f * unit->getStunlevel() / health;
	}
	else
	{
		return 0.0f;
	}
}

float basicEnergyRegeneration(const BattleUnit *unit)
{
	Soldier *solder = unit->getGeoscapeSoldier();
	if (solder != 0)
	{
		return solder->getInitStats()->tu / 3;
	}
	else
	{
		return unit->getUnitRules()->getEnergyRecovery();
	}
}

template<BonusStatFunc func, int p>
float power(const BattleUnit *unit)
{
	return std::pow(func(unit), p);
}

const size_t statDataFuncSize = 3;

/**
 * Data describing same functions but with different exponent.
 */
struct BonusStatDataFunc
{
	BonusStatFunc power[statDataFuncSize];
};
/**
 * Data describing basic stat getter.
 */
struct BonusStatData
{
	std::string name;
	BonusStatDataFunc func;
};

/**
 * Helper function creating BonusStatData with proper functions.
 */
template<BonusStatFunc func>
BonusStatDataFunc create()
{
	BonusStatDataFunc data =
	{
		{
			&power< func, 1>,
			&power< func, 2>,
			&power< func, 3>,
		}
	};
	return data;
}

/**
 * Helper function creating BonusStatData with proper functions.
 */
template<int Val>
BonusStatDataFunc create0()
{
	return create<&stat0<Val> >();
}

/**
 * Helper function creating BonusStatData with proper functions.
 */
template<int UnitStats::* fieldA>
BonusStatDataFunc create1()
{
	return create<&stat1<fieldA> >();
}

/**
 * Helper function creating BonusStatData with proper functions.
 */
template<int UnitStats::* fieldA, int UnitStats::* fieldB>
BonusStatDataFunc create2()
{
	return create<&stat2<fieldA, fieldB> >();
}

/**
 * List of all possible getters of basic stats.
 */
BonusStatData statDataMap[] =
{
	{ "flatOne", create0<1>() },
	{ "flatHundred", create0<100>() },
	{ "strength", create1<&UnitStats::strength>() },
	{ "psi", create2<&UnitStats::psiSkill, &UnitStats::psiStrength>() },
	{ "psiSkill", create1<&UnitStats::psiSkill>() },
	{ "psiStrength", create1<&UnitStats::psiStrength>() },
	{ "throwing", create1<&UnitStats::throwing>() },
	{ "bravery", create1<&UnitStats::bravery>() },
	{ "firing", create1<&UnitStats::firing>() },
	{ "health", create1<&UnitStats::health>() },
	{ "tu", create1<&UnitStats::tu>() },
	{ "reactions", create1<&UnitStats::reactions>() },
	{ "stamina", create1<&UnitStats::stamina>() },
	{ "melee", create1<&UnitStats::melee>() },
	{ "strengthMelee", create2<&UnitStats::strength, &UnitStats::melee>() },
	{ "strengthThrowing", create2<&UnitStats::strength, &UnitStats::throwing>() },
	{ "firingReactions", create2<&UnitStats::firing, &UnitStats::reactions>() },

	{ "rank", create<&currentRank>() },
	{ "fatalWounds", create<&currentFatalWounds>() },

	{ "healthCurrent", create<&curretHealth>() },
	{ "tuCurrent", create<&curretTimeUnits>() },
	{ "energyCurrent", create<&curretEnergy>() },
	{ "moraleCurrent", create<&curretMorale>() },
	{ "stunCurrent", create<&curretStun>() },

	{ "healthNormalized", create<&normalizedHealth>() },
	{ "tuNormalized", create<&normalizedTimeUnits>() },
	{ "energyNormalized", create<&normalizedEnergy>() },
	{ "moraleNormalized", create<&normalizedMorale>() },
	{ "stunNormalized", create<&normalizedStun>() },

	{ "energyRegen", create<&basicEnergyRegeneration>() },
};
const size_t statDataMapSize = sizeof(statDataMap) / sizeof(statDataMap[0]);

} //namespace

/**
 * Default constructor.
 */
RuleStatBonus::RuleStatBonus()
{

}
/**
 * Loads the item from a YAML file.
 * @param node YAML node.
 */
void RuleStatBonus::load(const YAML::Node& node)
{
	if (node)
	{
		_bonus.clear();
		for (size_t i = 0; i < statDataMapSize; ++i)
		{
			if (const YAML::Node &dd = node[statDataMap[i].name])
			{
				if (dd.IsScalar())
				{
					_bonus.push_back(std::make_pair(statDataMap[i].func.power[0], dd.as<float>()));
				}
				else
				{
					for (size_t j = 0; j < statDataFuncSize && j < dd.size(); ++j)
					{
						float val = dd[j].as<float>();
						if (!AreSame(val, 0.0f))
						{
							_bonus.push_back(std::make_pair(statDataMap[i].func.power[j], val));
						}
					}
				}
			}
		}
	}
}

/**
 * Set default bonus for firing accuracy.
 */
void RuleStatBonus::setFiring()
{
	_bonus.clear();
	_bonus.push_back(RuleStatBonusData(&stat1<&UnitStats::firing>, 1.0f));
}

/**
 * Set default bonus for melee accuracy.
 */
void RuleStatBonus::setMelee()
{
	_bonus.clear();
	_bonus.push_back(RuleStatBonusData(&stat1<&UnitStats::melee>, 1.0f));
}

/**
 * Set default bonus for throwing accuracy.
 */
void RuleStatBonus::setThrowing()
{
	_bonus.clear();
	_bonus.push_back(RuleStatBonusData(&stat1<&UnitStats::throwing>, 1.0f));
}

/**
 * Set default bonus for close quarters combat
 */
void RuleStatBonus::setCloseQuarters()
{
	_bonus.clear();
	_bonus.push_back(RuleStatBonusData(&stat1<&UnitStats::melee>, 0.5f));
	_bonus.push_back(RuleStatBonusData(&stat1<&UnitStats::reactions>, 0.5f));
}

/**
 * Set default bonus for psi attack accuracy.
 */
void RuleStatBonus::setPsiAttack()
{
	_bonus.clear();
	_bonus.push_back(RuleStatBonusData(&stat2<&UnitStats::psiSkill, &UnitStats::psiStrength>, 0.02f));
}

/**
 * Set default bonus for psi defense.
 */
void RuleStatBonus::setPsiDefense()
{
	_bonus.clear();
	_bonus.push_back(RuleStatBonusData(&stat1<&UnitStats::psiStrength>, 1.0f));
	_bonus.push_back(RuleStatBonusData(&stat1<&UnitStats::psiSkill>, 0.2f));
}

/**
 * Set flat bonus equal 100% hit chance.
 */
void RuleStatBonus::setFlatHundred()
{
	_bonus.clear();
	_bonus.push_back(RuleStatBonusData(&stat0<100>, 1.0f));
}

/**
 * Set default bonus for melee power.
 */
void RuleStatBonus::setStrength()
{
	_bonus.clear();
	_bonus.push_back(RuleStatBonusData(&stat1<&UnitStats::strength>, 1.0f));
}

/**
 *  Set default for TU recovery.
 */
void RuleStatBonus::setTimeRecovery()
{
	_bonus.clear();
	_bonus.push_back(RuleStatBonusData(&stat1<&UnitStats::tu>, 1.0f));
}

/**
 * Set default for Energy recovery.
 */
void RuleStatBonus::setEnergyRecovery()
{
	_bonus.clear();
	_bonus.push_back(RuleStatBonusData(&basicEnergyRegeneration, 1.0f));
}

/**
 *  Set default for Stun recovery.
 */
void RuleStatBonus::setStunRecovery()
{
	_bonus.clear();
	_bonus.push_back(RuleStatBonusData(&stat0<1>, 1.0f));
}

/**
 * Calculate bonus based on unit stats.
 */
int RuleStatBonus::getBonus(const BattleUnit* unit) const
{
	float power = 0;
	for (size_t i = 0; i < _bonus.size(); ++i)
		power += _bonus[i].first(unit) * _bonus[i].second;

	return (int)floor(power + 0.5f); // no more random flickering!
}

} //namespace OpenXcom
