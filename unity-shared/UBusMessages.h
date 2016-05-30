/*
 * Copyright (C) 2010 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 */
#ifndef UBUS_MESSAGES_H
#define UBUS_MESSAGES_H


// Add ubus messages here so we can easily export them around the place
// keep ordered

#define UBUS_DASH_EXTERNAL_ACTIVATION "DASH_EXTERNAL_ACTIVATION"
#define UBUS_OVERLAY_CLOSE_REQUEST "OVERLAY_CLOSE_REQUEST"

// Request a PlaceEntry to be shown.
// Payload should be: (sus) = (id, section, search_string).
// id = entry->GetId(), search_string can be ""
#define UBUS_PLACE_ENTRY_ACTIVATE_REQUEST "PLACE_ENTRY_ACTIVATE_REQUEST"
#define UBUS_DASH_ABOUT_TO_SHOW "DASH_ABOUT_TO_SHOW"

// Signal sent when an overlay interface is shown, includes a gvariant
// gvariant format is (sb), (interface-name, can_maximize?, width, height)
#define UBUS_OVERLAY_FORMAT_STRING "(sbiii)"
#define UBUS_OVERLAY_HIDDEN "OVERLAY_HIDDEN"
#define UBUS_OVERLAY_SHOWN "OVERLAY_SHOWN"

// Signal send by Launcher/Quicklist when it wants to exit key-nav and wants to
// get rid of keyboard-input-focus
#define UBUS_LAUNCHER_START_KEY_NAV  "LAUNCHER_START_KEY_NAV"
#define UBUS_LAUNCHER_END_KEY_NAV    "LAUNCHER_END_KEY_NAV"
#define UBUS_LAUNCHER_PREV_KEY_NAV   "LAUNCHER_PREV_KEY_NAV"
#define UBUS_LAUNCHER_NEXT_KEY_NAV   "LAUNCHER_NEXT_KEY_NAV"
#define UBUS_LAUNCHER_OPEN_QUICKLIST "LAUNCHER_OPEN_QUICKLIST"
#define UBUS_LAUNCHER_START_KEY_SWITCHER "LAUNCHER_START_KEY_SWITCHER"
#define UBUS_LAUNCHER_END_KEY_SWITCHER   "LAUNCHER_END_KEY_SWITCHER"
#define UBUS_LAUNCHER_SELECTION_CHANGED  "LAUNCHER_ICON_SELECTION_CHANGED"
#define UBUS_QUICKLIST_START_KEY_NAV "QUICKLIST_START_KEY_NAV"
#define UBUS_QUICKLIST_END_KEY_NAV   "QUICKLIST_END_KEY_NAV"

// Signal to force the launcher into locked mode, (b)
#define UBUS_LAUNCHER_LOCK_HIDE "LAUNCHER_LOCK_HIDE"

#define UBUS_DASH_SIZE_CHANGED "DASH_SIZE_CHANGED"
// FIXME - fix the nux focus api so we don't need this
#define UBUS_RESULT_VIEW_KEYNAV_CHANGED "RESULT_VIEW_KEYNAV_CHANGED"

// called when previews wish to navigate left/right or close (is)
// -1 = left, 0 = close, 1 = right,
// string is the uri string that last result activated was
#define UBUS_DASH_PREVIEW_NAVIGATION_REQUEST "DASH_PREVIEW_NAVIGATION_REQUEST"

// Sends a string datatype containing the new icon name
#define UBUS_HUD_ICON_CHANGED "HUD_ICON_CHANGED"
#define UBUS_HUD_CLOSE_REQUEST "HUD_CLOSE_REQUEST"

// Signals sent when the switcher is shown, hidden or changes selection
#define UBUS_SWITCHER_SHOWN             "SWITCHER_SHOWN"
#define UBUS_SWITCHER_START             "SWITCHER_SHOWN_START"
#define UBUS_SWITCHER_END               "SWITCHER_SHOWN_END"
#define UBUS_SWITCHER_SELECTION_CHANGED "SWITCHER_SELECTION_CHANGED"

#endif // UBUS_MESSAGES_H
