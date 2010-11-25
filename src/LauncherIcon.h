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
#include "QuicklistView.h"

class Launcher;
class QuicklistView;

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

typedef enum
{
  LAUNCHER_ICON_QUIRK_VISIBLE,
  LAUNCHER_ICON_QUIRK_ACTIVE,
  LAUNCHER_ICON_QUIRK_RUNNING,
  LAUNCHER_ICON_QUIRK_URGENT,
  LAUNCHER_ICON_QUIRK_PRESENTED,
  LAUNCHER_ICON_QUIRK_STARTING,
  
  LAUNCHER_ICON_QUIRK_LAST,
} LauncherIconQuirk;

class LauncherIcon : public nux::InitiallyUnownedObject, public sigc::trackable
{
public:
    LauncherIcon(Launcher* launcher);
    ~LauncherIcon();

    void SetTooltipText (const TCHAR* text);
    
    nux::NString GetTooltipText ();
    
    void RecvMouseEnter ();
    void RecvMouseLeave ();
    void RecvMouseDown (int button);
    void RecvMouseUp (int button);
    
    void RecvShowQuicklist (nux::BaseWindow *quicklist);
    void RecvHideQuicklist (nux::BaseWindow *quicklist);
    
    void HideTooltip ();
    
    void        SetCenter (nux::Point3 center);
    nux::Point3 GetCenter ();
    
    int SortPriority ();
    
    int RelatedWindows ();
    int PresentUrgency ();
    
    bool GetQuirk (LauncherIconQuirk quirk);
    struct timespec GetQuirkTime (LauncherIconQuirk quirk);
    
    LauncherIconType Type ();
    
    nux::Color BackgroundColor ();
    nux::Color GlowColor ();
    
    nux::BaseTexture * TextureForSize (int size);
    
    std::list<DbusmenuClient *> Menus ();
    
    
    sigc::signal<void, int> MouseDown;
    sigc::signal<void, int> MouseUp;
    sigc::signal<void>      MouseEnter;
    sigc::signal<void>      MouseLeave;
    sigc::signal<void, int> MouseClick;
    
    sigc::signal<void, void *> show;
    sigc::signal<void, void *> hide;
    sigc::signal<void, void *> remove;
    sigc::signal<void, void *> needs_redraw;
protected:
    void SetQuirk (LauncherIconQuirk quirk, bool value);

    void UpdateQuirkTime (LauncherIconQuirk quirk);
    void ResetQuirkTime (LauncherIconQuirk quirk);

    void SetRelatedWindows (int windows);
    void Remove ();
    
    
    void Present (int urgency, int length);
    void Unpresent ();
    
    void SetIconType (LauncherIconType type);
    void SetSortPriority (int priority);

    virtual std::list<DbusmenuClient *> GetMenus ();
    virtual nux::BaseTexture * GetTextureForSize (int size) = 0;
    
    virtual void OnCenterStabilized (nux::Point3 center) {};
    virtual bool IconOwnsWindow (Window w) { return false; }

    nux::BaseTexture * TextureFromGtkTheme (const char *name, int size);

    nux::NString m_TooltipText;
    //! the window this icon belong too.
    nux::BaseWindow* m_Window;
    Launcher* _launcher;

    std::map<std::string, nux::Vector4*> _xform_coords;
    bool          _mouse_inside;
    float         _folding_angle;

    nux::Tooltip *_tooltip;
    QuicklistView *_quicklist;

    static nux::Tooltip *_current_tooltip;
    static QuicklistView *_current_quicklist;


    friend class Launcher;
    friend class LauncherController;

private:
    static void ChildRealized (DbusmenuMenuitem *newitem, QuicklistView *quicklist);
    static void RootChanged (DbusmenuClient * client, DbusmenuMenuitem *newroot, QuicklistView *quicklist);
    static gboolean OnPresentTimeout (gpointer data);
    static gboolean OnCenterTimeout (gpointer data);

    void ColorForIcon (GdkPixbuf *pixbuf, nux::Color &background, nux::Color &glow);

    nux::Color       _background_color;
    nux::Color       _glow_color;
    int              _sort_priority;
    int              _related_windows;
    int              _present_urgency;
    guint            _present_time_handle;
    guint            _center_stabilize_handle;
    bool             _quicklist_is_initialized;
    
    nux::Point3      _center;
    nux::Point3      _last_stable;
    LauncherIconType _icon_type;
    
    bool             _quirks[LAUNCHER_ICON_QUIRK_LAST];
    struct timespec  _quirk_times[LAUNCHER_ICON_QUIRK_LAST];
    
};

#endif // LAUNCHERICON_H

