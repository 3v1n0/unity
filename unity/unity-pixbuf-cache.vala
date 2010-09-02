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
  public class PixbufCacheTask
  {
    public string  filename;
    public unowned Ctk.Image image;
    public int     size;
    public string  key;

    public PixbufCacheTask ()
    {
    }
  }

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

    private PriorityQueue<PixbufCacheTask> queue;

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

        Timeout.add (30, () => {
          load_iteration ();
        return true;
      });
    }

    construct
    {
      queue = new PriorityQueue<PixbufCacheTask> ();

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

    private void load_iteration ()
    {
      int i = 0;

      while (queue.size > 0 && i < 10)
        {
          var task = queue.poll ();

          if (task.image is Ctk.Image)
            {
              load_from_file_async.begin (task.image, task.filename, task.size, task.key);
            }

          i++;
        }
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
      var key = hash_template.printf (icon_name, size);
      Pixbuf? ret = cache[key];

      if (ret is Pixbuf)
        {
          image.set_from_pixbuf (ret);
          return;
        }

      Idle.add (set_image_from_icon_name.callback);
      yield;

      if (ret == null)
        {
          try {
            var info = theme.lookup_icon (icon_name, size, 0);
            if (info != null)
              {
                var filename = info.get_filename ();
                ret = yield load_from_filepath (filename, size, image, key);
              }

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
      var key = hash_template.printf (gicon_as_string, size);
      Pixbuf? ret = cache[key];

      if (ret is Pixbuf)
        {
          image.set_from_pixbuf (ret);
          return;
        }

      Idle.add (set_image_from_gicon.callback);
      yield;

      if (ret == null)
        {
          if (gicon_as_string[0] == '/')
            {
              try {
                ret = yield load_from_filepath (gicon_as_string, size, image, key);
              } catch (Error err) {
                message (@"Unable to load $gicon_as_string as file: %s",
                         err.message);
              }
            }

          if (ret == null)
            {
              try {
                unowned GLib.Icon icon = GLib.Icon.new_for_string (gicon_as_string);
                var info = theme.lookup_by_gicon (icon, size, 0);
                if (info != null)
                  {
                    var filename = info.get_filename ();

                    ret = yield load_from_filepath (filename, size, image, key);
                  }

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
                        info = theme.lookup_icon (real_name, size, 0);
                        if (info != null)
                          {
                            var fname = info.get_filename ();
                            ret = yield load_from_filepath (fname, size, image, key);
                          }
                      }
                  }

              } catch (Error e) {
                warning (@"Unable to load icon $gicon_as_string: '%s'", e.message);
              }
            }

          if (ret is Pixbuf)
            {
              cache[key] = ret;
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

    public async Gdk.Pixbuf? load_from_filepath (string filename, int size, Ctk.Image? image=null, string key)
    {
      var task = new PixbufCacheTask ();
      task.filename = filename;
      task.size = size;
      task.image = image;
      task.key = key;

      queue.add (task);

      return null;
      /*
      if (filename != null)
        {
          File datafile = File.new_for_path (filename);
          try
            {
              var stream =  yield datafile.read_async (Priority.DEFAULT, null);

              if (stream is FileInputStream)
                {
                  uchar[] buf = new uchar[16];
                  void *data;
                  size_t data_size;
                  yield IO.read_stream_async (stream, buf, 16, Priority.DEFAULT,

                                              null, out data, out data_size);
                  string sdata = ((string)data).ndup(data_size);

                  var loader = new Gdk.PixbufLoader ();
                  loader.write ((uchar[])data, data_size);
                  loader.close ();
                  
                  return loader.get_pixbuf ();
                }
            }
          catch (Error ee)
            {
              warning ("Unable to load image file '%s': %s", filename, ee.message);
            }
        }

      return null;
      */
    }

    private async void load_from_file_async (Ctk.Image i,
                                             string f,
                                             int    s,
                                             string k)
    {
      unowned Ctk.Image image = i;
      string filename = f;
      int size = s;
      string key = k;

      if (filename != null)
        {
          File datafile = File.new_for_path (filename);
          try
            {
              var stream =  yield datafile.read_async (Priority.DEFAULT, null);

              if (stream is FileInputStream)
                {
                  uchar[] buf = new uchar[16];
                  void *data;
                  size_t data_size;
                  yield IO.read_stream_async (stream, buf, 16, Priority.DEFAULT,

                                              null, out data, out data_size);
                  string sdata = ((string)data).ndup(data_size);

                  var loader = new Gdk.PixbufLoader ();
                  loader.write ((uchar[])data, data_size);
                  loader.close ();
                  
                  image.set_from_pixbuf (loader.get_pixbuf ());

                  cache[key] = loader.get_pixbuf ();
                }
            }
          catch (Error ee)
            {
              warning ("Unable to load image file '%s': %s", filename, ee.message);
            }
        }
    }

    /*
     * Private Methods
     */
  }
}
