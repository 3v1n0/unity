/*
 *      unity-favorites.c
 *      Copyright (C) 2010 Canonical Ltd
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 *
 *
 *      Authored by Gordon Allott <gord.allott@canonical.com>
 */
using Gee;

namespace Unity
{
  private static Unity.Favorites favorites_singleton;

  public abstract class Favorites : Object
  {
    public static Unity.Favorites get_default ()
    {
      if (!(Unity.favorites_singleton is Unity.Favorites))
        Unity.favorites_singleton = new Unity.GConfFavorites ();
      return Unity.favorites_singleton;
    }

    public signal void favorite_added (string uid);
    public signal void favorite_removed (string uid);
    public signal void favorite_changed (string uid);

    public abstract ArrayList<string> get_favorites ();

    public abstract void add_favorite (string uid);
    public abstract void remove_favorite (string uid);
    public abstract bool is_favorite (string uid);

    public abstract string? get_string (string uid, string name);
    public abstract void set_string (string uid, string name, string value);

    public abstract int? get_int (string uid, string name);
    public abstract void set_int (string uid, string name, int value);

    public abstract float? get_float (string uid, string name);
    public abstract void set_float (string uid, string name, float value);

    public abstract bool? get_bool (string uid, string name);
    public abstract void set_bool (string uid, string name, bool value);
  }

  public class GConfFavorites : Favorites
  {
    static const string path = "/desktop/unity/launcher/favorites/";
    private GConf.Client? client;
    private SList<string> fav_ids;
    private HashMap<string, uint> notify_map;

    construct
    {
      client = GConf.Client.get_default ();
      notify_map = new HashMap<string, uint> ();
      favorite_added.connect (on_favorite_added);
      favorite_removed.connect (on_favorite_removed);
      try
        {
          fav_ids = client.get_list (path + "favorites_list", GConf.ValueType.STRING);
        }
      catch (Error e)
        {
          warning ("Could not grab favorites from gconf %s", e.message);
          fav_ids = new SList<string> ();
        }

      // set a watch on the favorites list
      try
        {
          client.add_dir (path + "favorites_list",
                          GConf.ClientPreloadType.NONE);
          client.notify_add (path + "favorites_list",
                             notify_on_favorites_list_changed);
        }
      catch (Error e)
        {
          warning ("Unable to monitor gconf for favorites changes: %s",
                   e.message);
        }
    }

    /* abstract class implimentation */

    public override ArrayList<string> get_favorites ()
    {
      ArrayList<string> favorites = new ArrayList<string> ();
      foreach (string id in fav_ids)
        {
          favorites.add (id);
        }
      return favorites;
    }

    public override void add_favorite (string uid)
    {
      if (!is_favorite (uid))
        {
          fav_ids.append (uid);
          try
            {
              client.set_list (path + "favorites_list", GConf.ValueType.STRING, fav_ids);
            }
          catch (Error e)
            {
              warning ("Could not set the favorites list: %s", e.message);
            }

					favorite_added (uid);
        }
    }

    public override void remove_favorite (string uid)
    {
      if (is_favorite (uid))
        {
					unowned SList<string> l = null;
					for (l = fav_ids; l != null; l = l.next)
						{
							string id = l.data;
							if (id == uid)
								{
									fav_ids.remove (l.data);
									break;
								}
						}
          try
            {
              client.set_list (path + "favorites_list", GConf.ValueType.STRING, fav_ids);
            }
          catch (Error e)
            {
              warning ("Could not set the favorites list: %s", e.message);
            }

					favorite_removed (uid);
        }
    }

    public override bool is_favorite (string uid)
    {
			foreach (string id in fav_ids)
				{
					if (id == uid)
						{
							return true;
						}
				}
			return false;
		}

    public override string? get_string (string uid, string name)
    {
      string? return_string = null;
      try
        {
          return_string = client.get_string (path + uid + "/" + name);
        }
      catch (Error e)
        {
          warning ("GConf string lookup failed: %s", e.message);
        }
      return return_string;
    }

    public override void set_string (string uid, string name, string value)
    {
      try
        {
          client.set_string (path + uid + "/" + name, value);
        }
      catch (Error e)
        {
          warning ("GConf string setting failed: %s", e.message);
        }
    }

    public override int? get_int (string uid, string name)
    {
      int? return_val = null;
      try
        {
          return_val = client.get_int (path + uid + "/" + name);
        }
      catch (Error e)
        {
          warning ("GConf int lookup failed: %s", e.message);
        }
      return return_val;
    }

    public override void set_int (string uid, string name, int value)
    {
      try
        {
          client.set_int (path + uid + "/" + name, value);
        }
      catch (Error e)
        {
          warning ("GConf int setting failed: %s", e.message);
        }
    }

    public override float? get_float (string uid, string name)
    {
      float? return_val = null;
      try
        {
          return_val = (float)(client.get_float (path + uid + "/" + name));
        }
      catch (Error e)
        {
          warning ("GConf float lookup failed: %s", e.message);
        }
      return return_val;
    }

    public override void set_float (string uid, string name, float value)
    {
      try
        {
          client.set_float (path + uid + "/" + name, value);
        }
      catch (Error e)
        {
          warning ("GConf float set failed: %s", e.message);
        }
    }

    public override bool? get_bool (string uid, string name)
    {
      bool? return_val = null;
      try
        {
          return_val = client.get_bool (path + uid + "/" + name);
        }
      catch (Error e)
        {
          warning ("GConf bool lookup failed: %s", e.message);
        }
      return return_val;
    }

    public override void set_bool (string uid, string name, bool value)
    {
      try
        {
          client.set_bool (path + uid + "/" + name, value);
        }
      catch (Error e)
        {
          warning ("GConf bool setting failed: %s", e.message);
        }
    }

    /* private methods */
    /* Expects a list of strings */
    private void compare_string_list (SList<string> old_list, SList<string> new_list,
                                      out SList<string> added, out SList<string> removed)
    {
      SList<string> unchanged = new SList<string> ();

      foreach (string id in new_list)
        {
					string? item = null;
					foreach (string old_item in old_list)
						{
							if (id == old_item)
								{
									item = old_item;
								}
						}

          if (item != null)
            {
              unchanged.append (id);
            }
          else
            {
              added.append (id);
            }
        }

      foreach (string id in old_list)
        {
					string? item = null;
					foreach (string unchanged_item in unchanged)
						{
							if (unchanged_item == id)
								{
									item = unchanged_item;
								}
						}

          if (item == null)
            {
              removed.append (id);
            }
        }
    }


    /* callbacks */
    private void notify_on_favorites_list_changed ()
    {
      SList<string> items_added = new SList<string> ();
      SList<string> items_removed = new SList<string> ();
      SList<string> new_favs;
      try
        {
          new_favs = client.get_list (path + "favorites_list", GConf.ValueType.STRING);
        }
      catch (Error e)
        {
          warning ("Could not get favourite list from gconf %s", e.message);
          return; // we return here because we can't do anything with no working gconf
        }

      compare_string_list (fav_ids, new_favs, out items_added, out items_removed);
      foreach (string id in items_added)
        {
          favorite_added (id);
        }
      foreach (string id in items_removed)
        {
          favorite_removed (id);
        }
    }

    private void on_favorite_added (string uid)
    {
       // set a watch on the favorite
      try
        {
          client.add_dir (path + uid + "/",
                          GConf.ClientPreloadType.NONE);
          uint notify_id = client.notify_add (path + uid + "/",
                                              notify_on_favorite_changed);
          notify_map[uid] = notify_id;
        }
      catch (Error e)
        {
          warning ("Unable to monitor gconf for favorite changes: %s",
                   e.message);
        }
    }

    private void on_favorite_removed (string uid)
    {
      uint notify_id = notify_map[uid];
      client.notify_remove (notify_id);
      notify_map.unset (uid);

      fav_ids.remove_all (uid);
    }

    private void notify_on_favorite_changed (GConf.Client _client, uint cnxn_id, GConf.Entry entry)
    {
      // we need to figure out our uid
      // we expect /desktop/unity/launcher/favorites/uid/foobar so we just use splits
      // regex's shouldn't be needed unless we do crazy stuff

      string uid = entry.key.split (path, 2)[1].split ("/", 2)[0]; //should split up nicely :)
      if (is_favorite (uid))
        {
          favorite_changed (uid);
        }
      else
        {
          warning ("got strange uid %s: %s", uid, entry.key);
          return;
        }
    }
  }

}
