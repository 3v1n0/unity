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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#include <sys/time.h>

#include "Nux/Nux.h"
#include "Nux/VScrollBar.h"
#include "Nux/HLayout.h"
#include "Nux/VLayout.h"
#include "Nux/MenuPage.h"
#include "Nux/WindowCompositor.h"
#include "Nux/BaseWindow.h"
#include "Nux/MenuPage.h"
#include "NuxCore/Color.h"

#include "LauncherIcon.h"
#include "Launcher.h"

#include "QuicklistMenuItem.h"
#include "QuicklistMenuItemLabel.h"
#include "QuicklistMenuItemSeparator.h"

#define DEFAULT_ICON "application-default-icon"

nux::Tooltip *LauncherIcon::_current_tooltip = 0;
QuicklistView *LauncherIcon::_current_quicklist = 0;

LauncherIcon::LauncherIcon(Launcher* launcher)
{
  _folding_angle = 0;
  _launcher = launcher;
  m_TooltipText = "blank";

  _show_time.tv_sec = 0;
  _hide_time.tv_sec = 0;
  _running_time.tv_sec = 0;
  _urgent_time.tv_sec = 0;
  _present_time.tv_sec = 0;
  _unpresent_time.tv_sec = 0;

  _show_time.tv_nsec = 0;
  _hide_time.tv_nsec = 0;
  _running_time.tv_nsec = 0;
  _urgent_time.tv_nsec = 0;
  _present_time.tv_nsec = 0;
  _unpresent_time.tv_nsec = 0;

  _active    = false;
  _running   = false;
  _visible   = false;
  _urgent    = false;
  _presented = false;
  
  _related_windows = 0;

  _background_color = nux::Color::White;
  _mouse_inside = false;
  _tooltip = new nux::Tooltip ();
  _icon_type = LAUNCHER_ICON_TYPE_NONE;
  _sort_priority = 0;

  _quicklist = new QuicklistView ();
  _quicklist->sigVisible.connect (sigc::mem_fun (this, &LauncherIcon::RecvShowQuicklist));
  _quicklist->sigHidden.connect (sigc::mem_fun (this, &LauncherIcon::RecvHideQuicklist));
  _quicklist_is_initialized = false;
  
  MouseEnter.connect (sigc::mem_fun(this, &LauncherIcon::RecvMouseEnter));
  MouseLeave.connect (sigc::mem_fun(this, &LauncherIcon::RecvMouseLeave));
  MouseDown.connect (sigc::mem_fun(this, &LauncherIcon::RecvMouseDown));
  MouseUp.connect (sigc::mem_fun(this, &LauncherIcon::RecvMouseUp));
  
}

LauncherIcon::~LauncherIcon()
{
}

nux::Color LauncherIcon::BackgroundColor ()
{
  return _background_color;
}

nux::BaseTexture * LauncherIcon::TextureForSize (int size)
{
  nux::BaseTexture * result = GetTextureForSize (size);
  return result;
}

nux::Color LauncherIcon::ColorForIcon (GdkPixbuf *pixbuf)
{
  unsigned int width = gdk_pixbuf_get_width (pixbuf);
  unsigned int height = gdk_pixbuf_get_height (pixbuf);
  unsigned int row_bytes = gdk_pixbuf_get_rowstride (pixbuf);
  
  long int rtotal = 0, gtotal = 0, btotal = 0;
  float total = 0.0f;
  
  
  guchar *img = gdk_pixbuf_get_pixels (pixbuf);
  
  for (unsigned int i = 0; i < width; i++)
  {
    for (unsigned int j = 0; j < height; j++)
    {
      guchar *pixels = img + ( j * row_bytes + i * 4);
      guchar r = *(pixels + 0);
      guchar g = *(pixels + 1);
      guchar b = *(pixels + 2);
      guchar a = *(pixels + 3);
      
      float saturation = (MAX (r, MAX (g, b)) - MIN (r, MIN (g, b))) / 255.0f;
      float relevance = .1 + .9 * (a / 255.0f) * saturation;
      
      rtotal += (guchar) (r * relevance);
      gtotal += (guchar) (g * relevance);
      btotal += (guchar) (b * relevance);
      
      total += relevance * 255;
    }
  }
  
  float r, g, b, h, s, v;
  r = rtotal / total;
  g = gtotal / total;
  b = btotal / total;
  
  nux::RGBtoHSV (r, g, b, h, s, v);
  
  if (s > .15f)
    s = 0.4f;
  v = .85f;
  
  nux::HSVtoRGB (r, g, b, h, s, v);
  
  return nux::Color (r, g, b);
}

nux::BaseTexture * LauncherIcon::TextureFromGtkTheme (const char *icon_name, int size)
{
  GdkPixbuf *pbuf;
  GtkIconTheme *theme;
  GtkIconInfo *info;
  nux::BaseTexture *result;
  GError *error = NULL;
  
  theme = gtk_icon_theme_get_default ();
      
  if (!icon_name)
    icon_name = g_strdup (DEFAULT_ICON);
   
  info = gtk_icon_theme_lookup_icon (theme,
                                     icon_name,
                                     size,
                                     (GtkIconLookupFlags) 0);            
  if (!info)
  {
    info = gtk_icon_theme_lookup_icon (theme,
                                       DEFAULT_ICON,
                                       size,
                                       (GtkIconLookupFlags) 0);
  }
        
  if (gtk_icon_info_get_filename (info) == NULL)
  {
    info = gtk_icon_theme_lookup_icon (theme,
                                       DEFAULT_ICON,
                                       size,
                                       (GtkIconLookupFlags) 0);
  }
  
  pbuf = gtk_icon_info_load_icon (info, &error);

  if (GDK_IS_PIXBUF (pbuf))
  {
    result = nux::CreateTextureFromPixbuf (pbuf); 
    _background_color = ColorForIcon (pbuf);
  
    g_object_unref (pbuf);
  }
  else
  {
    g_warning ("Unable to load '%s' from icon theme: %s",
               icon_name,
               error ? error->message : "unknown");

    if (g_strcmp0 (icon_name, "folder") == 0)
      return NULL;
    else
      return TextureFromGtkTheme ("folder", size);
  }
  
  return result;
}

void LauncherIcon::SetTooltipText(const TCHAR* text)
{
    m_TooltipText = text;
    _tooltip->SetText (m_TooltipText);
}

nux::NString LauncherIcon::GetTooltipText()
{
    return m_TooltipText;
}

void
LauncherIcon::RecvMouseEnter ()
{
  if (_quicklist_is_initialized == false)
  {
    std::list<DbusmenuClient *> menus_list = Menus ();
    std::list<DbusmenuClient *>::iterator it;
    for (it = menus_list.begin (); it != menus_list.end (); it++)
    {
      g_signal_connect(G_OBJECT(*it), DBUSMENU_CLIENT_SIGNAL_ROOT_CHANGED, G_CALLBACK(&LauncherIcon::root_changed), _quicklist);
      dbusmenu_client_add_type_handler (*it, DBUSMENU_CLIENT_TYPES_DEFAULT, (&LauncherIcon::label_handler));
      dbusmenu_client_add_type_handler (*it, DBUSMENU_CLIENT_TYPES_SEPARATOR, (&LauncherIcon::separator_handler));
    }
    
    _quicklist_is_initialized = true;
  }
  
  if (_launcher->GetActiveQuicklist ())
  {
    // A quicklist is active
    return;
  }
  
//   int icon_x = _xform_screen_coord[0].x;
//   int icon_y = _xform_screen_coord[0].y;
//   int icon_w = _xform_screen_coord[2].x - _xform_screen_coord[0].x;
//   int icon_h = _xform_screen_coord[2].y - _xform_screen_coord[0].y;

    //int icon_x = _xform_screen_coord[0].x;
  int icon_y = _xform_screen_coord[0].y;
  //int icon_w = _xform_screen_coord[2].x - _xform_screen_coord[0].x;
  int icon_h = _xform_screen_coord[2].y - _xform_screen_coord[0].y;

  int tip_x = _launcher->GetBaseWidth () + 1; //icon_x + icon_w;
  int tip_y = 24 + // The BaseWindow where the launcher resides is 24 pixels away from the top of the screen: find a better way to get that number.
          icon_y +
          (icon_h / 2);
          
  _tooltip->ShowTooltipWithTipAt (tip_x, tip_y);
  
  if (!_quicklist->IsVisible ())
  {
    _tooltip->ShowWindow (true);
  }
}

void LauncherIcon::RecvMouseLeave ()
{
  _tooltip->ShowWindow (false);
}


gboolean LauncherIcon::label_handler (DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient * client)
{
  //const gchar* s = dbusmenu_menuitem_property_get (newitem, DBUSMENU_MENUITEM_PROP_LABEL);
  //printf ("label: %s\n", s);
  
  return true;
}

gboolean LauncherIcon::separator_handler (DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient * client)
{
  //const gchar* s = dbusmenu_menuitem_property_get (newitem, DBUSMENU_MENUITEM_PROP_LABEL);
  //printf ("separator: %s\n", s);

  return true;
}

void LauncherIcon::child_realized (DbusmenuMenuitem *newitem, QuicklistView *quicklist)
{
  const gchar* type = dbusmenu_menuitem_property_get (newitem, DBUSMENU_MENUITEM_PROP_TYPE);
  
  if (g_strcmp0 (type, DBUSMENU_CLIENT_TYPES_SEPARATOR) == 0)
  {
    quicklist->AddMenuItem ("-----------------");
    //QuicklistMenuItemSeparator* item = new QuicklistMenuItemSeparator (newitem, NUX_TRACKER_LOCATION);
    //quicklist->AddMenuItem (item);
  }
  else
  {
    QuicklistMenuItemLabel* item = new QuicklistMenuItemLabel (newitem, NUX_TRACKER_LOCATION);
    quicklist->AddMenuItem (item);
  }
    
}

void LauncherIcon::root_changed (DbusmenuClient * client, DbusmenuMenuitem * newroot, QuicklistView *quicklist)
{
  GList * child = NULL;
  for (child = dbusmenu_menuitem_get_children(newroot); child != NULL; child = g_list_next(child))
  {
    g_signal_connect(G_OBJECT(child->data), DBUSMENU_MENUITEM_SIGNAL_REALIZED, G_CALLBACK(child_realized), quicklist);    
  }
}

void LauncherIcon::RecvMouseDown (int button)
{
  if (button == 3)
  {
    if (_launcher->GetActiveQuicklist () == _quicklist)
    {
      // this quicklist is already active
      return;
    }
    
    if (_launcher->GetActiveQuicklist ())
    {
      // Hide the active quicklist. This will prevent it from Ungrabing the pointer in 
      // QuicklistView::RecvMouseDownOutsideOfQuicklist or void QuicklistView::RecvMouseClick.
      // So the new quicklist that is about to be set as active will keep the grab of the pointer.
      // Also disable theinput window.
      _launcher->GetActiveQuicklist ()->EnableInputWindow (false);
      _launcher->GetActiveQuicklist ()->CaptureMouseDownAnyWhereElse (false);
      // This call must be last, because after, _launcher->GetActiveQuicklist () will return Null.
      // the launcher listen to the sigHidden signal emitted by the BaseWindow when it becomes invisible
      // and it set the active window to Null.
      _launcher->GetActiveQuicklist ()->ShowWindow (false);
    }
    
    _tooltip->ShowWindow (false);
    
    
    //int icon_x = _xform_screen_coord[0].x;
    int icon_y = _xform_screen_coord[0].y;
    //int icon_w = _xform_screen_coord[2].x - _xform_screen_coord[0].x;
    int icon_h = _xform_screen_coord[2].y - _xform_screen_coord[0].y;

    int tip_x = _launcher->GetBaseWidth () + 1; //icon_x + icon_w;
    int tip_y = 24 + // The BaseWindow where the launcher resides is 24 pixels away from the top of the screen: find a better way to get that number.
            icon_y +
            (icon_h / 2);

    _quicklist->ShowQuicklistWithTipAt (tip_x, tip_y);
    _quicklist->EnableInputWindow (true);
    _quicklist->GrabPointer ();
    nux::GetWindowCompositor ().SetAlwaysOnFrontWindow (_quicklist);
    _quicklist->NeedRedraw ();
  }
}

void LauncherIcon::RecvMouseUp (int button)
{
  if (button == 3)
  {
    if (_quicklist->IsVisible ())
      _quicklist->CaptureMouseDownAnyWhereElse (true);
  }
}

void LauncherIcon::RecvShowQuicklist (nux::BaseWindow *quicklist)
{
  _launcher->SetActiveQuicklist (_quicklist);
}

void LauncherIcon::RecvHideQuicklist (nux::BaseWindow *quicklist)
{
  _launcher->CancelActiveQuicklist (_quicklist);
}

void LauncherIcon::HideTooltip ()
{
  _tooltip->ShowWindow (false);
}

struct timespec LauncherIcon::ShowTime ()
{
  return _show_time;
}

struct timespec LauncherIcon::HideTime ()
{
  return _hide_time;
}

struct timespec LauncherIcon::RunningTime ()
{
  return _running_time;
}

struct timespec LauncherIcon::UrgentTime ()
{
  return _urgent_time;
}

struct timespec LauncherIcon::PresentTime ()
{
  return _present_time;
}

struct timespec LauncherIcon::UnpresentTime ()
{
  return _unpresent_time;
}

void
LauncherIcon::SetVisible (bool visible)
{
  if (visible == _visible)
    return;
      
  _visible = visible;
  
  needs_redraw.emit (this);

  if (visible)
  {
    Present (1500);
    clock_gettime (CLOCK_MONOTONIC, &_show_time);
    show.emit (this);
  }
  else
  {
    clock_gettime (CLOCK_MONOTONIC, &_hide_time);
    hide.emit (this);
  }
}

void
LauncherIcon::SetActive (bool active)
{
  if (active == _active)
    return;
    
  _active = active;
  needs_redraw.emit (this);
}

void 
LauncherIcon::SetRunning (bool running)
{
  if (running == _running)
    return;
    
  _running = running;
  clock_gettime (CLOCK_MONOTONIC, &_running_time);
  needs_redraw.emit (this);
}

void 
LauncherIcon::SetUrgent (bool urgent)
{
  if (urgent == _urgent)
    return;
  
  _urgent = urgent;
  
  if (urgent)
  {
      Present (1500);
      clock_gettime (CLOCK_MONOTONIC, &_urgent_time);
  }
  
  needs_redraw.emit (this);
}

gboolean
LauncherIcon::OnPresentTimeout (gpointer data)
{
  LauncherIcon *self = (LauncherIcon*) data;
  if (!self->_presented)
    return false;
  
  self->_present_time_handle = 0;
  self->Unpresent ();
  
  return false;
}

void 
LauncherIcon::Present (int length)
{
  if (_presented)
    return;
  
  _presented = true;
  
  _present_time_handle = g_timeout_add (length, &LauncherIcon::OnPresentTimeout, this);
  clock_gettime (CLOCK_MONOTONIC, &_present_time);
  needs_redraw.emit (this);
}

void
LauncherIcon::Unpresent ()
{
  if (!_presented)
    return;
  
  if (_present_time_handle > 0)
    g_source_remove (_present_time_handle);
  
  _presented = false;
  clock_gettime (CLOCK_MONOTONIC, &_unpresent_time);
  needs_redraw.emit (this);
}

void 
LauncherIcon::SetRelatedWindows (int windows)
{
  if (_related_windows == windows)
    return;
    
  _related_windows = windows;
  needs_redraw.emit (this);
}

void 
LauncherIcon::Remove ()
{
  SetVisible (false);
  remove.emit (this);
}

void 
LauncherIcon::SetIconType (LauncherIconType type)
{
  _icon_type = type;
}

void 
LauncherIcon::SetSortPriority (int priority)
{
  _sort_priority = priority;
}

int 
LauncherIcon::SortPriority ()
{
  return _sort_priority;
}
    
LauncherIconType 
LauncherIcon::Type ()
{
  return _icon_type;
}

bool
LauncherIcon::Visible ()
{
  return _visible;
}

bool
LauncherIcon::Active ()
{
  return _active;
}

bool
LauncherIcon::Running ()
{
  return _running;
}

bool
LauncherIcon::Urgent ()
{
  return _urgent;
}

bool
LauncherIcon::Presented ()
{
  return _presented;
}

int
LauncherIcon::RelatedWindows ()
{
  return _related_windows;
}

std::list<DbusmenuClient *> LauncherIcon::Menus ()
{
  return GetMenus ();
}

std::list<DbusmenuClient *> LauncherIcon::GetMenus ()
{
  std::list<DbusmenuClient *> result;

  return result;
}
