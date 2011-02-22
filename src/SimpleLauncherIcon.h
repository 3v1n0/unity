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
 */

#ifndef SIMPLELAUNCHERICON_H
#define SIMPLELAUNCHERICON_H

#include "LauncherIcon.h"

class Launcher;

class SimpleLauncherIcon : public LauncherIcon
{
public:
    SimpleLauncherIcon(Launcher* IconManager);
    virtual ~SimpleLauncherIcon();
    
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
    void ActivateLauncherIcon ();
};

#endif // BAMFLAUNCHERICON_H

