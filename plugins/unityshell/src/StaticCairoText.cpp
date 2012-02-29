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

#include <Nux/Nux.h>
#include <Nux/Layout.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <Nux/Validator.h>

#include "CairoTexture.h"
#include "StaticCairoText.h"

using unity::texture_from_cairo_graphics;

// TODO: Tim Penhey 2011-05-16
// We shouldn't be pushing stuff into the nux namespace from the unity
// codebase, that is just rude.
namespace nux
{
  NUX_IMPLEMENT_OBJECT_TYPE (StaticCairoText);

StaticCairoText::StaticCairoText(std::string const& text,
                                 NUX_FILE_LINE_DECL) :
  View(NUX_FILE_LINE_PARAM),
  _baseline(0),
  _fontstring(NULL),
  _cairoGraphics(NULL),
  _texture2D(NULL),
  _lines(-2),
  _actual_lines(0)
{
  _textColor  = Color(1.0f, 1.0f, 1.0f, 1.0f);
  _text       = text;
  _texture2D  = 0;
  _need_new_extent_cache = true;
  _pre_layout_width = 0;
  _pre_layout_height = 0;

  SetMinimumSize(1, 1);
  _ellipsize = NUX_ELLIPSIZE_END;
  _align = NUX_ALIGN_LEFT;
  _valign = NUX_ALIGN_TOP;
  _fontstring = NULL;

  _accept_key_nav_focus = false;
  SetAcceptKeyNavFocusOnMouseDown(false);
}

StaticCairoText::~StaticCairoText()
{
  GtkSettings* settings = gtk_settings_get_default();  // not ref'ed
  g_signal_handlers_disconnect_by_func(settings,
                                       (void*) &StaticCairoText::OnFontChanged,
                                       this);
  if (_texture2D)
    _texture2D->UnReference();

  if (_fontstring)
    g_free(_fontstring);
}

void
StaticCairoText::SetTextEllipsize(EllipsizeState state)
{
  _ellipsize = state;
  NeedRedraw();
}

void
StaticCairoText::SetTextAlignment(AlignState state)
{
  _align = state;
  NeedRedraw();
}

void
StaticCairoText::SetTextVerticalAlignment(AlignState state)
{
  _valign = state;
  QueueDraw();
}

void
StaticCairoText::SetLines(int lines)
{
  _lines = lines;
  UpdateTexture();
  QueueDraw();
}

void StaticCairoText::PreLayoutManagement()
{
  int textWidth  = 0;
  int textHeight = 0;

  textWidth = _cached_extent_width;
  textHeight = _cached_extent_height;

  _pre_layout_width = GetBaseWidth();
  _pre_layout_height = GetBaseHeight();

  SetBaseSize(textWidth, textHeight);

  if ((_texture2D == 0))
  {
    GtkSettings* settings = gtk_settings_get_default();  // not ref'ed
    g_signal_connect(settings, "notify::gtk-font-name",
                     (GCallback) &StaticCairoText::OnFontChanged, this);
    g_signal_connect(settings, "notify::gtk-xft-dpi",
                     (GCallback) &StaticCairoText::OnFontChanged, this);

    UpdateTexture();
  }

  View::PreLayoutManagement();
}

long StaticCairoText::PostLayoutManagement(long layoutResult)
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

void
StaticCairoText::Draw(GraphicsEngine& gfxContext,
                      bool             forceDraw)
{
  Geometry base = GetGeometry();

  if (!_texture2D || _cached_base_width != base.width || _cached_base_height != base.height)
  {
    _cached_base_width = base.width;
    _cached_base_height = base.height;
    UpdateTexture();
  }

  gfxContext.PushClippingRectangle(base);

  gPainter.PaintBackground(gfxContext, base);

  TexCoordXForm texxform;
  texxform.SetWrap(TEXWRAP_REPEAT, TEXWRAP_REPEAT);
  texxform.SetTexCoordType(TexCoordXForm::OFFSET_COORD);

  unsigned int alpha = 0, src = 0, dest = 0;

  gfxContext.GetRenderStates().GetBlend(alpha, src, dest);
  gfxContext.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  Color col = color::Black;
  col.alpha = 0;
  gfxContext.QRP_Color(base.x,
                       base.y,
                       base.width,
                       base.height,
                       col);

  gfxContext.QRP_1Tex(base.x,
                      base.y + ((base.height - _cached_extent_height) / 2),
                      base.width,
                      base.height,
                      _texture2D->GetDeviceTexture(),
                      texxform,
                      _textColor);

  gfxContext.GetRenderStates().SetBlend(alpha, src, dest);

  gfxContext.PopClippingRectangle();
}

void
StaticCairoText::DrawContent(GraphicsEngine& gfxContext,
                             bool             forceDraw)
{
  // intentionally left empty
}

void
StaticCairoText::PostDraw(GraphicsEngine& gfxContext,
                          bool             forceDraw)
{
  // intentionally left empty
}

void
StaticCairoText::SetText(std::string const& text)
{
  if (_text != text)
  {
    _text = text;
    _need_new_extent_cache = true;
    int width = 0;
    int height = 0;
    GetTextExtents(width, height);
    UpdateTexture();
    sigTextChanged.emit(this);
  }
}

std::string
StaticCairoText::GetText() const
{
  return _text;
}

void
StaticCairoText::SetTextColor(Color const& textColor)
{
  if (_textColor != textColor)
  {
    _textColor = textColor;
    UpdateTexture();
    QueueDraw();

    sigTextColorChanged.emit(this);
  }
}

void
StaticCairoText::SetFont(const char* fontstring)
{
  g_free(_fontstring);
  _fontstring = g_strdup(fontstring);
  _need_new_extent_cache = true;
  int width = 0;
  int height = 0;
  GetTextExtents(width, height);
  SetMinimumHeight(height);
  NeedRedraw();
  sigFontChanged.emit(this);
}

int
StaticCairoText::GetLineCount()
{
  return _actual_lines;
}

int StaticCairoText::GetBaseline() const
{
  return _baseline;
}

void StaticCairoText::GetTextExtents(int& width, int& height)
{
  GtkSettings* settings = gtk_settings_get_default();  // not ref'ed
  gchar*       fontName = NULL;

  if (_fontstring == NULL)
  {
    g_object_get(settings, "gtk-font-name", &fontName, NULL);
  }
  else
  {
    fontName = g_strdup(_fontstring);
  }

  GetTextExtents(fontName, width, height);
  g_free(fontName);
}

void StaticCairoText::GetTextExtents(const TCHAR* font,
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
  GdkScreen*            screen   = gdk_screen_get_default();    // is not ref'ed
  GtkSettings*          settings = gtk_settings_get_default();  // is not ref'ed

  // sanity check
  if (!font)
    return;

  if (!_need_new_extent_cache)
  {
    width = _cached_extent_width;
    height = _cached_extent_height;
    return;
  }

  int maxwidth = GetMaximumWidth();

  surface = cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1);
  cr = cairo_create(surface);
  cairo_set_font_options(cr, gdk_screen_get_font_options(screen));

  layout = pango_cairo_create_layout(cr);
  desc = pango_font_description_from_string(font);
  pango_layout_set_font_description(layout, desc);
  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

  if (_ellipsize == NUX_ELLIPSIZE_START)
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_START);
  else if (_ellipsize == NUX_ELLIPSIZE_MIDDLE)
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_MIDDLE);
  else if (_ellipsize == NUX_ELLIPSIZE_END)
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
  else
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);

  if (_align == NUX_ALIGN_LEFT)
    pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
  else if (_align == NUX_ALIGN_CENTRE)
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
  else
    pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);

  pango_layout_set_markup(layout, _text.c_str(), -1);
  pango_layout_set_height(layout, _lines);
  pango_layout_set_width(layout, maxwidth * PANGO_SCALE);

  pangoCtx = pango_layout_get_context(layout);  // is not ref'ed
  pango_cairo_context_set_font_options(pangoCtx,
                                       gdk_screen_get_font_options(screen));
  g_object_get(settings, "gtk-xft-dpi", &dpi, NULL);
  if (dpi == -1)
  {
    // use some default DPI-value
    pango_cairo_context_set_resolution(pangoCtx, 96.0f);
  }
  else
  {
    pango_cairo_context_set_resolution(pangoCtx,
                                       (float) dpi / (float) PANGO_SCALE);
  }
  pango_layout_context_changed(layout);
  pango_layout_get_extents(layout, NULL, &logRect);

  width  = logRect.width / PANGO_SCALE;
  height = logRect.height / PANGO_SCALE;
  _cached_extent_height = height;
  _cached_extent_width = width;
  _baseline = pango_layout_get_baseline(layout) / PANGO_SCALE;

  // clean up
  pango_font_description_free(desc);
  g_object_unref(layout);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
}

void StaticCairoText::DrawText(cairo_t*   cr,
                               int        width,
                               int        height,
                               Color color)
{
  PangoLayout*          layout     = NULL;
  PangoFontDescription* desc       = NULL;
  PangoContext*         pangoCtx   = NULL;
  int                   dpi        = 0;
  GdkScreen*            screen     = gdk_screen_get_default();    // not ref'ed
  GtkSettings*          settings   = gtk_settings_get_default();  // not ref'ed
  gchar*                fontName   = NULL;

  if (_fontstring == NULL)
    g_object_get(settings, "gtk-font-name", &fontName, NULL);
  else
    fontName = g_strdup(_fontstring);

  cairo_set_font_options(cr, gdk_screen_get_font_options(screen));
  layout = pango_cairo_create_layout(cr);
  desc = pango_font_description_from_string(fontName);

  pango_layout_set_font_description(layout, desc);
  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

  if (_ellipsize == NUX_ELLIPSIZE_START)
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_START);
  else if (_ellipsize == NUX_ELLIPSIZE_MIDDLE)
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_MIDDLE);
  else if (_ellipsize == NUX_ELLIPSIZE_END)
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
  else
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);

  if (_align == NUX_ALIGN_LEFT)
    pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
  else if (_align == NUX_ALIGN_CENTRE)
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
  else
    pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);

  pango_layout_set_markup(layout, _text.c_str(), -1);
  pango_layout_set_width(layout, width * PANGO_SCALE);
  pango_layout_set_height(layout, height * PANGO_SCALE);

  pango_layout_set_height(layout, _lines);
  pangoCtx = pango_layout_get_context(layout);  // is not ref'ed
  pango_cairo_context_set_font_options(pangoCtx,
                                       gdk_screen_get_font_options(screen));
  g_object_get(settings, "gtk-xft-dpi", &dpi, NULL);
  if (dpi == -1)
  {
    // use some default DPI-value
    pango_cairo_context_set_resolution(pangoCtx, 96.0f);
  }
  else
  {
    pango_cairo_context_set_resolution(pangoCtx,
                                       (float) dpi / (float) PANGO_SCALE);
  }

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);

  pango_layout_context_changed(layout);

  cairo_move_to(cr, 0.0f, 0.0f);
  pango_cairo_show_layout(cr, layout);

  _actual_lines = pango_layout_get_line_count(layout);

  // clean up
  pango_font_description_free(desc);
  g_object_unref(layout);
  g_free(fontName);
}

void StaticCairoText::UpdateTexture()
{
  int width = 0;
  int height = 0;
  GetTextExtents(width, height);
  SetBaseSize(width, height);

  _cairoGraphics = new CairoGraphics(CAIRO_FORMAT_ARGB32,
                                     GetBaseWidth(),
                                     GetBaseHeight());
  cairo_t* cr = cairo_reference(_cairoGraphics->GetContext());

  DrawText(cr, GetBaseWidth(), GetBaseHeight(), _textColor);

  cairo_destroy(cr);

  // NTexture2D is the high level representation of an image that is backed by
  // an actual opengl texture.

  if (_texture2D)
  {
    _texture2D->UnReference();
  }

  _texture2D = texture_from_cairo_graphics(*_cairoGraphics);

  cairo_destroy(cr);

  delete _cairoGraphics;
}

void StaticCairoText::OnFontChanged(GObject* gobject, GParamSpec* pspec,
                                    gpointer data)
{
  StaticCairoText* self = (StaticCairoText*) data;
  self->_need_new_extent_cache = true;
  int width = 0;
  int height = 0;
  self->GetTextExtents(width, height);
  self->UpdateTexture();
  self->sigFontChanged.emit(self);
}

//
// Key navigation
//

void
StaticCairoText::SetAcceptKeyNavFocus(bool accept)
{
  _accept_key_nav_focus = accept;
}

bool
StaticCairoText::AcceptKeyNavFocus()
{
  return _accept_key_nav_focus;
}

}
