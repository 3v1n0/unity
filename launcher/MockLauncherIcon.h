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
 *
 */

#ifndef MOCKLAUNCHERICON_H
#define MOCKLAUNCHERICON_H

#include <Nux/Nux.h>
#include <NuxCore/Math/MathInc.h>

#include <Nux/BaseWindow.h>
#include <Nux/View.h>

#include <gtk/gtk.h>

#include <sigc++/sigc++.h>

#include <libdbusmenu-glib/menuitem.h>

#include "AbstractLauncherIcon.h"

namespace unity
{
namespace launcher
{

class MockLauncherIcon : public AbstractLauncherIcon
{
  NUX_DECLARE_OBJECT_TYPE(MockLauncherIcon, AbstractLauncherIcon);
public:
  MockLauncherIcon(IconType type = IconType::APPLICATION)
    : icon_(0)
    , type_(type)
    , sort_priority_(DefaultPriority(type))
    , remote_uri_("fake")
  {
    tooltip_text = "Mock Icon";
    position = Position::FLOATING;

    for (unsigned i = 0; i < unsigned(Quirk::LAST); ++i)
      quirks_[i] = false;
  }

  std::string GetName() const { return "MockLauncherIcon"; }
  
  void AddProperties(GVariantBuilder* builder) {}

  void HideTooltip() {}

  void    SetShortcut(guint64 shortcut) {}

  guint64 GetShortcut()
  {
    return 0;
  }

  std::vector<Window> Windows ()
  {
    std::vector<Window> result;

    result.push_back ((100 << 16) + 200);
    result.push_back ((500 << 16) + 200);
    result.push_back ((300 << 16) + 200);
    result.push_back ((200 << 16) + 200);
    result.push_back ((300 << 16) + 200);
    result.push_back ((100 << 16) + 200);
    result.push_back ((300 << 16) + 200);
    result.push_back ((600 << 16) + 200);

    return result;
  }

  std::vector<Window> WindowsOnViewport ()
  {
    std::vector<Window> result;

    result.push_back ((100 << 16) + 200);
    result.push_back ((500 << 16) + 200);
    result.push_back ((300 << 16) + 200);
    result.push_back ((200 << 16) + 200);
    result.push_back ((300 << 16) + 200);
    result.push_back ((100 << 16) + 200);
    result.push_back ((300 << 16) + 200);
    result.push_back ((600 << 16) + 200);

    return result;
  }

  std::vector<Window> WindowsForMonitor (int monitor)
  {
    std::vector<Window> result;

    result.push_back ((100 << 16) + 200);
    result.push_back ((500 << 16) + 200);
    result.push_back ((300 << 16) + 200);
    result.push_back ((200 << 16) + 200);
    result.push_back ((300 << 16) + 200);
    result.push_back ((100 << 16) + 200);
    result.push_back ((300 << 16) + 200);
    result.push_back ((600 << 16) + 200);

    return result;
  }

  std::string NameForWindow (Window window)
  {
    return std::string();
  }

  void SetSortPriority(int priority) { sort_priority_ = priority; }

  bool OpenQuicklist(bool select_first_item = false, int monitor = -1)
  {
    return false;
  }

  void SetCenter(nux::Point3 const& center, int monitor, nux::Geometry const& geo)
  {
    center_[monitor] = center;
    center_[monitor].x += geo.x;
    center_[monitor].y += geo.y;
  }

  nux::Point3 GetCenter(int monitor)
  {
    return center_[monitor];
  }

  nux::Point3 GetSavedCenter(int monitor)
  {
    return nux::Point3();
  }

  void SaveCenter() {}

  void Activate(ActionArg arg) {}

  void OpenInstance(ActionArg arg) {}

  int SortPriority()
  {
    return sort_priority_;
  }

  int RelatedWindows()
  {
    return 7;
  }

  const bool WindowVisibleOnViewport()
  {
    return false;
  }

  const bool WindowVisibleOnMonitor(int monitor)
  {
    return false;
  }

  void SetVisibleOnMonitor(int monitor, bool visible) {}

  bool IsVisibleOnMonitor(int monitor) const
  {
    return true;
  }

  float PresentUrgency()
  {
    return 0.0f;
  }

  float GetProgress()
  {
    return 0.0f;
  }

  bool ShowInSwitcher(bool current)
  {
    return true;
  }

  void InsertEntryRemote(LauncherEntryRemote::Ptr const& remote) {}

  void RemoveEntryRemote(LauncherEntryRemote::Ptr const& remote) {}

  unsigned long long SwitcherPriority()
  {
    return 0;
  }

  bool GetQuirk(Quirk quirk) const
  {
    return quirks_[unsigned(quirk)];
  }

  void SetQuirk(Quirk quirk, bool value)
  {
    quirks_[unsigned(quirk)] = value;
    clock_gettime(CLOCK_MONOTONIC, &(quirk_times_[unsigned(quirk)]));
  }

  void ResetQuirkTime(Quirk quirk) {};

  struct timespec GetQuirkTime(Quirk quirk)
  {
    return quirk_times_[unsigned(quirk)];
  }

  IconType GetIconType() const
  {
    return type_;
  }

  nux::Color BackgroundColor() const
  {
    return nux::Color(0xFFAAAAAA);
  }

  nux::Color GlowColor()
  {
    return nux::Color(0xFFAAAAAA);
  }

  std::string RemoteUri()
  {
    return remote_uri_;
  }

  nux::BaseTexture* TextureForSize(int size)
  {
    if (icon_ && size == icon_->GetHeight())
      return icon_;

    if (icon_)
      icon_->UnReference();
    icon_ = 0;

    icon_ = TextureFromGtkTheme("firefox", size);
    return icon_;
  }

  nux::BaseTexture* Emblem()
  {
    return 0;
  }

  MenuItemsVector Menus()
  {
    return MenuItemsVector ();
  }

  nux::DndAction QueryAcceptDrop(DndData const& dnd_data)
  {
    return nux::DNDACTION_NONE;
  }

  bool ShouldHighlightOnDrag(DndData const& dnd_data)
  {
    return false;
  }

  void AcceptDrop(DndData const& dnd_data) {}

  void SendDndEnter() {}

  void SendDndLeave() {}

  std::string DesktopFile() { return std::string(""); }

  bool IsSticky() const { return false; }

  bool IsVisible() const { return false; }

  void AboutToRemove() {}
  
  void Stick(bool save = true) {}
  
  void UnStick() {}

private:
  nux::BaseTexture* TextureFromGtkTheme(const char* icon_name, int size)
  {
    GdkPixbuf* pbuf;
    GtkIconInfo* info;
    nux::BaseTexture* result = NULL;
    GError* error = NULL;
    GIcon* icon;

    GtkIconTheme* theme = gtk_icon_theme_get_default();

    icon = g_icon_new_for_string(icon_name, NULL);

    if (G_IS_ICON(icon))
    {
      info = gtk_icon_theme_lookup_by_gicon(theme, icon, size, (GtkIconLookupFlags)0);
      g_object_unref(icon);
    }
    else
    {
      info = gtk_icon_theme_lookup_icon(theme,
                                        icon_name,
                                        size,
                                        (GtkIconLookupFlags) 0);
    }

    if (!info)
      return NULL;

    pbuf = gtk_icon_info_load_icon(info, &error);
    gtk_icon_info_free(info);

    if (GDK_IS_PIXBUF(pbuf))
    {
      result = nux::CreateTexture2DFromPixbuf(pbuf, true);
      g_object_unref(pbuf);
    }
    else
    {
      g_warning("Unable to load '%s' from icon theme: %s",
                icon_name,
                error ? error->message : "unknown");
      g_error_free(error);
    }

    return result;
  }

  nux::BaseTexture* icon_;
  IconType type_;
  int sort_priority_;
  bool quirks_[unsigned(Quirk::LAST)];
  timespec quirk_times_[unsigned(Quirk::LAST)];
  std::map<int, nux::Point3> center_;
  std::string remote_uri_;
};

}
}

#endif // MOCKLAUNCHERICON_H

