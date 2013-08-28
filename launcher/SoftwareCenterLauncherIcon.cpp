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

#include <functional>

#include <NuxCore/Logger.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include "SoftwareCenterLauncherIcon.h"
#include "Launcher.h"
#include "LauncherDragWindow.h"
#include "LauncherModel.h"
#include "DesktopUtilities.h"
#include "MultiMonitor.h"

namespace unity
{
namespace launcher
{

namespace
{
const std::string SOURCE_SHOW_TOOLTIP = "ShowTooltip";
const std::string SOURCE_HIDE_TOOLTIP = "HideTooltip";
const int INSTALL_TIP_DURATION = 1500;
}

NUX_IMPLEMENT_OBJECT_TYPE(SoftwareCenterLauncherIcon);
SoftwareCenterLauncherIcon::SoftwareCenterLauncherIcon(ApplicationPtr const& app,
                                                       std::string const& aptdaemon_trans_id,
                                                       std::string const& icon_path)
  : ApplicationLauncherIcon(app)
  , aptdaemon_trans_(std::make_shared<glib::DBusProxy>("org.debian.apt",
                                                       aptdaemon_trans_id,
                                                       "org.debian.apt.transaction",
                                                       G_BUS_TYPE_SYSTEM,
                                                       G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START))
  , finished_(true)
  , needs_urgent_(false)
  , aptdaemon_trans_id_(aptdaemon_trans_id)
{
  SetQuirk(Quirk::VISIBLE, false);
  aptdaemon_trans_->Connect("PropertyChanged", sigc::mem_fun(this, &SoftwareCenterLauncherIcon::OnPropertyChanged));
  aptdaemon_trans_->Connect("Finished", sigc::mem_fun(this, &SoftwareCenterLauncherIcon::OnFinished));
  aptdaemon_trans_->GetProperty("Progress", [this] (GVariant *value) {
    int32_t progress = glib::Variant(value).GetInt32();
    SetProgress(progress/100.0f);
    SetQuirk(Quirk::PROGRESS, (progress > 0));
  });

  if (!icon_path.empty())
    icon_name = icon_path;

  if (!aptdaemon_trans_id_.empty()) // Application is being installed, or hasn't been installed yet
    tooltip_text = _("Waiting to install");
}

void SoftwareCenterLauncherIcon::Animate(nux::ObjectPtr<Launcher> const& launcher, int start_x, int start_y)
{
  using namespace std::placeholders;

  // FIXME: this needs testing, if there is no useful coordinates
  //        then do not animate
  if (start_x <= 0 && start_y <= 0)
  {
    SetQuirk(Quirk::VISIBLE, true);
    return;
  }

  auto* floating_icon = new SimpleLauncherIcon(GetIconType());
  AbstractLauncherIcon::Ptr floating_icon_ptr(floating_icon);
  floating_icon->icon_name = icon_name();
  int monitor = launcher->monitor();

  // Transform this in a spacer-icon and make it visible only on launcher's monitor
  for (unsigned i = 0; i < monitors::MAX; ++i)
    SetVisibleOnMonitor(i, static_cast<int>(i) == monitor);

  icon_name = "";
  SetQuirk(Quirk::VISIBLE, true);

  auto const& icon_center = GetCenter(monitor);
  auto rcb = std::bind(&Launcher::RenderIconToTexture, launcher.GetPointer(), _1, _2, floating_icon_ptr);
  drag_window_ = new LauncherDragWindow(launcher->GetWidth(), rcb);
  drag_window_->SetBaseXY(start_x, start_y);
  drag_window_->SetAnimationTarget(icon_center.x, icon_center.y + (launcher->GetIconSize() / 2));

  launcher->ForceReveal(true);
  drag_window_->ShowWindow(true);

  auto cb = sigc::bind(sigc::mem_fun(this, &SoftwareCenterLauncherIcon::OnDragAnimationFinished), launcher, floating_icon->icon_name());
  drag_window_->anim_completed.connect(cb);
  drag_window_->StartSlowAnimation();
}

void SoftwareCenterLauncherIcon::OnDragAnimationFinished(nux::ObjectPtr<Launcher> const& launcher, std::string const& final_icon)
{
  icon_name = final_icon;
  drag_window_->ShowWindow(false);
  launcher->ForceReveal(false);
  drag_window_ = nullptr;

  for (unsigned i = 0; i < monitors::MAX; ++i)
    SetVisibleOnMonitor(i, true);
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
  auto const& desktop_file = DesktopFile();

  // take /usr/share/app-install/desktop/foo:subdir__bar.desktop
  // and tranform it
  if (desktop_file.find("/share/app-install/desktop/") != std::string::npos)
  {
    auto colon_pos = desktop_file.rfind(":");
    auto filename = desktop_file.substr(colon_pos + 1, desktop_file.length() - colon_pos);
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
  else if (desktop_file.find("/tmp/software-center-agent:") == 0)
  {
    // by convention the software-center-agent uses
    //   /usr/share/applications/$pkgname.desktop
    // or
    //   /usr/share/applications/extras-$pkgname.desktop
    auto colon_pos = desktop_file.rfind(":");
    auto desktopf = desktop_file.substr(colon_pos + 1, desktop_file.length() - colon_pos);

    auto filename = DesktopUtilities::GetDesktopPathById(desktopf);

    if (!filename.empty())
      return filename;

    // now try extras-$pkgname.desktop
    filename = DesktopUtilities::GetDesktopPathById("extras-" + desktopf);
    if (!filename.empty())
      return filename;

    // FIXME: test if there is a file now and if not, search
    //        /var/lib/dpkg/info/$pkgname.list for a desktop file
  }

  return desktop_file;
}

void SoftwareCenterLauncherIcon::OnFinished(GVariant *params)
{
  if (glib::Variant(params).GetString() == "exit-success")
  {
    SetQuirk(Quirk::PROGRESS, false);
    SetQuirk(Quirk::URGENT, true);
    SetProgress(0.0f);
    finished_ = true;
    needs_urgent_ = true;

    // find and update to the real desktop file
    std::string const& new_desktop_path = GetActualDesktopFileAfterInstall();

    // exchange the temp Application with the real one
    auto& app_manager = ApplicationManager::Default();
    auto const& new_app = app_manager.GetApplicationForDesktopFile(new_desktop_path);
    new_app->sticky = IsSticky();
    SetApplication(new_app);
    Stick();

    _source_manager.AddIdle([this] {
      ShowTooltip();
      _source_manager.AddTimeout(INSTALL_TIP_DURATION, [this] {
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

  aptdaemon_trans_.reset();
};

void SoftwareCenterLauncherIcon::OnPropertyChanged(GVariant* params)
{
  glib::Variant property_name(g_variant_get_child_value(params, 0), glib::StealRef());

  if (property_name.GetString() == "Progress")
  {
    int32_t progress = glib::Variant(g_variant_get_child_value(params, 1), glib::StealRef()).GetInt32();

    if (progress < 100)
    {
      SetQuirk(Quirk::PROGRESS, true);
      finished_ = false;
      tooltip_text = _("Installing…");
    }

    SetProgress(progress/100.0f);
  }
}

std::string SoftwareCenterLauncherIcon::GetName() const
{
  return "SoftwareCenterLauncherIcon";
}

}
}
