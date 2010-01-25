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

namespace Unity.Panel.Indicators
{
  public class View : Ctk.Box
  {
    public View ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL,
              spacing:12,
              homogeneous:false);
    }

    construct
    {
      Idle.add (this.load_indicators);
    }

    private bool load_indicators ()
    {
      Gtk.IconTheme.get_default ().append_search_path (INDICATORICONSDIR);

      print ("Reading indicators from: %s\n", INDICATORDIR);

      var dir = File.new_for_path (INDICATORDIR);

      try
        {
          var e = dir.enumerate_children (FILE_ATTRIBUTE_STANDARD_NAME,
                                          0,
                                          null);

          FileInfo file_info;
          while ((file_info = e.next_file (null)) != null)
            {
              this.load_indicator (INDICATORDIR + file_info.get_name ());
            }
        }
      catch (Error error)
        {
          print ("Unable to read indicators: %s\n", error.message);        }

      return false;
    }

    private void load_indicator (string filename)
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

    public IndicatorItem ()
    {
      Object (orientation: Ctk.Orientation.HORIZONTAL,
              spacing:12,
              homogeneous:false);
    }

    construct
    {
    }

    public void set_object (Indicator.Object object)
    {
      this.object = object;

      unowned List list = object.get_entries ();

      for (int i = 0; i < list.length (); i++)
        {
          unowned Indicator.ObjectEntry entry;

          entry = (Indicator.ObjectEntry) list.nth_data (i);

          IndicatorEntry e = new IndicatorEntry (entry);
          this.add_actor (e);
          this.show ();
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
        this.image.stock_id = this.entry.image.icon_name;

      if (this.entry.label is Gtk.Label)
        this.text.text = this.entry.label.label;

      this.button_release_event.connect ((e) =>
        {
          this.entry.menu.popup (null,
                                 null,
                                 null,
                                 e.button.button,
                                 e.button.time);

          return true;
        });
    }
  }
}
