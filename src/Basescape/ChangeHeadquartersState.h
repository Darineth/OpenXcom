#ifndef OPENXCOM_CHANGEHEADQUARTERSSTATE_H
#define OPENXCOM_CHANGEHEADQUARTERSSTATE_H

#include "../Engine/State.h"

namespace OpenXcom
{

class Base;
class TextButton;
class Window;
class Text;

/**
 * Window shown when the player tries to
 * change the HQ (right-click on a base).
 */
class ChangeHeadquartersState : public State
{
private:
	Base *_base;

	TextButton *_btnOk, *_btnCancel;
	Window *_window;
	Text *_txtTitle, *_txtBase;
public:
	/// Creates the Change HQ state.
	ChangeHeadquartersState(Base *base);
	/// Cleans up the Change HQ state.
	~ChangeHeadquartersState();
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action *action);
};

}

#endif
