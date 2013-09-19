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
 *              Nick Dedekind <nick.dedekind@canonical.com>
 */

#include "StaticCairoText.h"

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <NuxCore/Size.h>

#include <Nux/TextureArea.h>
#include <NuxGraphics/CairoGraphics.h>

#include <pango/pango.h>
#include <pango/pangocairo.h>

#include <UnityCore/GLibWrapper.h>

#include "CairoTexture.h"

using namespace nux;

namespace unity
{
struct StaticCairoText::Impl
{
  Impl(StaticCairoText* parent, std::string const& text);
  ~Impl();

  PangoEllipsizeMode GetPangoEllipsizeMode() const;
  PangoAlignment GetPangoAlignment() const;

  std::string GetEffectiveFont() const;

  struct CacheTexture
  {
    typedef std::shared_ptr<CacheTexture> Ptr;
    CacheTexture()
    : start_index(0)
    , length((unsigned)std::string::npos)
    , height(0)
    {}

    unsigned start_index;
    unsigned length;
    unsigned height;

    std::shared_ptr<CairoGraphics> cr;
  };

  void UpdateBaseSize();
  Size GetTextExtents() const;

  void SetAttributes(PangoLayout* layout);
  void DrawText(CacheTexture::Ptr const& cached_texture);

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
  mutable std::list<CacheTexture::Ptr> cache_textures_;

  std::string text_;
  Color text_color_;

  EllipsizeState ellipsize_;
  AlignState align_;
  AlignState valign_;
  UnderlineState underline_;

  std::string font_;

  std::list<BaseTexturePtr> textures2D_;

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
  , valign_(NUX_ALIGN_CENTRE)
  , underline_(NUX_UNDERLINE_NONE)
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
  SetAcceptKeyNavFocusOnMouseDown(false);
}

StaticCairoText::StaticCairoText(std::string const& text, bool escape_text,
                                 NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , pimpl(new Impl(this, escape_text ? GetEscapedText(text) : text))
{
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
  Geometry const& geo = GetGeometry();

  pimpl->pre_layout_size_.width = geo.width;
  pimpl->pre_layout_size_.height = geo.height;
  pimpl->UpdateBaseSize();

  if (pimpl->textures2D_.empty())
    pimpl->UpdateTexture();

  View::PreLayoutManagement();
}

long StaticCairoText::PostLayoutManagement(long layoutResult)
{
  long result = 0;

  Geometry const& geo = GetGeometry();

  int old_width = pimpl->pre_layout_size_.width;
  if (old_width < geo.width)
    result |= SIZE_LARGER_WIDTH;
  else if (old_width > geo.width)
    result |= SIZE_SMALLER_WIDTH;
  else
    result |= SIZE_EQUAL_WIDTH;

  int old_height = pimpl->pre_layout_size_.height;
  if (old_height < geo.height)
    result |= SIZE_LARGER_HEIGHT;
  else if (old_height > geo.height)
    result |= SIZE_SMALLER_HEIGHT;
  else
    result |= SIZE_EQUAL_HEIGHT;

  return result;
}

void StaticCairoText::Draw(GraphicsEngine& gfxContext, bool forceDraw)
{
  Geometry const& base = GetGeometry();

  if (pimpl->textures2D_.empty() ||
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

  gfxContext.QRP_Color(base.x, base.y, base.width, base.height, color::Transparent);

  int current_x = base.x;
  int current_y = base.y;

  if (pimpl->align_ == NUX_ALIGN_CENTRE)
  {
    current_x += std::round((base.width - pimpl->cached_extent_.width) / 2.0f);
  }
  else if (pimpl->align_ == NUX_ALIGN_RIGHT)
  {
    current_x += base.width - pimpl->cached_extent_.width;
  }

  if (pimpl->valign_ == NUX_ALIGN_CENTRE)
  {
    current_y += std::round((base.height - pimpl->cached_extent_.height) / 2.0f);
  }
  else if (pimpl->valign_ == NUX_ALIGN_BOTTOM)
  {
    current_y += base.height - pimpl->cached_extent_.height;
  }

  for (BaseTexturePtr tex : pimpl->textures2D_)
  {
    nux::ObjectPtr<nux::IOpenGLBaseTexture> text_tex = tex->GetDeviceTexture();
    if (!text_tex)
      break;

    gfxContext.QRP_1Tex(current_x,
                    current_y,
                    text_tex->GetWidth(),
                    text_tex->GetHeight(),
                    text_tex,
                    texxform,
                    pimpl->text_color_);

    current_y += text_tex->GetHeight();
  }

  gfxContext.GetRenderStates().SetBlend(alpha, src, dest);

  gfxContext.PopClippingRectangle();
}

void StaticCairoText::DrawContent(GraphicsEngine& gfxContext, bool forceDraw)
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
    QueueDraw();
    sigTextChanged.emit(this);
  }
}

void StaticCairoText::SetTextAlpha(unsigned int alpha)
{
  if (pimpl->text_color_.alpha != alpha)
  {
    pimpl->text_color_.alpha = alpha;
    pimpl->UpdateTexture();
    QueueDraw();
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

std::string StaticCairoText::GetFont()
{
  return pimpl->font_;
}

void StaticCairoText::SetUnderline(UnderlineState underline)
{
  if (pimpl->underline_ != underline)
  {
    pimpl->underline_ = underline;
    pimpl->need_new_extent_cache_ = true;
    Size s = GetTextExtents();
    SetMinimumHeight(s.height);
    NeedRedraw();
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

std::vector<unsigned> StaticCairoText::GetTextureStartIndices()
{
  pimpl->GetTextExtents();

  std::vector<unsigned> list;
  auto iter = pimpl->cache_textures_.begin();
  for (; iter != pimpl->cache_textures_.end(); ++iter)
  {
    Impl::CacheTexture::Ptr const& cached_texture = *iter;
    list.push_back(cached_texture->start_index);
  }
  return list;
}

std::vector<unsigned> StaticCairoText::GetTextureEndIndices()
{
  pimpl->GetTextExtents();

  std::vector<unsigned> list;
  auto iter = pimpl->cache_textures_.begin();
  for (; iter != pimpl->cache_textures_.end(); ++iter)
  {
    Impl::CacheTexture::Ptr const& cached_texture = *iter;
    if (cached_texture->length == (unsigned)std::string::npos)
    {
      list.push_back((unsigned)std::string::npos);
    }
    else
    {
      if (cached_texture->start_index > 0 || cached_texture->length > 0)
        list.push_back((unsigned)(cached_texture->start_index + cached_texture->length - 1));
      else
        list.push_back(0);
    }
  }
  return list;
}

std::string StaticCairoText::GetName() const
{
  return "StaticCairoText";
}

void StaticCairoText::AddProperties(debug::IntrospectionData& introspection)
{
  introspection.add(GetAbsoluteGeometry())
               .add("text", pimpl->text_);
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
  int                   dpi      = 0;
  GdkScreen*            screen   = gdk_screen_get_default();    // is not ref'ed
  GtkSettings*          settings = gtk_settings_get_default();  // is not ref'ed

  if (!need_new_extent_cache_)
  {
    return cached_extent_;
  }

  Size result;
  std::string font = GetEffectiveFont();
  nux::Size layout_size(-1, lines_ < 0 ? lines_ : std::numeric_limits<int>::min());

  surface = cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1);
  cr = cairo_create(surface);
  cairo_set_font_options(cr, gdk_screen_get_font_options(screen));

  layout = pango_cairo_create_layout(cr);
  desc = pango_font_description_from_string(font.c_str());
  pango_layout_set_font_description(layout, desc);
  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_ellipsize(layout, GetPangoEllipsizeMode());
  pango_layout_set_alignment(layout, GetPangoAlignment());
  pango_layout_set_width(layout, layout_size.width);
  pango_layout_set_height(layout, layout_size.height);
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
  pango_layout_get_pixel_size(layout, &result.width, &result.height);

  if (result.width > parent_->GetMaximumWidth())
  {
    pango_layout_set_width(layout, parent_->GetMaximumWidth() * PANGO_SCALE);
    pango_layout_context_changed(layout);
    pango_layout_get_pixel_size(layout, &result.width, &result.height);
  }

  cached_extent_ = result;
  baseline_ = pango_layout_get_baseline(layout) / PANGO_SCALE;
  need_new_extent_cache_ = false;

  cache_textures_.clear();
  PangoLayoutIter* iter = pango_layout_get_iter(layout);
  CacheTexture::Ptr current_tex(new CacheTexture());
  const int max_height = GetGraphicsDisplay()->GetGpuDevice()->GetGpuInfo().GetMaxTextureSize();
  if (max_height < 0)
    return nux::Size(0, 0);

  do
  {
    PangoLayoutLine* line = pango_layout_iter_get_line_readonly(iter);
    int y0 = 0, y1 = 0;
    pango_layout_iter_get_line_yrange(iter, &y0, &y1);
    y0 /= PANGO_SCALE;
    y1 /= PANGO_SCALE;

    if (line->start_index < 0 || y1 < y0)
    {
      current_tex.reset();
      break;      
    }
    unsigned line_start_index = line->start_index;
    unsigned height = y1-y0;

    if (current_tex->height + height > (unsigned)max_height)
    {
      if (line_start_index > current_tex->start_index)
        current_tex->length = line_start_index - current_tex->start_index;
      else
        current_tex->length = 0;
      cache_textures_.push_back(current_tex);

      // new texture.
      current_tex.reset(new CacheTexture());
      current_tex->start_index = line_start_index;
      current_tex->height = 0;
    }
    current_tex->height += height;
  }
  while(pango_layout_iter_next_line(iter));
  
  if (current_tex) { cache_textures_.push_back(current_tex); }

  pango_layout_iter_free(iter);

  // clean up
  pango_font_description_free(desc);
  g_object_unref(layout);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
  return result;
}

void StaticCairoText::Impl::SetAttributes(PangoLayout *layout)
{
  PangoAttrList* attr_list  = NULL;
  PangoAttribute* underline_attr = NULL;

  attr_list = pango_layout_get_attributes(layout);
  if(!attr_list)
  {
    attr_list = pango_attr_list_new();
  }

  PangoUnderline underline_type;

  switch(underline_){
    case(NUX_UNDERLINE_SINGLE):
      underline_type = PANGO_UNDERLINE_SINGLE;
      break;
    case(NUX_UNDERLINE_DOUBLE):
      underline_type = PANGO_UNDERLINE_DOUBLE;
      break;
    case(NUX_UNDERLINE_LOW):
      underline_type = PANGO_UNDERLINE_LOW;
      break;
    default:
      underline_type = PANGO_UNDERLINE_NONE;
  }
  underline_attr = pango_attr_underline_new(underline_type);
  pango_attr_list_insert(attr_list, underline_attr);
  pango_layout_set_attributes(layout, attr_list);
}

void StaticCairoText::Impl::DrawText(CacheTexture::Ptr const& texture)
{
  if (!texture)
    return;

  nux::Size layout_size(-1, lines_ < 0 ? lines_ : std::numeric_limits<int>::min());
  texture->cr.reset(new CairoGraphics(CAIRO_FORMAT_ARGB32, cached_extent_.width, cached_extent_.height));
  cairo_t* cr = texture->cr->GetInternalContext();

  PangoLayout*          layout     = NULL;
  PangoFontDescription* desc       = NULL;
  PangoContext*         pangoCtx   = NULL;
  int                   dpi        = 0;
  GdkScreen*            screen     = gdk_screen_get_default();    // not ref'ed
  GtkSettings*          settings   = gtk_settings_get_default();  // not ref'ed

  std::string text = text_.substr(texture->start_index, texture->length);

  std::string font(GetEffectiveFont());
  cairo_set_font_options(cr, gdk_screen_get_font_options(screen));

  layout = pango_cairo_create_layout(cr);


  desc = pango_font_description_from_string(font.c_str());
  pango_layout_set_font_description(layout, desc);
  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_ellipsize(layout, GetPangoEllipsizeMode());
  pango_layout_set_alignment(layout, GetPangoAlignment());
  pango_layout_set_markup(layout, text.c_str(), -1);
  pango_layout_set_width(layout, layout_size.width);
  pango_layout_set_height(layout, layout_size.height);
  pango_layout_set_spacing(layout, line_spacing_ * PANGO_SCALE);

  SetAttributes(layout);

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

  Size result;
  pango_layout_context_changed(layout);
  pango_layout_get_pixel_size(layout, &result.width, &result.height);

  if (result.width > parent_->GetMaximumWidth())
  {
    pango_layout_set_width(layout, parent_->GetMaximumWidth() * PANGO_SCALE);
    pango_layout_context_changed(layout);
  }

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(cr, text_color_.red, text_color_.green, text_color_.blue, text_color_.alpha);

  cairo_move_to(cr, 0.0f, 0.0f);
  pango_cairo_show_layout(cr, layout);

  actual_lines_ = pango_layout_get_line_count(layout);

  // clean up
  pango_font_description_free(desc);
  g_object_unref(layout);
}

void StaticCairoText::Impl::UpdateBaseSize()
{
  parent_->SetBaseSize(cached_extent_.width, cached_extent_.height);
}

void StaticCairoText::Impl::UpdateTexture()
{
  GetTextExtents();
  UpdateBaseSize();

  textures2D_.clear();
  for (auto const& texture : cache_textures_)
  {
    DrawText(texture);

    textures2D_.push_back(texture_ptr_from_cairo_graphics(*texture->cr));
  }
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
