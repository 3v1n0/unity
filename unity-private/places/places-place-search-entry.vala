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
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

namespace Unity.Places
{
  public class PlaceSearchEntry : Ctk.Box
  {
    static const string SEARCH_ICON_FILE = Config.PKGDATADIR + "/search_icon.png";
    static const float  PADDING = 1.0f;
    static const int    LIVE_SEARCH_TIMEOUT = 200; /* Milliseconds */
    const Clutter.Color nofocus_color = { 0xff, 0xff, 0xff, 0xbb };
    const Clutter.Color focus_color   = { 0xff, 0xff, 0xff, 0xff };

    public Ctk.Image left_icon;
    public Ctk.Text  hint_text;
    public Ctk.Text  text;
    public ThemeImage right_icon;

    private uint live_search_timeout = 0;

    /* i18n: This is meant to be used like Search $PLACES_NAME, so eg.
     * 'Search Applications'
     */
    private string _static_text = _("Search %s");

    public signal void text_changed (string? text);

    public PlaceSearchEntry ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL,
              homogeneous:false,
              spacing:6);
    }

    construct
    {
      padding = { PADDING, PADDING * 4, PADDING , PADDING * 4};

      left_icon = new Ctk.Image.from_filename (18, SEARCH_ICON_FILE);
      pack (left_icon, false, true);
      left_icon.show ();

      hint_text = new Ctk.Text (_static_text.printf (""));
      hint_text.reactive = false;
      hint_text.selectable = false;
      hint_text.editable = false;
      hint_text.activatable = false;
      hint_text.single_line_mode = true;
      hint_text.cursor_visible = false;
      hint_text.color = nofocus_color;
      hint_text.set_parent (this);

      text = new Ctk.Text ("");
      text.reactive = true;
      text.selectable = true;
      text.editable = true;
      text.activatable = true;
      text.single_line_mode = true;
      text.cursor_visible = false;
      text.color = focus_color;
      pack (text, true, true);
      text.show ();

      text.text_changed.connect (on_text_changed);
      text.key_focus_in.connect (on_key_focus_in);
      text.key_focus_out.connect (on_key_focus_out);

      right_icon = new ThemeImage ("gtk-close");
      pack (right_icon, false, true);
      right_icon.show ();
    }

    private override void get_preferred_height (float     for_width,
                                               out float min_height,
                                               out float nat_height)
    {
      float mheight, nheight;
      text.get_preferred_height (400, out mheight, out nheight);

      min_height = mheight > 20.0f ? mheight : 20.0f;
      min_height += PADDING * 2;
      nat_height = min_height;
    }

    private override void get_preferred_width (float     for_height,
                                               out float min_width,
                                               out float nat_width)
    {
      /* FIXME: Make these dependant on size of screen & font */
      min_width = 270.0f;
      nat_width = 270.0f;
    }

    private void on_text_changed ()
    {
      hint_text.animate (Clutter.AnimationMode.EASE_OUT_QUAD, 100,
                         "opacity", text.text == "" ? 255 : 0);

      if (live_search_timeout != 0)
        Source.remove (live_search_timeout);

      live_search_timeout = Timeout.add (LIVE_SEARCH_TIMEOUT, () => {
        text_changed (text.text);
        live_search_timeout = 0;

        return false;
      });
    }

    private override void allocate (Clutter.ActorBox box,
                                    Clutter.AllocationFlags flags)
    {
      base.allocate (box, flags);

      Clutter.ActorBox child_box = Clutter.ActorBox ();
      child_box.x1 = text.x + 8;
      child_box.x2 = text.x + text.width;
      child_box.y1 = text.y;
      child_box.y2 = text.y + text.height;
      hint_text.allocate (child_box, flags);
    }

    private override void paint ()
    {
      hint_text.paint ();
      base.paint ();
    }

    private override void map ()
    {
      base.map ();
      hint_text.map ();
    }

    private override void unmap ()
    {
      base.unmap ();
      hint_text.unmap ();
    }

    private void on_key_focus_in ()
    {
      text.cursor_visible = true;
    }

    private void on_key_focus_out ()
    {
      text.cursor_visible = false;
    }

    public void reset ()
    {
      text.cursor_visible = false;
      text.text = "";
    }

    public void set_active_entry (PlaceEntry entry)
    {
      string name = "";

      if (entry is PlaceHomeEntry == false)
        name = entry.name;

      hint_text.text = _static_text.printf (name);
    }
  }
}
