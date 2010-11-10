#ifndef TOOLTIP_H
#define TOOLTIP_H

#include "Nux/Nux.h"
#include "Nux/BaseWindow.h"
#include "NuxGraphics/GraphicsEngine.h"
#include "Nux/TextureArea.h"
#include "NuxImage/CairoGraphics.h"
#include "StaticCairoText.h"

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
  class VLayout;
  class HLayout;
  class SpaceLayout;
  
  class Tooltip : public BaseWindow
  {
    NUX_DECLARE_OBJECT_TYPE (Tooltip, BaseWindow);
  public:
    Tooltip ();

    ~Tooltip ();

    long ProcessEvent (IEvent& iEvent,
      long    traverseInfo,
      long    processEventInfo);

    void Draw (GraphicsEngine& gfxContext,
      bool             forceDraw);

    void DrawContent (GraphicsEngine& gfxContext,
      bool             forceDraw);

    void SetText (NString text);

  private:
    void RecvCairoTextChanged (StaticCairoText& cairo_text);

    void PreLayoutManagement ();

    long PostLayoutManagement (long layoutResult);

    void PositionChildLayout (float offsetX,
      float offsetY);

    void LayoutWindowElements ();

    void NotifyConfigurationChange (int width,
      int height);

    //nux::CairoGraphics*   _cairo_graphics;
    int                   _anchorX;
    int                   _anchorY;
    nux::NString          _labelText;
    int                   _dpiX;
    int                   _dpiY;

    cairo_font_options_t* _fontOpts;

    nux::StaticCairoText* _tooltip_text;
    nux::BaseTexture*     _texture_bg;
    nux::BaseTexture*     _texture_mask;
    nux::BaseTexture*     _texture_outline;

    float _anchor_width;
    float _anchor_height;
    float _corner_radius;
    float _padding;
    nux::HLayout *_hlayout;
    VLayout *_vlayout;
    nux::SpaceLayout *_left_space;  //!< Space from the left of the widget to the left of the text.
    nux::SpaceLayout *_right_space; //!< Space from the right of the text to the right of the widget.
    nux::SpaceLayout *_top_space;  //!< Space from the left of the widget to the left of the text.
    nux::SpaceLayout *_bottom_space; //!< Space from the right of the text to the right of the widget.

    bool _cairo_text_has_changed;
    void UpdateTexture ();
  };
}

#endif // TOOLTIP_H

