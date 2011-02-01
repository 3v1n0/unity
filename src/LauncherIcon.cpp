// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#include "QuicklistManager.h"
#include "QuicklistMenuItem.h"
#include "QuicklistMenuItemLabel.h"
#include "QuicklistMenuItemSeparator.h"
#include "QuicklistMenuItemCheckmark.h"
#include "QuicklistMenuItemRadio.h"

#define DEFAULT_ICON "application-default-icon"

NUX_IMPLEMENT_OBJECT_TYPE (LauncherIcon);

nux::Tooltip *LauncherIcon::_current_tooltip = 0;
QuicklistView *LauncherIcon::_current_quicklist = 0;

LauncherIcon::LauncherIcon(Launcher* launcher)
{
  _folding_angle = 0;
  _launcher = launcher;
  m_TooltipText = "blank";

  for (int i = 0; i < QUIRK_LAST; i++)
  {
    _quirks[i] = 0;
    _quirk_times[i].tv_sec = 0;
    _quirk_times[i].tv_nsec = 0;
  }

  _related_windows = 0;

  _background_color = nux::Color::White;
  _glow_color = nux::Color::White;
  
  _mouse_inside = false;
  _has_visible_window = false;
  _tooltip = new nux::Tooltip ();
  _icon_type = TYPE_NONE;
  _sort_priority = 0;
  
  _emblem = 0;

  _quicklist = new QuicklistView ();
  _quicklist_is_initialized = false;
  
  _present_time_handle = 0;
  _center_stabilize_handle = 0;

  QuicklistManager::Default ()->RegisterQuicklist (_quicklist);
  
  // Add to introspection
  AddChild (_quicklist);
  AddChild (_tooltip);
  
  MouseEnter.connect (sigc::mem_fun(this, &LauncherIcon::RecvMouseEnter));
  MouseLeave.connect (sigc::mem_fun(this, &LauncherIcon::RecvMouseLeave));
  MouseDown.connect (sigc::mem_fun(this, &LauncherIcon::RecvMouseDown));
  MouseUp.connect (sigc::mem_fun(this, &LauncherIcon::RecvMouseUp));
  
}

LauncherIcon::~LauncherIcon()
{
  // Remove from introspection
  RemoveChild (_quicklist);
  RemoveChild (_tooltip);
  
  if (_present_time_handle)
    g_source_remove (_present_time_handle);
  _present_time_handle = 0;
  
  if (_center_stabilize_handle)
    g_source_remove (_center_stabilize_handle);
  _center_stabilize_handle = 0;
}

bool
LauncherIcon::HasVisibleWindow ()
{
  return _has_visible_window;
}

const gchar *
LauncherIcon::GetName ()
{
  return "LauncherIcon";
}

void
LauncherIcon::AddProperties (GVariantBuilder *builder)
{
  g_variant_builder_add (builder, "{sv}", "x", _center.x);
  g_variant_builder_add (builder, "{sv}", "y", _center.y);
  g_variant_builder_add (builder, "{sv}", "z", _center.z);
  g_variant_builder_add (builder, "{sv}", "related-windows", g_variant_new_int32 (_related_windows));
  g_variant_builder_add (builder, "{sv}", "icon-type", g_variant_new_int32 (_icon_type));
  g_variant_builder_add (builder, "{sv}", "tooltip-text", g_variant_new_string (m_TooltipText.GetTCharPtr ()));
  
  g_variant_builder_add (builder, "{sv}", "sort-priority", g_variant_new_int32 (_sort_priority));
  g_variant_builder_add (builder, "{sv}", "quirk-active", g_variant_new_boolean (GetQuirk (QUIRK_ACTIVE)));
  g_variant_builder_add (builder, "{sv}", "quirk-visible", g_variant_new_boolean (GetQuirk (QUIRK_VISIBLE)));
  g_variant_builder_add (builder, "{sv}", "quirk-urgent", g_variant_new_boolean (GetQuirk (QUIRK_URGENT)));
  g_variant_builder_add (builder, "{sv}", "quirk-running", g_variant_new_boolean (GetQuirk (QUIRK_RUNNING)));
  g_variant_builder_add (builder, "{sv}", "quirk-presented", g_variant_new_boolean (GetQuirk (QUIRK_PRESENTED)));
}

nux::Color LauncherIcon::BackgroundColor ()
{
  return _background_color;
}

nux::Color LauncherIcon::GlowColor ()
{
  return _glow_color;
}

nux::BaseTexture * LauncherIcon::TextureForSize (int size)
{
  nux::BaseTexture * result = GetTextureForSize (size);
  return result;
}

void LauncherIcon::ColorForIcon (GdkPixbuf *pixbuf, nux::Color &background, nux::Color &glow)
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
    s = 0.65f;
  v = 0.90f;
  
  nux::HSVtoRGB (r, g, b, h, s, v);
  background = nux::Color (r, g, b);
  
  v = 1.0f;
  nux::HSVtoRGB (r, g, b, h, s, v);
  glow = nux::Color (r, g, b);
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
    ColorForIcon (pbuf, _background_color, _glow_color);
  
    g_object_unref (pbuf);
  }
  else
  {
    g_warning ("Unable to load '%s' from icon theme: %s",
               icon_name,
               error ? error->message : "unknown");
    g_error_free (error);

    if (g_strcmp0 (icon_name, "folder") == 0)
      return NULL;
    else
      return TextureFromGtkTheme ("folder", size);
  }
  
  return result;
}

nux::BaseTexture * LauncherIcon::TextureFromPath (const char *icon_name, int size)
{

  GdkPixbuf *pbuf;
  nux::BaseTexture *result;
  GError *error = NULL;
  
  if (!icon_name)
    return TextureFromGtkTheme (DEFAULT_ICON, size);
  
  pbuf = gdk_pixbuf_new_from_file_at_size (icon_name, size, size, &error);

  if (GDK_IS_PIXBUF (pbuf))
  {
    result = nux::CreateTextureFromPixbuf (pbuf); 
    ColorForIcon (pbuf, _background_color, _glow_color);
  
    g_object_unref (pbuf);
  }
  else
  {
    g_warning ("Unable to load '%s' icon: %s",
               icon_name,
               error->message);
    g_error_free (error);

    return TextureFromGtkTheme (DEFAULT_ICON, size);
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
  if (QuicklistManager::Default ()->Current ())
  {
    // A quicklist is active
    return;
  }
  
  int tip_x = _launcher->GetBaseWidth () + 1; //icon_x + icon_w;
  int tip_y = _center.y + _launcher->GetParent ()->GetGeometry ().y;
          
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

void LauncherIcon::RecvMouseDown (int button)
{
  if (button == 3)
  {
    _tooltip->ShowWindow (false);
    
    _quicklist->RemoveAllMenuItem ();
    
    std::list<DbusmenuMenuitem *> menus = Menus ();
    if (menus.empty ())
      return;

    std::list<DbusmenuMenuitem *>::iterator it;
    for (it = menus.begin (); it != menus.end (); it++)
    {
      DbusmenuMenuitem *menu_item = *it;
    
      const gchar* type = dbusmenu_menuitem_property_get (menu_item, DBUSMENU_MENUITEM_PROP_TYPE);
      const gchar* toggle_type = dbusmenu_menuitem_property_get (menu_item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE);

      if (g_strcmp0 (type, DBUSMENU_CLIENT_TYPES_SEPARATOR) == 0)
      {
        QuicklistMenuItemSeparator* item = new QuicklistMenuItemSeparator (menu_item, NUX_TRACKER_LOCATION);
        _quicklist->AddMenuItem (item);
      }
      else if (g_strcmp0 (toggle_type, DBUSMENU_MENUITEM_TOGGLE_CHECK) == 0)
      {
        QuicklistMenuItemCheckmark* item = new QuicklistMenuItemCheckmark (menu_item, NUX_TRACKER_LOCATION);
        _quicklist->AddMenuItem (item);
      }
      else if (g_strcmp0 (toggle_type, DBUSMENU_MENUITEM_TOGGLE_RADIO) == 0)
      {
        QuicklistMenuItemRadio* item = new QuicklistMenuItemRadio (menu_item, NUX_TRACKER_LOCATION);
        _quicklist->AddMenuItem (item);
      }
      else //(g_strcmp0 (type, DBUSMENU_MENUITEM_PROP_LABEL) == 0)
      {
        QuicklistMenuItemLabel* item = new QuicklistMenuItemLabel (menu_item, NUX_TRACKER_LOCATION);
        _quicklist->AddMenuItem (item);
      }
    } 
    
    int tip_x = _launcher->GetBaseWidth () + 1; //icon_x + icon_w;
    int tip_y = _center.y + _launcher->GetParent ()->GetGeometry ().y;
    QuicklistManager::Default ()->ShowQuicklist (_quicklist, tip_x, tip_y);
    nux::GetWindowCompositor ().SetAlwaysOnFrontWindow (_quicklist);
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

void LauncherIcon::HideTooltip ()
{
  _tooltip->ShowWindow (false);
}

gboolean
LauncherIcon::OnCenterTimeout (gpointer data)
{
  LauncherIcon *self = (LauncherIcon*)data;
  
  if (self->_last_stable != self->_center)
  {
    self->OnCenterStabilized (self->_center);
    self->_last_stable = self->_center;
  }

  self->_center_stabilize_handle = 0;
  return false;
}

void 
LauncherIcon::SetCenter (nux::Point3 center)
{
  _center = center;
  
  int tip_x = _launcher->GetBaseWidth () + 1; //icon_x + icon_w;
  int tip_y = _center.y + _launcher->GetParent ()->GetGeometry ().y;
    
  if (_quicklist->IsVisible ())
    QuicklistManager::Default ()->ShowQuicklist (_quicklist, tip_x, tip_y);
  else if (_tooltip->IsVisible ())
    _tooltip->ShowTooltipWithTipAt (tip_x, tip_y);
    
  if (_center_stabilize_handle)
    g_source_remove (_center_stabilize_handle);
  
  _center_stabilize_handle = g_timeout_add (500, &LauncherIcon::OnCenterTimeout, this);
}

nux::Point3
LauncherIcon::GetCenter ()
{
  return _center;
}

void
LauncherIcon::SaveCenter ()
{
  _saved_center = _center;
  UpdateQuirkTime (QUIRK_CENTER_SAVED);
}

void 
LauncherIcon::SetHasVisibleWindow (bool val)
{
  if (_has_visible_window == val)
    return;
  
  _has_visible_window = val;
  needs_redraw.emit (this);
}

gboolean
LauncherIcon::OnPresentTimeout (gpointer data)
{
  LauncherIcon *self = (LauncherIcon*) data;
  if (!self->GetQuirk (QUIRK_PRESENTED))
    return false;
  
  self->_present_time_handle = 0;
  self->Unpresent ();
  
  return false;
}

float LauncherIcon::PresentUrgency ()
{
  return _present_urgency;
}

void 
LauncherIcon::Present (float present_urgency, int length)
{
  if (GetQuirk (QUIRK_PRESENTED))
    return;
  
  if (length >= 0)
    _present_time_handle = g_timeout_add (length, &LauncherIcon::OnPresentTimeout, this);
  
  _present_urgency = CLAMP (present_urgency, 0.0f, 1.0f);
  SetQuirk (QUIRK_PRESENTED, true);
}

void
LauncherIcon::Unpresent ()
{
  if (!GetQuirk (QUIRK_PRESENTED))
    return;
  
  if (_present_time_handle > 0)
    g_source_remove (_present_time_handle);
  
  SetQuirk (QUIRK_PRESENTED, false);
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
  SetQuirk (QUIRK_VISIBLE, false);
  remove.emit (this);
}

void 
LauncherIcon::SetIconType (IconType type)
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
    
LauncherIcon::IconType 
LauncherIcon::Type ()
{
  return _icon_type;
}

bool
LauncherIcon::GetQuirk (LauncherIcon::Quirk quirk)
{
  return _quirks[quirk];
}

void
LauncherIcon::SetQuirk (LauncherIcon::Quirk quirk, bool value)
{
  if (_quirks[quirk] == value)
    return;
    
  _quirks[quirk] = value;
  if (quirk == QUIRK_VISIBLE)
    Launcher::SetTimeStruct (&(_quirk_times[quirk]), &(_quirk_times[quirk]), ANIM_DURATION_SHORT);
  else
    clock_gettime (CLOCK_MONOTONIC, &(_quirk_times[quirk]));
  needs_redraw.emit (this);
  
  // Present on urgent as a general policy
  if (quirk == QUIRK_VISIBLE && value)
    Present (0.5f, 1500);
  if (quirk == QUIRK_URGENT && value)
    Present (0.5f, 1500);
}

gboolean
LauncherIcon::OnDelayedUpdateTimeout (gpointer data)
{
  DelayedUpdateArg *arg = (DelayedUpdateArg *) data;
  LauncherIcon *self = arg->self;

  clock_gettime (CLOCK_MONOTONIC, &(self->_quirk_times[arg->quirk]));
  self->needs_redraw.emit (self);
  
  return false;
}

void
LauncherIcon::UpdateQuirkTimeDelayed (guint ms, LauncherIcon::Quirk quirk)
{
  DelayedUpdateArg *arg = new DelayedUpdateArg ();
  arg->self = this;
  arg->quirk = quirk;
  
  g_timeout_add (ms, &LauncherIcon::OnDelayedUpdateTimeout, arg);
}

void
LauncherIcon::UpdateQuirkTime (LauncherIcon::Quirk quirk)
{
  clock_gettime (CLOCK_MONOTONIC, &(_quirk_times[quirk]));
  needs_redraw.emit (this);
}

void 
LauncherIcon::ResetQuirkTime (LauncherIcon::Quirk quirk)
{
  _quirk_times[quirk].tv_sec = 0;
  _quirk_times[quirk].tv_nsec = 0;
}

struct timespec
LauncherIcon::GetQuirkTime (LauncherIcon::Quirk quirk)
{
  return _quirk_times[quirk];
}

int
LauncherIcon::RelatedWindows ()
{
  return _related_windows;
}

void
LauncherIcon::SetProgress (float progress)
{
  if (progress == _progress)
    return;
  
  _progress = progress;
  needs_redraw.emit (this);
}

float
LauncherIcon::GetProgress ()
{
  return _progress;
}

std::list<DbusmenuMenuitem *> LauncherIcon::Menus ()
{
  return GetMenus ();
}

std::list<DbusmenuMenuitem *> LauncherIcon::GetMenus ()
{
  std::list<DbusmenuMenuitem *> result;
  return result;
}

nux::BaseTexture *
LauncherIcon::Emblem ()
{
  return _emblem;
}

void 
LauncherIcon::SetEmblem (nux::BaseTexture *emblem)
{
  if (_emblem == emblem)
    return;
  
  if (_emblem)
    _emblem->UnReference ();
  
  _emblem = emblem;
  needs_redraw.emit (this);
}
