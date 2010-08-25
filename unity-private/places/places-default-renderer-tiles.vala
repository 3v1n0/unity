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
    public unowned Dee.ModelIter iter { get; construct; }

    public string  display_name { get; construct; }
    public string? icon_hint    { get; construct; }
    public string  uri          { get; construct; }
    public string? mimetype     { get; construct; }
    public string? comment      { get; construct; }

    public signal void activated (string uri, string mimetype);

    public abstract void about_to_show ();
  }

  public class DefaultTile : Tile
  {
    static const int ICON_SIZE = 48;
    static const string DEFAULT_ICON = "text-x-preview";
/*
    public unowned Dee.ModelIter iter { get; construct; }

    public string  display_name { get; construct; }
    public string? icon_hint    { get; construct; }
    public string  uri          { get; construct; }
    public string? mimetype     { get; construct; }
    public string? comment      { get; construct; }*/

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
      mwidth = 150.0f;
      nwidth = 150.0f;
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
          cache.set_image_from_gicon_string (get_image (), icon_hint, ICON_SIZE);
        }
      else if (mimetype != null && mimetype != "")
        {
          var icon = GLib.g_content_type_get_icon (mimetype);
          cache.set_image_from_gicon_string (get_image (), icon.to_string(), ICON_SIZE);
        }
      else
        {
          cache.set_image_from_icon_name (get_image (), DEFAULT_ICON, ICON_SIZE);
        }
    }
  }
}
