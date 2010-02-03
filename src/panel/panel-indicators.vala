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

    public View ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL,
              spacing:12,
              homogeneous:false,
              reactive:true);
    }

    construct
    {
      this.indicator_order = new HashMap<string, int> ();

      this.button_press_event.connect (this.on_button_press_event);
      this.button_release_event.connect (this.on_button_release_event);
      this.motion_event.connect (this.on_motion_event);

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

      x -= item.x;
      list = item.get_children ();
      for (int i = 0; i < list.length (); i++)
        {
          unowned IndicatorEntry ent = (IndicatorEntry)list.nth_data (i);

          if (ent.x < x && x < (ent.x + ent.width))
            {
              entry = ent;
            }
        }

      return entry;
    }

    private bool on_button_press_event (Clutter.Event e)
    {
      debug ("button press event");

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
          this.popped = entry.menu;
        }
      return true;
    }

    private bool on_button_release_event (Clutter.Event e)
    {
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

          if (entry.menu != this.popped)
            {
              this.popped.popdown ();
              this.popped = entry.menu;
              this.popped.popup (null,
                                 null,
                                 this.position_menu,
                                 e.button.button,
                                 e.button.time);
            }
        }

      return true;
    }
  }

  public class IndicatorItem : Ctk.Box
  {
    /**
     * Represents one Indicator.Object
     **/
    private Indicator.Object object;
    public  int              position;

    public IndicatorItem ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL,
              spacing:12,
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

    private Ctk.Image image;
    private Ctk.Text  text;

    public Gtk.Menu menu { get { return this.entry.menu; } }

    public IndicatorEntry (Indicator.ObjectEntry entry)
    {
      Object (entry:entry,
              orientation:Ctk.Orientation.HORIZONTAL,
              spacing:2,
              reactive:false);
    }

    construct
    {
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

      this.button_release_event.connect ((e) =>
        {
          Gtk.WidgetFlags flags = this.entry.menu.get_flags ();
          bool visible = (flags & Gtk.WidgetFlags.VISIBLE) != 0;

          if (visible)
            {
            this.entry.menu.popdown ();
            }
          else
            {
            /*
              this.entry.menu.popup (null,
                                     null,
                                     this.position_menu,
                                     e.button.button,
                                     e.button.time);*/
            }

          return true;
        });
    }
  }
}
