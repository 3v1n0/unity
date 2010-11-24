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
 * Authored by: Jay Taoko <jay.taoko@canonical.com>
 */

#include "Nux/Nux.h"
#include "QuicklistMenuItem.h"

#if defined(NUX_OS_LINUX)
#include <X11/Xlib.h>
#endif

void
GetDPI (int &dpi_x, int &dpi_y)
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

  dpi_x = (int) (dpiX + 0.5);
  dpi_y = (int) (dpiY + 0.5);

  XCloseDisplay (display);
#elif defined(NUX_OS_WINDOWS)
  dpi_x = 72;
  dpi_y = 72;
#endif
}

static void
OnPropertyChanged (gchar*             property,
                   GValue*            value,
                   QuicklistMenuItem* self);

static void
OnItemActivated (guint              timestamp,
                 QuicklistMenuItem* self);

QuicklistMenuItem::QuicklistMenuItem (DbusmenuMenuitem* item,
                                      NUX_FILE_LINE_DECL) :
View (NUX_FILE_LINE_PARAM)
{
  _color      = nux::Color (1.0f, 1.0f, 1.0f, 1.0f);
  _menuItem   = item;
  _debug      = false;
  _item_type  = MENUITEM_TYPE_UNKNOWN;
  if (_menuItem)
  {
    g_signal_connect (_menuItem,
                    "property-changed",
                    G_CALLBACK (OnPropertyChanged),
                    this);
    g_signal_connect (_menuItem,
                    "item-activated",
                    G_CALLBACK (OnItemActivated),
                    this);
  }
  
  OnMouseDown.connect (sigc::mem_fun (this, &QuicklistMenuItem::RecvMouseDown));
  OnMouseUp.connect (sigc::mem_fun (this, &QuicklistMenuItem::RecvMouseUp));
  OnMouseClick.connect (sigc::mem_fun (this, &QuicklistMenuItem::RecvMouseClick));
  OnMouseMove.connect (sigc::mem_fun (this, &QuicklistMenuItem::RecvMouseMove));
  OnMouseDrag.connect (sigc::mem_fun (this, &QuicklistMenuItem::RecvMouseDrag));
  OnMouseEnter.connect (sigc::mem_fun (this, &QuicklistMenuItem::RecvMouseEnter));
  OnMouseLeave.connect (sigc::mem_fun (this, &QuicklistMenuItem::RecvMouseLeave));
  
  // Get the original states of the item
  _enabled = true; //GetEnabled ();
  _active = true; //GetActive ();
  _prelight = false;
}

QuicklistMenuItem::QuicklistMenuItem (DbusmenuMenuitem* item,
                                      bool              debug,
                                      NUX_FILE_LINE_DECL) :
View (NUX_FILE_LINE_PARAM)
{
  _color      = nux::Color (1.0f, 1.0f, 1.0f, 1.0f);
  _menuItem   = item;
  _debug      = debug;
  _item_type  = MENUITEM_TYPE_UNKNOWN;
  
  OnMouseDown.connect (sigc::mem_fun (this, &QuicklistMenuItem::RecvMouseDown));
  OnMouseUp.connect (sigc::mem_fun (this, &QuicklistMenuItem::RecvMouseUp));
  OnMouseClick.connect (sigc::mem_fun (this, &QuicklistMenuItem::RecvMouseClick));
  OnMouseMove.connect (sigc::mem_fun (this, &QuicklistMenuItem::RecvMouseMove));
  OnMouseDrag.connect (sigc::mem_fun (this, &QuicklistMenuItem::RecvMouseDrag));
  OnMouseEnter.connect (sigc::mem_fun (this, &QuicklistMenuItem::RecvMouseEnter));
  OnMouseLeave.connect (sigc::mem_fun (this, &QuicklistMenuItem::RecvMouseLeave));
  
  // Get the original states of the item
  _enabled = GetEnabled ();
  _active = GetActive ();
  _prelight = false;
}

QuicklistMenuItem::~QuicklistMenuItem ()
{
}

QuicklistMenuItemType QuicklistMenuItem::GetItemType ()
{
  return _item_type;
}

void
QuicklistMenuItem::PreLayoutManagement ()
{
  View::PreLayoutManagement ();
}

long
QuicklistMenuItem::PostLayoutManagement (long layoutResult)
{
  long result = View::PostLayoutManagement (layoutResult);

  return result;
}

long
QuicklistMenuItem::ProcessEvent (nux::IEvent& event,
                                 long         traverseInfo,
                                 long         processEventInfo)
{
  long result = traverseInfo;

  result = nux::View::PostProcessEvent2 (event, result, processEventInfo);
  return result;

}

void
QuicklistMenuItem::Draw (nux::GraphicsEngine& gfxContext,
                         bool                 forceDraw)
{
}

void
QuicklistMenuItem::DrawContent (nux::GraphicsEngine& gfxContext,
                                bool                 forceDraw)
{
}

void
QuicklistMenuItem::PostDraw (nux::GraphicsEngine& gfxContext,
                             bool                 forceDraw)
{
}

const gchar*
QuicklistMenuItem::GetLabel ()
{
  if (_menuItem == 0)
    return 0;
  return dbusmenu_menuitem_property_get (_menuItem,
                                         DBUSMENU_MENUITEM_PROP_LABEL);
}

bool
QuicklistMenuItem::GetEnabled ()
{
  if (_menuItem == 0)
    return false;
  return dbusmenu_menuitem_property_get_bool (_menuItem,
                                              DBUSMENU_MENUITEM_PROP_ENABLED);
}

bool
QuicklistMenuItem::GetActive ()
{
  if (_menuItem == 0)
    return false;
  return dbusmenu_menuitem_property_get_bool (_menuItem,
                                              DBUSMENU_MENUITEM_PROP_TOGGLE_STATE);
}

void
QuicklistMenuItem::SetText (nux::NString text)
{
  if (_text != text)
  {
    _text = text;
    UpdateTexture ();
    sigTextChanged.emit (this);
  }
}

void
QuicklistMenuItem::SetFontName (nux::NString fontName)
{
  if (_fontName != fontName)
  {
    _fontName = fontName;
    UpdateTexture ();
    sigTextChanged.emit (this);
  }
}

void
QuicklistMenuItem::SetFontSize (float fontSize)
{
  if (_fontSize != fontSize)
  {
    _fontSize = fontSize;
    UpdateTexture ();
    sigTextChanged.emit (this);
  }
}

void
QuicklistMenuItem::SetFontWeight (FontWeight fontWeight)
{
  if (_fontWeight != fontWeight)
  {
    _fontWeight = fontWeight;
    UpdateTexture ();
    sigTextChanged.emit (this);
  }
}

void
QuicklistMenuItem::SetFontStyle (FontStyle fontStyle)
{
  if (_fontStyle != fontStyle)
  {
    _fontStyle = fontStyle;
    UpdateTexture ();
    sigTextChanged.emit (this);
  }
}

void
QuicklistMenuItem::SetColor (nux::Color color)
{
  if (_color != color)
  {
    _color = color;
    //UpdateTextures ();
    sigColorChanged.emit (this);
  }
}

void QuicklistMenuItem::ItemActivated ()
{
  if (_debug)
    sigChanged.emit (*this);

  std::cout << "ItemActivated() called" << std::endl;
}

void QuicklistMenuItem::DrawRoundedRectangle ()
{
}

void QuicklistMenuItem::GetTextExtents (int &width, int &height)
{
  nux::NString str = nux::NString::Printf (TEXT ("%s %.2f"),
                                           _fontName.GetTCharPtr (),
                                           _fontSize);
  GetTextExtents (str.GetTCharPtr (), width, height);
}

void QuicklistMenuItem::GetTextExtents (const TCHAR* font,
                                            int&         width,
                                            int&         height)
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

static void
OnPropertyChanged (gchar*             property,
                   GValue*            value,
                   QuicklistMenuItem* self)
{
  //todo
  //self->UpdateTexture ();
}

static void
OnItemActivated (guint              timestamp,
                 QuicklistMenuItem* self)
{
  //todo:
  //self->ItemActivated ();
}

void QuicklistMenuItem::RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags)
{

}

void QuicklistMenuItem::RecvMouseUp (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  sigMouseReleased.emit (this);
}

void QuicklistMenuItem::RecvMouseClick (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  if (!_enabled)
  {
    sigMouseClick.emit (this);
    return;
  }
  _active = !_active;
  sigMouseClick.emit (this);
}

void QuicklistMenuItem::RecvMouseMove (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  
}

void QuicklistMenuItem::RecvMouseDrag (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  
}

void QuicklistMenuItem::RecvMouseEnter (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  _prelight = true;
  sigMouseEnter.emit (this);
}

void QuicklistMenuItem::RecvMouseLeave (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  _prelight = false;
  sigMouseLeave.emit (this);
}
