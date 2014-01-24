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
#include <NuxCore/Color.h>
#include <NuxCore/Logger.h>

#include "LauncherIcon.h"
#include "unity-shared/AnimationUtils.h"
#include "unity-shared/CairoTexture.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UScreen.h"

#include "QuicklistManager.h"
#include "QuicklistMenuItem.h"
#include "QuicklistMenuItemLabel.h"
#include "QuicklistMenuItemSeparator.h"
#include "QuicklistMenuItemCheckmark.h"
#include "QuicklistMenuItemRadio.h"

#include "MultiMonitor.h"

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GTKWrapper.h>
#include <UnityCore/Variant.h>

namespace unity
{
namespace launcher
{
DECLARE_LOGGER(logger, "unity.launcher.icon");

namespace
{
const int IGNORE_REPEAT_SHORTCUT_DURATION = 250;

const std::string DEFAULT_ICON = "application-default-icon";
const std::string MONO_TEST_ICON = "gnome-home";
const std::string UNITY_THEME_NAME = "unity-icon-theme";

const std::string CENTER_STABILIZE_TIMEOUT = "center-stabilize-timeout";
const std::string PRESENT_TIMEOUT = "present-timeout";
const std::string QUIRK_DELAY_TIMEOUT = "quirk-delay-timeout";
}

NUX_IMPLEMENT_OBJECT_TYPE(LauncherIcon);

int LauncherIcon::_current_theme_is_mono = -1;
glib::Object<GtkIconTheme> LauncherIcon::_unity_theme;

LauncherIcon::LauncherIcon(IconType type)
  : _icon_type(type)
  , _sticky(false)
  , _remote_urgent(false)
  , _present_urgency(0)
  , _progress(0.0f)
  , _sort_priority(DefaultPriority(type))
  , _order(0)
  , _last_monitor(0)
  , _background_color(nux::color::White)
  , _glow_color(nux::color::White)
  , _shortcut(0)
  , _allow_quicklist_to_show(true)
  , _center(monitors::MAX)
  , _quirks(monitors::MAX)
  , _quirk_animations(monitors::MAX, decltype(_quirk_animations)::value_type(unsigned(Quirk::LAST)))
  , _last_stable(monitors::MAX)
  , _saved_center(monitors::MAX)
{
  tooltip_enabled = true;
  tooltip_enabled.changed.connect(sigc::mem_fun(this, &LauncherIcon::OnTooltipEnabledChanged));
  tooltip_text.SetSetterFunction(sigc::mem_fun(this, &LauncherIcon::SetTooltipText));

  position = Position::FLOATING;
  removed = false;

  // FIXME: the abstraction is already broken, should be fixed for O
  // right now, hooking the dynamic quicklist the less ugly possible way
  mouse_enter.connect(sigc::mem_fun(this, &LauncherIcon::RecvMouseEnter));
  mouse_leave.connect(sigc::mem_fun(this, &LauncherIcon::RecvMouseLeave));
  mouse_down.connect(sigc::mem_fun(this, &LauncherIcon::RecvMouseDown));
  mouse_up.connect(sigc::mem_fun(this, &LauncherIcon::RecvMouseUp));
  mouse_click.connect(sigc::mem_fun(this, &LauncherIcon::RecvMouseClick));

  panel::Style::Instance().changed.connect(sigc::mem_fun(this, &LauncherIcon::LoadTooltip));

  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    for (unsigned j = 0; j < static_cast<unsigned>(Quirk::LAST); ++j)
    {
      _quirk_animations[i][j] = std::make_shared<Animation>();
      _quirk_animations[i][j]->updated.connect([this, i, j] (float value) { EmitNeedsRedraw(i); });
    }

    // Center must have set a default value
    animation::SetValue(GetQuirkAnimation(Quirk::CENTER_SAVED, i), animation::Direction::FORWARD);
  }
}

void LauncherIcon::LoadTooltip()
{
  _tooltip = new Tooltip();
  _tooltip->SetOpacity(0.0f);
  AddChild(_tooltip.GetPointer());

  _tooltip->text = tooltip_text();
}

void LauncherIcon::LoadQuicklist()
{
  _quicklist = new QuicklistView();
  AddChild(_quicklist.GetPointer());

  _quicklist->mouse_down_outside_pointer_grab_area.connect([this] (int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    _allow_quicklist_to_show = false;
  });

  QuicklistManager::Default()->RegisterQuicklist(_quicklist);
}

const bool LauncherIcon::WindowVisibleOnMonitor(int monitor)
{
  return _has_visible_window[monitor];
}

const bool LauncherIcon::WindowVisibleOnViewport()
{
  return _has_visible_window.any();
}

std::string
LauncherIcon::GetName() const
{
  return "LauncherIcon";
}

void LauncherIcon::AddProperties(debug::IntrospectionData& introspection)
{
  std::vector<bool> monitors_active,
                    monitors_visible,
                    monitors_urgent,
                    monitors_running,
                    monitors_starting,
                    monitors_desaturated,
                    monitors_presented;

  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    monitors_active.push_back(GetQuirk(Quirk::ACTIVE, i));
    monitors_visible.push_back(IsVisibleOnMonitor(i));
    monitors_urgent.push_back(GetQuirk(Quirk::URGENT, i));
    monitors_running.push_back(GetQuirk(Quirk::RUNNING, i));
    monitors_starting.push_back(GetQuirk(Quirk::STARTING, i));
    monitors_desaturated.push_back(GetQuirk(Quirk::DESAT, i));
    monitors_presented.push_back(GetQuirk(Quirk::PRESENTED, i));
  }

  introspection
  .add("center", _center[unity::UScreen::GetDefault()->GetMonitorWithMouse()])
  .add("related_windows", Windows().size())
  .add("icon_type", unsigned(_icon_type))
  .add("tooltip_text", tooltip_text())
  .add("sort_priority", _sort_priority)
  .add("shortcut", _shortcut)
  .add("order", _order)
  .add("monitors_active", glib::Variant::FromVector(monitors_active))
  .add("monitors_visibility", glib::Variant::FromVector(monitors_visible))
  .add("monitors_urgent", glib::Variant::FromVector(monitors_urgent))
  .add("monitors_running", glib::Variant::FromVector(monitors_running))
  .add("monitors_starting", glib::Variant::FromVector(monitors_starting))
  .add("monitors_desaturated", glib::Variant::FromVector(monitors_desaturated))
  .add("monitors_presented", glib::Variant::FromVector(monitors_presented))
  .add("active", GetQuirk(Quirk::ACTIVE))
  .add("visible", GetQuirk(Quirk::VISIBLE))
  .add("urgent", GetQuirk(Quirk::URGENT))
  .add("running", GetQuirk(Quirk::RUNNING))
  .add("starting", GetQuirk(Quirk::STARTING))
  .add("desaturated", GetQuirk(Quirk::DESAT))
  .add("presented", GetQuirk(Quirk::PRESENTED));
}

bool LauncherIcon::IsActionArgValid(ActionArg const& arg)
{
  if (arg.source != ActionArg::Source::LAUNCHER_KEYBINDING)
    return true;

  time::Spec now;
  now.SetToNow();

  return (now.TimeDelta(_last_action) > IGNORE_REPEAT_SHORTCUT_DURATION);
}

void LauncherIcon::Activate(ActionArg arg)
{
  if (!IsActionArgValid(arg))
    return;

  /* Launcher Icons that handle spread will adjust the spread state
   * accordingly, for all other icons we should terminate spread */
  WindowManager& wm = WindowManager::Default();
  if (wm.IsScaleActive() && !HandlesSpread())
    wm.TerminateScale();

  ActivateLauncherIcon(arg);

  _last_action.SetToNow();
}

void LauncherIcon::OpenInstance(ActionArg arg)
{
  if (!IsActionArgValid(arg))
    return;

  WindowManager& wm = WindowManager::Default();
  if (wm.IsScaleActive())
    wm.TerminateScale();

  OpenInstanceLauncherIcon(arg.timestamp);

  _last_action.SetToNow();
}

nux::Color LauncherIcon::BackgroundColor() const
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
  gtk::IconInfo info;
  default_theme = gtk_icon_theme_get_default();

  _current_theme_is_mono = (int)false;
  info = gtk_icon_theme_lookup_icon(default_theme, MONO_TEST_ICON.c_str(), icon_size(), (GtkIconLookupFlags)0);

  if (!info)
    return (bool)_current_theme_is_mono;

  // yeah, it's evil, but it's blessed upstream
  if (g_strrstr(gtk_icon_info_get_filename(info), "ubuntu-mono") != NULL)
    _current_theme_is_mono = (int)true;

  return (bool)_current_theme_is_mono;
}

GtkIconTheme* LauncherIcon::GetUnityTheme()
{
  // The theme object is invalid as soon as you add a new icon to change the theme.
  // invalidate the cache then and rebuild the theme the first time after a icon theme update.
  if (!_unity_theme.IsType(GTK_TYPE_ICON_THEME))
  {
    _unity_theme = gtk_icon_theme_new();
    gtk_icon_theme_set_custom_theme(_unity_theme, UNITY_THEME_NAME.c_str());
  }
  return _unity_theme;
}

BaseTexturePtr LauncherIcon::TextureFromGtkTheme(std::string icon_name, int size, bool update_glow_colors)
{
  GtkIconTheme* default_theme;
  BaseTexturePtr result;

  if (icon_name.empty())
    icon_name = DEFAULT_ICON;

  default_theme = gtk_icon_theme_get_default();

  // FIXME: we need to create some kind of -unity postfix to see if we are looking to the unity-icon-theme
  // for dedicated unity icons, then remove the postfix and degrade to other icon themes if not found
  if (icon_name.find("workspace-switcher") == 0)
    result = TextureFromSpecificGtkTheme(GetUnityTheme(), icon_name, size, update_glow_colors);

  if (!result)
    result = TextureFromSpecificGtkTheme(default_theme, icon_name, size, update_glow_colors, true);

  if (!result)
  {
    if (icon_name != "folder")
      result = TextureFromSpecificGtkTheme(default_theme, "folder", size, update_glow_colors);
  }

  return result;
}

BaseTexturePtr LauncherIcon::TextureFromSpecificGtkTheme(GtkIconTheme* theme,
                                                         std::string const& icon_name,
                                                         int size,
                                                         bool update_glow_colors,
                                                         bool is_default_theme)
{
  glib::Object<GIcon> icon(g_icon_new_for_string(icon_name.c_str(), nullptr));
  gtk::IconInfo info;
  auto flags = static_cast<GtkIconLookupFlags>(0);

  if (icon.IsType(G_TYPE_ICON))
  {
    info = gtk_icon_theme_lookup_by_gicon(theme, icon, size, flags);
  }
  else
  {
    info = gtk_icon_theme_lookup_icon(theme, icon_name.c_str(), size, flags);
  }

  if (!info && !is_default_theme)
    return BaseTexturePtr();

  if (!info)
  {
    info = gtk_icon_theme_lookup_icon(theme, DEFAULT_ICON.c_str(), size, flags);
  }

  if (!gtk_icon_info_get_filename(info))
  {
    info = gtk_icon_theme_lookup_icon(theme, DEFAULT_ICON.c_str(), size, flags);
  }

  glib::Error error;
  glib::Object<GdkPixbuf> pbuf(gtk_icon_info_load_icon(info, &error));

  if (pbuf.IsType(GDK_TYPE_PIXBUF))
  {
    if (update_glow_colors)
      ColorForIcon(pbuf, _background_color, _glow_color);

    BaseTexturePtr result;
    result.Adopt(nux::CreateTexture2DFromPixbuf(pbuf, true));

    return result;
  }
  else
  {
    LOG_WARN(logger) << "Unable to load '" << icon_name
                     <<  "' from icon theme: " << error;
  }

  return BaseTexturePtr();
}

BaseTexturePtr LauncherIcon::TextureFromPath(std::string const& icon_name, int size, bool update_glow_colors)
{
  if (icon_name.empty())
    return TextureFromGtkTheme(DEFAULT_ICON, size, update_glow_colors);

  glib::Error error;
  glib::Object<GdkPixbuf> pbuf(gdk_pixbuf_new_from_file_at_size(icon_name.c_str(), size, size, &error));

  if (GDK_IS_PIXBUF(pbuf.RawPtr()))
  {
    if (update_glow_colors)
      ColorForIcon(pbuf, _background_color, _glow_color);

    BaseTexturePtr result;
    result.Adopt(nux::CreateTexture2DFromPixbuf(pbuf, true));
    return result;
  }
  else
  {
    LOG_WARN(logger) << "Unable to load '" << icon_name
                     <<  "' icon: " << error;

    return TextureFromGtkTheme(DEFAULT_ICON, size, update_glow_colors);
  }

  return BaseTexturePtr();
}

bool LauncherIcon::SetTooltipText(std::string& target, std::string const& value)
{
  auto const& escaped = glib::String(g_markup_escape_text(value.c_str(), -1)).Str();

  if (escaped != target)
  {
    target = escaped;

    if (_tooltip)
      _tooltip->text = target;

    return true;
  }

  return false;
}

void LauncherIcon::OnTooltipEnabledChanged(bool value)
{
  if (!value)
    HideTooltip();
}

void LauncherIcon::SetShortcut(guint64 shortcut)
{
  // only relocate a digit with a digit (don't overwrite other shortcuts)
  if ((!_shortcut || (g_ascii_isdigit((gchar)_shortcut)))
      || !(g_ascii_isdigit((gchar) shortcut)))
    _shortcut = shortcut;
}

guint64 LauncherIcon::GetShortcut()
{
  return _shortcut;
}

nux::Point LauncherIcon::GetTipPosition(int monitor) const
{
  return nux::Point(_center[monitor].x + icon_size()/2 + 1, _center[monitor].y);
}

void LauncherIcon::ShowTooltip()
{
  if (!tooltip_enabled || tooltip_text().empty() || (_quicklist && _quicklist->IsVisible()))
    return;

  if (!_tooltip)
    LoadTooltip();

  auto const& pos = GetTipPosition(_last_monitor);
  _tooltip->text = tooltip_text();
  _tooltip->ShowTooltipWithTipAt(pos.x, pos.y);
  tooltip_visible.emit(_tooltip);
}

void LauncherIcon::RecvMouseEnter(int monitor)
{
  _last_monitor = monitor;
}

void LauncherIcon::RecvMouseLeave(int monitor)
{
  _last_monitor = -1;
  _allow_quicklist_to_show = true;
}

bool LauncherIcon::OpenQuicklist(bool select_first_item, int monitor)
{
  MenuItemsVector const& menus = Menus();

  if (!_quicklist)
    LoadQuicklist();

  if (menus.empty())
    return false;

  if (_tooltip)
  {
    // Hide the tooltip without fade animation
    _tooltip->ShowWindow(false);
  }

  _quicklist->RemoveAllMenuItem();

  for (auto const& menu_item : menus)
  {
    QuicklistMenuItem* ql_item = nullptr;

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

  WindowManager& win_manager = WindowManager::Default();

  if (win_manager.IsScaleActive())
    win_manager.TerminateScale();

  auto const& pos = GetTipPosition(monitor);

  /* If the expo plugin is active, we need to wait it to be terminated, before
   * showing the icon quicklist. */
  if (win_manager.IsExpoActive())
  {
    auto conn = std::make_shared<sigc::connection>();
    *conn = win_manager.terminate_expo.connect([this, conn, pos] {
      QuicklistManager::Default()->ShowQuicklist(_quicklist, pos.x, pos.y);
      conn->disconnect();
    });
  }
  else
  {
    QuicklistManager::Default()->ShowQuicklist(_quicklist, pos.x, pos.y);
  }

  return true;
}

void LauncherIcon::CloseQuicklist()
{
  _quicklist->HideAndEndQuicklistNav();
}

void LauncherIcon::RecvMouseDown(int button, int monitor, unsigned long key_flags)
{
  if (button == 3)
    OpenQuicklist(false, monitor);
}

void LauncherIcon::RecvMouseUp(int button, int monitor, unsigned long key_flags)
{
  if (button == 3)
  {
    if (_allow_quicklist_to_show)
    {
      OpenQuicklist(false, monitor);
    }

    if (_quicklist && _quicklist->IsVisible())
    {
      _quicklist->CaptureMouseDownAnyWhereElse(true);
    }
  }
  _allow_quicklist_to_show = true;
}

void LauncherIcon::RecvMouseClick(int button, int monitor, unsigned long key_flags)
{
  auto timestamp = nux::GetGraphicsDisplay()->GetCurrentEvent().x11_timestamp;

  ActionArg arg(ActionArg::Source::LAUNCHER, button, timestamp);
  arg.monitor = monitor;

  bool shift_pressed = nux::GetKeyModifierState(key_flags, nux::NUX_STATE_SHIFT);

  // Click without shift
  if (button == 1 && !shift_pressed)
    Activate(arg);
  // Middle click or click with shift
  else if ((button == 2) || (button == 1 && shift_pressed))
    OpenInstance(arg);
}

void LauncherIcon::HideTooltip()
{
  if (_tooltip)
    _tooltip->Hide();

  tooltip_visible.emit(nux::ObjectPtr<nux::View>());
}

void LauncherIcon::SetCenter(nux::Point3 const& new_center, int monitor)
{
  nux::Point3& center = _center[monitor];

  if (center == new_center)
    return;

  center = new_center;

  if (monitor == _last_monitor)
  {
    if (_quicklist && _quicklist->IsVisible())
    {
      auto const& pos = GetTipPosition(monitor);
      QuicklistManager::Default()->MoveQuicklist(_quicklist, pos.x, pos.y);
    }
    else if (_tooltip && _tooltip->IsVisible())
    {
      auto const& pos = GetTipPosition(monitor);
      _tooltip->SetTooltipPosition(pos.x, pos.y);
    }
  }

  _source_manager.AddTimeout(500, [this] {
    if (!std::equal(_center.begin(), _center.end(), _last_stable.begin()))
    {
      if (!removed())
        OnCenterStabilized(_center);

      _last_stable = _center;
    }

    return false;
  }, CENTER_STABILIZE_TIMEOUT + std::to_string(monitor));
}

void LauncherIcon::ResetCenters(int monitor)
{
  if (monitor < 0)
  {
    for (unsigned i = 0; i < monitors::MAX; ++i)
      _center[i].Set(0, 0, 0);
  }
  else
  {
    _center[monitor].Set(0, 0, 0);
  }
}

nux::Point3 LauncherIcon::GetCenter(int monitor)
{
  return _center[monitor];
}

nux::Point3 LauncherIcon::GetSavedCenter(int monitor)
{
  return _saved_center[monitor];
}

std::vector<nux::Point3> LauncherIcon::GetCenters()
{
  return _center;
}

void LauncherIcon::SaveCenter()
{
  _saved_center = _center;
  FullyAnimateQuirk(Quirk::CENTER_SAVED, 0);
}

std::pair<int, nux::Point3> LauncherIcon::GetCenterForMonitor(int monitor) const
{
  monitor = CLAMP(monitor, 0, static_cast<int>(_center.size() - 1));

  if (_center[monitor].x && _center[monitor].y)
    return {monitor, _center[monitor]};

  for (unsigned i = 0; i < _center.size(); ++i)
  {
    if (_center[i].x && _center[i].y)
      return {i, _center[i]};
  }

  return {-1, nux::Point3()};
}

void LauncherIcon::SetWindowVisibleOnMonitor(bool val, int monitor)
{
  if (_has_visible_window[monitor] == val)
    return;

  _has_visible_window[monitor] = val;
  EmitNeedsRedraw(monitor);
}

void LauncherIcon::SetVisibleOnMonitor(int monitor, bool visible)
{
  SetQuirk(Quirk::VISIBLE, visible, monitor);
}

bool LauncherIcon::IsVisibleOnMonitor(int monitor) const
{
  return GetQuirk(Quirk::VISIBLE, monitor);
}

float LauncherIcon::PresentUrgency()
{
  return _present_urgency;
}

void LauncherIcon::Present(float present_urgency, int length, int monitor)
{
  if (GetQuirk(Quirk::PRESENTED, monitor))
    return;

  if (length >= 0)
  {
    _source_manager.AddTimeout(length, [this, monitor] {
      if (!GetQuirk(Quirk::PRESENTED, monitor))
        return false;

      Unpresent(monitor);
      return false;
    }, PRESENT_TIMEOUT + std::to_string(monitor));
  }

  _present_urgency = CLAMP(present_urgency, 0.0f, 1.0f);
  SetQuirk(Quirk::PRESENTED, true, monitor);
  SetQuirk(Quirk::UNFOLDED, true, monitor);
}

void LauncherIcon::Unpresent(int monitor)
{
  if (!GetQuirk(Quirk::PRESENTED, monitor))
    return;

  _source_manager.Remove(PRESENT_TIMEOUT + std::to_string(monitor));
  SetQuirk(Quirk::PRESENTED, false, monitor);
  SetQuirk(Quirk::UNFOLDED, false, monitor);
}

void LauncherIcon::Remove()
{
  if (_quicklist && _quicklist->IsVisible())
      _quicklist->Hide();

  if (_tooltip && _tooltip->IsVisible())
    _tooltip->Hide();

  SetQuirk(Quirk::VISIBLE, false);
  EmitRemove();

  // Disconnect all the callbacks that may interact with the icon data
  _source_manager.RemoveAll();
  sigc::trackable::notify_callbacks();

  removed = true;
}

void LauncherIcon::SetSortPriority(int priority)
{
  _sort_priority = priority;
}

int LauncherIcon::SortPriority()
{
  return _sort_priority;
}

void LauncherIcon::SetOrder(int order)
{
  _order = order;
}

LauncherIcon::IconType
LauncherIcon::GetIconType() const
{
  return _icon_type;
}

bool LauncherIcon::GetQuirk(LauncherIcon::Quirk quirk, int monitor) const
{
  if (monitor < 0)
  {
    for (unsigned i = 0; i < monitors::MAX; ++i)
    {
      if (!_quirks[i][unsigned(quirk)])
        return false;
    }

    return true;
  }

  return _quirks[monitor][unsigned(quirk)];
}

void LauncherIcon::SetQuirk(LauncherIcon::Quirk quirk, bool value, int monitor)
{
  bool changed = false;

  if (monitor < 0)
  {
    for (unsigned i = 0; i < monitors::MAX; ++i)
    {
      if (_quirks[i][unsigned(quirk)] != value)
      {
        _quirks[i][unsigned(quirk)] = value;
        animation::StartOrReverseIf(GetQuirkAnimation(quirk, i), value);
        changed = true;
      }
    }
  }
  else
  {
    if (_quirks[monitor][unsigned(quirk)] != value)
    {
      _quirks[monitor][unsigned(quirk)] = value;
      animation::StartOrReverseIf(GetQuirkAnimation(quirk, monitor), value);
      changed = true;
    }
  }

  if (!changed)
    return;

  // Present on urgent and visible as a general policy
  if (value && (quirk == Quirk::URGENT || quirk == Quirk::VISIBLE))
  {
    Present(0.5f, 1500, monitor);
  }

  if (quirk == Quirk::VISIBLE)
    visibility_changed.emit(monitor);
}

void LauncherIcon::FullyAnimateQuirkDelayed(guint ms, LauncherIcon::Quirk quirk, int monitor)
{
  _source_manager.AddTimeout(ms, [this, quirk, monitor] {
    FullyAnimateQuirk(quirk, monitor);
    return false;
  }, QUIRK_DELAY_TIMEOUT + std::to_string(unsigned(quirk)) + std::to_string(monitor));
}

void LauncherIcon::FullyAnimateQuirk(LauncherIcon::Quirk quirk, int monitor)
{
  if (monitor < 0)
  {
    for (unsigned i = 0; i < monitors::MAX; ++i)
      animation::Start(GetQuirkAnimation(quirk, i), animation::Direction::FORWARD);
  }
  else
  {
    animation::Start(GetQuirkAnimation(quirk, monitor), animation::Direction::FORWARD);
  }
}

void LauncherIcon::SkipQuirkAnimation(LauncherIcon::Quirk quirk, int monitor)
{
  if (monitor < 0)
  {
    for (unsigned i = 0; i < monitors::MAX; ++i)
    {
      animation::Skip(GetQuirkAnimation(quirk, i));
    }
  }
  else
  {
    animation::Skip(GetQuirkAnimation(quirk, monitor));
  }
}

float LauncherIcon::GetQuirkProgress(Quirk quirk, int monitor) const
{
  return GetQuirkAnimation(quirk, monitor).GetCurrentValue();
}

void LauncherIcon::SetQuirkDuration(Quirk quirk, unsigned duration, int monitor)
{
  if (monitor < 0)
  {
    for (unsigned i = 0; i < monitors::MAX; ++i)
      GetQuirkAnimation(quirk, i).SetDuration(duration);
  }
  else
  {
    GetQuirkAnimation(quirk, monitor).SetDuration(duration);
  }
}

void LauncherIcon::SetProgress(float progress)
{
  if (progress == _progress)
    return;

  _progress = progress;
  EmitNeedsRedraw();
}

float LauncherIcon::GetProgress()
{
  return _progress;
}

AbstractLauncherIcon::MenuItemsVector LauncherIcon::Menus()
{
  return GetMenus();
}

AbstractLauncherIcon::MenuItemsVector LauncherIcon::GetMenus()
{
  MenuItemsVector result;
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
  AddChild(remote.get());

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
  RemoveChild(remote.get());

  DeleteEmblem();
  SetQuirk(Quirk::PROGRESS, false);

  if (_remote_urgent)
    SetQuirk(Quirk::URGENT, false);

  _remote_menus = nullptr;
}

void
LauncherIcon::OnRemoteUrgentChanged(LauncherEntryRemote* remote)
{
  _remote_urgent = remote->Urgent();
  SetQuirk(Quirk::URGENT, remote->Urgent());
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

  if (remote->Count() / 10000 != 0)
  {
    SetEmblemText("****");
  }
  else
  {
    SetEmblemText(std::to_string(remote->Count()));
  }
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
  _remote_menus = remote->Quicklist();
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
  SetQuirk(Quirk::PROGRESS, remote->ProgressVisible());

  if (remote->ProgressVisible())
    SetProgress(remote->Progress());
}

glib::Object<DbusmenuMenuitem> LauncherIcon::GetRemoteMenus() const
{
  if (!_remote_menus.IsType(DBUSMENU_TYPE_CLIENT))
    return glib::Object<DbusmenuMenuitem>();

  glib::Object<DbusmenuMenuitem> root(dbusmenu_client_get_root(_remote_menus), glib::AddRef());

  if (!root.IsType(DBUSMENU_TYPE_MENUITEM) ||
      !dbusmenu_menuitem_property_get_bool(root, DBUSMENU_MENUITEM_PROP_VISIBLE))
  {
    return glib::Object<DbusmenuMenuitem>();
  }

  return root;
}

void LauncherIcon::EmitNeedsRedraw(int monitor)
{
  if (OwnsTheReference() && GetReferenceCount() > 0)
  {
    if (monitor < 0)
    {
      needs_redraw.emit(AbstractLauncherIcon::Ptr(this), monitor);
    }
    else
    {
      auto const& visibilty = GetQuirkAnimation(Quirk::VISIBLE, monitor);

      if (visibilty.GetCurrentValue() > 0.0f || visibilty.CurrentState() == na::Animation::State::Running)
        needs_redraw.emit(AbstractLauncherIcon::Ptr(this), monitor);
    }
  }
}

void LauncherIcon::EmitRemove()
{
  if (OwnsTheReference() && GetReferenceCount() > 0)
    remove.emit(AbstractLauncherIcon::Ptr(this));
}

void LauncherIcon::Stick(bool save)
{
  if (_sticky && !save)
    return;

  _sticky = true;

  if (save)
    position_saved.emit();

  SetQuirk(Quirk::VISIBLE, true);
}

void LauncherIcon::UnStick()
{
  if (!_sticky)
    return;

  _sticky = false;

  position_forgot.emit();

  SetQuirk(Quirk::VISIBLE, false);
}

void LauncherIcon::PerformScroll(ScrollDirection direction, Time timestamp)
{}


} // namespace launcher
} // namespace unity
