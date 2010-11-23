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
QuicklistMenuItem::SetColor (nux::Color color)
{
  if (_color != color)
  {
    _color = color;
    //UpdateTextures ();
    sigColorChanged.emit (this);
  }
}

void
QuicklistMenuItem::ItemActivated ()
{
  if (_debug)
    sigChanged.emit (*this);

  std::cout << "ItemActivated() called" << std::endl;
}

void
QuicklistMenuItem::DrawRoundedRectangle ()
{
}

void
QuicklistMenuItem::GetTextExtents (int &width, int &height)
{
  
}

void QuicklistMenuItem::UpdateTexture ()
{
  
}

static void
OnPropertyChanged (gchar*             property,
                   GValue*            value,
                   QuicklistMenuItem* self)
{
  self->UpdateTexture ();
}

static void
OnItemActivated (guint              timestamp,
                 QuicklistMenuItem* self)
{
  self->ItemActivated ();
}

void QuicklistMenuItem::RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags)
{

}

void QuicklistMenuItem::RecvMouseUp (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  printf ("Mouse Up\n");
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
