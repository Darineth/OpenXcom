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
#include "MedikitState.h"
#include "MedikitView.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Action.h"
#include "../Engine/Palette.h"
#include "../Interface/Text.h"
#include "../Engine/Screen.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Mod/RuleItem.h"
#include "../Mod/Mod.h"
#include <sstream>
#include "../Engine/Options.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/BattleUnitStatistics.h"
#include "TileEngine.h"

namespace OpenXcom
{

/**
 * Helper function that returns a string representation of a type (mainly used for numbers).
 * @param t The value to stringify.
 * @return A string representation of the value.
 */
template<typename type>
std::wstring toString (type t)
{
	std::wostringstream ss;
	ss << t;
	return ss.str();
}

/**
 * Helper class for the medikit title.
 */
class MedikitTitle : public Text
{
public:
	/// Creates a medikit title.
	MedikitTitle(int y, const std::wstring & title);
};

/**
 * Initializes a Medikit title.
 * @param y The title's y origin.
 * @param title The title.
 */
MedikitTitle::MedikitTitle (int y, const std::wstring & title) : Text (73, 9, 186, y)
{
	this->setText(title);
	this->setHighContrast(true);
	this->setAlign(ALIGN_CENTER);
}

/**
 * Helper class for the medikit value.
 */
class MedikitTxt : public Text
{
public:
	/// Creates a medikit text.
	MedikitTxt(int y);
};

/**
 * Initializes a Medikit text.
 * @param y The text's y origin.
 */
MedikitTxt::MedikitTxt(int y) : Text(33, 17, 220, y)
{
	// Note: we can't set setBig here. The needed font is only set when added to State
	this->setColor(Palette::blockOffset(1));
	this->setHighContrast(true);
	this->setAlign(ALIGN_CENTER);
}

/**
 * Helper class for the medikit button.
 */
class MedikitButton : public InteractiveSurface
{
public:
	/// Creates a medikit button.
	MedikitButton(int y);
};

/**
 * Initializes a Medikit button.
 * @param y The button's y origin.
 */
MedikitButton::MedikitButton(int y) : InteractiveSurface(30, 20, 190, y)
{
}

/**
 * Initializes the Medikit State.
 * @param game Pointer to the core game.
 * @param targetUnit The wounded unit.
 * @param action The healing action.
 */
MedikitState::MedikitState (BattleUnit *targetUnit, BattleAction *action, TileEngine *tile) : _targetUnit(targetUnit), _action(action), _tileEngine(tile)
{
	_game->getScreen()->pushMaximizeInfoScreen(true);

	_unit = action->actor;
	_item = action->weapon;
	_bg = new Surface(320, 200);

	// Set palette
	_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);

	if (_game->getScreen()->getDY() > 50)
	{
		_screen = false;
		_bg->drawRect(67, 44, 190, 100, Palette::blockOffset(15)+15);
	}
	_partTxt = new Text(62, 9, 82, 120);
	_woundTxt = new Text(14, 9, 145, 120);
	_medikitView = new MedikitView(52, 58, 95, 60, _game, _targetUnit, _partTxt, _woundTxt);
	_endButton = new InteractiveSurface(20, 20, 220, 140);
	_stimulantButton = new MedikitButton(84);
	_pkButton = new MedikitButton(48);
	_healButton = new MedikitButton(120);
	_pkText = new MedikitTxt (52);
	_stimulantTxt = new MedikitTxt (88);
	_healTxt = new MedikitTxt (124);
	_txtStatus = new Text(78, 9, 81, 50);
	_txtHealth = new Text(78, 9, 81, 58);
	_txtStun = new Text(78, 9, 81, 66);
	_txtTU = new Text(78, 9, 81, 112);
	add(_bg);
	add(_medikitView, "body", "medikit", _bg);
	add(_endButton, "buttonEnd", "medikit", _bg);
	add(new MedikitTitle (37, tr("STR_PAIN_KILLER")), "textPK", "medikit", _bg);
	add(new MedikitTitle (73, tr("STR_STIMULANT")), "textStim", "medikit", _bg);
	add(new MedikitTitle (109, tr("STR_HEAL")), "textHeal", "medikit", _bg);
	add(_healButton, "buttonHeal", "medikit", _bg);
	add(_stimulantButton, "buttonStim", "medikit", _bg);
	add(_pkButton, "buttonPK", "medikit", _bg);
	add(_pkText, "numPK", "medikit", _bg);
	add(_stimulantTxt, "numStim", "medikit", _bg);
	add(_healTxt, "numHeal", "medikit", _bg);
	add(_partTxt, "textPart", "medikit", _bg);
	add(_woundTxt, "numWounds", "medikit", _bg);
	add(_txtStatus);
	add(_txtHealth);
	add(_txtStun);
	add(_txtTU);

	centerAllSurfaces();

	_game->getMod()->getSurface("MEDIBORD.PCK")->blit(_bg);
	_pkText->setBig();
	_stimulantTxt->setBig();
	_healTxt->setBig();
	_partTxt->setHighContrast(true);
	_woundTxt->setHighContrast(true);
	_txtStatus->setColor(Palette::blockOffset(4));
	_txtStatus->setSecondaryColor(Palette::blockOffset(4));
	_txtStatus->setHighContrast(true);
	_txtHealth->setColor(Palette::blockOffset(4));
	_txtHealth->setSecondaryColor(Palette::blockOffset(2));
	_txtHealth->setHighContrast(true);
	_txtStun->setColor(Palette::blockOffset(4));
	_txtStun->setSecondaryColor(Palette::blockOffset(8));
	_txtStun->setHighContrast(true);
	_txtTU->setColor(Palette::blockOffset(4));
	_txtTU->setSecondaryColor(Palette::blockOffset(4));
	_txtTU->setHighContrast(true);
	//_txtStatus->setSecondaryColor(Palette::blockOffset(1));
	//_txtStatus->setAlign(ALIGN_CENTER);
	_endButton->onMouseClick((ActionHandler)&MedikitState::onEndClick);
	_endButton->onKeyboardPress((ActionHandler)&MedikitState::onEndClick, Options::keyCancel);
	_healButton->onMouseClick((ActionHandler)&MedikitState::onHealClick);
	_stimulantButton->onMouseClick((ActionHandler)&MedikitState::onStimulantClick);
	_pkButton->onMouseClick((ActionHandler)&MedikitState::onPainKillerClick);
	update();
}

/**
 * Closes the window on right-click.
 * @param action Pointer to an action.
 */
void MedikitState::handle(Action *action)
{
	State::handle(action);
	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN && action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		onEndClick(0);
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void MedikitState::onEndClick(Action *)
{
	_game->getScreen()->popMaximizeInfoScreen();
	_game->popState();
	_tileEngine->medikitRemoveIfEmpty(_action);
}

/**
 * Handler for clicking on the heal button.
 * @param action Pointer to an action.
 */
void MedikitState::onHealClick(Action *)
{
	if (_item->getHealQuantity() == 0)
	{
		return;
	}

	if (_action->spendTU(&_action->result))
	{
		_unit->cancelEffects(ECT_ACTIVATE);
		_unit->addBattleExperience("STR_HEAL");

		_tileEngine->medikitHeal(_action, _targetUnit, _medikitView->getSelectedPart());
		_medikitView->updateSelectedPart();
		_medikitView->invalidate();
		_action->actor->getStatistics()->woundsHealed++;
		update();

		if (_targetUnit->getStatus() == STATUS_UNCONSCIOUS && _targetUnit->getStunlevel() < _targetUnit->getHealth() && _targetUnit->getHealth() > 0)
		{
			_targetUnit->setTimeUnits(0);
			_action->actor->getStatistics()->revivedSoldier++;
		}
	}
	else
	{
		_action->result = "STR_NOT_ENOUGH_TIME_UNITS";
		onEndClick(0);
	}
}

/**
 * Handler for clicking on the stimulant button.
 * @param action Pointer to an action.
 */
void MedikitState::onStimulantClick(Action *)
{
	if (_item->getStimulantQuantity() == 0)
	{
		return;
	}

	if (_action->spendTU(&_action->result))
	{
		_unit->cancelEffects(ECT_ACTIVATE);
		_unit->addBattleExperience("STR_HEAL");

		_tileEngine->medikitStimulant(_action, _targetUnit);
		_action->actor->getStatistics()->appliedStimulant++;
		update();

		// if the unit has revived we quit this screen automatically
		if (_targetUnit->getStatus() == STATUS_UNCONSCIOUS && _targetUnit->getStunlevel() < _targetUnit->getHealth() && _targetUnit->getHealth() > 0)
		{
			_targetUnit->setTimeUnits(0);
			_action->actor->getStatistics()->revivedSoldier++;
			onEndClick(0);
		}
	}
	else
	{
		_action->result = "STR_NOT_ENOUGH_TIME_UNITS";
		onEndClick(0);
	}
}

/**
 * Handler for clicking on the pain killer button.
 * @param action Pointer to an action.
 */
void MedikitState::onPainKillerClick(Action *)
{
	if (_item->getPainKillerQuantity() == 0)
	{
		return;
	}

	if (_action->spendTU(&_action->result))
	{
		_unit->cancelEffects(ECT_ACTIVATE);
		_unit->addBattleExperience("STR_HEAL");

		_tileEngine->medikitPainKiller(_action, _targetUnit);
		_action->actor->getStatistics()->appliedPainKill++;
		update();
	}
	else
	{
		_action->result = "STR_NOT_ENOUGH_TIME_UNITS";
		onEndClick(0);
	}
}

/**
 * Updates the medikit state.
 */
void MedikitState::update()
{
	_pkText->setText(toString(_item->getPainKillerQuantity()));
	_stimulantTxt->setText(toString(_item->getStimulantQuantity()));
	_healTxt->setText(toString(_item->getHealQuantity()));

	_txtTU->setText(tr("STR_MEDIKIT_TU").arg(_unit->getTimeUnits()).arg(_unit->getBaseStats()->tu));

	_txtHealth->setText(tr("STR_MEDIKIT_HP").arg(_targetUnit->getHealth()).arg(_targetUnit->getBaseStats()->health));

	if(_targetUnit->getBleedingOut())
	{
		_txtStatus->setSecondaryColor(Palette::blockOffset(2));
		_txtStatus->setText(tr("STR_MEDIKIT_STATUS").arg(tr("STR_MEDIKIT_BLEEDING")));
		_txtHealth->setText(tr("STR_MEDIKIT_HP_WOUNDS").arg((-_targetUnit->getDeathHealth()) + _targetUnit->getHealth()).arg(-_targetUnit->getDeathHealth()).arg(-_targetUnit->getFatalWounds()));
		_txtStun->clear();
	}
	else if(_targetUnit->getHealth() <= 0)
	{
		_txtStatus->setSecondaryColor(Palette::blockOffset(2));
		_txtStatus->setText(tr("STR_MEDIKIT_STATUS").arg(tr("STR_MEDIKIT_STABILIZED")));
		_txtHealth->setText(tr("STR_MEDIKIT_HP").arg(_targetUnit->getHealth()).arg(_targetUnit->getBaseStats()->health));
		_txtStun->clear();
	}
	else if(_targetUnit->getStunlevel() >= _targetUnit->getHealth())
	{
		_txtStatus->setSecondaryColor(Palette::blockOffset(8));
		_txtStatus->setText(tr("STR_MEDIKIT_STATUS").arg(tr("STR_MEDIKIT_STUNNED")));
		if (_targetUnit->getFatalWounds())
		{
			_txtHealth->setText(tr("STR_MEDIKIT_HP_WOUNDS").arg(_targetUnit->getHealth()).arg(_targetUnit->getBaseStats()->health).arg(_targetUnit->getFatalWounds()));
		}
		else
		{
			_txtHealth->setText(tr("STR_MEDIKIT_HP").arg(_targetUnit->getHealth()).arg(_targetUnit->getBaseStats()->health));
		}
		_txtStun->setText(tr("STR_MEDIKIT_STUN").arg(_targetUnit->getStunlevel()).arg(_targetUnit->getBaseStats()->health));
	}
	else
	{
		if (_targetUnit->getFatalWounds())
		{
			_txtStatus->setSecondaryColor(Palette::blockOffset(2));
			_txtStatus->setText(tr("STR_MEDIKIT_STATUS").arg(tr("STR_MEDIKIT_WOUNDED")));
		}
		else if (_targetUnit->getHealth() < _targetUnit->getBaseStats()->health)
		{
			_txtStatus->setSecondaryColor(Palette::blockOffset(1));
			_txtStatus->setText(tr("STR_MEDIKIT_STATUS").arg(tr("STR_MEDIKIT_INJURED")));
		}
		else
		{
			_txtStatus->setSecondaryColor(Palette::blockOffset(3));
			_txtStatus->setText(tr("STR_MEDIKIT_STATUS").arg(tr("STR_MEDIKIT_OK")));
		}

		if (_targetUnit->getFatalWounds())
		{
			_txtHealth->setText(tr("STR_MEDIKIT_HP_WOUNDS").arg(_targetUnit->getHealth()).arg(_targetUnit->getBaseStats()->health).arg(_targetUnit->getFatalWounds()));
		}
		else
		{
			_txtHealth->setText(tr("STR_MEDIKIT_HP").arg(_targetUnit->getHealth()).arg(_targetUnit->getBaseStats()->health));
		}
		_txtStun->setText(tr("STR_MEDIKIT_STUN").arg(_targetUnit->getStunlevel()).arg(_targetUnit->getBaseStats()->health));
	}


	_medikitView->invalidate();
}

}
