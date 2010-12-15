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

namespace Unity.Tests.Unit
{
  public class IOSuite
  {
    /* Copy of the data found in data/test_desktop_file.deskop */
    static string TEST_DATA = """[Desktop Entry]
Name=Test Application
Comment=This is a test application desktop file
Exec=xclock
Terminal=false
Type=Application
StartupNotify=false
Icon=test_desktop_icon.png
""";
  
    public IOSuite ()
    {
      Test.add_data_func ("/Unit/IO/AsyncDektopFile",
                          IOSuite.test_async_find_and_load);
    }

    internal static void test_async_find_and_load()
    {
      MainLoop mainloop = new MainLoop();
      do_test_async_find_and_load.begin(mainloop);
      mainloop.run ();
    }
    
    internal static async void do_test_async_find_and_load (MainLoop mainloop)
    {
      string[] dirs = new string[1];
      dirs[0] = Path.build_filename (Config.TESTUNITDIR, "data", null
      );
      try
        {
          var input = yield IO.open_from_dirs ("test_desktop_file.desktop",
                                               dirs);
          assert (input is FileInputStream);
          
          /* Read in small chunks to test reading across buffer pages */
          uchar[] buf = new uchar[16];
          void* data;
          size_t data_size;
          yield IO.read_stream_async (input, buf, 16, Priority.DEFAULT,  null,
                                      out data, out data_size);
          
          /* The test file is 177 bytes long */
          assert (data_size == 177);
          
          /* null terminate the data so we can use it string comparison */
          string sdata = ((string)data).ndup(data_size);
          assert (sdata == TEST_DATA);
        }
      catch (Error e)
        {
          error ("Failed read test file: %s", e.message);
        }
      
      mainloop.quit ();
    }
        
  }
}
