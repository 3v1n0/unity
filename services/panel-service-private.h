/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef _PANEL_SERVICE_PRIVATE_H_
#define _PANEL_SERVICE_PRIVATE_H_

#include <X11/Xlib.h>
#include <X11/keysym.h>

G_BEGIN_DECLS

typedef struct _KeyBinding
{
  KeySym key;
  KeySym fallback;
  guint32 modifiers;
} KeyBinding;

#define AltMask Mod1Mask
#define SuperMask Mod4Mask

void parse_string_keybinding (const char *, KeyBinding *);

G_END_DECLS

#endif /* _PANEL_SERVICE_PRIVATE_H_ */
