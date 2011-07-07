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

#ifndef ABSTRACTLAUNCHERICON_H
#define ABSTRACTLAUNCHERICON_H

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>

#include <sigc++/sigc++.h>

#include <libdbusmenu-glib/menuitem.h>

#include "LauncherEntryRemote.h"

class AbstractLauncherIcon
{
public:
    typedef enum
    {
      TYPE_NONE,
      TYPE_BEGIN,
      TYPE_FAVORITE,
      TYPE_APPLICATION,
      TYPE_EXPO,
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
      
      QUIRK_LAST,
    } Quirk;

    virtual ~AbstractLauncherIcon();

    virtual  void HideTooltip () = 0;

    virtual void SetTooltipText (const TCHAR* text) = 0;
    
    virtual nux::NString GetTooltipText () = 0;
    
    virtual void    SetShortcut (guint64 shortcut) = 0;
    
    virtual guint64 GetShortcut () = 0;
    
    virtual void SetSortPriority (int priority) = 0;    

    virtual gboolean OpenQuicklist (bool default_to_first_item = false) = 0;

    virtual void        SetCenter (nux::Point3 center) = 0;

    virtual nux::Point3 GetCenter () = 0;

    virtual void Activate () = 0;
    
    virtual void OpenInstance () = 0;

    virtual void SaveCenter () = 0;
    
    virtual int SortPriority () = 0;
    
    virtual int RelatedWindows () = 0;
    
    virtual bool HasWindowOnViewport () = 0;
    
    virtual bool IsSpacer () = 0;
    
    virtual float PresentUrgency () = 0;

    virtual float GetProgress () = 0;
    
    virtual void SetEmblemIconName (const char *name) = 0;
    
    virtual void SetEmblemText (const char *text) = 0;
    
    virtual void DeleteEmblem () = 0;
    
    virtual bool ShowInSwitcher () = 0;

    virtual unsigned int SwitcherPriority () = 0;
    
    virtual bool GetQuirk (Quirk quirk) = 0;

    virtual void SetQuirk (Quirk quirk, bool value) = 0;

    virtual struct timespec GetQuirkTime (Quirk quirk) = 0;
    
    virtual IconType Type () = 0;
    
    virtual nux::Color BackgroundColor () = 0;

    virtual nux::Color GlowColor () = 0;
    
    virtual const gchar * RemoteUri () = 0;
    
    virtual nux::BaseTexture * TextureForSize (int size) = 0;

    virtual nux::BaseTexture * Emblem () = 0;

    virtual nux::BaseTexture* GetSuperkeyLabel () = 0;

    virtual std::list<DbusmenuMenuitem *> Menus () = 0;

    virtual void InsertEntryRemote (LauncherEntryRemote *remote) = 0;

    virtual void RemoveEntryRemote (LauncherEntryRemote *remote) = 0;

    virtual nux::DndAction QueryAcceptDrop (std::list<char *> paths) = 0;

    virtual void AcceptDrop (std::list<char *> paths) = 0;

    virtual void SendDndEnter () = 0;

    virtual void SendDndLeave () = 0;
    
    virtual void SetIconType (IconType type) = 0;
    
    sigc::signal<void, int> MouseDown;
    sigc::signal<void, int> MouseUp;
    sigc::signal<void, int> MouseClick;
    sigc::signal<void>      MouseEnter;
    sigc::signal<void>      MouseLeave;
    
    sigc::signal<void, AbstractLauncherIcon *> show;
    sigc::signal<void, AbstractLauncherIcon *> hide;
    sigc::signal<void, AbstractLauncherIcon *> needs_redraw;

protected:
    virtual const gchar * GetName () = 0;

    virtual void AddProperties (GVariantBuilder *builder) = 0;

    virtual void SetRelatedWindows (int windows) = 0;
    
    virtual void Remove () = 0;
    
    virtual void SetProgress (float progress) = 0;
    
    virtual void SetHasWindowOnViewport (bool val) = 0;
    
    virtual void Present (float urgency, int length) = 0;
    
    virtual void Unpresent () = 0;

};

#endif // LAUNCHERICON_H

