/* -*- Mode: vala; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
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
using Unity;
using Unity.Place;
using Unity.Testing;
using Gee;

namespace Unity.Tests.Unit
{
  public class PlaceBrowserSuite
  {
    public PlaceBrowserSuite ()
    {
      Test.add_data_func ("/Unit/Place/Stack",
                          PlaceBrowserSuite.test_stack);
      Test.add_data_func ("/Unit/Place/Browser",
                          PlaceBrowserSuite.test_browser);
    }

    internal static void test_stack()
    {
      /* Test the empty stack */
      var stack = new Stack<string> ();
      assert (stack.is_empty ());
      assert (stack.size () == 0);
      assert (stack.pop () == null);
      
      /* Test with one element */
      stack.push ("foo");
      assert (!stack.is_empty ());
      assert (stack.size () == 1);
      assert ("foo" == stack.pop ());
      assert (stack.is_empty ());
      assert (stack.size () == 0);
      assert (stack.pop () == null);
      
      /* Test with two elements */
      stack.push ("bar");
      assert (!stack.is_empty ());
      assert (stack.size () == 1);
      stack.push ("baz");
      assert (!stack.is_empty ());
      assert (stack.size () == 2);
      assert ("baz" == stack.pop ());
      assert (!stack.is_empty ());
      assert (stack.size () == 1);
      assert ("bar" == stack.pop ());
      assert (stack.is_empty ());
      assert (stack.size () == 0);
      assert (stack.pop () == null);
    }
    
    internal static void test_browser ()
    {
      Gee.List<string> playback = new ArrayList<string> ();
      Browser<string> browser = new Browser<string> ("/org/example/browser/1");
      
      assert (browser.dbus_path == "/org/example/browser/1");
      
      browser.back.connect ( (b, state, comment) => {
        playback.add ((string)state);
      });
      
      browser.forward.connect ( (b, state, comment) => {
        playback.add ((string)state);
      });
      
      browser.record_state ("First", "Comment1");
      browser.record_state ("Second", "Comment2");
      browser.record_state ("Third", "Comment3");
      browser.record_state ("Fourth", "Comment4");
      
      browser.go_back ();
      assert (playback[playback.size - 1] == "Third");
      
      browser.go_back ();
      assert (playback[playback.size - 1] == "Second");
      
      browser.record_state ("Fifth", "Comment5");
      browser.record_state ("Sixth", "Comment6");
      browser.record_state ("Seven", "Comment7");
      
      browser.go_back ();
      assert (playback[playback.size - 1] == "Sixth");
      
      browser.go_back ();
      assert (playback[playback.size - 1] == "Fifth");
      
      browser.go_back ();      
      assert (playback[playback.size - 1] == "Second");
      
      browser.go_back ();
      var last_size = playback.size;
      assert (playback[playback.size - 1] == "First");
      assert (playback.size == last_size);
      
      browser.go_back (); // no-op      
      assert (playback[playback.size - 1] == "First");
      assert (playback.size == last_size);
      
      browser.go_forward ();
      assert (playback[playback.size - 1] == "Second");
      
      browser.go_forward ();
      assert (playback[playback.size - 1] == "Fifth");
      
      browser.go_forward ();
      assert (playback[playback.size - 1] == "Sixth");
    }

  }
}
