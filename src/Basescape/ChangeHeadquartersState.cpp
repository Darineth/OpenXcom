#include "ChangeHeadquartersState.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in a Change HQ window.
 * @param base Pointer to the base to get info from.
 */
ChangeHeadquartersState::ChangeHeadquartersState(Base *base) : _base(base)
{
	_screen = false;

	// Create objects
	_window = new Window(this, 152, 80, 20, 60);
	_btnOk = new TextButton(44, 16, 36, 115);
	_btnCancel = new TextButton(44, 16, 112, 115);
	_txtTitle = new Text(142, 9, 25, 75);
	_txtBase = new Text(142, 9, 25, 85);

	// Set palette
	setInterface("changeHeadquarters");

	add(_window, "window", "changeHeadquarters");
	add(_btnOk, "button", "changeHeadquarters");
	add(_btnCancel, "button", "changeHeadquarters");
	add(_txtTitle, "text", "changeHeadquarters");
	add(_txtBase, "text", "changeHeadquarters");

	centerAllSurfaces();

	// Set up objects
	_window->setBackground(_game->getMod()->getSurface("BACK13.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&ChangeHeadquartersState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&ChangeHeadquartersState::btnOkClick, Options::keyOk);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)&ChangeHeadquartersState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)&ChangeHeadquartersState::btnCancelClick, Options::keyCancel);

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_MOVE_HQ_TO"));

	_txtBase->setAlign(ALIGN_CENTER);
	_txtBase->setText(_base->getName());
}

/**
 *
 */
ChangeHeadquartersState::~ChangeHeadquartersState()
{

}

/**
 * Changes the HQ and returns
 * to the previous screen.
 * @param action Pointer to an action.
 */
void ChangeHeadquartersState::btnOkClick(Action *)
{
	for (std::vector<Base*>::iterator i = _game->getSavedGame()->getBases()->begin(); i != _game->getSavedGame()->getBases()->end(); ++i)
	{
		if (*i == _base)
		{
			_game->getSavedGame()->getBases()->erase(i);
			_game->getSavedGame()->getBases()->insert(_game->getSavedGame()->getBases()->begin(), _base);
			break;
		}
	}

	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void ChangeHeadquartersState::btnCancelClick(Action *)
{
	_game->popState();
}

}
