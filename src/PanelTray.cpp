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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "PanelTray.h"

PanelTray::PanelTray ()
{

}

PanelTray::~PanelTray ()
{

}

void
PanelTray::OnEntryAdded (IndicatorObjectEntryProxy *proxy)
{

}

void
PanelTray::OnEntryMoved (IndicatorObjectEntryProxy *proxy)
{

}

void
PanelTray::OnEntryRemoved (IndicatorObjectEntryProxy *proxy)
{

}

const gchar *
PanelTray::GetName ()
{
  return "PanelTray";
}

const gchar *
PanelTray::GetChildsName ()
{
  return "";
}

void
PanelTray::AddProperties (GVariantBuilder *builder)
{

}

