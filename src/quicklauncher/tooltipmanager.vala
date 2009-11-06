/* -*- Mode: vala; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * Copyright (C) 2009 Canonical Ltd
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
 * Authored by Mirco "MacSlow" Müller <mirco.mueller@canonical.com>
 *
 */

namespace Unity
{
  public class TooltipManager : Object
  {
    public static TooltipManager get_default ()
    {
      if (_instance == null)
      {
        _instance = new TooltipManager ();
	_instance.init ();
      }

      return _instance;
    }

    public class Tooltip : Object
    {
      public Gtk.Window window = null;
      public string     text   = "";

      private void
      _draw_round_rect (ref Cairo.Context cr,
                        double            aspect,
                        double            x,
                        double            y,
                        double            corner_radius,
                        double            width,
                        double            height)
      {
        double radius = corner_radius / aspect;

        cr.move_to (x + radius, y);
        cr.line_to (x + width - radius, y);
        cr.arc (x + width - radius,
                y + radius,
                radius,
                -90.0f * GLib.Math.PI / 180.0f,
                0.0f * GLib.Math.PI / 180.0f);
        cr.line_to (x + width, y + height - radius);
        cr.arc (x + width - radius,
                y + height - radius,
                radius,
                0.0f * GLib.Math.PI / 180.0f,
                90.0f * GLib.Math.PI / 180.0f);
        cr.line_to (x + radius, y + height);
        cr.arc (x + radius,
                y + height - radius,
                radius,
                90.0f * GLib.Math.PI / 180.0f,
                180.0f * GLib.Math.PI / 180.0f);
        cr.line_to (x, y + radius);
        cr.arc (x + radius,
                y + radius,
                radius,
                180.0f * GLib.Math.PI / 180.0f,
                270.0f * GLib.Math.PI / 180.0f);
      }

      private bool
      _on_expose (Gtk.Widget      widget,
                  Gdk.EventExpose event)
      {
        float width  = (float) widget.allocation.width;
        float height = (float) widget.allocation.height;
        float border = 1.0f;

        var cr = Gdk.cairo_create (widget.window);
        cr.scale (1.0f, 1.0f);
        cr.set_operator (Cairo.Operator.CLEAR);
        cr.paint ();

        cr.set_operator (Cairo.Operator.OVER);
        cr.set_source_rgba (0.5f, 0.5f, 0.5f, 0.9f);
        _draw_round_rect (ref cr,
                          width/height,
                          border, border,
                          20.0f,
                          width - 2.0f * border, height - 2.0f * border);
        cr.fill_preserve ();
        cr.set_source_rgba (1.0f, 1.0f, 1.0f, 1.0f);
	cr.set_line_width (0.5f);
        cr.stroke ();

        cr.select_font_face ("Sans",
                             Cairo.FontSlant.NORMAL,
                             Cairo.FontWeight.BOLD);
        cr.set_font_size (10.0f);
        cr.move_to (12.0f, 21.0f);
        cr.text_path (this.text);
        cr.set_source_rgba (0.0f, 0.0f, 0.0f, 0.75f);
        cr.fill ();
        cr.move_to (10.0f, 19.0f);
        cr.text_path (this.text);
        cr.set_source_rgba (1.0f, 1.0f, 1.0f, 1.0f);
        cr.fill ();

        return false;
      }

      public Gtk.Window
      create_window ()
      {
        Gtk.Window window = new Gtk.Window (Gtk.WindowType.TOPLEVEL);

        window.set_default_size (230, 30);
        window.title = "unity-tooltip";
        window.type_hint = Gdk.WindowTypeHint.TOOLTIP;
        window.set_colormap (window.get_screen ().get_rgba_colormap ());
        window.realize ();
        window.get_window().set_back_pixmap (null, false);
        window.expose_event.connect (_on_expose);
        window.app_paintable     = true;
        window.decorated         = false;
        window.skip_pager_hint   = true;
        window.skip_taskbar_hint = true;
        window.set_keep_above (true);
        window.resizable         = true;
        window.focus_on_map      = false;
        window.accept_focus      = false;
        window.opacity = 255;

        return window;
      }
    }

    private static TooltipManager  _instance = null;
    private uint                   _motion_handler_id;
    private Gee.ArrayList<Tooltip> _tooltips;
    private GLib.TimeVal           _last_timestamp;
    private Tooltip                _shown_tooltip;
    public  Gtk.Window             top_level;

    private bool
    _on_timeout ()
    {
      GLib.TimeVal current_timestamp = GLib.TimeVal ();

      current_timestamp.get_current_time ();
      if (current_timestamp.tv_sec - this._last_timestamp.tv_sec > 1 &&
          current_timestamp.tv_usec > 500000)
      {
        if (this._shown_tooltip != null)
        {
          this.show_quicklist (this._shown_tooltip);
          return false;
        }
      }

      return true;
    }

    private void init ()
    {
      this._motion_handler_id = GLib.Timeout.add (100, _on_timeout);
      this._tooltips = new Gee.ArrayList<Tooltip> ();
      this._last_timestamp = GLib.TimeVal ();
    }

    public void 
    terminate ()
    {
      stdout.printf ("tooltip-listsize: %d\n", this._tooltips.size);
      GLib.Source.remove (this._motion_handler_id);
      stdout.printf ("tooltip-listsize: %d\n", this._tooltips.size);
    }

    public Tooltip
    create (string text)
    {
      Tooltip tooltip = new Tooltip ();

      if (text == null)
        tooltip.text = "no text supplied";
      else
        tooltip.text = text;

      tooltip.window = tooltip.create_window ();

      this._tooltips.add (tooltip);

      return tooltip;
    }

    public bool
    show (Tooltip? tooltip, int x, int y)
    {
      if (tooltip == null)
        return false;

      this._motion_handler_id = GLib.Timeout.add (100, _on_timeout);

      GLib.TimeVal current_timestamp = GLib.TimeVal ();

      current_timestamp.get_current_time ();
      this._last_timestamp.tv_sec = current_timestamp.tv_sec;
      this._shown_tooltip = tooltip;
      int root_x;
      int root_y;
      this.top_level.get_position (out root_x, out root_y);
      tooltip.window.move (root_x + x, root_y + y);
      tooltip.window.show ();

      return true;
    }

    public bool
    hide (Tooltip? tooltip)
    {
      if (tooltip == null) 
        return false;

      GLib.Source.remove (this._motion_handler_id);

      if (this._shown_tooltip == tooltip)
        this._shown_tooltip = null;

      tooltip.window.hide ();

      return true;
    }

    public bool
    show_quicklist (Tooltip tooltip)
    {
      if (tooltip == null)
        return false;

      stdout.printf ("show_quicklist(): showing %s\n", tooltip.text);

      return true;
    }

    public bool
    hide_quicklist (Tooltip tooltip)
    {
      if (tooltip == null)
        return false;

      stdout.printf ("show_quicklist(): hiding %s\n", tooltip.text);

      return true;
    }
  }
}
