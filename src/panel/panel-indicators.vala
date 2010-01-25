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

    public View ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL,
              spacing:12,
              homogeneous:false);
    }

    construct
    {
      this.indicator_order = new HashMap<string, int> ();

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
      debug ("Reading indicators from: %s\n", INDICATORDIR);

      var dir = File.new_for_path (INDICATORDIR);
      try
        {
          var e = dir.enumerate_children (FILE_ATTRIBUTE_STANDARD_NAME,
                                          0,
                                          null);

          FileInfo file_info;
          while ((file_info = e.next_file (null)) != null)
            {
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
          print ("Successfully loaded: %s\n", filename);

          var i = new IndicatorItem ();
          i.set_object (o);
          this.add_actor (i);
          i.show ();

          i.position = (int)this.indicator_order[leaf];
          print ("%s: %d\n", leaf, i.position);
        }
      else
        {
          warning ("Unable to load %s\n", filename);
        }
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
      Object (orientation: Ctk.Orientation.HORIZONTAL,
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

    public IndicatorEntry (Indicator.ObjectEntry entry)
    {
      Object (entry:entry,
              orientation:Ctk.Orientation.HORIZONTAL,
              spacing:2,
              reactive:true);
    }

    construct
    {
      this.image = new Ctk.Image (22);
      this.add_actor (this.image);
      this.image.show ();

      this.text = new Ctk.Text ("");
      this.add_actor (this.text);
      this.text.show ();

      if (this.entry.image is Gtk.Image)
        {
          this.image.stock_id = this.entry.image.icon_name;

          this.entry.image.notify["icon-name"].connect (() =>
            {
              this.image.stock_id = this.entry.image.icon_name;
            });
        }

      if (this.entry.label is Gtk.Label)
        {
          this.text.text = this.entry.label.label;

          this.entry.label.notify["label"].connect (() =>
            {
              this.text.text = this.entry.label.label;
            });
        }

      this.button_release_event.connect ((e) =>
        {
          this.entry.menu.popup (null,
                                 null,
                                 this.position_menu,
                                 e.button.button,
                                 e.button.time);

          return true;
        });
    }

    private void position_menu (Gtk.Menu menu,
                                out int  x,
                                out int  y,
                                out bool push_in)
    {
      y = (int)this.height;
    }
  }
}
