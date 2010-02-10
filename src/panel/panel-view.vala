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

namespace Unity.Panel
{
  static const int PANEL_HEIGHT = 24;

  public class View : Ctk.Actor
  {
    public Shell shell { get; construct;}

    private Clutter.Rectangle rect;
    private Tray.View         tray;
    private HomeButton        home;
    private Indicators.View   indicators;

    private Entry entry;

    private int indicators_width = 0;

    public View (Shell shell)
    {
      Object (shell:shell);
      this.tray.manage_stage (this.shell.get_stage ());
    }

    construct
    {
      START_FUNCTION ();

      Clutter.Color color = { 25, 25, 25, 255 };

      this.rect = new Clutter.Rectangle.with_color (color);
      this.rect.set_parent (this);
      this.rect.show ();

      this.indicators = new Indicators.View ();
      this.indicators.set_parent (this);
      this.indicators.show ();

      this.tray = new Tray.View ();
      this.tray.set_parent (this);
      this.tray.show ();

      this.home = new HomeButton (this.shell);
      this.home.set_parent (this);
      this.home.show ();

      this.entry = new Unity.Entry ("Search");
      this.entry.set_parent (this);
      this.entry.show ();
      this.entry.activate.connect (() =>
        {
          var text = Uri.escape_string (this.entry.text, "", true);

          var command = "xdg-open http://www.google.com/search?ie=UTF-8&q=%s".printf (text);

          try
            {
              Process.spawn_command_line_async (command);
            }
          catch (Error e)
            {
              warning ("Unable to search for '$text': %s", e.message);
            }
        });

      END_FUNCTION ();
    }

    private override void allocate (Clutter.ActorBox        box,
                                    Clutter.AllocationFlags flags)
    {
      Clutter.ActorBox child_box = { 0, 0, box.x2 - box.x1, box.y2 - box.y1 };
      float            width;
      float            child_width;
      float            i_width;

      width = box.x2 - box.x1;

      /* First the background */
      this.rect.allocate (child_box, flags);

      /* Home button */
      child_box.x1 = 17; /* (QL_width - logo_width)/2.0 */
      child_box.x2 = child_box.x1 + PANEL_HEIGHT;
      child_box.y1 = 0;
      child_box.y2 = PANEL_HEIGHT;
      this.home.allocate (child_box, flags);

      /* Entry */
      child_box.x1 = child_box.x2 + 12;
      child_box.x2 = child_box.x1 + 250; /* Random width */
      this.entry.allocate (child_box, flags);

      /* Indicators */
      this.indicators.get_preferred_width (PANEL_HEIGHT,
                                           out child_width,
                                           out child_width);
      child_box.x1 = width - child_width;
      child_box.x2 = width;
      this.indicators.allocate (child_box, flags);

      width -= child_width + 12; /* 12 = space between icons */

      /* Systray */
      this.tray.get_preferred_width (PANEL_HEIGHT,
                                     out child_width,
                                     out child_width);
      child_box.x1 = width - child_width;
      child_box.x2 = width;
      this.tray.allocate (child_box, flags);

      width -= child_box.x2 - child_box.x1;
      i_width = box.x2 - box.x1 - width;
      if (this.indicators_width != (int)i_width)
        {
          this.indicators_width = (int)i_width;
          this.shell.indicators_changed (this.indicators_width);
        }
    }

    private override void paint ()
    {
      base.paint ();
      this.rect.paint ();
      this.tray.paint ();
      this.home.paint ();
      this.indicators.paint ();
      this.entry.paint ();
    }

    private override void pick (Clutter.Color color)
    {
      this.tray.paint ();
      this.home.paint ();
      this.indicators.paint ();
      this.entry.paint ();
    }

    private override void map ()
    {
      base.map ();
      this.rect.map ();
      this.tray.map ();
      this.home.map ();
      this.indicators.map ();
      this.entry.map ();
    }

    private override void unmap ()
    {
      base.unmap ();
      this.rect.unmap ();
      this.tray.unmap ();
      this.home.unmap ();
      this.indicators.unmap ();
      this.entry.unmap ();
    }

    public int get_indicators_width ()
    {
      return (int)this.indicators_width;
    }

    public void set_indicator_mode (bool mode)
    {
      float x;

      x = mode ? this.width - this.get_indicators_width () : 0;

      this.rect.set_clip (x,
                          0,
                          mode ? this.get_indicators_width () : this.width,
                          mode ? PANEL_HEIGHT -1 : PANEL_HEIGHT);
    }
  }
}
