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
  
  /**
   * UnityPlace_RendererInfo:
   *
   * Private helper struct used for marshalling a RendererInfo object over DBus
   */
  private struct _RendererInfo {
  	public string default_renderer;
  	public string groups_model;
  	public string results_model;
  	public HashTable<string,string> hints;
  }
  
  /**
   * SECTION:unity-place-renderer-info 
   * @short_description: Encapsulates all place entry metadata the Unity shell needs in order to render this model
   * @include: unity.h
   *
   * 
   */
  public class RendererInfo : GLib.Object {
    
    private _RendererInfo info;
    private Dee.Model _groups_model;
    private Dee.Model _results_model;
    
    /*
     * Properties
     */
    public string default_renderer {
      get { return info.default_renderer; }
      set { info.default_renderer = value; }
    }
    
    public Dee.Model groups_model {
      get { return _groups_model; }
      set {
        _groups_model = value;
        if (value is Dee.SharedModel)
        {
          Dee.SharedModel model = value as Dee.SharedModel;
          info.groups_model = model.get_swarm_name();
        }
        else
          info.groups_model = "__local__";
      }
    }
    
    public Dee.Model results_model {
      get { return _results_model; }
      set {
        _results_model = value;
        if (value is Dee.SharedModel)
        {
          Dee.SharedModel model = value as Dee.SharedModel;
          info.results_model = model.get_swarm_name();
        }
        else
          info.results_model = "__local__";
      }
    }
    
    /*
     * Constructors
     */
    
    internal RendererInfo (_RendererInfo info)
    {
      this.info = info;
      info.hints = new HashTable<string,string>(str_hash, str_equal);
    }
    
    /*
     * Public API
     */
    
    public void set_hint (string hint, string val)
    {
      info.hints.insert (hint, val);
    }
    
    public string? get_hint (string hint)
    {
      return info.hints.lookup (hint);
    }
    
    public void clear_hint (string hint)
    {
      info.hints.remove (hint);
    }
    
    public void clear_hints ()
    {
      info.hints.remove_all ();
    }
  }
  
  /**
   * UnityPlace_EntryInfo:
   *
   * Private helper struct used for marshalling an EntryInfo object over DBus
   */
  private struct _EntryInfo {
  	public string   dbus_path;
    public string   display_name;
    public string   icon;
    public uint     position;
    public string[] mimetypes;
    public bool     sensitive;
    public string   sections_model;
    public HashTable<string,string> hints;
    public _RendererInfo entry_renderer_info;
    public _RendererInfo global_renderer_info;
  }
  
  public class EntryInfo : GLib.Object {
  
    /* The _EntryInfo needs to be set before we set properties, so it's
     * paramount we do it here */
    private _EntryInfo info = _EntryInfo();
    private RendererInfo _entry_renderer_info;
    private RendererInfo _global_renderer_info;
    private Dee.Model _sections_model;
    
    /*
     * Properties
     */
    public RendererInfo entry_renderer_info {
      get { return _entry_renderer_info; }
    }
    
    public RendererInfo global_renderer_info {
      get { return _global_renderer_info; }
    }
    
    public string dbus_path {
      get { return info.dbus_path; }
      construct set { info.dbus_path = value; }
    }
    
    public string display_name {
      get { return info.display_name; }
      construct set { info.display_name = value; }
    }
    
    public string icon {
      get { return info.icon; }
      construct set { info.icon = value; }
    }
    
    public uint position {
      get { return info.position; }
      construct set { info.position = value; }
    }
    
    public string[] mimetypes {
      get { return info.mimetypes; }
      construct set { info.mimetypes = value; }
    }
    
    public bool sensitive {
      get { return info.sensitive; }
      construct set { info.sensitive = value; }
    }
    
    public Dee.Model sections_model {
      get { return _sections_model; }
      construct set {
        _sections_model = value;
        if (value is Dee.SharedModel)
        {
          Dee.SharedModel model = value as Dee.SharedModel;
          info.sections_model = model.get_swarm_name();
        }
        else
          info.sections_model = "__local__";
      }
    }
    
    /*
     * Constructors
     */
     
    construct {
      _entry_renderer_info = new RendererInfo (info.entry_renderer_info);
      _global_renderer_info = new RendererInfo (info.global_renderer_info);
      info.hints = new HashTable<string,string>(str_hash, str_equal);
    }
    
    public EntryInfo () {
      /* Construct does all we need */
    }
    
    /*
     * Public API
     */
    
    public void set_hint (string hint, string val)
    {
      info.hints.insert (hint, val);
    }
    
    public string? get_hint (string hint)
    {
      return info.hints.lookup (hint);
    }
    
    public void clear_hint (string hint)
    {
      info.hints.remove (hint);
    }
    
    public void clear_hints ()
    {
      info.hints.remove_all ();
    }
    
    /*
     * Internal API
     */
     internal _EntryInfo get_raw () 
     {
       return info;
     }
  }
  
  /**
   * UnityPlaceService:
   *
   * DBus interface exported by a place daemon
   */
  [DBus (name = "com.canonical.Unity.Place")]
  private interface Service : GLib.Object
  {
    public abstract _EntryInfo[] get_entries () throws DBus.Error;

    public signal void entry_added (_EntryInfo entry);
    
    public signal void entry_removed (string entry_dbus_path);
  }
  
  /**
   * UnityPlaceEntryService:
   *
   * DBus interface for a given place entry exported by a place daemon
   */
  [DBus (name = "com.canonical.Unity.PlaceEntry")]
  private interface EntryService : GLib.Object
  {
    public abstract uint set_global_search (string search,
                                            HashTable<string,string> hints) throws DBus.Error;
    
    public abstract uint set_search (string search,
                                     HashTable<string,string> hints) throws DBus.Error;
    
    public abstract void set_active (bool is_active) throws DBus.Error;

    public abstract void set_active_section (uint section_id) throws DBus.Error;
    
    public signal void renderer_info_changed (_RendererInfo renderer_info);
  }
  
  /**
   * UnityServiceImpl:
   *
   * Helper class to shield of DBus details and ugly internal structs used
   * for marshalling
   */
  private class ServiceImpl : GLib.Object, Service
  {
    private HashTable<string,EntryInfo> entries;
    
    construct {
      entries = new HashTable<string,EntryInfo> (str_hash, str_equal);
    }
    
    public _EntryInfo[] get_entries () throws DBus.Error
    {
      _EntryInfo[] result = new _EntryInfo[entries.size ()];
      
      int i = 0;
      foreach (var entry in entries.get_values ())
      {
        result[i] = entry.get_raw();
        i++;
      }
      
      return result;
    }
    
    public void add_entry (EntryInfo entry)
    {
      if (entries.lookup (entry.dbus_path) != null)
        return;
      
      entries.insert (entry.dbus_path, entry);
      entry_added (entry.get_raw ());
      
      // FIXME: Export entry service
    }
    
    public EntryInfo? get_entry (string dbus_path)
    {
      return entries.lookup (dbus_path);
    }
    
    public void remove_entry (string dbus_path)
    {
      EntryInfo entry = entries.lookup (dbus_path);
      
      if (entry == null)
        return;
      
      entry_removed (entry.dbus_path); 
      
      // FIXME: Retract entry service
      
      entries.remove (dbus_path);
    }
  }
  
  /*public class Manager : GLib.Object
  {
    
  }*/
  
  /*public class TestPlaceDaemon : GLib.Object, PlaceService
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
   	
   	public _EntryInfo[] get_entries () throws DBus.Error
   	{
   	  var entry = _EntryInfo();
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
      
      _EntryInfo[] entries = new _EntryInfo[1];
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
  }*/
} /* namespace */
