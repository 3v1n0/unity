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

#include "Nux/Nux.h"
#include "Nux/BaseWindow.h"

#include "SimpleLauncherIcon.h"
#include "Launcher.h"

SimpleLauncherIcon::SimpleLauncherIcon (Launcher* IconManager)
:   LauncherIcon(IconManager)
{
    m_Icon = 0;
    m_IconName = 0;
    
    LauncherIcon::MouseDown.connect (sigc::mem_fun (this, &SimpleLauncherIcon::OnMouseDown));
    LauncherIcon::MouseUp.connect (sigc::mem_fun (this, &SimpleLauncherIcon::OnMouseUp));
    LauncherIcon::MouseClick.connect (sigc::mem_fun (this, &SimpleLauncherIcon::OnMouseClick));
    LauncherIcon::MouseEnter.connect (sigc::mem_fun (this, &SimpleLauncherIcon::OnMouseEnter));
    LauncherIcon::MouseLeave.connect (sigc::mem_fun (this, &SimpleLauncherIcon::OnMouseLeave));
}

SimpleLauncherIcon::~SimpleLauncherIcon()
{
    if (m_Icon)
      m_Icon->UnReference ();
}

void
SimpleLauncherIcon::OnMouseDown (int button)
{
}

void
SimpleLauncherIcon::OnMouseUp (int button)
{
}

void
SimpleLauncherIcon::OnMouseClick (int button)
{
}

void
SimpleLauncherIcon::OnMouseEnter ()
{
}

void
SimpleLauncherIcon::OnMouseLeave ()
{
}

nux::BaseTexture *
SimpleLauncherIcon::GetTextureForSize (int size)
{
    if (m_Icon && size == m_Icon->GetHeight ())
      return m_Icon;
      
    if (m_Icon)
      m_Icon->UnReference ();
    
    if (!m_IconName)
      return 0;
    
    m_Icon = TextureFromGtkTheme (m_IconName, size);
    return m_Icon;
}

void 
SimpleLauncherIcon::SetIconName (const char *name)
{
    m_IconName = g_strdup (name);
}
