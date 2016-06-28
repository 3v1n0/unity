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
 * Authored by: Mirco Müller <mirco.mueller@canonical.com
 *              Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "DashStyle.h"

#include <string>
#include <vector>
#include <map>
#include <cmath>

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <pango/pango.h>

#include <NuxCore/Color.h>
#include <NuxCore/Logger.h>
#include <NuxGraphics/ImageSurface.h>
#include <NuxGraphics/CairoGraphics.h>
#include <Nux/PaintLayer.h>
#include <UnityCore/GLibWrapper.h>

#include "CairoTexture.h"
#include "JSONParser.h"
#include "TextureCache.h"
#include "ThemeSettings.h"
#include "UnitySettings.h"
#include "config.h"

#define DASH_WIDGETS_FILE UNITYDATADIR"/themes/dash-widgets.json"

namespace unity
{
namespace dash
{
DECLARE_LOGGER(logger, "unity.dash.style");
namespace
{
Style* style_instance = nullptr;

const int STATES = 5;

// These cairo overrides may also be reused somewhere...
void cairo_set_source_rgba(cairo_t* cr, nux::Color const& color)
{
  ::cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
}

inline double _align(double val, bool odd=true)
{
  double fract = val - (int) val;

  if (odd)
  {
    // for strokes with an odd line-width
    if (fract != 0.5f)
      return (double) ((int) val + 0.5f);
    else
      return val;
  }
  else
  {
    // for strokes with an even line-width
    if (fract != 0.0f)
      return (double) ((int) val);
    else
      return val;
  }
}

template <typename T>
inline void get_actual_cairo_size(cairo_t* cr, T* width, T* height)
{
  double w_scale, h_scale;
  auto* surface = cairo_get_target(cr);
  cairo_surface_get_device_scale(surface, &w_scale, &h_scale);
  *width = cairo_image_surface_get_width(surface) / w_scale;
  *height = cairo_image_surface_get_height(surface) / h_scale;
}

class LazyLoadTexture : public sigc::trackable
{
public:
  LazyLoadTexture(std::string const& filename, int size = -1);
  BaseTexturePtr const& texture();
private:
  void LoadTexture();
  void UnloadTexture();
private:
  std::string filename_;
  int size_;
  BaseTexturePtr texture_;
};

} // anon namespace


struct Style::Impl : sigc::trackable
{
  Impl(Style* owner);
  ~Impl();

  void Blur(cairo_t* cr, int size);

  void LoadStyleFile();
  void SetDefaultValues();

  void GetTextExtents(int& width,
                      int& height,
                      int  maxWidth,
                      int  maxHeight,
                      std::string const& text);

  void Text(cairo_t* cr,
            nux::Color const& color,
            std::string const& label,
            int font_size,
            double horizMargin = 4.0,
            Alignment alignment = Alignment::CENTER);

  void ButtonOutlinePath(cairo_t* cr, bool align);

  void ButtonOutlinePathSegment(cairo_t* cr, Segment segment);

  void ArrowPath(cairo_t* cr, Arrow arrow);

  cairo_operator_t SetBlendMode(cairo_t* cr, BlendMode mode);

  void DrawOverlay(cairo_t*  cr,
                   double    opacity,
                   BlendMode mode,
                   int       blurSize);

  void RoundedRectSegment(cairo_t*   cr,
                          double     aspect,
                          double     x,
                          double     y,
                          double     cornerRadius,
                          double     width,
                          double     height,
                          Segment    segment);

  void RoundedRectSegmentBorder(cairo_t*   cr,
                          double     aspect,
                          double     x,
                          double     y,
                          double     cornerRadius,
                          double     width,
                          double     height,
                          Segment    segment,
                          Arrow      arrow,
                          nux::ButtonVisualState state);

  void Refresh();
  void UpdateFormFactor(FormFactor);

  BaseTexturePtr LoadScaledTexture(std::string const& name, double scale)
  {
    int w, h;
    auto const& path = theme::Settings::Get()->ThemedFilePath(name, {PKGDATADIR});
    gdk_pixbuf_get_file_info(path.c_str(), &w, &h);
    return TextureCache::GetDefault().FindTexture(name, RawPixel(w).CP(scale), RawPixel(h).CP(scale));
  }

  // Members
  Style* owner_;

  cairo_font_options_t* default_font_options_;

  std::vector<nux::Color> button_label_border_color_;
  std::vector<double>     button_label_border_size_;
  double                  button_label_border_radius_;
  double                  button_label_text_size_;

  std::vector<nux::Color> button_label_text_color_;
  std::vector<nux::Color> button_label_fill_color_;

  std::vector<double>    button_label_overlay_opacity_;
  std::vector<BlendMode> button_label_overlay_mode_;
  std::vector<int>       button_label_blur_size_;

  nux::Color            regular_text_color_;
  double                regular_text_size_;
  BlendMode             regular_text_mode_;
  FontWeight            regular_text_weight_;

  double                separator_size_;
  nux::Color            separator_color_;
  double                separator_overlay_opacity_;
  BlendMode             separator_overlay_mode_;
  int                   separator_blur_size_;

  int                   scrollbar_size_;
  int                   scrollbar_overlay_size_;
  int                   scrollbar_buttons_size_;
  nux::Color            scrollbar_color_;
  nux::Color            scrollbar_overlay_color_;
  nux::Color            scrollbar_track_color_;
  double                scrollbar_corner_radius_;
  double                scrollbar_overlay_corner_radius_;

  nux::Color text_color_;

  int text_width_;
  int text_height_;

  LazyLoadTexture category_texture_;
  LazyLoadTexture category_texture_no_filters_;

  LazyLoadTexture dash_shine_;

  LazyLoadTexture information_texture_;

  LazyLoadTexture refine_gradient_corner_;
  LazyLoadTexture refine_gradient_dash_;

  LazyLoadTexture group_unexpand_texture_;
  LazyLoadTexture group_expand_texture_;

  LazyLoadTexture star_deselected_texture_;
  LazyLoadTexture star_selected_texture_;
  LazyLoadTexture star_highlight_texture_;
};

Style::Impl::Impl(Style* owner)
  : owner_(owner)
  , button_label_border_color_(STATES)
  , button_label_border_size_(STATES)
  , button_label_text_color_(STATES)
  , button_label_fill_color_(STATES)
  , button_label_overlay_opacity_(STATES)
  , button_label_overlay_mode_(STATES)
  , button_label_blur_size_(STATES)
  , text_color_(nux::color::White)
  , text_width_(0)
  , text_height_(0)
  , category_texture_("category_gradient")
  , category_texture_no_filters_("category_gradient_no_refine")
  , dash_shine_("dash_sheen")
  , information_texture_("information_icon")
  , refine_gradient_corner_("refine_gradient_corner")
  , refine_gradient_dash_("refine_gradient_dash")
  , group_unexpand_texture_("dash_group_unexpand")
  , group_expand_texture_("dash_group_expand")
  , star_deselected_texture_("star_deselected")
  , star_selected_texture_("star_selected")
  , star_highlight_texture_("star_highlight")
{
  auto refresh_cb = sigc::hide(sigc::mem_fun(this, &Impl::Refresh));

  auto theme_settings = theme::Settings::Get();
  theme_settings->font.changed.connect(refresh_cb);
  theme_settings->theme.changed.connect(sigc::hide(sigc::mem_fun(this, &Impl::LoadStyleFile)));

  auto& settings = Settings::Instance();
  settings.font_scaling.changed.connect(refresh_cb);
  settings.form_factor.changed.connect(sigc::mem_fun(this, &Impl::UpdateFormFactor));

  TextureCache::GetDefault().themed_invalidated.connect(sigc::mem_fun(&owner_->textures_changed, &decltype(owner_->textures_changed)::emit));

  Refresh();
  UpdateFormFactor(settings.form_factor());

  // create fallback font-options
  default_font_options_ = cairo_font_options_create();
  if (cairo_font_options_status(default_font_options_) == CAIRO_STATUS_SUCCESS)
  {
    cairo_font_options_set_antialias(default_font_options_,
                                     CAIRO_ANTIALIAS_GRAY);
    cairo_font_options_set_subpixel_order(default_font_options_,
                                          CAIRO_SUBPIXEL_ORDER_RGB);
    cairo_font_options_set_hint_style(default_font_options_,
                                      CAIRO_HINT_STYLE_SLIGHT);
    cairo_font_options_set_hint_metrics(default_font_options_,
                                        CAIRO_HINT_METRICS_ON);
  }

  LoadStyleFile();
}

void Style::Impl::LoadStyleFile()
{
  json::Parser parser;

  // Since the parser skips values if they are not found, make sure everything
  // is initialised.
  SetDefaultValues();

  if (!parser.Open(theme::Settings::Get()->ThemedFilePath("dash-widgets", {UNITYDATADIR"/themes"}, {"json"})))
  {
    LOG_ERROR(logger) << "Impossible to find a dash-widgets.json in theme paths";
    return;
  }

  // button-label
  parser.ReadColors("button-label", "border-color", "border-opacity",
                    button_label_border_color_);
  parser.ReadDoubles("button-label", "border-size", button_label_border_size_);
  parser.ReadDouble("button-label", "border-radius", button_label_border_radius_);
  parser.ReadDouble("button-label", "text-size", button_label_text_size_);
  parser.ReadColors("button-label", "text-color", "text-opacity",
                    button_label_text_color_);
  parser.ReadColors("button-label", "fill-color", "fill-opacity",
                    button_label_fill_color_);
  parser.ReadDoubles("button-label", "overlay-opacity", button_label_overlay_opacity_);

  std::map<std::string, BlendMode> blend_mode_map;
  blend_mode_map["normal"] = BlendMode::NORMAL;
  blend_mode_map["multiply"] = BlendMode::MULTIPLY;
  blend_mode_map["screen"] = BlendMode::SCREEN;

  parser.ReadMappedStrings("button-label", "overlay-mode", blend_mode_map,
                           button_label_overlay_mode_);
  parser.ReadInts("button-label", "blur-size", button_label_blur_size_);

  // regular-text
  parser.ReadColor("regular-text", "text-color", "text-opacity",
                   regular_text_color_);
  parser.ReadDouble("regular-text", "text-size", regular_text_size_);
  parser.ReadMappedString("regular-text", "text-mode", blend_mode_map,
                          regular_text_mode_);

  std::map<std::string, FontWeight> font_weight_map;
  font_weight_map["light"] = FontWeight::LIGHT;
  font_weight_map["regular"] = FontWeight::REGULAR;
  font_weight_map["bold"] = FontWeight::BOLD;

  parser.ReadMappedString("regular-text", "text-weight", font_weight_map,
                          regular_text_weight_);

  // separator
  parser.ReadDouble("separator", "size", separator_size_);
  parser.ReadColor("separator", "color", "opacity", separator_color_);
  parser.ReadDouble("separator", "overlay-opacity", separator_overlay_opacity_);
  parser.ReadMappedString("separator", "overlay-mode", blend_mode_map,
                          separator_overlay_mode_);
  parser.ReadInt("separator", "blur-size", separator_blur_size_);

  // scrollbar
  parser.ReadInt("scrollbar", "size", scrollbar_size_);
  parser.ReadInt("scrollbar-overlay", "size", scrollbar_overlay_size_);
  parser.ReadInt("scrollbar", "buttons-size", scrollbar_buttons_size_);
  parser.ReadColor("scrollbar", "color", "opacity", scrollbar_color_);
  parser.ReadColor("scrollbar-overlay", "color", "opacity", scrollbar_overlay_color_);
  parser.ReadColor("scrollbar-track", "color", "opacity", scrollbar_track_color_);
  parser.ReadDouble("scrollbar", "corner-radius", scrollbar_corner_radius_);
  parser.ReadDouble("scrollbar-overlay", "corner-radius", scrollbar_overlay_corner_radius_);

  owner_->changed.emit();
}

Style::Impl::~Impl()
{
  if (cairo_font_options_status(default_font_options_) == CAIRO_STATUS_SUCCESS)
    cairo_font_options_destroy(default_font_options_);
}

void Style::Impl::Refresh()
{
  const char* const SAMPLE_MAX_TEXT = "Chromium Web Browser";

  nux::CairoGraphics util_cg(CAIRO_FORMAT_ARGB32, 1, 1);
  cairo_t* cr = util_cg.GetInternalContext();

  auto const& font = theme::Settings::Get()->font();
  PangoFontDescription* desc = ::pango_font_description_from_string(font.c_str());
  ::pango_font_description_set_weight(desc, PANGO_WEIGHT_NORMAL);
  ::pango_font_description_set_size(desc, 9 * PANGO_SCALE);

  glib::Object<PangoLayout> layout(::pango_cairo_create_layout(cr));
  ::pango_layout_set_font_description(layout, desc);
  ::pango_layout_set_text(layout, SAMPLE_MAX_TEXT, -1);

  PangoContext* cxt = ::pango_layout_get_context(layout);

  GdkScreen* screen = ::gdk_screen_get_default();
  ::pango_cairo_context_set_font_options(cxt, ::gdk_screen_get_font_options(screen));
  ::pango_cairo_context_set_resolution(cxt, 96.0 * Settings::Instance().font_scaling());
  ::pango_layout_context_changed(layout);

  PangoRectangle log_rect;
  ::pango_layout_get_pixel_extents(layout, NULL, &log_rect);
  text_width_ = log_rect.width;
  text_height_ = log_rect.height;

  owner_->changed.emit();

  pango_font_description_free(desc);
}

void Style::Impl::UpdateFormFactor(FormFactor form_factor)
{
  owner_->always_maximised = (form_factor == FormFactor::NETBOOK || form_factor == FormFactor::TV);
}

Style::Style()
  : columns_number(6)
  , always_maximised(false)
  , pimpl(new Impl(this))
{
  if (style_instance)
  {
    LOG_ERROR(logger) << "More than one dash::Style created.";
  }
  else
  {
    style_instance = this;
  }
}

Style::~Style ()
{
  delete pimpl;
  if (style_instance == this)
    style_instance = nullptr;
}

Style& Style::Instance()
{
  if (!style_instance)
  {
    LOG_ERROR(logger) << "No dash::Style created yet.";
  }

  return *style_instance;
}

void Style::RoundedRect(cairo_t* cr,
                            double   aspect,
                            double   x,
                            double   y,
                            double   cornerRadius,
                            double   width,
                            double   height)
{
  // sanity check
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS &&
      cairo_surface_get_type(cairo_get_target(cr)) != CAIRO_SURFACE_TYPE_IMAGE)
    return;

  bool odd = cairo_get_line_width (cr) == 2.0 ? false : true;

  double radius = cornerRadius / aspect;

  // top-left, right of the corner
  cairo_move_to(cr, _align (x + radius, odd), _align (y, odd));

  // top-right, left of the corner
  cairo_line_to(cr, _align(x + width - radius, odd), _align(y, odd));

  // top-right, below the corner
  cairo_arc(cr,
            _align(x + width - radius, odd),
            _align(y + radius, odd),
            radius,
            -90.0f * G_PI / 180.0f,
            0.0f * G_PI / 180.0f);

  // bottom-right, above the corner
  cairo_line_to(cr, _align(x + width, odd), _align(y + height - radius, odd));

  // bottom-right, left of the corner
  cairo_arc(cr,
            _align(x + width - radius, odd),
            _align(y + height - radius, odd),
            radius,
            0.0f * G_PI / 180.0f,
            90.0f * G_PI / 180.0f);

  // bottom-left, right of the corner
  cairo_line_to(cr, _align(x + radius, odd), _align(y + height, odd));

  // bottom-left, above the corner
  cairo_arc(cr,
            _align(x + radius, odd),
            _align(y + height - radius, odd),
            radius,
            90.0f * G_PI / 180.0f,
            180.0f * G_PI / 180.0f);

  // top-left, right of the corner
  cairo_arc(cr,
            _align(x + radius, odd),
            _align(y + radius, odd),
            radius,
            180.0f * G_PI / 180.0f,
            270.0f * G_PI / 180.0f);
}

static inline void _blurinner(guchar* pixel,
                              gint*   zR,
                              gint*   zG,
                              gint*   zB,
                              gint*   zA,
                              gint    alpha,
                              gint    aprec,
                              gint    zprec)
{
  gint   r;
  gint   g;
  gint   b;
  guchar a;

  r = *pixel;
  g = *(pixel + 1);
  b = *(pixel + 2);
  a = *(pixel + 3);

  *zR += (alpha * ((r << zprec) - *zR)) >> aprec;
  *zG += (alpha * ((g << zprec) - *zG)) >> aprec;
  *zB += (alpha * ((b << zprec) - *zB)) >> aprec;
  *zA += (alpha * ((a << zprec) - *zA)) >> aprec;

  *pixel       = *zR >> zprec;
  *(pixel + 1) = *zG >> zprec;
  *(pixel + 2) = *zB >> zprec;
  *(pixel + 3) = *zA >> zprec;
}

static inline void _blurrow(guchar* pixels,
                            gint    width,
                            gint    height,
                            gint    channels,
                            gint    line,
                            gint    alpha,
                            gint    aprec,
                            gint    zprec)
{
  gint    zR;
  gint    zG;
  gint    zB;
  gint    zA;
  gint    index;
  guchar* scanline;

  scanline = &(pixels[line * width * channels]);

  zR = *scanline << zprec;
  zG = *(scanline + 1) << zprec;
  zB = *(scanline + 2) << zprec;
  zA = *(scanline + 3) << zprec;

  for (index = 0; index < width; index ++)
    _blurinner(&scanline[index * channels], &zR, &zG, &zB, &zA, alpha, aprec,
               zprec);

  for (index = width - 2; index >= 0; index--)
    _blurinner(&scanline[index * channels], &zR, &zG, &zB, &zA, alpha, aprec,
               zprec);
}

static inline void _blurcol(guchar* pixels,
                            gint    width,
                            gint    height,
                            gint    channels,
                            gint    x,
                            gint    alpha,
                            gint    aprec,
                            gint    zprec)
{
  gint zR;
  gint zG;
  gint zB;
  gint zA;
  gint index;
  guchar* ptr;

  ptr = pixels;

  ptr += x * channels;

  zR = *((guchar*) ptr    ) << zprec;
  zG = *((guchar*) ptr + 1) << zprec;
  zB = *((guchar*) ptr + 2) << zprec;
  zA = *((guchar*) ptr + 3) << zprec;

  for (index = width; index < (height - 1) * width; index += width)
    _blurinner((guchar*) &ptr[index * channels], &zR, &zG, &zB, &zA, alpha,
               aprec, zprec);

  for (index = (height - 2) * width; index >= 0; index -= width)
    _blurinner((guchar*) &ptr[index * channels], &zR, &zG, &zB, &zA, alpha,
               aprec, zprec);
}

//
// pixels   image-data
// width    image-width
// height   image-height
// channels image-channels
//
// in-place blur of image 'img' with kernel of approximate radius 'radius'
//
// blurs with two sided exponential impulse response
//
// aprec = precision of alpha parameter in fixed-point format 0.aprec
//
// zprec = precision of state parameters zR,zG,zB and zA in fp format 8.zprec
//
void _expblur(guchar* pixels,
              gint    width,
              gint    height,
              gint    channels,
              gint    radius,
              gint    aprec,
              gint    zprec)
{
  gint alpha;
  gint row = 0;
  gint col = 0;

  if (radius < 1)
    return;

  // calculate the alpha such that 90% of
  // the kernel is within the radius.
  // (Kernel extends to infinity)
  alpha = (gint) ((1 << aprec) * (1.0f - expf (-2.3f / (radius + 1.f))));

  for (; row < height; row++)
    _blurrow(pixels, width, height, channels, row, alpha, aprec, zprec);

  for(; col < width; col++)
    _blurcol(pixels, width, height, channels, col, alpha, aprec, zprec);

  return;
}

void Style::Blur(cairo_t* cr, int size)
{
  pimpl->Blur(cr, size);
}

void Style::Impl::Blur(cairo_t* cr, int size)
{
  // sanity check
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS &&
      cairo_surface_get_type(cairo_get_target(cr)) != CAIRO_SURFACE_TYPE_IMAGE)
    return;

  cairo_surface_t* surface;
  guchar*          pixels;
  guint            width;
  guint            height;
  cairo_format_t   format;

  surface = cairo_get_target(cr);

  // before we mess with the surface execute any pending drawing
  cairo_surface_flush(surface);

  pixels = cairo_image_surface_get_data(surface);
  format = cairo_image_surface_get_format(surface);
  get_actual_cairo_size(cr, &width, &height);

  switch (format)
  {
  case CAIRO_FORMAT_ARGB32:
    _expblur(pixels, width, height, 4, size, 16, 7);
    break;

  case CAIRO_FORMAT_RGB24:
    _expblur(pixels, width, height, 3, size, 16, 7);
    break;

  case CAIRO_FORMAT_A8:
    _expblur(pixels, width, height, 1, size, 16, 7);
    break;

  default :
    // do nothing
    break;
  }

  // inform cairo we altered the surfaces contents
  cairo_surface_mark_dirty(surface);
}

void Style::Impl::SetDefaultValues()
{
  // button-label
  button_label_border_color_[nux::VISUAL_STATE_NORMAL] = nux::Color(0.53, 1.0, 0.66, 0.5);
  button_label_border_color_[nux::VISUAL_STATE_PRESSED] = nux::Color(1.0, 1.0, 1.0, 0.8);
  button_label_border_color_[nux::VISUAL_STATE_PRELIGHT] = nux::Color(0.06, 0.13, 1.0, 0.5);
  //button_label_border_color_[nux::NUX_STATE_SELECTED] = nux::Color(0.07, 0.2, 0.33, 0.5);
  //button_label_border_color_[nux::NUX_STATE_INSENSITIVE] = nux::Color(0.39, 0.26, 0.12, 0.5);

  button_label_border_size_[nux::VISUAL_STATE_NORMAL]          = 0.5;
  button_label_border_size_[nux::VISUAL_STATE_PRESSED]          = 2.0;
  button_label_border_size_[nux::VISUAL_STATE_PRELIGHT]        = 0.5;
  //button_label_border_size_[nux::NUX_STATE_SELECTED]        = 0.5;
  //button_label_border_size_[nux::NUX_STATE_INSENSITIVE]     = 0.5;

  button_label_border_radius_                               = 4.0;
  button_label_text_size_                                   = 1.0;

  button_label_text_color_[nux::VISUAL_STATE_NORMAL] = nux::color::White;
  button_label_text_color_[nux::VISUAL_STATE_PRESSED] = nux::color::Black;
  button_label_text_color_[nux::VISUAL_STATE_PRELIGHT] = nux::color::White;
  //button_label_text_color_[nux::NUX_STATE_SELECTED] = nux::color::White;
  //button_label_text_color_[nux::NUX_STATE_INSENSITIVE] = nux::color::White;

  button_label_fill_color_[nux::VISUAL_STATE_NORMAL] = nux::color::Transparent;
  button_label_fill_color_[nux::VISUAL_STATE_PRESSED] = nux::color::Transparent;
  button_label_fill_color_[nux::VISUAL_STATE_PRELIGHT] = nux::color::Transparent;
  //button_label_fill_color_[nux::NUX_STATE_SELECTED] = nux::color::Transparent;
  //button_label_fill_color_[nux::NUX_STATE_INSENSITIVE] = nux::color::Transparent;

  button_label_overlay_opacity_[nux::VISUAL_STATE_NORMAL]      = 0.0;
  button_label_overlay_opacity_[nux::VISUAL_STATE_PRESSED]      = 0.3;
  button_label_overlay_opacity_[nux::VISUAL_STATE_PRELIGHT]    = 0.0;
  //button_label_overlay_opacity_[nux::NUX_STATE_SELECTED]    = 0.0;
  //button_label_overlay_opacity_[nux::NUX_STATE_INSENSITIVE] = 0.0;

  button_label_overlay_mode_[nux::VISUAL_STATE_NORMAL]         = BlendMode::NORMAL;
  button_label_overlay_mode_[nux::VISUAL_STATE_PRESSED]         = BlendMode::NORMAL;
  button_label_overlay_mode_[nux::VISUAL_STATE_PRELIGHT]       = BlendMode::NORMAL;
  //button_label_overlay_mode_[nux::NUX_STATE_SELECTED]       = BlendMode::NORMAL;
  //button_label_overlay_mode_[nux::NUX_STATE_INSENSITIVE]    = BlendMode::NORMAL;

  button_label_blur_size_[nux::VISUAL_STATE_NORMAL]            = 0;
  button_label_blur_size_[nux::VISUAL_STATE_PRESSED]            = 5;
  button_label_blur_size_[nux::VISUAL_STATE_PRELIGHT]          = 0;
  //button_label_blur_size_[nux::NUX_STATE_SELECTED]          = 0;
  //button_label_blur_size_[nux::NUX_STATE_INSENSITIVE]       = 0;

  // regular-text
  regular_text_color_ = nux::color::White;
  regular_text_size_       = 13.0;
  regular_text_mode_       = BlendMode::NORMAL;
  regular_text_weight_     = FontWeight::LIGHT;

  // separator
  separator_size_           = 1.0;
  separator_color_ = nux::Color(1.0, 1.0, 1.0, 0.15);
  separator_overlay_opacity_ = 0.47;
  separator_overlay_mode_    = BlendMode::NORMAL;
  separator_blur_size_       = 6;

  // scrollbar
  scrollbar_size_                  = 8;
  scrollbar_overlay_size_          = 3;
  scrollbar_buttons_size_          = 0;
  scrollbar_color_                 = nux::color::White;
  scrollbar_overlay_color_         = nux::color::White;
  scrollbar_track_color_           = nux::color::White * 0.4;
  scrollbar_corner_radius_         = 3;
  scrollbar_overlay_corner_radius_ = 1.5;
}

void Style::Impl::ArrowPath(cairo_t* cr, Arrow arrow)
{
  double x  = 0.0;
  double y  = 0.0;
  double w, h;
  get_actual_cairo_size(cr, &w, &h);
  /*double xt = 0.0;
    double yt = 0.0;*/

  // the real shape from the SVG
  // 3.5/1.5, 20.5/22.5, (17x21)
  /*xt = 16.25;
    yt = 9.028;
    cairo_move_to (cr, xt, yt);
    cairo_curve_to (cr, xt - 1.511, yt - 1.006, xt - 3.019, yt - 1.971, xt - 4.527, yt - 2.897);
    xt -= 4.527;
    yt -= 2.897
    cairo_curve_to (cr, 10.213, 5.2, 8.743, 4.335, 7.313, 3.532);
    cairo_curve_to (cr, 8.743, 4.335, 4.613, 2.051, 3.5, 1.5);
    cairo_rel_line_to (cr, 0.0, 21.0);
    cairo_rel_curve_to (cr, 1.164, -0.552, 2.461, -1.229, 3.892, -2.032);
    cairo_rel_curve_to (cr, 1.431, -0.803, 2.9, -1.682, 4.409, -2.634);
    cairo_rel_curve_to (cr, 1.51, -0.953, 3.004, -1.932, 4.488, -2.937);
    cairo_rel_curve_to (cr, 1.481, -1.002, 2.887, -1.981, 4.211, -2.935);
    cairo_curve_to (cr, 19.176, 11.009, 17.759, 10.03, 16.25, 9.028);
    cairo_close_path (cr);*/

  if (arrow == Arrow::LEFT || arrow == Arrow::BOTH)
  {
    x = 1.0;
    y = h / 2.0 - 3.5;
    cairo_move_to(cr, _align(x), _align(y));
    cairo_line_to(cr, _align(x + 5.0), _align(y + 3.5));
    cairo_line_to(cr, _align(x), _align(y + 7.0));
    cairo_close_path(cr);
  }

  if (arrow == Arrow::RIGHT || arrow == Arrow::BOTH)
  {
    x = w - 1.0;
    y = h / 2.0 - 3.5;
    cairo_move_to(cr, _align(x), _align(y));
    cairo_line_to(cr, _align(x - 5.0), _align(y + 3.5));
    cairo_line_to(cr, _align(x), _align(y + 7.0));
    cairo_close_path(cr);
  }
}

void Style::Impl::ButtonOutlinePath (cairo_t* cr, bool align)
{
  double x  = 2.0;
  double y  = 2.0;
  
  double w, h;
  get_actual_cairo_size(cr, &w, &h);
  w -= 4.0;
  h -= 4.0;

  // - these absolute values are the "cost" of getting only a SVG from design
  // and not a generic formular how to approximate the curve-shape, thus
  // the smallest possible button-size is 22.18x24.0
  double width  = w - 22.18;
  double height = h - 24.0;

  double xt = x + width + 22.18;
  double yt = y + 12.0;

  if (align)
  {
    // top right
    cairo_move_to(cr, _align(xt), _align(yt));
    cairo_curve_to(cr, _align(xt - 0.103),
                   _align(yt - 4.355),
                   _align(xt - 1.037),
                   _align(yt - 7.444),
                   _align(xt - 2.811),
                   _align(yt - 9.267));
    xt -= 2.811;
    yt -= 9.267;
    cairo_curve_to(cr, _align(xt - 1.722),
                   _align(yt - 1.823),
                   _align(xt - 4.531),
                   _align(yt - 2.735),
                   _align(xt - 8.28),
                   _align(yt - 2.735));
    xt -= 8.28;
    yt -= 2.735;

    // top
    cairo_line_to(cr, _align(xt - width), _align(yt));
    xt -= width;

    // top left
    cairo_curve_to(cr, _align(xt - 3.748),
                   _align(yt),
                   _align(xt - 6.507),
                   _align(yt + 0.912),
                   _align(xt - 8.279),
                   _align(yt + 2.735));
    xt -= 8.279;
    yt += 2.735;
    cairo_curve_to(cr, _align(xt - 1.773),
                   _align(yt + 1.822),
                   _align(xt - 2.708),
                   _align(yt + 4.911),
                   _align(xt - 2.811),
                   _align(yt + 9.267));
    xt -= 2.811;
    yt += 9.267;

    // left
    cairo_line_to(cr, _align(xt), _align(yt + height));
    yt += height;

    // bottom left
    cairo_curve_to(cr, _align(xt + 0.103),
                   _align(yt + 4.355),
                   _align(xt + 1.037),
                   _align(yt + 7.444),
                   _align(xt + 2.811),
                   _align(yt + 9.267));
    xt += 2.811;
    yt += 9.267;
    cairo_curve_to(cr, _align(xt + 1.772),
                   _align(yt + 1.823),
                   _align(xt + 4.531),
                   _align(yt + 2.735),
                   _align(xt + 8.28),
                   _align(yt + 2.735));
    xt += 8.28;
    yt += 2.735;

    // bottom
    cairo_line_to(cr, _align(xt + width), _align(yt));
    xt += width;

    // bottom right
    cairo_curve_to(cr, _align(xt + 3.748),
                   _align(yt),
                   _align(xt + 6.507),
                   _align(yt - 0.912),
                   _align(xt + 8.279),
                   _align(yt - 2.735));
    xt += 8.279;
    yt -= 2.735;
    cairo_curve_to(cr, _align(xt + 1.773),
                   _align(yt - 1.822),
                   _align(xt + 2.708),
                   _align(yt - 4.911),
                   _align(xt + 2.811),
                   _align(yt - 9.267));
  }
  else
  {
    // top right
    cairo_move_to(cr, x + width + 22.18, y + 12.0);
    cairo_rel_curve_to(cr, -0.103, -4.355, -1.037, -7.444, -2.811, -9.267);
    cairo_rel_curve_to(cr, -1.722, -1.823, -4.531, -2.735, -8.28, -2.735);

    // top
    cairo_rel_line_to(cr, -width, 0.0);

    // top left
    cairo_rel_curve_to(cr, -3.748, 0.0, -6.507, 0.912, -8.279, 2.735);
    cairo_rel_curve_to(cr, -1.773, 1.822, -2.708, 4.911, -2.811, 9.267);

    // left
    cairo_rel_line_to(cr, 0.0, height);

    // bottom left
    cairo_rel_curve_to(cr, 0.103, 4.355, 1.037, 7.444, 2.811, 9.267);
    cairo_rel_curve_to(cr, 1.772, 1.823, 4.531, 2.735, 8.28, 2.735);

    // bottom
    cairo_rel_line_to(cr, width, 0.0);

    // bottom right
    cairo_rel_curve_to(cr, 3.748, 0.0, 6.507, -0.912, 8.279, -2.735);
    cairo_rel_curve_to(cr, 1.773, -1.822, 2.708, -4.911, 2.811, -9.267);
  }

  // right
  cairo_close_path(cr);
}

void Style::Impl::RoundedRectSegment(cairo_t*   cr,
                                         double     aspect,
                                         double     x,
                                         double     y,
                                         double     cornerRadius,
                                         double     width,
                                         double     height,
                                         Segment    segment)
{
  double radius  = cornerRadius / aspect;

  bool odd = cairo_get_line_width (cr) == 2.0 ? false : true;

  switch (segment)
  {
  case Segment::LEFT:
    // top-left, right of the corner
    cairo_move_to(cr, _align(x + radius, odd), _align(y, odd));

    // top-right
    cairo_line_to(cr, _align(x + width, odd), _align(y, odd));

    // bottom-right
    cairo_line_to(cr, _align(x + width, odd), _align(y + height, odd));

    // bottom-left, right of the corner
    cairo_line_to(cr, _align(x + radius, odd), _align(y + height, odd));

    // bottom-left, above the corner
    cairo_arc(cr,
              _align(x + radius, odd),
              _align(y + height - radius, odd),
              radius,
              90.0f * G_PI / 180.0f,
              180.0f * G_PI / 180.0f);

    // left, right of the corner
    cairo_line_to(cr, _align(x, odd), _align(y + radius, odd));

    // top-left, right of the corner
    cairo_arc(cr,
              _align(x + radius, odd),
              _align(y + radius, odd),
              radius,
              180.0f * G_PI / 180.0f,
              270.0f * G_PI / 180.0f);

    break;

  case Segment::MIDDLE:
    // top-left
    cairo_move_to(cr, _align(x, odd), _align(y, odd));

    // top-right
    cairo_line_to(cr, _align(x + width, odd), _align(y, odd));

    // bottom-right
    cairo_line_to(cr, _align(x + width, odd), _align(y + height, odd));

    // bottom-left
    cairo_line_to(cr, _align(x, odd), _align(y + height, odd));

    // back to top-left
    cairo_close_path(cr);
    break;

  case Segment::RIGHT:
    // top-left, right of the corner
    cairo_move_to(cr, _align(x, odd), _align(y, odd));

    // top-right, left of the corner
    cairo_line_to(cr, _align(x + width - radius, odd), _align(y, odd));

    // top-right, below the corner
    cairo_arc(cr,
              _align(x + width - radius, odd),
              _align(y + radius, odd),
              radius,
              -90.0f * G_PI / 180.0f,
              0.0f * G_PI / 180.0f);

    // bottom-right, above the corner
    cairo_line_to(cr, _align(x + width, odd), _align(y + height - radius, odd));

    // bottom-right, left of the corner
    cairo_arc(cr,
              _align(x + width - radius, odd),
              _align(y + height - radius, odd),
              radius,
              0.0f * G_PI / 180.0f,
              90.0f * G_PI / 180.0f);

    // bottom-left
    cairo_line_to(cr, _align(x, odd), _align(y + height, odd));

    // back to top-left
    cairo_close_path(cr);
    break;
  }
}


void Style::Impl::RoundedRectSegmentBorder(cairo_t*   cr,
                                         double     aspect,
                                         double     x,
                                         double     y,
                                         double     cornerRadius,
                                         double     width,
                                         double     height,
                                         Segment    segment,
                                         Arrow      arrow,
                                         nux::ButtonVisualState state)
{
  double radius  = cornerRadius / aspect;
  double arrow_w = radius / 1.5;
  double arrow_h = radius / 2.0;

  bool odd = cairo_get_line_width (cr) == 2.0 ? false : true;

  switch (segment)
  {
  case Segment::LEFT:
    // top-left, right of the corner
    cairo_move_to(cr, _align(x + radius, odd), _align(y, odd));

    // top-right
    cairo_line_to(cr, _align(x + width, odd), _align(y, odd));

    if (arrow == Arrow::RIGHT || arrow == Arrow::BOTH)
    {
      cairo_line_to(cr, _align(x + width, odd),           _align(y + height / 2.0 - arrow_h, odd));
      cairo_line_to(cr, _align(x + width - arrow_w, odd), _align(y + height / 2.0, odd));
      cairo_line_to(cr, _align(x + width, odd),           _align(y + height / 2.0 + arrow_h, odd));
    
      // bottom-right
      cairo_line_to(cr, _align(x + width, odd), _align(y + height, odd));
    }
    else
    {
      // bottom-right
      cairo_move_to(cr, _align(x + width, odd), _align(y + height, odd));
    }

    // bottom-left, right of the corner
    cairo_line_to(cr, _align(x + radius, odd), _align(y + height, odd));

    // bottom-left, above the corner
    cairo_arc(cr,
              _align(x + radius, odd),
              _align(y + height - radius, odd),
              radius,
              90.0f * G_PI / 180.0f,
              180.0f * G_PI / 180.0f);

    // left, right of the corner
    cairo_line_to(cr, _align(x, odd), _align(y + radius, odd));

    // top-left, right of the corner
    cairo_arc(cr,
              _align(x + radius, odd),
              _align(y + radius, odd),
              radius,
              180.0f * G_PI / 180.0f,
              270.0f * G_PI / 180.0f);

    break;

  case Segment::MIDDLE:
    // top-left
    cairo_move_to(cr, _align(x, odd), _align(y, odd));

    // top-right
    cairo_line_to(cr, _align(x + width, odd), _align(y, odd));

    if ((arrow == Arrow::RIGHT || arrow == Arrow::BOTH) && state == nux::VISUAL_STATE_PRESSED)
    {
      cairo_line_to(cr, _align(x + width, odd),           _align(y + height / 2.0 - arrow_h, odd));
      cairo_line_to(cr, _align(x + width - arrow_w, odd), _align(y + height / 2.0, odd));
      cairo_line_to(cr, _align(x + width, odd),           _align(y + height / 2.0 + arrow_h, odd));
      
      // bottom-right
      cairo_line_to(cr, _align(x + width, odd), _align(y + height, odd));
    }
    else
    {
      // bottom-right
      cairo_move_to(cr, _align(x + width, odd), _align(y + height, odd));      
    }

    // bottom-left
    cairo_line_to(cr, _align(x, odd), _align(y + height, odd));

    if ((arrow == Arrow::LEFT || arrow == Arrow::BOTH) && state == nux::VISUAL_STATE_PRESSED)
    {
      cairo_line_to(cr, _align(x, odd),           _align(y + height / 2.0 + arrow_h, odd));
      cairo_line_to(cr, _align(x + arrow_w, odd), _align(y + height / 2.0, odd));
      cairo_line_to(cr, _align(x, odd),           _align(y + height / 2.0 - arrow_h, odd));

      // top-left
      cairo_line_to(cr, _align(x, odd), _align(y, odd));
    }

    break;

  case Segment::RIGHT:
    // top-left, right of the corner
    cairo_move_to(cr, _align(x, odd), _align(y, odd));

    // top-right, left of the corner
    cairo_line_to(cr, _align(x + width - radius, odd), _align(y, odd));

    // top-right, below the corner
    cairo_arc(cr,
              _align(x + width - radius, odd),
              _align(y + radius, odd),
              radius,
              -90.0f * G_PI / 180.0f,
              0.0f * G_PI / 180.0f);

    // bottom-right, above the corner
    cairo_line_to(cr, _align(x + width, odd), _align(y + height - radius, odd));

    // bottom-right, left of the corner
    cairo_arc(cr,
              _align(x + width - radius, odd),
              _align(y + height - radius, odd),
              radius,
              0.0f * G_PI / 180.0f,
              90.0f * G_PI / 180.0f);

    // bottom-left
    cairo_line_to(cr, _align(x, odd), _align(y + height, odd));

    if ((arrow == Arrow::LEFT || arrow == Arrow::BOTH) && state == nux::VISUAL_STATE_PRESSED)
    {
      cairo_line_to(cr, _align(x, odd),           _align(y + height / 2.0 + arrow_h, odd));
      cairo_line_to(cr, _align(x + arrow_w, odd), _align(y + height / 2.0, odd));
      cairo_line_to(cr, _align(x, odd),           _align(y + height / 2.0 - arrow_h, odd));

      // top-left, 
      cairo_line_to(cr, _align(x, odd), _align(y, odd));
    }

    break;
  }
}


void Style::Impl::ButtonOutlinePathSegment(cairo_t* cr, Segment segment)
{
  double   x  = 0.0;
  double   y  = 2.0;

  double w, h;
  get_actual_cairo_size(cr, &w, &h);
  h -= 4.0;

  // - these absolute values are the "cost" of getting only a SVG from design
  // and not a generic formular how to approximate the curve-shape, thus
  // the smallest possible button-size is 22.18x24.0
  double width  = w - 22.18;
  double height = h - 24.0;

  double xt = x + width + 22.18;
  double yt = y + 12.0;

  switch (segment)
  {
  case Segment::LEFT:
    yt -= (9.267 + 2.735);
    cairo_move_to(cr, _align(xt), _align(yt));
    xt -= (2.811 + 8.28);

    // top
    cairo_line_to(cr, _align(xt - width), _align(yt));
    xt -= width;

    // top left
    cairo_curve_to(cr, _align(xt - 3.748),
                   _align(yt),
                   _align(xt - 6.507),
                   _align(yt + 0.912),
                   _align(xt - 8.279),
                   _align(yt + 2.735));
    xt -= 8.279;
    yt += 2.735;
    cairo_curve_to(cr, _align(xt - 1.773),
                   _align(yt + 1.822),
                   _align(xt - 2.708),
                   _align(yt + 4.911),
                   _align(xt - 2.811),
                   _align(yt + 9.267));
    xt -= 2.811;
    yt += 9.267;

    // left
    cairo_line_to(cr, _align(xt), _align(yt + height));
    yt += height;

    // bottom left
    cairo_curve_to(cr, _align(xt + 0.103),
                   _align(yt + 4.355),
                   _align(xt + 1.037),
                   _align(yt + 7.444),
                   _align(xt + 2.811),
                   _align(yt + 9.267));
    xt += 2.811;
    yt += 9.267;
    cairo_curve_to(cr, _align(xt + 1.772),
                   _align(yt + 1.823),
                   _align(xt + 4.531),
                   _align(yt + 2.735),
                   _align(xt + 8.28),
                   _align(yt + 2.735));
    xt += 8.28 + width + 8.279 + 2.811;
    yt += 2.735;

    // bottom
    cairo_line_to(cr, _align(xt), _align(yt));

    cairo_close_path(cr);
    break;

  case Segment::MIDDLE:
    yt -= (9.267 + 2.735);
    cairo_move_to(cr, _align(xt), _align(yt));
    xt -= (2.811 + 8.28);

    // top
    cairo_line_to(cr, _align(xt - width - 8.279 - 2.811), _align(yt));

    // top left
    xt -= width;
    xt -= 8.279;
    yt += 2.735;
    xt -= 2.811;
    yt += 9.267 + height + 2.735 + 9.267;

    // left
    cairo_line_to(cr, _align(xt), _align(yt));

    // bottom left
    xt += 2.811;
    xt += 8.28 + width + 8.279 + 2.811;
    // bottom
    cairo_line_to(cr, _align(xt), _align(yt));

    cairo_close_path(cr);
    break;

  case Segment::RIGHT:
    // top right
    cairo_move_to(cr, _align(xt), _align(yt));
    cairo_curve_to(cr, _align(xt - 0.103),
                   _align(yt - 4.355),
                   _align(xt - 1.037),
                   _align(yt - 7.444),
                   _align(xt - 2.811),
                   _align(yt - 9.267));
    xt -= 2.811;
    yt -= 9.267;
    cairo_curve_to(cr, _align(xt - 1.722),
                   _align(yt - 1.823),
                   _align(xt - 4.531),
                   _align(yt - 2.735),
                   _align(xt - 8.28),
                   _align(yt - 2.735));
    xt -= (8.28 + width + 8.279 + 2.811);
    yt -= 2.735;

    // top
    cairo_line_to(cr, _align(xt), _align(yt));

    // left
    yt += 9.267 + 2.735 + height + 2.735 + 9.267;
    cairo_line_to(cr, _align(xt), _align(yt));

    // bottom left
    xt += 2.811 + 8.28;

    // bottom
    cairo_line_to(cr, _align(xt + width), _align(yt));
    xt += width;

    // bottom right
    cairo_curve_to(cr, _align(xt + 3.748),
                   _align(yt),
                   _align(xt + 6.507),
                   _align(yt - 0.912),
                   _align(xt + 8.279),
                   _align(yt - 2.735));
    xt += 8.279;
    yt -= 2.735;
    cairo_curve_to(cr, _align(xt + 1.773),
                   _align(yt - 1.822),
                   _align(xt + 2.708),
                   _align(yt - 4.911),
                   _align(xt + 2.811),
                   _align(yt - 9.267));

    cairo_close_path(cr);
    break;
  }
}

void Style::Impl::GetTextExtents(int& width,
                                     int& height,
                                     int  maxWidth,
                                     int  maxHeight,
                                     std::string const& text)
{
  cairo_surface_t*      surface  = NULL;
  cairo_t*              cr       = NULL;
  PangoLayout*          layout   = NULL;
  PangoFontDescription* desc     = NULL;
  PangoContext*         pangoCtx = NULL;
  PangoRectangle        inkRect  = {0, 0, 0, 0};
  GdkScreen*            screen   = gdk_screen_get_default();  // is not ref'ed

  surface = cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1);
  cr = cairo_create(surface);
  if (!screen)
    cairo_set_font_options(cr, default_font_options_);
  else
    cairo_set_font_options(cr, gdk_screen_get_font_options(screen));

  layout = pango_cairo_create_layout(cr);
  desc = pango_font_description_from_string(theme::Settings::Get()->font().c_str());

  pango_layout_set_font_description(layout, desc);
  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
  pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

  pango_layout_set_markup(layout, text.c_str(), -1);
  pango_layout_set_height(layout, maxHeight);
  pango_layout_set_width(layout, maxWidth * PANGO_SCALE);

  pangoCtx = pango_layout_get_context(layout);  // is not ref'ed

  if (!screen)
    pango_cairo_context_set_font_options(pangoCtx, default_font_options_);
  else
    pango_cairo_context_set_font_options(pangoCtx,
                                         gdk_screen_get_font_options(screen));

  pango_cairo_context_set_resolution(pangoCtx, 96.0 * Settings::Instance().font_scaling());
  pango_layout_context_changed(layout);
  pango_layout_get_pixel_extents(layout, &inkRect, NULL);

  width  = inkRect.width;
  height = inkRect.height;

  // clean up
  pango_font_description_free(desc);
  g_object_unref(layout);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
}

void Style::Impl::Text(cairo_t*    cr,
                       nux::Color const&  color,
                       std::string const& label,
                       int text_size,
                       double horizMargin,
                       Alignment alignment)
{
  double                x           = 0.0;
  double                y           = 0.0;
  int                   w           = 0;
  int                   h           = 0;
  PangoLayout*          layout      = NULL;
  PangoFontDescription* desc        = NULL;
  PangoContext*         pangoCtx    = NULL;
  GdkScreen*            screen      = gdk_screen_get_default();   // not ref'ed
  gchar*                fontName    = NULL;

  get_actual_cairo_size(cr, &w, &h);
  w -= 2 * horizMargin;

  if (!screen)
    cairo_set_font_options(cr, default_font_options_);
  else
    cairo_set_font_options(cr, gdk_screen_get_font_options(screen));

  layout = pango_cairo_create_layout(cr);
  desc = pango_font_description_from_string(theme::Settings::Get()->font().c_str());

  if (text_size > 0)
  {
    text_size = pango_units_from_double(Settings::Instance().font_scaling() * text_size);
    pango_font_description_set_absolute_size(desc, text_size);
  }

  PangoWeight weight;
  switch (regular_text_weight_)
  {
  case FontWeight::LIGHT:
    weight = PANGO_WEIGHT_LIGHT;
    break;

  case FontWeight::BOLD:
    weight = PANGO_WEIGHT_BOLD;
    break;

  case FontWeight::REGULAR:
  default:
    weight = PANGO_WEIGHT_NORMAL;
  }
  pango_font_description_set_weight(desc, weight);

  pango_layout_set_font_description(layout, desc);
  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);

  PangoAlignment pango_alignment = PANGO_ALIGN_LEFT;
  switch (alignment)
  {
  case Alignment::LEFT:
    pango_alignment = PANGO_ALIGN_LEFT;
    break;

  case Alignment::CENTER:
    pango_alignment = PANGO_ALIGN_CENTER;
    break;

  case Alignment::RIGHT:
    pango_alignment = PANGO_ALIGN_RIGHT;
    break;
  }
  pango_layout_set_alignment(layout, pango_alignment);

  pango_layout_set_markup(layout, label.c_str(), -1);
  pango_layout_set_width(layout, w * PANGO_SCALE);

  pango_layout_set_height(layout, h);
  pangoCtx = pango_layout_get_context(layout);  // is not ref'ed

  if (!screen)
    pango_cairo_context_set_font_options(pangoCtx, default_font_options_);
  else
    pango_cairo_context_set_font_options(pangoCtx,
                                         gdk_screen_get_font_options(screen));

  pango_cairo_context_set_resolution(pangoCtx, 96.0 * Settings::Instance().font_scaling());

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(cr, color);
  pango_layout_context_changed(layout);
  int layout_w = 0;
  int layout_h = 0;
  pango_layout_get_pixel_size(layout, &layout_w, &layout_h);
  x = horizMargin; // let pango alignment handle the x position
  y = (h - layout_h) / 2.0f;

  cairo_move_to(cr, x, y);
  pango_cairo_show_layout(cr, layout);

  // clean up
  pango_font_description_free(desc);
  g_object_unref(layout);
  g_free(fontName);
}

cairo_operator_t Style::Impl::SetBlendMode(cairo_t* cr, BlendMode mode)
{
  // sanity checks
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
    return CAIRO_OPERATOR_OVER;

  cairo_operator_t old;

  old = cairo_get_operator(cr);

  if (mode == BlendMode::NORMAL)
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  else if (mode == BlendMode::MULTIPLY)
    cairo_set_operator(cr, CAIRO_OPERATOR_MULTIPLY);
  else if (mode == BlendMode::SCREEN)
    cairo_set_operator(cr, CAIRO_OPERATOR_SCREEN);

  return old;
}

void Style::Impl::DrawOverlay(cairo_t*  cr,
                                  double    opacity,
                                  BlendMode mode,
                                  int       blurSize)
{
  // sanity checks
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS ||
      opacity <= 0.0                            ||
      blurSize <= 0)
    return;

  cairo_surface_t*     target     = NULL;
  const unsigned char* data       = NULL;
  int                  width      = 0;
  int                  height     = 0;
  double               w_scale    = 0;
  double               h_scale    = 0;
  int                  stride     = 0;
  unsigned char*       buffer     = NULL;
  cairo_surface_t*     surface    = NULL;
  cairo_t*             blurred_cr = NULL;
  cairo_operator_t     old        = CAIRO_OPERATOR_CLEAR;

  // aquire info about image-surface
  target = cairo_get_target(cr);
  data   = cairo_image_surface_get_data(target);
  stride = cairo_image_surface_get_stride(target);
  get_actual_cairo_size(cr, &width, &height);
  cairo_surface_get_device_scale(target, &w_scale, &h_scale);
  cairo_format_t format = cairo_image_surface_get_format(target);

  // get buffer
  buffer = (unsigned char*) calloc(1, height * stride);
  if (!buffer)
    return;

  // copy initial image-surface
  memcpy(buffer, data, height * stride);
  surface = cairo_image_surface_create_for_data(buffer,
                                                format,
                                                width,
                                                height,
                                                stride);
  if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
  {
    cairo_surface_destroy(surface);
    free(buffer);
    return;
  }

  // prepare context
  blurred_cr = cairo_create(surface);
  if (cairo_status(blurred_cr) != CAIRO_STATUS_SUCCESS)
  {
    cairo_destroy(blurred_cr);
    cairo_surface_destroy(surface);
    free(buffer);
    return;
  }

  // blur and blend overlay onto initial image-surface
  cairo_surface_set_device_scale(surface, w_scale, h_scale);
  Blur(blurred_cr, blurSize);
  cairo_set_source_surface(cr, surface, 0.0, 0.0);
  old = SetBlendMode(cr, mode);
  cairo_paint_with_alpha(cr, opacity);

  // clean up
  cairo_destroy(blurred_cr);
  cairo_surface_destroy(surface);
  free(buffer);
  cairo_set_operator(cr, old);
}

bool Style::Button(cairo_t* cr, nux::ButtonVisualState state,
                   std::string const& label, int font_px_size,
                   Alignment alignment, bool zeromargin)
{
  // sanity checks
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
    return false;

  if (cairo_surface_get_type(cairo_get_target(cr)) != CAIRO_SURFACE_TYPE_IMAGE)
    return false;

  unsigned int garnish = 0;
  if (zeromargin == false)
   garnish = GetButtonGarnishSize();

  //ButtonOutlinePath(cr, true);
  double w, h;
  get_actual_cairo_size(cr, &w, &h);

  cairo_set_line_width(cr, pimpl->button_label_border_size_[state]);

  if (pimpl->button_label_border_size_[state] == 2.0)
    RoundedRect(cr,
                1.0,
                (double) (garnish) + 1.0,
                (double) (garnish) + 1.0,
                pimpl->button_label_border_radius_,
                w - (double) (2 * garnish) - 2.0,
                h - (double) (2 * garnish) - 2.0);
  else
    RoundedRect(cr,
                1.0,
                (double) (garnish) + 0.5,
                (double) (garnish) + 0.5,
                pimpl->button_label_border_radius_,
                w - (double) (2 * garnish) - 1.0,
                h - (double) (2 * garnish) - 1.0);


  if (pimpl->button_label_fill_color_[state].alpha != 0.0)
  {
    cairo_set_source_rgba(cr, pimpl->button_label_fill_color_[state]);
    cairo_fill_preserve(cr);
  }
  cairo_set_source_rgba(cr, pimpl->button_label_border_color_[state]);
  cairo_stroke(cr);

  pimpl->DrawOverlay(cr,
                     pimpl->button_label_overlay_opacity_[state],
                     pimpl->button_label_overlay_mode_[state],
                     pimpl->button_label_blur_size_[state] * 0.75);

  static double internal_padding = 5.0f;

  pimpl->Text(cr,
              pimpl->button_label_text_color_[state],
              label,
              font_px_size,
              internal_padding,
              alignment);

  return true;
}

bool Style::LockScreenButton(cairo_t* cr, std::string const& label,
                             int font_px_size)
{
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
    return false;

  if (cairo_surface_get_type(cairo_get_target(cr)) != CAIRO_SURFACE_TYPE_IMAGE)
    return false;

  double w, h;
  get_actual_cairo_size(cr, &w, &h);

  cairo_set_line_width(cr, 1);

  double radius = 5.0;
  RoundedRect(cr, 1.0, 0.5, 0.5, radius, w - 1.0, h - 1.0);

  cairo_set_source_rgba(cr, 0.0f, 0.0f, 0.0f, 0.35f);
  cairo_fill_preserve(cr);

  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 0.7f);
  cairo_stroke(cr);

  static double internal_padding = 10.0f;

  pimpl->Text(cr,
              nux::color::White,
              label,
              font_px_size,
              internal_padding,
              dash::Alignment::LEFT);

  return true;
}

nux::AbstractPaintLayer* Style::FocusOverlay(int width, int height)
{
  nux::CairoGraphics cg(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t* cr = cg.GetInternalContext();

  RoundedRect(cr,
              1.0f,
              0.0f,
              0.0f,
              2.0f, // radius
              width,
              height);

  cairo_set_source_rgba(cr, nux::Color(1.0f, 1.0f, 1.0f, 0.2f));
  cairo_fill(cr);

  // Create the texture layer
  nux::TexCoordXForm texxform;

  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  return (new nux::TextureLayer(texture_ptr_from_cairo_graphics(cg)->GetDeviceTexture(),
                                texxform,
                                nux::color::White,
                                false,
                                rop));
}

bool Style::SquareButton(cairo_t* cr, nux::ButtonVisualState state,
                         std::string const& label, bool curve_bottom,
                         int font_px_size, Alignment alignment,
                         bool zeromargin)
{
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
    return false;

  if (cairo_surface_get_type(cairo_get_target(cr)) != CAIRO_SURFACE_TYPE_IMAGE)
    return false;

  unsigned int garnish = 0;
  if (zeromargin == false)
    garnish = GetButtonGarnishSize();

  double w, h;
  get_actual_cairo_size(cr, &w, &h);

  double x = garnish;
  double y = garnish;

  double width = w - (2.0 * garnish) - 1.0;
  double height = h - (2.0 * garnish) - 1.0;

  bool odd = true;

  // draw the grid background
  {
    cairo_set_line_width(cr, 1);
    cairo_move_to(cr, _align(x + width, odd), y);
    if (curve_bottom)
    {
      double radius = pimpl->button_label_border_radius_;
      LOG_DEBUG(logger) << "curve: " << _align(x + width, odd) << " - " << _align(y + height - radius, odd);
      // line to bottom-right corner
      cairo_line_to(cr, _align(x + width, odd), _align(y + height - radius, odd));

      // line to bottom-right, left of the corner
      cairo_arc(cr,
                _align(x + width - radius, odd),
                _align(y + height - radius, odd),
                radius,
                0.0f * G_PI / 180.0f,
                90.0f * G_PI / 180.0f);

      // line to bottom-left, right of the corner
      cairo_line_to(cr, _align(x + radius, odd), _align(y + height, odd));

      // line to bottom-left, above the corner
      cairo_arc(cr,
                _align(x + radius, odd),
                _align(y + height - radius, odd),
                radius,
                90.0f * G_PI / 180.0f,
                180.0f * G_PI / 180.0f);

      // line to top
      cairo_line_to(cr, _align(x, odd), _align(y, odd));
    }
    else
    {
      cairo_line_to(cr, _align(x + width, odd), _align(y + height, odd));
      cairo_line_to(cr, _align(x, odd), _align(y + height, odd));
      cairo_line_to(cr, _align(x, odd), y);
    }

    cairo_set_source_rgba(cr, pimpl->button_label_border_color_[nux::ButtonVisualState::VISUAL_STATE_NORMAL]);
    cairo_stroke(cr);
  }

  cairo_set_line_width(cr, pimpl->button_label_border_size_[state]);
  odd = cairo_get_line_width(cr) == 2.0 ? false : true;


  if (pimpl->button_label_border_size_[state] == 2.0)
  {
    x += 1;
    y += 1;
    width -= 1.0;
    height -= 1.0;
  }

  if (state == nux::ButtonVisualState::VISUAL_STATE_PRESSED)
  {
    RoundedRect(cr,
                1.0,
                _align(x, odd), _align(y, odd),
                5.0,
                _align(width, odd), _align(height, odd));

    if (pimpl->button_label_fill_color_[state].alpha != 0.0)
    {
      cairo_set_source_rgba(cr, pimpl->button_label_fill_color_[state]);
      cairo_fill_preserve(cr);
    }
    cairo_set_source_rgba(cr, pimpl->button_label_border_color_[state]);
    cairo_stroke(cr);
  }

  pimpl->DrawOverlay(cr,
                     pimpl->button_label_overlay_opacity_[state],
                     pimpl->button_label_overlay_mode_[state],
                     pimpl->button_label_blur_size_[state] * 0.75);

  // FIXME - magic value of 42 here for the offset in the HUD,
  // replace with a nicer style system that lets hud override
  // default values when it needs to
  pimpl->Text(cr,
              pimpl->button_label_text_color_[state],
              label,
              font_px_size,
              42.0 + 10.0,
              alignment);

  return true;
}

bool Style::ButtonFocusOverlay(cairo_t* cr, float alpha)
{
  // sanity checks
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
    return false;

  if (cairo_surface_get_type(cairo_get_target(cr)) != CAIRO_SURFACE_TYPE_IMAGE)
    return false;

  double w, h;
  get_actual_cairo_size(cr, &w, &h);

  nux::Color color(nux::color::White);
  color.alpha = alpha;
  cairo_set_line_width(cr, pimpl->button_label_border_size_[nux::VISUAL_STATE_NORMAL]);

  RoundedRect(cr,
              1.0,
              (double) 0.5,
              (double) 0.5,
              pimpl->button_label_border_radius_,
              w - 1.0,
              h - 1.0);

  cairo_set_source_rgba(cr, color);
  cairo_fill_preserve(cr);
  cairo_stroke(cr);

  return true;
}

bool Style::MultiRangeSegment(cairo_t*    cr,
                              nux::ButtonVisualState  state,
                              std::string const& label,
                              int font_px_size,
                              Arrow       arrow,
                              Segment     segment)
{
  // sanity checks
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
    return false;

  if (cairo_surface_get_type(cairo_get_target(cr)) != CAIRO_SURFACE_TYPE_IMAGE)
    return false;

  //ButtonOutlinePathSegment(cr, segment);
  double x  = 0.0;
  double y  = 2.0;
  double w, h;
  get_actual_cairo_size(cr, &w, &h);
  h -= 4.0;

	if (segment == Segment::LEFT)
	{
    x = 2.0;
    w -= 2.0;
	}

	if (segment == Segment::RIGHT)
	{
    w -= 2.0;
	}

  cairo_set_line_width(cr, pimpl->button_label_border_size_[nux::VISUAL_STATE_NORMAL]);

  pimpl->RoundedRectSegment(cr,
                            1.0,
                            x,
                            y,
                            pimpl->button_label_border_radius_,
                            w,
                            h,
                            segment);

  if (pimpl->button_label_fill_color_[state].alpha != 0.0)
  {
    cairo_set_source_rgba(cr, pimpl->button_label_fill_color_[state]);
    cairo_fill_preserve(cr);
  }

  cairo_set_source_rgba(cr, pimpl->button_label_border_color_[nux::VISUAL_STATE_NORMAL]);
  cairo_stroke(cr); // do not preseve path

  if (state == nux::VISUAL_STATE_PRESSED)
  {
    int line_width = pimpl->button_label_border_size_[state];
    cairo_set_line_width(cr, line_width);

    pimpl->RoundedRectSegmentBorder(cr,
                              1.0,
                              x,
                              y + line_width/2,
                              pimpl->button_label_border_radius_,
                              w,
                              h - line_width,
                              segment,
                              arrow,
                              state);

    cairo_set_source_rgba(cr, pimpl->button_label_border_color_[state]);
    cairo_stroke(cr); // do not preseve path
  }


  pimpl->Text(cr,
              pimpl->button_label_text_color_[state],
              label,
              font_px_size);

  return true;
}

bool Style::MultiRangeFocusOverlay(cairo_t* cr,
                                   Arrow arrow,
                                   Segment segment)
{
  // sanity checks
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
    return false;

  if (cairo_surface_get_type(cairo_get_target(cr)) != CAIRO_SURFACE_TYPE_IMAGE)
    return false;

  double   x  = 0.0;
  double   y  = 2.0;
  double w, h;
  get_actual_cairo_size(cr, &w, &h);
  h -= 4.0;

  if (segment == Segment::LEFT)
  {
    x = 2.0;
    w -= 2.0;
  }

  if (segment == Segment::RIGHT)
  {
    w -= 2.0;
  }

  cairo_set_line_width(cr, pimpl->button_label_border_size_[nux::ButtonVisualState::VISUAL_STATE_NORMAL]);

  pimpl->RoundedRectSegment(cr,
                            1.0,
                            x,
                            y,
                            h / 4.0,
                            w,
                            h,
                            segment);

  cairo_set_source_rgba(cr, nux::Color(1.0f, 1.0f, 1.0f, 0.5f));
  cairo_fill_preserve(cr);
  cairo_stroke(cr);

  return true;
}

bool Style::TrackViewNumber(cairo_t*    cr,
                            nux::ButtonVisualState state,
                            std::string const& trackNumber)
{
  // sanity checks
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
    return false;

  if (cairo_surface_get_type(cairo_get_target(cr)) != CAIRO_SURFACE_TYPE_IMAGE)
    return false;

  return true;
}

bool Style::TrackViewPlay(cairo_t*   cr,
                          nux::ButtonVisualState state)
{
  // sanity checks
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
    return false;

  if (cairo_surface_get_type(cairo_get_target(cr)) != CAIRO_SURFACE_TYPE_IMAGE)
    return false;

  return true;
}

bool Style::TrackViewPause(cairo_t*   cr,
                           nux::ButtonVisualState state)
{
  // sanity checks
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
    return false;

  if (cairo_surface_get_type(cairo_get_target(cr)) != CAIRO_SURFACE_TYPE_IMAGE)
    return false;

  return true;
}

bool Style::TrackViewProgress(cairo_t* cr)
{
  // sanity checks
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
    return false;

  if (cairo_surface_get_type(cairo_get_target(cr)) != CAIRO_SURFACE_TYPE_IMAGE)
    return false;

  return true;
}

bool Style::SeparatorVert(cairo_t* cr)
{
  // sanity checks
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
    return false;

  if (cairo_surface_get_type(cairo_get_target(cr)) != CAIRO_SURFACE_TYPE_IMAGE)
    return false;

  double w, h;
  get_actual_cairo_size(cr, &w, &h);
  double x = w / 2.0;
  double y = 2.0;

  cairo_set_line_width(cr, pimpl->separator_size_);
  cairo_set_source_rgba(cr, pimpl->separator_color_);
  cairo_move_to(cr, _align(x), _align(y));
  cairo_line_to(cr, _align(x), _align(h - 4.0));
  cairo_stroke(cr);

  pimpl->DrawOverlay(cr,
                     pimpl->separator_overlay_opacity_,
                     pimpl->separator_overlay_mode_,
                     pimpl->separator_blur_size_);

  return true;
}

bool Style::SeparatorHoriz(cairo_t* cr)
{
  // sanity checks
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
    return false;

  if (cairo_surface_get_type(cairo_get_target(cr)) != CAIRO_SURFACE_TYPE_IMAGE)
    return false;

  double w, h;
  get_actual_cairo_size(cr, &w, &h);
  double x = 2.0;
  double y = h / 2.0;

  cairo_set_line_width(cr, pimpl->separator_size_);
  cairo_set_source_rgba(cr, pimpl->separator_color_);
  cairo_move_to(cr, _align(x), _align(y));
  cairo_line_to(cr, _align(w - 4.0), _align(y));
  cairo_stroke(cr);

  pimpl->DrawOverlay(cr,
                     pimpl->separator_overlay_opacity_,
                     pimpl->separator_overlay_mode_,
                     pimpl->separator_blur_size_);

  return true;
}

BaseTexturePtr Style::GetDashHorizontalTile(double scale, Position dash_position) const
{
  std::string horizontal_tile;
  if (dash_position == Position::BOTTOM)
    horizontal_tile = "dash_top_border_tile";
  else
    horizontal_tile = "dash_bottom_border_tile";
  return pimpl->LoadScaledTexture(horizontal_tile, scale);
}

BaseTexturePtr Style::GetDashHorizontalTileMask(double scale, Position dash_position) const
{
  std::string horizontal_tile_mask;
  if (dash_position == Position::BOTTOM)
    horizontal_tile_mask = "dash_top_border_tile_mask";
  else
    horizontal_tile_mask = "dash_bottom_border_tile_mask";
  return pimpl->LoadScaledTexture(horizontal_tile_mask, scale);
}

BaseTexturePtr Style::GetDashRightTile(double scale) const
{
  return pimpl->LoadScaledTexture("dash_right_border_tile", scale);
}

BaseTexturePtr Style::GetDashRightTileMask(double scale) const
{
  return pimpl->LoadScaledTexture("dash_right_border_tile_mask", scale);
}

BaseTexturePtr Style::GetDashLeftTile(double scale) const
{
  return pimpl->LoadScaledTexture("dash_left_tile", scale);
}

BaseTexturePtr Style::GetDashTopOrBottomTile(double scale, Position dash_position) const
{
  std::string top_bottom_tile;
  if (dash_position == Position::BOTTOM)
    top_bottom_tile = "dash_bottom_tile";
  else
    top_bottom_tile = "dash_top_tile";
  return pimpl->LoadScaledTexture(top_bottom_tile, scale);
}

BaseTexturePtr Style::GetDashCorner(double scale, Position dash_position) const
{
  std::string corner;
  if (dash_position == Position::BOTTOM)
    corner = "dash_top_right_corner_rotated";
  else
    corner = "dash_bottom_right_corner";
  return pimpl->LoadScaledTexture(corner, scale);
}

BaseTexturePtr Style::GetDashCornerMask(double scale, Position dash_position) const
{
  std::string corner_mask;
  if (dash_position == Position::BOTTOM)
    corner_mask = "dash_top_right_corner_rotated_mask";
  else
    corner_mask = "dash_bottom_right_corner_mask";
  return pimpl->LoadScaledTexture(corner_mask, scale);
}

BaseTexturePtr Style::GetDashLeftCorner(double scale, Position dash_position) const
{
  std::string left_corner;
  if (dash_position == Position::BOTTOM)
    left_corner = "dash_top_left_corner";
  else
    left_corner = "dash_bottom_left_corner";
  return pimpl->LoadScaledTexture(left_corner, scale);
}

BaseTexturePtr Style::GetDashLeftCornerMask(double scale, Position dash_position) const
{
  std::string left_corner_mask;
  if (dash_position == Position::BOTTOM)
    left_corner_mask = "dash_top_left_corner_mask";
  else
    left_corner_mask = "dash_bottom_left_corner_mask";
  return pimpl->LoadScaledTexture(left_corner_mask, scale);
}

BaseTexturePtr Style::GetDashRightCorner(double scale, Position dash_position) const
{
  std::string right_corner;
  if (dash_position == Position::BOTTOM)
    right_corner = "dash_bottom_right_corner_rotated";
  else
    right_corner = "dash_top_right_corner";
  return pimpl->LoadScaledTexture(right_corner, scale);
}

BaseTexturePtr Style::GetDashRightCornerMask(double scale, Position dash_position) const
{
  std::string right_corner_mask;
  if (dash_position == Position::BOTTOM)
    right_corner_mask = "dash_bottom_right_corner_rotated_mask";
  else
    right_corner_mask = "dash_top_right_corner_mask";
  return pimpl->LoadScaledTexture(right_corner_mask, scale);
}

BaseTexturePtr Style::GetSearchMagnifyIcon(double scale) const
{
  return pimpl->LoadScaledTexture("search_magnify", scale);
}

BaseTexturePtr Style::GetSearchCircleIcon(double scale) const
{
  return pimpl->LoadScaledTexture("search_circle", scale);
}

BaseTexturePtr Style::GetSearchCloseIcon(double scale) const
{
  return pimpl->LoadScaledTexture("search_close", scale);
}

BaseTexturePtr Style::GetSearchSpinIcon(double scale) const
{
  return pimpl->LoadScaledTexture("search_spin", scale);
}

BaseTexturePtr Style::GetLockScreenActivator(double scale) const
{
  return pimpl->LoadScaledTexture("arrow_right", scale);
}

RawPixel Style::GetButtonGarnishSize() const
{
  int maxBlurSize = 0;

  for (int i = 0; i < STATES; i++)
  {
    if (maxBlurSize < pimpl->button_label_blur_size_[i])
      maxBlurSize = pimpl->button_label_blur_size_[i];
  }

  return 2 * maxBlurSize;
}

RawPixel Style::GetSeparatorGarnishSize() const
{
  return pimpl->separator_blur_size_;
}

nux::Color const& Style::GetTextColor() const
{
  return pimpl->text_color_;
}

RawPixel Style::GetTileGIconSize() const
{
  return 64;
}

RawPixel Style::GetTileImageSize() const
{
  return 96;
}

RawPixel Style::GetTileWidth() const
{
  return std::max(pimpl->text_width_, 150);
}

RawPixel Style::GetTileHeight() const
{
  return std::max(GetTileImageSize() + (pimpl->text_height_ * 2) + 15,
                  GetTileImageSize() + 32); // magic design numbers.
}

RawPixel Style::GetTileIconHightlightHeight() const
{
  return 106;
}

RawPixel Style::GetTileIconHightlightWidth() const
{
  return 106;
}

RawPixel Style::GetHomeTileIconSize() const
{
  return 104;
}

RawPixel Style::GetHomeTileWidth() const
{
  return pimpl->text_width_ * 1.2;
}

RawPixel Style::GetHomeTileHeight() const
{
  return GetHomeTileIconSize() + (pimpl->text_height_ * 5);
}

RawPixel Style::GetTextLineHeight() const
{
  return pimpl->text_height_;
}


BaseTexturePtr const& Style::GetCategoryBackground() const
{
  return pimpl->category_texture_.texture();
}

BaseTexturePtr const& Style::GetCategoryBackgroundNoFilters() const
{
  return pimpl->category_texture_no_filters_.texture(); 
}

BaseTexturePtr const& Style::GetInformationTexture() const
{
  return pimpl->information_texture_.texture(); 
}

BaseTexturePtr const& Style::GetRefineTextureCorner() const
{
  return pimpl->refine_gradient_corner_.texture();
}

BaseTexturePtr const& Style::GetRefineTextureDash() const
{
  return pimpl->refine_gradient_dash_.texture(); 
}

BaseTexturePtr const& Style::GetGroupUnexpandIcon() const
{
  return pimpl->group_unexpand_texture_.texture();
}

BaseTexturePtr const& Style::GetGroupExpandIcon() const
{
  return pimpl->group_expand_texture_.texture();
}

BaseTexturePtr const& Style::GetStarDeselectedIcon() const
{
  return pimpl->star_deselected_texture_.texture();
}

BaseTexturePtr const& Style::GetStarSelectedIcon() const
{
  return pimpl->star_selected_texture_.texture();
}

BaseTexturePtr const& Style::GetStarHighlightIcon() const
{
  return pimpl->star_highlight_texture_.texture();
}

BaseTexturePtr const& Style::GetDashShine() const
{
  return pimpl->dash_shine_.texture();
}

RawPixel Style::GetDashHorizontalBorderHeight() const
{
  return 30;
}

RawPixel Style::GetDashVerticalBorderWidth() const
{
  return 30;
}

RawPixel Style::GetVSeparatorSize() const
{
  return 1;
}

RawPixel Style::GetHSeparatorSize() const
{
  return 1;
}

RawPixel Style::GetFilterBarWidth() const
{
  return 300;
}

RawPixel Style::GetFilterBarLeftPadding() const
{
  return 5;
}

RawPixel Style::GetFilterBarRightPadding() const
{
  return 5;
}

RawPixel Style::GetDashViewTopPadding() const
{
  return 10;
}

RawPixel Style::GetSearchBarLeftPadding() const
{
  return 10;
}

RawPixel Style::GetSearchBarRightPadding() const
{
  return 10;
}

RawPixel Style::GetSearchBarHeight() const
{
  return 42;
}

RawPixel Style::GetFilterResultsHighlightRightPadding() const
{
  return 5;
}

RawPixel Style::GetFilterResultsHighlightLeftPadding() const
{
  return 5;
}

RawPixel Style::GetFilterBarTopPadding() const
{
  return 10;
}

RawPixel Style::GetFilterHighlightPadding() const
{
  return 2;
}

RawPixel Style::GetSpaceBetweenFilterWidgets() const
{
  return 12;
}

RawPixel Style::GetAllButtonHeight() const
{
  return 30;
}

RawPixel Style::GetFilterButtonHeight() const
{
  return 30;
}

RawPixel Style::GetSpaceBetweenScopeAndFilters() const
{
  return 10;
}

RawPixel Style::GetFilterViewRightPadding() const
{
  return 10;
}

RawPixel Style::GetOverlayScrollbarSize() const
{
  return pimpl->scrollbar_overlay_size_;
}

RawPixel Style::GetScrollbarSize() const
{
  return pimpl->scrollbar_size_;
}

RawPixel Style::GetScrollbarButtonsSize() const
{
  return pimpl->scrollbar_buttons_size_;
}

RawPixel Style::GetOverlayScrollbarCornerRadius() const
{
  return pimpl->scrollbar_overlay_corner_radius_;
}

RawPixel Style::GetScrollbarCornerRadius() const
{
  return pimpl->scrollbar_corner_radius_;
}

nux::Color Style::GetOverlayScrollbarColor() const
{
  return pimpl->scrollbar_overlay_color_;
}

nux::Color Style::GetScrollbarColor() const
{
  return pimpl->scrollbar_color_;
}

nux::Color Style::GetScrollbarTrackColor() const
{
  return pimpl->scrollbar_track_color_;
}

RawPixel Style::GetCategoryIconSize() const
{
  return 22;
}

RawPixel Style::GetCategoryHighlightHeight() const
{
  return 24;
}

RawPixel Style::GetPlacesGroupTopSpace() const
{
  return 7;
}

RawPixel Style::GetPlacesGroupResultTopPadding() const
{
  return 2;
}

RawPixel Style::GetPlacesGroupResultLeftPadding() const
{
  return 25;
}

RawPixel Style::GetCategoryHeaderLeftPadding() const
{
  return 19;
}

RawPixel Style::GetCategorySeparatorLeftPadding() const
{
  return 15;
}

RawPixel Style::GetCategorySeparatorRightPadding() const
{
  return 15;
}

namespace
{

LazyLoadTexture::LazyLoadTexture(std::string const& filename, int size)
  : filename_(filename)
  , size_(size)
{
  TextureCache::GetDefault().themed_invalidated.connect(sigc::mem_fun(this, &LazyLoadTexture::UnloadTexture));
}

BaseTexturePtr const& LazyLoadTexture::texture()
{
  if (!texture_)
    LoadTexture();
  return texture_;
}

void LazyLoadTexture::LoadTexture()
{
  TextureCache& cache = TextureCache::GetDefault();
  texture_ = cache.FindTexture(filename_, size_, size_);
}

void LazyLoadTexture::UnloadTexture()
{
  texture_ = nullptr;
}

} // anon namespace

} // namespace dash
} // namespace unity
