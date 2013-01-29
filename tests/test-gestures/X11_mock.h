/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Daniel d'Andrada <daniel.dandrada@canonical.com>
 *
 */

#ifndef X11_MOCK_H
#define X11_MOCK_H

#include <X11/Xlib.h>

Cursor XCreateFontCursorMock(Display *display, unsigned int shape);

int XFreeCursorMock(Display *display, Cursor cursor);

int XSyncMock(Display *display, Bool discard);

int XWarpPointerMock(Display *display, Window src_w, Window dest_w,
                     int src_x, int src_y,
                     unsigned int src_width, unsigned int src_height,
                     int dest_x, int dest_y);

#endif // X11_MOCK_H
