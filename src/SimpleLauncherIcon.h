#ifndef SIMPLELAUNCHERICON_H
#define SIMPLELAUNCHERICON_H

/* Compiz */
#include <core/core.h>

#include <Nux/BaseWindow.h>
#include <NuxCore/Math/MathInc.h>
#include <sigc++/sigc++.h>

#include "LauncherIcon.h"

class Launcher;

class SimpleLauncherIcon : public LauncherIcon
{
public:
    SimpleLauncherIcon(Launcher* IconManager, NUX_FILE_LINE_PROTO);
    ~SimpleLauncherIcon();
    
    /* override */
    nux::BaseTexture * GetTextureForSize (int size);
    
    void SetIconName (const char *name);

protected:
    virtual void OnMouseDown ();
    virtual void OnMouseUp ();
    virtual void OnMouseClick ();
    virtual void OnMouseEnter ();
    virtual void OnMouseLeave ();
private:
    
    char *m_IconName;
    nux::BaseTexture *m_Icon;

};

#endif // BAMFLAUNCHERICON_H

