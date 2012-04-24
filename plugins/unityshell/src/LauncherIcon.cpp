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

#include <Nux/Nux.h>
#include <Nux/VScrollBar.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <Nux/MenuPage.h>
#include <Nux/WindowCompositor.h>
#include <Nux/BaseWindow.h>
#include <Nux/MenuPage.h>
#include <NuxCore/Color.h>
#include <NuxCore/Logger.h>

#include "CairoTexture.h"
#include "LauncherIcon.h"
#include "Launcher.h"
#include "TimeUtil.h"

#include "QuicklistManager.h"
#include "QuicklistMenuItem.h"
#include "QuicklistMenuItemLabel.h"
#include "QuicklistMenuItemSeparator.h"
#include "QuicklistMenuItemCheckmark.h"
#include "QuicklistMenuItemRadio.h"

#include "MultiMonitor.h"
#include "WindowManager.h"

#include "ubus-server.h"
#include "UBusMessages.h"
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Variant.h>

namespace unity
{
namespace launcher
{

namespace
{
nux::logging::Logger logger("unity.launcher");
const std::string DEFAULT_ICON = "application-default-icon";
const std::string MONO_TEST_ICON = "gnome-home";
const std::string UNITY_THEME_NAME = "unity-icon-theme";
}

NUX_IMPLEMENT_OBJECT_TYPE(LauncherIcon);

int LauncherIcon::_current_theme_is_mono = -1;
glib::Object<GtkIconTheme> LauncherIcon::_unity_theme;

LauncherIcon::LauncherIcon()
  : _remote_urgent(false)
  , _present_urgency(0)
  , _progress(0)
  , _center_stabilize_handle(0)
  , _present_time_handle(0)
  , _time_delay_handle(0)
  , _sort_priority(0)
  , _last_monitor(0)
  , _background_color(nux::color::White)
  , _glow_color(nux::color::White)
  , _shortcut(0)
  , _icon_type(TYPE_NONE)
  , _center(max_num_monitors)
  , _has_visible_window(max_num_monitors)
  , _last_stable(max_num_monitors)
  , _parent_geo(max_num_monitors)
  , _saved_center(max_num_monitors)
{
  for (int i = 0; i < QUIRK_LAST; i++)
  {
    _quirks[i] = false;
    _quirk_times[i].tv_sec = 0;
    _quirk_times[i].tv_nsec = 0;
  }

  _is_visible_on_monitor.resize(max_num_monitors);

  for (int i = 0; i < max_num_monitors; ++i)
    _is_visible_on_monitor[i] = true;

  tooltip_text.SetSetterFunction(sigc::mem_fun(this, &LauncherIcon::SetTooltipText));
  tooltip_text = "blank";

  // FIXME: the abstraction is already broken, should be fixed for O
  // right now, hooking the dynamic quicklist the less ugly possible way


  mouse_enter.connect(sigc::mem_fun(this, &LauncherIcon::RecvMouseEnter));
  mouse_leave.connect(sigc::mem_fun(this, &LauncherIcon::RecvMouseLeave));
  mouse_down.connect(sigc::mem_fun(this, &LauncherIcon::RecvMouseDown));
  mouse_up.connect(sigc::mem_fun(this, &LauncherIcon::RecvMouseUp));
  mouse_click.connect(sigc::mem_fun(this, &LauncherIcon::RecvMouseClick));
}

LauncherIcon::~LauncherIcon()
{
  SetQuirk(QUIRK_URGENT, false);

  if (_present_time_handle)
    g_source_remove(_present_time_handle);
  _present_time_handle = 0;

  if (_center_stabilize_handle)
    g_source_remove(_center_stabilize_handle);
  _center_stabilize_handle = 0;

  if (_time_delay_handle)
    g_source_remove(_time_delay_handle);
  _time_delay_handle = 0;

  // clean up the whole signal-callback mess
  if (needs_redraw_connection.connected())
    needs_redraw_connection.disconnect();

  if (on_icon_added_connection.connected())
    on_icon_added_connection.disconnect();

  if (on_icon_removed_connection.connected())
    on_icon_removed_connection.disconnect();

  if (on_order_changed_connection.connected())
    on_order_changed_connection.disconnect();

  if (_unity_theme)
  {
    _unity_theme = NULL;
  }
}

void LauncherIcon::LoadTooltip()
{
  _tooltip = new Tooltip();
  AddChild(_tooltip.GetPointer());

  _tooltip->SetText(tooltip_text());
}

void LauncherIcon::LoadQuicklist()
{
  _quicklist = new QuicklistView();
  AddChild(_quicklist.GetPointer());

  QuicklistManager::Default()->RegisterQuicklist(_quicklist.GetPointer());
}

const bool
LauncherIcon::WindowVisibleOnMonitor(int monitor)
{
  return _has_visible_window[monitor];
}

const bool LauncherIcon::WindowVisibleOnViewport()
{
  for (int i = 0; i < max_num_monitors; ++i)
    if (_has_visible_window[i])
      return true;

  return false;
}

std::string
LauncherIcon::GetName() const
{
  return "LauncherIcon";
}

void
LauncherIcon::AddProperties(GVariantBuilder* builder)
{
  GVariantBuilder monitors_builder;
  g_variant_builder_init(&monitors_builder, G_VARIANT_TYPE ("ab"));

  for (int i = 0; i < max_num_monitors; ++i)
    g_variant_builder_add(&monitors_builder, "b", IsVisibleOnMonitor(i));

  unity::variant::BuilderWrapper(builder)
  .add("center_x", _center[0].x)
  .add("center_y", _center[0].y)
  .add("center_z", _center[0].z)
  .add("related_windows", static_cast<unsigned int>(Windows().size()))
  .add("icon_type", _icon_type)
  .add("tooltip_text", tooltip_text())
  .add("sort_priority", _sort_priority)
  .add("shortcut", _shortcut)
  .add("monitors_visibility", g_variant_builder_end(&monitors_builder))
  .add("active", GetQuirk(QUIRK_ACTIVE))
  .add("visible", GetQuirk(QUIRK_VISIBLE))
  .add("urgent", GetQuirk(QUIRK_URGENT))
  .add("running", GetQuirk(QUIRK_RUNNING))
  .add("starting", GetQuirk(QUIRK_STARTING))
  .add("desaturated", GetQuirk(QUIRK_DESAT))
  .add("presented", GetQuirk(QUIRK_PRESENTED));
}

void
LauncherIcon::Activate(ActionArg arg)
{
  /* Launcher Icons that handle spread will adjust the spread state
   * accordingly, for all other icons we should terminate spread */
  if (WindowManager::Default()->IsScaleActive() && !HandlesSpread ())
    WindowManager::Default()->TerminateScale();

  ActivateLauncherIcon(arg);

  UpdateQuirkTime(QUIRK_LAST_ACTION);
}

void
LauncherIcon::OpenInstance(ActionArg arg)
{
  if (WindowManager::Default()->IsScaleActive())
    WindowManager::Default()->TerminateScale();

  OpenInstanceLauncherIcon(arg);

  UpdateQuirkTime(QUIRK_LAST_ACTION);
}

nux::Color LauncherIcon::BackgroundColor()
{
  return _background_color;
}

nux::Color LauncherIcon::GlowColor()
{
  return _glow_color;
}

nux::BaseTexture* LauncherIcon::TextureForSize(int size)
{
  nux::BaseTexture* result = GetTextureForSize(size);
  return result;
}

void LauncherIcon::ColorForIcon(GdkPixbuf* pixbuf, nux::Color& background, nux::Color& glow)
{
  unsigned int width = gdk_pixbuf_get_width(pixbuf);
  unsigned int height = gdk_pixbuf_get_height(pixbuf);
  unsigned int row_bytes = gdk_pixbuf_get_rowstride(pixbuf);

  long int rtotal = 0, gtotal = 0, btotal = 0;
  float total = 0.0f;

  guchar* img = gdk_pixbuf_get_pixels(pixbuf);

  for (unsigned int i = 0; i < width; i++)
  {
    for (unsigned int j = 0; j < height; j++)
    {
      guchar* pixels = img + (j * row_bytes + i * 4);
      guchar r = *(pixels + 0);
      guchar g = *(pixels + 1);
      guchar b = *(pixels + 2);
      guchar a = *(pixels + 3);

      float saturation = (MAX(r, MAX(g, b)) - MIN(r, MIN(g, b))) / 255.0f;
      float relevance = .1 + .9 * (a / 255.0f) * saturation;

      rtotal += (guchar)(r * relevance);
      gtotal += (guchar)(g * relevance);
      btotal += (guchar)(b * relevance);

      total += relevance * 255;
    }
  }

  nux::color::RedGreenBlue rgb(rtotal / total,
                               gtotal / total,
                               btotal / total);
  nux::color::HueSaturationValue hsv(rgb);

  if (hsv.saturation > 0.15f)
    hsv.saturation = 0.65f;

  hsv.value = 0.90f;
  background = nux::Color(nux::color::RedGreenBlue(hsv));

  hsv.value = 1.0f;
  glow = nux::Color(nux::color::RedGreenBlue(hsv));
}

/*
 * FIXME, all this code (and below), should be put in a facility for IconLoader
 * to share between launcher and places the same Icon loading logic and not look
 * having etoomanyimplementationofsamethings.
 */
/* static */
bool LauncherIcon::IsMonoDefaultTheme()
{

  if (_current_theme_is_mono != -1)
    return (bool)_current_theme_is_mono;

  GtkIconTheme* default_theme;
  GtkIconInfo* info;
  int size = 48;

  default_theme = gtk_icon_theme_get_default();

  _current_theme_is_mono = (int)false;
  info = gtk_icon_theme_lookup_icon(default_theme, MONO_TEST_ICON.c_str(), size, (GtkIconLookupFlags)0);

  if (!info)
    return (bool)_current_theme_is_mono;

  // yeah, it's evil, but it's blessed upstream
  if (g_strrstr(gtk_icon_info_get_filename(info), "ubuntu-mono") != NULL)
    _current_theme_is_mono = (int)true;

  gtk_icon_info_free(info);
  return (bool)_current_theme_is_mono;

}

GtkIconTheme* LauncherIcon::GetUnityTheme()
{
  // The theme object is invalid as soon as you add a new icon to change the theme.
  // invalidate the cache then and rebuild the theme the first time after a icon theme update.
  if (!GTK_IS_ICON_THEME(_unity_theme.RawPtr()))
  {
    _unity_theme =  gtk_icon_theme_new();
    gtk_icon_theme_set_custom_theme(_unity_theme, UNITY_THEME_NAME.c_str());
  }
  return _unity_theme;
}

nux::BaseTexture* LauncherIcon::TextureFromGtkTheme(std::string icon_name, int size, bool update_glow_colors)
{
  GtkIconTheme* default_theme;
  nux::BaseTexture* result = NULL;

  if (icon_name.empty())
  {
    icon_name = DEFAULT_ICON;
  }

  default_theme = gtk_icon_theme_get_default();

  // FIXME: we need to create some kind of -unity postfix to see if we are looking to the unity-icon-theme
  // for dedicated unity icons, then remove the postfix and degrade to other icon themes if not found
  if (icon_name == "workspace-switcher" && IsMonoDefaultTheme())
    result = TextureFromSpecificGtkTheme(GetUnityTheme(), icon_name, size, update_glow_colors);

  if (!result)
    result = TextureFromSpecificGtkTheme(default_theme, icon_name, size, update_glow_colors, true);

  if (!result)
  {
    if (icon_name == "folder")
      result = NULL;
    else
      result = TextureFromSpecificGtkTheme(default_theme, "folder", size, update_glow_colors);
  }

  return result;

}

nux::BaseTexture* LauncherIcon::TextureFromSpecificGtkTheme(GtkIconTheme* theme,
                                                            std::string const& icon_name,
                                                            int size,
                                                            bool update_glow_colors,
                                                            bool is_default_theme)
{
  GtkIconInfo* info;
  nux::BaseTexture* result = NULL;
  GIcon* icon;
  GtkIconLookupFlags flags = (GtkIconLookupFlags) 0;

  icon = g_icon_new_for_string(icon_name.c_str(), NULL);

  if (G_IS_ICON(icon))
  {
    info = gtk_icon_theme_lookup_by_gicon(theme, icon, size, flags);
    g_object_unref(icon);
  }
  else
  {
    info = gtk_icon_theme_lookup_icon(theme, icon_name.c_str(), size, flags);
  }

  if (!info && !is_default_theme)
    return NULL;

  if (!info)
  {
    info = gtk_icon_theme_lookup_icon(theme, DEFAULT_ICON.c_str(), size, flags);
  }

  if (gtk_icon_info_get_filename(info) == NULL)
  {
    gtk_icon_info_free(info);
    info = gtk_icon_theme_lookup_icon(theme, DEFAULT_ICON.c_str(), size, flags);
  }

  glib::Error error;
  glib::Object<GdkPixbuf> pbuf(gtk_icon_info_load_icon(info, &error));
  gtk_icon_info_free(info);

  if (GDK_IS_PIXBUF(pbuf.RawPtr()))
  {
    result = nux::CreateTexture2DFromPixbuf(pbuf, true);

    if (update_glow_colors)
      ColorForIcon(pbuf, _background_color, _glow_color);
  }
  else
  {
    LOG_WARN(logger) << "Unable to load '" << icon_name
                     <<  "' from icon theme: " << error;
  }

  return result;
}

nux::BaseTexture* LauncherIcon::TextureFromPath(std::string const& icon_name, int size, bool update_glow_colors)
{
  nux::BaseTexture* result;

  if (icon_name.empty())
    return TextureFromGtkTheme(DEFAULT_ICON, size, update_glow_colors);

  glib::Error error;
  glib::Object<GdkPixbuf> pbuf(gdk_pixbuf_new_from_file_at_size(icon_name.c_str(), size, size, &error));

  if (GDK_IS_PIXBUF(pbuf.RawPtr()))
  {
    result = nux::CreateTexture2DFromPixbuf(pbuf, true);

    if (update_glow_colors)
      ColorForIcon(pbuf, _background_color, _glow_color);
  }
  else
  {
    LOG_WARN(logger) << "Unable to load '" << icon_name
                     <<  "' icon: " << error;

    result = TextureFromGtkTheme(DEFAULT_ICON, size, update_glow_colors);
  }

  return result;
}

bool LauncherIcon::SetTooltipText(std::string& target, std::string const& value)
{
  bool result = false;

  gchar* esc = g_markup_escape_text(value.c_str(), -1);
  std::string escaped = esc;
  g_free(esc);

  if (escaped != target)
  {
    target = escaped;
    if (_tooltip)
      _tooltip->SetText(target);
    result = true;
  }

  return result;
}

void
LauncherIcon::SetShortcut(guint64 shortcut)
{
  // only relocate a digit with a digit (don't overwrite other shortcuts)
  if ((!_shortcut || (g_ascii_isdigit((gchar)_shortcut)))
      || !(g_ascii_isdigit((gchar) shortcut)))
    _shortcut = shortcut;
}

guint64
LauncherIcon::GetShortcut()
{
  return _shortcut;
}

void
LauncherIcon::ShowTooltip()
{
  if (_quicklist && _quicklist->IsVisible())
    return;

  int tip_x = 100;
  int tip_y = 100;
  if (_last_monitor >= 0)
  {
    nux::Geometry geo = _parent_geo[_last_monitor];
    tip_x = geo.x + geo.width - 4 * geo.width / 48;
    tip_y = _center[_last_monitor].y;
  }

  if (!_tooltip)
    LoadTooltip();
  _tooltip->ShowTooltipWithTipAt(tip_x, tip_y);
  _tooltip->ShowWindow(!tooltip_text().empty());
}

void
LauncherIcon::RecvMouseEnter(int monitor)
{
  _last_monitor = monitor;
  if (QuicklistManager::Default()->Current())
  {
    // A quicklist is active
    return;
  }

  ShowTooltip();
}

void LauncherIcon::RecvMouseLeave(int monitor)
{
  _last_monitor = -1;

  if (_tooltip)
    _tooltip->ShowWindow(false);
}

bool LauncherIcon::OpenQuicklist(bool select_first_item, int monitor)
{
  std::list<DbusmenuMenuitem*> menus = Menus();

  if (!_quicklist)
    LoadQuicklist();

  if (menus.empty())
    return false;

  if (_tooltip)
    _tooltip->ShowWindow(false);
  _quicklist->RemoveAllMenuItem();

  for (auto menu_item : menus)
  {
    QuicklistMenuItem* ql_item;

    const gchar* type = dbusmenu_menuitem_property_get(menu_item, DBUSMENU_MENUITEM_PROP_TYPE);
    const gchar* toggle_type = dbusmenu_menuitem_property_get(menu_item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE);
    gboolean prop_visible = dbusmenu_menuitem_property_get_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE);

    // Skip this item, it is invisible right now.
    if (!prop_visible)
      continue;

    if (g_strcmp0(type, DBUSMENU_CLIENT_TYPES_SEPARATOR) == 0)
    {
      ql_item = new QuicklistMenuItemSeparator(menu_item, NUX_TRACKER_LOCATION);
    }
    else if (g_strcmp0(toggle_type, DBUSMENU_MENUITEM_TOGGLE_CHECK) == 0)
    {
      ql_item = new QuicklistMenuItemCheckmark(menu_item, NUX_TRACKER_LOCATION);
    }
    else if (g_strcmp0(toggle_type, DBUSMENU_MENUITEM_TOGGLE_RADIO) == 0)
    {
      ql_item = new QuicklistMenuItemRadio(menu_item, NUX_TRACKER_LOCATION);
    }
    else //(g_strcmp0 (type, DBUSMENU_MENUITEM_PROP_LABEL) == 0)
    {
      ql_item = new QuicklistMenuItemLabel(menu_item, NUX_TRACKER_LOCATION);
    }

    _quicklist->AddMenuItem(ql_item);
  }

  if (select_first_item)
    _quicklist->SelectFirstItem();

  if (monitor < 0)
  {
    if (_last_monitor >= 0)
      monitor = _last_monitor;
    else
      monitor = 0;
  }

  nux::Geometry geo = _parent_geo[monitor];
  int tip_x = geo.x + geo.width - 4 * geo.width / 48;
  int tip_y = _center[monitor].y;

  auto win_manager = WindowManager::Default();

  if (win_manager->IsScaleActive())
    win_manager->TerminateScale();

  /* If the expo plugin is active, we need to wait it to be termated, before
   * shwing the icon quicklist. */
  if (win_manager->IsExpoActive())
  {
    on_expo_terminated_connection = win_manager->terminate_expo.connect([&, tip_x, tip_y]() {
        QuicklistManager::Default()->ShowQuicklist(_quicklist.GetPointer(), tip_x, tip_y);
        on_expo_terminated_connection.disconnect();
    });
  }
  else
  {
    QuicklistManager::Default()->ShowQuicklist(_quicklist.GetPointer(), tip_x, tip_y);
  }

  return true;
}

void LauncherIcon::RecvMouseDown(int button, int monitor)
{
  if (button == 3)
    OpenQuicklist();
}

void LauncherIcon::RecvMouseUp(int button, int monitor)
{
  if (button == 3)
  {
    if (_quicklist && _quicklist->IsVisible())
      _quicklist->CaptureMouseDownAnyWhereElse(true);
  }
}

void LauncherIcon::RecvMouseClick(int button, int monitor)
{
  ActionArg arg(ActionArg::LAUNCHER, button);
  arg.monitor = monitor;

  if (button == 1)
    Activate(arg);
  else if (button == 2)
    OpenInstance(arg);
}

void LauncherIcon::HideTooltip()
{
  if (_tooltip)
    _tooltip->ShowWindow(false);
}

gboolean
LauncherIcon::OnCenterTimeout(gpointer data)
{
  LauncherIcon* self = (LauncherIcon*)data;

  if (!std::equal(self->_center.begin(), self->_center.end(), self->_last_stable.begin()))
  {
    self->OnCenterStabilized(self->_center);
    self->_last_stable = self->_center;
  }

  self->_center_stabilize_handle = 0;
  return false;
}

void
LauncherIcon::SetCenter(nux::Point3 center, int monitor, nux::Geometry geo)
{
  center.x += geo.x;
  center.y += geo.y;
  _center[monitor] = center;
  _parent_geo[monitor] = geo;

  if (monitor == _last_monitor)
  {
    int tip_x, tip_y;
    tip_x = geo.x + geo.width - 4 * geo.width / 48;
    tip_y = _center[monitor].y;

    if (_quicklist && _quicklist->IsVisible())
      QuicklistManager::Default()->ShowQuicklist(_quicklist.GetPointer(), tip_x, tip_y);
    else if (_tooltip && _tooltip->IsVisible())
      _tooltip->ShowTooltipWithTipAt(tip_x, tip_y);
  }

  if (_center_stabilize_handle)
    g_source_remove(_center_stabilize_handle);

  _center_stabilize_handle = g_timeout_add(500, &LauncherIcon::OnCenterTimeout, this);
}

nux::Point3
LauncherIcon::GetCenter(int monitor)
{
  return _center[monitor];
}

nux::Point3
LauncherIcon::GetSavedCenter(int monitor)
{
  return _saved_center[monitor];
}

std::vector<nux::Point3> LauncherIcon::GetCenters()
{
  return _center;
}

void
LauncherIcon::SaveCenter()
{
  _saved_center = _center;
  UpdateQuirkTime(QUIRK_CENTER_SAVED);
}

void
LauncherIcon::SetWindowVisibleOnMonitor(bool val, int monitor)
{
  if (_has_visible_window[monitor] == val)
    return;

  _has_visible_window[monitor] = val;
  EmitNeedsRedraw();
}

void
LauncherIcon::SetVisibleOnMonitor(int monitor, bool visible)
{
  if (_is_visible_on_monitor[monitor] == visible)
    return;

  _is_visible_on_monitor[monitor] = visible;
  EmitNeedsRedraw();
}

bool
LauncherIcon::IsVisibleOnMonitor(int monitor) const
{
  return _is_visible_on_monitor[monitor];
}

gboolean
LauncherIcon::OnPresentTimeout(gpointer data)
{
  LauncherIcon* self = (LauncherIcon*) data;
  if (!self->GetQuirk(QUIRK_PRESENTED))
    return false;

  self->_present_time_handle = 0;
  self->Unpresent();

  return false;
}

float LauncherIcon::PresentUrgency()
{
  return _present_urgency;
}

void
LauncherIcon::Present(float present_urgency, int length)
{
  if (GetQuirk(QUIRK_PRESENTED))
    return;

  if (length >= 0)
    _present_time_handle = g_timeout_add(length, &LauncherIcon::OnPresentTimeout, this);

  _present_urgency = CLAMP(present_urgency, 0.0f, 1.0f);
  SetQuirk(QUIRK_PRESENTED, true);
}

void
LauncherIcon::Unpresent()
{
  if (!GetQuirk(QUIRK_PRESENTED))
    return;

  if (_present_time_handle > 0)
    g_source_remove(_present_time_handle);
  _present_time_handle = 0;

  SetQuirk(QUIRK_PRESENTED, false);
}

void
LauncherIcon::Remove()
{
  if (_quicklist && _quicklist->IsVisible())
      _quicklist->Hide();

  SetQuirk(QUIRK_VISIBLE, false);
  EmitRemove();
}

void
LauncherIcon::SetIconType(IconType type)
{
  _icon_type = type;
}

void
LauncherIcon::SetSortPriority(int priority)
{
  _sort_priority = priority;
}

int
LauncherIcon::SortPriority()
{
  return _sort_priority;
}

LauncherIcon::IconType
LauncherIcon::GetIconType()
{
  return _icon_type;
}

bool
LauncherIcon::GetQuirk(LauncherIcon::Quirk quirk) const
{
  return _quirks[quirk];
}

void
LauncherIcon::SetQuirk(LauncherIcon::Quirk quirk, bool value)
{
  if (_quirks[quirk] == value)
    return;

  _quirks[quirk] = value;
  if (quirk == QUIRK_VISIBLE)
    TimeUtil::SetTimeStruct(&(_quirk_times[quirk]), &(_quirk_times[quirk]), Launcher::ANIM_DURATION_SHORT);
  else
    clock_gettime(CLOCK_MONOTONIC, &(_quirk_times[quirk]));
  EmitNeedsRedraw();

  // Present on urgent as a general policy
  if (quirk == QUIRK_VISIBLE && value)
    Present(0.5f, 1500);
  if (quirk == QUIRK_URGENT)
  {
    if (value)
    {
      Present(0.5f, 1500);
    }

    UBusServer* ubus = ubus_server_get_default();
    ubus_server_send_message(ubus, UBUS_LAUNCHER_ICON_URGENT_CHANGED, g_variant_new_boolean(value));
  }

  if (quirk == QUIRK_VISIBLE)
  {
     visibility_changed.emit();
  }
}

gboolean
LauncherIcon::OnDelayedUpdateTimeout(gpointer data)
{
  DelayedUpdateArg* arg = (DelayedUpdateArg*) data;
  LauncherIcon* self = arg->self;

  clock_gettime(CLOCK_MONOTONIC, &(self->_quirk_times[arg->quirk]));
  self->EmitNeedsRedraw();

  self->_time_delay_handle = 0;

  return false;
}

void
LauncherIcon::UpdateQuirkTimeDelayed(guint ms, LauncherIcon::Quirk quirk)
{
  DelayedUpdateArg* arg = new DelayedUpdateArg();
  arg->self = this;
  arg->quirk = quirk;

  _time_delay_handle = g_timeout_add(ms, &LauncherIcon::OnDelayedUpdateTimeout, arg);
}

void
LauncherIcon::UpdateQuirkTime(LauncherIcon::Quirk quirk)
{
  clock_gettime(CLOCK_MONOTONIC, &(_quirk_times[quirk]));
  EmitNeedsRedraw();
}

void
LauncherIcon::ResetQuirkTime(LauncherIcon::Quirk quirk)
{
  _quirk_times[quirk].tv_sec = 0;
  _quirk_times[quirk].tv_nsec = 0;
}

struct timespec
LauncherIcon::GetQuirkTime(LauncherIcon::Quirk quirk)
{
  return _quirk_times[quirk];
}

void
LauncherIcon::SetProgress(float progress)
{
  if (progress == _progress)
    return;

  _progress = progress;
  EmitNeedsRedraw();
}

float
LauncherIcon::GetProgress()
{
  return _progress;
}

std::list<DbusmenuMenuitem*> LauncherIcon::Menus()
{
  return GetMenus();
}

std::list<DbusmenuMenuitem*> LauncherIcon::GetMenus()
{
  std::list<DbusmenuMenuitem*> result;
  return result;
}

nux::BaseTexture*
LauncherIcon::Emblem()
{
  return _emblem.GetPointer();
}

void
LauncherIcon::SetEmblem(LauncherIcon::BaseTexturePtr const& emblem)
{
  _emblem = emblem;
  EmitNeedsRedraw();
}

void
LauncherIcon::SetEmblemIconName(std::string const& name)
{
  BaseTexturePtr emblem;

  if (name.at(0) == '/')
    emblem = TextureFromPath(name, 22, false);
  else
    emblem = TextureFromGtkTheme(name, 22, false);

  SetEmblem(emblem);
  // Ownership isn't taken, but shared, so we need to unref here.
  emblem->UnReference();
}

void
LauncherIcon::SetEmblemText(std::string const& text)
{
  PangoLayout*          layout     = NULL;

  PangoContext*         pangoCtx   = NULL;
  PangoFontDescription* desc       = NULL;
  GdkScreen*            screen     = gdk_screen_get_default();    // not ref'ed
  GtkSettings*          settings   = gtk_settings_get_default();  // not ref'ed
  gchar*                fontName   = NULL;

  int width = 32;
  int height = 8 * 2;
  int font_height = height - 5;


  nux::CairoGraphics cg(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t* cr = cg.GetInternalContext();

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);


  layout = pango_cairo_create_layout(cr);

  g_object_get(settings, "gtk-font-name", &fontName, NULL);
  desc = pango_font_description_from_string(fontName);
  pango_font_description_set_absolute_size(desc, pango_units_from_double(font_height));

  pango_layout_set_font_description(layout, desc);
  pango_font_description_free(desc);

  pango_layout_set_width(layout, pango_units_from_double(width - 4.0f));
  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);
  pango_layout_set_markup_with_accel(layout, text.c_str(), -1, '_', NULL);

  pangoCtx = pango_layout_get_context(layout);  // is not ref'ed
  pango_cairo_context_set_font_options(pangoCtx,
                                       gdk_screen_get_font_options(screen));

  PangoRectangle logical_rect, ink_rect;
  pango_layout_get_extents(layout, &logical_rect, &ink_rect);

  /* DRAW OUTLINE */
  float radius = height / 2.0f - 1.0f;
  float inset = radius + 1.0f;

  cairo_move_to(cr, inset, height - 1.0f);
  cairo_arc(cr, inset, inset, radius, 0.5 * M_PI, 1.5 * M_PI);
  cairo_arc(cr, width - inset, inset, radius, 1.5 * M_PI, 0.5 * M_PI);
  cairo_line_to(cr, inset, height - 1.0f);

  cairo_set_source_rgba(cr, 0.35f, 0.35f, 0.35f, 1.0f);
  cairo_fill_preserve(cr);

  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);
  cairo_set_line_width(cr, 2.0f);
  cairo_stroke(cr);

  cairo_set_line_width(cr, 1.0f);

  /* DRAW TEXT */
  cairo_move_to(cr,
                (int)((width - pango_units_to_double(logical_rect.width)) / 2.0f),
                (int)((height - pango_units_to_double(logical_rect.height)) / 2.0f - pango_units_to_double(logical_rect.y)));
  pango_cairo_show_layout(cr, layout);

  SetEmblem(texture_ptr_from_cairo_graphics(cg));

  // clean up
  g_object_unref(layout);
  g_free(fontName);
}

void
LauncherIcon::DeleteEmblem()
{
  SetEmblem(BaseTexturePtr());
}

void
LauncherIcon::InsertEntryRemote(LauncherEntryRemote::Ptr const& remote)
{
  if (std::find(_entry_list.begin(), _entry_list.end(), remote) != _entry_list.end())
    return;

  _entry_list.push_front(remote);

  remote->emblem_changed.connect(sigc::mem_fun(this, &LauncherIcon::OnRemoteEmblemChanged));
  remote->count_changed.connect(sigc::mem_fun(this, &LauncherIcon::OnRemoteCountChanged));
  remote->progress_changed.connect(sigc::mem_fun(this, &LauncherIcon::OnRemoteProgressChanged));
  remote->quicklist_changed.connect(sigc::mem_fun(this, &LauncherIcon::OnRemoteQuicklistChanged));

  remote->emblem_visible_changed.connect(sigc::mem_fun(this, &LauncherIcon::OnRemoteEmblemVisibleChanged));
  remote->count_visible_changed.connect(sigc::mem_fun(this, &LauncherIcon::OnRemoteCountVisibleChanged));
  remote->progress_visible_changed.connect(sigc::mem_fun(this, &LauncherIcon::OnRemoteProgressVisibleChanged));

  remote->urgent_changed.connect(sigc::mem_fun(this, &LauncherIcon::OnRemoteUrgentChanged));


  if (remote->EmblemVisible())
    OnRemoteEmblemVisibleChanged(remote.get());

  if (remote->CountVisible())
    OnRemoteCountVisibleChanged(remote.get());

  if (remote->ProgressVisible())
    OnRemoteProgressVisibleChanged(remote.get());

  if (remote->Urgent())
    OnRemoteUrgentChanged(remote.get());

  OnRemoteQuicklistChanged(remote.get());
}

void
LauncherIcon::RemoveEntryRemote(LauncherEntryRemote::Ptr const& remote)
{
  if (std::find(_entry_list.begin(), _entry_list.end(), remote) == _entry_list.end())
    return;

  _entry_list.remove(remote);

  DeleteEmblem();
  SetQuirk(QUIRK_PROGRESS, false);

  if (_remote_urgent)
    SetQuirk(QUIRK_URGENT, false);

  _menuclient_dynamic_quicklist = nullptr;
}

void
LauncherIcon::OnRemoteUrgentChanged(LauncherEntryRemote* remote)
{
  _remote_urgent = remote->Urgent();
  SetQuirk(QUIRK_URGENT, remote->Urgent());
}

void
LauncherIcon::OnRemoteEmblemChanged(LauncherEntryRemote* remote)
{
  if (!remote->EmblemVisible())
    return;

  SetEmblemIconName(remote->Emblem());
}

void
LauncherIcon::OnRemoteCountChanged(LauncherEntryRemote* remote)
{
  if (!remote->CountVisible())
    return;

  std::string text;
  if (remote->Count() > 9999)
    text = "****";
  else
    text = std::to_string(remote->Count());

  SetEmblemText(text);
}

void
LauncherIcon::OnRemoteProgressChanged(LauncherEntryRemote* remote)
{
  if (!remote->ProgressVisible())
    return;

  SetProgress(remote->Progress());
}

void
LauncherIcon::OnRemoteQuicklistChanged(LauncherEntryRemote* remote)
{
  _menuclient_dynamic_quicklist = remote->Quicklist();
}

void
LauncherIcon::OnRemoteEmblemVisibleChanged(LauncherEntryRemote* remote)
{
  if (remote->EmblemVisible())
    SetEmblemIconName(remote->Emblem());
  else
    DeleteEmblem();
}

void
LauncherIcon::OnRemoteCountVisibleChanged(LauncherEntryRemote* remote)
{
  if (remote->CountVisible())
  {
    SetEmblemText(std::to_string(remote->Count()));
  }
  else
  {
    DeleteEmblem();
  }
}

void
LauncherIcon::OnRemoteProgressVisibleChanged(LauncherEntryRemote* remote)
{
  SetQuirk(QUIRK_PROGRESS, remote->ProgressVisible());

  if (remote->ProgressVisible())
    SetProgress(remote->Progress());
}

void LauncherIcon::EmitNeedsRedraw()
{
  if (OwnsTheReference() && GetReferenceCount() > 0)
    needs_redraw.emit(AbstractLauncherIcon::Ptr(this));
}

void LauncherIcon::EmitRemove()
{
  if (OwnsTheReference() && GetReferenceCount() > 0)
    remove.emit(AbstractLauncherIcon::Ptr(this));
}


} // namespace launcher
} // namespace unity
