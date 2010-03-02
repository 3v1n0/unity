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

namespace Unity.Panel.Indicators
{
  public class View : Ctk.Box
  {
    private HashMap<string, int> indicator_order;

    private Gtk.Menu? popped;
    private uint32 click_time;
    private float last_found_entry_x = 0.0f;

    public View ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL,
              spacing:0,
              homogeneous:false,
              reactive:true);
    }

    construct
    {
      this.indicator_order = new HashMap<string, int> ();

      this.button_press_event.connect (this.on_button_press_event);
      this.button_release_event.connect (this.on_button_release_event);
      this.motion_event.connect (this.on_motion_event);

      if (Environment.get_variable ("UNITY_DISABLE_IDLES") != null)
        this.load_indicators ();
      else
        Idle.add (this.load_indicators);
    }

    private bool load_indicators ()
    {
      /* Create the order */
      this.indicator_order.set ("libapplication.so", 1);
      this.indicator_order.set ("libmessaging.so", 2);
      this.indicator_order.set ("libdatetime.so", 3);
      this.indicator_order.set ("libme.so", 4);
      this.indicator_order.set ("libsession.so", 5);

      /* We need to look for icons in an specific location */
      Gtk.IconTheme.get_default ().append_search_path (INDICATORICONSDIR);

      /* Start loading 'em in */
      var dir = File.new_for_path (INDICATORDIR);
      try
        {
          var e = dir.enumerate_children (FILE_ATTRIBUTE_STANDARD_NAME,
                                          0,
                                          null);

          FileInfo file_info;
          while ((file_info = e.next_file (null)) != null)
            {
              string leaf = file_info.get_name ();

              if (leaf[leaf.len()-2:leaf.len()] == "so")
                this.load_indicator (INDICATORDIR + file_info.get_name (),
                                     file_info.get_name ());
            }
        }
      catch (Error error)
        {
          print ("Unable to read indicators: %s\n", error.message);        }

      this.sort_children ((CompareFunc)reorder_icons);

      return false;
    }

    public static int reorder_icons (IndicatorItem a, IndicatorItem b)
    {
      return a.position - b.position;
    }

    private void load_indicator (string filename, string leaf)
    {
      Indicator.Object o;

      o = new Indicator.Object.from_file (filename);

      if (o is Indicator.Object)
        {
          var i = new IndicatorItem ();
          i.set_object (o);
          this.add_actor (i);
          i.show ();

          i.menu_moved.connect (this.on_menu_moved);

          i.position = (int)this.indicator_order[leaf];
        }
      else
        {
          warning ("Unable to load %s\n", filename);
        }
    }

    private void position_menu (Gtk.Menu menu,
                                out int  x,
                                out int  y,
                                out bool push_in)
    {
      y = (int)this.height;
      x = (int)this.last_found_entry_x;
    }

    private unowned IndicatorEntry? entry_for_event (float root_x)
    {
      unowned IndicatorItem?  item = null;
      unowned IndicatorEntry? entry = null;
      float x = root_x - this.x;

      /* Find which entry has the button press, pop it up */
      unowned GLib.List list = this.get_children ();
      for (int i = 0; i < list.length (); i++)
        {
          unowned IndicatorItem it = (IndicatorItem)list.nth_data (i);

          if (it.x < x && x < (it.x + it.width))
            {
              item = it;
              break;
            }
        }

      if (item == null)
        return null;

      x -= item.x;
      list = item.get_children ();
      for (int i = 0; i < list.length (); i++)
        {
          unowned IndicatorEntry ent = (IndicatorEntry)list.nth_data (i);

          if (ent.x < x && x < (ent.x + ent.width))
            {
              this.last_found_entry_x = ent.x + item.x + this.x;
              entry = ent;
            }
        }

      return entry;
    }

    public void show_entry (IndicatorEntry entry)
    {
      if (this.popped is Gtk.Menu
          && (this.popped.get_flags () & Gtk.WidgetFlags.VISIBLE) !=0)
        {
          this.popped.popdown ();
          this.popped = null;
        }

      this.last_found_entry_x = entry.x + entry.get_parent ().x + this.x;

      entry.menu.popup (null,
                        null,
                        this.position_menu,
                        1,
                        Clutter.get_current_event_time ());
      click_time = Clutter.get_current_event_time ();
      this.popped = entry.menu;
      entry.menu_shown ();
    }

    private bool on_button_press_event (Clutter.Event e)
    {
      if (this.popped is Gtk.Menu
          && (this.popped.get_flags () & Gtk.WidgetFlags.VISIBLE) !=0)
        {
          this.popped.popdown ();
          this.popped = null;
        }

      unowned IndicatorEntry? entry = this.entry_for_event (e.button.x);
      if (entry is IndicatorEntry)
        {
          entry.menu.popup (null,
                            null,
                            this.position_menu,
                            e.button.button,
                            e.button.time);
          click_time = e.button.time;
          this.popped = entry.menu;
          entry.menu_shown ();
        }
      return true;
    }

    private bool on_button_release_event (Clutter.Event e)
    {
      if (e.button.time - click_time < 300)
        return true;

      if (this.popped is Gtk.Menu
          && (this.popped.get_flags () & Gtk.WidgetFlags.VISIBLE) !=0)
        {
          this.popped.popdown ();
          this.popped = null;
        }

      return true;
    }

    private bool on_motion_event (Clutter.Event e)
    {
      if (this.popped is Gtk.Menu
          && (this.popped.get_flags () & Gtk.WidgetFlags.VISIBLE) !=0)
        {
          unowned IndicatorEntry? entry;

          entry = entry_for_event (e.motion.x);

          if (entry == null)
            return true;

          if (entry.menu != this.popped)
            {
              this.popped.popdown ();
              this.popped = entry.menu;
              this.popped.popup (null,
                                 null,
                                 this.position_menu,
                                 e.button.button,
                                 e.button.time);
              entry.menu_shown ();
            }
        }

      return true;
    }

    private void on_menu_moved (IndicatorItem         item,
                                Gtk.MenuDirectionType type)
    {
      unowned GLib.List list = this.get_children ();

      int pos = list.index (item);

      if (pos == -1)
          return;

      /* For us, PARENT = previous menu, CHILD = next menu */
      if (type == Gtk.MenuDirectionType.PARENT)
        {
          if (pos == 0)
              return;
          pos -= 1;
        }
      else if (type == Gtk.MenuDirectionType.CHILD)
        {
          if (pos == list.length ()-1)
              return;
          pos +=1;
        }

      /* Get the prev/next item */
      IndicatorItem new_item = list.nth_data (pos) as IndicatorItem;

      /* Find the right entry to activate */
      unowned GLib.List l = new_item.get_children ();
      int p = type == Gtk.MenuDirectionType.PARENT ? (int)l.length ()-1 : 0;
      IndicatorEntry? new_entry = l.nth_data (p) as IndicatorEntry;

      if (new_entry is IndicatorEntry)
        this.show_entry (new_entry);
    }
  }

  public class IndicatorItem : Ctk.Box
  {
    /**
     * Represents one Indicator.Object
     **/
    private Indicator.Object object;
    public  int              position;

    public signal void menu_moved (Gtk.MenuDirectionType type);

    public IndicatorItem ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL,
              spacing:0,
              homogeneous:false);
    }

    construct
    {
    }

    private void remove_entry (Indicator.ObjectEntry entry)
    {
      unowned GLib.List list = this.get_children ();

      for (int i = 0; i < list.length (); i++)
        {
          IndicatorEntry e;

          e = (IndicatorEntry) list.nth_data (i);

          if (e.entry == entry)
            {
              this.remove_actor (e);
            }
        }
    }

    private void create_entry (Indicator.ObjectEntry entry)
    {
      IndicatorEntry e = new IndicatorEntry (entry);
      this.add_actor (e);
      this.show ();

      e.menu_moved.connect (this.on_menu_moved);
    }

    private void on_menu_moved (IndicatorEntry        entry,
                                Gtk.MenuDirectionType type)
    {
      unowned GLib.List list = this.get_children ();

      int pos = list.index (entry);

      if (pos == -1)
        {
          this.menu_moved (type);
          return;
        }

      /* For us, PARENT = previous menu, CHILD = next menu */
      if (type == Gtk.MenuDirectionType.PARENT)
        {
          if (pos == 0)
            {
              this.menu_moved (type);
              return;
            }
          pos -= 1;
        }
      else if (type == Gtk.MenuDirectionType.CHILD)
        {
          if (pos == list.length ()-1)
            {
              this.menu_moved (type);
              return;
            }
          pos +=1;
        }

      IndicatorEntry new_entry = list.nth_data (pos) as IndicatorEntry;

      Indicators.View parent = this.get_parent () as Indicators.View;
      parent.show_entry (new_entry);
    }

    public void set_object (Indicator.Object object)
    {
      this.object = object;

      object.entry_added.connect (this.create_entry);
      object.entry_removed.connect (this.remove_entry);

      unowned GLib.List list = object.get_entries ();

      for (int i = 0; i < list.length (); i++)
        {
          unowned Indicator.ObjectEntry entry;

          entry = (Indicator.ObjectEntry) list.nth_data (i);

          this.create_entry (entry);
        }
    }

    public Indicator.Object get_object ()
    {
      return this.object;
    }
  }

  public class IndicatorEntry : Ctk.Box
  {
    public unowned Indicator.ObjectEntry entry { get; construct; }

    private Clutter.CairoTexture bg;

    private Ctk.Image     image;
    private Ctk.Text      text;

    public Gtk.Menu menu { get { return this.entry.menu; } }

    public signal void menu_moved (Gtk.MenuDirectionType type);

    public IndicatorEntry (Indicator.ObjectEntry entry)
    {
      Object (entry:entry,
              orientation:Ctk.Orientation.HORIZONTAL,
              spacing:4,
              reactive:false);
    }

    construct
    {
      this.padding = { 0, 6.0f, 0, 6.0f };

      this.bg = new Clutter.CairoTexture (10, 10);
      this.bg.set_parent (this);
      this.bg.opacity = 0;
      this.bg.show ();

      if (this.entry.image is Gtk.Image)
        {
          this.image = new Ctk.Image (22);
          this.add_actor (this.image);
          this.image.show ();

          this.image.stock_id = this.entry.image.icon_name;

          this.entry.image.notify["icon-name"].connect (() =>
            {
              this.image.stock_id = this.entry.image.icon_name;
            });

          unowned Gtk.IconTheme theme = Gtk.IconTheme.get_default ();
          theme.changed.connect (() =>
            {
              this.image.stock_id = this.entry.image.icon_name;
            });
        }

      if (this.entry.label is Gtk.Label)
        {
          this.text = new Ctk.Text ("");
          this.add_actor (this.text);
          this.text.show ();

          this.text.text = this.entry.label.label;

          this.entry.label.notify["label"].connect (() =>
            {
              this.text.text = this.entry.label.label;
            });
        }
    }

    public void menu_shown ()
    {
      if (this.entry.menu is Gtk.Menu)
        {
          this.entry.menu.move_current.connect (this.menu_key_moved);
          this.entry.menu.notify["visible"].connect (this.menu_vis_changed);
          this.bg.animate (Clutter.AnimationMode.EASE_OUT_QUAD, 200,
                           "opacity", 255);
        }
    }

    public void menu_vis_changed ()
    {
      bool vis = (this.entry.menu.get_flags () & Gtk.WidgetFlags.VISIBLE) != 0;

      if (vis == false)
        {
          this.bg.animate (Clutter.AnimationMode.EASE_OUT_QUAD, 200,
                           "opacity", 0);

          this.entry.menu.move_current.disconnect (this.menu_key_moved);
          this.entry.menu.notify["visible"].disconnect (this.menu_vis_changed);
        }
    }

    public void menu_key_moved (Gtk.MenuDirectionType type)
    {
      if (type != Gtk.MenuDirectionType.PARENT &&
          type != Gtk.MenuDirectionType.CHILD)
        return;

      this.menu_moved (type);
    }

    private void update_bg (int width, int height)
    {
      Cairo.Context cr;

      this.bg.set_surface_size (width, height);

      cr = this.bg.create ();

      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      cr.set_operator (Cairo.Operator.OVER);

      cr.set_line_width (1.0);

      cr.set_source_rgba (1.0, 1.0, 1.0, 0.2);

      cr.move_to (1, height);

      cr.line_to (1, 7);

      cr.curve_to (1, 2,
                   1, 2,
                   10, 2);

      cr.line_to (width-10, 2);

      cr.curve_to (width, 2,
                   width, 2,
                   width, 7);

      cr.line_to (width, height);

      cr.line_to (1, height);

      var pat = new Cairo.Pattern.linear (1, 0, 1, height);
      pat.add_color_stop_rgba (0.0, 1.0f, 1.0f, 1.0f, 0.6f);
      pat.add_color_stop_rgba (1.0, 1.0f, 1.0f, 1.0f, 0.2f);

      cr.set_source (pat);
      cr.fill ();
    }

    /*
     * CLUTTER OVERRIDES
     */
    private override void allocate (Clutter.ActorBox        box,
                                    Clutter.AllocationFlags flags)
    {
      float width;
      float height;

      base.allocate (box, flags);

      width = Math.floorf (box.x2 - box.x1);
      height = Math.floorf (box.y2 - box.y1) - 1;

      Clutter.ActorBox child_box = { 0 };
      child_box.x1 = 0;
      child_box.x2 = width;
      child_box.y1 = 0;
      child_box.y2 = height;

      if (width != this.bg.width ||
          height != this.bg.height)
        {
          this.update_bg ((int)width, (int)height);
        }

      this.bg.allocate (child_box, flags);
    }

    private override void paint ()
    {
      this.bg.paint ();
      base.paint ();
    }

    private override void map ()
    {
      base.map ();
      this.bg.map ();
    }

    private override void unmap ()
    {
      base.unmap ();
      this.bg.unmap ();
    }
  }
}
