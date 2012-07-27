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

#include <NuxCore/Logger.h>
#include <glib/gi18n-lib.h>
#include "SoftwareCenterLauncherIcon.h"
#include "Launcher.h"
#include "LauncherDragWindow.h"
#include "LauncherModel.h"

namespace unity
{
namespace launcher
{

NUX_IMPLEMENT_OBJECT_TYPE(SoftwareCenterLauncherIcon);

SoftwareCenterLauncherIcon::SoftwareCenterLauncherIcon(BamfApplication* app,
                                                       std::string const& aptdaemon_trans_id,
                                                       std::string const& icon_path)
: BamfLauncherIcon(app),
  aptdaemon_trans_("org.debian.apt",
                   aptdaemon_trans_id,
                   "org.debian.apt.transaction",
                   G_BUS_TYPE_SYSTEM,
                   G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START)
, finished_(true)
, needs_urgent_(false)
, aptdaemon_trans_id_(aptdaemon_trans_id)
{

  aptdaemon_trans_.Connect("PropertyChanged", sigc::mem_fun(this, &SoftwareCenterLauncherIcon::OnPropertyChanged));
  aptdaemon_trans_.Connect("Finished", sigc::mem_fun(this, &SoftwareCenterLauncherIcon::OnFinished));

  SetIconType(TYPE_APPLICATION);
  icon_name = icon_path;
  if (!aptdaemon_trans_id_.empty()) // Application is being installed, or hasn't been installed yet
    tooltip_text = _("Waiting to install");
}

void SoftwareCenterLauncherIcon::Animate(nux::ObjectPtr<Launcher> launcher,
                                        int icon_x,
                                        int icon_y,
                                        int icon_size)
{
  int target_x = 0;
  int target_y = 0;

  launcher_ = launcher;

  icon_texture_ = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture(
    launcher->GetIconSize(),
    launcher->GetIconSize(),
    1,
    nux::BITFMT_R8G8B8A8);

  drag_window_ = new LauncherDragWindow(icon_texture_);

  launcher->RenderIconToTexture(nux::GetWindowThread()->GetGraphicsEngine(),
                                AbstractLauncherIcon::Ptr(this),
                                icon_texture_);

  drag_window_->SetBaseXY(icon_x, icon_y);
  drag_window_->ShowWindow(true);

  // Find out the center of last BamfLauncherIcon with non-zero co-ordinates
  auto bamf_icons = launcher->GetModel()->GetSublist<BamfLauncherIcon>();
  //TODO: don't iterate through them and pick the last one, just use back() to get the last one.
  for (auto current_bamf_icon : bamf_icons)
  {
    auto icon_center = current_bamf_icon->GetCenter(launcher->monitor);

    if (icon_center.x != 0 && icon_center.y != 0)
    {
       target_x = icon_center.x;
       target_y = icon_center.y;
    }
  }

  target_y = target_y + (launcher->GetIconSize() / 2);
  drag_window_->SetAnimationTarget(target_x, target_y);

  drag_window_->on_anim_completed = drag_window_->anim_completed.connect(sigc::mem_fun(this, &SoftwareCenterLauncherIcon::OnDragAnimationFinished));
  drag_window_->StartAnimation();
}

void SoftwareCenterLauncherIcon::OnDragAnimationFinished()
{
  drag_window_->ShowWindow(false);
  launcher_->icon_animation_complete.emit(AbstractLauncherIcon::Ptr(this));
  drag_window_ = nullptr;
}

void SoftwareCenterLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  if (finished_)
  {
      if (needs_urgent_)
      {
          SetQuirk(QUIRK_URGENT, false);
          needs_urgent_ = false;
      }
      BamfLauncherIcon::ActivateLauncherIcon(arg);
  }
  else
      SetQuirk(QUIRK_STARTING, false);
}

void SoftwareCenterLauncherIcon::OnFinished(GVariant *params)
{
   glib::String exit_state;
   g_variant_get_child(params, 0, "s", &exit_state);

   if (exit_state.Str() == "exit-success")
   {
      tooltip_text = BamfName();
      SetQuirk(QUIRK_PROGRESS, false);
      SetQuirk(QUIRK_URGENT, true);
      SetProgress(0.0f);
      finished_ = true;
      needs_urgent_ = true;
   } else {
      // failure condition, remove icon again
      UnStick();
   }
};

void SoftwareCenterLauncherIcon::OnPropertyChanged(GVariant* params)
{
  gint32 progress;
  glib::String property_name;

  g_variant_get_child(params, 0, "s", &property_name);

  if (property_name.Str() == "Progress")
  {
    GVariant* property_value = nullptr;
    g_variant_get_child(params, 1, "v", &property_value);
    g_variant_get(property_value, "i", &progress);

    if (progress < 100)
    {
      SetQuirk(QUIRK_PROGRESS, true);
      finished_ = false;
    }

    SetProgress(progress/100.0f);
    g_variant_unref(property_value);
  }
}

std::string SoftwareCenterLauncherIcon::GetName() const
{
    return "SoftwareCenterLauncherIcon";
}

}
}
