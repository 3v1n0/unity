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
  static const string SEARCH_TEMPLATE = "xdg-open http://search.yahoo.com/search?p=%s&fr=ubuntu&ei=UTF-8";
  static const string SEARCH_HINT = "Yahoo!";

  public bool? search_entry_has_focus = null;

  public class View : Ctk.Actor
  {
    public Shell shell { get; construct;}

    private ThemeImage        rect;
    private Tray.View         tray;
    private HomeButton        home;
    private Indicators.View   indicators;

    private ThemeImage entry_icon;
    private Entry      entry;
    private Unity.Places.CairoDrawing.EntryBackground entry_background;

    private int indicators_width = 0;

    private GConf.Client client;

    private bool places_enabled = false;

    public View (Shell shell)
    {
      Object (shell:shell, reactive:false);
      this.tray.manage_stage (this.shell.get_stage ());

      Unity.Testing.ObjectRegistry.get_default ().register ("PanelView", this);
    }

    construct
    {
      START_FUNCTION ();

      this.places_enabled = Environment.get_variable ("UNITY_ENABLE_PLACES") != null;

      this.rect = new ThemeImage ("panel_background");
      this.rect.set_repeat (true, false);
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
      this.home.clicked.connect (this.on_home_clicked);

      if (this.places_enabled == false)
        {
          this.entry_background = new Unity.Places.CairoDrawing.EntryBackground ();
          this.entry_background.set_parent (this);
          this.entry_background.show ();

          this.entry_icon = new ThemeImage ("search_field");
          this.entry_icon.reactive = true;
          this.entry_icon.button_press_event.connect ((e) =>
            {
               this.entry.do_event (e, false);
               this.entry.grab_key_focus ();
               Unity.global_shell.grab_keyboard (true, e.button.time);
            });
          this.entry_icon.set_parent (this);
          this.entry_icon.show ();

          this.entry = new Unity.Entry ("");
          this.entry.static_text = this.get_search_hint ();
          this.entry.set_parent (this);
          this.entry.show ();
          this.entry.activate.connect (this.on_entry_activated);

          this.entry.key_focus_in.connect (this.on_entry_focus_in);
          this.entry.key_focus_out.connect (this.on_entry_focus_out);
          Unity.Panel.search_entry_has_focus = false;
        }

      END_FUNCTION ();
    }

    private void on_home_clicked ()
    {
      Unity.global_shell.show_window_picker ();
    }

    private void on_entry_focus_in ()
    {
      Unity.Panel.search_entry_has_focus = true;
    }

    private void on_entry_focus_out ()
    {
      Unity.Panel.search_entry_has_focus = false;
    }

    private string get_search_hint ()
    {
      string hint = "";
      this.client = GConf.Client.get_default ();

      try
        {
          hint = client.get_string ("/desktop/unity/panel/search_hint");

          try
            {
              client.add_dir ("/desktop/unity/panel",
                              GConf.ClientPreloadType.NONE);
              client.notify_add ("/desktop/unity/panel/search_hint",
                                 this.on_search_hint_changed);
            }
          catch (Error e)
            {
              warning ("Unable to monitor gconf for search hint changes: %s",
                       e.message);
            }

          if (hint == "" || hint == null)
            hint = SEARCH_HINT;
        }
      catch (Error e)
        {
          hint = SEARCH_HINT;
        }

      return hint;
    }

    private void on_search_hint_changed ()
    {
      try
        {
          var hint = client.get_string ("/desktop/unity/panel/search_hint");

          this.entry.static_text = hint;
        }
      catch (Error e)
        {
          warning ("Unable to get search hint: %s", e.message);
        }
    }

    private void on_entry_activated ()
    {
      string template = "";

      var client = GConf.Client.get_default ();
      try
        {
          template = client.get_string ("/desktop/unity/panel/search_template");

          if (template == "" || template == null)
            template = SEARCH_TEMPLATE;
        }
      catch (Error e)
        {
          template = SEARCH_TEMPLATE;
        }

      var command=template.replace ("%s",
                                Uri.escape_string (this.entry.text, "", true));

      try
        {
          Process.spawn_command_line_async (command);
        }
      catch (Error e)
        {
          warning ("Unable to search for '%s': %s",
                   this.entry.text,
                   e.message);
        }
    }

    private override void allocate (Clutter.ActorBox        box,
                                    Clutter.AllocationFlags flags)
    {
      Clutter.ActorBox child_box = { 0, 0, box.x2 - box.x1, box.y2 - box.y1 };
      float            width;
      float            child_width;
      float            i_width;

      base.allocate (box, flags);

      width = box.x2 - box.x1;

      this.rect.set_clip (0, 0, width, box.y2 - box.y1);

      /* First the background */
      child_box.y2 += 3.0f;
      this.rect.allocate (child_box, flags);

      /* Home button */
      child_box.x1 = 0;
      child_box.x2 = 60;
      child_box.y1 = 0;
      child_box.y2 = PANEL_HEIGHT;
      this.home.allocate (child_box, flags);

      if (this.places_enabled == false)
        {
          /* Entry */
          child_box.x1 = Math.floorf (child_box.x2 + 12);
          child_box.x2 = Math.floorf (child_box.x1 + 170); /* Random width */
          child_box.y1 = Math.floorf (2);
          child_box.y2 = Math.floorf (PANEL_HEIGHT);

          this.entry_background.allocate (child_box, flags);
          if ((this.entry_background.Width != (int)(child_box.x2 - child_box.x1)) && (this.entry_background.height != (int)(child_box.y2 - child_box.y1-2)))
          {
            this.entry_background.create_search_entry_background ((int)(child_box.x2 - child_box.x1), (int)(child_box.y2 - child_box.y1-2));
          }

          child_box.x1 += 6;
          child_box.x2 = child_box.x1 + 16;
          child_box.y1 = Math.floorf ((PANEL_HEIGHT-16)/2.0f);
          child_box.y2 = child_box.y1 + 16;
          this.entry_icon.allocate (child_box, flags);

          child_box.x1 = child_box.x2 + 4; /* (QL_width - logo_width)/2.0 */
          child_box.x2 = child_box.x1 + 150 - 16;
          child_box.y1 = 9;
          child_box.y2 = 15;
          this.entry.allocate (child_box, flags);

          child_box.y1 = 0;
          child_box.y2 = PANEL_HEIGHT;
        }

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

      if (this.places_enabled == true)
        return;

      this.entry_background.paint ();
      this.entry_icon.paint ();
      this.entry.paint ();
    }

    private override void pick (Clutter.Color color)
    {
      base.pick (color);
      this.tray.paint ();
      this.home.paint ();
      this.indicators.paint ();

      if (this.places_enabled == true)
        return;

      this.entry_background.paint ();
      this.entry_icon.paint ();
      this.entry.paint ();
    }

    private override void map ()
    {
      base.map ();
      this.rect.map ();
      this.tray.map ();
      this.home.map ();
      this.indicators.map ();

      if (this.places_enabled == true)
        return;

      this.entry_background.map ();
      this.entry_icon.map ();
      this.entry.map ();
    }

    private override void unmap ()
    {
      base.unmap ();
      this.rect.unmap ();
      this.tray.unmap ();
      this.home.unmap ();
      this.indicators.unmap ();

      if (this.places_enabled == true)
        return;

      this.entry_background.unmap ();
      this.entry_icon.unmap ();
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
