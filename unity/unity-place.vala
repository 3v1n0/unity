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
 * Authored by Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 *
 */

/*
 * IMPLEMENTATION NOTE:
 * It may appear this code it is bit more bloated than it needs to be
 * (considering the pure number of classes and indirections), but this has
 * some good reasons.
 *
 * Firstly we want to hide away Vala's internal DBus marshalling which would
 * expose raw structs in the API. These structs are hidden away in _RendererInfo,
 * and _EntryInfo. We wrap these in handy GObjects with properties and what not.
 * In fact we want to hide all DBusisms, which is also why the DBus interfaces
 * are declared private.
 *
 * Secondly we want the generatedd C API to be nice and not too Vala-ish. We
 * must anticipate that place daemons consuming libunity will be written in
 * both Vala and C.
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
    
    /* We use a pointer to the _RendererInfo to avoid Vala duping the struct
     * values. The actual struct pointed to is owned by the parent EntryInfo */
    private _RendererInfo* info;
    private Dee.Model _groups_model;
    private Dee.Model _results_model;

    /*
     * Properties
     */
    public string default_renderer {
      get { return info->default_renderer; }
      set { info->default_renderer = value; }
    }

    public Dee.Model groups_model {
      get { return _groups_model; }
      set {
        _groups_model = value;
        if (value is Dee.SharedModel)
        {
          Dee.SharedModel model = value as Dee.SharedModel;
          info->groups_model = model.get_swarm_name();
        }
        else
          info->groups_model = "__local__";
      }
    }

    public Dee.Model results_model {
      get { return _results_model; }
      set {
        _results_model = value;
        if (value is Dee.SharedModel)
        {
          Dee.SharedModel model = value as Dee.SharedModel;
          info->results_model = model.get_swarm_name();
        }
        else
          info->results_model = "__local__";
      }
    }

    /*
     * Constructors
     */

    internal RendererInfo (_RendererInfo* info)
    {
      this.info = info;
    }

    /*
     * Public API
     */

    public void set_hint (string hint, string val)
    {
      info->hints.insert (hint, val);
    }

    public string? get_hint (string hint)
    {
      return info->hints.lookup (hint);
    }

    public void clear_hint (string hint)
    {
      info->hints.remove (hint);
    }

    public void clear_hints ()
    {
      info->hints.remove_all ();
    }

    public uint num_hints ()
    {
      return info->hints.size ();
    }
    
    internal _RendererInfo get_raw ()
    {
      return *info;
    }
  }

  /**
   *
   */
  public class Search : InitiallyUnowned {

    private string search;
    private HashTable<string,string> hints;

    /*
     * Public API
     */

    public Search (string search, HashTable<string,string> hints)
    {
      GLib.Object();
      this.search = search;
      this.hints = hints;
    }

    public string get_search_string ()
    {
      return search;
    }

    public void set_hint (string hint, string val)
    {
      hints.insert (hint, val);
    }

    public string? get_hint (string hint)
    {
      return hints.lookup (hint);
    }

    public void clear_hint (string hint)
    {
      hints.remove (hint);
    }

    public void clear_hints ()
    {
      hints.remove_all ();
    }

    public uint num_hints ()
    {
      return hints.size ();
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

  /**
   * UnityPlace_EntryInfoData:
   *
   * Private helper struct used for marshalling the EntryInfo data without
   * the RenderingInfo data over the bus
   */
  private struct _EntryInfoData {
  	public string   dbus_path;
    public string   display_name;
    public string   icon;
    public uint     position;
    public string[] mimetypes;
    public bool     sensitive;
    public string   sections_model;
    public HashTable<string,string> hints;
  }

  public class EntryInfo : GLib.Object {

    /* The _EntryInfo needs to be set before we set properties, so it's
     * paramount we do it here */
    private _EntryInfo info = _EntryInfo();
    private RendererInfo _entry_renderer_info;
    private RendererInfo _global_renderer_info;
    private Dee.Model _sections_model;
    private bool _active = false;
    private uint _active_section = 0;

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

    public bool active {
      get { return _active; }
      set { _active = value; }
    }

    public uint active_section {
      get { return _active_section; }
      set { _active_section = value; }
    }

    public Search active_search { get; set; }
    public Search active_global_search { get; set; }

    /* The browser will automagically be exported/unexported on the bus
     * when this property is set or unset. Setting the browser also sets the
     * UnityPlaceBrowserPath and UnitySectionStyle hints accordingly. */
    private Browser? _browser = null;
    public Browser? browser {
      get { return _browser; }
      set {
        _browser = value;
        if (value != null)
          set_hint ("UnityPlaceBrowserPath", value.dbus_path);
          set_hint ("UnitySectionStyle", "breadcrumb");
        else
          clear_hint ("UnityPlaceBrowserPath");
          clear_hint ("UnitySectionStyle");
      }
    }

    /*
     * Constructors
     */

    construct {
      if (info.dbus_path == null)
        {
          critical ("""No DBus path set for EntryInfo.
'dbus-path' property in the UnityPlaceEntryInfo constructor""");
          info.dbus_path = "";
        }
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

      /* Pass in the address of the _RendererInfos as they are embedded
       * by value inside the _EntryInfo struct. The RendererInfo will then
       * only access the pointer to the struct to avoid memory duping which
       * would give incorrect return values for our DBus methods */
      _entry_renderer_info = new RendererInfo (&(info.entry_renderer_info));
      _global_renderer_info = new RendererInfo (&(info.global_renderer_info));
    }

    public EntryInfo (string dbus_path) {
      /* We need the _empty hack here to avoid a bug in valac, otherwise
       * valac will set it to NULL somehow and cause a g_critical in
       * g_strv_length() */
      var _empty = new string[0];
      GLib.Object(dbus_path : dbus_path,
                  mimetypes : _empty);
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

    public uint num_hints ()
    {
      return info.hints.size ();
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
    public abstract void set_global_search (string search,
                                            HashTable<string,string> hints) throws DBus.Error;

    public abstract void set_search (string search,
                                     HashTable<string,string> hints) throws DBus.Error;
    
    public abstract void set_active (bool is_active) throws DBus.Error;

    public abstract void set_active_section (uint section_id) throws DBus.Error;

    public signal void entry_renderer_info_changed (_RendererInfo renderer_info);
    
    public signal void global_renderer_info_changed (_RendererInfo renderer_info);
    
    public signal void place_entry_info_changed (_EntryInfoData entry_info_data);
  }

  /**
   * UnityPlaceServiceImpl:
   *
   * Private helper class to shield of DBus details and ugly
   * internal structs used for marshalling
   */
  private class ServiceImpl : GLib.Object, Service, Activation
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
    
    public Activation? activation { get; set; default = null; }

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

    public EntryServiceImpl? get_entry_service (string dbus_path)
    {
      var entry = entries.lookup (dbus_path);
      if (entry != null)
        return entry;
      else
        return null;
    }

    public uint num_entries ()
    {
      return entries.size ();
    }

    public string[] get_entry_paths ()
    {
      string[] result = new string[entries.size ()];

      int i = 0;
      foreach (var entry in entries.get_values ())
      {
        result[i] = entry.entry_info.dbus_path;
        i++;
      }

      return result;
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
    
    /* Default impl of the Unity.Place.Activation interface,
     * delegates to the installed Activation impl if there is one */
    public async bool activate (string uri)
    {
      if (activation == null )
        return false;
      
      try {
        bool activated = yield activation.activate (uri);
        return activated;
      } catch (DBus.Error e) {
        return false;
      }
    }
    
  } /* End: ServiceImpl */

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
    
    /* Keep a ref to the previous browser to properly handle it leaving and
     * entering the bus */
    private Browser? _browser;

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
      _browser = entry_info.browser;
      
      entry_info.notify["browser"].connect (on_browser_changed);
    }

    /*
     * DBus API
     */

    public void set_global_search (string search,
                                   HashTable<string,string> hints)
    {
      this._entry_info.active_global_search = new Search (search, hints);
    }

    public void set_search (string search,
                            HashTable<string,string> hints)
    {
      this._entry_info.active_search = new Search (search, hints);
    }

    public void set_active (bool is_active)
    {
      this._entry_info.active = is_active;
    }

    public void set_active_section (uint section_id)
    {
      this._entry_info.active_section = section_id;
    }

    /*
     * Internal API
     */

    public void export () throws DBus.Error
    {
      var conn = DBus.Bus.get (DBus.BusType. SESSION);
      conn.register_object (_entry_info.dbus_path, this);

      if (_entry_info.browser != null)
        {
          debug ("Exporting browser at '%s'", _entry_info.browser.dbus_path);
          conn.register_object (_entry_info.browser.dbus_path,
                                _entry_info.browser.get_service ());

        }
      else
        debug ("No browser to export");
      
      _exported = true;
      notify_property("exported");
    }

    public void unexport () throws DBus.Error
    {
      var conn = DBus.Bus.get (DBus.BusType. SESSION);
      conn.unregister_object (this);

      if (_entry_info.browser != null)
        {
          debug ("Unexporting browser '%s'", _entry_info.browser.dbus_path);
          conn.unregister_object (_entry_info.browser.get_service ());
        }

      _exported = false;
      notify_property("exported");
    }
    
    private void on_browser_changed (GLib.Object obj, ParamSpec pspec)
    {
      DBus.Connection conn;
      try {
        conn = DBus.Bus.get (DBus.BusType. SESSION);
      } catch (DBus.Error e) {
        warning ("Unable to connect to session bus: %s", e.message);
        return;
      }

      /* clear previous browser if any */
      if (_browser != null && _exported)
        {
          debug ("Unexporting browser '%s'", _browser.dbus_path);
          conn.unregister_object (_browser.get_service ());
        }

      _browser = entry_info.browser;

      /* Export the new one if any */
      if (_browser != null && _exported)
        {
          debug ("Exporting browser '%s'", _browser.dbus_path);
          conn.register_object (_browser.dbus_path, _browser.get_service ());
        }
    }
    
  } /* End: EntryServiceImpl */

  /*
   * Private helper struct used to keep track of the installed signal handlers
   * for an entry added to a Controller
   */
  internal struct _EntrySignals
  {
    ulong place_entry_info_changed_id;
    ulong entry_renderer_info_changed_id;
    ulong global_renderer_info_changed_id;
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
    private HashTable<string, _EntrySignals?> entry_signals;

    /*
     * Properties
     */

    [Property(nick = "DBus object path", blurb = "The DBus path this object is exported under")]
    public string dbus_path {
      get { return _dbus_path; }
      construct { _dbus_path = value; }
    }

    /* Whether or not this controller exports its installed entries and
     * activation service on the bus */
    public bool exported {
      get { return _exported; }
    }
    
    /* Defer URI activation to this instance. If there is no activation set
     * for the controller it will always decline any request for activation */
    public Activation? activation {
      get { return service.activation; }
      set { service.activation = value; }
    }

    /*
     * Constructors
     */

    construct {
      service = new ServiceImpl (_dbus_path);
      entry_signals = new HashTable<string, _EntrySignals?>(str_hash, str_equal);
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
      
      var signals = _EntrySignals ();
      
      signals.place_entry_info_changed_id = entry.notify.connect (on_entry_changed);
      
      /* We use a closure here because we need to capture the entry in order
       * to look up the EntryServiceImpl */
      signals.entry_renderer_info_changed_id = entry.entry_renderer_info.notify.connect (
        (obj, pspec) => {
          var renderer_info = (obj as RendererInfo);
          var entry_service = service.get_entry_service (entry.dbus_path);
          if (entry_service == null)
            {
              warning ("Entry renderer info changed for unknown entry '%s'", entry.dbus_path);
            }
          else
            entry_service.entry_renderer_info_changed (renderer_info.get_raw ());
          
        }
      );
      
      /* We use a closure here because we need to capture the entry in order
       * to look up the EntryServiceImpl */
      signals.global_renderer_info_changed_id = entry.global_renderer_info.notify.connect (
        (obj, pspec) => {
          var renderer_info = (obj as RendererInfo);
          var entry_service = service.get_entry_service (entry.dbus_path);
          if (entry_service == null)
            {
              warning ("Global renderer info changed for unknown entry '%s'", entry.dbus_path);
            }
          else
            entry_service.global_renderer_info_changed (renderer_info.get_raw ());
          
        }
      );
      
      /* Store all signal handler ids so we can remove them later */
      entry_signals.insert (entry.dbus_path, signals);
    }

    public EntryInfo? get_entry (string dbus_path)
    {
      return service.get_entry (dbus_path);
    }

    public void remove_entry (string dbus_path)
    {
      /* Disconnect all signals on the entry before we remove
       * it from the ServiceImpl */
      var signals = entry_signals.lookup (dbus_path);
      if (signals == null)
        {
          warning ("No signals connected for unknown entry '%s'",
                   dbus_path);
          service.remove_entry (dbus_path);
          return;
        }
      
      var entry = service.get_entry (dbus_path);
      if (entry == null)
        {
          warning ("Can not disconnect signals for unknown entry '%s'",
                   dbus_path);
          entry_signals.remove (dbus_path);
          return;
        }
      
      entry.disconnect (signals.place_entry_info_changed_id);
      entry.entry_renderer_info.disconnect (signals.entry_renderer_info_changed_id);
      entry.global_renderer_info.disconnect (signals.global_renderer_info_changed_id);
      
      service.remove_entry (dbus_path);
    }

    public uint num_entries ()
    {
      return service.num_entries ();
    }

    public string[] get_entry_paths ()
    {
      return service.get_entry_paths ();
    }

    public EntryInfo[] get_entries ()
    {
      uint len = num_entries();
      var result = new EntryInfo[len];
      var entry_paths = get_entry_paths ();

      int i = 0;
      for (i = 0; i < len; i++)
        {
          result[i] = get_entry (entry_paths[i]);
        }

      return result;
    }

    public void export () throws DBus.Error
    {
      /* Export the Place and Activation insterfaces */
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
    
    /* Callback for when an entry property changes */
    private void on_entry_changed (GLib.Object obj, ParamSpec psec)
    {
      var entry = (obj as EntryInfo);
      var entry_data = _EntryInfoData();      
      var entry_service = service.get_entry_service (entry.dbus_path);
      
      if (entry_service == null)
        {
          warning ("Got change signal from unknown entry service '%s'",
                   entry.dbus_path);
          return;
        }
      
      var _entry = entry.get_raw ();
      entry_data.dbus_path = _entry.dbus_path;
      entry_data.display_name = _entry.display_name;
      entry_data.icon = _entry.icon;
      entry_data.position = _entry.position;
      entry_data.mimetypes = _entry.mimetypes;
      entry_data.sensitive = _entry.sensitive;  
      entry_data.sections_model = _entry.sections_model;    
      entry_data.hints = _entry.hints;      
    
      entry_service.place_entry_info_changed (entry_data);
    }
    
  } /* End: Controller class */

} /* namespace */
