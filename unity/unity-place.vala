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
 * Authored by Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 *
 */
using Dee;
using DBus;

namespace Unity.Place {  
  
  public struct RendererInfo {
  	public string default_renderer;
  	public string groups_model;
  	public string results_model;
  	public HashTable<string,string> hints;
  }
  
  public struct EntryInfo {
  	public string   dbus_path;
    public string   name;
    public string   icon;
    public uint     position;
    public string[] mimetypes;
    public bool     sensitive;
    public string   sections_model;
    public HashTable<string,string> hints;
    public RendererInfo entry_renderer_info;
    public RendererInfo global_renderer_info;
  }
  
  [DBus (name = "com.canonical.Unity.Place")]
  public interface PlaceService : GLib.Object
  {
    public abstract EntryInfo[] get_entries () throws DBus.Error;

    //public signal void view_changed (HashTable<string,string> properties);
  }
  
  public class TestPlaceDaemon : GLib.Object, PlaceService
  {
   	private static string[] supported_mimetypes = {"text/plain", "text/html"};	
   	
   	public TestPlaceDaemon()
   	{
   	  try {
        var conn = DBus.Bus.get (DBus.BusType. SESSION);

        dynamic DBus.Object bus = conn.get_object ("org.freedesktop.DBus",
                                                   "/org/freedesktop/DBus",
                                                   "org.freedesktop.DBus");
        
        // try to register service in session bus
        uint request_name_result = bus.request_name ("org.ayatana.TestPlace", (uint) 0);

        if (request_name_result == DBus.RequestNameReply.PRIMARY_OWNER) {
            conn.register_object ("/org/ayatana/places/test", this);            
        } else {        
          stderr.printf ("Unable to grab DBus name. Another test server is probably running\n");  
        }
      } catch (DBus.Error e) {
          stderr.printf ("Oops: %s\n", e.message);
      }
   	}
   	
   	public EntryInfo[] get_entries () throws DBus.Error
   	{
   	  var entry = EntryInfo();
      entry.dbus_path = "/org/foo/bar";
      entry.name = "My Entry Name";
      entry.icon = "__icon__";
      entry.position = 0;
      entry.mimetypes = supported_mimetypes;
      entry.sensitive = true;
      entry.sections_model = "org.ayatana.MySectionsModel";
      entry.hints = new HashTable<string, string> (str_hash, str_equal);
      entry.entry_renderer_info.default_renderer = "DefaultRenderer";
      entry.entry_renderer_info.groups_model = "org.ayatana.MyGroupsModel";
      entry.entry_renderer_info.results_model = "org.ayatana.MyResultsModel";
      entry.entry_renderer_info.hints = new HashTable<string, string> (str_hash, str_equal);
      entry.global_renderer_info.default_renderer = "DefaultRenderer";
      entry.global_renderer_info.groups_model = "org.ayatana.MyGlobalGroupsModel";
      entry.global_renderer_info.results_model = "org.ayatana.MyGlobalResultsModel";
      entry.global_renderer_info.hints = new HashTable<string, string> (str_hash, str_equal);
      
      EntryInfo[] entries = new EntryInfo[1];
      entries[0] = entry;
      return entries;
   	}
   	
    public static int main (string[] args)
    {
      var loop = new MainLoop (null, false);
      var daemon = new TestPlaceDaemon();
      loop.run ();
      
      return 0;
    }
  }
} /* namespace */
