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
 * Authored by Gordon Allott <gord.allott@canonical.com>,
 *             Mirco "MacSlow" Müller <mirco.mueller@canonical.com>
 *
 */

namespace Unity.Launcher
{
  // we subclass Ctk.MenuItem here because we need to adapt it's appearance
  public class QuicklistMenuItem : Ctk.MenuItem
  {
    Ctk.LayerActor item_background;
    int            last_width;
    int            last_height;
    string         old_label;

    private override void
    paint ()
    {
      if (this.item_background is Ctk.LayerActor)
        this.item_background.paint ();
    }

    public override void
    get_preferred_height (float     for_width,
                          out float min_height_p,
                          out float natural_height_p)
    {
      min_height_p = (float) Ctk.em_to_pixel (ITEM_HEIGHT);
      natural_height_p = min_height_p;
    }

    public override void
    get_preferred_width (float for_height,
                         out float min_width_p,
                         out float natural_width_p)
    {
      int width;
      int height;
      Gtk.Settings settings = Gtk.Settings.get_default ();
      Unity.QuicklistRendering.Item.get_text_extents (settings.gtk_font_name,
                                                      this.label,
                                                      out width,
                                                      out height);
      min_width_p = (float) width + (float) Ctk.em_to_pixel (2 * MARGIN);
      natural_width_p = min_width_p;
    }

    private override void
    allocate (Clutter.ActorBox        box,
              Clutter.AllocationFlags flags)
    {
      int new_width  = 0;
      int new_height = 0;

      base.allocate (box, flags);

      new_width  = (int) (box.x2 - box.x1);
      new_height = (int) (box.y2 - box.y1);

      // exit early if the allocation-width/height didn't change, this is needed
      // because clutter triggers calling allocate even if nothing changed
      if ((last_width == new_width) && (last_height == new_height))
        return;

      // store the new width/height
      last_width  = new_width;
      last_height = new_height;

      Timeout.add (0, _update_item_background);
    }

    private bool
    _update_item_background ()
    {
      Clutter.Color white_color = Clutter.Color () {
        red   = 255,
        green = 255,
        blue  = 255,
        alpha = 255
      };

      // before creating a new CtkLayerActor make sure we don't leak any memory
      if (this.item_background is Ctk.LayerActor)
      {
        this.item_background.unparent ();
        this.item_background.destroy ();
      }

      this.item_background = new Ctk.LayerActor (this.last_width,
                                                 this.last_height);

      Ctk.Layer normal_layer = new Ctk.Layer (this.last_width,
                                              this.last_height,
                                              Ctk.LayerRepeatMode.NONE,
                                              Ctk.LayerRepeatMode.NONE);
      Ctk.Layer selected_layer = new Ctk.Layer (this.last_width,
                                                this.last_height,
                                                Ctk.LayerRepeatMode.NONE,
                                                Ctk.LayerRepeatMode.NONE);

      Cairo.Surface normal_surf = new Cairo.ImageSurface (Cairo.Format.ARGB32,
                                                          this.last_width,
                                                          this.last_height);
      Cairo.Surface selected_surf = new Cairo.ImageSurface (Cairo.Format.ARGB32,
                                                            this.last_width,
                                                            this.last_height);

      Cairo.Context normal_cr = new Cairo.Context (normal_surf);
      Cairo.Context selected_cr = new Cairo.Context (selected_surf);
      Gtk.Settings settings = Gtk.Settings.get_default ();

      string formatted_label = Utils.strip_characters (label, "", "_", "_");

      Unity.QuicklistRendering.Item.normal_mask (normal_cr,
                                                 this.last_width,
                                                 this.last_height,
                                                 settings.gtk_font_name,
                                                 formatted_label);
      Unity.QuicklistRendering.Item.selected_mask (selected_cr,
                                                   this.last_width,
                                                   this.last_height,
                                                   settings.gtk_font_name,
                                                   formatted_label);

      normal_layer.set_mask_from_surface (normal_surf);
      normal_layer.set_color (white_color);

      selected_layer.set_mask_from_surface (selected_surf);
      selected_layer.set_color (white_color);

      this.item_background.add_layer (normal_layer);
      this.item_background.add_layer (selected_layer);

      this.item_background.get_layer(0).set_enabled (true);
      this.item_background.get_layer(1).set_enabled (false);
      if (this.get_stage () is Clutter.Stage)
        this.do_queue_redraw ();

      this.item_background.set_parent (this);
      this.item_background.map ();
      this.item_background.show ();

      return false;
    }

    private bool _on_enter (Clutter.Event event)
      requires (this is QuicklistMenuItem)
    {
      this.item_background.get_layer(0).set_enabled (false);
      this.item_background.get_layer(1).set_enabled (true);
      if (this.get_stage () is Clutter.Stage)
        this.do_queue_redraw ();

      return false;
    }

    private bool _on_leave (Clutter.Event event)
      requires (this is QuicklistMenuItem)
    {
      this.item_background.get_layer(0).set_enabled (true);
      this.item_background.get_layer(1).set_enabled (false);
      if (this.get_stage () is Clutter.Stage)
        this.do_queue_redraw ();

      return false;
    }

    private void _on_label_changed ()
    {
      // if the contents of the label didn't really change exit early
      if (old_label == this.label)
        return;

      old_label = this.label;
    }

    private bool _on_mouse_down (Clutter.Event event)
    {
      this.notify["label"].disconnect (this._on_label_changed);
      this.enter_event.disconnect (this._on_enter);
      this.leave_event.disconnect (this._on_leave);
      this.button_press_event.disconnect (this._on_mouse_down);
      this.activated ();
      return true;
    }

    public QuicklistMenuItem ()
    {
      Object (label: "");
    }

    public QuicklistMenuItem.with_label (string label)
    {
      Object (label: label);
    }

    ~QuicklistMenuItem ()
    {
      this.notify["label"].disconnect (this._on_label_changed);
      this.enter_event.disconnect (this._on_enter);
      this.leave_event.disconnect (this._on_leave);
      this.button_press_event.disconnect (this._on_mouse_down);
    }

    construct
    {
      Ctk.Padding padding = Ctk.Padding () {
        left   = (int) Ctk.em_to_pixel (MARGIN),
        right  = (int) Ctk.em_to_pixel (MARGIN),
        top    = (int) Ctk.em_to_pixel (MARGIN),
        bottom = (int) Ctk.em_to_pixel (MARGIN)
      };
      this.set_padding (padding);

      this.notify["label"].connect (this._on_label_changed);
      this.enter_event.connect (this._on_enter);
      this.leave_event.connect (this._on_leave);
      this.button_press_event.connect (this._on_mouse_down);

      this.reactive = true;

      last_width  = -1;
      last_height = -1;
      old_label   = "";
    }
  }
}
