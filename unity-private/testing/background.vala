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

namespace Unity.Testing
{
  public class Background : Ctk.Bin
  {
    /* Gnome Desktop background gconf keys */
    private string BG_DIR    = "/desktop/gnome/background";
    private string BG_FILE   = "/desktop/gnome/background/picture_filename";
    private string BG_OPTION = "/desktop/gnome/background/picture_options";

    private string filename;
    private string option;

    private Clutter.Texture bg;

    construct
    {
      START_FUNCTION ();
      var client = GConf.Client.get_default ();

      /* Setup the initial properties and notifies */
      try
        {
          client.add_dir (this.BG_DIR, GConf.ClientPreloadType.NONE);
        }
      catch (GLib.Error e)
        {
          warning ("Background: Unable to monitor background: %s", e.message);
        }

      try
        {
          this.filename = client.get_string (this.BG_FILE);
        }
      catch (Error e)
        {
          this.filename = "/usr/share/backgrounds/warty-final.png";
        }

      try
        {
          client.notify_add (this.BG_FILE, this.on_filename_changed);
        }
      catch (Error e)
        {
          warning ("Background: Unable to monitor background filename: %s",
                   e.message);
        }
      try
        {
          this.option = client.get_string (this.BG_OPTION);
        }
      catch (Error e)
        {
          this.option = "wallpaper";
        }
      try
        {
          client.notify_add (this.BG_OPTION, this.on_option_changed);
        }
      catch (Error e)
        {
          warning ("Background: Unable to monitor background options: %s",
                   e.message);
        }

      /* The texture that will show the background */
      this.bg = new Clutter.Texture ();
      this.bg.set_load_async (true);
      this.add_actor (this.bg);
      this.bg.show ();

      /* Load the texture */
      this.ensure_layout ();
      END_FUNCTION ();
    }

    private void on_filename_changed (GConf.Client client,
                                      uint         cxnid,
                                      GConf.Entry  entry)
    {
      var new_filename = entry.get_value ().get_string ();

      if (new_filename == this.filename)
        return;

      this.filename = new_filename;
      this.ensure_layout ();
    }

    private void on_option_changed (GConf.Client client,
                                     uint         cxnid,
                                     GConf.Entry  entry)
    {
      var new_option = entry.get_value ().get_string ();

      if (new_option == this.option)
        return;

      this.option = new_option;
      this.ensure_layout ();
    }

    private void ensure_layout ()
    {
      try
        {
          this.bg.set_from_file (this.filename);
        }
      catch (Error e)
        {
          warning ("Background: Unable to load background file %s: %s",
                   this.filename,
                   e.message);
        }

      this.queue_relayout ();
    }
  }
}
