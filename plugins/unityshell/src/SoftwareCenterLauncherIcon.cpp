// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Bilal Akhtar <bilalakhtar@ubuntu.com>
 *              Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include <glib/gi18n-lib.h>
#include "SoftwareCenterLauncherIcon.h"
#include "Launcher.h"

namespace unity
{
namespace launcher
{

SoftwareCenterLauncherIcon::SoftwareCenterLauncherIcon(BamfApplication* app,
                                                       std::string const& aptdaemon_trans_id,
                                                       std::string const& icon_path)
: BamfLauncherIcon(app),
  _aptdaemon_trans("org.debian.apt",
                   aptdaemon_trans_id,
                   "org.debian.apt.transaction",
                   G_BUS_TYPE_SYSTEM,
                   G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START)
{

  _aptdaemon_trans.Connect("PropertyChanged", sigc::mem_fun(this, &SoftwareCenterLauncherIcon::OnPropertyChanged));
  _aptdaemon_trans.Connect("Finished", [&] (GVariant *) {
    tooltip_text = BamfName();
    SetQuirk(QUIRK_PROGRESS, false);
    SetQuirk(QUIRK_URGENT, true);
    SetProgress(0.0f);
    finished = true;
    finished_just_now = true;
  });

  SetIconType(TYPE_APPLICATION);
  icon_name = icon_path.c_str();
  tooltip_text = _("Waiting to install");

  // Since the transaction COULD have completed by now, assume true for now
  finished = true;
  finished_just_now = false;
  
}

void
SoftwareCenterLauncherIcon::Animate(nux::ObjectPtr<Launcher> launcher,
                                    gint32 icon_x,
                                    gint32 icon_y,
                                    gint32 icon_size)
{
    
  _icon_texture = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture(launcher->GetIconSize(), launcher->GetIconSize(), 1, nux::BITFMT_R8G8B8A8);
  _drag_window = new LauncherDragWindow(_icon_texture);

  launcher->RenderIconToTexture(nux::GetWindowThread()->GetGraphicsEngine(), AbstractLauncherIcon::Ptr(this), _icon_texture);
  nux::Geometry geo = _drag_window->GetGeometry();
  //_drag_window->SetBaseXY(geo.width / 2, geo.height / 2);
  _drag_window->SetBaseXY(icon_x, icon_y);
  _drag_window->ShowWindow(true);
  _drag_window->SinkReference();

  g_debug("Co-ordinates are: %f, %f", GetCenter(0).x, GetCenter(0).y);

  _drag_window->SetAnimationTarget(icon_x+60,icon_x+60);
  _drag_window->StartAnimation();
}

void SoftwareCenterLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
    if (finished)
    {
        if (finished_just_now)
        {
            SetQuirk(QUIRK_URGENT, false);
            finished_just_now = false;
        }
        BamfLauncherIcon::ActivateLauncherIcon(arg);
    }
    else
        SetQuirk(QUIRK_STARTING, false);
}


void
SoftwareCenterLauncherIcon::OnPropertyChanged(GVariant* params)
{
  gint32 progress;
  glib::String property_name;
  GVariant* property_value;

  g_variant_get_child(params, 0, "s", property_name.AsOutParam());

  if (property_name.Str() == "Progress")
  {
    g_variant_get_child(params, 1, "v", &property_value);
    g_variant_get(property_value, "i", &progress);

    if (progress < 100)
    {
      SetQuirk(QUIRK_PROGRESS, true);
      finished = false;
    }

    SetProgress(progress/100.0f);
  }

  g_variant_unref(property_value);
}

}
}
