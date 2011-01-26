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

#define UBUS_HOME_BUTTON_ACTIVATED "PANEL_HOME_ACTIVATED"

// When other parts of Unity want to close the place view
#define UBUS_PLACE_VIEW_CLOSE_REQUEST "PLACE_VIEW_CLOSE_REQUEST"

// Request a PlaceEntry to be shown.
// Payload should be: (sus) = (id, section, search_string).
// id = entry->GetId(), search_string can be ""
#define UBUS_PLACE_ENTRY_ACTIVATE_REQUEST "PLACEE_ENTRY_ACTIVATE_REQUEST"

// Signal send when places are shown or hidden
#define UBUS_PLACE_VIEW_HIDDEN "PLACE_VIEW_HIDDEN"
#define UBUS_PLACE_VIEW_SHOWN "PLACE_VIEW_SHOWN"

#endif // UBUS_MESSAGES_H
