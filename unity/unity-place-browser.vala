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

using Gee;

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
  internal interface BrowserService : GLib.Object
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
   * Manages the browsing state of a place entry. The browsing state is stored
   * as a generic type so this class can address a range of browsing contexts.
   *
   * You can monitor for back/forward navigation by listening to the
   * appropriate signals on the browser instance. When navigating into a
   * new state you should record the state in the Browser object by calling
   * browser.record_state(). Note that you should not record the state when
   * changing state because of back/forward signals. This is done automatically
   * by the browser instance.
   *
   * To actually expose this browsing mechanism for a place entry
   * set the 'browser' property of the EntryInfo instance to this and when
   * browsing is disabled entry_info.browser = null;.
   */
  public class Browser<E> : GLib.Object
  {
    private BrowserServiceImpl service;
    private Stack<State<E>> back_stack;
    private Stack<State<E>> forward_stack;
    
    public string dbus_path { get; private set; }
    
    public signal void back (E state, string comment);
    public signal void forward (E state, string comment);
    
    public Browser (string dbus_path)
    {
      this.dbus_path = dbus_path;
      service = new BrowserServiceImpl (dbus_path);
      back_stack = new Stack<State<E>> ();
      forward_stack = new Stack<State<E>> ();
      
      /* Relay signals from backend service */
      service.back.connect (on_back);
      service.forward.connect (on_forward);
      
      update_service_state ();
    }
    
    /**
     * Use this method to track a state (and associated comment) in
     * the back/forward stacks
     */
    public void record_state (E state, string comment)
    {
      var s = new State<E>();
      s.state = state;
      s.comment = comment;      
      back_stack.push (s);
      update_service_state ();
    }
    
    public void clear ()
    {
      back_stack.clear ();
      forward_stack.clear ();
    }
    
    private void on_back ()
    {
      var state = back_stack.pop();
      if (state != null)
        {
          forward_stack.push (state);
          update_service_state ();
          this.back (state.state, state.comment);
        }
    }
    
    private void on_forward ()
    {
      var state = forward_stack.pop();
      if (state != null)
        {
          back_stack.push (state);
          update_service_state ();
          this.forward (state.state, state.comment);
        }
    }
    
    private void update_service_state ()
    {
      service.browsing_state[BrowsingOp.BACK].sensitive =
                                                        !back_stack.is_empty ();
      service.browsing_state[BrowsingOp.BACK].comment =
                         back_stack.is_empty() ? "" : back_stack.peek().comment;
                 
      service.browsing_state[BrowsingOp.FORWARD].sensitive =
                                                     !forward_stack.is_empty ();
      service.browsing_state[BrowsingOp.FORWARD].comment =
                   forward_stack.is_empty() ? "" : forward_stack.peek().comment;
    }
    
    internal BrowserService get_service ()
    {
      return service;
    }
    
  } /* End: Browser class */
  
  /* Helper class to encapsulate a generic state and a comment string */
  private class State<E>
  {
    public E state = null;
    public string? comment = null;
  }
  
  /**
   * Private shim class to have a propor stack api
   */
  private class Stack<E>
  {
    private LinkedList<E> list;
    
    public Stack ()
    {
      list = new LinkedList<E>();
    }
    
    public Stack<E> push (E element)
    {
      list.offer_head (element);
      return this;
    }
    
    public E pop ()
    {
      return list.poll_head ();
    }
    
    public E peek ()
    {
      return list.peek_head ();
    }
    
    public int size ()
    {
      return list.size;
    }
    
    public void clear ()
    {
      list.clear ();
    }
    
    public bool is_empty ()
    {
      return size() == 0;
    }
  }

} /* namespace */
