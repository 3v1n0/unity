/*
 * Copyright (C) 2010 Canonical, Ltd.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * version 3.0 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3.0 for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
  <http://www.gnu.org/licenses/>.
 *
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 */

using Gdk;
using Gee;
using GLib;

namespace Unity
{
  /* This is what we use for lookups, namely the id of the icon and it's size.
   * Currently this means we just use one HashMap for all the pixbufs, it
   * might be useful to have one HashMap per size in the future...
   */
  static const string hash_template = "%s%d";

  static PixbufCache _pixbuf_cache = null;

  public class PixbufCache : Object
  {
    /*
     * Properties
     */
    private unowned Gtk.IconTheme       theme;
    private HashMap<string, Gdk.Pixbuf> cache;

    private bool autodispose = false;

    public uint size { get { return cache.size; } }

    /*
     * Construction
     */
    public PixbufCache (bool _autodispose=false)
    {
      Object ();

      autodispose = _autodispose;
      if (autodispose)
        {
          if (global_shell is Unity.Shell)
            global_shell.weak_ref (on_shell_destroyed);
        }
    }

    construct
    {
      theme = Gtk.IconTheme.get_default ();
      cache = new HashMap<string, Gdk.Pixbuf> ();
    }

    private void on_shell_destroyed ()
    {
      if (_pixbuf_cache == this)
        {
          _pixbuf_cache = null;
        }
      else
        this.unref ();
    }

    /*
     * Public Methods
     */
    public static PixbufCache get_default ()
    {
      if (_pixbuf_cache == null)
        {
          _pixbuf_cache = new PixbufCache (true);
        }

      return _pixbuf_cache;
    }

    public new void set (string icon_id, Pixbuf pixbuf, int size)
    {
      cache[hash_template.printf (icon_id, size)] = pixbuf;
    }

    public new Pixbuf? get (string icon_id, int size)
    {
      return cache[hash_template.printf (icon_id, size)];
    }

    public void clear ()
    {
      cache.clear ();
    }

    public async void set_image_from_icon_name (Ctk.Image image,
                                                string    icon_name,
                                                int       size)
    {
      Idle.add (set_image_from_icon_name.callback);
      yield;

      var key = hash_template.printf (icon_name, size);

      Pixbuf? ret = cache[key];

      if (ret == null)
        {
          try {
            ret = theme.load_icon (icon_name, size, 0);

            if (ret is Pixbuf)
              {
                cache[key] = ret;
              }

          } catch (Error e) {
            warning ("Unable to load icon_name: %s", e.message);
          }
        }

      if (ret is Pixbuf)
        {
          image.set_from_pixbuf (ret);
        }
    }

    public async void set_image_from_gicon_string (Ctk.Image image,
                                                   string    gicon_as_string,
                                                   int       size)
    {
      Idle.add (set_image_from_gicon.callback);
      yield;

      var key = hash_template.printf (gicon_as_string, size);

      Pixbuf? ret = cache[key];

      if (ret == null)
        {
          try {
            unowned GLib.Icon icon = GLib.Icon.new_for_string (gicon_as_string);
            var info = theme.lookup_by_gicon (icon, size, 0);
            if (info != null)
              ret = info.load_icon ();

            if (ret == null)
              {
                /* There is some funkiness in some programs where they install
                 * their icon to /usr/share/icons/hicolor/apps/, but they
                 * name the Icon= key as `foo.$extension` which breaks loading
                 * So we can try and work around that here.
                 */
                if (gicon_as_string.has_suffix (".png")
                    || gicon_as_string.has_suffix (".xpm")
                    || gicon_as_string.has_suffix (".gir")
                    || gicon_as_string.has_suffix (".jpg"))
                  {
                    string real_name = gicon_as_string[0:gicon_as_string.length-4];
                    ret = theme.load_icon (real_name, size, 0);
                  }
              }

            if (ret is Pixbuf)
              {
                cache[key] = ret;
              }
          } catch (Error e) {
            warning (@"Unable to load icon $gicon_as_string: '%s'", e.message);
          }
        }

      if (ret is Pixbuf)
        {
          image.set_from_pixbuf (ret);
        }
    }

    public async void set_image_from_gicon (Ctk.Image image,
                                            GLib.Icon icon,
                                            int       size)
    {
      yield set_image_from_gicon_string (image, icon.to_string (), size);
    }

    /*
     * Private Methods
     */
  }
}
