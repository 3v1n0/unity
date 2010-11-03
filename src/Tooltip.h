#ifndef TOOLTIP_H
#define TOOLTIP_H

#include "Nux/Nux.h"
#include "Nux/BaseWindow.h"
#include "NuxGraphics/OpenGLEngine.h"
#include "NuxGraphics/GfxEventsX11.h"
#include "NuxGraphics/GfxSetupX11.h"
#include "Nux/TextureArea.h"
#include "NuxImage/CairoGraphics.h"

#include <pango/pango.h>
#include <pango/pangocairo.h>

#if defined(NUX_OS_LINUX)
#include <X11/Xlib.h>
#endif

#define ANCHOR_WIDTH         10.0f
#define ANCHOR_HEIGHT        18.0f
#define HIGH_LIGHT_Y         -30.0f
#define HIGH_LIGHT_MIN_WIDTH 200.0f 
#define RADIUS               5.0f
#define BLUR_INTENSITY       8
#define LINE_WIDTH           1.0f
#define PADDING_SIZE         1
#define H_MARGIN             30
#define V_MARGIN             4
#define FONT_FACE            "Ubuntu 13"

namespace nux
{
  class Tooltip : public BaseWindow
  {
    NUX_DECLARE_OBJECT_TYPE (Tooltip, BaseWindow);
    public:
      Tooltip ();

      ~Tooltip ();

      long ProcessEvent (IEvent& iEvent,
                         long    traverseInfo,
                         long    processEventInfo);

      void Draw (GraphicsContext& gfxContext,
                 bool             forceDraw);

      void DrawContent (GraphicsContext& gfxContext,
                        bool             forceDraw);

      void SetText (NString text);

    protected:
      void PreLayoutManagement ();

      long PostLayoutManagement (long layoutResult);

      void PositionChildLayout (float offsetX,
                                float offsetY);

      void LayoutWindowElements ();

      void NotifyConfigurationChange (int width,
                                      int height);
                                      
      nux::CairoGraphics*   _cairo_graphics;
      nux::BaseTexture*     _texture2D;
      int                   _anchorX;
      int                   _anchorY;
      nux::NString          _labelText;
      int                   _dpiX;
      int                   _dpiY;
      cairo_font_options_t* _fontOpts;

    private:
      void UpdateTexture ();

      void ComputeFullMaskPath (cairo_t* cr,
                                gfloat   anchor_width,
                                gfloat   anchor_height,
                                gint     width,
                                gint     height,
                                gint     upper_size,
                                gfloat   radius,
                                guint    pad);

      void DrawDraw (cairo_t* cr,
                     gboolean outline,
                     gfloat   line_width,
                     gfloat*  rgba,
                     gboolean negative,
                     gboolean stroke);

      void DrawTintDotHighlight (cairo_t* cr,
                                 gint     width,
                                 gint     height,
                                 gfloat   hl_x,
                                 gfloat   hl_y,
                                 gfloat   hl_size,
                                 gfloat*  rgba_tint,
                                 gfloat*  rgba_hl);

      void DrawOutlineShadow (cairo_t* cr,
                              gint     width,
                              gint     height,
                              gfloat   anchor_width,
                              gfloat   anchor_height,
                              gint     upper_size,
                              gfloat   corner_radius,
                              guint    blur_coeff,
                              gfloat*  rgba_shadow,
                              gfloat   line_width,
                              gint     padding_size,
                              gfloat*  rgba_line);

      void ComputeOutline (cairo_t* cr,
                           gfloat   line_width,
                           gfloat*  rgba_line,
                           gint     width,
                           gfloat   anchor_width,
                           gfloat   corner_radius,
                           gint     padding_size);

      void DrawMask (cairo_t* cr,
                     gint     width,
                     gint     height,
                     gfloat   radius,
                     guint    shadow_radius,
                     gfloat   anchor_width,
                     gfloat   anchor_height,
                     gint     upper_size,
                     gboolean negative,
                     gboolean outline,
                     gfloat   line_width,
                     gint     padding_size,
                     gfloat*  rgba);

      void GetDPI ();

      void GetTextExtents (char* font,
                           int*  width,
                           int*  height);

      void DrawLabel (cairo_t* cr,
                      gint     width,
                      gint     height,
                      gfloat*  rgba);
  };
}

#endif // TOOLTIP_H

