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
 */

#include <math.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <pango/pango.h>
#include <json-glib/json-glib.h>

#include "DashStyle.h"

#define DASH_WIDGETS_FILE "/usr/share/unity/themes/dash-widgets.json"

namespace unity
{
  bool DashStyle::ReadColorSingle (JsonNode*    root,
                                   const gchar* nodeName,
                                   const gchar* memberName,
                                   double*      color)
  {
  JsonObject* object = NULL;
  JsonNode*   node   = NULL;

    if (!root || !nodeName || !memberName || !color)
      return false;

    object = json_node_get_object (root);
    node   = json_object_get_member (object, nodeName);
    object = json_node_get_object (node);

    const gchar* string = NULL;
    PangoColor   col    = {0, 0, 0};

    string = json_object_get_string_member (object, memberName);
    pango_color_parse (&col, string);
    color[R] = (double) col.red   / (double) 0xffff;
    color[G] = (double) col.green / (double) 0xffff;
    color[B] = (double) col.blue  / (double) 0xffff;

    return true;
  }

  bool DashStyle::ReadColorArray (JsonNode*    root,
                                  const gchar* nodeName,
                                  const gchar* memberName,
                                  double colors[][CHANNELS])
  {
  JsonObject*  object = NULL;
  JsonNode*    node   = NULL;
    JsonArray*   array  = NULL;
    unsigned int i      = 0;

    if (!root || !nodeName || !memberName || !colors)
      return false;

    object = json_node_get_object (root);
    node   = json_object_get_member (object, nodeName);
    object = json_node_get_object (node);
    array  = json_object_get_array_member (object, memberName);

    for (i = 0; i < json_array_get_length (array); i++)
    {
      const gchar* string = NULL;
      PangoColor   color  = {0, 0, 0};
      string = json_array_get_string_element (array, i);
      pango_color_parse (&color, string);
      colors[i][R] = (double) color.red   / (double) 0xffff;
      colors[i][G] = (double) color.green / (double) 0xffff;
      colors[i][B] = (double) color.blue  / (double) 0xffff;
    }

    return true;
  }

  bool DashStyle::ReadDoubleSingle (JsonNode*    root,
                                    const gchar* nodeName,
                                    const gchar* memberName,
                                    double*      value)
  {
  JsonObject* object = NULL;
  JsonNode*   node   = NULL;

    if (!root || !nodeName || !memberName || !value)
      return false;

    object = json_node_get_object (root);
    node   = json_object_get_member (object, nodeName);
    object = json_node_get_object (node);

    *value = json_object_get_double_member (object, memberName);

    return true;
  }

  bool DashStyle::ReadDoubleArray (JsonNode*    root,
                                   const gchar* nodeName,
                                   const gchar* memberName,
                                   double*      values)
  {
  JsonObject*  object = NULL;
  JsonNode*    node   = NULL;
    JsonArray*   array  = NULL;
    unsigned int i      = 0;

    if (!root || !nodeName || !memberName || !values)
      return false;

    object = json_node_get_object (root);
    node   = json_object_get_member (object, nodeName);
    object = json_node_get_object (node);
    array  = json_object_get_array_member (object, memberName);

    for (i = 0; i < json_array_get_length (array); i++)
      values[i] = json_array_get_double_element (array, i);

    return true;
  }

  bool DashStyle::ReadIntSingle (JsonNode*    root,
                                 const gchar* nodeName,
                                 const gchar* memberName,
                                 int*         value)
  {
  JsonObject* object = NULL;
  JsonNode*   node   = NULL;

    if (!root || !nodeName || !memberName || !value)
      return false;

    object = json_node_get_object (root);
    node   = json_object_get_member (object, nodeName);
    object = json_node_get_object (node);

    *value = json_object_get_int_member (object, memberName);

    return true;
  }

  bool DashStyle::ReadIntArray (JsonNode*    root,
                                const gchar* nodeName,
                                const gchar* memberName,
                                int*         values)
  {
  JsonObject*  object = NULL;
  JsonNode*    node   = NULL;
    JsonArray*   array  = NULL;
    unsigned int i      = 0;

    if (!root || !nodeName || !memberName || !values)
      return false;

    object = json_node_get_object (root);
    node   = json_object_get_member (object, nodeName);
    object = json_node_get_object (node);
    array  = json_object_get_array_member (object, memberName);

    for (i = 0; i < json_array_get_length (array); i++)
      values[i] = json_array_get_int_element (array, i);

    return true;
  }

  bool DashStyle::ReadModeSingle (JsonNode*    root,
                                  const gchar* nodeName,
                                  const gchar* memberName,
                                  BlendMode*   mode)
  {
  JsonObject*  object = NULL;
  JsonNode*    node   = NULL;
    const gchar* string = NULL;

    if (!root || !nodeName || !memberName || !mode)
      return false;

    object = json_node_get_object (root);
    node   = json_object_get_member (object, nodeName);
    object = json_node_get_object (node);

    string = json_object_get_string_member (object, memberName);
    if (!g_strcmp0 (string, "normal"))
      *mode = BLEND_MODE_NORMAL ;

    if (!g_strcmp0 (string, "multiply"))
      *mode = BLEND_MODE_MULTIPLY ;

    if (!g_strcmp0 (string, "screen"))
      *mode = BLEND_MODE_SCREEN ;

    return true;
  }

  bool DashStyle::ReadModeArray (JsonNode*    root,
                                 const gchar* nodeName,
                                 const gchar* memberName,
                                 BlendMode*   modes)
  {
  JsonObject*  object = NULL;
  JsonNode*    node   = NULL;
    JsonArray*   array  = NULL;
  unsigned int i      = 0;

    if (!root || !nodeName || !memberName || !modes)
      return false;

    object = json_node_get_object (root);
    node   = json_object_get_member (object, nodeName);
    object = json_node_get_object (node);
    array  = json_object_get_array_member (object, memberName);

    for (i = 0; i < json_array_get_length (array); i++)
    {
      const gchar* string = NULL;

      string = json_array_get_string_element (array, i);

      if (!g_strcmp0 (string, "normal"))
        modes[i] = BLEND_MODE_NORMAL;

      if (!g_strcmp0 (string, "multiply"))
        modes[i] = BLEND_MODE_MULTIPLY;

      if (!g_strcmp0 (string, "screen"))
        modes[i] = BLEND_MODE_SCREEN;
  }

    return true;
  }

  bool DashStyle::ReadWeightSingle (JsonNode*    root,
                                    const gchar* nodeName,
                                    const gchar* memberName,
                                    FontWeight*  weight)
  {
  JsonObject*  object = NULL;
  JsonNode*    node   = NULL;
    const gchar* string = NULL;

    if (!root || !nodeName || !memberName || !weight)
      return false;

    object = json_node_get_object (root);
    node   = json_object_get_member (object, nodeName);
    object = json_node_get_object (node);

    string = json_object_get_string_member (object, memberName);
    if (!g_strcmp0 (string, "light"))
      *weight = FONT_WEIGHT_LIGHT;

    if (!g_strcmp0 (string, "regular"))
      *weight = FONT_WEIGHT_REGULAR;

    if (!g_strcmp0 (string, "bold"))
      *weight = FONT_WEIGHT_BOLD;

    return true;
  }

  bool DashStyle::ReadWeightArray (JsonNode*    root,
                                   const gchar* nodeName,
                                   const gchar* memberName,
                                   FontWeight*  weights)
  {
  JsonObject*  object = NULL;
  JsonNode*    node   = NULL;
    JsonArray*   array  = NULL;
  unsigned int i      = 0;

    if (!root || !nodeName || !memberName || !weights)
      return false;

    object = json_node_get_object (root);
    node   = json_object_get_member (object, nodeName);
    object = json_node_get_object (node);
    array  = json_object_get_array_member (object, memberName);

    for (i = 0; i < json_array_get_length (array); i++)
    {
      const gchar* string = NULL;

      string = json_array_get_string_element (array, i);

      if (!g_strcmp0 (string, "light"))
        weights[i] = FONT_WEIGHT_LIGHT;

      if (!g_strcmp0 (string, "regular"))
        weights[i] = FONT_WEIGHT_REGULAR;

      if (!g_strcmp0 (string, "bold"))
        weights[i] = FONT_WEIGHT_BOLD;
  }

    return true;
  }

  DashStyle::DashStyle ()
  {
    JsonParser*  parser = NULL;
    GError*      error  = NULL;
    gboolean     result = FALSE;
    JsonNode*    root   = NULL;

  g_type_init ();

    parser = json_parser_new ();
    result = json_parser_load_from_file (parser, DASH_WIDGETS_FILE, &error);
    if (!result)
    {
      g_object_unref (parser);
      g_warning ("Failure: %s", error->message);
      g_error_free (error);
      UseDefaultValues ();

    return;
    }

    root = json_parser_get_root (parser); // not ref'ed

    if (JSON_NODE_TYPE (root) != JSON_NODE_OBJECT)
    {
      g_warning ("Root node is not an object, fail.  It's an: %s",
                 json_node_type_name (root));
      g_object_unref (parser);
      UseDefaultValues ();
      return;
    }

    // button-label
    ReadDoubleArray (root,
                     "button-label",
                     "border-opacity",
                     _buttonLabelBorderOpacity);

    ReadColorArray (root,
                    "button-label",
                    "border-color",
                    _buttonLabelBorderColor);

    ReadDoubleSingle (root,
                      "button-label",
                      "border-size",
                      &_buttonLabelBorderSize);

    ReadDoubleSingle (root,
                      "button-label",
                      "text-size",
                      &_buttonLabelTextSize);

    ReadColorArray (root,
                    "button-label",
                    "text-color",
                    _buttonLabelTextColor);

    ReadDoubleArray (root,
                     "button-label",
                     "text-opacity",
                     _buttonLabelTextOpacity);

    ReadColorArray (root,
                    "button-label",
                    "fill-color",
                    _buttonLabelFillColor);

    ReadDoubleArray (root,
                     "button-label",
                     "fill-opacity",
                     _buttonLabelFillOpacity);

    ReadDoubleArray (root,
                     "button-label",
                     "overlay-opacity",
                     _buttonLabelOverlayOpacity);

    ReadModeArray (root,
                   "button-label",
                   "overlay-mode",
                   _buttonLabelOverlayMode);

    ReadIntArray (root,
                  "button-label",
                  "blur-size",
                  _buttonLabelBlurSize);

    // regular-text
    ReadColorSingle (root,
                     "regular-text",
                     "text-color",
                     _regularTextColor);

    ReadDoubleSingle (root,
                      "regular-text",
                      "text-opacity",
                      &_regularTextOpacity);

    ReadDoubleSingle (root,
                      "regular-text",
                      "text-size",
                      &_regularTextSize);

    ReadModeSingle (root,
                    "regular-text",
                    "text-mode",
                    &_regularTextMode);

    ReadWeightSingle (root,
                      "regular-text",
                      "text-weight",
                      &_regularTextWeight);

    g_object_unref (parser);

    // create fallback font-options
  _defaultFontOptions = NULL;
    _defaultFontOptions = cairo_font_options_create ();
    if (cairo_font_options_status (_defaultFontOptions) == CAIRO_STATUS_SUCCESS)
    {
      cairo_font_options_set_antialias (_defaultFontOptions,
                                        CAIRO_ANTIALIAS_SUBPIXEL);
      cairo_font_options_set_subpixel_order (_defaultFontOptions,
                                             CAIRO_SUBPIXEL_ORDER_RGB);
      cairo_font_options_set_hint_style (_defaultFontOptions,
                                         CAIRO_HINT_STYLE_SLIGHT);
      cairo_font_options_set_hint_metrics (_defaultFontOptions,
                                           CAIRO_HINT_METRICS_ON);
  }
  }

  DashStyle::~DashStyle ()
  {
  if (cairo_font_options_status (_defaultFontOptions) == CAIRO_STATUS_SUCCESS)
      cairo_font_options_destroy (_defaultFontOptions);
  }

  DashStyle* DashStyle::GetDefault()
  {
    //FIXME - replace with a nice C++ singleton method
    static DashStyle* default_loader = NULL;

    if (G_UNLIKELY(!default_loader))
      default_loader = new DashStyle();

    return default_loader;
  }

  static inline double
  _align (double val)
  {
    double fract = val - (int) val;

    if (fract != 0.5f)
      return (double) ((int) val + 0.5f);
    else
      return val;
  }

  void
  DashStyle::RoundedRect (cairo_t* cr,
                          double   aspect,
                          double   x,
                          double   y,
                          double   cornerRadius,
                          double   width,
                          double   height,
                          bool     align)
  {
    // sanity check
    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS &&
        cairo_surface_get_type (cairo_get_target (cr)) != CAIRO_SURFACE_TYPE_IMAGE)
      return;

    double radius = cornerRadius / aspect;

    if (align)
    {
      // top-left, right of the corner
      cairo_move_to (cr, _align (x + radius), _align (y));

      // top-right, left of the corner
      cairo_line_to (cr, _align (x + width - radius), _align (y));

      // top-right, below the corner
      cairo_arc (cr,
                 _align (x + width - radius),
                 _align (y + radius),
                 radius,
                 -90.0f * G_PI / 180.0f,
                 0.0f * G_PI / 180.0f);

      // bottom-right, above the corner
      cairo_line_to (cr, _align (x + width), _align (y + height - radius));

      // bottom-right, left of the corner
      cairo_arc (cr,
                 _align (x + width - radius),
                 _align (y + height - radius),
                 radius,
                 0.0f * G_PI / 180.0f,
                 90.0f * G_PI / 180.0f);

      // bottom-left, right of the corner
      cairo_line_to (cr, _align (x + radius), _align (y + height));

      // bottom-left, above the corner
      cairo_arc (cr,
                 _align (x + radius),
                 _align (y + height - radius),
                 radius,
                 90.0f * G_PI / 180.0f,
                 180.0f * G_PI / 180.0f);

      // top-left, right of the corner
      cairo_arc (cr,
                 _align (x + radius),
                 _align (y + radius),
                 radius,
                 180.0f * G_PI / 180.0f,
                 270.0f * G_PI / 180.0f);
    }
    else
    {
      // top-left, right of the corner
      cairo_move_to (cr, x + radius, y);

      // top-right, left of the corner
      cairo_line_to (cr, x + width - radius, y);

      // top-right, below the corner
      cairo_arc (cr,
                 x + width - radius,
                 y + radius,
                 radius,
                 -90.0f * G_PI / 180.0f,
                 0.0f * G_PI / 180.0f);

      // bottom-right, above the corner
      cairo_line_to (cr, x + width, y + height - radius);

      // bottom-right, left of the corner
      cairo_arc (cr,
                 x + width - radius,
                 y + height - radius,
                 radius,
                 0.0f * G_PI / 180.0f,
                 90.0f * G_PI / 180.0f);

      // bottom-left, right of the corner
      cairo_line_to (cr, x + radius, y + height);

      // bottom-left, above the corner
      cairo_arc (cr,
                 x + radius,
                 y + height - radius,
                 radius,
                 90.0f * G_PI / 180.0f,
                 180.0f * G_PI / 180.0f);

      // top-left, right of the corner
      cairo_arc (cr,
                 x + radius,
                 y + radius,
                 radius,
                 180.0f * G_PI / 180.0f,
                 270.0f * G_PI / 180.0f);
    }
  }

  static inline void _blurinner (guchar* pixel,
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

  static inline void _blurrow (guchar* pixels,
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
      _blurinner (&scanline[index * channels], &zR, &zG, &zB, &zA, alpha, aprec,
      zprec);

    for (index = width - 2; index >= 0; index--)
      _blurinner (&scanline[index * channels], &zR, &zG, &zB, &zA, alpha, aprec,
      zprec);
  }

  static inline void _blurcol (guchar* pixels,
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
      _blurinner ((guchar*) &ptr[index * channels], &zR, &zG, &zB, &zA, alpha,
      aprec, zprec);

    for (index = (height - 2) * width; index >= 0; index -= width)
      _blurinner ((guchar*) &ptr[index * channels], &zR, &zG, &zB, &zA, alpha,
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
  void _expblur (guchar* pixels,
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
      _blurrow (pixels, width, height, channels, row, alpha, aprec, zprec);

    for(; col < width; col++)
      _blurcol (pixels, width, height, channels, col, alpha, aprec, zprec);

    return;
  }

  void DashStyle::Blur (cairo_t* cr, int size)
  {
    // sanity check
    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS &&
        cairo_surface_get_type (cairo_get_target (cr)) != CAIRO_SURFACE_TYPE_IMAGE)
      return;

    cairo_surface_t* surface;
    guchar*          pixels;
    guint            width;
    guint            height;
    cairo_format_t   format;

    surface = cairo_get_target (cr);

    // before we mess with the surface execute any pending drawing
    cairo_surface_flush (surface);

    pixels = cairo_image_surface_get_data (surface);
    width  = cairo_image_surface_get_width (surface);
    height = cairo_image_surface_get_height (surface);
    format = cairo_image_surface_get_format (surface);

    switch (format)
    {
      case CAIRO_FORMAT_ARGB32:
        _expblur (pixels, width, height, 4, size, 16, 7);
      break;

      case CAIRO_FORMAT_RGB24:
        _expblur (pixels, width, height, 3, size, 16, 7);
      break;

      case CAIRO_FORMAT_A8:
        _expblur (pixels, width, height, 1, size, 16, 7);
      break;

      default :
        // do nothing
      break;
    }

    // inform cairo we altered the surfaces contents
    cairo_surface_mark_dirty (surface);
  }

  void DashStyle::Star (cairo_t* cr, double size)
  {
    double outter[5][2] = {{0.0, 0.0},
                           {0.0, 0.0},
                           {0.0, 0.0},
                           {0.0, 0.0},
                           {0.0, 0.0}};
    double inner[5][2]  = {{0.0, 0.0},
                           {0.0, 0.0},
                           {0.0, 0.0},
                           {0.0, 0.0},
                           {0.0, 0.0}};
    double angle[5]     = {-90.0, -18.0, 54.0, 126.0, 198.0};
    double outterRadius = size;
    double innerRadius  = size/2.0;

    for (int i = 0; i < 5; i++)
    {
      outter[i][0] = outterRadius * cos (angle[i] * M_PI / 180.0);
      outter[i][1] = outterRadius * sin (angle[i] * M_PI / 180.0);
      inner[i][0]  = innerRadius * cos ((angle[i] + 36.0) * M_PI / 180.0);
      inner[i][1]  = innerRadius * sin ((angle[i] + 36.0) * M_PI / 180.0);
    }

    cairo_move_to (cr, outter[0][0], outter[0][1]);
    cairo_line_to (cr, inner[0][0], inner[0][1]);
    cairo_line_to (cr, outter[1][0], outter[1][1]);
    cairo_line_to (cr, inner[1][0], inner[1][1]);
    cairo_line_to (cr, outter[2][0], outter[2][1]);
    cairo_line_to (cr, inner[2][0], inner[2][1]);
    cairo_line_to (cr, outter[3][0], outter[3][1]);
    cairo_line_to (cr, inner[3][0], inner[3][1]);
    cairo_line_to (cr, outter[4][0], outter[4][1]);
    cairo_line_to (cr, inner[4][0], inner[4][1]);
  cairo_close_path (cr);
  }

  void DashStyle::UseDefaultValues ()
  {
    // button-label
    _buttonLabelBorderColor[nux::NUX_STATE_NORMAL][R]      = 0.53;
    _buttonLabelBorderColor[nux::NUX_STATE_NORMAL][G]      = 1.0;
    _buttonLabelBorderColor[nux::NUX_STATE_NORMAL][B]      = 0.66;
    _buttonLabelBorderColor[nux::NUX_STATE_ACTIVE][R]      = 1.0;
    _buttonLabelBorderColor[nux::NUX_STATE_ACTIVE][G]      = 1.0;
    _buttonLabelBorderColor[nux::NUX_STATE_ACTIVE][B]      = 1.0;
    _buttonLabelBorderColor[nux::NUX_STATE_PRELIGHT][R]    = 0.06;
    _buttonLabelBorderColor[nux::NUX_STATE_PRELIGHT][G]    = 0.13;
    _buttonLabelBorderColor[nux::NUX_STATE_PRELIGHT][B]    = 1.0;
    _buttonLabelBorderColor[nux::NUX_STATE_SELECTED][R]    = 0.07;
    _buttonLabelBorderColor[nux::NUX_STATE_SELECTED][G]    = 0.2;
    _buttonLabelBorderColor[nux::NUX_STATE_SELECTED][B]    = 0.33;
    _buttonLabelBorderColor[nux::NUX_STATE_INSENSITIVE][R] = 0.39;
    _buttonLabelBorderColor[nux::NUX_STATE_INSENSITIVE][G] = 0.26;
    _buttonLabelBorderColor[nux::NUX_STATE_INSENSITIVE][B] = 0.12;

    _buttonLabelBorderOpacity[nux::NUX_STATE_NORMAL]       = 0.5;
    _buttonLabelBorderOpacity[nux::NUX_STATE_ACTIVE]       = 0.8;
    _buttonLabelBorderOpacity[nux::NUX_STATE_PRELIGHT]     = 0.5;
    _buttonLabelBorderOpacity[nux::NUX_STATE_SELECTED]     = 0.5;
    _buttonLabelBorderOpacity[nux::NUX_STATE_INSENSITIVE]  = 0.5;

    _buttonLabelBorderSize                                 = 1.0;

    _buttonLabelTextSize                                   = 1.0;

    _buttonLabelTextColor[nux::NUX_STATE_NORMAL][R]        = 1.0;
    _buttonLabelTextColor[nux::NUX_STATE_NORMAL][G]        = 1.0;
    _buttonLabelTextColor[nux::NUX_STATE_NORMAL][B]        = 1.0;
    _buttonLabelTextColor[nux::NUX_STATE_ACTIVE][R]        = 0.0;
    _buttonLabelTextColor[nux::NUX_STATE_ACTIVE][G]        = 0.0;
    _buttonLabelTextColor[nux::NUX_STATE_ACTIVE][B]        = 0.0;
    _buttonLabelTextColor[nux::NUX_STATE_PRELIGHT][R]      = 1.0;
    _buttonLabelTextColor[nux::NUX_STATE_PRELIGHT][G]      = 1.0;
    _buttonLabelTextColor[nux::NUX_STATE_PRELIGHT][B]      = 1.0;
    _buttonLabelTextColor[nux::NUX_STATE_SELECTED][R]      = 1.0;
    _buttonLabelTextColor[nux::NUX_STATE_SELECTED][G]      = 1.0;
    _buttonLabelTextColor[nux::NUX_STATE_SELECTED][B]      = 1.0;
    _buttonLabelTextColor[nux::NUX_STATE_INSENSITIVE][R]   = 1.0;
    _buttonLabelTextColor[nux::NUX_STATE_INSENSITIVE][G]   = 1.0;
    _buttonLabelTextColor[nux::NUX_STATE_INSENSITIVE][B]   = 1.0;

    _buttonLabelTextOpacity[nux::NUX_STATE_NORMAL]         = 1.0;
    _buttonLabelTextOpacity[nux::NUX_STATE_ACTIVE]         = 1.0;
    _buttonLabelTextOpacity[nux::NUX_STATE_PRELIGHT]       = 1.0;
    _buttonLabelTextOpacity[nux::NUX_STATE_SELECTED]       = 1.0;
    _buttonLabelTextOpacity[nux::NUX_STATE_INSENSITIVE]    = 1.0;

    _buttonLabelFillColor[nux::NUX_STATE_NORMAL][R]        = 0.0;
    _buttonLabelFillColor[nux::NUX_STATE_NORMAL][G]        = 0.0;
    _buttonLabelFillColor[nux::NUX_STATE_NORMAL][B]        = 0.0;
    _buttonLabelFillColor[nux::NUX_STATE_ACTIVE][R]        = 0.0;
    _buttonLabelFillColor[nux::NUX_STATE_ACTIVE][G]        = 0.0;
    _buttonLabelFillColor[nux::NUX_STATE_ACTIVE][B]        = 0.0;
    _buttonLabelFillColor[nux::NUX_STATE_PRELIGHT][R]      = 0.0;
    _buttonLabelFillColor[nux::NUX_STATE_PRELIGHT][G]      = 0.0;
    _buttonLabelFillColor[nux::NUX_STATE_PRELIGHT][B]      = 0.0;
    _buttonLabelFillColor[nux::NUX_STATE_SELECTED][R]      = 0.0;
    _buttonLabelFillColor[nux::NUX_STATE_SELECTED][G]      = 0.0;
    _buttonLabelFillColor[nux::NUX_STATE_SELECTED][B]      = 0.0;
    _buttonLabelFillColor[nux::NUX_STATE_INSENSITIVE][R]   = 0.0;
    _buttonLabelFillColor[nux::NUX_STATE_INSENSITIVE][G]   = 0.0;
    _buttonLabelFillColor[nux::NUX_STATE_INSENSITIVE][B]   = 0.0;

    _buttonLabelFillOpacity[nux::NUX_STATE_NORMAL]         = 0.0;
    _buttonLabelFillOpacity[nux::NUX_STATE_ACTIVE]         = 0.0;
    _buttonLabelFillOpacity[nux::NUX_STATE_PRELIGHT]       = 0.0;
    _buttonLabelFillOpacity[nux::NUX_STATE_SELECTED]       = 0.0;
    _buttonLabelFillOpacity[nux::NUX_STATE_INSENSITIVE]    = 0.0;

    _buttonLabelOverlayOpacity[nux::NUX_STATE_NORMAL]      = 0.0;
    _buttonLabelOverlayOpacity[nux::NUX_STATE_ACTIVE]      = 0.3;
    _buttonLabelOverlayOpacity[nux::NUX_STATE_PRELIGHT]    = 0.0;
    _buttonLabelOverlayOpacity[nux::NUX_STATE_SELECTED]    = 0.0;
    _buttonLabelOverlayOpacity[nux::NUX_STATE_INSENSITIVE] = 0.0;

    _buttonLabelOverlayMode[nux::NUX_STATE_NORMAL]         = BLEND_MODE_NORMAL;
    _buttonLabelOverlayMode[nux::NUX_STATE_ACTIVE]         = BLEND_MODE_NORMAL;
    _buttonLabelOverlayMode[nux::NUX_STATE_PRELIGHT]       = BLEND_MODE_NORMAL;
    _buttonLabelOverlayMode[nux::NUX_STATE_SELECTED]       = BLEND_MODE_NORMAL;
    _buttonLabelOverlayMode[nux::NUX_STATE_INSENSITIVE]    = BLEND_MODE_NORMAL;

    _buttonLabelBlurSize[nux::NUX_STATE_NORMAL]            = 0;
    _buttonLabelBlurSize[nux::NUX_STATE_ACTIVE]            = 5;
    _buttonLabelBlurSize[nux::NUX_STATE_PRELIGHT]          = 0;
    _buttonLabelBlurSize[nux::NUX_STATE_SELECTED]          = 0;
    _buttonLabelBlurSize[nux::NUX_STATE_INSENSITIVE]       = 0;

    // regular-text
    _regularTextColor[R]   = 1.0;
    _regularTextColor[G]   = 1.0;
    _regularTextColor[B]   = 1.0;
    _regularTextOpacity    = 1.0;
    _regularTextSize       = 13.0;
    _regularTextMode       = BLEND_MODE_NORMAL;
    _regularTextWeight     = FONT_WEIGHT_LIGHT;
  }

  void DashStyle::ButtonOutlinePath (cairo_t* cr)
  {
    double   x = 2.0;
    double   y = 2.0;
    double   w = cairo_image_surface_get_width (cairo_get_target (cr)) - 4.0;
    double   h = cairo_image_surface_get_height (cairo_get_target (cr)) - 4.0;

  // - these absolute values are the "cost" of getting only a SVG from design
  // and not a generic formular how to approximate the curve-shape, thus
  // the smallest possible button-size is 22.18x24.0
  // - also making this align to the pixel-grid with the relative coordinates
  // is impossible
  double width  = w - 22.18;
  double height = h - 24.0;

  // top right
    cairo_move_to (cr, x + width + 22.18, y + 12.0);
    cairo_rel_curve_to (cr, -0.103, -4.355, -1.037, -7.444, -2.811, -9.267);
    cairo_rel_curve_to (cr, -1.722, -1.823, -4.531, -2.735, -8.28, -2.735);

  // top
    cairo_rel_line_to (cr, -width, 0.0);

  // top left
    cairo_rel_curve_to (cr, -3.748, 0.0, -6.507, 0.912, -8.279, 2.735);
    cairo_rel_curve_to (cr, -1.773, 1.822, -2.708, 4.911, -2.811, 9.267);

  // left
    cairo_rel_line_to (cr, 0.0, height);

  // bottom left
    cairo_rel_curve_to (cr, 0.103, 4.355, 1.037, 7.444, 2.811, 9.267);
    cairo_rel_curve_to (cr, 1.772, 1.823, 4.531, 2.735, 8.28, 2.735);

  // bottom
    cairo_rel_line_to (cr, width, 0.0);

  // bottom right
    cairo_rel_curve_to (cr, 3.748, 0.0, 6.507, -0.912, 8.279, -2.735);
    cairo_rel_curve_to (cr, 1.773, -1.822, 2.708, -4.911, 2.811, -9.267);

  // right
    cairo_close_path (cr);
  }

  void DashStyle::GetTextExtents (int& width,
                                  int& height,
                                  int  maxWidth,
                                  int  maxHeight,
                                  const std::string text)
  {
    cairo_surface_t*      surface  = NULL;
    cairo_t*              cr       = NULL;
    PangoLayout*          layout   = NULL;
    PangoFontDescription* desc     = NULL;
    PangoContext*         pangoCtx = NULL;
    PangoRectangle        logRect  = {0, 0, 0, 0};
    int                   dpi      = 0;
  char*                 fontName = NULL;
    GdkScreen*            screen   = gdk_screen_get_default();  // is not ref'ed
    GtkSettings*          settings = gtk_settings_get_default();// is not ref'ed

    surface = cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1);
    cr = cairo_create(surface);
    if (!screen)
      cairo_set_font_options(cr, _defaultFontOptions);
    else
      cairo_set_font_options(cr, gdk_screen_get_font_options(screen));
    layout = pango_cairo_create_layout(cr);

    g_object_get(settings, "gtk-font-name", &fontName, NULL);
    if (!fontName)
      desc = pango_font_description_from_string("Sans 10");
    else
    {
      desc = pango_font_description_from_string(fontName);
    g_free (fontName);
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
      pango_cairo_context_set_font_options(pangoCtx, _defaultFontOptions);
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
    pango_layout_get_extents(layout, NULL, &logRect);

    width  = (logRect.x + logRect.width) / PANGO_SCALE;
    height = (logRect.y + logRect.height) / PANGO_SCALE;

    // clean up
    pango_font_description_free(desc);
    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
  }

  void DashStyle::Text (cairo_t*    cr,
                        double      size,
                        double*     color,
                        double      opacity,
                        std::string label)
  {
  double                x          = 0.0;
    double                y          = 0.0;
    int                   w          = 0;
    int                   h          = 0;
    int                   textWidth  = 0;
  int                   textHeight = 0;
    PangoLayout*          layout     = NULL;
    PangoFontDescription* desc       = NULL;
    PangoContext*         pangoCtx   = NULL;
    int                   dpi        = 0;
    GdkScreen*            screen     = gdk_screen_get_default();   // not ref'ed
    GtkSettings*          settings   = gtk_settings_get_default(); // not ref'ed
    gchar*                fontName   = NULL;

    w = cairo_image_surface_get_width (cairo_get_target (cr));
    h = cairo_image_surface_get_height (cairo_get_target (cr));
    GetTextExtents (textWidth, textHeight, w, h, label);
  x = (w - textWidth) / 2.0;
    y = (h - textHeight) / 2.0;

    if (!screen)
      cairo_set_font_options(cr, _defaultFontOptions);
    else
      cairo_set_font_options(cr, gdk_screen_get_font_options(screen));
    layout = pango_cairo_create_layout(cr);

    g_object_get(settings, "gtk-font-name", &fontName, NULL);
    if (!fontName)
      desc = pango_font_description_from_string("Sans 10");
    else
      desc = pango_font_description_from_string(fontName);

    PangoWeight weight;
  switch (_regularTextWeight)
    {
      case FONT_WEIGHT_REGULAR:
        weight = PANGO_WEIGHT_NORMAL;
      break;

      case FONT_WEIGHT_LIGHT:
        weight = PANGO_WEIGHT_LIGHT;
      break;

      case FONT_WEIGHT_BOLD:
        weight = PANGO_WEIGHT_BOLD;
      break;
  }
    pango_font_description_set_weight(desc, weight);

    pango_layout_set_font_description(layout, desc);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
    pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);

    pango_layout_set_markup(layout, label.c_str(), -1);
    pango_layout_set_width(layout, w * PANGO_SCALE);

    pango_layout_set_height(layout, h);
    pangoCtx = pango_layout_get_context(layout);  // is not ref'ed

    if (!screen)
      pango_cairo_context_set_font_options(pangoCtx, _defaultFontOptions);
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
  cairo_set_source_rgba (cr, color[R], color[G], color[B], opacity);
    pango_layout_context_changed(layout);
    cairo_move_to (cr, x, y);
    pango_cairo_show_layout(cr, layout);

    // clean up
    pango_font_description_free(desc);
    g_object_unref(layout);
    g_free(fontName);
  }

  bool DashStyle::Button (cairo_t* cr, nux::State state, std::string label)
  {
  // sanity checks
    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
      return false;

    if (cairo_surface_get_type (cairo_get_target (cr)) != CAIRO_SURFACE_TYPE_IMAGE)
      return false;

    ButtonOutlinePath (cr);
    if (_buttonLabelFillOpacity[state] != 0.0)
    {
      cairo_set_source_rgba (cr,
                             _buttonLabelFillColor[state][R],
                             _buttonLabelFillColor[state][G],
                             _buttonLabelFillColor[state][B],
                             _buttonLabelFillOpacity[state]);
      cairo_fill_preserve (cr);
    }
    cairo_set_source_rgba (cr,
                           _buttonLabelBorderColor[state][R],
                           _buttonLabelBorderColor[state][G],
                           _buttonLabelBorderColor[state][B],
                           _buttonLabelBorderOpacity[state]);
    cairo_set_line_width (cr, _buttonLabelBorderSize);
    cairo_stroke (cr);
  Text (cr,
        _buttonLabelTextSize,
        _buttonLabelTextColor[state],
        _buttonLabelTextOpacity[state],
        label);

    return true;
  }

  bool DashStyle::Rating (cairo_t* cr, double rating)
  {
  // sanity checks
    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
      return false;

    if (cairo_surface_get_type (cairo_get_target (cr)) != CAIRO_SURFACE_TYPE_IMAGE)
      return false;

  if (rating < 0.0 || rating > 1.0)
      return false;

    double w = cairo_image_surface_get_width (cairo_get_target (cr));
    double h = cairo_image_surface_get_height (cairo_get_target (cr));
    double size = w / 5;
    double radius = .85 * size / 2.0;

  cairo_save (cr);

  cairo_pattern_t* pattern = NULL;
    pattern = cairo_pattern_create_linear (0.0, 0.0, w, 0.0);
  cairo_pattern_add_color_stop_rgba (pattern, 0.0, 1.0, 1.0, 1.0, 1.0);
    cairo_pattern_add_color_stop_rgba (pattern, rating, 1.0, 1.0, 1.0, 1.0);
    cairo_pattern_add_color_stop_rgba (pattern, rating + 0.01, 1.0, 1.0, 1.0, 0.2);
    cairo_pattern_add_color_stop_rgba (pattern, 1.0, 1.0, 1.0, 1.0, 0.2);
  cairo_set_source (cr, pattern);

    cairo_translate (cr, size / 2.0, h / 2.0);
    Star (cr, radius);
    cairo_translate (cr, size, 0.0);
    Star (cr, radius);
    cairo_translate (cr, size, 0.0);
    Star (cr, radius);
    cairo_translate (cr, size, 0.0);
    Star (cr, radius);
    cairo_translate (cr, size, 0.0);
    Star (cr, radius);

  cairo_fill_preserve (cr);
    cairo_pattern_destroy (pattern);
  cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
    cairo_stroke (cr);
    cairo_restore (cr);

    return true;
  }

}
