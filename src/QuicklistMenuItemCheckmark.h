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
 * Authored by: Mirco Müller <mirco.mueller@canonical.com>
 */

#ifndef QUICKLISTMENUITEMCHECKMARK_H
#define QUICKLISTMENUITEMCHECKMARK_H

#include "Nux/Nux.h"
#include "Nux/View.h"
#include "QuicklistMenuItem.h"

#if defined(NUX_OS_LINUX)
#include <X11/Xlib.h>
#endif

class QuicklistMenuItemCheckmark : public QuicklistMenuItem
{
  public:
    QuicklistMenuItemCheckmark (DbusmenuMenuitem* item,
                                NUX_FILE_LINE_PROTO);

    QuicklistMenuItemCheckmark (DbusmenuMenuitem* item,
                                bool              debug,
                                NUX_FILE_LINE_PROTO);

    ~QuicklistMenuItemCheckmark ();
	  
  private:
    nux::BaseTexture* _normalTexture;
    nux::BaseTexture* _prelightTexture;

    void DrawText ();
    void GetExtents ();
    void UpdateTextures ();
    void DrawRoundedRectangle ();
};

#endif // QUICKLISTMENUITEMCHECKMARK_H
