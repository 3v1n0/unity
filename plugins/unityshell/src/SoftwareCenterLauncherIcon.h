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
 * Authored by: Bilal Akhtar <bilalakhtar@ubuntu.com>
 */

#ifndef SOFTWARECENTERLAUNCHERICON_H
#define SOFTWARECENTERLAUNCHERICON_H

#include "BamfLauncherIcon.h"
#include <Nux/BaseWindow.h>
#include <NuxCore/Math/MathInc.h>
#include <core/core.h>
#include <gio/gio.h>
#include <glib.h>
#include <glib/gvariant.h>
#include <UnityCore/GLibDBusProxy.h>


namespace unity
{
namespace launcher
{


class SoftwareCenterLauncherIcon : public BamfLauncherIcon
{
public:

    SoftwareCenterLauncherIcon(Launcher* IconManager, BamfApplication* app, char* aptdaemon_trans_id, char* icon_path);
    virtual ~SoftwareCenterLauncherIcon();

    gchar* original_tooltip_text;

private:
    char* _aptdaemon_trans_id;
    unity::glib::DBusProxy* _aptdaemon_trans;

    void OnFinished(GVariant* params);

    void OnPropertyChanged(GVariant* params);
};

}
}

#endif
