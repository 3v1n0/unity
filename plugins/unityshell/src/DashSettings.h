// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2010, 2011 Canonical Ltd
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
* Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
*/

#ifndef DASH_SETTINGS_H
#define DASH_SETTINGS_H

#include <sigc++/signal.h>

namespace unity
{
namespace dash
{

enum class FormFactor
{
  DESKTOP = 1,
  NETBOOK
};

class Settings
{
public:
  Settings();
  ~Settings();

  static Settings& Instance();

  // NOTE: could potentially refactor this into a nux::Property
  FormFactor GetFormFactor() const;
  void SetFormFactor(FormFactor factor);

  sigc::signal<void> changed;

private:
  class Impl;
  Impl* pimpl;
};

}
}

#endif // DASH_SETTINGS_H
