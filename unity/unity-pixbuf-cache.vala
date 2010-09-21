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
    public string  data;
    public unowned Ctk.Image image;
    public int     size;
    public string  key;
    public PixbufRequestType type;

    public PixbufCacheTask ()
    {
    }
  }

  public enum PixbufRequestType
  {
    ICON_NAME,
    GICON_STRING
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

    private uint queue_timeout = 0;

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
      queue = new PriorityQueue<PixbufCacheTask> ();

      theme = Gtk.IconTheme.get_default ();
      cache = new HashMap<string, Gdk.Pixbuf> ();
    }

    ~PixbufCache ()
    {
      /* Make sure we don't process icons after we're dead */
      if (queue_timeout != 0)
        {
          Source.remove (queue_timeout);
          queue_timeout = 0;
        }
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

    public bool load_iteration ()
    {
      int i = 0;

      while (queue.size > 0 && i < 10)
        {
          var task = queue.poll ();

          if (task.image is Ctk.Image)
            {
              if (task.type == PixbufRequestType.ICON_NAME)
                set_image_from_icon_name_real (task.image, task.data, task.size);
              else if (task.type == PixbufRequestType.GICON_STRING)
                set_image_from_gicon_string_real (task.image, task.data, task.size);
            }

          i++;
        }

      if (queue.size == 0)
        queue_timeout = 0;

      return queue.size != 0;
    }

    public void set_image_from_icon_name (Ctk.Image image,
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

      var task = new PixbufCacheTask ();
      task.data = icon_name;
      task.image = image;
      task.size = size;
      task.type = PixbufRequestType.ICON_NAME;

      queue.add (task);

      if (queue_timeout == 0)
        queue_timeout = Idle.add (load_iteration);
    }

    private async void set_image_from_icon_name_real (Ctk.Image image,
                                                      string    icon_name,
                                                      int       size)
    {
      var key = hash_template.printf (icon_name, size);
      Pixbuf? ret = cache[key];

      /* We need a secondary cache check because the icon may have
       * been cached while we where waiting in the queue */
      if (ret is Pixbuf)
        {
          image.set_from_pixbuf (ret);
          return;
        }
      
      if (ret == null)
        {
          try {
            var info = theme.lookup_icon (icon_name, size, 0);
            if (info != null)
              {
                var filename = info.get_filename ();
                ret = yield load_from_filepath (filename, size);
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

    public void set_image_from_gicon_string (Ctk.Image image,
                                                   string data,
                                                   int    size)
    {
      var key = hash_template.printf (data, size);
      Pixbuf? ret = cache[key];

      if (ret is Pixbuf)
        {
          image.set_from_pixbuf (ret);
          return;
        }

      var task = new PixbufCacheTask ();
      task.image = image;
      task.data = data;
      task.size = size;
      task.type = PixbufRequestType.GICON_STRING;

      queue.add (task);

      if (queue_timeout == 0)
        queue_timeout = Idle.add (load_iteration);
    }

    private async void set_image_from_gicon_string_real (Ctk.Image image,
                                                         string    gicon_as_string,
                                                         int       size)
    {
      var key = hash_template.printf (gicon_as_string, size);
      Pixbuf? ret = cache[key];

      /* We need a secondary cache check because the icon may have
       * been cached while we where waiting in the queue */
      if (ret is Pixbuf)
        {
          image.set_from_pixbuf (ret);
          return;
        }
      
      if (ret == null)
        {
          if (gicon_as_string[0] == '/')
            {
              try {
                ret = yield load_from_filepath (gicon_as_string, size);
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

                    ret = yield load_from_filepath (filename, size);
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
                        || gicon_as_string.has_suffix (".gif")
                        || gicon_as_string.has_suffix (".jpg"))
                      {
                        string real_name = gicon_as_string[0:gicon_as_string.length-4];
                        info = theme.lookup_icon (real_name, size, 0);
                        if (info != null)
                          {
                            var fname = info.get_filename ();
                            ret = yield load_from_filepath (fname, size);
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
      set_image_from_gicon_string (image, icon.to_string (), size);
    }

    public async Gdk.Pixbuf? load_from_filepath (string filename, int size)
    {
      
      if (filename != null)
        {
          File datafile = File.new_for_path (filename);
          FileInfo info = yield datafile.query_info_async (
                                      FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                      FileQueryInfoFlags.NONE,
                                      Priority.LOW);
          string mimetype = info.get_content_type ();
          
          try
            {
              var stream =  yield datafile.read_async (Priority.DEFAULT, null);

              if (stream is FileInputStream)
                {
                  /* TODO: Tweak buf size to optimize io */
                  uchar[] buf = new uchar[1024];
                  void *data;
                  size_t data_size;
                  yield IO.read_stream_async (stream, buf, buf.length, Priority.DEFAULT,
                                              null, out data, out data_size);

                  /* Construct a loader for a given mimetype so it doesn't
                   * have to do expensive second guesssing */
                  var loader = new Gdk.PixbufLoader.with_mime_type(mimetype);
                  loader.write ((uchar[])data, data_size);
                  loader.close ();
                  
                  /* We must manually free the data block allocated by
                   * IO.read_stream_async */
                  g_free (data);
                  
                  return loader.get_pixbuf ();
                }
            }
          catch (Error ee)
            {
              warning ("Unable to load image file '%s': %s", filename, ee.message);
            }
        }

      return null;
    }

  }
}
