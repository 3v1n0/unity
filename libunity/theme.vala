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
using Gee;

namespace Unity
{
  /* An async object, designed to be used to grab filepath from the theme
   * use add_icon_theme to add a theme to the lookup, will always default to
   * the default theme if it can't find one in the the themes you provide
   */
  public class ThemeFilePath : Object
  {
    PriorityQueue<Gtk.IconTheme> themes;
    public signal void found_icon_path (string filepath);

    construct
    {
      this.themes = new PriorityQueue<Gtk.IconTheme> ();
    }
    /* adds a theme name to lookup icons from */
    public void add_icon_theme (Gtk.IconTheme theme)
    {
      this.themes.add (theme);
    }

    public async void get_icon_filepath (string icon_name)
    {
      string filepath = "";
      foreach (Gtk.IconTheme theme in this.themes)
        {
          filepath = yield this.path_from_theme (icon_name, theme);
          if (filepath != "")
            {
              break;
            }
        }

      this.found_icon_path (filepath);
    }

    private async string path_from_theme (string icon_name, Gtk.IconTheme theme)
    {
      Idle.add (path_from_theme.callback);
      yield;

      if (theme.has_icon (icon_name))
        {
          var info = theme.lookup_icon (icon_name,
                                             48,
                                             0);
          if (info != null)
            {
              var filename = info.get_filename ();
              if (FileUtils.test(filename, FileTest.IS_REGULAR))
                {
                  return filename;
                }
            }
        }
      return "";
    }

  }


  public static bool icon_name_exists_in_theme (string icon_name, string theme)
  {
    var icontheme = new Gtk.IconTheme ();
    icontheme.set_custom_theme (theme);
    return icontheme.has_icon (icon_name);
  }

  public class ThemeImage : Clutter.Texture
  {
    private ThemeFilePath? theme = null;
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
/*
      this.set_load_async (true);
*/
      if (this.theme == null)
        {
          this.theme = new ThemeFilePath ();
          var gtktheme = new Gtk.IconTheme ();
          gtktheme.set_custom_theme ("unity-icon-theme");
          this.theme.add_icon_theme (gtktheme);
        }

      if (!this.try_load_icon_from_datadir ())
        if (!this.try_load_icon_from_theme ())
          this.load_missing_icon ();
    }

    private bool try_load_icon_from_theme ()
    {
      this.theme.found_icon_path.connect ((theme, filepath) => {
        try
          {
            this.set_from_file (filepath);
          }
        catch (Error e)
          {
            warning (@"could not load theme image $filepath");
          }
      });
      this.theme.get_icon_filepath (this.icon_name);

      return true;
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
