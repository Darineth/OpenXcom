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

#define MIN_REQUIRED_RULESET_VERSION_NUMBER 8,6,0,0

// DX tracks TWO independent version axes (see the dual-track discussion / plans):
//  1. The OXCE BASE version (OPENXCOM_VERSION_NUMBER) - the upstream OXCE this DX last synced to. This
//     drives the "Extended" engine-compat entry (so OXCE mods requiring "Extended <=X" load on DX), the
//     save-header version, and the update check. Bump it ONLY when merging a new upstream OXCE.
//  2. DX's OWN version (OPENXCOM_VERSION_NUMBER_DX) - DX's feature-set identity, independent of OXCE.
//     Drives the "Extended DX" engine-compat entry, so a DX-specific mod can require a minimum DX
//     version. Bump it when DX ships ruleset-visible features/changes a mod might gate on.
#define OPENXCOM_VERSION_ENGINE "Extended DX"     // DX engine name (mods gate on this for DX-only features)
#define OPENXCOM_VERSION_ENGINE_OXCE "Extended"   // OXCE engine name DX stays backward-compatible with

// OXCE base (axis 1)
#define OPENXCOM_VERSION_OXCE "8.6.2"
#define OPENXCOM_VERSION_LONG "8.6.2.0"
#define OPENXCOM_VERSION_NUMBER 8,6,2,0

// DX's own version (axis 2)
#define OPENXCOM_VERSION_DX "0.9.0"
#define OPENXCOM_VERSION_NUMBER_DX 0,9,0,0

// Compact display (UI - main menu, titles, loading): "Extended DX 0.9.0". Kept short on purpose;
// the main menu has almost no room for the OXCE base.
#define OPENXCOM_VERSION_SHORT OPENXCOM_VERSION_ENGINE " " OPENXCOM_VERSION_DX
// Full display with the OXCE base, for places with room where lineage helps (logs, --version, save
// headers): "Extended DX 0.9.0 (OXCE 8.6.2)".
#define OPENXCOM_VERSION_SHORT_OXCE OPENXCOM_VERSION_SHORT " (OXCE " OPENXCOM_VERSION_OXCE ")"

#ifndef OPENXCOM_VERSION_GIT
#define OPENXCOM_VERSION_GIT " (v2026-07-25)"
#endif
