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
 * <http://www.gnu.org/licenses/>.
 *
 * Authored by Gordon Allott <gord.allott@canonical.com>
 *
 */

namespace Unity.Webapp
{

  public class Prism : Object
  {
    static const string webapp_ini_template = """[Parameters]
id=%s@unity.app
name=%s
uri=%s
status=yes
location=no
sidebar=no
navigation=no""";

    static const string webapp_desktop_template = """[Desktop Entry]
Name=%s
Type=Application
Comment=Web Application
Exec="prism" -webapp %s@unity.app
Categories=GTK;Network;
StartupWMClass=Prism
StartupNotify=true
Icon=%s
""";

    public string url {get; construct;}
    public string icon {get; construct;}
    public string name;
    public string id;
    string webapp_dir;

    public Prism (string address, string icon)
    {
      Object (url:address, icon:icon);
    }

    construct
    {
      //FIXME - we need to get a "name" for webapps somehow, not sure how...
      var split_url = url.split ("://", 2);
      name = split_url[1];

      try {
        var regex = new Regex ("(/)");
        name = regex.replace (name, -1, 0, "-");
      } catch (RegexError e) {
        warning ("%s", e.message);
      }

      id = name;
      webapp_dir = Environment.get_home_dir () +
                    "/.webapps/%s@unity.app".printf (id);

      bool exists = check_existance_of_app ();
      if (!exists)
      {
        build_webapp ();
      }
    }

    public string desktop_file_path ()
    {
      return Environment.get_home_dir () + "/.webapps/%s@unity.app".printf (id);
    }

    /* checks for the webapp given based on the url stored */
    private bool check_existance_of_app ()
    {
      if (this.url == "")
      {
        return true;
      }

      var webapp_dir_file = File.new_for_path (webapp_dir);
      if (webapp_dir_file.query_exists (null))
      {
        return true;
      }
      return false;
    }

    private void build_webapp ()
    {

      string webapp_ini = webapp_ini_template.printf (id, name, url);
      string webapp_desktop = webapp_desktop_template.printf (name, id, this.icon);

      var webapp_directory = File.new_for_path (webapp_dir);
      try
      {
        webapp_directory.make_directory_with_parents (null);
      } catch (Error e)
      {
        warning ("%s", e.message);
        return;
      }

      var inifile = File.new_for_path (webapp_dir + "/webapp.ini");
      try
      {
        var file_stream = inifile.create (FileCreateFlags.NONE, null);
        var data_stream = new DataOutputStream (file_stream);
        data_stream.put_string (webapp_ini, null);

      } catch (Error e)
      {
        warning ("%s", e.message);
        return;
      }

      var desktop_file = File.new_for_path (webapp_dir +
                                            "/%s.desktop".printf (name));
      try
      {
        var file_stream = desktop_file.create (FileCreateFlags.NONE, null);
        var data_stream = new DataOutputStream (file_stream);
        data_stream.put_string (webapp_desktop, null);
      } catch (Error e)
      {
        warning ("could not write to %s/%s.desktop", webapp_dir, name);
        return;
      }
    }

    public void add_to_favorites ()
    {
      var favorites = Unity.Favorites.get_default ();
      string uid = get_fav_uid ();
      if (uid != "")
        {
          // we are a favorite already
          warning ("%s is already a favorite", name);
          return;
        }

      string desktop_path = webapp_dir + "/%s.desktop".printf (name);
      uid = "webapp-" + Path.get_basename (desktop_path);
      uid = Unity.Webapp.urlify (uid);

      // we are not a favorite and we need to be favorited!
      favorites.set_string (uid, "type", "application");
      favorites.set_string (uid, "desktop_file", desktop_path);
      favorites.add_favorite (uid);
    }


     /**
     * gets the favorite uid for this desktop file
     */
    private string get_fav_uid ()
    {
      string myuid = "";
      string my_desktop_path = webapp_dir + "/%s.desktop".printf (name);
      var favorites = Unity.Favorites.get_default ();
      Gee.ArrayList<string> favorite_list = favorites.get_favorites();
      foreach (string uid in favorite_list)
        {
          // we only want favorite *applications* for the moment
          var type = favorites.get_string(uid, "type");
          if (type != "application")
              continue;

          string desktop_file = favorites.get_string(uid, "desktop_file");
          if (desktop_file == my_desktop_path)
            {
              myuid = uid;
            }
        }
      return myuid;
    }
  }

}
