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
  SimpleLauncherIcon();
  virtual ~SimpleLauncherIcon();

  // override
  nux::BaseTexture* GetTextureForSize(int size);

  // Properties
  nux::Property<std::string> icon_name;

  // Signals
  sigc::signal<void> activate;

protected:
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

  virtual void OnMouseDown(int button, int monitor, unsigned long key_flags = 0);
  virtual void OnMouseUp(int button, int monitor, unsigned long key_flags = 0);
  virtual void OnMouseClick(int button, int monitor, unsigned long key_flags = 0);
  virtual void OnMouseEnter(int monitor);
  virtual void OnMouseLeave(int monitor);
  virtual void ActivateLauncherIcon(ActionArg arg);

private:
  void ReloadIcon();
  static void OnIconThemeChanged(GtkIconTheme* icon_theme, gpointer data);
  bool SetIconName(std::string& target, std::string const& value);

private:
  guint32 theme_changed_id_;

  std::map<int, nux::BaseTexture*> texture_map;
  int last_size_;
};

}
}

#endif // SIMPLELAUNCHERICON_H

