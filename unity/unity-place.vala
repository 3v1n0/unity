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
      
      if (info.dbus_path == null)
        info.dbus_path = "";
      if (info.display_name == null)
        info.display_name = "";
      if (info.icon == null)
        info.icon = "";
      info.position = 0;
      if (info.mimetypes == null)
        info.mimetypes = new string[0];
      info.sensitive = true;
      if (info.sections_model == null)
        info.sections_model = "";
      info.hints = new HashTable<string,string>(str_hash, str_equal);
      
      info.entry_renderer_info.default_renderer = "";
      info.entry_renderer_info.groups_model = "";
      info.entry_renderer_info.results_model = "";
      info.entry_renderer_info.hints = new HashTable<string, string> (str_hash, str_equal);
      
      info.global_renderer_info.default_renderer = "";
      info.global_renderer_info.groups_model = "";
      info.global_renderer_info.results_model = "";
      info.global_renderer_info.hints = new HashTable<string, string> (str_hash, str_equal);
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
   * UnityPlaceServiceImpl:
   *
   * Private helper class to shield of DBus details and ugly
   * internal structs used for marshalling
   */
  private class ServiceImpl : GLib.Object, Service
  {
    private string _dbus_path;
    private HashTable<string,EntryServiceImpl> entries;
    private bool _exported;
    
    /*
     * Properties
     */
    
    [Property(nick = "DBus object path", blurb = "The DBus path this object is exported under")]
    public string dbus_path {
      get { return _dbus_path; }
      construct { _dbus_path = value; }
    }
    
    public bool exported {
      get { return _exported; }
    }
    
    /*
     * Constructors
     */
    
    construct {
      entries = new HashTable<string,EntryServiceImpl> (str_hash, str_equal);
    }
    
    public ServiceImpl (string dbus_path)
    {
      GLib.Object (dbus_path : dbus_path);
    }
    
    /*
     * DBus API
     */
    
    public _EntryInfo[] get_entries ()
    {
      _EntryInfo[] result = new _EntryInfo[entries.size ()];
      
      int i = 0;
      foreach (var entry in entries.get_values ())
      {
        result[i] = entry.entry_info.get_raw();
        debug("GET %i", i);
        i++;
      }
      
      return result;
    }
    
    /*
     * Internal API
     */
    
    public void add_entry (EntryInfo entry_info)
    {
      if (entries.lookup (entry_info.dbus_path) != null)
        return;
      
      var entry = new EntryServiceImpl(entry_info);
      entries.insert (entry_info.dbus_path, entry);      
      if (_exported)
        {
          try {
            entry.export ();
          } catch (DBus.Error e) {
            critical ("Failed to export place entry '%s': %s",
                      entry_info.dbus_path, e.message);
          }
        }
      entry_added (entry_info.get_raw ());
    }
    
    public EntryInfo? get_entry (string dbus_path)
    {
      var entry = entries.lookup (dbus_path);
      if (entry != null)
        return entry.entry_info;
      else
        return null;
    }
    
    public void remove_entry (string dbus_path)
    {
      var entry = entries.lookup (dbus_path);
      
      if (entry == null)
        return;
      
      entry_removed (dbus_path); 
      if (_exported)
        {
          try {
            entry.unexport ();
          } catch (DBus.Error e) {
            critical ("Failed to unexport place entry '%s': %s",
                      entry.entry_info.dbus_path, e.message);
          }
        }
      
      entries.remove (dbus_path);
    }
    
    public void export () throws DBus.Error
    {
      var conn = DBus.Bus.get (DBus.BusType. SESSION);
      conn.register_object (_dbus_path, this);
      
      foreach (var entry in entries.get_values ())
      {
        entry.export ();
      }
      
      _exported = true;
      notify_property("exported");
    }
    
    public void unexport () throws DBus.Error
    {
      foreach (var entry in entries.get_values ())
      {
        entry.unexport ();
      }
    
      var conn = DBus.Bus.get (DBus.BusType. SESSION);
      conn.unregister_object (this);

      _exported = false;
      notify_property("exported");
    }
  }
  
  /**
   * UnityPlaceEntryServiceImpl:
   *
   * Private helper class to shield of DBus details and ugly
   * internal structs used for marshalling
   */
  private class EntryServiceImpl : GLib.Object, EntryService
  {
    private bool _exported = false;
    private EntryInfo _entry_info;
    
    /*
     * Properties
     */
    
    public EntryInfo entry_info {
      get { return _entry_info; }
      construct { _entry_info = value; }
    }
    
    public bool exported {
      get { return _exported; }
    }
    
    /*
     * Constructors
     */
    
    public EntryServiceImpl (EntryInfo entry_info)
    {
      GLib.Object(entry_info : entry_info);
    }
    
    /*
     * DBus API
     */

    public uint set_global_search (string search,
                                   HashTable<string,string> hints)
    {
      return 0;
    }

    public uint set_search (string search,
                            HashTable<string,string> hints)
    {
      return 0;
    }
    
    public void set_active (bool is_active)
    {
      // pass
    }

    public void set_active_section (uint section_id)
    {
      // pass
    }
    
    /*
     * Internal API
     */
    
    public void export () throws DBus.Error
    {
      var conn = DBus.Bus.get (DBus.BusType. SESSION);
      conn.register_object (_entry_info.dbus_path, this);
      
      _exported = true;
      notify_property("exported");
    }
    
    public void unexport () throws DBus.Error
    {
      var conn = DBus.Bus.get (DBus.BusType. SESSION);
      conn.unregister_object (this);
      
      _exported = false;
      notify_property("exported");
    }
  }
  
  /**
   * UnityPlaceController:
   *
   * Main handle for controlling the place entries managed by a place daemon
   */
  public class Controller : GLib.Object
  {
    private ServiceImpl service;
    private string _dbus_path;
    private bool _exported = false;
    private HashTable<string,EntryInfo> entries;
    
    /*
     * Properties
     */
    
    [Property(nick = "DBus object path", blurb = "The DBus path this object is exported under")]
    public string dbus_path {
      get { return _dbus_path; }
      construct { _dbus_path = value; }
    }
    
    public bool exported {
      get { return _exported; }
    }
    
    /*
     * Constructors
     */
    
    construct {
      service = new ServiceImpl (_dbus_path);
    }
    
    public Controller (string dbus_path)
    {
      GLib.Object (dbus_path : dbus_path);
    } 
    
    /*
     * Public API
     */
     
    public void add_entry (EntryInfo entry)
    {
      service.add_entry (entry);
    }
    
    public EntryInfo? get_entry (string dbus_path)
    {
      return service.get_entry (dbus_path);
    }
    
    public void remove_entry (string dbus_path)
    {
      service.remove_entry (dbus_path);
    }
    
    public void export () throws DBus.Error
    {
      service.export ();
      _exported = true;
      notify_property("exported");
    }
    
    public void unexport () throws DBus.Error
    {
      service.unexport ();      
      _exported = false;
      notify_property("exported");
    }
  }
  
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

  /*public static int main (string[] args)
    {
      var loop = new MainLoop (null, false);
      var ctl = new Controller ("/foo/bar");
      ctl.export();
      
      var entry = new EntryInfo();
      entry.dbus_path = "/foo/bar/entries/0";
      entry.display_name = "Display Name";
      entry.set_hint ("hintkey1", "hintvalue1");
      ctl.add_entry (entry);
      
      loop.run ();

      return 0;
    }*/
  
} /* namespace */
