// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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
 *              Mirco Müller <mirco.mueller@canonical.com>
 *              Tim Penhey <tim.penhey@canonical.com>
 */

#include "StaticCairoText.h"

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <NuxCore/Size.h>

#include <Nux/TextureArea.h>
#include <NuxGraphics/CairoGraphics.h>

#include <pango/pango.h>
#include <pango/pangocairo.h>

#if defined(NUX_OS_LINUX)
#include <X11/Xlib.h>
#endif

#include <UnityCore/GLibWrapper.h>

#include "CairoTexture.h"

using namespace unity;

// TODO: Tim Penhey 2011-05-16
// We shouldn't be pushing stuff into the nux namespace from the unity
// codebase, that is just rude.
namespace nux
{
struct StaticCairoText::Impl
{
  Impl(StaticCairoText* parent, std::string const& text);
  ~Impl();

  PangoEllipsizeMode GetPangoEllipsizeMode() const;
  PangoAlignment GetPangoAlignment() const;

  std::string GetEffectiveFont() const;
  Size GetTextExtents() const;

  void DrawText(cairo_t* cr, int width, int height, int line_spacing, Color const& color);

  void UpdateTexture();
  void OnFontChanged();

  static void FontChanged(GObject* gobject, GParamSpec* pspec, gpointer data);

  StaticCairoText* parent_;
  bool accept_key_nav_focus_;
  mutable bool need_new_extent_cache_;
  // The three following are all set in get text extents.
  mutable Size cached_extent_;
  mutable Size cached_base_;
  mutable int baseline_;

  std::string text_;
  Color text_color_;

  EllipsizeState ellipsize_;
  AlignState align_;
  AlignState valign_;

  std::string font_;

  BaseTexturePtr texture2D_;

  Size pre_layout_size_;

  int lines_;
  int actual_lines_;
  float line_spacing_;
};

StaticCairoText::Impl::Impl(StaticCairoText* parent, std::string const& text)
  : parent_(parent)
  , accept_key_nav_focus_(false)
  , need_new_extent_cache_(true)
  , baseline_(0)
  , text_(text)
  , text_color_(color::White)
  , ellipsize_(NUX_ELLIPSIZE_END)
  , align_(NUX_ALIGN_LEFT)
  , valign_(NUX_ALIGN_TOP)
  , lines_(-2)  // should find out why -2...
    // the desired height of the layout in Pango units if positive, or desired
    // number of lines if negative.
  , actual_lines_(0)
  , line_spacing_(0.5)
{
  GtkSettings* settings = gtk_settings_get_default();  // not ref'ed
  g_signal_connect(settings, "notify::gtk-font-name",
                   (GCallback)FontChanged, this);
  g_signal_connect(settings, "notify::gtk-xft-dpi",
                   (GCallback)FontChanged, this);
}

StaticCairoText::Impl::~Impl()
{
  GtkSettings* settings = gtk_settings_get_default();  // not ref'ed
  g_signal_handlers_disconnect_by_func(settings,
                                       (void*)FontChanged,
                                       this);
}

PangoEllipsizeMode StaticCairoText::Impl::GetPangoEllipsizeMode() const
{
  switch (ellipsize_)
  {
  case NUX_ELLIPSIZE_START:
    return PANGO_ELLIPSIZE_START;
  case NUX_ELLIPSIZE_MIDDLE:
    return PANGO_ELLIPSIZE_MIDDLE;
  case NUX_ELLIPSIZE_END:
    return PANGO_ELLIPSIZE_END;
  default:
    return PANGO_ELLIPSIZE_NONE;
  }
}

PangoAlignment StaticCairoText::Impl::GetPangoAlignment() const
{
  switch (align_)
  {
  case NUX_ALIGN_LEFT:
    return PANGO_ALIGN_LEFT;
  case NUX_ALIGN_CENTRE:
    return PANGO_ALIGN_CENTER;
  default:
    return PANGO_ALIGN_RIGHT;
  }
}


  NUX_IMPLEMENT_OBJECT_TYPE (StaticCairoText);

StaticCairoText::StaticCairoText(std::string const& text,
                                 NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , pimpl(new Impl(this, text))
{
  SetMinimumSize(1, 1);
  SetAcceptKeyNavFocusOnMouseDown(false);
}

StaticCairoText::StaticCairoText(std::string const& text, bool escape_text,
                                 NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , pimpl(new Impl(this, escape_text ? GetEscapedText(text) : text))
{
  SetMinimumSize(1, 1);
  SetAcceptKeyNavFocusOnMouseDown(false);
}


StaticCairoText::~StaticCairoText()
{
  delete pimpl;
}

void StaticCairoText::SetTextEllipsize(EllipsizeState state)
{
  pimpl->ellipsize_ = state;
  NeedRedraw();
}

void StaticCairoText::SetTextAlignment(AlignState state)
{
  pimpl->align_ = state;
  NeedRedraw();
}

void StaticCairoText::SetTextVerticalAlignment(AlignState state)
{
  pimpl->valign_ = state;
  QueueDraw();
}

void StaticCairoText::SetLines(int lines)
{
  pimpl->lines_ = lines;
  pimpl->UpdateTexture();
  QueueDraw();
}

void StaticCairoText::SetLineSpacing(float line_spacing)
{
  pimpl->line_spacing_ = line_spacing;
  pimpl->UpdateTexture();
  QueueDraw();
}

void StaticCairoText::PreLayoutManagement()
{
  Geometry geo = GetGeometry();
  pimpl->pre_layout_size_.width = geo.width;
  pimpl->pre_layout_size_.height = geo.height;

  SetBaseSize(pimpl->cached_extent_.width,
              pimpl->cached_extent_.height);

  if (pimpl->texture2D_.IsNull())
  {
    pimpl->UpdateTexture();
  }

  View::PreLayoutManagement();
}

long StaticCairoText::PostLayoutManagement(long layoutResult)
{
  long result = 0;

  Geometry const& geo = GetGeometry();

  int old_width = pimpl->pre_layout_size_.width;
  if (old_width < geo.width)
    result |= eLargerWidth;
  else if (old_width > geo.width)
    result |= eSmallerWidth;
  else
    result |= eCompliantWidth;

  int old_height = pimpl->pre_layout_size_.height;
  if (old_height < geo.height)
    result |= eLargerHeight;
  else if (old_height > geo.height)
    result |= eSmallerHeight;
  else
    result |= eCompliantHeight;

  return result;
}

void StaticCairoText::Draw(GraphicsEngine& gfxContext, bool forceDraw)
{
  Geometry const& base = GetGeometry();

  if (pimpl->texture2D_.IsNull() ||
      pimpl->cached_base_.width != base.width ||
      pimpl->cached_base_.height != base.height)
  {
    pimpl->cached_base_.width = base.width;
    pimpl->cached_base_.height = base.height;
    pimpl->UpdateTexture();
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
                      base.y + ((base.height - pimpl->cached_extent_.height) / 2),
                      base.width,
                      base.height,
                      pimpl->texture2D_->GetDeviceTexture(),
                      texxform,
                      pimpl->text_color_);

  gfxContext.GetRenderStates().SetBlend(alpha, src, dest);

  gfxContext.PopClippingRectangle();
}

void StaticCairoText::DrawContent(GraphicsEngine& gfxContext, bool forceDraw)
{
  // intentionally left empty
}

void StaticCairoText::PostDraw(GraphicsEngine& gfxContext, bool forceDraw)
{
  // intentionally left empty
}

void StaticCairoText::SetText(std::string const& text, bool escape_text)
{
  std::string tmp_text = escape_text ? GetEscapedText(text) : text;

  if (pimpl->text_ != tmp_text)
  {
    pimpl->text_ = tmp_text;
    pimpl->need_new_extent_cache_ = true;
    pimpl->UpdateTexture();
    sigTextChanged.emit(this);
  }
}

void StaticCairoText::SetMaximumSize(int w, int h)
{
  if (w != GetMaximumWidth())
  {
    pimpl->need_new_extent_cache_ = true;
    View::SetMaximumSize(w, h);
    pimpl->UpdateTexture();
    return;
  } 

  View::SetMaximumSize(w, h);
}

void StaticCairoText::SetMaximumWidth(int w)
{
  if (w != GetMaximumWidth())
  {
    pimpl->need_new_extent_cache_ = true;
    View::SetMaximumWidth(w);
    pimpl->UpdateTexture();
  }
}

std::string StaticCairoText::GetText() const
{
  return pimpl->text_;
}

std::string StaticCairoText::GetEscapedText(std::string const& text)
{
  return glib::String(g_markup_escape_text(text.c_str(), -1)).Str();
}

Color StaticCairoText::GetTextColor() const
{
  return pimpl->text_color_;
}

void StaticCairoText::SetTextColor(Color const& textColor)
{
  if (pimpl->text_color_ != textColor)
  {
    pimpl->text_color_ = textColor;
    pimpl->UpdateTexture();
    QueueDraw();

    sigTextColorChanged.emit(this);
  }
}

void StaticCairoText::SetFont(std::string const& font)
{
  if (pimpl->font_ != font)
  {  
    pimpl->font_ = font;
    pimpl->need_new_extent_cache_ = true;
    Size s = GetTextExtents();
    SetMinimumHeight(s.height);
    NeedRedraw();
    sigFontChanged.emit(this);
  }
}

int StaticCairoText::GetLineCount() const
{
  return pimpl->actual_lines_;
}

int StaticCairoText::GetBaseline() const
{
  return pimpl->baseline_;
}

Size StaticCairoText::GetTextExtents() const
{
  return pimpl->GetTextExtents();
}

void StaticCairoText::GetTextExtents(int& width, int& height) const
{
  Size s = pimpl->GetTextExtents();
  width = s.width;
  height = s.height;
}

std::string StaticCairoText::Impl::GetEffectiveFont() const
{
  if (font_.empty())
  {
    GtkSettings* settings = gtk_settings_get_default();  // not ref'ed
    glib::String font_name;
    g_object_get(settings, "gtk-font-name", &font_name, NULL);
    return font_name.Str();
  }

  return font_;
}

Size StaticCairoText::Impl::GetTextExtents() const
{
  cairo_surface_t*      surface  = NULL;
  cairo_t*              cr       = NULL;
  PangoLayout*          layout   = NULL;
  PangoFontDescription* desc     = NULL;
  PangoContext*         pangoCtx = NULL;
  PangoRectangle        inkRect  = {0, 0, 0, 0};
  PangoRectangle        logRect  = {0, 0, 0, 0};
  int                   dpi      = 0;
  GdkScreen*            screen   = gdk_screen_get_default();    // is not ref'ed
  GtkSettings*          settings = gtk_settings_get_default();  // is not ref'ed

  if (!need_new_extent_cache_)
  {
    return cached_extent_;
  }

  Size result;
  std::string font = GetEffectiveFont();

  int maxwidth = parent_->GetMaximumWidth();

  surface = cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1);
  cr = cairo_create(surface);
  cairo_set_font_options(cr, gdk_screen_get_font_options(screen));

  layout = pango_cairo_create_layout(cr);
  desc = pango_font_description_from_string(font.c_str());
  pango_layout_set_font_description(layout, desc);
  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_ellipsize(layout, GetPangoEllipsizeMode());
  pango_layout_set_alignment(layout, GetPangoAlignment());
  pango_layout_set_height(layout, lines_);
  pango_layout_set_width(layout, maxwidth * PANGO_SCALE);
  pango_layout_set_markup(layout, text_.c_str(), -1);
  pango_layout_set_spacing(layout, line_spacing_ * PANGO_SCALE);

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
  pango_layout_get_extents(layout, &inkRect, &logRect);

  // logRect has some issues using italic style
  if (inkRect.x + inkRect.width > logRect.x + logRect.width)
    result.width = std::ceil(static_cast<float>(inkRect.x + inkRect.width - logRect.x) / PANGO_SCALE);
  else
    result.width  = std::ceil(static_cast<float>(logRect.width) / PANGO_SCALE);

  result.height = std::ceil(static_cast<float>(logRect.height) / PANGO_SCALE);
  cached_extent_ = result;
  baseline_ = pango_layout_get_baseline(layout) / PANGO_SCALE;
  need_new_extent_cache_ = false;

  // clean up
  pango_font_description_free(desc);
  g_object_unref(layout);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
  return result;
}

void StaticCairoText::Impl::DrawText(cairo_t* cr,
                                     int width,
                                     int height,
                                     int line_spacing,
                                     Color const& color)
{
  PangoLayout*          layout     = NULL;
  PangoFontDescription* desc       = NULL;
  PangoContext*         pangoCtx   = NULL;
  int                   dpi        = 0;
  GdkScreen*            screen     = gdk_screen_get_default();    // not ref'ed
  GtkSettings*          settings   = gtk_settings_get_default();  // not ref'ed

  std::string font(GetEffectiveFont());

  cairo_set_font_options(cr, gdk_screen_get_font_options(screen));
  layout = pango_cairo_create_layout(cr);
  desc = pango_font_description_from_string(font.c_str());

  pango_layout_set_font_description(layout, desc);
  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_ellipsize(layout, GetPangoEllipsizeMode());
  pango_layout_set_alignment(layout, GetPangoAlignment());
  pango_layout_set_markup(layout, text_.c_str(), -1);
  pango_layout_set_width(layout, width * PANGO_SCALE);
  pango_layout_set_height(layout, height * PANGO_SCALE);
  pango_layout_set_spacing(layout, line_spacing * PANGO_SCALE);

  pango_layout_set_height(layout, lines_);
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

  actual_lines_ = pango_layout_get_line_count(layout);

  // clean up
  pango_font_description_free(desc);
  g_object_unref(layout);
}

void StaticCairoText::Impl::UpdateTexture()
{
  Size size = GetTextExtents();
  parent_->SetBaseSize(size.width, size.height);
  // Now reget the internal geometry as it is clipped by the max size.
  Geometry const& geo = parent_->GetGeometry();

  CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32,
                               geo.width, geo.height);

  DrawText(cairo_graphics.GetInternalContext(),
           geo.width, geo.height, line_spacing_, text_color_);

  texture2D_ = texture_ptr_from_cairo_graphics(cairo_graphics);
}

void StaticCairoText::Impl::FontChanged(GObject* gobject,
                                        GParamSpec* pspec,
                                        gpointer data)
{
  StaticCairoText::Impl* self = static_cast<StaticCairoText::Impl*>(data);
  self->OnFontChanged();
}

void StaticCairoText::Impl::OnFontChanged()
{
  need_new_extent_cache_ = true;
  UpdateTexture();
  parent_->sigFontChanged.emit(parent_);
}

//
// Key navigation
//

void StaticCairoText::SetAcceptKeyNavFocus(bool accept)
{
  pimpl->accept_key_nav_focus_ = accept;
}

bool StaticCairoText::AcceptKeyNavFocus()
{
  return pimpl->accept_key_nav_focus_;
}

}
