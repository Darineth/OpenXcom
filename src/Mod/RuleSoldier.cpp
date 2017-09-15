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
#include "RuleSoldier.h"
#include "Mod.h"
#include "SoldierNamePool.h"
#include "StatString.h"
#include "../Engine/FileMap.h"

namespace OpenXcom
{

/**
 * Creates a blank ruleunit for a certain
 * type of soldier.
 * @param type String defining the type.
 */
RuleSoldier::RuleSoldier(const std::string &type) : _type(type), _costBuy(0), _costSalary(0),
	_costSalarySquaddie(0), _costSalarySergeant(0), _costSalaryCaptain(0), _costSalaryColonel(0), _costSalaryCommander(0),
	_standHeight(0), _kneelHeight(0), _floatHeight(0), _femaleFrequency(50), _avatarOffsetX(66), _avatarOffsetY(42), _flagOffset(0),
	_isVehicle(false), _requires(), _inventoryLayout("STR_STANDARD_INV"), _levelExperience(),
	_allowPromotion(true), _allowPiloting(true)
{
}

/**
 *
 */
RuleSoldier::~RuleSoldier()
{
	for (std::vector<SoldierNamePool*>::iterator i = _names.begin(); i != _names.end(); ++i)
	{
		delete *i;
	}
	for (std::vector<StatString*>::iterator i = _statStrings.begin(); i != _statStrings.end(); ++i)
	{
		delete *i;
	}
}

/**
 * Loads the soldier from a YAML file.
 * @param node YAML node.
 * @param mod Mod for the unit.
 */
void RuleSoldier::load(const YAML::Node &node, Mod *mod)
{
	if (const YAML::Node &parent = node["refNode"])
	{
		load(parent, mod);
	}
	_type = node["type"].as<std::string>(_type);
	// Just in case
	if (_type == "XCOM")
		_type = "STR_SOLDIER";
	_requires = node["requires"].as< std::vector<std::string> >(_requires);
	_minStats.merge(node["minStats"].as<UnitStats>(_minStats));
	_maxStats.merge(node["maxStats"].as<UnitStats>(_maxStats));
	_statCaps.merge(node["statCaps"].as<UnitStats>(_statCaps));
	if (node["trainingStatCaps"])
	{
		_trainingStatCaps.merge(node["trainingStatCaps"].as<UnitStats>(_trainingStatCaps));
	}
	else
	{
		_trainingStatCaps.merge(node["statCaps"].as<UnitStats>(_trainingStatCaps));
	}
	_dogfightExperience.merge(node["dogfightExperience"].as<UnitStats>(_dogfightExperience));
	_armor = node["armor"].as<std::string>(_armor);
	_armorForAvatar = node["armorForAvatar"].as<std::string>(_armorForAvatar);
	_avatarOffsetX = node["avatarOffsetX"].as<int>(_avatarOffsetX);
	_avatarOffsetY = node["avatarOffsetY"].as<int>(_avatarOffsetY);
	_flagOffset = node["flagOffset"].as<int>(_flagOffset);
	_allowPromotion = node["allowPromotion"].as<bool>(_allowPromotion);
	_allowPiloting = node["allowPiloting"].as<bool>(_allowPiloting);
	_costBuy = node["costBuy"].as<int>(_costBuy);
	_costSalary = node["costSalary"].as<int>(_costSalary);
	_costSalarySquaddie = node["costSalarySquaddie"].as<int>(_costSalarySquaddie);
	_costSalarySergeant = node["costSalarySergeant"].as<int>(_costSalarySergeant);
	_costSalaryCaptain = node["costSalaryCaptain"].as<int>(_costSalaryCaptain);
	_costSalaryColonel = node["costSalaryColonel"].as<int>(_costSalaryColonel);
	_costSalaryCommander = node["costSalaryCommander"].as<int>(_costSalaryCommander);
	_standHeight = node["standHeight"].as<int>(_standHeight);
	_kneelHeight = node["kneelHeight"].as<int>(_kneelHeight);
	_floatHeight = node["floatHeight"].as<int>(_floatHeight);
	_femaleFrequency = node["femaleFrequency"].as<int>(_femaleFrequency);

	if (node["deathMale"])
	{
		_deathSoundMale.clear();
		if (node["deathMale"].IsSequence())
		{
			for (YAML::const_iterator i = node["deathMale"].begin(); i != node["deathMale"].end(); ++i)
			{
				_deathSoundMale.push_back(mod->getSoundOffset(i->as<int>(), "BATTLE.CAT"));
			}
		}
		else
		{
			_deathSoundMale.push_back(mod->getSoundOffset(node["deathMale"].as<int>(), "BATTLE.CAT"));
		}
	}
	if (node["deathFemale"])
	{
		_deathSoundFemale.clear();
		if (node["deathFemale"].IsSequence())
		{
			for (YAML::const_iterator i = node["deathFemale"].begin(); i != node["deathFemale"].end(); ++i)
			{
				_deathSoundFemale.push_back(mod->getSoundOffset(i->as<int>(), "BATTLE.CAT"));
			}
		}
		else
		{
			_deathSoundFemale.push_back(mod->getSoundOffset(node["deathFemale"].as<int>(), "BATTLE.CAT"));
		}
	}

	for (YAML::const_iterator i = node["soldierNames"].begin(); i != node["soldierNames"].end(); ++i)
	{
		std::string fileName = (*i).as<std::string>();
		if (fileName == "delete")
		{
			for (std::vector<SoldierNamePool*>::iterator j = _names.begin(); j != _names.end(); ++j)
			{
				delete *j;
			}
			_names.clear();
		}
		else
		{
			if (fileName.substr(fileName.length() - 1, 1) == "/")
			{
				// load all *.nam files in given directory
				std::set<std::string> names = FileMap::filterFiles(FileMap::getVFolderContents(fileName), "nam");
				for (std::set<std::string>::iterator j = names.begin(); j != names.end(); ++j)
				{
					addSoldierNamePool(fileName + *j);
				}
			}
			else
			{
				// load given file
				addSoldierNamePool(fileName);
			}
		}
	}

	for (YAML::const_iterator i = node["statStrings"].begin(); i != node["statStrings"].end(); ++i)
	{
		StatString *statString = new StatString();
		statString->load(*i);
		_statStrings.push_back(statString);
	}

	_isVehicle = node["isVehicle"].as<bool>(_isVehicle);
	_inventoryLayout = node["inventoryLayout"].as<std::string>(_inventoryLayout);
	_levelExperience = node["levelExperience"].as<std::vector<int> >(_levelExperience);
}

void RuleSoldier::addSoldierNamePool(const std::string &namFile)
{
	SoldierNamePool *pool = new SoldierNamePool();
	pool->load(FileMap::getFilePath(namFile));
	_names.push_back(pool);
}

/**
 * Returns the language string that names
 * this soldier. Each soldier type has a unique name.
 * @return Soldier name.
 */
std::string RuleSoldier::getType() const
{
	return _type;
}

/**
 * Gets the list of research required to
 * acquire this soldier.
 * @return The list of research IDs.
*/
const std::vector<std::string> &RuleSoldier::getRequirements() const
{
	return _requires;
}

/**
 * Gets the minimum stats for the random stats generator.
 * @return The minimum stats.
 */
UnitStats RuleSoldier::getMinStats() const
{
	return _minStats;
}

/**
 * Gets the maximum stats for the random stats generator.
 * @return The maximum stats.
 */
UnitStats RuleSoldier::getMaxStats() const
{
	return _maxStats;
}

/**
 * Gets the stat caps.
 * @return The stat caps.
 */
UnitStats RuleSoldier::getStatCaps() const
{
	return _statCaps;
}

/**
* Gets the training stat caps.
* @return The training stat caps.
*/
UnitStats RuleSoldier::getTrainingStatCaps() const
{
	return _trainingStatCaps;
}

/**
* Gets the improvement chances for pilots (after dogfight).
* @return The improvement changes.
*/
UnitStats RuleSoldier::getDogfightExperience() const
{
	return _dogfightExperience;
}

/**
 * Gets the cost of hiring this soldier.
 * @return The cost.
 */
int RuleSoldier::getBuyCost() const
{
	return _costBuy;
}

/**
* Does salary depend on rank?
* @return True if salary depends on rank, false otherwise.
*/
bool RuleSoldier::isSalaryDynamic() const
{
	return _costSalarySquaddie || _costSalarySergeant || _costSalaryCaptain || _costSalaryColonel || _costSalaryCommander;
}

/**
 * Gets the cost of salary for a month (for a given rank).
 * @param rank Soldier rank.
 * @return The cost.
 */
int RuleSoldier::getSalaryCost(int rank) const
{
	int total = _costSalary;
	switch (rank)
	{
		case 1: total += _costSalarySquaddie; break;
		case 2: total += _costSalarySergeant; break;
		case 3: total += _costSalaryCaptain; break;
		case 4: total += _costSalaryColonel; break;
		case 5: total += _costSalaryCommander; break;
		default: break;
	}
	return total;
}

/**
 * Gets the height of the soldier when it's standing.
 * @return The standing height.
 */
int RuleSoldier::getStandHeight() const
{
	return _standHeight;
}

/**
 * Gets the height of the soldier when it's kneeling.
 * @return The kneeling height.
 */
int RuleSoldier::getKneelHeight() const
{
	return _kneelHeight;
}

/**
 * Gets the elevation of the soldier when it's flying.
 * @return The floating height.
 */
int RuleSoldier::getFloatHeight() const
{
	return _floatHeight;
}

/**
 * Gets the default armor name.
 * @return The armor name.
 */
std::string RuleSoldier::getArmor() const
{
	return _armor;
}

/**
* Gets the armor for avatar.
* @return The armor name.
*/
std::string RuleSoldier::getArmorForAvatar() const
{
	return _armorForAvatar;
}

/**
* Gets the avatar's X offset.
* @return The X offset.
*/
int RuleSoldier::getAvatarOffsetX() const
{
	return _avatarOffsetX;
}

/**
* Gets the avatar's Y offset.
* @return The Y offset.
*/
int RuleSoldier::getAvatarOffsetY() const
{
	return _avatarOffsetY;
}

/**
* Gets the flag offset.
* @return The flag offset.
*/
int RuleSoldier::getFlagOffset() const
{
	return _flagOffset;
}

/**
* Gets the allow promotion flag.
* @return True if promotion is allowed.
*/
bool RuleSoldier::getAllowPromotion() const
{
	return _allowPromotion;
}

/**
* Gets the allow piloting flag.
* @return True if piloting is allowed.
*/
bool RuleSoldier::getAllowPiloting() const
{
	return _allowPiloting;
}

/**
 * Gets the female appearance ratio.
 * @return The percentage ratio.
 */
int RuleSoldier::getFemaleFrequency() const
{
	return _femaleFrequency;
}

/**
 * Gets the death sounds for male soldiers.
 * @return List of sound IDs.
 */
const std::vector<int> &RuleSoldier::getMaleDeathSounds() const
{
	return _deathSoundMale;
}

/**
 * Gets the death sounds for female soldiers.
 * @return List of sound IDs.
 */
const std::vector<int> &RuleSoldier::getFemaleDeathSounds() const
{
	return _deathSoundFemale;
}

/**
* Returns the list of soldier name pools.
* @return Pointer to soldier name pool list.
*/
const std::vector<SoldierNamePool*> &RuleSoldier::getNames() const
{
	return _names;
}

/**
* Gets the list of StatStrings.
* @return The list of StatStrings.
*/
const std::vector<StatString *> &RuleSoldier::getStatStrings() const
{
	return _statStrings;
}

bool RuleSoldier::isVehicle() const
{
	return _isVehicle;
}

const std::string &RuleSoldier::getInventoryLayout() const
{
	return _inventoryLayout;
}

const std::vector<int> &RuleSoldier::getLevelExperience() const
{
	return _levelExperience;
}

int RuleSoldier::getMaxLevel() const
{
	return _levelExperience.size();
}

int RuleSoldier::getMaxLevelExperience() const
{
	return _levelExperience.size() ? _levelExperience[_levelExperience.size() - 1] : 0;
}

}
