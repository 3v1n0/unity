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

class LauncherIcon: public sigc::trackable
{
public:
    LauncherIcon(Launcher* IconManager);
    ~LauncherIcon();

    void         SetTooltipText (const TCHAR* text);
    
    nux::NString GetTooltipText ();
    
    bool Visible ();
    bool Active  ();
    bool Running ();

    void RecvMouseEnter ();

    void RecvMouseLeave ();
    
    void HideTooltip ();
    
    int SortPriority ();
    
    LauncherIconType Type ();
    
    nux::Color BackgroundColor ();
    
    nux::BaseTexture * TextureForSize (int size);
    
    sigc::signal<void> MouseDown;
    sigc::signal<void> MouseUp;
    sigc::signal<void> MouseEnter;
    sigc::signal<void> MouseLeave;
    sigc::signal<void> MouseClick;
    
    sigc::signal<void, void *> show;
    sigc::signal<void, void *> hide;
    sigc::signal<void, void *> remove;
    sigc::signal<void, void *> needs_redraw;
protected:

    void SetVisible (bool visible);
    void SetActive (bool active);
    void SetRunning (bool running);
    
    void SetIconType (LauncherIconType type);
    void SetSortPriority (int priority);

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
    nux::Color ColorForIcon (GdkPixbuf *pixbuf);

    nux::Color       _background_color;
    bool             _visible;
    bool             _active;
    bool             _running;
    int              _sort_priority;
    LauncherIconType _icon_type;
};

#endif // LAUNCHERICON_H

