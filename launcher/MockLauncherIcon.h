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

#include <bitset>
#include <Nux/Nux.h>

#include <Nux/BaseWindow.h>
#include <Nux/View.h>

#include <gtk/gtk.h>

#include <sigc++/sigc++.h>

#include <libdbusmenu-glib/menuitem.h>
#include "unity-shared/ApplicationManager.h"
#include "unity-shared/TimeUtil.h"
#include <UnityCore/GTKWrapper.h>

#include "AbstractLauncherIcon.h"
#include "MultiMonitor.h"

namespace unity
{
namespace launcher
{
class MockApplicationWindow : public ApplicationWindow
{
public:
  MockApplicationWindow(Window xid) : xid_(xid)
  {
    title.SetGetterFunction([this] { return "MockApplicationWindow"; });
    icon.SetGetterFunction([this] { return ""; });
  }

  virtual std::string type() const { return "mock"; }

  virtual Window window_id() const { return xid_; }
  virtual int monitor() const { return -1; }
  virtual ApplicationPtr application() const { return ApplicationPtr(); }
  virtual bool Focus() const { return false; }
  virtual void Quit() const {}
private:
  Window xid_;
};


class MockLauncherIcon : public AbstractLauncherIcon
{
  NUX_DECLARE_OBJECT_TYPE(MockLauncherIcon, AbstractLauncherIcon);
public:
  MockLauncherIcon(IconType type = IconType::APPLICATION)
    : icon_(0)
    , type_(type)
    , sort_priority_(DefaultPriority(type))
    , quirks_(monitors::MAX)
    , quirk_progress_(monitors::MAX, decltype(quirk_progress_)::value_type(unsigned(Quirk::LAST), 0.0f))
    , remote_uri_("fake")
    , is_tooltip_visible_(false)
  {
    tooltip_text = "Mock Icon";
    position = Position::FLOATING;
  }

  std::string GetName() const { return "MockLauncherIcon"; }

  void AddProperties(GVariantBuilder* builder) {}

  void ShowTooltip() { is_tooltip_visible_ = true; }
  void HideTooltip() { is_tooltip_visible_ = false; }
  bool IsTooltipVisible() { return is_tooltip_visible_; }

  void    SetShortcut(guint64 shortcut) {}

  guint64 GetShortcut()
  {
    return 0;
  }

  WindowList Windows ()
  {
    WindowList result;

    result.push_back(std::make_shared<MockApplicationWindow>((100 << 16) + 200));
    result.push_back(std::make_shared<MockApplicationWindow>((500 << 16) + 200));
    result.push_back(std::make_shared<MockApplicationWindow>((300 << 16) + 200));
    result.push_back(std::make_shared<MockApplicationWindow>((200 << 16) + 200));
    result.push_back(std::make_shared<MockApplicationWindow>((300 << 16) + 200));
    result.push_back(std::make_shared<MockApplicationWindow>((100 << 16) + 200));
    result.push_back(std::make_shared<MockApplicationWindow>((300 << 16) + 200));
    result.push_back(std::make_shared<MockApplicationWindow>((600 << 16) + 200));

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

  void SetSortPriority(int priority) { sort_priority_ = priority; }

  bool OpenQuicklist(bool select_first_item = false, int monitor = -1)
  {
    return false;
  }

  void CloseQuicklist()
  {
  }

  void SetCenter(nux::Point3 const& center, int monitor)
  {
    center_[monitor] = center;
  }

  void ResetCenters(int monitor = -1)
  {
    for (auto& pair : center_)
      pair.second.Set(0, 0, 0);
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

  bool AllowDetailViewInSwitcher() const override
  {
    return true;
  }

  void InsertEntryRemote(LauncherEntryRemote::Ptr const& remote) {}

  void RemoveEntryRemote(LauncherEntryRemote::Ptr const& remote) {}

  uint64_t SwitcherPriority()
  {
    return 0;
  }

  bool GetQuirk(Quirk quirk, int monitor = -1) const
  {
    if (monitor < 0)
    {
      for (unsigned i = 0; i < monitors::MAX; ++i)
      {
        if (!quirks_[i][unsigned(quirk)])
          return false;
      }

      return true;
    }

    return quirks_[monitor][unsigned(quirk)];
  }

  void SetQuirk(Quirk quirk, bool value, int monitor = -1)
  {
    if (monitor < 0)
    {
      for (unsigned i = 0; i < monitors::MAX; ++i)
      {
        quirks_[i][unsigned(quirk)] = value;
        quirk_progress_[i][unsigned(quirk)] = value ? 1.0f : 0.0f;
      }
    }
    else
    {
      quirks_[monitor][unsigned(quirk)] = value;
      quirk_progress_[monitor][unsigned(quirk)] = value ? 1.0f : 0.0f;
    }
  }

  void SkipQuirkAnimation(Quirk quirk, int monitor)
  {
    float value = GetQuirk(quirk, monitor) ? 1.0f : 0.0f;

    if (monitor < 0)
    {
      for (unsigned i = 0; i < monitors::MAX; ++i)
        quirk_progress_[i][unsigned(quirk)] = value;
    }
    else
    {
      quirk_progress_[monitor][unsigned(quirk)] = value;
    }
  }

  float GetQuirkProgress(Quirk quirk, int monitor) const
  {
    return quirk_progress_[monitor][unsigned(quirk)];
  }

  void SetQuirkDuration(Quirk quirk, unsigned duration, int monitor = -1)
  {}

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

  std::string RemoteUri() const
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

  std::string DesktopFile() const { return std::string(""); }

  bool IsSticky() const { return false; }

  bool IsVisible() const { return false; }

  void AboutToRemove() {}
  
  void Stick(bool save = true) {}
  
  void UnStick() {}

  void PerformScroll(ScrollDirection /*direction*/, Time /*timestamp*/) override {}

private:
  nux::BaseTexture* TextureFromGtkTheme(const char* icon_name, int size)
  {
    GdkPixbuf* pbuf;
    gtk::IconInfo info;
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
  std::vector<std::bitset<std::size_t(Quirk::LAST)>> quirks_;
  std::vector<std::vector<float>> quirk_progress_;
  std::map<int, nux::Point3> center_;
  std::string remote_uri_;
  bool is_tooltip_visible_;
};

}
}

#endif // MOCKLAUNCHERICON_H

