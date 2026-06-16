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
#include "ActionMenuItem.h"
#include "../Interface/Text.h"
#include "../Interface/Frame.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleInterface.h"

namespace OpenXcom
{

/**
 * Sets up an Action menu item.
 * @param id The unique identifier of the menu item.
 * @param game Pointer to the game.
 * @param x Position on the x-axis.
 * @param y Position on the y-axis.
 */
ActionMenuItem::ActionMenuItem(int id, Game *game, int x, int y) : InteractiveSurface(272, 25, x + 24, y - (id*25)), _highlighted(false), _action(BA_NONE), _skill(nullptr), _tu(0), _affordable(true)
{
	Font *big = game->getMod()->getFont("FONT_BIG"), *small = game->getMod()->getFont("FONT_SMALL");
	Language *lang = game->getLanguage();

	const Element *actionMenu = game->getMod()->getInterface("battlescape")->getElement("actionMenu");
	const Element *disabled = game->getMod()->getInterface("battlescape")->getElementOptional("actionMenuDisabled");
	const Element *warning = game->getMod()->getInterface("battlescape")->getElementOptional("actionMenuWarning");

	_highlightModifier = actionMenu->TFTDMode ? 12 : 3;
	_normalColor = actionMenu->color;
	_disabledColor = disabled ? disabled->color : actionMenu->color2;
	_warningColor = warning ? warning->color : _disabledColor;
	_borderColor = actionMenu->border;

	_frame = new Frame(getWidth(), getHeight(), 0, 0);
	_frame->setHighContrast(true);
	_frame->setColor(_borderColor);
	_frame->setSecondaryColor(actionMenu->color2);
	_frame->setThickness(3);

	// compact small-font row layout
	_txtKey = new Text(12, 9, 5, 8);
	_txtKey->initText(big, small, lang);
	_txtKey->setSmall();
	_txtKey->setHighContrast(true);
	_txtKey->setColor(_normalColor);

	_txtDescription = new Text(104, 9, 18, 8);
	_txtDescription->initText(big, small, lang);
	_txtDescription->setSmall();
	_txtDescription->setHighContrast(true);
	_txtDescription->setColor(_normalColor);
	_txtDescription->setVisible(true);

	_txtShots = new Text(66, 9, 122, 8);
	_txtShots->initText(big, small, lang);
	_txtShots->setSmall();
	_txtShots->setHighContrast(true);
	_txtShots->setColor(_normalColor);

	_txtAcc = new Text(46, 9, 188, 8);
	_txtAcc->initText(big, small, lang);
	_txtAcc->setSmall();
	_txtAcc->setHighContrast(true);
	_txtAcc->setColor(_normalColor);

	_txtTU = new Text(36, 9, 234, 8);
	_txtTU->initText(big, small, lang);
	_txtTU->setSmall();
	_txtTU->setHighContrast(true);
	_txtTU->setColor(_normalColor);
}

/**
 * Deletes the ActionMenuItem.
 */
ActionMenuItem::~ActionMenuItem()
{
	delete _frame;
	delete _txtKey;
	delete _txtDescription;
	delete _txtShots;
	delete _txtAcc;
	delete _txtTU;
}

/**
 * Links with an action and fills in the text fields.
 * @param action The battlescape action.
 * @param description The actions description.
 * @param accuracy The actions accuracy, including the Acc> prefix.
 * @param timeunits The timeunits string, including the TUs> prefix.
 * @param tu The timeunits value.
 */
void ActionMenuItem::setAction(BattleActionType action, const std::string &description, const std::string &accuracy, const std::string &timeunits, int tu)
{
	_action = action;
	_txtDescription->setText(description);
	_txtAcc->setText(accuracy);
	_txtTU->setText(timeunits);
	_tu = tu;
	// reset the optional columns/affordability so reused rows don't keep stale data
	_affordable = true;
	_txtKey->setText("");
	_txtShots->setText("");
	_redraw = true;
}

/**
 * Sets the on-row hotkey label.
 * @param key The key glyph (e.g. "3"), or empty for none.
 */
void ActionMenuItem::setHotkey(const std::string &key)
{
	_txtKey->setText(key);
	_redraw = true;
}

/**
 * Sets the shot-count column text.
 * @param shots e.g. "x3 (9 pellets)", or empty for none.
 */
void ActionMenuItem::setShots(const std::string &shots)
{
	_txtShots->setText(shots);
	_redraw = true;
}

/**
 * Flags the action as unaffordable: dims every column to the disabled color and shows the reason
 * tag in the shot-count column (an unaffordable action's shot count is moot). Empty = affordable.
 * @param reason Localized reason tag (e.g. "No TU" / "No Ammo"), or empty to leave affordable.
 */
void ActionMenuItem::setUnaffordable(const std::string &reason)
{
	_affordable = reason.empty();
	int color = _affordable ? _normalColor : _disabledColor;
	_txtKey->setColor(color);
	_txtDescription->setColor(color);
	_txtShots->setColor(color);
	_txtAcc->setColor(color);
	_txtTU->setColor(color);
	_frame->setColor(_affordable ? _borderColor : _disabledColor);
	if (!_affordable)
	{
		_txtShots->setText(reason);
	}
	_redraw = true;
}

/**
 * Flags partial ammo: the action can fire, but there aren't enough rounds for the full shot count.
 * Recolors every column and the frame to the warning color, keeping the shot-count text visible.
 */
void ActionMenuItem::setPartialAmmo()
{
	_txtKey->setColor(_warningColor);
	_txtDescription->setColor(_warningColor);
	_txtShots->setColor(_warningColor);
	_txtAcc->setColor(_warningColor);
	_txtTU->setColor(_warningColor);
	_frame->setColor(_warningColor);
	_redraw = true;
}

/**
 * Links with a skill.
 * @param skill The linked skill.
 */
void ActionMenuItem::setSkill(const RuleSkill *skill)
{
	_skill = skill;
}

/**
 * Gets the action that was linked to this menu item.
 * @return Action that was linked to this menu item.
 */
BattleActionType ActionMenuItem::getAction() const
{
	return _action;
}

/**
 * Gets the skill that was linked to this menu item.
 * @return Skill that was linked to this menu item.
 */
const RuleSkill* ActionMenuItem::getSkill() const
{
	return _skill;
}

/**
 * Gets the action tus that were linked to this menu item.
 * @return The timeunits that were linked to this menu item.
 */
int ActionMenuItem::getTUs() const
{
	return _tu;
}

/**
 * Replaces a certain amount of colors in the surface's palette.
 * @param colors Pointer to the set of colors.
 * @param firstcolor Offset of the first color to replace.
 * @param ncolors Amount of colors to replace.
 */
void ActionMenuItem::setPalette(const SDL_Color *colors, int firstcolor, int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	_frame->setPalette(colors, firstcolor, ncolors);
	_txtKey->setPalette(colors, firstcolor, ncolors);
	_txtDescription->setPalette(colors, firstcolor, ncolors);
	_txtShots->setPalette(colors, firstcolor, ncolors);
	_txtAcc->setPalette(colors, firstcolor, ncolors);
	_txtTU->setPalette(colors, firstcolor, ncolors);
}

/**
 * Draws the bordered box.
 */
void ActionMenuItem::draw()
{
	_frame->blit(this->getSurface());
	_txtKey->blit(this->getSurface());
	_txtDescription->blit(this->getSurface());
	_txtShots->blit(this->getSurface());
	_txtAcc->blit(this->getSurface());
	_txtTU->blit(this->getSurface());
}

/**
 * Processes a mouse hover in event.
 * @param action Pointer to an action.
 * @param state Pointer to a state.
 */
void ActionMenuItem::mouseIn(Action *action, State *state)
{
	_highlighted = true;
	_frame->setSecondaryColor(_frame->getSecondaryColor() - _highlightModifier);
	draw();
	InteractiveSurface::mouseIn(action, state);
}


/**
 * Processes a mouse hover out event.
 * @param action Pointer to an action.
 * @param state Pointer to a state.
 */
void ActionMenuItem::mouseOut(Action *action, State *state)
{
	_highlighted = false;
	_frame->setSecondaryColor(_frame->getSecondaryColor() + _highlightModifier);
	draw();
	InteractiveSurface::mouseOut(action, state);
}


}
