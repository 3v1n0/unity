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
 * Authored by Mirco Müller <mirco.mueller@canonical.com>
 *
 */

using Unity;
using Unity.Testing;
using Unity.Panel;

bool g_flag = false;

namespace Unity.Tests.UI
{
  public class HomeButtonSuite : Object, Shell
  {
    private const string DOMAIN = "/UI/HomeButton";

    Unity.Testing.Window?  window;
    Clutter.Stage?         stage;
    Unity.Testing.Director director;
    Unity.Panel.HomeButton home_button;

    public HomeButtonSuite ()
    {
      Logging.init_fatal_handler ();

      /* Testup the test window */
      window = new Unity.Testing.Window (true, 1024, 600);
      window.init_test_mode ();
      stage = window.stage;
      window.title = "HomeButton Test";
      window.show_all ();

      home_button = new Unity.Panel.HomeButton (this);
      director = new Unity.Testing.Director (stage);

      Test.add_data_func (DOMAIN + "/HomeButton", test_click_home_button);
    }


    private void test_click_home_button ()
    {
      director.button_press (home_button, 1, true, 1.0f, 1.0f, false);

      Logging.init_fatal_handler ();

      assert (g_flag == true);
    }

    //
    // mostly a dummy shell-implementation to satisfy the interface
    //
    public bool menus_swallow_events { get { return true; } }

    public ShellMode
    get_mode ()
    {
      return ShellMode.MINIMIZED;
    }

    public Clutter.Stage
    get_stage ()
    {
      return this.stage;
    }

    public void
    show_unity ()
    {
      g_flag = true;
    }

    public void
    hide_unity ()
    {

    }

    public int
    get_indicators_width ()
    {
      // stub
      return 0;
    }

    public int
    get_launcher_width_foobar ()
    {
      // stub
      return 0;
    }

    public int
    get_panel_height_foobar ()
    {
      // stub
      return 0;
    }

    public void
    ensure_input_region ()
    {
      // stub
    }

    public void
    add_fullscreen_request (Object o)
    {
      // stub
    }

    public bool
    remove_fullscreen_request (Object o)
    {
      // stub
      return false;
    }

    public void
    grab_keyboard (bool grab, uint32 timestamp)
    {
      // stub
    }

    public void
    about_to_show_places ()
    {
      // stub
    }

    public void
    close_xids (Array<uint32> xids)
    {
      // stub
    }

    public void
    show_window (uint32 xid)
    {
      // stub
    }

		public void
		expose_xids (Array<uint32> xids)
		{
      // stub
		}

		public void
		stop_expose ()
		{
      // stub
		}

    public uint32 get_current_time ()
    {
      return Clutter.get_current_event_time ();
    }
  }
}

