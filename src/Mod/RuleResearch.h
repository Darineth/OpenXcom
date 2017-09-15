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

namespace OpenXcom
{
/**
 * Represents one research project.
 * Dependency is the list of RuleResearchs which must be discovered before a RuleResearch became available.
 * Unlocks are used to immediately unlock a RuleResearch (even if not all the dependencies have been researched).
 *
 * Fake ResearchProjects: A RuleResearch is fake one, if its cost is 0. They are used to to create check points in the dependency tree.
 *
 * For example, if we have a Research E which needs either A & B or C & D, we create two fake research projects:
 *  - F which need A & B
 *  - G which need C & D
 * both F and G can unlock E.
 */
class RuleResearch
{
 private:
	std::string _name, _lookup, _cutscene, _spawnedItem;
	int _cost, _points;
	std::vector<std::string> _dependencies, _unlocks, _disables, _getOneFree, _requires, _requiresBaseFunc, _needsAllItems, _needsAnyItems;
	bool _sequentialGetOneFree;
	std::map<std::string, std::vector<std::string> > _getOneFreeProtected;
	bool _needItem, _destroyItem;
	int _listOrder;
public:
	static const int RESEARCH_STATUS_NEW = 0;
	static const int RESEARCH_STATUS_NORMAL = 1;
	static const int RESEARCH_STATUS_DISABLED = 2;
	RuleResearch(const std::string &name);
	/// Loads the research from YAML.
	void load(const YAML::Node& node, int listOrder);
	/// Gets time needed to discover this ResearchProject.
	int getCost() const;
	/// Gets the research name.
	const std::string &getName() const;
	/// Gets the research dependencies.
	const std::vector<std::string> &getDependencies() const;
	/// Checks if this ResearchProject gives free topics in sequential order (or random order).
	bool sequentialGetOneFree() const;
	/// Gets the needed items to research this (only one required).
	const std::vector<std::string> & getNeededItemsAny() const;
	/// Gets the needed items to research this (all required).
	const std::vector<std::string> & getNeededItemsAll() const;
	/// Checks if this ResearchProject needs a corresponding Item to be researched.
	bool needItem() const;
	/// Checks if this ResearchProject consumes the corresponding Item when research completes.
	bool destroyItem() const;
	/// Gets the list of ResearchProjects unlocked by this research.
	const std::vector<std::string> &getUnlocked() const;
	/// Gets the list of ResearchProjects disabled by this research.
	const std::vector<std::string> &getDisabled() const;
	/// Gets the points earned for discovering this ResearchProject.
	int getPoints() const;
	/// Gets the list of ResearchProjects granted at random for free by this research.
	const std::vector<std::string> &getGetOneFree() const;
	/// Gets the list(s) of ResearchProjects granted at random for free by this research (if a defined prerequisite is met).
	const std::map<std::string, std::vector<std::string> > &getGetOneFreeProtected() const;
	/// Gets what to look up in the ufopedia.
	std::string getLookup() const;
	/// Gets the requirements for this ResearchProject.
	const std::vector<std::string> &getRequirements() const;
	/// Gets the base requirements for this ResearchProject.
	const std::vector<std::string> &getRequireBaseFunc() const;
	/// Gets the list weight for this research item.
	int getListOrder() const;
	/// Gets the cutscene to play when this item is researched
	const std::string & getCutscene() const;
	/// Gets the item to spawn in the base stores when this topic is researched.
	const std::string & getSpawnedItem() const;
};

/**
 * Compares RuleResearch pointers based on the research name.
 */
class CompareRuleResearch
{
public:
	bool operator()(const RuleResearch *a, const RuleResearch *b) const
	{
		return a->getName() < b->getName();
	}
};

}
