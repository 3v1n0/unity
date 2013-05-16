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
 *              Michael Vogt <mvo@ubuntu.com>
 */

#include "config.h"

#include <NuxCore/Logger.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include "SoftwareCenterLauncherIcon.h"
#include "Launcher.h"
#include "LauncherDragWindow.h"
#include "LauncherModel.h"
#include "DesktopUtilities.h"

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

SoftwareCenterLauncherIcon::SoftwareCenterLauncherIcon(ApplicationPtr const& app,
                                                       std::string const& aptdaemon_trans_id,
                                                       std::string const& icon_path)
  : ApplicationLauncherIcon(app)
  , aptdaemon_trans_("org.debian.apt",
                     aptdaemon_trans_id,
                     "org.debian.apt.transaction",
                     G_BUS_TYPE_SYSTEM,
                     G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START)
  , finished_(true)
  , needs_urgent_(false)
  , aptdaemon_trans_id_(aptdaemon_trans_id)
  , app_title_(app->title())
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

  // FIXME: this needs testing, if there is no useful coordinates 
  //        then do not animate
  if(start_x <= 0 && start_y <= 0)
  {
     SetQuirk(Quirk::VISIBLE, true);
     return;
  }

  icon_texture_ = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture(
    launcher->GetWidth(),
    launcher->GetWidth(),
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

std::string SoftwareCenterLauncherIcon::GetActualDesktopFileAfterInstall()
{
   // Fixup the _desktop_file because the one we get from software-center
   // is not the final one, e.g. the s-c-agent does send a temp one and
   // app-install-data points to the "wrong" one in /usr/share/app-install
   //
   // So:
   // - if there is a desktop file already and it startswith 
   //   /usr/share/app-install/desktop, then transform to 
   //   /usr/share/application
   // - if there is a desktop file with prefix /tmp/software-center-agent:
   //   transform to /usr/share/application
   //   (its using "/tmp/software-center-agent:$random:$pkgname.desktop")
   // maybe:
   // - and search in /var/lib/apt/lists/$pkgname.list
   //   for a desktop file that roughly matches what we want
   std::string filename = _desktop_file;

   // take /usr/share/app-install/desktop/foo:subdir__bar.desktop
   // and tranform it
   if (_desktop_file.find("/share/app-install/desktop/") != std::string::npos)
   {
      filename = filename.substr(filename.rfind(":") + 1, 
                                 filename.length() - filename.rfind(":"));
      // the app-install-data package encodes subdirs in a funny way, once
      // that is fixed, this code can be dropped
      if (filename.find("__") != std::string::npos)
      {
         int pos = filename.find("__");
         filename = filename.replace(pos, 2, "-");
      }
      filename = DesktopUtilities::GetDesktopPathById(filename);
      return filename;
   } 
   else if (_desktop_file.find("/tmp/software-center-agent:") == 0)
   {
      // by convention the software-center-agent uses 
      //   /usr/share/applications/$pkgname.desktop
      // or
      //   /usr/share/applications/extras-$pkgname.desktop
      std::string desktopf = filename.substr(filename.rfind(":") + 1, 
                                             filename.length() - filename.rfind(":"));
      filename = DesktopUtilities::GetDesktopPathById(desktopf);
      if (filename.size() > 0)
         return filename;
      // now try extras-$pkgname.desktop
      filename = DesktopUtilities::GetDesktopPathById(std::string("extras-") + desktopf);
      if (filename.size() > 0)
         return filename;

      // FIXME: test if there is a file now and if not, search
      //        /var/lib/dpkg/info/$pkgname.list for a desktop file
   }

   return _desktop_file;
}

void SoftwareCenterLauncherIcon::OnFinished(GVariant *params)
{
   glib::String exit_state;
   g_variant_get_child(params, 0, "s", &exit_state);

   if (exit_state.Str() == "exit-success")
   {
      tooltip_text = app_title_;
      SetQuirk(Quirk::PROGRESS, false);
      SetQuirk(Quirk::URGENT, true);
      SetProgress(0.0f);
      finished_ = true;
      needs_urgent_ = true;

      // find and update to  the real desktop file
      std::string new_desktop_path = GetActualDesktopFileAfterInstall();

      // exchange the temp Application with the real one
      SetApplication(ApplicationManager::Default().GetApplicationForDesktopFile(new_desktop_path));

      UpdateDesktopFile();
      UpdateRemoteUri();

      Stick();
      position_saved.emit();

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
