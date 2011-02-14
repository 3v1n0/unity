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

#ifndef PANEL_TRAY_H
#define PANEL_TRAY_H

#include <Nux/View.h>
#include <gtk/gtk.h>

#include "IndicatorObjectProxy.h"
#include "Introspectable.h"
#include "PanelIndicatorObjectView.h"

class PanelTray : public PanelIndicatorObjectView
{
public:

  PanelTray ();
  ~PanelTray ();

  void OnEntryAdded (IndicatorObjectEntryProxy *proxy);
  void OnEntryMoved (IndicatorObjectEntryProxy *proxy);
  void OnEntryRemoved (IndicatorObjectEntryProxy *proxy);

protected:
  const gchar * GetName ();
  const gchar * GetChildsName ();
  void          AddProperties (GVariantBuilder *builder);

private:
};
#endif
