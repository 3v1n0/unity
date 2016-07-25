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

namespace unity
{
namespace launcher
{

namespace
{
const int INSTALL_TIP_DURATION = 1500;
}

NUX_IMPLEMENT_OBJECT_TYPE(SoftwareCenterLauncherIcon);
SoftwareCenterLauncherIcon::SoftwareCenterLauncherIcon(ApplicationPtr const& app,
                                                       std::string const& aptdaemon_trans_id)
  : WindowedLauncherIcon(IconType::APPLICATION)
  , ApplicationLauncherIcon(app)
  , aptdaemon_trans_(std::make_shared<glib::DBusProxy>("org.debian.apt",
                                                       aptdaemon_trans_id,
                                                       "org.debian.apt.transaction",
                                                       G_BUS_TYPE_SYSTEM,
                                                       G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START))
  , finished_(false)
  , needs_urgent_(false)
  , aptdaemon_trans_id_(aptdaemon_trans_id)
{
  Stick(false);
  SetQuirk(Quirk::VISIBLE, false);
  SkipQuirkAnimation(Quirk::VISIBLE);

  aptdaemon_trans_->Connect("PropertyChanged", sigc::mem_fun(this, &SoftwareCenterLauncherIcon::OnPropertyChanged));
  aptdaemon_trans_->Connect("Finished", sigc::mem_fun(this, &SoftwareCenterLauncherIcon::OnFinished));
  aptdaemon_trans_->GetProperty("Progress", [this] (GVariant *value) {
    int32_t progress = glib::Variant(value).GetInt32();
    SetProgress(progress/100.0f);
    SetQuirk(Quirk::PROGRESS, (progress > 0));
  });

  if (app->icon_pixbuf())
    icon_pixbuf = app->icon_pixbuf();

  if (!aptdaemon_trans_id_.empty()) // Application is being installed, or hasn't been installed yet
    tooltip_text = _("Waiting to install");
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
  return DesktopUtilities::GetDesktopPathById(DesktopFile());
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
    SetApplication(new_app);

    if (new_app)
    {
      Stick();

      _source_manager.AddIdle([this] {
        ShowTooltip();
        _source_manager.AddTimeout(INSTALL_TIP_DURATION, [this] {
          HideTooltip();
          return false;
        });
        return false;
      });
    }
  }
  else
  {
    // failure condition, remove icon again
    Remove();
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
