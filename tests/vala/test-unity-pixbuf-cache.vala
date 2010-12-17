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
using Unity;
using Unity.Testing;
using Gdk;
using GLib.Test;

namespace Unity.Tests.Unit
{
  public class UnityPixbufCacheSuite : Object
  {
    public const string DOMAIN = "/Unit/Unity/PixbufCache";

    public UnityPixbufCacheSuite()
    {
      Logging.init_fatal_handler ();

      Test.add_data_func (DOMAIN + "/Allocation", test_allocation);
      Test.add_data_func (DOMAIN + "/Lookup", test_lookup);
      Test.add_data_func (DOMAIN + "/Setting", test_setting_async);
    }

    private void test_allocation ()
    {
      var cache = PixbufCache.get_default ();
      assert (cache is PixbufCache);
      assert (cache == PixbufCache.get_default ());
    }

    private void test_lookup ()
    {
      var pixbuf = new Pixbuf (Colorspace.RGB, true, 8, 69, 69);
      var cache = PixbufCache.get_default ();

      assert (cache is PixbufCache);
      assert (cache.size == 0);

      cache.set ("foo", pixbuf, 48);
      assert (cache.size == 1);
      assert (cache.get ("foo", 48) == pixbuf);
      assert (cache.get ("foo", 24) == null);
      assert (cache.get ("bar", 48) == null);

      cache.clear ();
      assert (cache.size == 0);
      assert (cache.get ("foo", 48) == null);
      assert (cache.get ("bar", 48) == null);
    }

    private void test_setting_async ()
    {
      var image = new Ctk.Image (48);
      var pixbuf = new Pixbuf (Colorspace.RGB, true, 8, 69, 69);
      var cache = PixbufCache.get_default ();

      assert (cache is PixbufCache);
      assert (cache.size == 0);

      cache.set ("foo", pixbuf, 48);
      assert (cache.size == 1);
      assert (cache.get ("foo", 48) == pixbuf);

      assert (image.get_pixbuf () != pixbuf);

      cache.set_image_from_icon_name (image, "foo", 48);
      while (Gtk.events_pending ())
        Gtk.main_iteration ();
      assert (image.get_pixbuf () == pixbuf);


      if (trap_fork (0, TestTrapFlags.SILENCE_STDOUT | TestTrapFlags.SILENCE_STDERR))
        {
          cache.set_image_from_icon_name (image, "bar", 24);
          while (Gtk.events_pending ())
            Gtk.main_iteration ();
          Posix.exit (0);
        }
        trap_has_passed ();
        trap_assert_stderr ("*Unable to load icon_name*");
        assert (image.get_pixbuf () == pixbuf); /* No change as failed*/
    }
  }
}
