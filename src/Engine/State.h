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
#include <vector>
#include <string>
#include <SDL.h>
#include "LocalizedText.h"

namespace OpenXcom
{

class Game;
class Surface;
class InteractiveSurface;
class Window;
class Action;
class SavedBattleGame;
class RuleInterface;
class Sound;

enum SoldierGender : char;

/**
 * A game state that receives user input and reacts accordingly.
 * Game states typically represent a whole window or screen that
 * the user interacts with, making the game... well, interactive.
 * They automatically handle child elements used to transmit
 * information from/to the user, and are linked to the core game
 * engine which manages them.
 */
class State
{
	friend class Timer;

protected:
	static Game *_game;
	std::vector<Surface*> _surfaces;
	std::vector<Surface*> _surfacesOwned;
	bool _screen;
	bool _soundPlayed;
	InteractiveSurface *_modal;
	RuleInterface *_ruleInterface;
	RuleInterface *_ruleInterfaceParent;
	const Sound* _customSound;

	SDL_Color _palette[256];
	Uint8 _cursorColor;
	// Base resolution that this state's surfaces are currently centered for.
	// Used by the central maximizeInfoScreens handling so a state that is
	// resumed (popped back to) is recentered relative to its own layout, not
	// whatever resolution another state left in Options::baseXResolution.
	int _layoutBaseX, _layoutBaseY;
public:
	/**
	 * How a state relates to the global display scale, used by the central
	 * maximizeInfoScreens handling in Game::run.
	 * - UI: a menu/detail/dialog screen. Maximizes to 320x200 when
	 *   maximizeInfoScreens is on, otherwise renders at the geoscape scale.
	 * - Geoscape: a primary geoscape view (or an overlay that must share its
	 *   resolution). Always uses the geoscape scale.
	 * - Battlescape: the primary battlescape view. Always uses the battlescape scale.
	 * - SelfManaged: the state sets its own resolution (loading/cutscene/intro);
	 *   the central handling leaves it alone.
	 */
	enum class ScaleContext { UI, Geoscape, Battlescape, SelfManaged };
	/// Creates a new state linked to a game.
	State();
	/// Cleans up the state.
	virtual ~State();
	/// Set interface rules.
	void setInterface(const std::string &s, bool alterPal = false, SavedBattleGame *battleGame = 0);
	/// Set window background.
	void setWindowBackground(Window *window, const std::string &s);
	/// Set window background by image name (instead of by interface name).
	void setWindowBackgroundImage(Window* window, const std::string& bgImageName);
	/// Add a optional child element but it will not be displayed.
	template<typename T>
	T* preAdd(T *surface)
	{
		static_assert(std::is_base_of_v<Surface, T>, "Type need to be surface");
		preAdd(static_cast<Surface*>(surface));
		return surface;
	}
	/// Add a optional child element but it will not be displayed.
	void preAdd(Surface *surface);
	/// Adds a child element to the state.
	void add(Surface *surface);
	/// Adds a child element to the state.
	void add(Surface *surface, const std::string &id, const std::string &category, Surface *parent = 0);
	/// Gets whether the state is a full-screen.
	bool isScreen() const;
	/// Toggles whether the state is a full-screen.
	void toggleScreen();
	/// Gets the state's display-scale context (for maximizeInfoScreens handling).
	virtual ScaleContext getScaleContext() const { return ScaleContext::UI; }
	/// Initializes the state.
	virtual void init();
	/// Handles any events.
	virtual void handle(Action *action);
	/// Runs state functionality every cycle.
	virtual void think();
	/// Blits the state to the screen.
	virtual void blit();
	/// Hides all the state surfaces.
	void hideAll();
	/// Shows all the state surfaces.
	void showAll();
	/// Resets all the state surfaces.
	void resetAll();
	/// Get the localized text.
	LocalizedText tr(const std::string &id) const;
	/// Get the localized text.
	LocalizedText trAlt(const std::string &id, int alt) const;
	/// Get the localized text.
	LocalizedText tr(const std::string &id, unsigned n) const;
	/// Get the localized text.
	LocalizedText tr(const std::string &id, SoldierGender gender) const;
	/// redraw all the text-type surfaces.
	void redrawText();
	/// does the state only have one text list (to scroll)?
	bool hasOnlyOneScrollableTextList() const;
	/// center all surfaces relative to the screen.
	void centerAllSurfaces();
	/// lower all surfaces by half the screen height.
	void lowerAllSurfaces();
	/// switch the colours to use the battlescape palette.
	void applyBattlescapeTheme(const std::string& category);
	/// Sets game object pointer
	static void setGamePtr(Game* game);
	/// Sets a modal surface.
	void setModal(InteractiveSurface *surface);

	/// Changes a set of colors on the state's 8bpp palette.
	void setStatePalette(const SDL_Color *colors, int firstcolor = 0, int ncolors = 256);
	/// Changes a set of colors on the state's 8bpp palette of helper surfaces.
	void setModPalette();

	/// Changes the state's 8bpp palette with certain resources.
	void setStandardPalette(const std::string &palette, int backpals = -1);
	/// Changes the state's 8bpp palette with certain resources.
	void setCustomPalette(SDL_Color *colors, int cursorColor);

	/// Gets the state's 8bpp palette.
	SDL_Color *getPalette();

	/// Let the state know the window has been resized.
	virtual void resize(int &dX, int &dY);
	/// Re-orients all the surfaces in the state.
	virtual void recenter(int dX, int dY);
	/// Gets the base resolution this state's surfaces are currently centered for.
	int getLayoutBaseX() const { return _layoutBaseX; }
	/// Gets the base resolution this state's surfaces are currently centered for.
	int getLayoutBaseY() const { return _layoutBaseY; }
	/// Records the base resolution this state's surfaces are now centered for.
	void setLayoutBase(int x, int y) { _layoutBaseX = x; _layoutBaseY = y; }

	/// Gets cursor X coordinate.
	int getCursorX() const;
	/// Gets cursor Y coordinate.
	int getCursorY() const;
};

}
