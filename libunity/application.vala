/*
 * Copyright (C) 2009 Canonical Ltd
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
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

namespace Unity
{
  public enum ApplicationCommands
  {
    SHOW = 1
  }

  public class Application : Unique.App
  {
    private Shell? _shell;
    public  Shell? shell {
      get { return _shell; }
      set { _shell = value; }
    }

    public Application ()
    {
      Object (name: "com.canonical.Unity");
    }

    construct
    {
      this.add_command ("show", ApplicationCommands.SHOW);

      this.message_received.connect (this.on_message_received);
    }

    public Unique.Response on_message_received (int                command,
                                                Unique.MessageData data,
                                                uint               time_)
    {
      Unique.Response res = Unique.Response.OK;

      debug ("Message Received: %d '%s' %d",
             command,
             data.get_text (),
             (int)time_);

      if (command == ApplicationCommands.SHOW)
        {
          if (this.shell is Shell)
            this.shell.show_unity ();
        }

      return res;
    }
  }
}
