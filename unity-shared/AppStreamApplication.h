// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2016 Canonical Ltd
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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#ifndef UNITYSHARED_APPSTREAM_APPLICATION_H
#define UNITYSHARED_APPSTREAM_APPLICATION_H

#include "DesktopApplicationManager.h"

namespace unity
{
namespace appstream
{

class Application : public desktop::Application
{
public:
  Application(std::string const& appstream_id);

  AppType type() const override;
  std::string repr() const override;

  WindowList const& GetWindows() const override;
  bool OwnsWindow(Window window_id) const override;

  std::vector<std::string> GetSupportedMimeTypes() const override;

  ApplicationWindowPtr GetFocusableWindow() const override;
  void Focus(bool show_on_visible, int monitor) const override;
  void Quit() const override;

  bool CreateLocalDesktopFile() const override;

  std::string desktop_id() const override;

private:
  std::string appstream_id_;
  std::string title_;
  glib::Object<_GdkPixbuf> icon_pixbuf_;
  WindowList window_list_;
};

}
}

#endif
