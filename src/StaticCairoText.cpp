// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Jay Taoko <jay.taoko@canonical.com>
 * Authored by: Mirco Müller <mirco.mueller@canonical.com
 */

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "Nux/Nux.h"
#include "Nux/Layout.h"
#include "Nux/HLayout.h"
#include "Nux/VLayout.h"
#include "Nux/Validator.h"
#include "StaticCairoText.h"

namespace nux
{
  StaticCairoText::StaticCairoText (const TCHAR* text,
                                    NUX_FILE_LINE_DECL) :
  View (NUX_FILE_LINE_PARAM),
  _fontstring (NULL),
  _cairoGraphics (NULL),
  _texture2D (NULL)
{
  _textColor  = Color(1.0f, 1.0f, 1.0f, 1.0f);
  _text       = TEXT (text);
  _texture2D  = 0;

  SetMinimumSize (1, 1);
  _ellipsize = NUX_ELLIPSIZE_END;
  _align = NUX_ALIGN_LEFT;
  _fontstring = NULL;
}

StaticCairoText::~StaticCairoText ()
{
  GtkSettings* settings = gtk_settings_get_default (); // not ref'ed
  g_signal_handlers_disconnect_by_func (settings,
                                        (void *) &StaticCairoText::OnFontChanged,
                                        this);
  if (_texture2D)
    delete (_texture2D);

  if (_fontstring)
    g_free (_fontstring);
}

void
StaticCairoText::SetTextEllipsize (EllipsizeState state)
{
  _ellipsize = state;
  NeedRedraw ();
}

void
StaticCairoText::SetTextAlignment (AlignState state)
{
  _align = state;
  NeedRedraw ();
}

void StaticCairoText::PreLayoutManagement ()
{
  int textWidth  = 0;
  int textHeight = 0;
  GetTextExtents (textWidth, textHeight);

  _pre_layout_width = GetBaseWidth ();
  _pre_layout_height = GetBaseHeight ();

  SetBaseSize (textWidth, textHeight);

  if((_texture2D == 0) )
  {
    GtkSettings* settings = gtk_settings_get_default (); // not ref'ed
    g_signal_connect (settings, "notify::gtk-font-name",
                      (GCallback) &StaticCairoText::OnFontChanged, this);
    g_signal_connect (settings, "notify::gtk-xft-dpi",
                      (GCallback) &StaticCairoText::OnFontChanged, this);

    UpdateTexture ();
  }

  View::PreLayoutManagement ();
}

long StaticCairoText::PostLayoutManagement (long layoutResult)
{
//  long result = View::PostLayoutManagement (layoutResult);

  long result = 0;

  int w = GetBaseWidth();
  int h = GetBaseHeight();

  if (_pre_layout_width < w)
    result |= eLargerWidth;
  else if (_pre_layout_width > w)
    result |= eSmallerWidth;
  else
    result |= eCompliantWidth;

  if (_pre_layout_height < h)
    result |= eLargerHeight;
  else if (_pre_layout_height > h)
    result |= eSmallerHeight;
  else
    result |= eCompliantHeight;

  return result;
}

long
StaticCairoText::ProcessEvent (IEvent& event,
                                    long    traverseInfo,
                                    long    processEventInfo)
{
  long ret = traverseInfo;

  ret = PostProcessEvent2 (event, ret, processEventInfo);
  return ret;
}

void
StaticCairoText::Draw (GraphicsEngine& gfxContext,
                            bool             forceDraw)
{
  Geometry base = GetGeometry ();

  if (_texture2D == 0)
    UpdateTexture ();

  gfxContext.PushClippingRectangle (base);

  TexCoordXForm texxform;
  texxform.SetWrap (TEXWRAP_REPEAT, TEXWRAP_REPEAT);
  texxform.SetTexCoordType (TexCoordXForm::OFFSET_COORD);

  gfxContext.GetRenderStates ().SetBlend (true);
  gfxContext.GetRenderStates ().SetPremultipliedBlend (nux::SRC_OVER);

  gfxContext.QRP_1Tex (base.x,
                        base.y,
                        base.width,
                        base.height,
                        _texture2D->GetDeviceTexture(),
                        texxform,
                        _textColor);

  gfxContext.GetRenderStates().SetBlend (false);

  gfxContext.PopClippingRectangle ();
}

void
StaticCairoText::DrawContent (GraphicsEngine& gfxContext,
                                   bool             forceDraw)
{
  // intentionally left empty
}

void
StaticCairoText::PostDraw (GraphicsEngine& gfxContext,
                                bool             forceDraw)
{
  // intentionally left empty
}

void
StaticCairoText::SetText (NString text)
{
  if (_text != text)
  {
    _text = text;
    UpdateTexture ();
    sigTextChanged.emit (this);
  }
}

void
StaticCairoText::SetTextColor (Color textColor)
{
  if (_textColor != textColor)
  {
    _textColor = textColor;
    sigTextColorChanged.emit (this);
  }
}

void
StaticCairoText::SetFont (const char *fontstring)
{
  g_free (_fontstring);
  _fontstring = g_strdup (fontstring);
  NeedRedraw ();
  sigFontChanged.emit (this);
}


void StaticCairoText::GetTextExtents (int &width, int &height)
{
  GtkSettings* settings = gtk_settings_get_default (); // not ref'ed
  gchar*       fontName = NULL;

  if (_fontstring == NULL)
  {
    g_object_get (settings, "gtk-font-name", &fontName, NULL);
  }
  else
  {
    fontName = g_strdup (_fontstring);
  }

  GetTextExtents (fontName, width, height);
  g_free (fontName);
}

void StaticCairoText::GetTextExtents (const TCHAR* font,
                                      int&  width,
                                      int&  height)
{
  cairo_surface_t*      surface  = NULL;
  cairo_t*              cr       = NULL;
  PangoLayout*          layout   = NULL;
  PangoFontDescription* desc     = NULL;
  PangoContext*         pangoCtx = NULL;
  PangoRectangle        logRect  = {0, 0, 0, 0};
  int                   dpi      = 0;
  GdkScreen*            screen   = gdk_screen_get_default ();   // is not ref'ed
  GtkSettings*          settings = gtk_settings_get_default (); // is not ref'ed

  // sanity check
  if (!font)
    return;

  int maxwidth = GetMaximumWidth ();

  surface = cairo_image_surface_create (CAIRO_FORMAT_A1, 1, 1);
  cr = cairo_create (surface);
  cairo_set_font_options (cr, gdk_screen_get_font_options (screen));
  layout = pango_cairo_create_layout (cr);
  desc = pango_font_description_from_string (font);
  pango_font_description_set_weight (desc, PANGO_WEIGHT_NORMAL);
  pango_layout_set_font_description (layout, desc);
  pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);

  if (_ellipsize == NUX_ELLIPSIZE_START)
    pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_START);
  else if (_ellipsize == NUX_ELLIPSIZE_MIDDLE)
    pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_MIDDLE);
  else if (_ellipsize == NUX_ELLIPSIZE_END)
    pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
  else
    pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_NONE);

  if (_align == NUX_ALIGN_LEFT)
    pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);
  else if (_align == NUX_ALIGN_CENTRE)
    pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
  else
    pango_layout_set_alignment (layout, PANGO_ALIGN_RIGHT);


  pango_layout_set_markup (layout, _text.GetTCharPtr(), -1);
  pango_layout_set_height (layout, -2);
  pango_layout_set_width (layout, maxwidth * PANGO_SCALE);

  pangoCtx = pango_layout_get_context (layout); // is not ref'ed
  pango_cairo_context_set_font_options (pangoCtx,
                                        gdk_screen_get_font_options (screen));
  g_object_get (settings, "gtk-xft-dpi", &dpi, NULL);
  if (dpi == -1)
  {
    // use some default DPI-value
    pango_cairo_context_set_resolution (pangoCtx, 96.0f);
  }
  else
  {
    pango_cairo_context_set_resolution (pangoCtx,
                                        (float) dpi / (float) PANGO_SCALE);
  }
  pango_layout_context_changed (layout);
  pango_layout_get_extents (layout, NULL, &logRect);

  width  = logRect.width / PANGO_SCALE;
  height = logRect.height / PANGO_SCALE;

  // clean up
  pango_font_description_free (desc);
  g_object_unref (layout);
  cairo_destroy (cr);
  cairo_surface_destroy (surface);

}

void StaticCairoText::DrawText (cairo_t*   cr,
                                int        width,
                                int        height,
                                Color color)
{
  int                   textWidth  = 0;
  int                   textHeight = 0;
  PangoLayout*          layout     = NULL;
  PangoFontDescription* desc       = NULL;
  PangoContext*         pangoCtx   = NULL;
  int                   dpi        = 0;
  GdkScreen*            screen     = gdk_screen_get_default ();   // not ref'ed
  GtkSettings*          settings   = gtk_settings_get_default (); // not ref'ed
  gchar*                fontName   = NULL;

  if (_fontstring == NULL)
    g_object_get (settings, "gtk-font-name", &fontName, NULL);
  else
    fontName = g_strdup (_fontstring);

  GetTextExtents (fontName, textWidth, textHeight);

  cairo_set_font_options (cr, gdk_screen_get_font_options (screen));
  layout = pango_cairo_create_layout (cr);
  desc = pango_font_description_from_string (fontName);

  pango_layout_set_font_description (layout, desc);
  pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);

  if (_ellipsize == NUX_ELLIPSIZE_START)
    pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_START);
  else if (_ellipsize == NUX_ELLIPSIZE_MIDDLE)
    pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_MIDDLE);
  else if (_ellipsize == NUX_ELLIPSIZE_END)
    pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
  else
    pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_NONE);


  if (_align == NUX_ALIGN_LEFT)
    pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);
  else if (_align == NUX_ALIGN_CENTRE)
    pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
  else
    pango_layout_set_alignment (layout, PANGO_ALIGN_RIGHT);

  pango_layout_set_markup (layout, _text.GetTCharPtr(), -1);
  pango_layout_set_width (layout, textWidth * PANGO_SCALE);
  pango_layout_set_height (layout, -2);
  pangoCtx = pango_layout_get_context (layout); // is not ref'ed
  pango_cairo_context_set_font_options (pangoCtx,
                                        gdk_screen_get_font_options (screen));
  g_object_get (settings, "gtk-xft-dpi", &dpi, NULL);
  if (dpi == -1)
  {
    // use some default DPI-value
    pango_cairo_context_set_resolution (pangoCtx, 96.0f);
  }
  else
  {
    pango_cairo_context_set_resolution (pangoCtx,
                                        (float) dpi / (float) PANGO_SCALE);
  }

  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba (cr, 0.0f, 0.0f, 0.0f, 0.0f);
  cairo_paint (cr);
  cairo_set_source_rgba (cr, color.R (),color.G (), color.B (), color.A ());

  pango_layout_context_changed (layout);

  cairo_move_to (cr, 0.0f, 0.0f);
  pango_cairo_show_layout (cr, layout);

  // clean up
  pango_font_description_free (desc);
  g_object_unref (layout);
  g_free (fontName);
}

void StaticCairoText::UpdateTexture ()
{
  int width = 0;
  int height = 0;
  GetTextExtents(width, height);

  SetBaseSize(width, height);

  _cairoGraphics = new CairoGraphics (CAIRO_FORMAT_ARGB32,
                                      GetBaseWidth (),
                                      GetBaseHeight ());
  cairo_t *cr = cairo_reference (_cairoGraphics->GetContext ());

  DrawText (cr, GetBaseWidth (), GetBaseHeight (), _textColor);

  cairo_destroy (cr);

  NBitmapData* bitmap = _cairoGraphics->GetBitmap ();

  // NTexture2D is the high level representation of an image that is backed by
  // an actual opengl texture.

  if (_texture2D)
    _texture2D->UnReference ();

  _texture2D = GetThreadGLDeviceFactory()->CreateSystemCapableTexture ();
  _texture2D->Update (bitmap);

  cairo_destroy (cr);

  delete _cairoGraphics;
}

void StaticCairoText::OnFontChanged (GObject *gobject, GParamSpec *pspec,
                                     gpointer data)
{
  StaticCairoText *self = (StaticCairoText*) data;
  self->UpdateTexture ();
  self->sigFontChanged.emit (self);
}

}
