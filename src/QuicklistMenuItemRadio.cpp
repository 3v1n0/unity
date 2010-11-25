/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by: Mirco Müller <mirco.mueller@canonical.com>
 */

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "Nux/Nux.h"
#include "QuicklistMenuItemRadio.h"

#define ITEM_INDENT_ABS        20.0f
#define ITEM_CORNER_RADIUS_ABS 4.0f

static double
_align (double val)
{
  double fract = val - (int) val;

  if (fract != 0.5f)
    return (double) ((int) val + 0.5f);
  else
    return val;
}

QuicklistMenuItemRadio::QuicklistMenuItemRadio (DbusmenuMenuitem* item,
                                                NUX_FILE_LINE_DECL) :
QuicklistMenuItem (item,
                   NUX_FILE_LINE_PARAM)
{
  Initialize (item);
}

QuicklistMenuItemRadio::QuicklistMenuItemRadio (DbusmenuMenuitem* item,
                                                bool              debug,
                                                NUX_FILE_LINE_DECL) :
QuicklistMenuItem (item,
                   debug,
                   NUX_FILE_LINE_PARAM)
{
  Initialize (item);
}

void
QuicklistMenuItemRadio::Initialize (DbusmenuMenuitem* item)
{
  _fontName   = TEXT ("Ubuntu");
  _fontSize   = 12;
  _fontStyle  = FONTSTYLE_NORMAL;
  _fontWeight = FONTWEIGHT_NORMAL;
  _item_type  = MENUITEM_TYPE_LABEL;
  
  if (item)
    _text = dbusmenu_menuitem_property_get (item, DBUSMENU_MENUITEM_PROP_LABEL);
  else
    _text = "QuicklistItem";
  
  _normalTexture[0]   = NULL;
  _normalTexture[1]   = NULL;
  _prelightTexture[0] = NULL;
  _prelightTexture[1] = NULL;


  SetMinimumSize (1, 1);
  // make sure _dpiX and _dpiY are initialized correctly
}

QuicklistMenuItemRadio::~QuicklistMenuItemRadio ()
{
  if (_normalTexture[0])
    _normalTexture[0]->UnReference ();

  if (_normalTexture[1])
    _normalTexture[1]->UnReference ();

  if (_prelightTexture[0])
    _prelightTexture[0]->UnReference ();

  if (_prelightTexture[1])
    _prelightTexture[1]->UnReference ();
}

void
QuicklistMenuItemRadio::PreLayoutManagement ()
{
}

long
QuicklistMenuItemRadio::PostLayoutManagement (long layoutResult)
{
  int w = GetBaseWidth();
  int h = GetBaseHeight();

  long result = 0;
  
  if (_pre_layout_width < w)
    result |= nux::eLargerWidth;
  else if (_pre_layout_width > w)
    result |= nux::eSmallerWidth;
  else
    result |= nux::eCompliantWidth;

  if (_pre_layout_height < h)
    result |= nux::eLargerHeight;
  else if (_pre_layout_height > h)
    result |= nux::eSmallerHeight;
  else
    result |= nux::eCompliantHeight;

  return result;
}

long
QuicklistMenuItemRadio::ProcessEvent (nux::IEvent& event,
                                      long         traverseInfo,
                                      long         processEventInfo)
{
  long result = traverseInfo;

  result = nux::View::PostProcessEvent2 (event, result, processEventInfo);
  return result;
}

void
QuicklistMenuItemRadio::Draw (nux::GraphicsEngine& gfxContext,
                              bool                 forceDraw)
{
  nux::IntrusiveSP<nux::IOpenGLBaseTexture> texture;

  nux::Geometry base = GetGeometry ();

  gfxContext.PushClippingRectangle (base);

  nux::TexCoordXForm texxform;
  texxform.SetWrap (nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);

  gfxContext.GetRenderStates().SetBlend (true,
                                         GL_ONE,
                                         GL_ONE_MINUS_SRC_ALPHA);

  if (GetActive ())
    if (GetEnabled ())
      texture = _prelightTexture[1]->GetDeviceTexture ();
    else
      texture = _prelightTexture[0]->GetDeviceTexture ();
  else
    if (GetEnabled ())
      texture = _normalTexture[1]->GetDeviceTexture ();
    else
      texture = _normalTexture[0]->GetDeviceTexture ();

  gfxContext.QRP_GLSL_1Tex (base.x,
                            base.y,
                            base.width,
                            base.height,
                            texture,
                            texxform,
                            _color);

  gfxContext.GetRenderStates().SetBlend (false);

  gfxContext.PopClippingRectangle ();
}

void
QuicklistMenuItemRadio::DrawContent (nux::GraphicsEngine& gfxContext,
                                     bool                 forceDraw)
{
}

void
QuicklistMenuItemRadio::PostDraw (nux::GraphicsEngine& gfxContext,
                                  bool                 forceDraw)
{
}

void
QuicklistMenuItemRadio::DrawText (cairo_t*   cr,
                                  int        width,
                                  int        height,
                                  nux::Color color)
{
  int                   textWidth  = 0;
  int                   textHeight = 0;
  PangoLayout*          layout     = NULL;
  PangoFontDescription* desc       = NULL;
  PangoContext*         pangoCtx   = NULL;
  int                   dpi      = 0;
  GdkScreen*            screen   = gdk_screen_get_default ();   // is not ref'ed
  GtkSettings*          settings = gtk_settings_get_default (); // is not ref'ed

  nux::NString str = nux::NString::Printf(TEXT("%s %.2f"),
                                          _fontName.GetTCharPtr (),
                                          _fontSize);
  GetTextExtents (str.GetTCharPtr (), textWidth, textHeight);

  cairo_set_font_options (cr, gdk_screen_get_font_options (screen));
  layout = pango_cairo_create_layout (cr);
  desc = pango_font_description_from_string (str.GetTCharPtr ());
  pango_font_description_set_weight (desc, (PangoWeight) _fontWeight);
  pango_layout_set_font_description (layout, desc);
  pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
  pango_layout_set_markup (layout, _text.GetTCharPtr(), -1);
  pangoCtx = pango_layout_get_context (layout); // is not ref'ed
  pango_cairo_context_set_font_options (pangoCtx,
                                        gdk_screen_get_font_options (screen));
  g_object_get (settings, "gtk-xft-dpi", &dpi, NULL);
  if (dpi == -1)
  {
    // use some default DPI-value
    pango_cairo_context_set_resolution (pangoCtx, 96.0f);
  }
  else
  {
    pango_cairo_context_set_resolution (pangoCtx,
                                        (float) dpi / (float) PANGO_SCALE);
  }

  pango_layout_context_changed (layout);

  cairo_move_to (cr, 0.0f, 0.0f);
  pango_cairo_show_layout (cr, layout);

  // clean up
  pango_font_description_free (desc);
  g_object_unref (layout);
}

void
QuicklistMenuItemRadio::UpdateTexture ()
{
  int width  = GetBaseWidth ();
  int height = GetBaseHeight ();

  _cairoGraphics = new nux::CairoGraphics (CAIRO_FORMAT_ARGB32, width, height);
  cairo_t *cr = _cairoGraphics->GetContext ();

  // draw normal, disabled version
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_scale (cr, 1.0f, 1.0f);
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cairo_set_line_width (cr, 1.0f);

  DrawText (cr, width, height, _textColor);

  nux::NBitmapData* bitmap = _cairoGraphics->GetBitmap ();

  if (_normalTexture[0])
    _normalTexture[0]->UnReference ();

  _normalTexture[0] = nux::GetThreadGLDeviceFactory()->CreateSystemCapableTexture ();
  _normalTexture[0]->Update (bitmap);

  // draw normal, enabled version
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_scale (cr, 1.0f, 1.0f);
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cairo_set_line_width (cr, 1.0f);

  double x      = _align (ITEM_INDENT_ABS / 2.0f);
  double y      = _align ((double) height / 2.0f);
  double radius = 3.5f;

  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cairo_arc (cr, x, y, radius, 0.0f * (G_PI / 180.0f), 360.0f * (G_PI / 180.0f));
  cairo_fill (cr);

  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
  DrawText (cr, width, height, _textColor);

  bitmap = _cairoGraphics->GetBitmap ();

  if (_normalTexture[1])
    _normalTexture[1]->UnReference ();

  _normalTexture[1] = nux::GetThreadGLDeviceFactory()->CreateSystemCapableTexture ();
  _normalTexture[1]->Update (bitmap);

  // draw active/prelight, unchecked version
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_scale (cr, 1.0f, 1.0f);
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cairo_set_line_width (cr, 1.0f);

  DrawRoundedRectangle (cr,
                        1.0f,
                        0.5f,
                        0.5f,
                        ITEM_CORNER_RADIUS_ABS,
                        width - 1.0f,
                        height - 1.0f);
  cairo_fill (cr);

  cairo_set_source_rgba (cr, 0.0f, 0.0f, 0.0f, 0.0f);

  DrawText (cr, width, height, _textColor);

  bitmap = _cairoGraphics->GetBitmap ();

  if (_prelightTexture[0])
    _prelightTexture[0]->UnReference ();

  _prelightTexture[0] = nux::GetThreadGLDeviceFactory()->CreateSystemCapableTexture ();
  _prelightTexture[0]->Update (bitmap);

  // draw active/prelight, unchecked version
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_scale (cr, 1.0f, 1.0f);
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cairo_set_line_width (cr, 1.0f);

  DrawRoundedRectangle (cr,
                        1.0f,
                        0.5f,
                        0.5f,
                        ITEM_CORNER_RADIUS_ABS,
                        width - 1.0f,
                        height - 1.0f);
  cairo_fill (cr);

  cairo_set_source_rgba (cr, 0.0f, 0.0f, 0.0f, 0.0f);

  cairo_arc (cr, x, y, radius, 0.0f * (G_PI / 180.0f), 360.0f * (G_PI / 180.0f));
  cairo_fill (cr);

  DrawText (cr, width, height, _textColor);

  bitmap = _cairoGraphics->GetBitmap ();

  if (_prelightTexture[1])
    _prelightTexture[1]->UnReference ();

  _prelightTexture[1] = nux::GetThreadGLDeviceFactory()->CreateSystemCapableTexture ();
  _prelightTexture[1]->Update (bitmap);

  // finally clean up
  delete _cairoGraphics;
}

void
QuicklistMenuItemRadio::SetText (nux::NString text)
{
  if (_text != text)
  {
    _text = text;
    UpdateTexture ();
    sigTextChanged.emit (this);
  }
}

void
QuicklistMenuItemRadio::SetFontName (nux::NString fontName)
{
  if (_fontName != fontName)
  {
    _fontName = fontName;
    UpdateTexture ();
    sigTextChanged.emit (this);
  }
}

void
QuicklistMenuItemRadio::SetFontSize (float fontSize)
{
  if (_fontSize != fontSize)
  {
    _fontSize = fontSize;
    UpdateTexture ();
    sigTextChanged.emit (this);
  }
}

void
QuicklistMenuItemRadio::SetFontWeight (FontWeight fontWeight)
{
  if (_fontWeight != fontWeight)
  {
    _fontWeight = fontWeight;
    UpdateTexture ();
    sigTextChanged.emit (this);
  }
}

void
QuicklistMenuItemRadio::SetFontStyle (FontStyle fontStyle)
{
  if (_fontStyle != fontStyle)
  {
    _fontStyle = fontStyle;
    UpdateTexture ();
    sigTextChanged.emit (this);
  }
}

int QuicklistMenuItemRadio::CairoSurfaceWidth ()
{
  if (_normalTexture[0])
    return _normalTexture[0]->GetWidth ();

  return 0;
}