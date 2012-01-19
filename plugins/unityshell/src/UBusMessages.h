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

#define UBUS_DASH_EXTERNAL_ACTIVATION "DASH_EXTERNAL_ACTIVATION"
#define UBUS_PLACE_VIEW_CLOSE_REQUEST "PLACE_VIEW_CLOSE_REQUEST"

// Request a PlaceEntry to be shown.
// Payload should be: (sus) = (id, section, search_string).
// id = entry->GetId(), search_string can be ""
#define UBUS_PLACE_ENTRY_ACTIVATE_REQUEST "PLACE_ENTRY_ACTIVATE_REQUEST"
#define UBUS_DASH_ABOUT_TO_SHOW "DASH_ABOUT_TO_SHOW"

// Signal send when places are shown or hidden
#define UBUS_PLACE_VIEW_HIDDEN "PLACE_VIEW_HIDDEN"
#define UBUS_PLACE_VIEW_SHOWN "PLACE_VIEW_SHOWN"

#define UBUS_PLACE_VIEW_QUEUE_DRAW "PLACE_VIEW_QUEUE_DRAW"

// Signal send by Launcher/Quicklist when it wants to exit key-nav and wants to
// get rid of keyboard-input-focus
#define UBUS_LAUNCHER_START_KEY_NAV  "LAUNCHER_START_KEY_NAV"
#define UBUS_LAUNCHER_END_KEY_NAV    "LAUNCHER_END_KEY_NAV"
#define UBUS_LAUNCHER_START_KEY_SWTICHER "LAUNCHER_START_KEY_SWITCHER"
#define UBUS_LAUNCHER_END_KEY_SWTICHER   "LAUNCHER_END_KEY_SWITCHER"
#define UBUS_LAUNCHER_ICON_URGENT_CHANGED "LAUNCHER_ICON_URGENT_CHANGED"
#define UBUS_QUICKLIST_START_KEY_NAV "QUICKLIST_START_KEY_NAV"
#define UBUS_QUICKLIST_END_KEY_NAV   "QUICKLIST_END_KEY_NAV"

// Signals that fired on various launcher dnd tasks
#define UBUS_LAUNCHER_START_DND      "LAUNCHER_START_DRAG"
#define UBUS_LAUNCHER_END_DND        "LAUNCHER_END_DRAG"
#define UBUS_LAUNCHER_ICON_START_DND "LAUNCHER_ICON_START_DND"
#define UBUS_LAUNCHER_ICON_END_DND   "LAUNCHER_ICON_END_DND"

// Signal to send on icon action and that you want to request hiding the launcher
#define UBUS_LAUNCHER_ACTION_DONE "LAUNCHER_ACTION_DONE"

// Signal to force the launcher into locked mode, (b)
#define UBUS_LAUNCHER_LOCK_HIDE "LAUNCHER_LOCK_HIDE"

// Signal sent when a quicklist is shown.
#define UBUS_QUICKLIST_SHOWN "QUICKLIST_SHOWN"

// Signal sent when a tooltip is shown.
#define UBUS_TOOLTIP_SHOWN "TOOLTIP_SHOWN"

// Signal sent when the background changes, contains average colour in float RGB format
#define UBUS_BACKGROUND_COLOR_CHANGED "BACKGROUND_COLOR_CHANGED"
#define UBUS_BACKGROUND_REQUEST_COLOUR_EMIT "REQUEST_BACKGROUND_COLOUR_EMIT"

#define UBUS_DASH_SIZE_CHANGED "DASH_SIZE_CHANGED"
// FIXME - fix the nux focus api so we don't need this
#define UBUS_RESULT_VIEW_KEYNAV_CHANGED "RESULT_VIEW_KEYNAV_CHANGED"

// Signals sent when the switcher is shown, hidden or changes selection
#define UBUS_SWITCHER_SHOWN             "SWITCHER_SHOWN"
#define UBUS_SWITCHER_SELECTION_CHANGED "SWITCHER_SELECTION_CHANGED"

#endif // UBUS_MESSAGES_H
