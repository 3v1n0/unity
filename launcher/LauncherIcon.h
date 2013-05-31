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

#ifndef LAUNCHERICON_H
#define LAUNCHERICON_H

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <NuxCore/Animation.h>

#include <gtk/gtk.h>
#include <libdbusmenu-glib/client.h>
#include <libdbusmenu-glib/menuitem.h>

#include <UnityCore/GLibSource.h>
#include "AbstractLauncherIcon.h"
#include "Tooltip.h"
#include "QuicklistView.h"
#include "unity-shared/Introspectable.h"
#include "LauncherEntryRemote.h"


namespace unity
{
namespace launcher
{

class Launcher;

class LauncherIcon : public AbstractLauncherIcon
{
  NUX_DECLARE_OBJECT_TYPE(LauncherIcon, AbstractLauncherIcon);

public:
  typedef nux::ObjectPtr<nux::BaseTexture> BaseTexturePtr;

  LauncherIcon(IconType type);

  virtual ~LauncherIcon();

  bool SetTooltipText(std::string& target, std::string const& value);

  void    SetShortcut(guint64 shortcut);

  guint64 GetShortcut();

  void SetSortPriority(int priority);

  void RecvMouseEnter(int monitor);

  void RecvMouseLeave(int monitor);

  void RecvMouseDown(int button, int monitor, unsigned long key_flags = 0);

  void RecvMouseUp(int button, int monitor, unsigned long key_flags = 0);

  void RecvMouseClick(int button, int monitor, unsigned long key_flags = 0);

  void HideTooltip();

  void ShowTooltip();

  bool OpenQuicklist(bool select_first_item = false, int monitor = -1);
  void CloseQuicklist();

  void SetCenter(nux::Point3 const& center, int parent_monitor, nux::Geometry const& parent_geo);

  nux::Point3 GetCenter(int monitor);

  virtual void Activate(ActionArg arg);

  void OpenInstance(ActionArg arg);

  void SaveCenter();

  nux::Point3 GetSavedCenter(int monitor);

  int SortPriority();

  virtual WindowList Windows() { return WindowList(); }

  virtual std::vector<Window> WindowsOnViewport() { return std::vector<Window> (); }

  virtual std::vector<Window> WindowsForMonitor(int monitor) { return std::vector<Window> (); }

  const bool WindowVisibleOnMonitor(int monitor);

  const bool WindowVisibleOnViewport();

  float PresentUrgency();

  float GetProgress();

  void SetEmblemIconName(std::string const& name);

  void SetEmblemText(std::string const& text);

  void DeleteEmblem();

  virtual bool ShowInSwitcher(bool current)
  {
    return false;
  };

  virtual bool AllowDetailViewInSwitcher() const override
  {
    return false;
  }

  virtual uint64_t SwitcherPriority()
  {
    return 0;
  }

  bool GetQuirk(Quirk quirk) const;

  void SetQuirk(Quirk quirk, bool value);

  struct timespec GetQuirkTime(Quirk quirk);

  IconType GetIconType() const;

  virtual nux::Color BackgroundColor() const;

  virtual nux::Color GlowColor();

  std::string RemoteUri()
  {
    return GetRemoteUri();
  }

  nux::BaseTexture* TextureForSize(int size);

  nux::BaseTexture* Emblem();

  MenuItemsVector Menus();

  void InsertEntryRemote(LauncherEntryRemote::Ptr const& remote);

  void RemoveEntryRemote(LauncherEntryRemote::Ptr const& remote);

  nux::DndAction QueryAcceptDrop(DndData const& dnd_data)
  {
    return OnQueryAcceptDrop(dnd_data);
  }


  bool ShouldHighlightOnDrag(DndData const& dnd_data)
  {
    return OnShouldHighlightOnDrag(dnd_data);
  }

  void AcceptDrop(DndData const& dnd_data)
  {
    return OnAcceptDrop(dnd_data);
  }

  void SendDndEnter()
  {
    OnDndEnter();
  }

  void SendDndLeave()
  {
    OnDndLeave();
  }

  virtual std::string DesktopFile() { return std::string(""); }

  virtual bool IsSticky() const { return _sticky; }

  virtual bool IsVisible() const { return GetQuirk(Quirk::VISIBLE); }

  virtual bool IsVisibleOnMonitor(int monitor) const;

  virtual void SetVisibleOnMonitor(int monitor, bool visible);

  virtual void AboutToRemove() {}

  virtual void Stick(bool save = true);

  virtual void UnStick();

  void PerformScroll(ScrollDirection direction, Time timestamp) override;

protected:
  std::vector<nux::Point3> GetCenters();

  std::string GetName() const;

  void AddProperties(GVariantBuilder* builder);

  void UpdateQuirkTimeDelayed(guint ms, Quirk quirk);

  void UpdateQuirkTime(Quirk quirk);

  void ResetQuirkTime(Quirk quirk);

  void Remove();

  void SetProgress(float progress);

  void SetWindowVisibleOnMonitor(bool val, int monitor);

  void Present(float urgency, int length);

  void Unpresent();

  void SetEmblem(BaseTexturePtr const& emblem);

  virtual MenuItemsVector GetMenus();

  virtual nux::BaseTexture* GetTextureForSize(int size) = 0;

  virtual void OnCenterStabilized(std::vector<nux::Point3> center) {}

  virtual std::string GetRemoteUri()
  {
    return "";
  }

  virtual nux::DndAction OnQueryAcceptDrop(DndData const& dnd_data)
  {
    return nux::DNDACTION_NONE;
  }

  virtual bool OnShouldHighlightOnDrag(DndData const& dnd_data)
  {
    return false;
  }

  virtual void OnAcceptDrop(DndData const& dnd_data) {}

  virtual void OnDndEnter() {}

  virtual void OnDndLeave() {}

  virtual void ActivateLauncherIcon(ActionArg arg) {}

  virtual void OpenInstanceLauncherIcon(Time timestamp) {}

  virtual bool HandlesSpread () { return false; }

  nux::BaseTexture* TextureFromGtkTheme(std::string name, int size, bool update_glow_colors = true);

  nux::BaseTexture* TextureFromSpecificGtkTheme(GtkIconTheme* theme, std::string const& name, int size, bool update_glow_colors = true, bool is_default_theme = false);

  nux::BaseTexture* TextureFromPath(std::string const& name, int size, bool update_glow_colors = true);

  static bool        IsMonoDefaultTheme();

  GtkIconTheme*      GetUnityTheme();

  void OnRemoteEmblemChanged(LauncherEntryRemote* remote);

  void OnRemoteCountChanged(LauncherEntryRemote* remote);

  void OnRemoteProgressChanged(LauncherEntryRemote* remote);

  void OnRemoteQuicklistChanged(LauncherEntryRemote* remote);

  void OnRemoteUrgentChanged(LauncherEntryRemote* remote);

  void OnRemoteEmblemVisibleChanged(LauncherEntryRemote* remote);

  void OnRemoteCountVisibleChanged(LauncherEntryRemote* remote);

  void OnRemoteProgressVisibleChanged(LauncherEntryRemote* remote);

  void EmitNeedsRedraw();

  void EmitRemove();

  // This looks like a case for boost::logical::tribool
  static int _current_theme_is_mono;

  glib::Object<DbusmenuClient> _menuclient_dynamic_quicklist;

private:
  IconType _icon_type;

  nux::ObjectPtr<Tooltip> _tooltip;
  nux::ObjectPtr<QuicklistView> _quicklist;

  static void ChildRealized(DbusmenuMenuitem* newitem, QuicklistView* quicklist);
  static void RootChanged(DbusmenuClient* client, DbusmenuMenuitem* newroot, QuicklistView* quicklist);
  bool OnPresentTimeout();
  bool OnCenterStabilizeTimeout();

  void ColorForIcon(GdkPixbuf* pixbuf, nux::Color& background, nux::Color& glow);

  void LoadTooltip();
  void LoadQuicklist();

  void OnTooltipEnabledChanged(bool value);

  bool              _sticky;
  bool              _remote_urgent;
  float             _present_urgency;
  float             _progress;
  int               _sort_priority;
  int               _last_monitor;
  nux::Color        _background_color;
  nux::Color        _glow_color;

  gint64            _shortcut;

  std::vector<nux::Point3> _center;
  std::vector<bool> _has_visible_window;
  std::vector<bool> _is_visible_on_monitor;
  std::vector<nux::Point3> _last_stable;
  std::vector<nux::Geometry> _parent_geo;
  std::vector<nux::Point3> _saved_center;

  static glib::Object<GtkIconTheme> _unity_theme;

  BaseTexturePtr _emblem;

  bool             _quirks[unsigned(Quirk::LAST)];
  struct timespec  _quirk_times[unsigned(Quirk::LAST)];

  bool             _allow_quicklist_to_show;

  std::list<LauncherEntryRemote::Ptr> _entry_list;

  nux::animation::AnimateValue<double> _tooltip_fade_animator;

protected:
  glib::SourceManager _source_manager;
};

}
}

#endif // LAUNCHERICON_H

