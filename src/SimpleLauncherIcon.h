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
    virtual void OnMouseDown (int button);
    virtual void OnMouseUp (int button);
    virtual void OnMouseClick (int button);
    virtual void OnMouseEnter ();
    virtual void OnMouseLeave ();
private:
    
    char *m_IconName;
    nux::BaseTexture *m_Icon;

};

#endif // BAMFLAUNCHERICON_H

