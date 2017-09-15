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
#ifndef OPENXCOM_RULESTATBONUS_H
#define	OPENXCOM_RULESTATBONUS_H

#include<vector>
#include<string>

namespace OpenXcom
{

class BattleUnit;
typedef std::pair<float (*)(const BattleUnit*), float> RuleStatBonusData;
/**
 * Helper class used for storing unit stat bonuses.
 */
class RuleStatBonus
{
	std::vector<RuleStatBonusData> _bonus;
public:
	/// Default constructor.
	RuleStatBonus();
	/// Loads item data from YAML.
	void load(const YAML::Node& node);
	/// Set default firing bonus.
	void setFiring();
	/// Set default melee bonus.
	void setMelee();
	/// Set default throwing bonus.
	void setThrowing();
	/// Set default close quarters combat bonus.
	void setCloseQuarters();
	/// Set default psi attack bonus.
	void setPsiAttack();
	/// Set default psi defense bonus.
	void setPsiDefense();
	/// Set default strength bonus.
	void setStrength();
	/// Set flat 100 bonus.
	void setFlatHundred();
	/// Set default for TU recovery.
	void setTimeRecovery();
	/// Set default for Energy recovery.
	void setEnergyRecovery();
	/// Set default for Stun recovery.
	void setStunRecovery();
	/// Get bonus based on unit stats.
	int getBonus(const BattleUnit* unit) const;
};

}

#endif	/* OPENXCOM_RULESTATBONUS_H */

