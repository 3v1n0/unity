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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *              Jay Taoko <jay.taoko@canonical.com>
 */

namespace Unity.Panel.Indicators
{
  public class IndicatorObjectEntryView : Ctk.Box
  {
    public unowned Indicator.ObjectEntry entry { get; construct; }
    public signal void menu_moved (Gtk.MenuDirectionType type);

    private Clutter.CairoTexture bg;
    private Ctk.Image     image;
    private Ctk.Text      text;

    public IndicatorObjectEntryView (Indicator.ObjectEntry _entry)
    {
      Object (entry:_entry,
              orientation: Ctk.Orientation.HORIZONTAL,
              spacing:4);
    }

    construct
    {
      /* Figure out if you need a label, text or both, create the ctk
       * representations.
       * Hook up the appropriate signals
       */
      this.padding = { 0, 3.0f, 0, 3.0f };

      this.bg = new Clutter.CairoTexture (10, 10);
      this.bg.set_parent (this);
      this.bg.opacity = 0;
      this.bg.show ();

      if (this.entry.image is Gtk.Image)
        {
          this.image = new Ctk.Image (22);
          this.add_actor (this.image);
          this.image.show ();

          if (this.entry.image.icon_name != null)
            {

              this.image.stock_id = this.entry.image.icon_name;

              this.entry.image.notify["icon-name"].connect (() =>
                {
                  this.image.stock_id = this.entry.image.icon_name;
                });

              unowned Gtk.IconTheme theme = Gtk.IconTheme.get_default ();
              theme.changed.connect (() =>
                {
                  this.image.stock_id = this.entry.image.icon_name;
                });
            }

          if (this.entry.image.pixbuf != null)
            {
              this.image.pixbuf = this.entry.image.pixbuf;

              this.entry.image.notify["pixbuf"].connect (() =>
                {
                  this.image.pixbuf = this.entry.image.pixbuf;
                });
            }
        }

      if (this.entry.label is Gtk.Label)
        {
          this.text = new Ctk.Text ("");
          this.text.color = { 233, 216, 200, 255 };
          this.add_actor (this.text);
          this.text.show ();

          this.text.text = this.entry.label.label;

          this.entry.label.notify["label"].connect (() =>
            {
              this.text.text = this.entry.label.label;
            });
        }
    }

    public void show_menu ()
    {
      /*
       * Register the menu with PanelMenuManager
       * Make sure all the right signals are connected
       * Emit menu_moved if the user presses left or right arrow in our menu,
       * we want to chain it up to our parent so it shows the next menu
       * or either left or right).
       */
       if (this.entry.menu is Gtk.Menu)
        {
          /* Show the menu and connect various signal to update the menu if
           * necessary.
           */
          this.entry.menu.move_current.connect (this.menu_key_moved);
          this.entry.menu.notify["visible"].connect (this.menu_vis_changed);
          this.bg.animate (Clutter.AnimationMode.EASE_OUT_QUAD, 200,
                           "opacity", 255);

          MenuManager.get_default ().register_visible_menu (this.entry.menu);
        }
    }

    public void menu_vis_changed ()
    {
      bool vis = (this.entry.menu.get_flags () & Gtk.WidgetFlags.VISIBLE) != 0;

      if (vis == false)
        {
          /* The menu isn't visible anymore. Disconnect some signals. */
          this.bg.animate (Clutter.AnimationMode.EASE_OUT_QUAD, 200,
                           "opacity", 0);
          this.entry.menu.move_current.disconnect (this.menu_key_moved);
          this.entry.menu.notify["visible"].disconnect (this.menu_vis_changed);
        }
    }

    public void menu_key_moved (Gtk.MenuDirectionType type)
    {
      if (type != Gtk.MenuDirectionType.PARENT &&
          type != Gtk.MenuDirectionType.CHILD)
        return;

      this.menu_moved (type);
    }

    /*
     * CLUTTER OVERRIDES
     */
    private override void allocate (Clutter.ActorBox        box,
                                    Clutter.AllocationFlags flags)
    {
      float width;
      float height;

      base.allocate (box, flags);

      width = Math.floorf (box.x2 - box.x1);
      height = Math.floorf (box.y2 - box.y1) - 1;

      Clutter.ActorBox child_box = { 0 };
      child_box.x1 = 0;
      child_box.x2 = width;
      child_box.y1 = 0;
      child_box.y2 = height;

      if (width != this.bg.width || height != this.bg.height)
        {
          this.update_bg ((int)width, (int)height);
        }

      this.bg.allocate (child_box, flags);
    }

    private override void paint ()
    {
      this.bg.paint ();
      base.paint ();
    }

    private override void map ()
    {
      base.map ();
      this.bg.map ();
    }

    private override void unmap ()
    {
      base.unmap ();
      this.bg.unmap ();
    }

    private void update_bg (int width, int height)
    {
      Cairo.Context cr;

      //this.bg.set_surface_size (width, height);

      cr = this.bg.create ();

      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      cr.set_operator (Cairo.Operator.OVER);

      cr.set_line_width (1.0);

      cr.set_source_rgba (1.0, 1.0, 1.0, 0.2);

      cr.move_to (1, height);

      cr.line_to (1, 7);

      cr.curve_to (1, 2,
                   1, 2,
                   10, 2);

      cr.line_to (width-10, 2);

      cr.curve_to (width, 2,
                   width, 2,
                   width, 7);

      cr.line_to (width, height);

      cr.line_to (1, height);

      var pat = new Cairo.Pattern.linear (1, 0, 1, height);
      pat.add_color_stop_rgba (0.0, 1.0f, 1.0f, 1.0f, 0.6f);
      pat.add_color_stop_rgba (1.0, 1.0f, 1.0f, 1.0f, 0.2f);

      cr.set_source (pat);
      cr.fill ();
    }
  }
}
