// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#include <gdk/gdk.h>
#include <string.h>
#include <cmath>

#include "KeyboardUtil.h"

namespace unity {
namespace ui {

KeyboardUtil::KeyboardUtil(Display *display)
  : display_(display)
{
  unsigned int fetch_mask = XkbGBN_KeyNamesMask | XkbGBN_ClientSymbolsMask | XkbGBN_GeometryMask;
  keyboard_ = XkbGetKeyboard (display, fetch_mask, XkbUseCoreKbd);
}

KeyboardUtil::~KeyboardUtil()
{
  XkbFreeKeyboard (keyboard_, 0, True);
}

bool KeyboardUtil::FindKeyInGeometry(XkbGeometryPtr geo, char *key_name, int& res_section, XkbBoundsRec& res_bounds) const
{
  // seems that Xkb does not give null terminated strings... was painful
  int name_length = XkbKeyNameLength;

  int sections_in_geometry = geo->num_sections;
  for (int i = 0; i < sections_in_geometry; i++)
  {
    XkbSectionPtr section = &geo->sections[i];

    int rows_in_section = section->num_rows;
    for (int j = 0; j < rows_in_section; j++)
    {
      XkbRowPtr row = &section->rows[j];
      int keys_in_row = row->num_keys;
      for (int k = 0; k < keys_in_row; k++)
      {
        XkbKeyPtr key = &row->keys[k];
        if (!strncmp (key->name.name, key_name, name_length))
        {
          res_section = i;
          res_bounds = GetAbsoluteKeyBounds(key, row, section, geo);
          return true;
        }
        // end key
      }
      // end row
    }
    // end section
  }

  return false;
}

bool KeyboardUtil::CompareOffsets(int current_x, int current_y, int best_x, int best_y) const
{
  // never EVER prefer something higher on the keyboard than what we have
  if (current_y > best_y)
    return false;

  if (current_x < best_x || current_y < best_y)
    return true;

  return false;
}

guint KeyboardUtil::ConvertKeyToKeycode(XkbKeyPtr key) const
{
  int min_code = keyboard_->min_key_code;
  int max_code = keyboard_->max_key_code;

  for (int i = min_code; i < max_code; i++)
  {
    if (!strncmp(key->name.name, keyboard_->names->keys[i].name, XkbKeyNameLength))
      return i;
  }
  return 0;
}

XkbBoundsRec KeyboardUtil::GetAbsoluteKeyBounds(XkbKeyPtr key, XkbRowPtr row, XkbSectionPtr section, XkbGeometryPtr geo) const
{
  XkbShapePtr shape = XkbKeyShape(geo, key);

  XkbBoundsRec result;
  result = shape->bounds;

  int x_offset = 0;
  int y_offset = 0;
  int i = 0;
  while (&row->keys[i] != key)
  {
    XkbKeyPtr local_key = &row->keys[i];
    XkbShapePtr local_shape = XkbKeyShape(geo, local_key);

    if (row->vertical)
      y_offset += local_shape->bounds.y2 - local_shape->bounds.y1;
    else
      x_offset += local_shape->bounds.x2 - local_shape->bounds.x1;

    i++;
  }

  result.x1 += row->left + section->left + key->gap + x_offset;
  result.x2 += row->left + section->left + key->gap + x_offset;
  result.y1 += row->top + section->top + key->gap + y_offset;
  result.y2 += row->top + section->top + key->gap + y_offset;

  return result;
}

bool KeyboardUtil::FindKeyInSectionAboveBounds(XkbGeometryPtr geo, int section_index, XkbBoundsRec const& target_bounds, guint &keycode) const
{
  XkbKeyPtr best = NULL;
  int best_x_offset = G_MAXINT;
  int best_y_offset = G_MAXINT;

  int sections_in_geometry = geo->num_sections;
  for (int k = 0; k < sections_in_geometry; k++)
  {

  XkbSectionPtr section = &geo->sections[section_index];
  int rows_in_section = section->num_rows;
  for (int i = 0; i < rows_in_section; i++)
  {
    XkbRowPtr row = &section->rows[i];

    int keys_in_row = row->num_keys;
    for (int j = 0; j < keys_in_row; j++)
    {
      XkbKeyPtr key = &row->keys[j];
      XkbBoundsRec bounds = GetAbsoluteKeyBounds (key, row, section, geo);

      // make sure we are actually over the target bounds
      int center = (bounds.x1 + bounds.x2) / 2;
      if (center < target_bounds.x1 || center > target_bounds.x2)
        continue;

      // make sure the key is actually above our target.
      int current_y_offset = target_bounds.y1 - bounds.y2;
      if (current_y_offset < 0)
        continue;

      int current_x_offset = std::abs(center - (target_bounds.x1 + target_bounds.x2) / 2);

      if (CompareOffsets(current_x_offset, current_y_offset, best_x_offset, best_y_offset))
      {
        best = key;
        best_x_offset = current_x_offset;
        best_y_offset = current_y_offset;
       }
    }
  }
  }

  if (best)
  {
    keycode = ConvertKeyToKeycode(best);
    return true;
  }
  return false;
}

guint KeyboardUtil::GetKeycodeAboveKeySymbol(KeySym key_symbol) const
{
  guint result = 0;

  int code = XKeysymToKeycode(display_, key_symbol);

  if (!code)
    return result;

  if (keyboard_->min_key_code > code || keyboard_->max_key_code < code)
    return result;

  char* key_str = keyboard_->names->keys[code].name;


  int key_section;
  XkbBoundsRec key_bounds;
  bool found_key = FindKeyInGeometry(keyboard_->geom, key_str, key_section, key_bounds);

  if (found_key)
  {
    guint maybe;
    found_key = FindKeyInSectionAboveBounds(keyboard_->geom, key_section, key_bounds, maybe);

    if (found_key)
      result = maybe;
  }

  return result;
}

bool KeyboardUtil::IsPrintableKeySymbol(KeySym sym)
{
  bool printable_key = false;

  if (sym == XK_Delete || sym == XK_BackSpace || sym == XK_Return)
  {
    printable_key = true;
  }
  else
  {
    unsigned int unicode = gdk_keyval_to_unicode(sym);
    printable_key = g_unichar_isprint(unicode);
  }

  return printable_key;
}

bool KeyboardUtil::IsMoveKeySymbol(KeySym sym)
{
  bool move_key = false;

  if (sym >= XK_Home && sym <= XK_Begin)
  {
    move_key = true;
  }

  return move_key;
}

}
}