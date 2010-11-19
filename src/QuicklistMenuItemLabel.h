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

#ifndef QUICKLISTMENUITEMLABEL_H
#define QUICKLISTMENUITEMLABEL_H

#include "Nux/Nux.h"
#include "Nux/View.h"
#include "NuxImage/CairoGraphics.h"

#include "QuicklistMenuItem.h"

#if defined(NUX_OS_LINUX)
#include <X11/Xlib.h>
#endif

class QuicklistMenuItemLabel : public QuicklistMenuItem
{
  public:
    QuicklistMenuItemLabel (DbusmenuMenuitem* item,
                            NUX_FILE_LINE_PROTO);

    QuicklistMenuItemLabel (DbusmenuMenuitem* item,
                            bool              debug,
                            NUX_FILE_LINE_PROTO);

    ~QuicklistMenuItemLabel ();

    void PreLayoutManagement ();

    long PostLayoutManagement (long layoutResult);

    long ProcessEvent (nux::IEvent& event,
                       long         traverseInfo,
                       long         processEventInfo);

    void Draw (nux::GraphicsEngine& gfxContext,
               bool                 forceDraw);

    void DrawContent (nux::GraphicsEngine& gfxContext,
                      bool                 forceDraw);

    void PostDraw (nux::GraphicsEngine& gfxContext,
                   bool                 forceDraw);

        // public API
    void SetText (nux::NString text);

    void SetFontName (nux::NString fontName);

    void SetFontSize (float fontSize);

    void SetFontWeight (FontWeight fontWeight);

    void SetFontStyle (FontStyle fontStyle);

    virtual void GetTextExtents (int &width, int &height);

    
  private:
    nux::NString    _text;
    nux::NString    _fontName;
    float           _fontSize;
    FontStyle       _fontStyle;
    FontWeight      _fontWeight;
    nux::Color      _textColor;

    nux::CairoGraphics*   _cairoGraphics;
    nux::BaseTexture*     _texture2D;
    int                   _dpiX;
    int                   _dpiY;
    cairo_font_options_t* _fontOpts;

    int _pre_layout_width;
    int _pre_layout_height;

    void GetTextExtents (const TCHAR* font, int &width, int &height);
    void DrawText (cairo_t*   cr, int width, int height, nux::Color color);
        
    ////
    nux::BaseTexture* _normalTexture;
    nux::BaseTexture* _prelightTexture;

    void Initialize (DbusmenuMenuitem* item);
    void DrawText ();
    virtual void UpdateTexture ();
    void DrawRoundedRectangle ();
};

#endif // QUICKLISTMENUITEMLABEL_H
