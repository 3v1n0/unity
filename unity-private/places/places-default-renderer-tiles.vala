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

namespace Unity.Places
{
  public abstract class Tile : Ctk.Button
  {
    static const string DEFAULT_ICON = "text-x-preview";

    public unowned Dee.ModelIter iter { get; construct; }

    public string  display_name { get; construct; }
    public string? icon_hint    { get; construct; }
    public string  uri          { get; construct; }
    public string? mimetype     { get; construct; }
    public string? comment      { get; construct; }

    public signal void activated (string uri, string mimetype);

    public abstract void about_to_show ();
  }

  public class FileInfoTile : Tile
  {
    static const int   ICON_WIDTH  = 48;
    static const int   ICON_HEIGHT = 48;
    static const float LINE_PADDING = 1.0f;
    static const float VPADDING = 4.0f;

    private bool shown = false;

    private Ctk.Image icon;
    private Ctk.Text  leaf;
    private Ctk.Text  folder;
    private Ctk.Text  time;

    public FileInfoTile (Dee.ModelIter iter,
                         string        uri,
                         string?       icon_hint,
                         string?       mimetype,
                         string        display_name,
                         string?       comment)
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL,
              iter:iter,
              display_name:display_name,
              icon_hint:icon_hint,
              uri:uri,
              mimetype:mimetype,
              comment:comment);
    }

    construct
    {
    }

    private override void get_preferred_width (float for_height,
                                               out float mwidth,
                                               out float nwidth)
    {
      mwidth = 210.0f;
      nwidth = 210.0f;
    }

    public override void about_to_show ()
    {
      if (shown)
        return;
      shown = true;
      
      icon = new Ctk.Image (ICON_HEIGHT);
      icon.set_parent (this);
      icon.show ();
     
      leaf = new Ctk.Text (" ");
      leaf.ellipsize = Pango.EllipsizeMode.MIDDLE;
      leaf.set_parent (this);
      leaf.show ();

      folder = new Ctk.Text ("");
      folder.ellipsize = Pango.EllipsizeMode.MIDDLE;
      folder.set_parent (this);
      folder.color = { 255, 255, 255, 150 };
      folder.show ();

      time = new Ctk.Text (uri == comment ? "" : comment);
      time.ellipsize = Pango.EllipsizeMode.MIDDLE;
      time.set_parent (this);
      time.show ();

      Timeout.add (0, () => {

        leaf.text = display_name;

        /* Set the parent directory */
        var file = File.new_for_uri (uri);
        if (file is File)
          {
            var parent = file.get_parent ();
            if (parent is File) 
              folder.text = parent.get_basename ();              
            else
              folder.text = "";
          }
        else
          folder.text = "";

        set_icon ();
        return false;
      });

      queue_relayout ();
    }

    private override void clicked ()
    {
      activated (uri, mimetype);
    }
    
    private void set_icon ()
    {
      var cache = PixbufCache.get_default ();

      if (icon_hint != null && icon_hint != "")
        {
          cache.set_image_from_gicon_string (icon,
                                             icon_hint,
                                             ICON_HEIGHT);
        }
      else if (mimetype != null && mimetype != "")
        {
          var icon = GLib.g_content_type_get_icon (mimetype);
          cache.set_image_from_gicon_string (this.icon,
                                             icon.to_string(),
                                             ICON_HEIGHT);
        }
      else
        {
          cache.set_image_from_icon_name (icon,
                                          Tile.DEFAULT_ICON,
                                          ICON_HEIGHT);
        }
    }

    private override void allocate (Clutter.ActorBox box,
                                    Clutter.AllocationFlags flags)
    {
      base.allocate (box, flags);

      if (icon is Ctk.Image == false)
        return;

      var width = box.x2 - box.x1;
      var height = box.y2 - box.y1;

      Clutter.ActorBox child_box = Clutter.ActorBox ();

      child_box.x1 = 0.0f;
      child_box.x2 = ICON_WIDTH;
      child_box.y1 = (height/2.0f) - (ICON_HEIGHT/2.0f);
      child_box.y2 = child_box.y1 + ICON_HEIGHT;
      icon.allocate (child_box, flags);

      float mh;
      leaf.get_preferred_height (width - ICON_WIDTH, out mh, null);

      var text_block = (mh * 3.0f) + (LINE_PADDING * 2.0f);
      var y = (height/2.0f) - (text_block/2.0f);
      var icon_text_pad = 12.0f;
      
      child_box.x1 = ICON_WIDTH + icon_text_pad;
      child_box.x2 = width;
      child_box.y1 = y;
      child_box.y2 = y + mh;
      leaf.allocate (child_box, flags);

      y+= LINE_PADDING + mh;
      child_box.y1 = y;
      child_box.y2 = y + mh;
      folder.allocate (child_box, flags);

      y+=LINE_PADDING + mh;
      child_box.y1 = y;
      child_box.y2 = y + mh;
      time.allocate (child_box, flags);
    }

    private override void get_preferred_height (float for_width,
                                                out float mheight,
                                                out float nheight)
    {
      if (icon is Ctk.Image)
        {
          float mh;
          leaf.get_preferred_height (for_width, out mh, null);

          mheight = Math.fmaxf (ICON_HEIGHT + (VPADDING*2.0f),
                                (mh*3.0f) + (LINE_PADDING*2.0f) + (VPADDING*2.0f));
          nheight = mheight;

        }
      else
        {
          mheight = ICON_HEIGHT + (VPADDING*2.0f);
          nheight = mheight;
        }
    }

    private override void paint ()
    {
      base.paint ();

      if (icon is Ctk.Image)
        {
          icon.paint ();
          leaf.paint ();
          folder.paint ();
          time.paint ();
        }
    }

    private override void pick (Clutter.Color color)
    {
      base.pick (color);

      if (icon is Ctk.Image)
        {
          icon.paint ();
          leaf.paint ();
          folder.paint ();
          time.paint ();
        }
    }

    private override void map ()
    {
      base.map ();
      if (icon is Ctk.Image)
        {
          icon.map ();
          leaf.map ();
          folder.map ();
          time.map ();
        }
    }

    private override void unmap ()
    {
      if (icon is Ctk.Image)
        {
          icon.unmap ();
          leaf.unmap ();
          folder.unmap ();
          time.unmap ();
        }
      base.unmap ();
    }
  }

  public class ShowcaseTile : Tile
  {
    static const int ICON_SIZE = 64;

    private bool shown = false;

    public ShowcaseTile (Dee.ModelIter iter,
                         string        uri,
                         string?       icon_hint,
                         string?       mimetype,
                         string        display_name,
                         string?       comment)
    {
      Object (orientation:Ctk.Orientation.VERTICAL,
              iter:iter,
              display_name:display_name,
              icon_hint:icon_hint,
              uri:uri,
              mimetype:mimetype,
              comment:comment);
    }

    construct
    {
      unowned Ctk.Text text = get_text ();
      text.ellipsize = Pango.EllipsizeMode.MIDDLE;
    }

    public override void about_to_show ()
    {
      if (shown)
        return;
      shown = true;

      Timeout.add (0, () => {
        set_label (display_name);
        set_icon ();
        return false;
      });
    }

    private override void get_preferred_width (float for_height,
                                               out float mwidth,
                                               out float nwidth)
    {
      mwidth = 160.0f;
      nwidth = 160.0f;
    }

    private override void clicked ()
    {
      activated (uri, mimetype);
    }
    
    private void set_icon ()
    {

      get_image ().size = ICON_SIZE;
      var cache = PixbufCache.get_default ();

      if (icon_hint != null && icon_hint != "")
        {
          cache.set_image_from_gicon_string (get_image (),
                                             icon_hint,
                                             ICON_SIZE);
        }
      else if (mimetype != null && mimetype != "")
        {
          var icon = GLib.g_content_type_get_icon (mimetype);
          cache.set_image_from_gicon_string (get_image (),
                                             icon.to_string(),
                                             ICON_SIZE);
        }
      else
        {
          cache.set_image_from_icon_name (get_image (),
                                          Tile.DEFAULT_ICON,
                                          ICON_SIZE);
        }
    }

  }

  public class DefaultTile : Tile
  {
    static const int ICON_SIZE = 48;

    private bool shown = false;

    public DefaultTile (Dee.ModelIter iter,
                        string        uri,
                        string?       icon_hint,
                        string?       mimetype,
                        string        display_name,
                        string?       comment)
    {
      Object (orientation:Ctk.Orientation.VERTICAL,
              iter:iter,
              display_name:display_name,
              icon_hint:icon_hint,
              uri:uri,
              mimetype:mimetype,
              comment:comment);
    }

    construct
    {
      unowned Ctk.Text text = get_text ();
      text.ellipsize = Pango.EllipsizeMode.MIDDLE;
    }

    public override void about_to_show ()
    {
      if (shown)
        return;
      shown = true;

      Timeout.add (0, () => {
        set_label (display_name);
        set_icon ();
        return false;
      });
    }

    private override void get_preferred_width (float for_height,
                                               out float mwidth,
                                               out float nwidth)
    {
      mwidth = 160.0f;
      nwidth = 160.0f;
    }

    private override void clicked ()
    {
      activated (uri, mimetype);
    }
    
    private void set_icon ()
    {
      var cache = PixbufCache.get_default ();

      if (icon_hint != null && icon_hint != "")
        {
          cache.set_image_from_gicon_string (get_image (),
                                             icon_hint,
                                             ICON_SIZE);
        }
      else if (mimetype != null && mimetype != "")
        {
          var icon = GLib.g_content_type_get_icon (mimetype);
          cache.set_image_from_gicon_string (get_image (),
                                             icon.to_string(),
                                             ICON_SIZE);
        }
      else
        {
          cache.set_image_from_icon_name (get_image (),
                                          Tile.DEFAULT_ICON,
                                          ICON_SIZE);
        }
    }
  }
}
