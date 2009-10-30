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
  public class Background : Ctk.Actor
  {
    /* Gnome Desktop background gconf keys */
    private string BG_DIR    = "/desktop/gnome/background";
    private string BG_FILE   = "/desktop/gnome/background/picture_filename";
    private string BG_OPTION = "/desktop/gnome/background/picture_options";
    
    private string filename;
    private string option;
    
    public Background ()
    {

    }

    construct
    {
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

      this.filename = client.get_string (this.BG_FILE);
      try
        {
          client.notify_add (this.BG_FILE, this.on_filename_changed);
        }
      catch (Error e)
        {
          warning ("Background: Unable to monitor background: %s", e.message);
        }

      this.option = client.get_string (this.BG_OPTION);
      try
        {
          client.notify_add (this.BG_OPTION, this.on_option_changed);
        }
      catch (Error e)
        {
          warning ("Background: Unable to monitor background: %s", e.message);
        }

      debug ("Background: %s %s", this.filename, this.option);
    }

    private void on_filename_changed (GConf.Client client,
                                      uint         cxnid,
                                      GConf.Entry  entry)
    {
      var new_filename = entry.get_value ().get_string ();
      
      if (new_filename == this.filename)
        return;
      
      this.filename = new_filename;
      debug ("Background: FilenameChanged: %s\n", this.filename);
    }

    private void on_option_changed (GConf.Client client,
                                     uint         cxnid,
                                     GConf.Entry  entry)
    {
      var new_option = entry.get_value ().get_string ();

      if (new_option == this.option)
        return;

      this.option = new_option;
      debug ("Background: OptionsChanged: %s\n", this.option);
    }
  }

}
