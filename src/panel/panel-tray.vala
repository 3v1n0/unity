/* -*- Mode: vala; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
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
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

using Gee;

namespace Unity.Panel.Tray
{
  public class View : Ctk.Box
  {
    private TrayManager manager;
    private Clutter.Stage stage;

    private int n_icons = 1;

    public View ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL,
              spacing:12);
    }

    construct
    {
      this.manager = new TrayManager ();
      this.manager.tray_icon_added.connect (this.on_tray_icon_added);
      this.manager.tray_icon_removed.connect (this.on_tray_icon_removed);
    }

    public void manage_stage (Clutter.Stage stage)
    {
      this.stage = stage;

      Idle.add (this.manage_tray_idle);
    }

    private bool manage_tray_idle ()
    {
      string? disable_tray = Environment.get_variable ("UNITY_DISABLE_TRAY");

      if (disable_tray == null)
        this.manager.manage_stage (this.stage);

      return false;
    }

    private int order_icons (Clutter.Actor a, Clutter.Actor b)
    {
      weak string stra = (string)a.get_data ("n_icon");
      weak string strb = (string)b.get_data ("n_icon");

      return strcmp ((stra != null) ? stra : "",
                     (strb != null) ? strb : "");
    }

    private void on_tray_icon_added (Clutter.Actor icon)
    {
      this.add_actor (icon);
      icon.set_size (22, 22);
      icon.show ();

      icon.set_data ("n_icon", (void*)"%d".printf (this.n_icons));

      this.sort_children ((CompareFunc)this.order_icons);

      this.n_icons++;
    }

    private void on_tray_icon_removed (Clutter.Actor icon)
    {
      this.remove_actor (icon);
    }
  }
}
