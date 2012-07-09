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

#ifndef KEYBOARDUTIL_H
#define KEYBOARDUTIL_H

#include <glib.h>

#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>

namespace unity
{
namespace ui
{

class KeyboardUtil
{
public:
  KeyboardUtil(Display *display);
  ~KeyboardUtil();

  guint GetKeycodeAboveKeySymbol(KeySym key_symbol) const;

  static bool IsPrintableKeySymbol(KeySym key_symbol);
  static bool IsMoveKeySymbol(KeySym sym);

private:
  bool CompareOffsets (int current_x, int current_y, int best_x, int best_y) const;
  guint ConvertKeyToKeycode (XkbKeyPtr key) const;

  bool FindKeyInGeometry(XkbGeometryPtr geo, char *key_name, int& res_section, XkbBoundsRec& res_bounds) const;
  bool FindKeyInSectionAboveBounds (XkbGeometryPtr geo, int section, XkbBoundsRec const& target_bounds, guint &keycode) const;

  XkbBoundsRec GetAbsoluteKeyBounds (XkbKeyPtr key, XkbRowPtr row, XkbSectionPtr section, XkbGeometryPtr geo) const;

  XkbDescPtr keyboard_;
  Display *display_;
};

}
}

#endif // KEYBOARDUTIL_H

