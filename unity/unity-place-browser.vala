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
 * We want the generatedd C API to be nice and not too Vala-ish. We must
 * anticipate that place daemons consuming libunity will be written in
 * both Vala and C.
 *
 */
namespace Unity.Place {

  /* Represents the state of a browsing element, so as the back button or
   * forward button. */
  private struct _BrowsingState
  {
    bool sensitive;
    string comment;
  }

  /* Offsets into the array of _BrowsingStates */
  private enum BrowsingOp
  {
    BACK,
    FORWARD,
    NUM_OPS
  }

  /**
   * Private DBus interface for managing browsing states
   */
  [DBus (name = "com.canonical.Unity.PlaceBrowser")]
  private interface BrowserService : GLib.Object
  {
    /**
     * Returns the new browsing state after the operation has been performed
     */
    public abstract _BrowsingState[] go_back () throws DBus.Error;
    
    /**
     * Returns the new browsing state after the operation has been performed
     */
    public abstract _BrowsingState[] go_forward () throws DBus.Error;
    
    /**
     * Returns the current browsing state
     */
    public abstract _BrowsingState[] get_state () throws DBus.Error;
  }
  
  /**
   * Helper class to shield away the ugly DBus wire format
   */
  private class BrowserServiceImpl : GLib.Object, BrowserService
  {
    public string dbus_path { get; private set; }
    public _BrowsingState[] browsing_state { get; private set; }
    
    public signal void back ();
    public signal void forward ();
    
    public BrowserServiceImpl (string dbus_path)
    {
      this.dbus_path = dbus_path;
      browsing_state = new _BrowsingState[BrowsingOp.NUM_OPS];
    }
  
    public _BrowsingState[] go_back ()
    {
      back ();
      return browsing_state;
    }
    
    public _BrowsingState[] go_forward ()
    {
      forward ();
      return browsing_state;
    }
    
    public _BrowsingState[] get_state ()
    {
      return browsing_state;
    }
  }
  
  /**
   * Manages the browsing state of a place entry.
   *
   * You can monitor for back/forward navigation by listening to the
   * appropriate signals on the browser instance. In the signal handlers
   * you should update the properties of the browser object to reflect
   * the browsing state after the operation has been carried out.
   *
   * To control the state that a consuming UI sees set the properties
   * on this object accordingly.
   *
   * To actually expose this browsing mechanism for a place entry
   * set the 'browser' property of the EntryInfo instance to this and when
   * browsing is disabled entry_info.browser = null;.
   */
  public class Browser : GLib.Object
  {
    private BrowserServiceImpl service;
    
    public string dbus_path { get; private set; }
    
    public bool can_go_back {
      get { return service.browsing_state[BrowsingOp.BACK].sensitive; }
      set { service.browsing_state[BrowsingOp.BACK].sensitive = value; }
    }
    
    public bool can_go_forward {
      get { return service.browsing_state[BrowsingOp.FORWARD].sensitive; }
      set { service.browsing_state[BrowsingOp.FORWARD].sensitive = value; }
    }
    
    public string go_back_comment {
      get { return service.browsing_state[BrowsingOp.BACK].comment; }
      set { service.browsing_state[BrowsingOp.BACK].comment = value; }
    }
    
    public string go_forward_comment {
      get { return service.browsing_state[BrowsingOp.FORWARD].comment; }
      set { service.browsing_state[BrowsingOp.FORWARD].comment = value; }
    }
    
    public signal void back ();
    public signal void forward ();
    
    public Browser (string dbus_path)
    {
      this.dbus_path = dbus_path;
      service = new BrowserServiceImpl (dbus_path);
      
      /* Relay signals from backend service */
      service.back.connect ( () => { this.back (); } );
      service.forward.connect ( () => { this.forward (); } );
    }
  }
  
  

} /* namespace */
