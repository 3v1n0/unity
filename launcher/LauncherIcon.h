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

#include <bitset>
#include <Nux/Nux.h>
#include <NuxCore/Animation.h>

#include <gtk/gtk.h>
#include <libdbusmenu-glib/client.h>
#include <libdbusmenu-glib/menuitem.h>

#include <UnityCore/GLibSource.h>
#include "AbstractLauncherIcon.h"
#include "MultiMonitor.h"
#include "Tooltip.h"
#include "QuicklistView.h"
#include "LauncherEntryRemote.h"
#include "unity-shared/TimeUtil.h"


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

  void SetCenter(nux::Point3 const& center, int parent_monitor);

  void ResetCenters(int monitor = -1);

  nux::Point3 GetCenter(int monitor);

  virtual void Activate(ActionArg arg);

  void OpenInstance(ActionArg arg);

  void SaveCenter();

  nux::Point3 GetSavedCenter(int monitor);

  int SortPriority();

  void SetOrder(int order);

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

  bool GetQuirk(Quirk quirk, int monitor = -1) const;

  void SetQuirk(Quirk quirk, bool value, int monitor = -1);

  float GetQuirkProgress(Quirk quirk, int monitor) const;

  void SetQuirkDuration(Quirk quirk, unsigned duration, int monitor = -1);

  void SkipQuirkAnimation(Quirk quirk, int monitor = -1);

  IconType GetIconType() const;

  virtual nux::Color BackgroundColor() const;

  virtual nux::Color GlowColor();

  std::string RemoteUri() const
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

  virtual std::string DesktopFile() const { return std::string(); }

  virtual bool IsSticky() const { return _sticky; }

  virtual bool IsVisible() const { return GetQuirk(Quirk::VISIBLE); }

  virtual bool IsVisibleOnMonitor(int monitor) const;

  virtual void SetVisibleOnMonitor(int monitor, bool visible);

  virtual void AboutToRemove() {}

  virtual void Stick(bool save = true);

  virtual void UnStick();

  virtual glib::Object<DbusmenuMenuitem> GetRemoteMenus() const;

  void PerformScroll(ScrollDirection direction, Time timestamp) override;

protected:
  std::vector<nux::Point3> GetCenters();

  std::pair<int, nux::Point3> GetCenterForMonitor(int monitor) const;

  std::string GetName() const;

  void AddProperties(debug::IntrospectionData&);

  void FullyAnimateQuirkDelayed(guint ms, Quirk quirk, int monitor = -1);

  void FullyAnimateQuirk(Quirk quirk, int monitor = -1);

  void Remove();

  void SetProgress(float progress);

  void SetWindowVisibleOnMonitor(bool val, int monitor);

  void Present(float urgency, int length, int monitor = -1);

  void Unpresent(int monitor = -1);

  void SetEmblem(BaseTexturePtr const& emblem);

  virtual MenuItemsVector GetMenus();

  virtual nux::BaseTexture* GetTextureForSize(int size) = 0;

  virtual void OnCenterStabilized(std::vector<nux::Point3> const& centers) {}

  virtual std::string GetRemoteUri() const
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

  BaseTexturePtr TextureFromGtkTheme(std::string name, int size, bool update_glow_colors = true);

  BaseTexturePtr TextureFromSpecificGtkTheme(GtkIconTheme* theme, std::string const& name, int size, bool update_glow_colors = true, bool is_default_theme = false);

  BaseTexturePtr TextureFromPath(std::string const& name, int size, bool update_glow_colors = true);

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

  void EmitNeedsRedraw(int monitor = -1);

  void EmitRemove();

  bool IsActionArgValid(ActionArg const&);

  typedef nux::animation::AnimateValue<float> Animation;
  inline Animation& GetQuirkAnimation(Quirk quirk, int monitor) const
  {
    return *_quirk_animations[monitor][unsigned(quirk)];
  }

  // This looks like a case for boost::logical::tribool
  static int _current_theme_is_mono;

private:
  IconType _icon_type;

  nux::ObjectPtr<Tooltip> _tooltip;
  nux::ObjectPtr<QuicklistView> _quicklist;

  static void ChildRealized(DbusmenuMenuitem* newitem, QuicklistView* quicklist);
  static void RootChanged(DbusmenuClient* client, DbusmenuMenuitem* newroot, QuicklistView* quicklist);
  void ColorForIcon(GdkPixbuf* pixbuf, nux::Color& background, nux::Color& glow);
  nux::Point GetTipPosition(int monitor) const;

  void LoadTooltip();
  void LoadQuicklist();

  void OnTooltipEnabledChanged(bool value);

  bool _sticky;
  bool _remote_urgent;
  float _present_urgency;
  float _progress;
  int _sort_priority;
  int _order;
  int _last_monitor;
  nux::Color _background_color;
  nux::Color _glow_color;
  gint64 _shortcut;
  bool _allow_quicklist_to_show;

  std::vector<nux::Point3> _center;
  std::bitset<monitors::MAX> _has_visible_window;
  std::vector<std::bitset<std::size_t(Quirk::LAST)>> _quirks;
  std::vector<std::vector<std::shared_ptr<Animation>>> _quirk_animations;
  std::vector<nux::Point3> _last_stable;
  std::vector<nux::Point3> _saved_center;
  time::Spec _last_action;

  BaseTexturePtr _emblem;

  std::list<LauncherEntryRemote::Ptr> _entry_list;
  glib::Object<DbusmenuClient> _remote_menus;

  static glib::Object<GtkIconTheme> _unity_theme;

protected:
  glib::SourceManager _source_manager;
};

}
}

#endif // LAUNCHERICON_H

