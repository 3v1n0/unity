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

// Signal fired when an autopilot test is finished
#define UBUS_AUTOPILOT_TEST_FINISHED "AUTOPILOT_TEST_FINISHED"

#define UBUS_HOME_BUTTON_ACTIVATED "PANEL_HOME_ACTIVATED"

#define UBUS_DASH_EXTERNAL_ACTIVATION   "DASH_EXTERNAL_ACTIVATION"
#define UBUS_HOME_BUTTON_TRIGGER_UPDATE "PANEL_HOME_BUTTON_TRIGGER_UPDATE"

// When other parts of Unity want to close the place view
#define UBUS_PLACE_VIEW_CLOSE_REQUEST "PLACE_VIEW_CLOSE_REQUEST"

// Request a PlaceEntry to be shown.
// Payload should be: (sus) = (id, section, search_string).
// id = entry->GetId(), search_string can be ""
#define UBUS_PLACE_ENTRY_ACTIVATE_REQUEST "PLACE_ENTRY_ACTIVATE_REQUEST"

// When a result wants to activate in a place
// Payload should be: (s) = (uri)
#define UBUS_PLACE_TILE_ACTIVATE_REQUEST "PLACE_TILE_ACTIVATE_REQUEST"

// Signal send when places are shown or hidden
#define UBUS_PLACE_VIEW_HIDDEN "PLACE_VIEW_HIDDEN"
#define UBUS_PLACE_VIEW_SHOWN "PLACE_VIEW_SHOWN"

// Signal send by Launcher/Quicklist when it wants to exit key-nav and wants to
// get rid of keyboard-input-focus
#define UBUS_LAUNCHER_START_KEY_NAV  "LAUNCHER_START_KEY_NAV"
#define UBUS_LAUNCHER_END_KEY_NAV    "LAUNCHER_END_KEY_NAV"
#define UBUS_QUICKLIST_START_KEY_NAV "QUICKLIST_START_KEY_NAV"
#define UBUS_QUICKLIST_END_KEY_NAV   "QUICKLIST_END_KEY_NAV"

// Signal to send on icon action and that you want to request hiding the launcher
#define UBUS_LAUNCHER_ACTION_DONE "LAUNCHER_ACTION_DONE"

// Signal sent when a quicklist is shown.
#define UBUS_QUICKLIST_SHOWN "QUICKLIST_SHOWN"

// Signal sent when a tooltip is shown.
#define UBUS_TOOLTIP_SHOWN "TOOLTIP_SHOWN"

#endif // UBUS_MESSAGES_H
