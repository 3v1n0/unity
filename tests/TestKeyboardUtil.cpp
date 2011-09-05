/*
 * Copyright 2010 Canonical Ltd.
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <dbus/dbus-glib.h>

#include <X11/Xlib.h>

#include "KeyboardUtil.h"

using namespace unity::ui;

void testKey (KeyboardUtil & key_util, Display *x_display, const char *key)
{
  guint above_keycode = key_util.GetKeycodeAboveKeySymbol (XStringToKeysym(key));

  KeySym above_keysym = XKeycodeToKeysym (x_display, above_keycode, 0);
  if (above_keysym != NoSymbol)
    printf ("Key above %s is %s\n", key, XKeysymToString(above_keysym));
  else
    printf ("Could not find key above %s!\n", key);
}

int main(int argc, char** argv)
{
  Display* x_display = XOpenDisplay (NULL);

  KeyboardUtil key_util (x_display);
  testKey (key_util, x_display, "Tab");
  testKey (key_util, x_display, "Shift_R");
  testKey (key_util, x_display, "Control_L");
  testKey (key_util, x_display, "space");
  testKey (key_util, x_display, "comma");
  testKey (key_util, x_display, "a");
  testKey (key_util, x_display, "b");
  testKey (key_util, x_display, "c");
  testKey (key_util, x_display, "d");
  testKey (key_util, x_display, "e");
  testKey (key_util, x_display, "f");
  testKey (key_util, x_display, "g");
  testKey (key_util, x_display, "h");
  testKey (key_util, x_display, "i");
  testKey (key_util, x_display, "j");
  testKey (key_util, x_display, "k");
  testKey (key_util, x_display, "l");
  testKey (key_util, x_display, "m");
  testKey (key_util, x_display, "n");
  testKey (key_util, x_display, "o");
  testKey (key_util, x_display, "p");
  testKey (key_util, x_display, "k");
  testKey (key_util, x_display, "r");
  testKey (key_util, x_display, "s");
  testKey (key_util, x_display, "t");
  testKey (key_util, x_display, "u");
  testKey (key_util, x_display, "v");
  testKey (key_util, x_display, "w");
  testKey (key_util, x_display, "x");
  testKey (key_util, x_display, "y");
  testKey (key_util, x_display, "z");

  return 0;
}