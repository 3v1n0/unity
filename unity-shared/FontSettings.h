/*
 * Copyright (C) 2011 Canonical Ltd
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

#ifndef UNITY_FONT_SETTINGS_H_
#define UNITY_FONT_SETTINGS_H_

#include "GtkUtils.h"

namespace unity
{

class FontSettings : public sigc::trackable
{
public:
  FontSettings();

private:
  void Refresh();

private:
  gtk::Setting<std::string> hintstyle_;
  gtk::Setting<std::string> rgba_;
  gtk::Setting<int> antialias_;
};

}

#endif
