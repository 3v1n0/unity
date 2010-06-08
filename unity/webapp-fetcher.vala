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
 * Authored by Gordon Allott <gord.allott@canonical.com>
 */
static const string apple_string = "rel=\"apple-touch-icon\"";
static const string fav_string = "rel=\"(icon|SHORTCUT ICON|shortcut icon|ICON)\"";
static const string uri_match_string = "href=\"(?P<icon_uri>[^\"]*)\"";
static const string tag_start_string = "<link[^>]*";
static const string tag_end_string = "[^>]*/?>";
static const string hostname_string = """(s)?http://(?P<hostname>.*)(/|$)""";

namespace Unity.Webapp
{
  public static string urlify (string uri)
  {
    string return_string = Uri.unescape_string (uri);
    /* ooooh this is going to be fun run of try/catch statements. yay error
     * checking
     */
    try {
			var regex = new GLib.Regex ("""^[ \\]+|[ \\]+$"""); //removes trailing whitespace
			return_string = regex.replace (return_string, -1, 0, "_");
		} catch (GLib.RegexError e) {
			warning (e.message);
		}
    try {
			var regex = new GLib.Regex ("""^.*?://"""); //removes http:// stuff
			return_string = regex.replace (return_string, -1, 0, "");
		} catch (GLib.RegexError e) {
			warning (e.message);
		}
    try {
			var regex = new GLib.Regex ("""(\s|/)"""); //converts spaces and /to underscore
			return_string = regex.replace (return_string, -1, 0, "_");
		} catch (GLib.RegexError e) {
			warning (e.message);
		}
    try {
			var regex = new GLib.Regex ("""[^([:alnum:]|\.|_)]+"""); //removes unneeded chars
			return_string = regex.replace (return_string, -1, 0, "");
		} catch (GLib.RegexError e) {
			warning (e.message);
		}

    return return_string;
  }

  public class FetchFile : Object
  {
    /* public variables */
    public string uri {get; construct;}

    /* private variables */
    private DataInputStream stream;
    private File? file;
    private ByteArray data;

    /* public signals */
    public signal void failed ();
    public signal void completed (ByteArray data);

    public FetchFile (string uri)
    {
      Object (uri: uri);
    }

    construct
    {
      this.file = File.new_for_uri(this.uri);
      this.data = new ByteArray ();
    }

    public async void fetch_data ()
    {
      //grab our data from our uri
      try {
        this.stream = new DataInputStream(this.file.read(null));
        this.stream.set_byte_order (DataStreamByteOrder.LITTLE_ENDIAN);
      } catch (GLib.Error e) {
        this.failed ();
      }
      this.read_something_async ();
    }

    private async void read_something_async ()
    {
      ssize_t size = 1024;
      uint8[] buffer = new uint8[size];

      ssize_t bufsize = 1;
      do {
        try {
          bufsize = yield this.stream.read_async (buffer, size, GLib.Priority.DEFAULT, null);
          if (bufsize < 1) { break;}

          if (bufsize != size)
            {
              uint8[] cpybuf = new uint8[bufsize];
              Memory.copy (cpybuf, buffer, bufsize);
              this.data.append (cpybuf);
            }
          else
            {
              this.data.append (buffer);
            }
        } catch (Error e) {
          this.failed ();
        }
      } while (bufsize > 0);
      this.completed (this.data);
    }
  }

  private Regex? primary_match_prefix = null;
  private Regex? primary_match_suffix = null;
  private Regex? secondary_match_prefix = null;
  private Regex? secondary_match_suffix = null;
  private Regex? hostname_match = null;

  public static string get_hostname (string uri)
  {
    if (Unity.Webapp.hostname_match == null)
      {
        try {
          Unity.Webapp.hostname_match = new Regex (hostname_string, RegexCompileFlags.UNGREEDY);
        } catch (Error e) {
          warning (e.message);
        }
      }

    MatchInfo matchinfo;
    // try and extract a hostname
    var ismatch = hostname_match.match (uri, 0, out matchinfo);
    string hostname = "";
    if (ismatch) { hostname = matchinfo.fetch_named ("hostname"); }
    return hostname;
  }

  public class WebiconFetcher : Object
  {
    /* constant variables */

    /* public variables */
    public string uri {get; construct;}
    public string destination {get; construct;}
    public string desktop_location {get; construct;}

    /* private variables */
    private FetchFile fetcher;
    private bool html_phase = false;
    private bool icon_phase = false;

    private Gee.List<string> icon_uris;

    /* public signals */
    public signal void failed ();
    public signal void completed (string location);

    public WebiconFetcher (string uri, string destination, string desktop_file)
    {
      Object (uri: uri,
              destination: destination,
              desktop_location: desktop_file);
    }

    construct
    {
      // only build the regular expressions once
      if (Unity.Webapp.primary_match_prefix == null)
        {
          string primary_match_prefix = tag_start_string + apple_string + "[^>]*" + uri_match_string + tag_end_string;
          string primary_match_suffix = tag_start_string + uri_match_string + "[^>]*" + apple_string + tag_end_string;
          string secondary_match_prefix = tag_start_string + fav_string + "[^>]*" + uri_match_string + tag_end_string;
          string secondary_match_suffix = tag_start_string + uri_match_string + "[^>]*" + fav_string + tag_end_string;

          try {
            Unity.Webapp.primary_match_prefix = new Regex (primary_match_prefix, RegexCompileFlags.UNGREEDY);
            Unity.Webapp.primary_match_suffix = new Regex (primary_match_suffix, RegexCompileFlags.UNGREEDY);
            Unity.Webapp.secondary_match_prefix = new Regex (secondary_match_prefix, RegexCompileFlags.UNGREEDY);
            Unity.Webapp.secondary_match_suffix = new Regex (secondary_match_suffix, RegexCompileFlags.UNGREEDY);
            Unity.Webapp.hostname_match = new Regex (hostname_string, RegexCompileFlags.UNGREEDY);
          } catch (Error e) {
            warning (e.message);
          }
        }
      this.icon_uris = new Gee.ArrayList<string> ();
      // touch our destination now so that inofity can pick up changes
      try {
        var make_file = File.new_for_path (this.destination);
        make_file.create (FileCreateFlags.NONE, null);
      } catch (Error e) {
      }

      // set the icon name in the desktop file to the default webapp icon
      this.set_desktop_file_icon ("webapp-default-icon");
    }

    private void set_desktop_file_icon (string iconname)
    {
      try {
        var file = File.new_for_path (this.desktop_location);
        // open the desktop file
        var file_stream = file.replace (null, false, FileCreateFlags.NONE, null);

        var desktop_file = new KeyFile ();
        desktop_file.load_from_file (this.desktop_location, 0);
        desktop_file.set_string ("Desktop Entry", "Icon", iconname);
        var desktop_data = desktop_file.to_data (null);

        // Write text data to file
        var data_stream = new DataOutputStream (file_stream);
        data_stream.put_string (desktop_data, null);
        data_stream.close (null);
      } catch (Error e) {
        warning (e.message);
      }
    }

    public void fetch_webapp_data ()
    {
      // fetch our file
      this.fetcher = new FetchFile (uri);
      this.fetcher.failed.connect (() => { this.failed ();});
      this.fetcher.completed.connect (this.on_fetcher_completed);
      this.html_phase = true;
      this.fetcher.fetch_data ();
    }

    private void on_fetcher_completed (ByteArray data)
    {

      if (this.html_phase)
        {
          debug ("we got the html");
          //we just completed getting our html
          this.html_phase = false;
          string html = (string)(data.data);
          string hostname = get_hostname (this.uri);
          // we have our html, try and get an icon from it
          this.icon_uris.add ("http://" + hostname + "/ubuntu-launcher.png");
          var primary_icons = this.extract_icon_from_html (html, true);
          foreach (string uri in primary_icons)
            {
              this.icon_uris.add (uri);
            }
          this.icon_uris.add ("http://" + hostname + "/apple-touch-icon.png");

          var secondary_icons = this.extract_icon_from_html (html, false);
          foreach (string uri in secondary_icons)
            {
              this.icon_uris.add (uri);
            }
          this.icon_uris.add ("http://" + hostname + "/favicon.ico");
          this.icon_uris.add ("http://" + hostname + "/favicon.png");

          this.attempt_fetch_icon ();
        }

        else if (this.icon_phase)
        {
          debug ("we got the icon");
          // we actually got an icon :o - need to save it to a temp file
          try {
            Gdk.PixbufLoader loader = new Gdk.PixbufLoader ();
            loader.write (data.data, data.len);
            loader.close ();
            Gdk.Pixbuf icon = loader.get_pixbuf ();
            icon.save (this.destination, "png");
          } catch (Error e) {
            // we failed getting a new icon, so we need to try and get the
            // next icon on our uri list
            this.attempt_fetch_icon ();
            return;
          }

          // if we ge there, we built an icon witout failing
          this.set_desktop_file_icon (get_hostname (this.uri));
          Idle.add (() => {
            Unity.global_shell.need_new_icon_cache ();
            return false;
          });
        }
    }

    private void on_fetcher_failed ()
    {
      //fetcher failed for some reason
      if (this.html_phase) { this.failed (); return; }
      if (this.icon_phase)
        {
          //failed getting a new icon soo we try the next one in the list
          this.attempt_fetch_icon ();
        }
    }

    private void attempt_fetch_icon ()
    {
      if (this.icon_uris.size < 1) { this.failed (); return; }
      this.icon_phase = true;
      string uri = this.icon_uris.get (0);
      this.icon_uris.remove_at (0);
      if (!("http://" in uri))
        {
          // we have a relative uri
          uri = this.uri + uri;
        }
      this.fetcher = new FetchFile (uri);
      this.fetcher.failed.connect (() => { this.on_fetcher_failed ();});
      this.fetcher.completed.connect (this.on_fetcher_completed);
      this.fetcher.fetch_data ();
    }

    private Gee.PriorityQueue<string> extract_icon_from_html (string html, bool preferred)
    {
      var return_uris = new Gee.PriorityQueue<string> ();
      MatchInfo matchinfo;
      Regex[] regex_array;
      if (preferred)
        {
          regex_array = {Unity.Webapp.primary_match_prefix, Unity.Webapp.primary_match_suffix};
        }
      else
        {
          regex_array = {Unity.Webapp.secondary_match_prefix, Unity.Webapp.secondary_match_suffix};
        }

      foreach (Regex regex in regex_array)
      {
        if (regex.match (html, 0, out matchinfo))
          {
            string match = matchinfo.fetch_named ("icon_uri");
            if (match != "" && match != null)
              {
                return_uris.offer (match);
              };
          }
      }
      return return_uris;
    }

  }

}
