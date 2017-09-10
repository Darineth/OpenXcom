#ifndef OPENXCOM_SOLDIERSORTUTIL_H
#define OPENXCOM_SOLDIERSORTUTIL_H

#include "../Engine/Game.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SavedGame.h"
#include "../Mod/Mod.h"

namespace OpenXcom
{

typedef int (*getStatFn_t)(Game *, Soldier *);

struct SortFunctor : public std::binary_function<Soldier *, Soldier *, bool>
{
	Game *_game;
	getStatFn_t _getStatFn;
	SortFunctor(Game *game, getStatFn_t getStatFn) : _game(game), _getStatFn(getStatFn) { }
	bool operator()(Soldier *a, Soldier *b)
	{
		bool ret = _getStatFn(_game, a) < _getStatFn(_game, b);
		return ret;
	}
	getStatFn_t getGetter()
	{
		return _getStatFn;
	}
};

#define GET_ATTRIB_STAT_FN(attrib) \
	int attrib##Stat(Game *game, Soldier *s);
GET_ATTRIB_STAT_FN(tu)
GET_ATTRIB_STAT_FN(stamina)
GET_ATTRIB_STAT_FN(health)
GET_ATTRIB_STAT_FN(bravery)
GET_ATTRIB_STAT_FN(reactions)
GET_ATTRIB_STAT_FN(firing)
GET_ATTRIB_STAT_FN(throwing)
GET_ATTRIB_STAT_FN(strength)
GET_ATTRIB_STAT_FN(psiStrength)
GET_ATTRIB_STAT_FN(psiSkill)
GET_ATTRIB_STAT_FN(melee)
#undef GET_ATTRIB_STAT_FN

#define GET_SOLDIER_STAT_FN(attrib, camelCaseAttrib) \
	int attrib##Stat(Game *game, Soldier *s);
GET_SOLDIER_STAT_FN(id, Id)
GET_SOLDIER_STAT_FN(name, Name)
GET_SOLDIER_STAT_FN(rank, Rank)
GET_SOLDIER_STAT_FN(missions, Missions)
GET_SOLDIER_STAT_FN(kills, Kills)
GET_SOLDIER_STAT_FN(woundRecovery, WoundRecovery)
#undef GET_SOLDIER_STAT_FN

}

#endif //OPENXCOM_SOLDIERSORTUTIL_H
