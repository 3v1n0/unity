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
#include <NuxCore/Math/MathInc.h>

#include <sigc++/trackable.h>
#include <sigc++/signal.h>
#include <sigc++/functors/ptr_fun.h>
#include <sigc++/functors/mem_fun.h>

#include <gtk/gtk.h>
#include <libdbusmenu-glib/client.h>

#include "Tooltip.h"

class Launcher;

typedef enum
{
  LAUNCHER_ICON_TYPE_NONE,
  LAUNCHER_ICON_TYPE_BEGIN,
  LAUNCHER_ICON_TYPE_FAVORITE,
  LAUNCHER_ICON_TYPE_APPLICATION,
  LAUNCHER_ICON_TYPE_PLACE,
  LAUNCHER_ICON_TYPE_DEVICE,
  LAUNCHER_ICON_TYPE_TRASH,
  LAUNCHER_ICON_TYPE_END,
} LauncherIconType;

class LauncherIcon: public nux::InitiallyUnownedObject, public sigc::trackable
{
public:
    LauncherIcon(Launcher* IconManager);
    ~LauncherIcon();

    void         SetTooltipText (const TCHAR* text);
    
    nux::NString GetTooltipText ();
    
    bool Visible ();
    bool Active  ();
    bool Running ();
    bool Urgent  ();
    bool Presented ();

    void RecvMouseEnter ();
    void RecvMouseLeave ();
    
    void HideTooltip ();
    
    int SortPriority ();
    
    int RelatedWindows ();
    
    struct timespec ShowTime ();
    struct timespec HideTime ();
    struct timespec RunningTime ();
    struct timespec UrgentTime ();
    struct timespec PresentTime ();
    struct timespec UnpresentTime ();
    
    LauncherIconType Type ();
    
    nux::Color BackgroundColor ();
    
    nux::BaseTexture * TextureForSize (int size);
    
    std::list<DbusmenuClient *> Menus ();
    
    sigc::signal<void, int> MouseDown;
    sigc::signal<void, int> MouseUp;
    sigc::signal<void> MouseEnter;
    sigc::signal<void> MouseLeave;
    sigc::signal<void, int> MouseClick;
    
    sigc::signal<void, void *> show;
    sigc::signal<void, void *> hide;
    sigc::signal<void, void *> remove;
    sigc::signal<void, void *> needs_redraw;
protected:

    void SetVisible (bool visible);
    void SetActive  (bool active);
    void SetRunning (bool running);
    void SetUrgent  (bool urgent);
    void SetRelatedWindows (int windows);
    void Remove ();
    
    void Present (int length);
    void Unpresent ();
    
    void SetIconType (LauncherIconType type);
    void SetSortPriority (int priority);

    virtual std::list<DbusmenuClient *> GetMenus ();
    virtual nux::BaseTexture * GetTextureForSize (int size) = 0;

    nux::BaseTexture * TextureFromGtkTheme (const char *name, int size);

    nux::NString m_TooltipText;
    //! the window this icon belong too.
    nux::BaseWindow* m_Window;
    Launcher* m_IconManager;

    nux::Vector4  _xform_screen_coord [4];
    nux::Vector4  _xform_icon_screen_coord [4];
    bool          _mouse_inside;
    float         _folding_angle;

    nux::Tooltip* _tooltip;


    friend class Launcher;
    friend class LauncherController;

private:
    static gboolean OnPresentTimeout (gpointer data);

    nux::Color ColorForIcon (GdkPixbuf *pixbuf);

    nux::Color       _background_color;
    bool             _visible;
    bool             _active;
    bool             _running;
    bool             _urgent;
    bool             _presented;
    int              _sort_priority;
    int              _related_windows;
    guint            _present_time_handle;
    
    LauncherIconType _icon_type;
    
    struct timespec   _show_time;
    struct timespec   _hide_time;
    struct timespec   _running_time;
    struct timespec   _urgent_time;
    struct timespec   _present_time;
    struct timespec   _unpresent_time;
};

#endif // LAUNCHERICON_H

