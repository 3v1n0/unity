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

namespace Utils
{
  string strip_characters (string text,
                           string replace_text,
                           string match_regex,
                           string replace_regex)
  {
    string         ret = "";
    bool           matching = false;
    GLib.MatchInfo info;

    {
      GLib.Regex regex = null;

      try
        {
          regex = new GLib.Regex (match_regex,
                                  GLib.RegexCompileFlags.DOTALL |
                                  GLib.RegexCompileFlags.OPTIMIZE,
                                  0);
        }
      catch (GLib.RegexError e)
        {
          warning ("Creating regular-expression failed: \"%s\"\n", e.message);
        }

      matching = regex.match (text, 0, out info);
    }

    if (matching)
      {
        GLib.Regex regex = null;

        try
          {
            regex = new GLib.Regex (replace_regex,
                                    GLib.RegexCompileFlags.DOTALL |
                                    GLib.RegexCompileFlags.OPTIMIZE,
                                    0);
          }
        catch (GLib.RegexError e)
          {
            warning ("Creating regular-expression failed: \"%s\"\n", e.message);
          }

        try
          {
            ret = regex.replace (text, -1, 0, replace_text, 0);
          }
        catch (GLib.RegexError e)
          {
            warning ("Replacing text failed: \"%s\"\n", e.message);
          }
      }
    else
      ret = text;

    return ret;
  }

  [CCode (lower_case_prefix = "utils_")]
  public extern void set_strut (Gtk.Window *window,
                                  uint32    strut_size,
                                  uint32    strut_start,
                                  uint32    strut_end,
                                  uint32    top_size,
                                  uint32    top_start,
                                  uint32    top_end);

  [CCode (lower_case_prefix = "utils_")]
  public extern void register_object_on_dbus (DBus.Connection conn,
                                              string path,
                                              GLib.Object object);

  [CCode (lower_case_prefix = "utils_")]
  public extern X.Window get_stage_window (Clutter.Stage stage);

  [CCode (lower_case_prefix = "utils_")]
  public extern bool save_snapshot (Clutter.Stage stage,
                                    string        filename,
                                    int           x,
                                    int           y,
                                    int           width,
                                    int           height);

  [CCode (lower_case_prefix = "utils_")]
  public extern bool compare_snapshot (Clutter.Stage stage,
                                       string        filename,
                                       int           x,
                                       int           y,
                                       int           width,
                                       int           height,
                                       bool          expected=true);

  [CCode (lower_case_prefix = "utils_")]
  public extern bool utils_compare_images (string img1_path,
                                           string img2_path);
}
