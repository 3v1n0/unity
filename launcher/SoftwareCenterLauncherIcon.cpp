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
#include "config.h"
#include <glib/gi18n-lib.h>
#include "SoftwareCenterLauncherIcon.h"
#include "Launcher.h"
#include "LauncherDragWindow.h"
#include "LauncherModel.h"

namespace unity
{
namespace launcher
{

namespace
{
#define SOURCE_SHOW_TOOLTIP "ShowTooltip"
#define SOURCE_HIDE_TOOLTIP "HideTooltip"
const int INSTALL_TIP_DURATION = 1500;
}

NUX_IMPLEMENT_OBJECT_TYPE(SoftwareCenterLauncherIcon);

SoftwareCenterLauncherIcon::SoftwareCenterLauncherIcon(BamfApplication* app,
                                                       std::string const& aptdaemon_trans_id,
                                                       std::string const& icon_path)
: ApplicationLauncherIcon(app),
  aptdaemon_trans_("org.debian.apt",
                   aptdaemon_trans_id,
                   "org.debian.apt.transaction",
                   G_BUS_TYPE_SYSTEM,
                   G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START)
, finished_(true)
, needs_urgent_(false)
, aptdaemon_trans_id_(aptdaemon_trans_id)
{
  SetQuirk(Quirk::VISIBLE, false);
  aptdaemon_trans_.Connect("PropertyChanged", sigc::mem_fun(this, &SoftwareCenterLauncherIcon::OnPropertyChanged));
  aptdaemon_trans_.Connect("Finished", sigc::mem_fun(this, &SoftwareCenterLauncherIcon::OnFinished));

  if (!icon_path.empty())
    icon_name = icon_path;

  if (!aptdaemon_trans_id_.empty()) // Application is being installed, or hasn't been installed yet
    tooltip_text = _("Waiting to install");
}

void SoftwareCenterLauncherIcon::Animate(nux::ObjectPtr<Launcher> const& launcher, int start_x, int start_y)
{
  launcher_ = launcher;

  icon_texture_ = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture(
    launcher->GetIconSize(),
    launcher->GetIconSize(),
    1,
    nux::BITFMT_R8G8B8A8);

  drag_window_ = new LauncherDragWindow(icon_texture_);

  launcher->ForceReveal(true);
  launcher->RenderIconToTexture(nux::GetWindowThread()->GetGraphicsEngine(),
                                AbstractLauncherIcon::Ptr(this),
                                icon_texture_);

  auto const& icon_center = GetCenter(launcher->monitor());
  drag_window_->SetBaseXY(start_x, start_y);
  drag_window_->ShowWindow(true);
  drag_window_->SetAnimationTarget(icon_center.x, icon_center.y + (launcher->GetIconSize() / 2));
  drag_window_->on_anim_completed = drag_window_->anim_completed.connect(sigc::mem_fun(this, &SoftwareCenterLauncherIcon::OnDragAnimationFinished));
  drag_window_->StartSlowAnimation();
}

void SoftwareCenterLauncherIcon::OnDragAnimationFinished()
{
  drag_window_->ShowWindow(false);
  drag_window_ = nullptr;
  launcher_->ForceReveal(false);
  launcher_ = nullptr;
  icon_texture_ = nullptr;
  SetQuirk(Quirk::VISIBLE, true);
}

void SoftwareCenterLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  if (finished_)
  {
    if (needs_urgent_)
    {
      SetQuirk(Quirk::URGENT, false);
      needs_urgent_ = false;
    }

    ApplicationLauncherIcon::ActivateLauncherIcon(arg);
  }
  else
  {
    SetQuirk(Quirk::STARTING, false);
  }
}

void SoftwareCenterLauncherIcon::OnFinished(GVariant *params)
{
   glib::String exit_state;
   g_variant_get_child(params, 0, "s", &exit_state);

   if (exit_state.Str() == "exit-success")
   {
      tooltip_text = BamfName();
      SetQuirk(Quirk::PROGRESS, false);
      SetQuirk(Quirk::URGENT, true);
      SetProgress(0.0f);
      finished_ = true;
      needs_urgent_ = true;

      _source_manager.AddIdle([this]()
      {
        ShowTooltip();
        _source_manager.AddTimeout(INSTALL_TIP_DURATION, [this]()
        {
          HideTooltip();
          return false;
        }, SOURCE_HIDE_TOOLTIP);
        return false;
      }, SOURCE_SHOW_TOOLTIP);
   }
   else
   {
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
      SetQuirk(Quirk::PROGRESS, true);
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
