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
using Unity.Webapp;

namespace Unity
{
  public enum ApplicationCommands
  {
    SHOW = 1,
    MAKE_WEBAPP
  }

  public class Application : Unique.App
  {
    private Shell? _shell;
    public  Shell? shell {
      get { return _shell; }
      set { _shell = value; }
    }
    private WebiconFetcher webicon_fetcher;

    public Application ()
    {
      Object (name: "com.canonical.Unity");
    }

    construct
    {
      this.add_command ("show", ApplicationCommands.SHOW);
      this.add_command ("make webapp", ApplicationCommands.MAKE_WEBAPP);

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

      switch (command)
        {
          case ApplicationCommands.SHOW:
            {
              if (this.shell is Shell)
                this.shell.show_unity ();
            }
            break;
          case ApplicationCommands.MAKE_WEBAPP:
            {
               //we have a http url, webapp it
              var uri = data.get_text ();
              var icon_dirstring = Environment.get_home_dir () + "/.local/share/icons/";
              var icon_directory = File.new_for_path (icon_dirstring);
              try {
                if (!icon_directory.query_exists (null))
                  {
                    icon_directory.make_directory_with_parents (null);
                  }
              } catch (Error e) {
                // do nothing, errors are fine :)
              }
              var split_url = uri.split ("://", 2);
              var name = split_url[1];
              // gotta sanitze our name
              try {
                var regex = new Regex ("(/)");
                name = regex.replace (name, -1, 0, "");
              } catch (RegexError e) {
                warning ("%s", e.message);
              }

              this.webicon_fetcher = new WebiconFetcher (uri, icon_dirstring + name + ".svg");
              this.webicon_fetcher.fetch_webapp_data ();

              var webapp = new ChromiumWebApp (uri, icon_dirstring + name + ".svg");
              webapp.add_to_favorites ();

            }
            break;
          default:
            break;
        }

      return res;
    }
  }
}
