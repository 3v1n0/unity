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

#ifndef DASH_STYLE_H
#define DASH_STYLE_H

#include "Nux/Nux.h"

#include <json-glib/json-glib.h>
#include <cairo.h>

#define STATES   5
#define CHANNELS 3
#define R        0
#define G        1
#define B        2

namespace unity
{
  class DashStyle
  {
    public:

      typedef enum {
        STOCK_ICON_CHECKMARK = 0,
        STOCK_ICON_CROSS,
        STOCK_ICON_GRID_VIEW,
        STOCK_ICON_FLOW_VIEW,
        STOCK_ICON_STAR
      } StockIcon;

      typedef enum {
        ORIENTATION_UP = 0,
        ORIENTATION_DOWN,
        ORIENTATION_LEFT,
        ORIENTATION_RIGHT
      } Orientation;

      typedef enum {
        BLEND_MODE_NORMAL = 0,
        BLEND_MODE_MULTIPLY,
        BLEND_MODE_SCREEN
    } BlendMode;

      typedef enum {
        FONT_WEIGHT_LIGHT = 0,
        FONT_WEIGHT_REGULAR,
        FONT_WEIGHT_BOLD
    } FontWeight;

      DashStyle ();
      ~DashStyle ();

      static DashStyle* GetDefault();

    virtual bool Button (cairo_t* cr, nux::State state, std::string label);
    virtual bool Rating (cairo_t* cr, double rating);

    void Blur (cairo_t* cr, int size);

    void RoundedRect (cairo_t* cr,
                      double   aspect,
                      double   x,
                      double   y,
                      double   cornerRadius,
                      double   width,
                      double   height,
                      bool     align);

    private:
      void UseDefaultValues ();

      bool ReadColorSingle (JsonNode*    root,
                            const gchar* nodeName,
                            const gchar* memberName,
                            double*      color);

      bool ReadColorArray (JsonNode*    root,
                           const gchar* nodeName,
                           const gchar* memberName,
                           double       colors[][CHANNELS]);

      bool ReadDoubleSingle (JsonNode*    root,
                             const gchar* nodeName,
                             const gchar* memberName,
                             double*      value);

      bool ReadDoubleArray (JsonNode*    root,
                            const gchar* nodeName,
                            const gchar* memberName,
                            double*      values);

      bool ReadIntSingle (JsonNode*    root,
                          const gchar* nodeName,
                          const gchar* memberName,
                          int*         value);

      bool ReadIntArray (JsonNode*    root,
                         const gchar* nodeName,
                         const gchar* memberName,
                         int*         values);

      bool ReadModeSingle (JsonNode*    root,
                           const gchar* nodeName,
                           const gchar* memberName,
                           BlendMode*   mode);

      bool ReadModeArray (JsonNode*    root,
                          const gchar* nodeName,
                          const gchar* memberName,
                          BlendMode*   modes);

      bool ReadWeightSingle (JsonNode*    root,
                             const gchar* nodeName,
                             const gchar* memberName,
                             FontWeight*  weight);

      bool ReadWeightArray (JsonNode*    root,
                            const gchar* nodeName,
                            const gchar* memberName,
                            FontWeight*  weights);

      void Star (cairo_t* cr, double size);

      void GetTextExtents (int& width,
                           int& height,
                           int  maxWidth,
                           int  maxHeight,
                           const std::string text);

      void Text (cairo_t*    cr,
                 double      size,
                 double*     color,
                 double      opacity,
                 std::string label);

      void ButtonOutlinePath (cairo_t* cr);

    private:
      cairo_font_options_t* _defaultFontOptions;

      double                _buttonLabelBorderColor[STATES][CHANNELS];
      double                _buttonLabelBorderOpacity[STATES];
      double                _buttonLabelBorderSize;
      double                _buttonLabelTextSize;
      double                _buttonLabelTextColor[STATES][CHANNELS];
      double                _buttonLabelTextOpacity[STATES];
      double                _buttonLabelFillColor[STATES][CHANNELS];
      double                _buttonLabelFillOpacity[STATES];
      double                _buttonLabelOverlayOpacity[STATES];
      BlendMode             _buttonLabelOverlayMode[STATES];
      int                   _buttonLabelBlurSize[STATES];

      double                _regularTextColor[CHANNELS];
      double                _regularTextOpacity;
      double                _regularTextSize;
      BlendMode             _regularTextMode;
      FontWeight            _regularTextWeight;
  };
}

#endif // DASH_STYLE_H
