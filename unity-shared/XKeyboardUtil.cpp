// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011-2015 Canonical Ltd
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
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <gdk/gdk.h>
#include <string.h>
#include <cmath>

#include <NuxCore/Logger.h>

#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>

#include "KeyboardUtil.h"
#include "XKeyboardUtil.h"

namespace unity
{
namespace keyboard
{
namespace
{
DECLARE_LOGGER(logger, "unity.keyboard.xutil");

const unsigned int FETCH_MASK = (XkbGBN_KeyNamesMask |
                                 XkbGBN_ClientSymbolsMask |
                                 XkbGBN_GeometryMask);

class KeyboardUtil
{
public:
  KeyboardUtil(Display* display);
  ~KeyboardUtil();

  enum class Position
  {
    LEFT,
    RIGHT,
    ABOVE,
    BELOW
  };

  KeyCode GetKeyCodeNearKeySymbol(KeySym key_symbol, Position) const;
  KeySym GetNearKey(KeySym key_symbol, Position) const;

private:
  bool CompareOffsets(int current_x, int current_y, int best_x, int best_y) const;
  KeyCode ConvertKeyToKeycode(XkbKeyPtr key) const;

  bool FindKeyInGeometry(XkbGeometryPtr, char *key_name, int& res_section, XkbBoundsRec&) const;
  bool FindKeyInVertical(Position, XkbGeometryPtr, int section, XkbBoundsRec const&, KeyCode&) const;
  bool FindKeyOnSides(Position, XkbGeometryPtr, int section_index, XkbBoundsRec const&, KeyCode &keycode) const;

  XkbBoundsRec GetAbsoluteKeyBounds (XkbKeyPtr, XkbRowPtr, XkbSectionPtr, XkbGeometryPtr) const;

  Display *display_;
  XkbDescPtr keyboard_;
};


KeyboardUtil::KeyboardUtil(Display* display)
  : display_(display)
  , keyboard_(XkbGetKeyboard(display_, FETCH_MASK, XkbUseCoreKbd))
{}

KeyboardUtil::~KeyboardUtil()
{
  XkbFreeKeyboard(keyboard_, 0, True);
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

KeyCode KeyboardUtil::ConvertKeyToKeycode(XkbKeyPtr key) const
{
  if (!keyboard_)
    return 0;

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

bool KeyboardUtil::FindKeyInVertical(Position pos, XkbGeometryPtr geo, int section_index, XkbBoundsRec const& target_bounds, KeyCode &keycode) const
{
  XkbKeyPtr best = nullptr;
  int best_x_offset = G_MAXINT;
  int best_y_offset = G_MAXINT;

  int sections_in_geometry = geo->num_sections;
  for (int k = 0; k < sections_in_geometry; ++k)
  {
    XkbSectionPtr section = &geo->sections[section_index];

    for (int i = 0; i < section->num_rows; ++i)
    {
      XkbRowPtr row = &section->rows[i];

      int keys_in_row = row->num_keys;
      for (int j = 0; j < keys_in_row; ++j)
      {
        XkbKeyPtr key = &row->keys[j];
        XkbBoundsRec bounds = GetAbsoluteKeyBounds(key, row, section, geo);

        // make sure we are actually over the target bounds
        int hcenter = (bounds.x1 + bounds.x2) / 2;
        if (hcenter < target_bounds.x1 || hcenter > target_bounds.x2)
          continue;

        // make sure the key is actually above our target.
        int current_y_offset;

        if (pos == Position::ABOVE)
          current_y_offset = target_bounds.y1 - bounds.y2;
        else // if (pos == Position::BELOW)
          current_y_offset = bounds.y1 - target_bounds.y2;

        // make sure the key is actually above our target.
        if (current_y_offset < 0)
          continue;

        int current_x_offset = std::abs(hcenter - (target_bounds.x1 + target_bounds.x2) / 2);

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

bool KeyboardUtil::FindKeyOnSides(Position pos, XkbGeometryPtr geo, int section_index, XkbBoundsRec const& target_bounds, KeyCode &keycode) const
{
  XkbKeyPtr best = nullptr;
  int best_x_offset = G_MAXINT;
  int best_y_offset = G_MAXINT;

  int sections_in_geometry = geo->num_sections;
  for (int k = 0; k < sections_in_geometry; ++k)
  {
    XkbSectionPtr section = &geo->sections[section_index];
    for (int i = 0; i < section->num_rows; ++i)
    {
      XkbRowPtr row = &section->rows[i];

      int keys_in_row = row->num_keys;
      for (int j = 0; j < keys_in_row; ++j)
      {
        XkbKeyPtr key = &row->keys[j];
        XkbBoundsRec bounds = GetAbsoluteKeyBounds(key, row, section, geo);

        // make sure we are actually over the target bounds
        int vcenter = (bounds.y1 + bounds.y2) / 2;
        if (vcenter < target_bounds.y1 || vcenter > target_bounds.y2)
          continue;

        int current_x_offset;

        if (pos == Position::LEFT)
          current_x_offset = target_bounds.x1 - bounds.x2;
        else // if (pos == Position::RIGHT)
          current_x_offset = target_bounds.x2 - bounds.x1;

        // make sure the key is actually above our target.
        if (current_x_offset < 0)
          continue;

        int current_y_offset = std::abs(vcenter - (target_bounds.y1 + target_bounds.y2) / 2);

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

KeyCode KeyboardUtil::GetKeyCodeNearKeySymbol(KeySym key_symbol, Position pos) const
{
  KeyCode result = 0;
  KeyCode code = XKeysymToKeycode(display_, key_symbol);

  if (!code || !keyboard_)
    return result;

  if (keyboard_->min_key_code > code || keyboard_->max_key_code < code)
    return result;

  char* key_str = keyboard_->names->keys[code].name;

  int key_section;
  XkbBoundsRec key_bounds;
  bool found_key = FindKeyInGeometry(keyboard_->geom, key_str, key_section, key_bounds);

  if (found_key)
  {
    KeyCode maybe;

    switch (pos)
    {
      case Position::LEFT:
      case Position::RIGHT:
        found_key = FindKeyOnSides(pos, keyboard_->geom, key_section, key_bounds, maybe);
        break;
      case Position::ABOVE:
      case Position::BELOW:
        found_key = FindKeyInVertical(pos, keyboard_->geom, key_section, key_bounds, maybe);
        break;
      default:
        LOG_ERROR(logger) << "Impossible to find key near to " << XKeysymToString(key_symbol)
                          << " at position " << static_cast<unsigned>(pos); // Get actual type name.
        found_key = false;
        break;
    }

    if (found_key)
      result = maybe;
  }

  return result;
}

KeySym KeyboardUtil::GetNearKey(KeySym key_symbol, Position pos) const
{
  KeyCode keycode = GetKeyCodeNearKeySymbol(key_symbol, pos);
  const unsigned int group_interest = 0;
  const unsigned int shift_interest = 0;
  return XkbKeycodeToKeysym(display_, keycode, group_interest, shift_interest);
}


} // anon namespace


bool is_printable_key_symbol(unsigned long key_symbol)
{
  bool printable_key = false;

  if (key_symbol == XK_Delete ||
      key_symbol == XK_BackSpace ||
      key_symbol == XK_Return)
  {
    printable_key = true;
  }
  else
  {
    unsigned int unicode = gdk_keyval_to_unicode(key_symbol);
    printable_key = g_unichar_isprint(unicode);
  }

  return printable_key;
}

bool is_move_key_symbol(unsigned long key_symbol)
{
  return (key_symbol >= XK_Home && key_symbol <= XK_Begin);
}

KeySym get_key_above_key_symbol(Display* display, KeySym key_symbol)
{
  return KeyboardUtil(display).GetNearKey(key_symbol, KeyboardUtil::Position::ABOVE);
}

KeySym get_key_below_key_symbol(Display* display, KeySym key_symbol)
{
  return KeyboardUtil(display).GetNearKey(key_symbol, KeyboardUtil::Position::BELOW);
}

KeySym get_key_right_to_key_symbol(Display* display, KeySym key_symbol)
{
  return KeyboardUtil(display).GetNearKey(key_symbol, KeyboardUtil::Position::RIGHT);
}

KeySym get_key_left_to_key_symbol(Display* display, KeySym key_symbol)
{
  return KeyboardUtil(display).GetNearKey(key_symbol, KeyboardUtil::Position::LEFT);
}

} // namespace keyboard
} // namespace unity
