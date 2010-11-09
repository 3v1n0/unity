/*
 * Copyright 2010 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of either or both of the following licenses:
 *
 * 1) the GNU Lesser General Public License version 3, as published by the
 * Free Software Foundation; and/or
 * 2) the GNU Lesser General Public License version 2.1, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 and version 2.1 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Mirco Müller <mirco.mueller@canonical.com
 */

#include "Nux/Nux.h"
#include "Nux/Layout.h"
#include "Nux/HLayout.h"
#include "Nux/VLayout.h"
#include "Nux/Validator.h"
#include "Nux/StaticCairoText.h"

namespace nux
{
  StaticCairoText::StaticCairoText (const TCHAR* text,
                                       NUX_FILE_LINE_DECL) :
  View (NUX_FILE_LINE_PARAM)
{
#if defined(NUX_OS_WINDOWS)
  _fontName   = TEXT ("Arial");
#elif defined(NUX_OS_LINUX)
  _fontName   = TEXT ("Ubuntu");
#endif
  _fontSize   = 12;
  _fontStyle  = eNormalStyle;
  _fontWeight = eNormalWeight;
  _textColor  = Color(1.0f, 1.0f, 1.0f, 1.0f);
  _text       = TEXT (text);
  _fontOpts   = cairo_font_options_create ();
  _texture2D  = 0;

  // FIXME: hard-coding these for the moment, as we don't have
  // gsettings-support in place right now
  cairo_font_options_set_antialias (_fontOpts, CAIRO_ANTIALIAS_SUBPIXEL);
  cairo_font_options_set_hint_metrics (_fontOpts, CAIRO_HINT_METRICS_ON);
  cairo_font_options_set_hint_style (_fontOpts, CAIRO_HINT_STYLE_SLIGHT);
  cairo_font_options_set_subpixel_order (_fontOpts, CAIRO_SUBPIXEL_ORDER_RGB);

  SetMinimumSize (1, 1);
  // make sure _dpiX and _dpiY are initialized correctly
  GetDPI ();
}

  StaticCairoText::StaticCairoText (const TCHAR* text,
                                       NString      fontName,
                                       float        fontSize,
                                       eFontStyle   fontStyle,
                                       eFontWeight  fontWeight,
                                       Color        textColor,
                                       NUX_FILE_LINE_DECL) :
  View (NUX_FILE_LINE_PARAM)
{
  _text = TEXT (text);
  _fontName = fontName;
  _fontSize = fontSize;
  _fontStyle = fontStyle;
  _fontWeight = fontWeight;
  _textColor = textColor;
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

StaticCairoText::~StaticCairoText ()
{
  cairo_font_options_destroy (_fontOpts);
  delete (_cairoGraphics);
  delete (_texture2D);
}
  
void StaticCairoText::PreLayoutManagement ()
{
  int textWidth  = 0;
  int textHeight = 0;

  NString str = NString::Printf(TEXT("%s %.2f"), _fontName.GetTCharPtr (), _fontSize);
  GetTextExtents (str.GetTCharPtr (), textWidth, textHeight);

  _pre_layout_width = GetBaseWidth ();
  _pre_layout_height = GetBaseHeight ();

  SetBaseSize (textWidth, textHeight);

  if((_texture2D == 0) )
  {    
    UpdateTexture ();
  }

  View::PreLayoutManagement ();
}

long StaticCairoText::PostLayoutManagement (long layoutResult)
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

long
StaticCairoText::ProcessEvent (IEvent& event,
                                    long    traverseInfo,
                                    long    processEventInfo)
{
  long ret = traverseInfo;

  ret = PostProcessEvent2 (event, ret, processEventInfo);
  return ret;
}

void
StaticCairoText::Draw (GraphicsEngine& gfxContext,
                            bool             forceDraw)
{
  Geometry base = GetGeometry ();

  gfxContext.PushClippingRectangle (base);

  TexCoordXForm texxform;
  texxform.SetWrap (TEXWRAP_REPEAT, TEXWRAP_REPEAT);
  texxform.SetTexCoordType (TexCoordXForm::OFFSET_COORD);

  gfxContext.GetRenderStates().SetBlend (true,
                                         GL_ONE,
                                         GL_ONE_MINUS_SRC_ALPHA);

  gfxContext.QRP_GLSL_1Tex (base.x,
                            base.y,
                            base.width,
                            base.height,
                            _texture2D->GetDeviceTexture(),
                            texxform,
                            Color(1.0f, 1.0f, 1.0f, 1.0f));

  gfxContext.GetRenderStates().SetBlend (false);

  gfxContext.PopClippingRectangle ();
}

void
StaticCairoText::DrawContent (GraphicsEngine& gfxContext,
                                   bool             forceDraw)
{
  // intentionally left empty
}

void
StaticCairoText::PostDraw (GraphicsEngine& gfxContext,
                                bool             forceDraw)
{
  // intentionally left empty
}

void
StaticCairoText::SetText (NString text)
{
  if (_text != text)
  {
    _text = text;
    UpdateTexture ();
    sigTextChanged.emit (*this);
  }
}

void
StaticCairoText::SetFontName (NString fontName)
{
  if (_fontName != fontName)
  {
    _fontName = fontName;
    UpdateTexture ();
    sigTextChanged.emit (*this);
  }
}

void
StaticCairoText::SetFontSize (float fontSize)
{
  if (_fontSize != fontSize)
  {
    _fontSize = fontSize;
    UpdateTexture ();
    sigTextChanged.emit (*this);
  }
}

void
StaticCairoText::SetFontWeight (eFontWeight fontWeight)
{
  if (_fontWeight != fontWeight)
  {
    _fontWeight = fontWeight;
    UpdateTexture ();
    sigTextChanged.emit (*this);
  }
}

void
StaticCairoText::SetFontStyle (eFontStyle fontStyle)
{
  if (_fontStyle != fontStyle)
  {
    _fontStyle = fontStyle;
    UpdateTexture ();
    sigTextChanged.emit (*this);
  }
}

void
StaticCairoText::SetTextColor (Color textColor)
{
  if (_textColor != textColor)
  {
    _textColor = textColor;
    UpdateTexture ();
    sigTextChanged.emit (*this);
  }
}

void
StaticCairoText::GetDPI ()
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

void StaticCairoText::GetTextExtents (int &width, int &height)
{
  NString str = NString::Printf(TEXT("%s %.2f"), _fontName.GetTCharPtr (), _fontSize);
  GetTextExtents (str.GetTCharPtr (), width, height);
}

void StaticCairoText::GetTextExtents (const TCHAR* font,
                                      int&  width,
                                      int&  height)
{
  cairo_surface_t*      surface  = NULL;
  cairo_t*              cr       = NULL;
  PangoLayout*          layout   = NULL;
  PangoFontDescription* desc     = NULL;
  PangoContext*         pangoCtx = NULL;
  PangoRectangle        logRect  = {0, 0, 0, 0};

  // sanity check
  if (!font)
    return;

  surface = cairo_image_surface_create (CAIRO_FORMAT_A1, 1, 1);
  cr = cairo_create (surface);
  layout = pango_cairo_create_layout (cr);
  desc = pango_font_description_from_string (font);
  pango_font_description_set_weight (desc, PANGO_WEIGHT_NORMAL);
  pango_layout_set_font_description (layout, desc);
  pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
  pango_layout_set_markup (layout, _text.GetTCharPtr(), -1);
  pangoCtx = pango_layout_get_context (layout); // is not ref'ed
  pango_cairo_context_set_font_options (pangoCtx, _fontOpts);
  pango_cairo_context_set_resolution (pangoCtx, _dpiX);
  pango_layout_context_changed (layout);
  pango_layout_get_extents (layout, NULL, &logRect);

  width  = logRect.width / PANGO_SCALE;
  height = logRect.height / PANGO_SCALE;

  // clean up
  pango_font_description_free (desc);
  g_object_unref (layout);
  cairo_destroy (cr);
  cairo_surface_destroy (surface);
}

void StaticCairoText::DrawText (cairo_t*   cr,
                                int        width,
                                int        height,
                                Color color)
{
  int                   textWidth  = 0;
  int                   textHeight = 0;
  PangoLayout*          layout     = NULL;
  PangoFontDescription* desc       = NULL;
  PangoContext*         pangoCtx   = NULL;

  NString str = NString::Printf(TEXT("%s %.2f"), _fontName.GetTCharPtr (), _fontSize);
  GetTextExtents (str.GetTCharPtr (), textWidth, textHeight);

  cairo_set_font_options (cr, _fontOpts);
  layout = pango_cairo_create_layout (cr);
  //desc = pango_font_description_from_string ((char*) FONT_FACE);
  desc = pango_font_description_from_string (str.GetTCharPtr ());
  pango_font_description_set_weight (desc, (PangoWeight) _fontWeight);
  pango_layout_set_font_description (layout, desc);
  pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
  pango_layout_set_markup (layout, _text.GetTCharPtr(), -1);
  pangoCtx = pango_layout_get_context (layout); // is not ref'ed
  pango_cairo_context_set_font_options (pangoCtx, _fontOpts);
  pango_cairo_context_set_resolution (pangoCtx, (double) _dpiX);

  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba (cr, 0.0f, 0.0f, 0.0f, 0.0f);
  cairo_paint (cr);
  cairo_set_source_rgba (cr, color.R (),color.G (), color.B (), color.A ());

  pango_layout_context_changed (layout);

  cairo_move_to (cr, 0.0f, 0.0f);
  pango_cairo_show_layout (cr, layout);

  // clean up
  pango_font_description_free (desc);
  g_object_unref (layout);
}

void StaticCairoText::UpdateTexture ()
{
  int width = 0;
  int height = 0;
  GetTextExtents(width, height);

  SetBaseSize(width, height);

  _cairoGraphics = new CairoGraphics (CAIRO_FORMAT_ARGB32,
                                      GetBaseWidth (),
                                      GetBaseHeight ());
  cairo_t *cr = _cairoGraphics->GetContext ();

  DrawText (cr, GetBaseWidth (), GetBaseHeight (), _textColor);

  NBitmapData* bitmap = _cairoGraphics->GetBitmap ();

  // NTexture2D is the high level representation of an image that is backed by
  // an actual opengl texture.

  if (_texture2D)
    _texture2D->UnReference ();

  _texture2D = GetThreadGLDeviceFactory()->CreateSystemCapableTexture ();
  _texture2D->Update (bitmap);

  delete _cairoGraphics;
}

}