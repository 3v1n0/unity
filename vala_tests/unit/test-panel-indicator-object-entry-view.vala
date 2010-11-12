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
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */
using Unity;
using Unity.Testing;
using Unity.Panel.Indicators;

namespace Unity.Tests.Unit
{
  public class PanelIndicatorObjectEntryViewSuite : Object
  {
    public const string DOMAIN = "/Unit/Panel/Indicator/ObjectEntry";

    private Indicator.ObjectEntry entry;
    private Gtk.Menu  menu;
    private Gtk.Label label;
    private Gtk.Image image;

    public PanelIndicatorObjectEntryViewSuite ()
    {
      Logging.init_fatal_handler ();

      entry = new Indicator.ObjectEntry ();

      menu = new Gtk.Menu ();
      entry.menu = menu;

      label = new Gtk.Label ("Test Label");
      entry.label = label;

      image = new Gtk.Image.from_icon_name ("gtk-apply", Gtk.IconSize.MENU);
      entry.image = image;

      Test.add_data_func (DOMAIN + "/Allocation", test_allocation);
      Test.add_data_func (DOMAIN + "/LabelSync", test_label_sync);
      Test.add_data_func (DOMAIN + "/ImageSync", test_image_sync);
    }

    private void test_allocation ()
    {
      var e = new IndicatorObjectEntryView (entry);

      assert (e is IndicatorObjectEntryView);
    }

    private void test_label_sync ()
    {
      var e = new IndicatorObjectEntryView (entry);

      assert (e is IndicatorObjectEntryView);

      /* Make sure text is in sync */
      assert (e.text.text == label.label);

      /* Update the label and check again */
      label.label = "Test Label 2";
      assert (e.text.text == label.label);

      label.label = "Test Label";
    }

    private void test_image_sync ()
    {
      var e = new IndicatorObjectEntryView (entry);
      assert (e is IndicatorObjectEntryView);

      /* Test that the icon name has synced */
      assert (e.image.stock_id == image.icon_name);

      /* Test that if it changes entry keeps it in sync */
      image.icon_name = "gtk-close";
      assert (e.image.stock_id == image.icon_name);

      /* Test pixbuf */
      var p = new Gdk.Pixbuf (Gdk.Colorspace.RGB, true, 8, 100, 100);
      image.pixbuf = p;
      assert (e.image.pixbuf == image.pixbuf);

      image.icon_name = "gtk-apply";
    }
  }
}
