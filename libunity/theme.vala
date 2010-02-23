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

namespace Unity
{
  public class ThemeImage : Clutter.Texture
  {
    private static Gtk.IconTheme? theme = null;

    /*
     * ThemeImage will load a icon name from one of the Unity theme directories
     * at the size of the file, returning a "missing" icon if an icon could not
     * be loaded.
     */
    public string icon_name { get; construct set; }

    public Gdk.Pixbuf? icon;

    public ThemeImage (string icon_name)
    {
      Object (icon_name:icon_name);
    }

    construct
    {
      if (this.theme == null)
        {
          this.theme = new Gtk.IconTheme ();
          this.theme.set_custom_theme ("unity-icon-theme");
        }

      if (!this.try_load_icon_from_datadir ())
        if (!this.try_load_icon_from_theme ())
          this.load_missing_icon ();
    }

    private bool try_load_icon_from_theme ()
    {
      /* Load the icon */
      var info = this.theme.lookup_icon (this.icon_name,
                                         24, /* This is not actually used */
                                         0);
      if (info != null)
        {
          var filename = info.get_filename ();

          if ((filename.str ("unity-icon-theme") != null))
            {
              try
                {
                  this.set_from_file (filename);
                  /*
                  this.icon = new Gdk.Pixbuf.from_file (filename);
                  this.size = icon.width;
                  this.pixbuf = icon;*/

                  return true;
                }
              catch (Error e)
                {
                  return false;
                }
            }
          return false;
        }
      return false;
    }

    private bool try_load_icon_from_dir (string dir)
    {
      string filename;

      filename = Path.build_filename (dir, this.icon_name + ".png");

      try
        {
          this.set_from_file (filename);
          /*
          this.icon = new Gdk.Pixbuf.from_file (filename);
          this.size = this.icon.width;
          this.pixbuf = this.icon;
          */
          return true;
        }
      catch (Error e)
        {
          return false;
        }
    }

    private bool try_load_icon_from_datadir ()
    {
      if (!this.try_load_icon_from_dir (PKGDATADIR))
        if (!this.try_load_icon_from_dir ("/usr/share/unity/themes"))
          if (!this.try_load_icon_from_dir ("/usr/share/unity/themes/launcher"))
            return false;

      return true;
    }

    private void load_missing_icon ()
    {
      ;
      /*
      this.icon = this.theme.load_icon ("gtk-missing-image",
                                        24,
                                        Gtk.IconLookupFlags.USE_BUILTIN);

      this.size = this.icon.width;
      this.pixbuf = this.icon;
      */

      warning ("Unable to load '%s' from Unity icon theme or Unity theme",
               this.icon_name);

    }
/*
    private override void get_preferred_width (float for_height,
                                               out float min_width,
                                               out float nat_width)
    {
      min_width = (float)this.icon.width;
      nat_width = (float)this.icon.width;
    }

    private override void get_preferred_height (float for_width,
                                                out float min_height,
                                                out float nat_height)
    {
      min_height = (float)this.icon.height;
      nat_height = (float)this.icon.height;
    }*/
  }
}
