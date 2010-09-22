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
  private class PixbufCacheTask
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

  private enum PixbufRequestType
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

    private Gee.Queue<PixbufCacheTask> queue;

    private uint queue_source = 0;

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
      queue = new LinkedList<PixbufCacheTask> ();
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

    private async void process_icon_queue_async ()
    {
      /* Hardcoded numbers make gord cry - but so be it - we are taking
       * a second guess of the inode size here, for the better of mankind */
      uchar[] buf = new uchar[4096];
      int count = 0;
      var timer = new Timer ();
      PixbufCacheTask task;
      
      while ((task = queue.poll()) != null)
        {
          if (task.image is Ctk.Image)
            {
              switch (task.type)
                {
                  case PixbufRequestType.ICON_NAME:
                    yield set_image_from_icon_name_real (task, buf, buf.length);
                    break;
                  case PixbufRequestType.GICON_STRING:
                    yield set_image_from_gicon_string_real (task, buf, buf.length);
                    break;
                  default:
                    critical ("Internal error. Unknown PixbufRequestType: %u",
                              task.type);
                    break;
                }
            }          
          count++;
        }
        
      timer.stop ();
      debug ("Loaded %i icons in %fms", count, timer.elapsed()*1000);
      
      /* Queue depleted */
      queue_source = 0;
    }

    private void process_icon_queue ()
    {
      if (queue_source == 0)
        {
          /* queue_source is set to 0 by process_icon_queue_async()
           * when the queue is depleted.
           * It's important that we dispatch in an idle call here since
           * it gives clients a chance to queue more icons before we
           * hammer the IO */
          queue_source = Idle.add (dispatch_process_icon_queue_async);          
        }
    }
    
    private bool dispatch_process_icon_queue_async ()
    {
      /* Run three processings in "parallel". While we are not multithreaded,
       * it still makes sense to do since one branch may be waiting in
       * 'yield' for async IO, while the other can be parsing image data.
       * The number of parallel workers has been chosen by best perceived
       * responsiveness for bringing up the Applications place */
       process_icon_queue_async.begin ();
       process_icon_queue_async.begin ();
       process_icon_queue_async.begin ();
       
       return false;
    }

    /**
     * If the icon is already cached then set it immediately on @image. Otherwise
     * does async IO to load and cache the icon, then setting it on @image.
     *
     * Note that this means that you should treat this method as an async
     * operation for all intents and purposes. You can not count on
     * image.pixbuf != null when this method call returns. That may just as well
     * happen in a subsequent idle call.
     */
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
      task.key = key;
      task.data = icon_name;
      task.image = image;
      task.size = size;
      task.type = PixbufRequestType.ICON_NAME;

      queue.offer (task);

      process_icon_queue ();
    }

    private async void set_image_from_icon_name_real (PixbufCacheTask task,
                                                      void* buf,
                                                      size_t buf_length)
    {
      Pixbuf? ret = cache[task.key];

      /* We need a secondary cache check because the icon may have
       * been cached while we where waiting in the queue */
      if (ret is Pixbuf)
        {
          task.image.set_from_pixbuf (ret);
          return;
        }
      
      try {
        var info = theme.lookup_icon (task.data, task.size, 0);
        if (info != null)
          {
            var filename = info.get_filename ();
            ret = yield load_from_filepath (filename, task.size, buf, buf_length);
          }

        if (ret is Pixbuf)
          {
            cache[task.key] = ret;
          }

      } catch (Error e) {
        warning ("Unable to load icon_name: %s", e.message);
      }

      if (ret is Pixbuf)
        {
          task.image.set_from_pixbuf (ret);
        }
    }

    /**
     * If the icon is already cached then set it immediately on @image. Otherwise
     * does async IO to load and cache the icon, then setting it on @image.
     *
     * Note that this means that you should treat this method as an async
     * operation for all intents and purposes. You can not count on
     * image.pixbuf != null when this method call returns. That may just as well
     * happen in a subsequent idle call.
     */
    public void set_image_from_gicon_string (Ctk.Image image,
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

      var task = new PixbufCacheTask ();
      task.key = key;
      task.image = image;
      task.data = gicon_as_string;
      task.size = size;
      task.type = PixbufRequestType.GICON_STRING;

      queue.offer (task);

      process_icon_queue ();
    }

    private async void set_image_from_gicon_string_real (PixbufCacheTask task,
                                                         void* buf, 
                                                         size_t buf_length)
    {
      Pixbuf? ret = cache[task.key];

      /* We need a secondary cache check because the icon may have
       * been cached while we where waiting in the queue */
      if (ret is Pixbuf)
        {
          task.image.set_from_pixbuf (ret);
          return;
        }
      
      if (task.data[0] == '/')
        {
          try {
            ret = yield load_from_filepath (task.data, task.size, buf, buf_length);
          } catch (Error err) {
            message (@"Unable to load $(task.data) as file: %s",
                     err.message);
          }
        }

      if (ret == null)
        {
          try {
            unowned GLib.Icon icon = GLib.Icon.new_for_string (task.data);
            var info = theme.lookup_by_gicon (icon, task.size, 0);
            if (info != null)
              {
                var filename = info.get_filename ();
                ret = yield load_from_filepath (filename, task.size, buf, buf_length);
              }

            if (ret == null)
              {
                /* There is some funkiness in some programs where they install
                 * their icon to /usr/share/icons/hicolor/apps/, but they
                 * name the Icon= key as `foo.$extension` which breaks loading
                 * So we can try and work around that here.
                 */
                 if (task.data.has_suffix (".png")
                     || task.data.has_suffix (".xpm")
                     || task.data.has_suffix (".gif")
                     || task.data.has_suffix (".jpg"))
                   {
                     string real_name = task.data[0:task.data.length-4];
                     info = theme.lookup_icon (real_name, task.size, 0);
                     if (info != null)
                       {
                         var fname = info.get_filename ();
                         ret = yield load_from_filepath (fname, task.size, buf, buf_length);
                       }
                   }
              }
          } catch (Error e) {
            warning (@"Unable to load icon $(task.data): '%s'", e.message);
          }
        }

      if (ret is Pixbuf)
        {
          cache[task.key] = ret;
          task.image.set_from_pixbuf (ret);
        }
    }

    /**
     * If @icon is already cached then set it immediately on @image. Otherwise
     * does async IO to load and cache the icon, then setting it on @image.
     *
     * Note that this means that you should treat this method as an async
     * operation for all intents and purposes. You can not count on
     * image.pixbuf != null when this method call returns. That may just as well
     * happen in a subsequent idle call.
     */
    public async void set_image_from_gicon (Ctk.Image image,
                                            GLib.Icon icon,
                                            int       size)
    {
      set_image_from_gicon_string (image, icon.to_string (), size);
    }

    private async Gdk.Pixbuf? load_from_filepath (string filename, int size,
                                                  void* buf, size_t buf_length) throws Error
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
                  void *data;
                  size_t data_size;
                  yield IO.read_stream_async (stream, buf, buf_length, Priority.DEFAULT,
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
