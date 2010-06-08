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
 * Authored by Neil Jay Taoko <jay.taoko.patel@canonical.com>
 *
 */
using Unity;
using Unity.Testing;
using Unity.Panel.Indicators;

namespace Unity.Tests.Unit
{
  public class FakeIndicatorObject: Indicator.Object
  {
    public Gee.ArrayList<Indicator.ObjectEntry> indicator_entry_array;
    public Gtk.Label _label;
    public Gtk.Image _image;
    public Gtk.Menu _menu;

    public FakeIndicatorObject ()
    {
      Object();
    }

    construct
    {
      START_FUNCTION ();
      indicator_entry_array = new Gee.ArrayList< unowned Indicator.ObjectEntry> ();
      
      _label = new Gtk.Label ("Test Label");
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

//     public override GLib.List<unowned Indicator.ObjectEntry> get_entries ()
//     {
//       GLib.List<unowned Indicator.ObjectEntry> list = new GLib.List<unowned Indicator.ObjectEntry> ();
//       foreach(unowned Indicator.ObjectEntry element in indicator_entry_array)
//       {
//         list.append (element);
//       }
//       return list;
//     }
  }

  public class PanelIndicatorObjectViewSuite : Object
  {
    public const string DOMAIN = "/Unit/Panel/Indicator/ObjectView";

    private Indicator.ObjectEntry entry;
    private Gtk.Menu  menu;
    private Gtk.Label label;
    private Gtk.Image image;
    
    public PanelIndicatorObjectViewSuite ()
    {
      Logging.init_fatal_handler ();

      entry = new Indicator.ObjectEntry ();

      menu = new Gtk.Menu ();
      entry.menu = menu;

      label = new Gtk.Label ("Test Label");
      entry.label = label;

      image = new Gtk.Image.from_icon_name ("gtk-apply", Gtk.IconSize.MENU);
      entry.image = image;

      Test.add_data_func (DOMAIN + "/FakeIndicator", test_fake_indicator_object);
      Test.add_data_func (DOMAIN + "/FakeIndicatorAddEntry", test_indicator_add_entry);
      Test.add_data_func (DOMAIN + "/TestEntryView", test_indicator_enty_view);
    }

    private void test_fake_indicator_object ()
    {
      var fakeobject = new FakeIndicatorObject ();
      
      assert (fakeobject is FakeIndicatorObject);
    }

    private void test_indicator_add_entry ()
    {
      var fakeobject = new FakeIndicatorObject ();
      var e = new IndicatorObjectView (fakeobject);

      fakeobject.add_entry (entry);
     
      assert (e.find_entry (entry));
    }

    private void test_indicator_enty_view ()
    {
      var fakeobject = new FakeIndicatorObject ();
      var e = new IndicatorObjectView (fakeobject);

      fakeobject.add_entry (entry);
      IndicatorObjectEntryView? entry_view = e.get_entry_view (entry);
      
      assert (entry_view.entry == entry);
      assert (entry_view.entry.label == entry.label);
      assert (entry_view.entry.image == entry.image);
      assert (entry_view.entry.menu == entry.menu);
    }
  }
}
