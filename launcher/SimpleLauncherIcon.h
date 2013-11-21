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

#ifndef UNITYSHELL_SIMPLELAUNCHERICON_H
#define UNITYSHELL_SIMPLELAUNCHERICON_H

#include <UnityCore/GLibSignal.h>
#include "LauncherIcon.h"

namespace unity
{
namespace launcher
{

class Launcher;

class SimpleLauncherIcon : public LauncherIcon
{
  NUX_DECLARE_OBJECT_TYPE(SimpleLauncherIcon, LauncherIcon);
public:
  SimpleLauncherIcon(IconType type);

  // Properties
  nux::Property<std::string> icon_name;

protected:
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

  nux::BaseTexture* GetTextureForSize(int size) override;
  virtual void ActivateLauncherIcon(ActionArg arg);

private:
  void ReloadIcon();
  bool SetIconName(std::string& target, std::string const& value);

private:
  std::unordered_map<int, BaseTexturePtr> texture_map_;
  glib::Signal<void, GtkIconTheme*> theme_changed_signal_;
};

}
}

#endif // SIMPLELAUNCHERICON_H

