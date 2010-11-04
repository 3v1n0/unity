#include "Nux/Nux.h"
#include "Nux/VLayout.h"
#include "Nux/HLayout.h"
#include "Nux/WindowThread.h"
#include "Nux/WindowCompositor.h"
#include "Nux/BaseWindow.h"
#include "Nux/Button.h"
#include "NuxGraphics/OpenGLEngine.h"
#include "NuxGraphics/GfxEventsX11.h"
#include "NuxGraphics/GfxSetupX11.h"
#include "Nux/TextureArea.h"
#include "NuxImage/CairoGraphics.h"

#include "Tooltip.h"

namespace nux
{
  NUX_IMPLEMENT_OBJECT_TYPE (Tooltip);
  
  void
  DrawCairo (cairo_t* cr,
             gboolean outline,
             gfloat   line_width,
             gfloat*  rgba,
             gboolean negative,
             gboolean stroke)
  {
    // prepare drawing
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

    // actually draw the mask
    if (outline)
      {
        cairo_set_line_width (cr, line_width);
        cairo_set_source_rgba (cr, rgba[0], rgba[1], rgba[2], rgba[3]);
      }
    else
      {
        if (negative)
          cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
        else
          cairo_set_source_rgba (cr, 0.0f, 0.0f, 0.0f, 0.0f);
      }

    // stroke or fill?
    if (stroke)
      cairo_stroke_preserve (cr);
    else
      cairo_fill_preserve (cr);
  }

  void
  nux::Tooltip::ComputeFullMaskPath (cairo_t* cr,
                                gfloat   anchor_width,
                                gfloat   anchor_height,
                                gint     width,
                                gint     height,
                                gint     upper_size,
                                gfloat   radius,
                                guint    pad)
  {
    //     0  1        2  3
    //     +--+--------+--+
    //     |              |
    //     + 14           + 4
    //     |              |
    //     |              |
    //     |              |
    //     + 13           |
    //    /               |
    //   /                |
    //  + 12              |
    //   \                |
    //    \               |
    //  11 +              |
    //     |              |
    //     |              |
    //     |              |
    //  10 +              + 5
    //     |              |
    //     +--+--------+--+ 6
    //     9  8        7

    gfloat padding    = pad;
    float  ZEROPOINT5 = 0.5f;
  
    gfloat HeightToAnchor = 0.0f;
    HeightToAnchor = ((gfloat) height - 2.0f * radius - anchor_height -2*padding) / 2.0f;
    if (HeightToAnchor < 0.0f)
    {
      g_warning ("Anchor-height and corner-radius a higher than whole texture!");
      return;
    }

    //gint dynamic_size = height - 2*radius - 2*padding - anchor_height;
    //gint upper_dynamic_size = upper_size;
    //gint lower_dynamic_size = dynamic_size - upper_dynamic_size;
  
    if (upper_size >= 0)
      {
        if(upper_size > height - 2.0f * radius - anchor_height -2 * padding)
          {
            //g_warning ("[_compute_full_mask_path] incorrect upper_size value");
            HeightToAnchor = 0;
          }
        else
          {
            HeightToAnchor = height - 2.0f * radius - anchor_height -2 * padding - upper_size;
          }
      }
    else
      {
        HeightToAnchor = (height - 2.0f * radius - anchor_height -2*padding) / 2.0f;
      }

    cairo_translate (cr, -0.5f, -0.5f);
    
    // create path
    cairo_move_to (cr, padding + anchor_width + radius + ZEROPOINT5, padding + ZEROPOINT5); // Point 1
    cairo_line_to (cr, width - padding - radius, padding + ZEROPOINT5);   // Point 2
    cairo_arc (cr,
               width  - padding - radius + ZEROPOINT5,
               padding + radius + ZEROPOINT5,
               radius,
               -90.0f * G_PI / 180.0f,
               0.0f * G_PI / 180.0f);   // Point 4
    cairo_line_to (cr,
                   (gdouble) width - padding + ZEROPOINT5,
                   (gdouble) height - radius - padding + ZEROPOINT5); // Point 5
    cairo_arc (cr,
               (gdouble) width - padding - radius + ZEROPOINT5,
               (gdouble) height - padding - radius + ZEROPOINT5,
               radius,
               0.0f * G_PI / 180.0f,
               90.0f * G_PI / 180.0f);  // Point 7
    cairo_line_to (cr,
                   anchor_width + padding + radius + ZEROPOINT5,
                   (gdouble) height - padding + ZEROPOINT5); // Point 8

    cairo_arc (cr,
               anchor_width + padding + radius + ZEROPOINT5,
               (gdouble) height - padding - radius,
               radius,
               90.0f * G_PI / 180.0f,
               180.0f * G_PI / 180.0f); // Point 10

    cairo_line_to (cr,
                   padding + anchor_width + ZEROPOINT5,
                   (gdouble) height - padding - radius - HeightToAnchor + ZEROPOINT5 );  // Point 11
    cairo_line_to (cr,
                   padding + ZEROPOINT5,
                   (gdouble) height - padding - radius - HeightToAnchor - anchor_height / 2.0f + ZEROPOINT5); // Point 12
    cairo_line_to (cr,
                   padding + anchor_width + ZEROPOINT5,
                   (gdouble) height - padding - radius - HeightToAnchor - anchor_height + ZEROPOINT5);  // Point 13
  
    cairo_line_to (cr, padding + anchor_width + ZEROPOINT5, 
                   padding + radius  + ZEROPOINT5);  // Point 14
    cairo_arc (cr,
               padding + anchor_width + radius + ZEROPOINT5,
               padding + radius + ZEROPOINT5,
               radius,
               180.0f * G_PI / 180.0f,
               270.0f * G_PI / 180.0f);

    cairo_close_path (cr);
  }

  void
  ComputeMask (cairo_t* cr)
  {
    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_fill_preserve (cr);
  }

  void
  nux::Tooltip::ComputeOutline (cairo_t* cr,
                           gfloat   line_width,
                           gfloat*  rgba_line,
                           gint     width,
                           gfloat   anchor_width,
                           gfloat   corner_radius,
                           gint     padding_size)
  {
    cairo_pattern_t* pattern = NULL;
    double           offset  = 0.0;

    if (width == 0)
    {
      g_warning ("%s(): passed in width is 0!", G_STRFUNC);
      return;
    }

    offset = ((double) padding_size + anchor_width) / (double) width;

    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_line_width (cr, line_width);

    pattern = cairo_pattern_create_linear (0.0, 0.0, (double) width, 0.0);
    cairo_pattern_add_color_stop_rgba (pattern,
                                       0.0,
                                       rgba_line[0],
                                       rgba_line[1],
                                       rgba_line[2],
                                       0.9);
    cairo_pattern_add_color_stop_rgba (pattern,
                                       offset,
                                       rgba_line[0],
                                       rgba_line[1],
                                       rgba_line[2],
                                       0.9);
    cairo_pattern_add_color_stop_rgba (pattern,
                                       offset + 0.0125,
                                       rgba_line[0],
                                       rgba_line[1],
                                       rgba_line[2],
                                       0.6);
    cairo_pattern_add_color_stop_rgba (pattern,
                                       1.0,
                                       rgba_line[0],
                                       rgba_line[1],
                                       rgba_line[2],
                                       0.6);

    cairo_set_source (cr, pattern);
    cairo_stroke (cr);
    cairo_pattern_destroy (pattern);
  }

  void
  Tooltip::DrawTintDotHighlight (cairo_t* cr,
                                      gint     width,
                                      gint     height,
                                      gfloat   hl_x,
                                      gfloat   hl_y,
                                      gfloat   hl_size,
                                      gfloat*  rgba_tint,
                                      gfloat*  rgba_hl)
  {
    cairo_surface_t* dots_surf    = NULL;
    cairo_t*         dots_cr      = NULL;
    cairo_pattern_t* dots_pattern = NULL;
    cairo_pattern_t* hl_pattern   = NULL;

    // create context for dot-pattern
    dots_surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 4, 4);
    dots_cr = cairo_create (dots_surf);

    // clear normal context
    cairo_scale (cr, 1.0f, 1.0f);
    cairo_set_source_rgba (cr, 0.0f, 0.0f, 0.0f, 0.0f);
    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint (cr);

    // prepare drawing for normal context
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

    // create path in normal context
    ComputeFullMaskPath (cr,
                         ANCHOR_WIDTH,
                         ANCHOR_HEIGHT,
                         width,
                         height,
                         -1,
                         RADIUS,
                         PADDING_SIZE);
    //cairo_rectangle (cr, 0.0f, 0.0f, (gdouble) width, (gdouble) height);  

    // fill path of normal context with tint
    cairo_set_source_rgba (cr,
                           rgba_tint[0],
                           rgba_tint[1],
                           rgba_tint[2],
                           rgba_tint[3]);
    cairo_fill_preserve (cr);

    // create pattern in dot-context
    cairo_set_operator (dots_cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint (dots_cr);
    cairo_scale (dots_cr, 1.0f, 1.0f);
    cairo_set_operator (dots_cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba (dots_cr,
                           rgba_hl[0],
                           rgba_hl[1],
                           rgba_hl[2],
                           rgba_hl[3]);
    cairo_rectangle (dots_cr, 0.0f, 0.0f, 1.0f, 1.0f);
    cairo_fill (dots_cr);
    cairo_rectangle (dots_cr, 2.0f, 2.0f, 1.0f, 1.0f);
    cairo_fill (dots_cr);

    dots_pattern = cairo_pattern_create_for_surface (dots_surf);

    // fill path of normal context with dot-pattern
    // FIXME: using the path from ComputeFullMaskPath() and not a plain rect.
    // path triggers a bug in cairo (yet to be filed), so repeating of the dot-
    // pattern produces wrong pattern in the end, a viable work-around still
    // needs to be thought up
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    cairo_set_source (cr, dots_pattern);
    cairo_pattern_set_extend (dots_pattern, CAIRO_EXTEND_REPEAT);
    cairo_fill_preserve (cr);
    cairo_pattern_destroy (dots_pattern);
    cairo_surface_destroy (dots_surf);
    cairo_destroy (dots_cr);

    // draw highlight
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    hl_pattern = cairo_pattern_create_radial (hl_x,
                                              hl_y,
                                              0.0f,
                                              hl_x,
                                              hl_y,
                                              hl_size);
    cairo_pattern_add_color_stop_rgba (hl_pattern,
                                       0.0f,
                                       1.0f,
                                       1.0f,
                                       1.0f,
                                       0.75f);
    cairo_pattern_add_color_stop_rgba (hl_pattern,
                                       1.0f,
                                       1.0f,
                                       1.0f,
                                       1.0f,
                                       0.0f);
    cairo_set_source (cr, hl_pattern);
    cairo_fill (cr);
    cairo_pattern_destroy (hl_pattern);
  }

  void
  Tooltip::DrawMask (cairo_t* cr,
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
                          gfloat*  rgba)
  {
    ComputeFullMaskPath (cr,
                         anchor_width,
                         anchor_height,
                         width,
                         height,
                         upper_size,
                         radius,
                         padding_size);
  }

  void
  Tooltip::GetDPI ()
  {
#if defined(NUX_OS_LINUX)
    Display* display     = NULL;
    int      screen      = 0;
    double   dpyWidth    = 0.0;
    double   dpyHeight   = 0.0;
    double   dpyWidthMM  = 0.0;
    double   dpyHeightMM = 0.0;
    double   dpiX        = 0.0;
    double   dpiY        = 0.0;

    display = XOpenDisplay (NULL);
    screen = DefaultScreen (display);

    dpyWidth    = (double) DisplayWidth (display, screen);
    dpyHeight   = (double) DisplayHeight (display, screen);
    dpyWidthMM  = (double) DisplayWidthMM (display, screen);
    dpyHeightMM = (double) DisplayHeightMM (display, screen);

    dpiX = dpyWidth * 25.4 / dpyWidthMM;
    dpiY = dpyHeight * 25.4 / dpyHeightMM;

    _dpiX = (int) (dpiX + 0.5);
    _dpiY = (int) (dpiY + 0.5);

    XCloseDisplay (display);
#elif defined(NUX_OS_WINDOWS)
    _dpiX = 72;
    _dpiY = 72;
#endif
  }

  void
  Tooltip::GetTextExtents (char* font,
                                int*  width,
                                int*  height)
  {
    cairo_surface_t*      surface  = NULL;
    cairo_t*              cr       = NULL;
    PangoLayout*          layout   = NULL;
    PangoFontDescription* desc     = NULL;
    PangoContext*         pangoCtx = NULL;
    PangoRectangle        logRect  = {0, 0, 0, 0};

    // sanity check
    if (!font || !width || !height)
      return;

    surface = cairo_image_surface_create (CAIRO_FORMAT_A1, 1, 1);
    cr = cairo_create (surface);
    layout = pango_cairo_create_layout (cr);
    desc = pango_font_description_from_string (font);
    pango_font_description_set_weight (desc, PANGO_WEIGHT_NORMAL);
    pango_layout_set_font_description (layout, desc);
    pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
    pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
    pango_layout_set_text (layout, _labelText.GetTCharPtr(), -1);
    pangoCtx = pango_layout_get_context (layout); // is not ref'ed
    pango_cairo_context_set_font_options (pangoCtx, _fontOpts);
    pango_cairo_context_set_resolution (pangoCtx, _dpiX);
    pango_layout_context_changed (layout);
    pango_layout_get_extents (layout, NULL, &logRect);

    *width  = logRect.width / PANGO_SCALE;
    *height = logRect.height / PANGO_SCALE;

    // clean up
    pango_font_description_free (desc);
    g_object_unref (layout);
    cairo_destroy (cr);
    cairo_surface_destroy (surface);
  }

  void
  Tooltip::DrawLabel (cairo_t* cr,
                      gint     width,
                      gint     height,
                      gfloat*  rgba)
  {
    int                   textWidth  = 0;
    int                   textHeight = 0;
    PangoLayout*          layout     = NULL;
    PangoFontDescription* desc       = NULL;
    PangoContext*         pangoCtx   = NULL;

    GetTextExtents ((char*) FONT_FACE,
                    &textWidth,
                    &textHeight);

    cairo_set_source_rgba (cr, rgba[0], rgba[1], rgba[2], rgba[3]);

    cairo_set_font_options (cr, _fontOpts);
    layout = pango_cairo_create_layout (cr);
    desc = pango_font_description_from_string ((char*) FONT_FACE);
    pango_font_description_set_weight (desc, PANGO_WEIGHT_NORMAL);
    pango_layout_set_font_description (layout, desc);
    pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
    pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
    pango_layout_set_text (layout, _labelText.GetTCharPtr(), -1);
    pangoCtx = pango_layout_get_context (layout); // is not ref'ed
    pango_cairo_context_set_font_options (pangoCtx, _fontOpts);
    pango_cairo_context_set_resolution (pangoCtx, (double) _dpiX);
    pango_layout_context_changed (layout);

    cairo_move_to (cr,
                   ANCHOR_WIDTH + (float) ((width - ANCHOR_WIDTH) - textWidth) / 2.0f,
                   (float) (height - textHeight) / 2.0f);
    pango_cairo_show_layout (cr, layout);
    cairo_fill (cr);

    // clean up
    pango_font_description_free (desc);
    g_object_unref (layout);
  }

  void
  Tooltip::DrawOutlineShadow (cairo_t* cr,
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
                                   gfloat*  rgba_line)
  {
    ComputeFullMaskPath (cr,
                         anchor_width,
                         anchor_height,
                         width,
                         height,
                         upper_size,
                         corner_radius,
                         padding_size);
    //DrawCairo (cr, TRUE, line_width, rgba_shadow, FALSE, FALSE);
    //ctk_surface_blur (surf, blur_coeff);
    //ComputeMask (cr);
    ComputeOutline (cr,
                    line_width,
                    rgba_line,
                    width,
                    anchor_width,
                    corner_radius,
                    padding_size);
  }

  Tooltip::Tooltip ()
  {
    _texture2D = 0;
    _anchorX   = 0;
    _anchorY   = 0;
    _labelText = TEXT ("unset");
    _fontOpts  = cairo_font_options_create ();

    // FIXME: hard-coding these for the moment, as we don't have
    // gsettings-support in place right now
    cairo_font_options_set_antialias (_fontOpts, CAIRO_ANTIALIAS_SUBPIXEL);
    cairo_font_options_set_hint_metrics (_fontOpts, CAIRO_HINT_METRICS_ON);
    cairo_font_options_set_hint_style (_fontOpts, CAIRO_HINT_STYLE_SLIGHT);
    cairo_font_options_set_subpixel_order (_fontOpts, CAIRO_SUBPIXEL_ORDER_RGB);

    // make sure _dpiX and _dpiY are initialized correctly
    GetDPI ();
  }

  Tooltip::~Tooltip ()
  {
    cairo_font_options_destroy (_fontOpts);
    if (_texture2D)
      _texture2D->UnReference ();
    if (_cairo_graphics)
      delete(_cairo_graphics);
  }

  long
  Tooltip::ProcessEvent (IEvent& ievent,
                         long    TraverseInfo,
                         long    ProcessEventInfo)
  {
    return TraverseInfo;
  }

  void
  Tooltip::Draw (GraphicsEngine& gfxContext,
                 bool             forceDraw)
  {
    Geometry base = GetGeometry();

    // the elements position inside the window are referenced to top-left window
    // corner. So bring base to (0, 0).
    base.SetX (0);
    base.SetY (0);
    gfxContext.PushClippingRectangle (base);

    gPainter.PushDrawShapeLayer (gfxContext,
                                 base,
                                 eSHAPE_CORNER_ROUND10,
                                 m_background_color, eAllCorners, true);

    TexCoordXForm texxform;
    texxform.SetWrap(TEXWRAP_REPEAT, TEXWRAP_REPEAT);
    texxform.SetTexCoordType (TexCoordXForm::OFFSET_COORD);

    gfxContext.QRP_GLSL_1Tex (base.x,
                              base.y,
                              base.width,
                              base.height,
                              _texture2D->GetDeviceTexture(),
                              texxform,
                              Color(1.0f, 1.0f, 1.0f, 1.0f));

    gPainter.PopBackground ();
    gfxContext.PopClippingRectangle ();
  }

  void
  Tooltip::DrawContent (GraphicsEngine& GfxContext,
                             bool             force_draw)
  {
    /*Geometry base = GetGeometry();
    int x = base.x;
    int y = base.y;
    // The elements position inside the window are referenced to top-left window corner. So bring base to (0, 0).
    base.SetX (0);
    base.SetY (0);

    if (UseBlurredBackground() )
    {
      TexCoordXForm texxform;
      texxform.uoffset = (float) x / (float) GetThreadWindowCompositor().GetScreenBlurTexture()->GetWidth();
      texxform.voffset = (float) y / (float) GetThreadWindowCompositor().GetScreenBlurTexture()->GetHeight();
      texxform.SetTexCoordType (TexCoordXForm::OFFSET_COORD);

      gPainter.PushTextureLayer (GfxContext, base, GetThreadWindowCompositor().GetScreenBlurTexture(), texxform, Color::White, true);
    }
    else
    {
      //nuxDebugMsg(TEXT("[BaseWindow::DrawContent]"));
      gPainter.PushShapeLayer (GfxContext, base, eSHAPE_CORNER_ROUND10, m_background_color, eAllCorners, true);
      //GfxContext.QRP_GLSL_Color(base.x, base.y, base.width, base.height, Color(1.0f, 0.0f, 0.0f, 1.0f));
      //GfxContext.QRP_GLSL_Color(base.x, base.y, base.width, base.height, Color(1.0f / (float) (std::rand () % 100), 1.0f / (float) (std::rand () % 100), 1.0f / (float) (std::rand () % 100), 0.5f));
    }

    if (m_layout)
    {
      GfxContext.PushClippingRectangle (base);
      m_layout->ProcessDraw (GfxContext, force_draw);
      GfxContext.PopClippingRectangle();
    }

    gPainter.PopBackground();*/
  }

  void Tooltip::PreLayoutManagement ()
  {
    int  newWidth   = 0;
    int  newHeight  = 0;
    int  textWidth  = 0;
    int  textHeight = 0;

    GetTextExtents ((char*) FONT_FACE,
                    &textWidth,
                    &textHeight);

    newWidth = textWidth + 2 * H_MARGIN + ANCHOR_WIDTH + 2 * PADDING_SIZE;
    newHeight = textHeight + 2 * V_MARGIN + 2 * PADDING_SIZE;
    SetBaseSize (newWidth, newHeight);

    UpdateTexture ();

    BaseWindow::PreLayoutManagement ();
  }

  void
  Tooltip::UpdateTexture ()
  {
    float rgbaTint[4]      = {0.0f, 0.0f, 0.0f, 0.7f};
    float rgbaHighlight[4] = {1.0f, 1.0f, 1.0f, 0.15f};
    float rgbaLine[4]      = {1.0f, 1.0f, 1.0f, 0.75f};
    float rgbaShadow[4]    = {0.0f, 0.0f, 0.0f, 0.48f};
    float rgbaText[4]      = {1.0f, 1.0f, 1.0f, 1.0f};
    float highLightY       = 0.0f;
    float highLightW       = 0.0f;

    _cairo_graphics = new CairoGraphics (CAIRO_FORMAT_ARGB32,
                                         GetBaseWidth (),
                                         GetBaseHeight ());
    cairo_t *cr = _cairo_graphics->GetContext ();

    highLightY = HIGH_LIGHT_Y;
    if (GetBaseWidth () / 1.5f < HIGH_LIGHT_MIN_WIDTH)
        highLightW = HIGH_LIGHT_MIN_WIDTH;
    else
        highLightW = GetBaseWidth () / 1.5f;

    DrawTintDotHighlight (cr,
                          GetBaseWidth (),
                          GetBaseHeight (),
                          GetBaseWidth () / 2.0f,
                          highLightY,
                          highLightW,
                          rgbaTint,
                          rgbaHighlight);

    DrawOutlineShadow (cr,
                       GetBaseWidth (),
                       GetBaseHeight (),
                       ANCHOR_WIDTH,
                       ANCHOR_HEIGHT,
                       -1,
                       RADIUS,
                       BLUR_INTENSITY,
                       rgbaShadow,
                       LINE_WIDTH,
                       PADDING_SIZE,
                       rgbaLine);

    DrawLabel (cr,
               GetBaseWidth (),
               GetBaseHeight (),
               rgbaText);

    NBitmapData* bitmap =  _cairo_graphics->GetBitmap();

    // BaseTexture is the high level representation of an image that is backed by
    // an actual opengl texture.
    if (_texture2D)
      _texture2D->UnReference ();
    
    _texture2D = GetThreadGLDeviceFactory()->CreateSystemCapableTexture ();
    _texture2D->Update(bitmap);
    
    delete _cairo_graphics;
    _cairo_graphics = 0;
  }

  long
  Tooltip::PostLayoutManagement (long LayoutResult)
  {
    long result = BaseWindow::PostLayoutManagement (LayoutResult);

    return result;
  }

  void
  Tooltip::PositionChildLayout (float offsetX,
                                float offsetY)
  {
  }

  void
  Tooltip::LayoutWindowElements ()
  {
  }

  void
  Tooltip::NotifyConfigurationChange (int width,
                                      int height)
  {
  }

  void
  Tooltip::SetText (NString text)
  {
    if (_labelText == text)
      return;

    _labelText = text;
    UpdateTexture ();
  }

} // namespace nux

