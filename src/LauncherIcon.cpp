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

#include "LauncherEntryRemoteModel.h"
#include "LauncherIcon.h"
#include "Launcher.h"

#include "QuicklistManager.h"
#include "QuicklistMenuItem.h"
#include "QuicklistMenuItemLabel.h"
#include "QuicklistMenuItemSeparator.h"
#include "QuicklistMenuItemCheckmark.h"
#include "QuicklistMenuItemRadio.h"

#include "ubus-server.h"
#include "UBusMessages.h"

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
  _shortcut = 0;
  
  _emblem = 0;
  _superkey_label = 0;

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
  MouseClick.connect (sigc::mem_fun (this, &LauncherIcon::RecvMouseClick));
}

LauncherIcon::~LauncherIcon()
{
  SetQuirk (QUIRK_URGENT, false);

  // Remove from introspection
  RemoveChild (_quicklist);
  RemoveChild (_tooltip);
  
  if (_present_time_handle)
    g_source_remove (_present_time_handle);
  _present_time_handle = 0;
  
  if (_center_stabilize_handle)
    g_source_remove (_center_stabilize_handle);
  _center_stabilize_handle = 0;

  if (_superkey_label)
    _superkey_label->UnReference ();

  // clean up the whole signal-callback mess
  if (on_icon_added_connection.connected ())
    on_icon_added_connection.disconnect ();

  if (on_icon_removed_connection.connected ())
    on_icon_removed_connection.disconnect ();

  if (on_order_changed_connection.connected ())
    on_order_changed_connection.disconnect ();
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

void
LauncherIcon::Activate ()
{
    if (PluginAdapter::Default ()->IsScaleActive())
      PluginAdapter::Default ()->TerminateScale ();
    
    ActivateLauncherIcon ();
}

void
LauncherIcon::OpenInstance ()
{
    if (PluginAdapter::Default ()->IsScaleActive())
      PluginAdapter::Default ()->TerminateScale ();
    
    OpenInstanceLauncherIcon ();
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

nux::BaseTexture * LauncherIcon::TextureFromGtkTheme (const char *icon_name, int size, bool update_glow_colors)
{
  GdkPixbuf *pbuf;
  GtkIconTheme *theme;
  GtkIconInfo *info;
  nux::BaseTexture *result;
  GError *error = NULL;
  GIcon *icon;

  if (!icon_name)
    icon_name = g_strdup (DEFAULT_ICON);
   
  theme = gtk_icon_theme_get_default ();
  icon = g_icon_new_for_string (icon_name, NULL);

  if (G_IS_ICON (icon))
  {
    info = gtk_icon_theme_lookup_by_gicon (theme, icon, size, (GtkIconLookupFlags)0);
    g_object_unref (icon);
  }
  else
  {   
    info = gtk_icon_theme_lookup_icon (theme,
                                       icon_name,
                                       size,
                                       (GtkIconLookupFlags) 0);
  }

  if (!info)
  {
    info = gtk_icon_theme_lookup_icon (theme,
                                       DEFAULT_ICON,
                                       size,
                                       (GtkIconLookupFlags) 0);
  }
        
  if (gtk_icon_info_get_filename (info) == NULL)
  {
    gtk_icon_info_free (info);

    info = gtk_icon_theme_lookup_icon (theme,
                                       DEFAULT_ICON,
                                       size,
                                       (GtkIconLookupFlags) 0);
  }

  pbuf = gtk_icon_info_load_icon (info, &error);
  gtk_icon_info_free (info);

  if (GDK_IS_PIXBUF (pbuf))
  {
    result = nux::CreateTexture2DFromPixbuf (pbuf, true);
    
    if (update_glow_colors)
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
      return TextureFromGtkTheme ("folder", size, update_glow_colors);
  }
  
  return result;
}

nux::BaseTexture * LauncherIcon::TextureFromPath (const char *icon_name, int size, bool update_glow_colors)
{

  GdkPixbuf *pbuf;
  nux::BaseTexture *result;
  GError *error = NULL;
  
  if (!icon_name)
    return TextureFromGtkTheme (DEFAULT_ICON, size, update_glow_colors);
  
  pbuf = gdk_pixbuf_new_from_file_at_size (icon_name, size, size, &error);

  if (GDK_IS_PIXBUF (pbuf))
  {
    result = nux::CreateTexture2DFromPixbuf (pbuf, true);
    
    if (update_glow_colors)
      ColorForIcon (pbuf, _background_color, _glow_color);
  
    g_object_unref (pbuf);
  }
  else
  {
    g_warning ("Unable to load '%s' icon: %s",
               icon_name,
               error->message);
    g_error_free (error);

    return TextureFromGtkTheme (DEFAULT_ICON, size, update_glow_colors);
  }
  
  return result;
}

void LauncherIcon::SetTooltipText(const TCHAR* text)
{
    m_TooltipText = g_markup_escape_text (text, -1);
    _tooltip->SetText (m_TooltipText);
}

nux::NString LauncherIcon::GetTooltipText()
{
    return m_TooltipText;
}

void
LauncherIcon::SetShortcut (guint64 shortcut)
{
  // only relocate a digit with a digit (don't overwrite other shortcuts)
  if ((!_shortcut || (g_ascii_isdigit ((gchar)_shortcut)))
        || !(g_ascii_isdigit ((gchar) shortcut)))
    _shortcut = shortcut;
}

guint64
LauncherIcon::GetShortcut ()
{
    return _shortcut;
}

void
LauncherIcon::RecvMouseEnter ()
{
  if (QuicklistManager::Default ()->Current ())
  {
    // A quicklist is active
    return;
  }
  
  nux::Geometry geo = _launcher->GetAbsoluteGeometry ();
  int tip_x = geo.x + geo.width + 1;
  int tip_y = geo.y + _center.y;
          
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

gboolean LauncherIcon::OpenQuicklist (bool default_to_first_item)
{
  _tooltip->ShowWindow (false);    
  _quicklist->RemoveAllMenuItem ();

  std::list<DbusmenuMenuitem *> menus = Menus ();
  if (menus.empty ())
    return false;
    
  if (PluginAdapter::Default ()->IsScaleActive())
    PluginAdapter::Default ()->TerminateScale ();

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

  if (default_to_first_item)
    _quicklist->DefaultToFirstItem ();

  nux::Geometry geo = _launcher->GetAbsoluteGeometry ();
  int tip_x = geo.x + geo.width + 1;
  int tip_y = geo.y + _center.y;
  QuicklistManager::Default ()->ShowQuicklist (_quicklist, tip_x, tip_y);
  
  return true;
}

void LauncherIcon::RecvMouseDown (int button)
{
  if (button == 3)
    OpenQuicklist ();
}

void LauncherIcon::RecvMouseUp (int button)
{
  if (button == 3)
  {
    if (_quicklist->IsVisible ())
      _quicklist->CaptureMouseDownAnyWhereElse (true);
  }
}

void LauncherIcon::RecvMouseClick (int button)
{
  if (button == 1)
    Activate ();
  else if (button == 2)
    OpenInstance ();
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
  
  nux::Geometry geo = _launcher->GetAbsoluteGeometry ();
  int tip_x = geo.x + geo.width + 1;
  int tip_y = geo.y + _center.y;
    
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
    Present (0.0f, 1500);
  if (quirk == QUIRK_URGENT)
  {
    if (value)
    {
      Present (0.0f, 1500);
    }
    
    UBusServer *ubus = ubus_server_get_default ();
    ubus_server_send_message (ubus, UBUS_LAUNCHER_ICON_URGENT_CHANGED, g_variant_new_boolean (value));
  }
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

void
LauncherIcon::SetSuperkeyLabel (nux::BaseTexture* label)
{
  if (_superkey_label == label)
    return;
  
  if (_superkey_label)
    _superkey_label->UnReference ();
  
  _superkey_label = label;  
}

nux::BaseTexture*
LauncherIcon::GetSuperkeyLabel ()
{
  return _superkey_label;
}

void 
LauncherIcon::SetEmblemIconName (const char *name)
{
  nux::BaseTexture *emblem;
  
  if (g_str_has_prefix (name, "/"))
    emblem = TextureFromPath (name, 22, false);
  else
    emblem = TextureFromGtkTheme (name, 22, false);
    
  SetEmblem (emblem);
}

void 
LauncherIcon::SetEmblemText (const char *text)
{
  if (text == NULL)
    return;

  nux::BaseTexture     *emblem;
  PangoLayout*          layout     = NULL;

  PangoContext*         pangoCtx   = NULL;
  PangoFontDescription* desc       = NULL;
  GdkScreen*            screen     = gdk_screen_get_default ();   // not ref'ed
  GtkSettings*          settings   = gtk_settings_get_default (); // not ref'ed
  gchar*                fontName   = NULL;

  int width = 32;
  int height = 8 * 2;
  int font_height = height - 5;
  
  
  nux::CairoGraphics *cg = new nux::CairoGraphics (CAIRO_FORMAT_ARGB32, width, height);
  cairo_t *cr = cg->GetContext ();

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  

  layout = pango_cairo_create_layout (cr);

  g_object_get (settings, "gtk-font-name", &fontName, NULL);
  desc = pango_font_description_from_string (fontName);
  pango_font_description_set_absolute_size (desc, pango_units_from_double (font_height));
  
  pango_layout_set_font_description (layout, desc);
  
  pango_layout_set_width (layout, pango_units_from_double (width - 4.0f));
  pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_NONE);
  pango_layout_set_markup_with_accel (layout, text, -1, '_', NULL);
  
  pangoCtx = pango_layout_get_context (layout); // is not ref'ed
  pango_cairo_context_set_font_options (pangoCtx,
                                        gdk_screen_get_font_options (screen));
                                        
  PangoRectangle logical_rect, ink_rect;
  pango_layout_get_extents (layout, &logical_rect, &ink_rect);
  
  /* DRAW OUTLINE */
  float radius = height / 2.0f - 1.0f;
  float inset = radius + 1.0f;

  cairo_move_to (cr, inset, height - 1.0f);
  cairo_arc (cr, inset, inset, radius, 0.5*M_PI, 1.5*M_PI);
  cairo_arc (cr, width - inset, inset, radius, 1.5*M_PI, 0.5*M_PI);
  cairo_line_to (cr, inset, height - 1.0f);
  
  cairo_set_source_rgba (cr, 0.35f, 0.35f, 0.35f, 1.0f);
  cairo_fill_preserve (cr);

  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cairo_set_line_width (cr, 2.0f);
  cairo_stroke (cr);
  
  cairo_set_line_width (cr, 1.0f);

  /* DRAW TEXT */
  cairo_move_to (cr, 
                 (int) ((width - pango_units_to_double (logical_rect.width)) / 2.0f), 
                 (int) ((height - pango_units_to_double (logical_rect.height)) / 2.0f - pango_units_to_double (logical_rect.y)));
  pango_cairo_show_layout (cr, layout);

  nux::NBitmapData* bitmap = cg->GetBitmap ();

  emblem = nux::GetThreadGLDeviceFactory()->CreateSystemCapableTexture ();
  emblem->Update (bitmap);
  delete bitmap;

  SetEmblem (emblem);

  // clean up
  g_object_unref (layout);
  g_free (fontName);
  delete cg;
}
    
void 
LauncherIcon::DeleteEmblem ()
{
  SetEmblem (0);
}

void 
LauncherIcon::InsertEntryRemote (LauncherEntryRemote *remote)
{
  if (std::find (_entry_list.begin (), _entry_list.end (), remote) != _entry_list.end ())
    return;
  
  _entry_list.push_front (remote);
  
  remote->emblem_changed.connect    (sigc::mem_fun(this, &LauncherIcon::OnRemoteEmblemChanged));
  remote->count_changed.connect     (sigc::mem_fun(this, &LauncherIcon::OnRemoteCountChanged));
  remote->progress_changed.connect  (sigc::mem_fun(this, &LauncherIcon::OnRemoteProgressChanged));
  remote->quicklist_changed.connect (sigc::mem_fun(this, &LauncherIcon::OnRemoteQuicklistChanged));
  
  remote->emblem_visible_changed.connect   (sigc::mem_fun(this, &LauncherIcon::OnRemoteEmblemVisibleChanged));
  remote->count_visible_changed.connect    (sigc::mem_fun(this, &LauncherIcon::OnRemoteCountVisibleChanged));
  remote->progress_visible_changed.connect (sigc::mem_fun(this, &LauncherIcon::OnRemoteProgressVisibleChanged));
  
  
  if (remote->EmblemVisible ())
    OnRemoteEmblemVisibleChanged (remote);
  
  if (remote->CountVisible ())
    OnRemoteCountVisibleChanged (remote);
    
  if (remote->ProgressVisible ())
    OnRemoteProgressVisibleChanged (remote);
}

void 
LauncherIcon::RemoveEntryRemote (LauncherEntryRemote *remote)
{
  if (std::find (_entry_list.begin (), _entry_list.end (), remote) == _entry_list.end ())
    return;
  
  _entry_list.remove (remote);
  
  DeleteEmblem ();
  SetQuirk (QUIRK_PROGRESS, false);
}

void
LauncherIcon::OnRemoteEmblemChanged (LauncherEntryRemote *remote)
{
  if (!remote->EmblemVisible ())
    return;
  
  SetEmblemIconName (remote->Emblem ());
}

void
LauncherIcon::OnRemoteCountChanged (LauncherEntryRemote *remote)
{
  if (!remote->CountVisible ())
    return;
  
  gchar *text = g_strdup_printf ("%i", (int) remote->Count ());
  SetEmblemText (text);
  g_free (text);
}

void
LauncherIcon::OnRemoteProgressChanged (LauncherEntryRemote *remote)
{
  if (!remote->ProgressVisible ())
    return;
  
  SetProgress ((float) remote->Progress ());
}

void
LauncherIcon::OnRemoteQuicklistChanged (LauncherEntryRemote *remote)
{
  // FIXME
}

void
LauncherIcon::OnRemoteEmblemVisibleChanged (LauncherEntryRemote *remote)
{
  if (remote->EmblemVisible ())
    SetEmblemIconName (remote->Emblem ());
  else
    DeleteEmblem ();
}

void
LauncherIcon::OnRemoteCountVisibleChanged (LauncherEntryRemote *remote)
{ 
  if (remote->CountVisible ())
  {
    gchar *text = g_strdup_printf ("%i", (int) remote->Count ());
    SetEmblemText (text);
    g_free (text);
  }
  else
  {
    DeleteEmblem ();
  }
}

void
LauncherIcon::OnRemoteProgressVisibleChanged (LauncherEntryRemote *remote)
{
  SetQuirk (QUIRK_PROGRESS, remote->ProgressVisible ());
  
  if (remote->ProgressVisible ())
    SetProgress ((float) remote->Progress ());
}
