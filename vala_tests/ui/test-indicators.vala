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
    //GLib.List<Indicator.ObjectEntry> list;
    
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
    
//     public override unowned GLib.List<Indicator.ObjectEntry> get_entries ()
//     {
//       list = new GLib.List<Indicator.ObjectEntry> ();
//       foreach(Indicator.ObjectEntry element in indicator_entry_array)
//       {
//         list.append (element);
//       }
//       return list;
//     }    
  }
  
  public class FakeIndicatorsModel : IndicatorsModel
  {
    public Gee.HashMap<Indicator.Object, string> indicator_map;
    public Gee.ArrayList<Indicator.Object> indicator_list;
    
    
    public FakeIndicatorObject indicator_object_0;
    public FakeIndicatorObject indicator_object_1;
    
    public FakeIndicatorsModel ()
    {
      START_FUNCTION ();

      indicator_map = new Gee.HashMap<Indicator.Object, string> ();
      indicator_list = new Gee.ArrayList<Indicator.Object> ();

      load_fake_indicators ();
      
      END_FUNCTION ();
    }

    private void load_fake_indicators ()
    {
      indicator_object_0 = new FakeIndicatorObject (0);
      indicator_object_1 = new FakeIndicatorObject (1);

      this.indicator_map[indicator_object_0] = "fakeindicator0";
      indicator_list.add (indicator_object_0);
      this.indicator_map[indicator_object_1] = "fakeindicator1";
      indicator_list.add (indicator_object_1);
          
    }

    public override Gee.ArrayList<Indicator.Object> get_indicators ()
    {
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

    public Indicator.ObjectEntry entry0;
    public Indicator.ObjectEntry entry1;

    private Gtk.Menu  menu0;
    private Gtk.Label label0;
    private Gtk.Image image0;
    
    private Gtk.Menu  menu1;
    private Gtk.Label label1;
    private Gtk.Image image1;
    
    private Gtk.MenuItem item;
    
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
    
      FakeIndicatorsModel indicator_model = IndicatorsModel.get_default () as FakeIndicatorsModel;
      
      menu0 = new Gtk.Menu ();
      label0 = new Gtk.Label ("Entry0");
      image0 = new Gtk.Image.from_icon_name ("gtk-apply", Gtk.IconSize.MENU);
      item = new Gtk.MenuItem.with_label ("MenuItem0");
      item.show();
      menu0.append (item);
      item = new Gtk.MenuItem.with_label ("MenuItem1");
      item.show();
      menu0.append (item);
      item = new Gtk.MenuItem.with_label ("MenuItem2");
      item.show();
      menu0.append (item);
      item = new Gtk.MenuItem.with_label ("MenuItem3");
      item.show();
      menu0.append (item);
      
      entry0 = new Indicator.ObjectEntry ();
      entry0.menu = menu0;
      entry0.label = label0;
      entry0.image = image0;
      indicator_model.indicator_object_0.add_entry (entry0);

      menu1 = new Gtk.Menu ();
      label1 = new Gtk.Label ("Entry1");
      image1 = new Gtk.Image.from_icon_name ("gtk-apply", Gtk.IconSize.MENU);
      item = new Gtk.MenuItem.with_label ("MenuItem0");
      item.show();
      menu1.append (item);
      item = new Gtk.MenuItem.with_label ("MenuItem1");
      item.show();
      menu1.append (item);
      item = new Gtk.MenuItem.with_label ("MenuItem2");
      item.show();
      menu1.append (item);
      item = new Gtk.MenuItem.with_label ("MenuItem3");
      item.show();
      menu1.append (item);
      
      entry1 =  new Indicator.ObjectEntry ();
      entry1.menu = menu1;
      entry1.label = label1;
      entry1.image = image1;
      indicator_model.indicator_object_1.add_entry (entry1);
      
      Test.add_data_func (DOMAIN + "/IndicatorMouseEvent", test_indicators_mouse_event);
      Test.add_data_func (DOMAIN + "/IndicatorKeyboardEvent", test_indicators_keyboard_event);
    }


    private void test_indicators_mouse_event ()
    {
      ObjectRegistry registry = ObjectRegistry.get_default ();
      Logging.init_fatal_handler ();
      
      FakeIndicatorsModel indicator_model = IndicatorsModel.get_default () as FakeIndicatorsModel;
      
      IndicatorBar indicator_bar = registry.lookup ("IndicatorBar").get (0) as IndicatorBar;
      assert (indicator_bar != null);

      IndicatorObjectView indicator_object_view0 = indicator_bar.get_indicator_view_matching (indicator_model.indicator_object_0);
      assert (indicator_object_view0 != null);
      IndicatorObjectView indicator_object_view1 = indicator_bar.get_indicator_view_matching (indicator_model.indicator_object_1);
      assert (indicator_object_view1 != null);

      /* Hack to remove the default entries: because get_entries has not been overriden in FakeIndicatorObject */
      indicator_object_view0.remove_first_entry ();
      indicator_object_view1.remove_first_entry ();
      
      indicator_bar.do_queue_redraw ();

      IndicatorObjectEntryView object_entry0 = indicator_object_view0.get_entry_view (entry0);
      assert (object_entry0 != null);
      IndicatorObjectEntryView object_entry1 = indicator_object_view1.get_entry_view (entry1);
      assert (object_entry1 != null);
      
      // Test button press on first indicator
      director.button_press (object_entry0, 1, true, 1.0f, 1.0f, false);
      assert (object_entry0.is_open () == true);
      assert (object_entry1.is_open () == false);
      director.do_wait_for_timeout (1000);

      // Test button press on second indicator      
      director.button_press (object_entry1, 1, true, 1.0f, 1.0f, false);
      assert (object_entry0.is_open () == false);
      assert (object_entry1.is_open () == true);
      director.do_wait_for_timeout (1000);
      
      // Press to close
      director.button_press (object_entry1, 1, true, 1.0f, 1.0f, false);
      assert (object_entry0.is_open () == false);
      assert (object_entry1.is_open () == false);
      
    }

    private void test_indicators_keyboard_event ()
    {
      ObjectRegistry registry = ObjectRegistry.get_default ();
      Logging.init_fatal_handler ();
      
      FakeIndicatorsModel indicator_model = IndicatorsModel.get_default () as FakeIndicatorsModel;
      
      IndicatorBar indicator_bar = registry.lookup ("IndicatorBar").get (0) as IndicatorBar;
      assert (indicator_bar != null);

      IndicatorObjectView indicator_object_view0 = indicator_bar.get_indicator_view_matching (indicator_model.indicator_object_0);
      assert (indicator_object_view0 != null);
      IndicatorObjectView indicator_object_view1 = indicator_bar.get_indicator_view_matching (indicator_model.indicator_object_1);
      assert (indicator_object_view1 != null);
      
      IndicatorObjectEntryView object_entry0 = indicator_object_view0.get_entry_view (entry0);
      assert (object_entry0 != null);
      IndicatorObjectEntryView object_entry1 = indicator_object_view1.get_entry_view (entry1);
      assert (object_entry1 != null);      
      
      //Simulate key navigation
      director.do_wait_for_timeout (1000);
      director.button_press (object_entry0, 1, true, 1.0f, 1.0f, false);
      assert (object_entry0.is_open () == true);
      assert (object_entry1.is_open () == false);
      director.do_wait_for_timeout (1000);
      
      object_entry0.entry.menu.move_current (Gtk.MenuDirectionType.CHILD);
      assert (object_entry0.is_open () == false);
      assert (object_entry1.is_open () == true);
      director.do_wait_for_timeout (1000);
      object_entry1.entry.menu.move_current (Gtk.MenuDirectionType.PARENT);
      assert (object_entry0.is_open () == true);
      assert (object_entry1.is_open () == false);
      director.do_wait_for_timeout (1000);
      
    }
  }
}

