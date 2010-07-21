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

namespace Unity.Panel
{
  public class HomeButton : Ctk.Button
  {
    public Shell shell { get; construct; }
    public Ctk.Image theme_image;
    public Ctk.EffectGlow glow;
    public Clutter.Texture bg_normal;
    public Clutter.Texture bg_prelight;
    public Clutter.Texture bg_active;

    public HomeButton (Shell shell)
    {
      Object (reactive:true, shell:shell);

      Unity.Testing.ObjectRegistry.get_default ().register ("PanelHomeButton",
                                                            this);
    }

    private override void allocate (Clutter.ActorBox        box,
                                    Clutter.AllocationFlags flags)
    {
      float       cwidth;
      float       cheight;
      float       lwidth;
      float       pheight;
      Ctk.Padding pad = { 0 };

      lwidth  = (float) shell.get_launcher_width_foobar ();
      pheight = (float) shell.get_panel_height_foobar ();
      print ("size: %3.2f, %3.2f\n", lwidth, pheight);
      theme_image.get_preferred_size (out cwidth, out cheight,
                                           out cwidth, out cheight);

      /* adapt icon-width to launcher-width with padding */
      if (lwidth - cwidth <= 0.0f)
        {
          /* icon is wider or as wide as launcher */
          pad.left   = 0.0f;
          pad.right  = pad.left;
        }
      else
        {
          /* icon is narrower than launcher */
          pad.left   = (box.x2 - box.x1 - cwidth) / 2.0f;
          pad.right  = pad.left;
        }

      /* adapt icon-height to panel-height with padding */
      if (pheight - cheight <= 0.0f)
        {
          /* icon higher or as high as launcher */
          pad.top    = 0.0f;
          pad.bottom = pad.top;
        }
      else
        {
          /* icon is smaller than launcher */
          pad.top    = (box.y2 - box.y1 - cheight) / 2.0f;
          pad.bottom = pad.top;
        }

      padding = pad;

      base.allocate (box, flags);
    }

    construct
    {
      theme_image = new Ctk.Image.from_filename (22, "/usr/share/icons/unity-icon-theme/places/22/distributor-logo.png");

      add_actor (theme_image);
      theme_image.show ();

      motion_event.connect (on_motion_event);
      clicked.connect (on_clicked);

      notify["state"].connect (on_state_changed);

      glow = new Ctk.EffectGlow ();
      glow.set_color ({ 255, 255, 255, 255 });
      glow.set_factor (0.0f);
      glow.set_margin (5);
      theme_image.add_effect (glow);

      bg_normal   = new Clutter.Texture.from_file ("./bg_normal.png");
      bg_prelight = new Clutter.Texture.from_file ("./bg_prelight.png");
      bg_active   = new Clutter.Texture.from_file ("./bg_active.png");

      set_background_for_state (Ctk.ActorState.STATE_NORMAL, bg_normal);
      set_background_for_state (Ctk.ActorState.STATE_PRELIGHT, bg_prelight);
      set_background_for_state (Ctk.ActorState.STATE_ACTIVE, bg_active);
    }

    private void on_state_changed ()
    {
      switch (this.get_state ())
      {
        case Ctk.ActorState.STATE_NORMAL:
          print ("state normal, (factor: %3.2f)\n", glow.get_factor ());
          glow.set_factor (0.0f);
          glow.set_invalidate_effect_cache (true);
          do_queue_redraw ();
        break;

        case Ctk.ActorState.STATE_PRELIGHT:
          print ("state prelight, (factor: %3.2f)\n", glow.get_factor ());
          glow.set_factor (0.8f);
          glow.set_invalidate_effect_cache (true);
          do_queue_redraw ();
        break;

        case Ctk.ActorState.STATE_ACTIVE:
          print ("state active, (factor: %3.2f)\n", glow.get_factor ());
          glow.set_factor (1.0f);
          glow.set_invalidate_effect_cache (true);
          do_queue_redraw ();
        break;
      }
    }

    /* We always want to be the width of the launcher */
    private override void get_preferred_width (float     for_height,
                                               out float min_width,
                                               out float nat_width)
    {
      min_width = shell.get_launcher_width_foobar ();
      nat_width = shell.get_launcher_width_foobar ();
    }

    private void on_clicked ()
    {
      shell.show_unity ();
      MenuManager manager = MenuManager.get_default ();
      manager.popdown_current_menu ();

    }

    private bool on_motion_event (Clutter.Event event)
    {
      shell.about_to_show_places ();

      return true;
    }
  }
}

/*
you probably need to add a new function to unity.shell to do that, and then
implement in Unity.Plugin and testing/test-window.vala (for the two targets)
*/

