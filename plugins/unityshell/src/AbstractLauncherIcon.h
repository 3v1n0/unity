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

#include <sigc++/sigc++.h>

#include <X11/Xlib.h>

#include <libdbusmenu-glib/menuitem.h>

#include "DndData.h"
#include "Introspectable.h"
#include "LauncherEntryRemote.h"

namespace unity
{
namespace launcher
{

class ActionArg
{
public:
  enum Source
  {
    LAUNCHER,
    SWITCHER,
    OTHER,
  };

  ActionArg()
    : source(OTHER)
    , button(0)
    , target(0)
    , monitor(-1)
  {
  }

  ActionArg(Source source, int button, Window target = 0, int monitor = -1)
    : source(source)
    , button(button)
    , target(target)
    , monitor(monitor)
  {
  }

  Source source;
  int button;
  Window target;
  int monitor;
};

class AbstractLauncherIcon : public nux::InitiallyUnownedObject, public debug::Introspectable
{
public:

  typedef std::vector<nux::Vector4> TransformVector;

  typedef enum
  {
    TYPE_NONE,
    TYPE_BEGIN,
    TYPE_HOME,
    TYPE_FAVORITE,
    TYPE_APPLICATION,
    TYPE_EXPO,
    TYPE_DESKTOP,
    TYPE_PLACE,
    TYPE_DEVICE,
    TYPE_TRASH,
    TYPE_END,
  } IconType;

  typedef enum
  {
    QUIRK_VISIBLE,
    QUIRK_ACTIVE,
    QUIRK_RUNNING,
    QUIRK_URGENT,
    QUIRK_PRESENTED,
    QUIRK_STARTING,
    QUIRK_SHIMMER,
    QUIRK_CENTER_SAVED,
    QUIRK_PROGRESS,
    QUIRK_DROP_PRELIGHT,
    QUIRK_DROP_DIM,
    QUIRK_DESAT,
    QUIRK_PULSE_ONCE,
    QUIRK_LAST_ACTION,

    QUIRK_LAST,
  } Quirk;

  enum TransformIndex
  {
    TRANSFORM_TILE,
    TRANSFORM_IMAGE,
    TRANSFORM_HIT_AREA,
    TRANSFORM_GLOW,
    TRANSFORM_EMBLEM,
  };

  virtual ~AbstractLauncherIcon() {}

  nux::Property<std::string> tooltip_text;

  virtual void HideTooltip() = 0;

  virtual void    SetShortcut(guint64 shortcut) = 0;

  virtual guint64 GetShortcut() = 0;

  virtual void SetSortPriority(int priority) = 0;

  virtual bool OpenQuicklist(bool default_to_first_item = false, int monitor = -1) = 0;

  virtual void        SetCenter(nux::Point3 center, int monitor, nux::Geometry parent_geo) = 0;

  virtual nux::Point3 GetCenter(int monitor) = 0;

  virtual nux::Point3 GetSavedCenter(int monitor) = 0;

  virtual void SaveCenter() = 0;

  virtual std::vector<nux::Vector4> & GetTransform(TransformIndex index, int monitor) = 0;

  virtual void Activate(ActionArg arg) = 0;

  virtual void OpenInstance(ActionArg arg) = 0;

  virtual int SortPriority() = 0;

  virtual std::vector<Window> Windows() = 0;

  virtual std::vector<Window> WindowsForMonitor(int monitor) = 0;

  virtual std::string NameForWindow (Window window) = 0;

  virtual const bool WindowVisibleOnMonitor(int monitor) = 0;

  virtual bool IsSpacer() = 0;

  virtual float PresentUrgency() = 0;

  virtual float GetProgress() = 0;

  virtual bool ShowInSwitcher(bool current) = 0;

  virtual unsigned long long SwitcherPriority() = 0;

  virtual bool GetQuirk(Quirk quirk) const = 0;

  virtual void SetQuirk(Quirk quirk, bool value) = 0;

  virtual struct timespec GetQuirkTime(Quirk quirk) = 0;

  virtual void ResetQuirkTime(Quirk quirk) = 0;

  virtual IconType Type() = 0;

  virtual nux::Color BackgroundColor() = 0;

  virtual nux::Color GlowColor() = 0;

  virtual const gchar* RemoteUri() = 0;

  virtual nux::BaseTexture* TextureForSize(int size) = 0;

  virtual nux::BaseTexture* Emblem() = 0;

  virtual std::list<DbusmenuMenuitem*> Menus() = 0;

  virtual nux::DndAction QueryAcceptDrop(unity::DndData& dnd_data) = 0;

  virtual void AcceptDrop(unity::DndData& dnd_data) = 0;

  virtual void SendDndEnter() = 0;

  virtual void SendDndLeave() = 0;

  virtual void InsertEntryRemote(LauncherEntryRemote* remote) = 0;

  virtual void RemoveEntryRemote(LauncherEntryRemote* remote) = 0;

  sigc::signal<void, int, int> mouse_down;
  sigc::signal<void, int, int> mouse_up;
  sigc::signal<void, int, int> mouse_click;
  sigc::signal<void, int>      mouse_enter;
  sigc::signal<void, int>      mouse_leave;

  sigc::signal<void, AbstractLauncherIcon*> show;
  sigc::signal<void, AbstractLauncherIcon*> hide;
  sigc::signal<void, AbstractLauncherIcon*> needs_redraw;
  sigc::signal<void, AbstractLauncherIcon*> remove;

  sigc::connection needs_redraw_connection;
  sigc::connection on_icon_added_connection;
  sigc::connection on_icon_removed_connection;
  sigc::connection on_order_changed_connection;
  sigc::connection on_expo_terminated_connection;
};

}
}

#endif // LAUNCHERICON_H

