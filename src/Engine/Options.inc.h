// Don't worry about Intellisense errors here, as this file is only used in conjunction with Options.h/Options.cpp
// To add a new option, add a new variable entry and a corresponding OptionInfo in Options.cpp

// General options
OPT int displayWidth, displayHeight, maxFrameSkip, baseXResolution, baseYResolution, baseXGeoscape, baseYGeoscape, baseXBattlescape, baseYBattlescape,
	soundVolume, musicVolume, uiVolume, audioSampleRate, audioBitDepth, audioChunkSize, pauseMode, windowedModePositionX, windowedModePositionY, FPS, FPSInactive,
	changeValueByMouseWheel, dragScrollTimeTolerance, dragScrollPixelTolerance, mousewheelSpeed, autosaveFrequency;
OPT bool fullscreen, asyncBlit, playIntro, useScaleFilter, useHQXFilter, useXBRZFilter, useOpenGL, checkOpenGLErrors, vSyncForOpenGL, useOpenGLSmoothing,
	autosave, allowResize, borderless, debug, debugUi, fpsCounter, newSeedOnLoad, keepAspectRatio, nonSquarePixelRatio,
	cursorInBlackBandsInFullscreen, cursorInBlackBandsInWindow, cursorInBlackBandsInBorderlessWindow, maximizeInfoScreens, musicAlwaysLoop, StereoSound, verboseLogging, soldierDiaries, touchEnabled,
	rootWindowedMode;
OPT std::string language, useOpenGLShader;
OPT KeyboardType keyboardMode;
OPT SaveSort saveOrder;
OPT MusicFormat preferredMusic;
OPT SoundFormat preferredSound;
OPT VideoFormat preferredVideo;
OPT SDL_GrabMode captureMouse;
OPT SDLKey keyOk, keyCancel, keyScreenshot, keyFps, keyQuickLoad, keyQuickSave;

// Geoscape options
OPT int geoClockSpeed, dogfightSpeed, geoScrollSpeed, geoDragScrollButton, geoscapeScale;
OPT bool includePrimeStateInSavedLayout, anytimePsiTraining, anytimeMartialTraining, weaponSelfDestruction, retainCorpses, craftLaunchAlways, globeSeasons, globeDetail, globeRadarLines, globeFlightPaths, globeAllRadarsOnBaseBuild,
	storageLimitsEnforced, canSellLiveAliens, canTransferCraftsWhileAirborne, customInitialBase, aggressiveRetaliation, geoDragScrollInvert,
	allowBuildingQueue, showFundsOnGeoscape, psiStrengthEval, allowPsiStrengthImprovement, fieldPromotions, meetingPoint;
OPT SDLKey keyGeoLeft, keyGeoRight, keyGeoUp, keyGeoDown, keyGeoZoomIn, keyGeoZoomOut, keyGeoSpeed1, keyGeoSpeed2, keyGeoSpeed3, keyGeoSpeed4, keyGeoSpeed5, keyGeoSpeed6,
	keyGeoIntercept, keyGeoBases, keyGeoGraphs, keyGeoUfopedia, keyGeoOptions, keyGeoFunding, keyGeoToggleDetail, keyGeoToggleRadar,
	keyBaseSelect1, keyBaseSelect2, keyBaseSelect3, keyBaseSelect4, keyBaseSelect5, keyBaseSelect6, keyBaseSelect7, keyBaseSelect8;

// Battlescape options
OPT ScrollType battleEdgeScroll;
OPT PathPreview battleNewPreviewPath;
OPT int battleScrollSpeed, battleDragScrollButton, battleFireSpeed, battleXcomSpeed, battleAlienSpeed, battleExplosionHeight, battlescapeScale;
OPT bool traceAI, sneakyAI, battleInstantGrenade, battleNotifyDeath, battleTooltips, battleHairBleach, battleAutoEnd,
	strafe, forceFire, showMoreStatsInInventoryView, allowPsionicCapture, skipNextTurnScreen, disableAutoEquip, battleDragScrollInvert,
	battleUFOExtenderAccuracy, battleConfirmFireMode, battleSmoothCamera, noAlienPanicMessages, alienBleeding, battleAdjustReloadCost;
OPT SDLKey keyBattleLeft, keyBattleRight, keyBattleUp, keyBattleDown, keyBattleLevelUp, keyBattleLevelDown, keyBattleCenterUnit, keyBattlePrevUnit, keyBattleNextUnit, keyBattleDeselectUnit,
	keyBattleUseLeftHand, keyBattleUseRightHand, keyBattleUseUtility, keyBattleInventory, keyBattleMap, keyBattleOptions, keyBattleEndTurn, keyBattleAbort, keyBattleStats, keyBattleKneel,
	keyBattleReserveKneel, keyBattleReload, keyBattlePersonalLighting, keyBattleReserveNone, keyBattleReserveSnap, keyBattleReserveAimed, keyBattleReserveAuto,
	keyBattleCenterEnemy1, keyBattleCenterEnemy2, keyBattleCenterEnemy3, keyBattleCenterEnemy4, keyBattleCenterEnemy5, keyBattleCenterEnemy6, keyBattleCenterEnemy7, keyBattleCenterEnemy8,
	keyBattleCenterEnemy9, keyBattleCenterEnemy10, keyBattleVoxelView, keyBattleZeroTUs, keyInvCreateTemplate, keyInvApplyTemplate, keyInvClear, keyInvAutoEquip;

// OpenXCOM+ Action Menu Keys
OPT SDLKey keyActionAuto, keyActionBurst, keyActionAimed, keyActionSnap, keyActionLaunch,
	keyActionThrow, keyActionPrime,
	keyActionOverwatch, keyActionDualFire, keyActionReload,
	keyActionMelee,
	keyActionUse, keyActionPsi1, keyActionPsi2, keyActionPsi3, keyActionPsi4;

// Extra hotkeys (Meridian's build)
OPT SDLKey keyGeoUfoTracker, keyGeoTechTreeViewer,
	keyGraphsZoomIn, keyGraphsZoomOut,
	keyToggleQuickSearch,
	keyCraftLoadoutSave, keyCraftLoadoutLoad,
	keyMarkAllAsSeen, keySelectAll, keyDeselectAll, keyResetAll,
	keyInventoryArmor, keyInventoryAvatar, keyInventorySave, keyInventoryLoad,
	/*keyBattleActionItem1, keyBattleActionItem2, keyBattleActionItem3, keyBattleActionItem4, keyBattleActionItem5,*/
	keyNightVisionToggle, keyNightVisionHold, keySelectMusicTrack;

// Air Combat Keys
OPT SDLKey keyAirMove, keyAirAttack, keyAirSpecial, keyAirWait,
	keyAirAdvance, keyAirBackOff, keyAirPursue,
	keyAirWeapon1, keyAirWeapon2, keyAirWeapon3, keyAirWeapon4,
	keyAirHold, keyAirEvade;

// Extra options (Meridian's build)
OPT bool executeUnconsciousEnemies, twoHandedIndicator, twoHandedIndicatorInventory, minimapBorderIndicator, everyoneFightsNobodyQuits;
OPT bool bleedingIndicator, knockOutIndicator, statisticalBulletConservation;

OPT bool showItemNameAndWeightInInventory;
OPT bool showGunMeleeOnTop;
OPT bool showAllCommendations;
OPT bool removeWoundedFromTraining;
OPT bool fullNightVision;
OPT int nightVisionColor;
OPT bool autoNightVision;
OPT bool autoSell;
OPT bool techTreeViewerSpoilerProtection;
OPT bool playBriefingMusicDuringEquipment;

// Flags and other stuff that don't need OptionInfo's.
OPT bool mute, reload, newOpenGL, newScaleFilter, newHQXFilter, newXBRZFilter, newRootWindowedMode, newFullscreen, newAllowResize, newBorderless;
OPT int newDisplayWidth, newDisplayHeight, newBattlescapeScale, newGeoscapeScale, newWindowedModePositionX, newWindowedModePositionY;
OPT std::string newOpenGLShader;
OPT std::vector< std::pair<std::string, bool> > mods; // ordered list of available mods (lowest priority to highest) and whether they are active
OPT SoundFormat currentSound;
