// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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

#ifndef ABSTRACTLAUNCHERICON_H
#define ABSTRACTLAUNCHERICON_H

#include <Nux/Nux.h>
#include <NuxCore/Property.h>
#include <NuxCore/Math/MathInc.h>
#include <UnityCore/ConnectionManager.h>

#include <sigc++/sigc++.h>
#include <libdbusmenu-glib/menuitem.h>

#include "DndData.h"
#include <unity-shared/ApplicationManager.h>
#include "unity-shared/Introspectable.h"
#include "unity-shared/IconTextureSource.h"
#include "unity-shared/WindowManager.h"

#include "LauncherEntryRemote.h"

namespace unity
{
namespace launcher
{

struct ActionArg
{
  enum class Source
  {
    LAUNCHER,
    SWITCHER,
    OTHER,
  };

  ActionArg()
    : source(Source::OTHER)
    , button(0)
    , timestamp(0)
    , target(0)
    , monitor(-1)
  {}

  ActionArg(Source source, int button, unsigned long timestamp = 0,  Window target = 0, int monitor = -1)
    : source(source)
    , button(button)
    , timestamp(timestamp)
    , target(target)
    , monitor(monitor)
  {}

  Source source;
  int button;
  unsigned long timestamp;
  Window target;
  int monitor;
};

class AbstractLauncherIcon : public ui::IconTextureSource, public debug::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE(AbstractLauncherIcon, ui::IconTextureSource);
public:
  typedef nux::ObjectPtr<AbstractLauncherIcon> Ptr;
  typedef std::vector<nux::Vector4> TransformVector;
  typedef std::vector<glib::Object<DbusmenuMenuitem>> MenuItemsVector;

  enum class IconType
  {
    NONE,
    BEGIN,
    HOME,
    HUD,
    FAVORITE,
    APPLICATION,
    EXPO,
    DESKTOP,
    PLACE,
    DEVICE,
    SPACER,
    TRASH,
    END
  };

  enum class Quirk
  {
    VISIBLE = 0,
    ACTIVE,
    RUNNING,
    URGENT,
    PRESENTED,
    UNFOLDED,
    STARTING,
    SHIMMER,
    CENTER_SAVED,
    PROGRESS,
    DROP_PRELIGHT,
    DROP_DIM,
    DESAT,
    PULSE_ONCE,
    LAST_ACTION,

    LAST
  };

  enum class Position
  {
    BEGIN,
    FLOATING,
    END
  };

  enum class ScrollDirection
  {
    UP,
    DOWN
  };

  virtual ~AbstractLauncherIcon() = default;

  static nux::Property<unsigned> icon_size;
  nux::Property<std::string> tooltip_text;
  nux::Property<bool> tooltip_enabled;
  nux::Property<Position> position;
  nux::Property<bool> removed;

  virtual void ShowTooltip() = 0;
  virtual void HideTooltip() = 0;

  virtual void    SetShortcut(guint64 shortcut) = 0;

  virtual guint64 GetShortcut() = 0;

  virtual void SetSortPriority(int priority) = 0;

  virtual bool OpenQuicklist(bool select_first_item = false, int monitor = -1) = 0;
  virtual void CloseQuicklist() = 0;

  virtual void SetCenter(nux::Point3 const& center, int monitor) = 0;

  virtual nux::Point3 GetCenter(int monitor) = 0;

  virtual nux::Point3 GetSavedCenter(int monitor) = 0;

  virtual void SaveCenter() = 0;

  virtual void Activate(ActionArg arg) = 0;

  virtual void OpenInstance(ActionArg arg) = 0;

  virtual int SortPriority() = 0;

  virtual WindowList Windows() = 0;

  virtual std::vector<Window> WindowsForMonitor(int monitor) = 0;

  virtual std::vector<Window> WindowsOnViewport() = 0;

  virtual const bool WindowVisibleOnMonitor(int monitor) = 0;

  virtual const bool WindowVisibleOnViewport() = 0;

  virtual float PresentUrgency() = 0;

  virtual float GetProgress() = 0;

  virtual bool ShowInSwitcher(bool current) = 0;
  virtual bool AllowDetailViewInSwitcher() const = 0;

  virtual uint64_t SwitcherPriority() = 0;

  virtual bool GetQuirk(Quirk quirk, int monitor = -1) const = 0;

  virtual void SetQuirk(Quirk quirk, bool value, int monitor = -1) = 0;

  virtual struct timespec GetQuirkTime(Quirk quirk, int monitor) = 0;

  virtual void ResetQuirkTime(Quirk quirk, int monitor = -1) = 0;

  virtual IconType GetIconType() const = 0;

  virtual std::string RemoteUri() const = 0;

  virtual MenuItemsVector Menus() = 0;

  virtual nux::DndAction QueryAcceptDrop(DndData const& dnd_data) = 0;

  virtual bool ShouldHighlightOnDrag(DndData const& dnd_data) = 0;

  virtual void AcceptDrop(DndData const& dnd_data) = 0;

  virtual void SendDndEnter() = 0;

  virtual void SendDndLeave() = 0;

  virtual void InsertEntryRemote(LauncherEntryRemote::Ptr const& remote) = 0;
  virtual void RemoveEntryRemote(LauncherEntryRemote::Ptr const& remote) = 0;

  virtual std::string DesktopFile() const = 0;

  virtual bool IsSticky() const = 0;

  virtual bool IsVisible() const = 0;

  virtual bool IsVisibleOnMonitor(int monitor) const = 0;

  virtual void SetVisibleOnMonitor(int monitor, bool visible) = 0;

  virtual void AboutToRemove() = 0;

  virtual void Stick(bool save = true) = 0;

  virtual void UnStick() = 0;

  static int DefaultPriority(IconType type)
  {
    return static_cast<int>(type) * 1000;
  }

  virtual void PerformScroll(ScrollDirection direction, Time timestamp) = 0;

  sigc::signal<void, int, int, unsigned long> mouse_down;
  sigc::signal<void, int, int, unsigned long> mouse_up;
  sigc::signal<void, int, int, unsigned long> mouse_click;
  sigc::signal<void, int>      mouse_enter;
  sigc::signal<void, int>      mouse_leave;

  sigc::signal<void, AbstractLauncherIcon::Ptr const&, int> needs_redraw;
  sigc::signal<void, AbstractLauncherIcon::Ptr const&> remove;
  sigc::signal<void, nux::ObjectPtr<nux::View>> tooltip_visible;
  sigc::signal<void, int> visibility_changed;
  sigc::signal<void> position_saved;
  sigc::signal<void> position_forgot;
  sigc::signal<void, std::string const&> uri_changed;

  connection::Wrapper on_icon_removed_connection;
};

}
}

#endif // LAUNCHERICON_H

