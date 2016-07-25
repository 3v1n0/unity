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
#include "unity-shared/ThemeSettings.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/UScreen.h"

#include "QuicklistManager.h"
#include "QuicklistMenuItem.h"
#include "QuicklistMenuItemLabel.h"
#include "QuicklistMenuItemSeparator.h"
#include "QuicklistMenuItemCheckmark.h"
#include "QuicklistMenuItemRadio.h"

#include "MultiMonitor.h"

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Variant.h>

#include <numeric>

namespace unity
{
namespace launcher
{
DECLARE_LOGGER(logger, "unity.launcher.icon");

namespace
{
const int IGNORE_REPEAT_SHORTCUT_DURATION = 250;

const std::string DEFAULT_ICON = "application-default-icon";

const std::string CENTER_STABILIZE_TIMEOUT = "center-stabilize-timeout";
const std::string PRESENT_TIMEOUT = "present-timeout";
const std::string QUIRK_DELAY_TIMEOUT = "quirk-delay-timeout";

const int COUNT_FONT_SIZE = 11;
const int COUNT_PADDING = 2;
}

NUX_IMPLEMENT_OBJECT_TYPE(LauncherIcon);

LauncherIcon::LauncherIcon(IconType type)
  : _icon_type(type)
  , _sticky(false)
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
  , _number_of_visible_windows(monitors::MAX)
  , _quirks(monitors::MAX)
  , _quirk_animations(monitors::MAX, decltype(_quirk_animations)::value_type(unsigned(Quirk::LAST)))
  , _last_stable(monitors::MAX)
  , _saved_center(monitors::MAX)
{
  tooltip_enabled = true;
  tooltip_enabled.changed.connect(sigc::mem_fun(this, &LauncherIcon::OnTooltipEnabledChanged));

  position = Position::FLOATING;
  removed = false;

  // FIXME: the abstraction is already broken, should be fixed for O
  // right now, hooking the dynamic quicklist the less ugly possible way
  mouse_enter.connect(sigc::mem_fun(this, &LauncherIcon::RecvMouseEnter));
  mouse_leave.connect(sigc::mem_fun(this, &LauncherIcon::RecvMouseLeave));
  mouse_down.connect(sigc::mem_fun(this, &LauncherIcon::RecvMouseDown));
  mouse_up.connect(sigc::mem_fun(this, &LauncherIcon::RecvMouseUp));
  mouse_click.connect(sigc::mem_fun(this, &LauncherIcon::RecvMouseClick));

  auto const& count_rebuild_cb = sigc::mem_fun(this, &LauncherIcon::CleanCountTextures);
  Settings::Instance().dpi_changed.connect(count_rebuild_cb);
  Settings::Instance().font_scaling.changed.connect(sigc::hide(count_rebuild_cb));
  icon_size.changed.connect(sigc::hide(count_rebuild_cb));

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
  int monitor = _last_monitor;
  if (monitor < 0)
    monitor = 0;

  _tooltip = new Tooltip(monitor);
  _tooltip->SetOpacity(0.0f);
  _tooltip->text = tooltip_text();
  _tooltip->hidden.connect([this] { _tooltip.Release(); });
  debug::Introspectable::AddChild(_tooltip.GetPointer());
}

void LauncherIcon::LoadQuicklist()
{
  int monitor = _last_monitor;
  if (monitor < 0)
    monitor = 0;

  _quicklist = new QuicklistView(monitor);
  _quicklist->hidden.connect([this] { _quicklist.Release(); });
  debug::Introspectable::AddChild(_quicklist.GetPointer());

  _quicklist->mouse_down_outside_pointer_grab_area.connect([this] (int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    _allow_quicklist_to_show = false;
  });

  QuicklistManager::Default()->RegisterQuicklist(_quicklist);
}

bool LauncherIcon::WindowVisibleOnMonitor(int monitor) const
{
  return _has_visible_window[monitor];
}

bool LauncherIcon::WindowVisibleOnViewport() const
{
  return _has_visible_window.any();
}

size_t LauncherIcon::WindowsVisibleOnMonitor(int monitor) const
{
  return _number_of_visible_windows[monitor];
}

size_t LauncherIcon::WindowsVisibleOnViewport() const
{
  return std::accumulate(begin(_number_of_visible_windows), end(_number_of_visible_windows), 0);
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

BaseTexturePtr LauncherIcon::TextureFromPixbuf(GdkPixbuf* pixbuf, int size, bool update_glow_colors)
{
  g_return_val_if_fail(GDK_IS_PIXBUF(pixbuf), BaseTexturePtr());

  glib::Object<GdkPixbuf> scaled_pixbuf(gdk_pixbuf_scale_simple(pixbuf, size, size,  GDK_INTERP_BILINEAR));

  if (update_glow_colors)
    ColorForIcon(scaled_pixbuf, _background_color, _glow_color);

  BaseTexturePtr result;
  result.Adopt(nux::CreateTexture2DFromPixbuf(scaled_pixbuf, true));
  return result;
}

BaseTexturePtr LauncherIcon::TextureFromGtkTheme(std::string icon_name, int size, bool update_glow_colors)
{
  GtkIconTheme* default_theme;
  BaseTexturePtr result;

  if (icon_name.empty())
    icon_name = DEFAULT_ICON;

  default_theme = gtk_icon_theme_get_default();
  result = TextureFromSpecificGtkTheme(default_theme, icon_name, size, update_glow_colors);

  if (!result)
    result = TextureFromSpecificGtkTheme(theme::Settings::Get()->UnityIconTheme(), icon_name, size, update_glow_colors);

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
  glib::Object<GtkIconInfo> info;
  auto flags = GTK_ICON_LOOKUP_FORCE_SIZE;

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
  auto const& converter = Settings::Instance().em(monitor);
  if (Settings::Instance().launcher_position() == LauncherPosition::LEFT)
  {
    return nux::Point(_center[monitor].x + converter->CP(icon_size()) / 2 + 1, _center[monitor].y);
  }
  else
  {
    return nux::Point(_center[monitor].x, _center[monitor].y - converter->CP(icon_size()) / 2 - 1);
  }
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

  // FIXME We need to look at why we need to set the last_monitor to -1 when it leaves.
  // As it would be nice to not have to re-create the tooltip everytime now :(
  LoadTooltip();
}

void LauncherIcon::RecvMouseLeave(int monitor)
{
  _last_monitor = -1;
  _allow_quicklist_to_show = true;
}

bool LauncherIcon::OpenQuicklist(bool select_first_item, int monitor, bool restore_input_focus)
{
  MenuItemsVector const& menus = Menus();

  if (menus.empty())
    return false;

  LoadQuicklist();

  if (_tooltip)
  {
    // Hide the tooltip without fade animation
    _tooltip->ShowWindow(false);
  }

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
  auto const& pos = GetTipPosition(monitor);

  /* If the expo plugin is active, we need to wait it to be terminated, before
   * showing the icon quicklist. */
  if (win_manager.IsExpoActive())
  {
    auto conn = std::make_shared<sigc::connection>();
    *conn = win_manager.terminate_expo.connect([this, conn, pos, restore_input_focus] {
      QuicklistManager::Default()->ShowQuicklist(_quicklist, pos.x, pos.y, restore_input_focus);
      conn->disconnect();
    });
  }
  else if (win_manager.IsScaleActive())
  {
    auto conn = std::make_shared<sigc::connection>();
    *conn = win_manager.terminate_spread.connect([this, conn, pos, restore_input_focus] {
      QuicklistManager::Default()->ShowQuicklist(_quicklist, pos.x, pos.y, restore_input_focus);
      conn->disconnect();
    });
    win_manager.TerminateScale();
  }
  else
  {
    QuicklistManager::Default()->ShowQuicklist(_quicklist, pos.x, pos.y, restore_input_focus);
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

void LauncherIcon::PromptHideTooltip()
{
  if (_tooltip)
    _tooltip->PromptHide();

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

void LauncherIcon::SetNumberOfWindowsVisibleOnMonitor(int number_of_windows, int monitor)
{
  if (_number_of_visible_windows[monitor] == number_of_windows)
    return;

  _has_visible_window[monitor] = (number_of_windows > 0);
  _number_of_visible_windows[monitor] = number_of_windows;

  windows_changed.emit(monitor);
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

  quirks_changed.emit(quirk, monitor);
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

nux::BaseTexture* LauncherIcon::Emblem() const
{
  return _emblem.GetPointer();
}

nux::BaseTexture* LauncherIcon::CountTexture(double scale)
{
  int count = Count();

  if (!count)
    return nullptr;

  auto it = _counts.find(scale);

  if (it != _counts.end())
    return it->second.GetPointer();

  auto const& texture = DrawCountTexture(count, scale);
  _counts[scale] = texture;
  return texture.GetPointer();
}

unsigned LauncherIcon::Count() const
{
  if (!_remote_entries.empty())
  {
    auto const& remote = _remote_entries.front();

    if (remote->CountVisible())
      return remote->Count();
  }

  return 0;
}

void LauncherIcon::SetEmblem(LauncherIcon::BaseTexturePtr const& emblem)
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

void LauncherIcon::CleanCountTextures()
{
  _counts.clear();
  EmitNeedsRedraw();
}

BaseTexturePtr LauncherIcon::DrawCountTexture(unsigned count, double scale)
{
  glib::Object<PangoContext> pango_ctx(gdk_pango_context_get());
  glib::Object<PangoLayout> layout(pango_layout_new(pango_ctx));

  auto const& font = theme::Settings::Get()->font();
  std::shared_ptr<PangoFontDescription> desc(pango_font_description_from_string(font.c_str()), pango_font_description_free);
  int font_size = pango_units_from_double(Settings::Instance().font_scaling() * COUNT_FONT_SIZE);
  pango_font_description_set_absolute_size(desc.get(), font_size);
  pango_layout_set_font_description(layout, desc.get());

  pango_layout_set_width(layout, pango_units_from_double(icon_size() * 0.75));
  pango_layout_set_height(layout, -1);
  pango_layout_set_wrap(layout, PANGO_WRAP_CHAR);
  pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_MIDDLE);
  pango_layout_set_text(layout, std::to_string(count).c_str(), -1);

  PangoRectangle ink_rect;
  pango_layout_get_pixel_extents(layout, &ink_rect, nullptr);

  /* DRAW OUTLINE */
  const float height = ink_rect.height + COUNT_PADDING * 4;
  const float inset = height / 2.0;
  const float radius = inset - 1.0f;
  const float width = ink_rect.width + inset + COUNT_PADDING * 2;

  nux::CairoGraphics cg(CAIRO_FORMAT_ARGB32, std::round(width * scale), std::round(height * scale));
  cairo_surface_set_device_scale(cg.GetSurface(), scale, scale);
  cairo_t* cr = cg.GetInternalContext();

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
  cairo_move_to(cr, (width - ink_rect.width) / 2.0 - ink_rect.x,
                    (height - ink_rect.height) / 2.0 - ink_rect.y);
  pango_cairo_show_layout(cr, layout);

  return texture_ptr_from_cairo_graphics(cg);
}

void
LauncherIcon::DeleteEmblem()
{
  SetEmblem(BaseTexturePtr());
}

void LauncherIcon::InsertEntryRemote(LauncherEntryRemote::Ptr const& remote)
{
  if (!remote || std::find(_remote_entries.begin(), _remote_entries.end(), remote) != _remote_entries.end())
    return;

  _remote_entries.push_back(remote);
  AddChild(remote.get());
  SelectEntryRemote(remote);
}

void LauncherIcon::SelectEntryRemote(LauncherEntryRemote::Ptr const& remote)
{
  if (!remote)
    return;

  auto& cm = _remote_connections;
  cm.Clear();

  cm.Add(remote->emblem_changed.connect(sigc::mem_fun(this, &LauncherIcon::OnRemoteEmblemChanged)));
  cm.Add(remote->count_changed.connect(sigc::mem_fun(this, &LauncherIcon::OnRemoteCountChanged)));
  cm.Add(remote->progress_changed.connect(sigc::mem_fun(this, &LauncherIcon::OnRemoteProgressChanged)));
  cm.Add(remote->quicklist_changed.connect(sigc::mem_fun(this, &LauncherIcon::OnRemoteQuicklistChanged)));

  cm.Add(remote->emblem_visible_changed.connect(sigc::mem_fun(this, &LauncherIcon::OnRemoteEmblemVisibleChanged)));
  cm.Add(remote->count_visible_changed.connect(sigc::mem_fun(this, &LauncherIcon::OnRemoteCountVisibleChanged)));
  cm.Add(remote->progress_visible_changed.connect(sigc::mem_fun(this, &LauncherIcon::OnRemoteProgressVisibleChanged)));

  cm.Add(remote->urgent_changed.connect(sigc::mem_fun(this, &LauncherIcon::OnRemoteUrgentChanged)));

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

void LauncherIcon::RemoveEntryRemote(LauncherEntryRemote::Ptr const& remote)
{
  auto remote_it = std::find(_remote_entries.begin(), _remote_entries.end(), remote);

  if (remote_it == _remote_entries.end())
    return;

  SetQuirk(Quirk::PROGRESS, false);

  if (remote->Urgent())
    SetQuirk(Quirk::URGENT, false);

  _remote_entries.erase(remote_it);
  RemoveChild(remote.get());
  DeleteEmblem();
  _remote_menus = nullptr;

  if (!_remote_entries.empty())
    SelectEntryRemote(_remote_entries.back());
}

void
LauncherIcon::OnRemoteUrgentChanged(LauncherEntryRemote* remote)
{
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

  CleanCountTextures();
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
  CleanCountTextures();
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
