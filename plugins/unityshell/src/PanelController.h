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

#ifndef _PANEL_CONTROLLER_H_
#define _PANEL_CONTROLLER_H_

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <vector>

#include "Introspectable.h"
#include "PanelView.h"

class PanelController : public nux::Object, public Introspectable
{
public:
  PanelController  ();
  ~PanelController ();

  void SetBFBSize (int size);
  void StartFirstMenuShow ();
  void EndFirstMenuShow ();
  void SetOpacity (float opacity);

protected:
  const gchar * GetName       ();
  void          AddProperties (GVariantBuilder *builder);

private:
  unity::PanelView * ViewForWindow (nux::BaseWindow *window);
  void OnScreenChanged (int primary_monitor, std::vector<nux::Geometry>& monitors);
  
  static void WindowConfigureCallback (int            window_width,
                                       int            window_height,
                                       nux::Geometry& geo,
                                       void          *user_data);
private:
  std::vector<nux::BaseWindow *> _windows;
  int _bfb_size;
  float _opacity;
  
  sigc::connection _on_screen_change_connection;

  bool _open_menu_start_received;
};

#endif // _PANEL_CONTROLLER_H_
