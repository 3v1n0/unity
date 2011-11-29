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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 *
 */

#include "config.h"

#include <glib.h>
#include <pango/pango.h>

#include "Nux/Nux.h"
#include "StaticCairoText.h"

static void
assert_width_increases_with_substring_length (const gchar *test_string)
{
  nux::StaticCairoText text("");
  gchar *substring = new gchar[strlen(test_string)];

  int width, oldwidth;
  int height;

  text.GetTextExtents(width, height);
  // The empty string should have no width...
  g_assert_cmpint (width, ==, 0);
  oldwidth = width;

  for (int substr_len = 1; substr_len <= g_utf8_strlen(test_string, -1); ++substr_len) {
    text.SetText (g_utf8_strncpy (substring, test_string, substr_len));
    text.GetTextExtents(width, height);
    g_assert_cmpint(width, >, oldwidth);
    oldwidth = width;
  }
  
  delete substring;
}

static void
TestLeftToRightExtentIncreasesWithLength ()
{
  const gchar *test_string = "Just a string of text";

  g_assert(g_utf8_validate(test_string, -1, NULL));
  g_assert(pango_find_base_dir(test_string, -1) == PANGO_DIRECTION_LTR);

  assert_width_increases_with_substring_length (test_string);
}

static void
TestRightToLeftExtentIncreasesWithLength ()
{
  const gchar *test_string = "מחרוזת אקראית עברית";

  g_assert(g_utf8_validate(test_string, -1, NULL));
  g_assert(pango_find_base_dir(test_string, -1) == PANGO_DIRECTION_RTL);
  
  assert_width_increases_with_substring_length (test_string);
}

void
TestStaticCairoTextCreateSuite()
{
#define _DOMAIN "/Unit/StaticCairoText"

  g_test_add_func(_DOMAIN"/TextLeftToRightExtentIncreasesWithLength", TestLeftToRightExtentIncreasesWithLength);
  g_test_add_func(_DOMAIN"/TestRightToLeftExtentIncreasesWithLength", TestRightToLeftExtentIncreasesWithLength);
}

