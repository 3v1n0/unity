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
 * Authored by Jay Taoko <jay.taoko@canonical.com>
 *
 */

using Unity;
using Unity.Testing;
using Unity.Panel;
using Unity.Panel.Indicators;

namespace Unity.Tests.UI
{
  public class FakeIndicatorObject: Indicator.Object
  {
    public Gee.ArrayList<Indicator.ObjectEntry> indicator_entry_array;
    public Gtk.Label _label = null;
    public Gtk.Image _image = null;
    public Gtk.Menu _menu = null;
    public int index {get; set construct;}
    
    public FakeIndicatorObject (int i)
    {
      Object(index:i);
    }
    
    construct
    {
      START_FUNCTION ();
      string index_string = "%d".printf(index);
      indicator_entry_array = new Gee.ArrayList< unowned Indicator.ObjectEntry> ();
      
      _label = new Gtk.Label ("Indicator" + index_string);
      _image = new Gtk.Image.from_icon_name ("gtk-apply", Gtk.IconSize.MENU);
      _menu = new Gtk.Menu ();
      //_menu.append (new Gtk.MenuItem.with_label ("lala"));
      END_FUNCTION ();
    }
    
    public void add_entry(Indicator.ObjectEntry entry)
    {
      int pos = indicator_entry_array.index_of (entry);
      if (pos != -1)
        return;
      
      indicator_entry_array.add (entry);
      entry_added (entry);
    }

    public void remove_entry(Indicator.ObjectEntry entry)
    {
      int pos = indicator_entry_array.index_of (entry);
      if (pos != -1)
        {
          indicator_entry_array.remove_at (pos);
          entry_removed (entry);
        }
    }
    
    public override unowned Gtk.Label get_label ()
    {
      return this._label;
    }
    
    public override unowned Gtk.Image get_image ()
    {
      return this._image;
    }
    
    public override unowned Gtk.Menu get_menu ()
    {
      return this._menu;
    }
    
/*    public override unowned GLib.List get_entries ()
    {
      
    }*/
  }
  
  public class FakeIndicatorsModel : IndicatorsModel
  {
    //public static Gee.HashMap<string, int> indicator_order = null;
    public Gee.HashMap<Indicator.Object, string> indicator_map;
    public Gee.ArrayList<Indicator.Object> indicator_list;
    
    private Indicator.ObjectEntry entry;
    private Gtk.Menu  menu;
    private Gtk.Label label;
    private Gtk.Image image;
    private Gtk.MenuItem item;
    
    public FakeIndicatorsModel ()
    {
      START_FUNCTION ();

      indicator_map = new Gee.HashMap<Indicator.Object, string> ();
      //indicator_order = new Gee.HashMap<string, int> ();
      indicator_list = new Gee.ArrayList<Indicator.Object> ();

      load_fake_indicators ();
      
      END_FUNCTION ();
    }

    private void load_fake_indicators ()
    {
      entry =  new Indicator.ObjectEntry ();
      menu = new Gtk.Menu ();
      item = new Gtk.MenuItem.with_label ("lala");
      //menu.append (item);
      entry.menu = menu;

      label = new Gtk.Label ("Label");
      entry.label = label;

      image = new Gtk.Image.from_icon_name ("gtk-apply", Gtk.IconSize.MENU);
      entry.image = image;
      
      FakeIndicatorObject o;

      o = new FakeIndicatorObject (0);

      if (o is Indicator.Object)
        {
          o.add_entry (entry);
          this.indicator_map[o] = "fakeindicator0";
          indicator_list.add (o);
        }
        
      /*o = new FakeIndicatorObject (1);
      this.indicator_map[o] = "fakeindicator1";
      indicator_list.add (o);*/
    }

    public override Gee.ArrayList<Indicator.Object> get_indicators ()
    {
      stdout.printf ("Returning indicator list\n");
      return indicator_list;
    }

    public override string get_indicator_name (Indicator.Object o)
    {
      return indicator_map[o];
    }
  }
  
  
  public class IndicatorTestSuite : Object
  {
    private const string DOMAIN = "/UI/Indicators";

    Unity.Testing.Window?  window;
    Clutter.Stage?         stage;
    Unity.Testing.Director director;

    public IndicatorTestSuite ()
    {
      Logging.init_fatal_handler ();

      FakeIndicatorsModel fake_indicator_model = new FakeIndicatorsModel ();
      IndicatorsModel.set_default (fake_indicator_model);
      
      /* Testup the test window */
      window = new Unity.Testing.Window (true, 1024, 600);
      window.init_test_mode ();
      stage = window.stage;
      window.title = "Indicators testing";
      window.show_all ();

      director = new Unity.Testing.Director (stage);

     
      Test.add_data_func (DOMAIN + "/Indicators", test_indicators);
    }


    private void test_indicators ()
    {
      //director.button_press (home_button, 1, true, 1.0f, 1.0f, false);

      //Logging.init_fatal_handler ();

      //assert (g_flag == true);
    }

    //
    // mostly a dummy shell-implementation to satisfy the interface
    //
    public bool menus_swallow_events { get { return true; } }

    public ShellMode
    get_mode ()
    {
      return ShellMode.UNDERLAY;
    }


  }
}

