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
#include <NuxImage/CairoGraphics.h>
#include <Nux/PaintLayer.h>

#include <UnityCore/GLibSignal.h>
#include <UnityCore/GLibWrapper.h>

#include "CairoTexture.h"
#include "JSONParser.h"
#include "config.h"

#define DASH_WIDGETS_FILE DATADIR"/unity/themes/dash-widgets.json"

typedef nux::ObjectPtr<nux::BaseTexture> BaseTexturePtr;

namespace unity
{
namespace dash
{
namespace
{
nux::logging::Logger logger("unity.dash");

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

class LazyLoadTexture
{
public:
  LazyLoadTexture(std::string const& filename, int size = -1);
  nux::BaseTexture* texture();
private:
  void LoadTexture();
private:
  std::string filename_;
  int size_;
  BaseTexturePtr texture_;
};

} // anon namespace


class Style::Impl
{
public:
  Impl(Style* owner);
  ~Impl();

  void Blur(cairo_t* cr, int size);

  void SetDefaultValues();

  void GetTextExtents(int& width,
                      int& height,
                      int  maxWidth,
                      int  maxHeight,
                      std::string const& text);

  void Text(cairo_t* cr,
            nux::Color const& color,
            std::string const& label,
            int font_size = -1,
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
                          Segment    segment,
                          Arrow      arrow,
                          nux::ButtonVisualState state);

  void Refresh();
  void OnFontChanged(GtkSettings* object, GParamSpec* pspec);

  // Members
  Style* owner_;

  cairo_font_options_t* default_font_options_;

  std::vector<nux::Color> button_label_border_color_;
  std::vector<double>     button_label_border_size_;
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

  nux::Color            scrollbar_color_;
  double                scrollbar_overlay_opacity_;
  BlendMode             scrollbar_overlay_mode_;
  int                   scrollbar_blur_size_;
  int                   scrollbar_size_;
  double                scrollbar_corner_radius_;

  glib::SignalManager signal_manager_;

  nux::Color text_color_;

  int text_width_;
  int text_height_;
  int number_of_columns_;

  LazyLoadTexture dash_bottom_texture_;
  LazyLoadTexture dash_bottom_texture_mask_;
  LazyLoadTexture dash_right_texture_;
  LazyLoadTexture dash_right_texture_mask_;
  LazyLoadTexture dash_corner_texture_;
  LazyLoadTexture dash_corner_texture_mask_;
  LazyLoadTexture dash_fullscreen_icon_;
  LazyLoadTexture dash_left_edge_;
  LazyLoadTexture dash_left_corner_;
  LazyLoadTexture dash_left_corner_mask_;
  LazyLoadTexture dash_left_tile_;
  LazyLoadTexture dash_top_corner_;
  LazyLoadTexture dash_top_corner_mask_;
  LazyLoadTexture dash_top_tile_;

  LazyLoadTexture dash_shine_;

  LazyLoadTexture search_magnify_texture_;
  LazyLoadTexture search_circle_texture_;
  LazyLoadTexture search_close_texture_;
  LazyLoadTexture search_spin_texture_;

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
  , number_of_columns_(6)
  , dash_bottom_texture_("/dash_bottom_border_tile.png")
  , dash_bottom_texture_mask_("/dash_bottom_border_tile_mask.png")
  , dash_right_texture_("/dash_right_border_tile.png")
  , dash_right_texture_mask_("/dash_right_border_tile_mask.png")
  , dash_corner_texture_("/dash_bottom_right_corner.png")
  , dash_corner_texture_mask_("/dash_bottom_right_corner_mask.png")
  , dash_fullscreen_icon_("/dash_fullscreen_icon.png")
  , dash_left_edge_("/dash_left_edge.png")
  , dash_left_corner_("/dash_bottom_left_corner.png")
  , dash_left_corner_mask_("/dash_bottom_left_corner_mask.png")
  , dash_left_tile_("/dash_left_tile.png")
  , dash_top_corner_("/dash_top_right_corner.png")
  , dash_top_corner_mask_("/dash_top_right_corner_mask.png")
  , dash_top_tile_("/dash_top_tile.png")
  , dash_shine_("/dash_sheen.png")
  , search_magnify_texture_("/search_magnify.png")
  , search_circle_texture_("/search_circle.svg", 32)
  , search_close_texture_("/search_close.svg", 32)
  , search_spin_texture_("/search_spin.svg", 32)
  , group_unexpand_texture_("/dash_group_unexpand.png")
  , group_expand_texture_("/dash_group_expand.png")
  , star_deselected_texture_("/star_deselected.png")
  , star_selected_texture_("/star_selected.png")
  , star_highlight_texture_("/star_highlight.png")
{
  signal_manager_.Add(new glib::Signal<void, GtkSettings*, GParamSpec*>
                      (gtk_settings_get_default(),
                       "notify::gtk-font-name",
                       sigc::mem_fun(this, &Impl::OnFontChanged)));
  signal_manager_.Add(new glib::Signal<void, GtkSettings*, GParamSpec*>
                      (gtk_settings_get_default(),
                       "notify::gtk-xft-dpi",
                       sigc::mem_fun(this, &Impl::OnFontChanged)));
  Refresh();

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

  json::Parser parser;
  // Since the parser skips values if they are not found, make sure everything
  // is initialised.
  SetDefaultValues();
  if (!parser.Open(DASH_WIDGETS_FILE))
    return;

  // button-label
  parser.ReadColors("button-label", "border-color", "border-opacity",
                    button_label_border_color_);
  parser.ReadDoubles("button-label", "border-size", button_label_border_size_);
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
  parser.ReadColor("scrollbar", "color", "opacity", scrollbar_color_);
  parser.ReadDouble("scrollbar", "overlay-opacity", scrollbar_overlay_opacity_);
  parser.ReadMappedString("scrollbar", "overlay-mode", blend_mode_map,
                          scrollbar_overlay_mode_);
  parser.ReadInt("scrollbar", "blur-size", scrollbar_blur_size_);
  parser.ReadInt("scrollbar", "size", scrollbar_size_);
  parser.ReadDouble("scrollbar", "corner-radius", scrollbar_corner_radius_);
}

Style::Impl::~Impl()
{
  if (cairo_font_options_status(default_font_options_) == CAIRO_STATUS_SUCCESS)
    cairo_font_options_destroy(default_font_options_);
}

void Style::Impl::Refresh()
{
  const char* const SAMPLE_MAX_TEXT = "Chromium Web Browser";
  GtkSettings* settings = ::gtk_settings_get_default();

  nux::CairoGraphics util_cg(CAIRO_FORMAT_ARGB32, 1, 1);
  cairo_t* cr = util_cg.GetInternalContext();

  glib::String font_description;
  int dpi = 0;
  ::g_object_get(settings,
                 "gtk-font-name", &font_description,
                 "gtk-xft-dpi", &dpi,
                 NULL);
  PangoFontDescription* desc = ::pango_font_description_from_string(font_description);
  ::pango_font_description_set_weight(desc, PANGO_WEIGHT_NORMAL);
  ::pango_font_description_set_size(desc, 9 * PANGO_SCALE);

  glib::Object<PangoLayout> layout(::pango_cairo_create_layout(cr));
  ::pango_layout_set_font_description(layout, desc);
  ::pango_layout_set_text(layout, SAMPLE_MAX_TEXT, -1);

  PangoContext* cxt = ::pango_layout_get_context(layout);

  GdkScreen* screen = ::gdk_screen_get_default();
  ::pango_cairo_context_set_font_options(cxt, ::gdk_screen_get_font_options(screen));
  float pango_scale = PANGO_SCALE;
  ::pango_cairo_context_set_resolution(cxt, dpi / pango_scale);
  ::pango_layout_context_changed(layout);

  PangoRectangle log_rect;
  ::pango_layout_get_extents(layout, NULL, &log_rect);
  text_width_ = log_rect.width / PANGO_SCALE;
  text_height_ = log_rect.height / PANGO_SCALE;

  owner_->changed.emit();

  pango_font_description_free(desc);
}


void Style::Impl::OnFontChanged(GtkSettings* object, GParamSpec* pspec)
{
  Refresh();
}


Style::Style()
  : pimpl(new Impl(this))
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

  bool odd = true;

  odd = cairo_get_line_width (cr) == 2.0 ? false : true;

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
  width  = cairo_image_surface_get_width(surface);
  height = cairo_image_surface_get_height(surface);
  format = cairo_image_surface_get_format(surface);

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
  scrollbar_color_ = nux::color::White;
  scrollbar_overlay_opacity_ = 0.3;
  scrollbar_overlay_mode_    = BlendMode::NORMAL;
  scrollbar_blur_size_       = 5;
  scrollbar_size_           = 3;
  scrollbar_corner_radius_   = 1.5;
}

void Style::Impl::ArrowPath(cairo_t* cr, Arrow arrow)
{
  double x  = 0.0;
  double y  = 0.0;
  double w  = cairo_image_surface_get_width(cairo_get_target(cr));
  double h  = cairo_image_surface_get_height(cairo_get_target(cr));
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
  double w  = cairo_image_surface_get_width(cairo_get_target(cr)) - 4.0;
  double h  = cairo_image_surface_get_height(cairo_get_target(cr)) - 4.0;
  double xt = 0.0;
  double yt = 0.0;

  // - these absolute values are the "cost" of getting only a SVG from design
  // and not a generic formular how to approximate the curve-shape, thus
  // the smallest possible button-size is 22.18x24.0
  double width  = w - 22.18;
  double height = h - 24.0;

  xt = x + width + 22.18;
  yt = y + 12.0;

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
                                         Segment    segment,
                                         Arrow      arrow,
                                         nux::ButtonVisualState state)
{
  double radius  = cornerRadius / aspect;
  double arrow_w = radius / 1.5;
  double arrow_h = radius / 2.0;
  bool odd = true;

  odd = cairo_get_line_width (cr) == 2.0 ? false : true;

  switch (segment)
  {
  case Segment::LEFT:
    // top-left, right of the corner
    cairo_move_to(cr, _align(x + radius, odd), _align(y, odd));

    // top-right
    cairo_line_to(cr, _align(x + width, odd), _align(y, odd));

    if (arrow == Arrow::RIGHT && state == nux::VISUAL_STATE_PRESSED)
		{
      cairo_line_to(cr, _align(x + width, odd),           _align(y + height / 2.0 - arrow_h, odd));
      cairo_line_to(cr, _align(x + width - arrow_w, odd), _align(y + height / 2.0, odd));
      cairo_line_to(cr, _align(x + width, odd),           _align(y + height / 2.0 + arrow_h, odd));
		}

    // bottom-right
    cairo_line_to(cr, _align(x + width, odd), _align(y + height, odd));

    // bottom-left, right of the corner
    cairo_line_to(cr, _align(x + radius, odd), _align(y + height, odd));

    // bottom-left, above the corner
    cairo_arc(cr,
              _align(x, odd) + _align(radius, odd),
              _align(y + height, odd) - _align(radius, odd),
              _align(radius, odd),
              90.0f * G_PI / 180.0f,
              180.0f * G_PI / 180.0f);

    // left, right of the corner
    cairo_line_to(cr, _align(x, odd), _align(y + radius, odd));

    // top-left, right of the corner
    cairo_arc(cr,
              _align(x, odd) + _align(radius, odd),
              _align(y, odd) + _align(radius, odd),
              _align(radius, odd),
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
		}

    // bottom-right
    cairo_line_to(cr, _align(x + width, odd), _align(y + height, odd));

    // bottom-left
    cairo_line_to(cr, _align(x, odd), _align(y + height, odd));

    if ((arrow == Arrow::LEFT || arrow == Arrow::BOTH) && state == nux::VISUAL_STATE_PRESSED)
		{
      cairo_line_to(cr, _align(x, odd),           _align(y + height / 2.0 + arrow_h, odd));
      cairo_line_to(cr, _align(x + arrow_w, odd), _align(y + height / 2.0, odd));
      cairo_line_to(cr, _align(x, odd),           _align(y + height / 2.0 - arrow_h, odd));
		}

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
              _align(x + width, odd) - _align(radius, odd),
              _align(y, odd) + _align(radius, odd),
              _align(radius, odd),
              -90.0f * G_PI / 180.0f,
              0.0f * G_PI / 180.0f);

    // bottom-right, above the corner
    cairo_line_to(cr, _align(x + width, odd), _align(y + height - radius, odd));

    // bottom-right, left of the corner
    cairo_arc(cr,
              _align(x + width, odd) - _align(radius, odd),
              _align(y + height, odd) - _align(radius, odd),
              _align(radius, odd),
              0.0f * G_PI / 180.0f,
              90.0f * G_PI / 180.0f);

    // bottom-left
    cairo_line_to(cr, _align(x, odd), _align(y + height, odd));

    if (arrow == Arrow::LEFT && state == nux::VISUAL_STATE_PRESSED)
		{
      cairo_line_to(cr, _align(x, odd),           _align(y + height / 2.0 + arrow_h, odd));
      cairo_line_to(cr, _align(x + arrow_w, odd), _align(y + height / 2.0, odd));
      cairo_line_to(cr, _align(x, odd),           _align(y + height / 2.0 - arrow_h, odd));
		}

    // back to top-left
    cairo_close_path(cr);
    break;
  }
}

void Style::Impl::ButtonOutlinePathSegment(cairo_t* cr, Segment segment)
{
  double   x  = 0.0;
  double   y  = 2.0;
  double   w  = cairo_image_surface_get_width(cairo_get_target(cr));
  double   h  = cairo_image_surface_get_height(cairo_get_target(cr)) - 4.0;
  double   xt = 0.0;
  double   yt = 0.0;

  // - these absolute values are the "cost" of getting only a SVG from design
  // and not a generic formular how to approximate the curve-shape, thus
  // the smallest possible button-size is 22.18x24.0
  double width  = w - 22.18;
  double height = h - 24.0;

  xt = x + width + 22.18;
  yt = y + 12.0;

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
  int                   dpi      = 0;
  char*                 fontName = NULL;
  GdkScreen*            screen   = gdk_screen_get_default();  // is not ref'ed
  GtkSettings*          settings = gtk_settings_get_default();// is not ref'ed

  surface = cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1);
  cr = cairo_create(surface);
  if (!screen)
    cairo_set_font_options(cr, default_font_options_);
  else
    cairo_set_font_options(cr, gdk_screen_get_font_options(screen));
  layout = pango_cairo_create_layout(cr);

  g_object_get(settings, "gtk-font-name", &fontName, NULL);
  if (!fontName)
    desc = pango_font_description_from_string("Sans 10");
  else
  {
    desc = pango_font_description_from_string(fontName);
    g_free(fontName);
  }

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
  pango_layout_get_extents(layout, &inkRect, NULL);

  width  = inkRect.width / PANGO_SCALE;
  height = inkRect.height / PANGO_SCALE;

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
  int                   dpi         = 0;
  GdkScreen*            screen      = gdk_screen_get_default();   // not ref'ed
  GtkSettings*          settings    = gtk_settings_get_default(); // not ref'ed
  gchar*                fontName    = NULL;
  //double                horizMargin = 10.0;

  w = cairo_image_surface_get_width(cairo_get_target(cr));
  h = cairo_image_surface_get_height(cairo_get_target(cr));

  w -= 2 * horizMargin;

  if (!screen)
    cairo_set_font_options(cr, default_font_options_);
  else
    cairo_set_font_options(cr, gdk_screen_get_font_options(screen));
  layout = pango_cairo_create_layout(cr);

  g_object_get(settings, "gtk-font-name", &fontName, NULL);
  if (!fontName)
    desc = pango_font_description_from_string("Sans 10");
  else
    desc = pango_font_description_from_string(fontName);

  if (text_size > 0)
  {
    pango_font_description_set_absolute_size(desc, text_size * PANGO_SCALE);
  }

  PangoWeight weight;
  switch (regular_text_weight_)
  {
  case FontWeight::REGULAR:
    weight = PANGO_WEIGHT_NORMAL;
    break;

  case FontWeight::LIGHT:
    weight = PANGO_WEIGHT_LIGHT;
    break;

  case FontWeight::BOLD:
    weight = PANGO_WEIGHT_BOLD;
    break;
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

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(cr, color);
  pango_layout_context_changed(layout);
  PangoRectangle ink = {0, 0, 0, 0};
  PangoRectangle log = {0, 0, 0, 0};
  pango_layout_get_extents(layout, &ink, &log);
  x = horizMargin; // let pango alignment handle the x position
  y = ((double) h - pango_units_to_double(log.height)) / 2.0;

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
  int                  stride     = 0;
  cairo_format_t       format     = CAIRO_FORMAT_INVALID;
  unsigned char*       buffer     = NULL;
  cairo_surface_t*     surface    = NULL;
  cairo_t*             blurred_cr = NULL;
  cairo_operator_t     old        = CAIRO_OPERATOR_CLEAR;

  // aquire info about image-surface
  target = cairo_get_target(cr);
  data   = cairo_image_surface_get_data(target);
  width  = cairo_image_surface_get_width(target);
  height = cairo_image_surface_get_height(target);
  stride = cairo_image_surface_get_stride(target);
  format = cairo_image_surface_get_format(target);

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
  Blur(blurred_cr, blurSize);
  //cairo_surface_write_to_png(surface, "/tmp/overlay-surface.png");
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
                   std::string const& label, int font_size,
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
  double w = cairo_image_surface_get_width(cairo_get_target(cr));
  double h = cairo_image_surface_get_height(cairo_get_target(cr));

  cairo_set_line_width(cr, pimpl->button_label_border_size_[state]);

  if (pimpl->button_label_border_size_[state] == 2.0)
    RoundedRect(cr,
                1.0,
                (double) (garnish) + 1.0,
                (double) (garnish) + 1.0,
                7.0,
                w - (double) (2 * garnish) - 2.0,
                h - (double) (2 * garnish) - 2.0);
  else
    RoundedRect(cr,
                1.0,
                (double) (garnish) + 0.5,
                (double) (garnish) + 0.5,
                7.0,
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

  pimpl->Text(cr,
              pimpl->button_label_text_color_[state],
              label,
              font_size,
              11.0, // 15px = 11pt
              alignment);

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
                         int font_size, Alignment alignment,
                         bool zeromargin)
{
  // sanity checks
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
    return false;

  if (cairo_surface_get_type(cairo_get_target(cr)) != CAIRO_SURFACE_TYPE_IMAGE)
    return false;

  unsigned int garnish = 0;
  if (zeromargin == false)
    garnish = GetButtonGarnishSize();

  double w = cairo_image_surface_get_width(cairo_get_target(cr));
  double h = cairo_image_surface_get_height(cairo_get_target(cr));

  double x = garnish;
  double y = garnish;

  double width = w - (2.0 * garnish) - 1.0;
  double height = h - (2.0 * garnish) - 1.0;

  bool odd = true;
  double radius = 7.0;

  // draw the grid background
  {
    cairo_set_line_width(cr, 1);
    cairo_move_to(cr, _align(x + width, odd), _align(y, odd));
    if (curve_bottom)
    {
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
      cairo_line_to(cr, _align(x, odd), _align(x + height, odd));
      cairo_line_to(cr, _align(x, odd), _align(y, odd));
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
              font_size,
              42.0 + 10.0,
              alignment);

  cairo_surface_write_to_png(cairo_get_target(cr), "/tmp/wut.png");

  return true;
}

bool Style::ButtonFocusOverlay(cairo_t* cr)
{
  // sanity checks
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
    return false;

  if (cairo_surface_get_type(cairo_get_target(cr)) != CAIRO_SURFACE_TYPE_IMAGE)
    return false;

  double w = cairo_image_surface_get_width(cairo_get_target(cr));
  double h = cairo_image_surface_get_height(cairo_get_target(cr));

  nux::Color color(nux::color::White);
  color.alpha = 0.50f;
  cairo_set_line_width(cr, pimpl->button_label_border_size_[nux::VISUAL_STATE_NORMAL]);

  RoundedRect(cr,
              1.0,
              (double) 0.5,
              (double) 0.5,
              7.0,
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
                              Arrow       arrow,
                              Segment     segment)
{
  // sanity checks
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
    return false;

  if (cairo_surface_get_type(cairo_get_target(cr)) != CAIRO_SURFACE_TYPE_IMAGE)
    return false;

  //ButtonOutlinePathSegment(cr, segment);
  double   x  = 0.0;
  double   y  = 2.0;
  double   w  = cairo_image_surface_get_width(cairo_get_target(cr));
  double   h  = cairo_image_surface_get_height(cairo_get_target(cr)) - 4.0;

	if (segment == Segment::LEFT)
	{
    x = 2.0;
    w -= 2.0;
	}

	if (segment == Segment::RIGHT)
	{
    w -= 2.0;
	}

  cairo_set_line_width(cr, pimpl->button_label_border_size_[state]);

  if (pimpl->button_label_border_size_[state] == 2.0)
    pimpl->RoundedRectSegment(cr,
                              1.0,
                              x+1.0,
                              y+1.0,
                              (h-1.0) / 4.0,
                              w-1.0,
                              h-1.0,
                              segment,
                              arrow,
                              state);
  else
    pimpl->RoundedRectSegment(cr,
                              1.0,
                              x,
                              y,
                              h / 4.0,
                              w,
                              h,
                              segment,
                              arrow,
                              state);

  if (pimpl->button_label_fill_color_[state].alpha != 0.0)
  {
    cairo_set_source_rgba(cr, pimpl->button_label_fill_color_[state]);
    cairo_fill_preserve(cr);
  }
  cairo_set_source_rgba(cr, pimpl->button_label_border_color_[state]);
  cairo_stroke(cr);
  pimpl->Text(cr,
              pimpl->button_label_text_color_[state],
              label,
              10); // 13px = 10pt

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
  double   w  = cairo_image_surface_get_width(cairo_get_target(cr));
  double   h  = cairo_image_surface_get_height(cairo_get_target(cr)) - 4.0;

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
                            segment,
                            arrow,
                            nux::ButtonVisualState::VISUAL_STATE_PRESSED);

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

  double w = cairo_image_surface_get_width(cairo_get_target(cr));
  double h = cairo_image_surface_get_height(cairo_get_target(cr));
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

  double w = cairo_image_surface_get_width(cairo_get_target(cr));
  double h = cairo_image_surface_get_height(cairo_get_target(cr));
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

int Style::GetButtonGarnishSize()
{
  int maxBlurSize = 0;

  for (int i = 0; i < STATES; i++)
  {
    if (maxBlurSize < pimpl->button_label_blur_size_[i])
      maxBlurSize = pimpl->button_label_blur_size_[i];
  }

  return 2 * maxBlurSize;
}

int Style::GetSeparatorGarnishSize()
{
  return pimpl->separator_blur_size_;
}

int Style::GetScrollbarGarnishSize()
{
  return pimpl->scrollbar_blur_size_;
}

nux::Color const& Style::GetTextColor() const
{
  return pimpl->text_color_;
}

int  Style::GetDefaultNColumns() const
{
  return pimpl->number_of_columns_;
}

void Style::SetDefaultNColumns(int n_cols)
{
  if (pimpl->number_of_columns_ == n_cols)
    return;

  pimpl->number_of_columns_ = n_cols;

  columns_changed.emit();
}

int Style::GetTileIconSize() const
{
  return 64;
}

int Style::GetTileWidth() const
{
  return std::max(pimpl->text_width_, 150);
}

int Style::GetTileHeight() const
{
  return std::max(GetTileIconSize() + (pimpl->text_height_ * 2) + 10,
                  GetTileIconSize() + 50 + 18); // magic design numbers.
}

int Style::GetHomeTileIconSize() const
{
  return 104;
}

int Style::GetHomeTileWidth() const
{
  return pimpl->text_width_ * 1.2;
}

int Style::GetHomeTileHeight() const
{
  return GetHomeTileIconSize() + (pimpl->text_height_ * 5);
}

int Style::GetTextLineHeight() const
{
  return pimpl->text_height_;
}


nux::BaseTexture* Style::GetDashBottomTile()
{
  return pimpl->dash_bottom_texture_.texture();
}

nux::BaseTexture* Style::GetDashBottomTileMask()
{
  return pimpl->dash_bottom_texture_mask_.texture();
}

nux::BaseTexture* Style::GetDashRightTile()
{
  return pimpl->dash_right_texture_.texture();
}

nux::BaseTexture* Style::GetDashRightTileMask()
{
  return pimpl->dash_right_texture_mask_.texture();
}

nux::BaseTexture* Style::GetDashCorner()
{
  return pimpl->dash_corner_texture_.texture();
}

nux::BaseTexture* Style::GetDashCornerMask()
{
  return pimpl->dash_corner_texture_mask_.texture();
}

nux::BaseTexture* Style::GetDashLeftEdge()
{
  return pimpl->dash_left_edge_.texture();
}

nux::BaseTexture* Style::GetDashLeftCorner()
{
  return pimpl->dash_left_corner_.texture();
}

nux::BaseTexture* Style::GetDashLeftCornerMask()
{
  return pimpl->dash_left_corner_mask_.texture();
}

nux::BaseTexture* Style::GetDashLeftTile()
{
  return pimpl->dash_left_tile_.texture();
}

nux::BaseTexture* Style::GetDashTopCorner()
{
  return pimpl->dash_top_corner_.texture();
}

nux::BaseTexture* Style::GetDashTopCornerMask()
{
  return pimpl->dash_top_corner_mask_.texture();
}

nux::BaseTexture* Style::GetDashTopTile()
{
  return pimpl->dash_top_tile_.texture();
}

nux::BaseTexture* Style::GetDashFullscreenIcon()
{
  return pimpl->dash_fullscreen_icon_.texture();
}

nux::BaseTexture* Style::GetSearchMagnifyIcon()
{
  return pimpl->search_magnify_texture_.texture();
}

nux::BaseTexture* Style::GetSearchCircleIcon()
{
  return pimpl->search_circle_texture_.texture();
}

nux::BaseTexture* Style::GetSearchCloseIcon()
{
  return pimpl->search_close_texture_.texture();
}

nux::BaseTexture* Style::GetSearchSpinIcon()
{
  return pimpl->search_spin_texture_.texture();
}

nux::BaseTexture* Style::GetGroupUnexpandIcon()
{
  return pimpl->group_unexpand_texture_.texture();
}

nux::BaseTexture* Style::GetGroupExpandIcon()
{
  return pimpl->group_expand_texture_.texture();
}

nux::BaseTexture* Style::GetStarDeselectedIcon()
{
  return pimpl->star_deselected_texture_.texture();
}

nux::BaseTexture* Style::GetStarSelectedIcon()
{
  return pimpl->star_selected_texture_.texture();
}

nux::BaseTexture* Style::GetStarHighlightIcon()
{
  return pimpl->star_highlight_texture_.texture();
}

nux::BaseTexture* Style::GetDashShine()
{
  return pimpl->dash_shine_.texture();
}

int Style::GetDashBottomTileHeight() const
{
  return 30;
}

int Style::GetDashRightTileWidth() const
{
  return 30;
}

int Style::GetVSeparatorSize() const
{
  return 1;
}

int Style::GetHSeparatorSize() const
{
  return 1;

}

int Style::GetFilterBarWidth() const
{
  return 300;
}


int Style::GetFilterBarLeftPadding() const
{
  return 5;
}

int Style::GetFilterBarRightPadding() const
{
  return 5;
}

int Style::GetDashViewTopPadding() const
{
  return 10;
}

int Style::GetSearchBarLeftPadding() const
{
  return 10;
}

int Style::GetSearchBarRightPadding() const
{
  return 10;
}

int Style::GetSearchBarHeight() const
{
  return 42;
}

int Style::GetFilterResultsHighlightRightPadding() const
{
  return 5;
}

int Style::GetFilterResultsHighlightLeftPadding() const
{
  return 5;
}

int Style::GetFilterBarTopPadding() const
{
  return 10;
}

int Style::GetFilterHighlightPadding() const
{
  return 2;
}

int Style::GetSpaceBetweenFilterWidgets() const
{
  return 12;
}

int Style::GetAllButtonHeight() const
{
  return 30;
}

int Style::GetFilterButtonHeight() const
{
  return 30;
}

int Style::GetSpaceBetweenLensAndFilters() const
{
  return 10;
}

int Style::GetFilterViewRightPadding() const
{
  return 10;
}

int Style::GetScrollbarWidth() const
{
  return 3;
}

int Style::GetCategoryHighlightHeight() const
{
  return 24;
}

int Style::GetPlacesGroupTopSpace() const
{
  return 15;
}

int Style::GetCategoryHeaderLeftPadding() const
{
  return 20;
}

int Style::GetCategorySeparatorLeftPadding() const
{
  return 15;
}

int Style::GetCategorySeparatorRightPadding() const
{
  return 15;
}

namespace
{

LazyLoadTexture::LazyLoadTexture(std::string const& filename, int size)
  : filename_(filename)
  , size_(size)
{
}

nux::BaseTexture* LazyLoadTexture::texture()
{
  if (!texture_)
    LoadTexture();
  return texture_.GetPointer();
}

void LazyLoadTexture::LoadTexture()
{
  std::string full_path = PKGDATADIR + filename_;
  glib::Object<GdkPixbuf> pixbuf;
  glib::Error error;

  pixbuf = ::gdk_pixbuf_new_from_file_at_size(full_path.c_str(), size_, size_, &error);
  if (error)
  {
    LOG_WARN(logger) << "Unable to texture " << full_path << ": " << error;
  }
  else
  {
    texture_.Adopt(nux::CreateTexture2DFromPixbuf(pixbuf, true));
  }
}

} // anon namespace

} // namespace dash
} // namespace unity
