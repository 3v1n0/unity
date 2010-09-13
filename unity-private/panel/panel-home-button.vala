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
    private Ctk.Image theme_image;
    private Ctk.EffectGlow glow;
    private Clutter.Texture bfb_bg_normal;
    private Clutter.Texture bfb_bg_prelight;
    private Clutter.Texture bfb_bg_active;
    private bool search_shown;

    public HomeButton (Shell shell)
    {
      Object (reactive:true, shell:shell);

      Unity.Testing.ObjectRegistry.get_default ().register ("PanelHomeButton",
                                                            this);
    }

    construct
    {
      theme_image = new Ctk.Image.from_filename (22, "/usr/share/icons/unity-icon-theme/places/22/distributor-logo.png");

      add_actor (theme_image);
      theme_image.show ();

      motion_event.connect (on_motion_event);
      clicked.connect (on_clicked);

      shell.mode_changed.connect (on_mode_changed);

      notify["state"].connect (on_state_changed);

      //width += 3.0f;

      glow = new Ctk.EffectGlow ();
      glow.set_color ({ 255, 255, 255, 255 });
      glow.set_factor (0.0f);
      glow.set_margin (5);

      // FIXME: the glow is currently not correctly updated when the state of
      // the button changes thus it's disabled for the moment, in order to get
      // the other rendering-features of the patch into trunk as they already
      // solve giving the user a clue that the BFB/COF is a clickable button
      //theme_image.add_effect (glow);

      try
        {
          bfb_bg_normal = new Clutter.Texture.from_file (
                                   "/usr/share/unity/themes/bfb_bg_normal.png");
        }
      catch (Error e)
        {
          warning ("Could not load normal-state bg-texture: %s", e.message);
        }

      try
        {
          bfb_bg_prelight = new Clutter.Texture.from_file (
                                 "/usr/share/unity/themes/bfb_bg_prelight.png");
        }
      catch (Error e)
        {
          warning ("Could not load prelight-state bg-texture: %s", e.message);
        }

      try
        {
          bfb_bg_active = new Clutter.Texture.from_file (
                                   "/usr/share/unity/themes/bfb_bg_active.png");
        }
      catch (Error e)
        {
          warning ("Could not load active-state bg-texture: %s", e.message);
        }

      set_background_for_state (Ctk.ActorState.STATE_NORMAL, bfb_bg_normal);
      set_background_for_state (Ctk.ActorState.STATE_PRELIGHT, bfb_bg_prelight);
      set_background_for_state (Ctk.ActorState.STATE_ACTIVE, bfb_bg_active);

      search_shown = false;
    }

    private override void allocate (Clutter.ActorBox        box,
                                    Clutter.AllocationFlags flags)
    {
      float       cwidth;
      float       cheight;
      float       lwidth;
      float       pheight;
      Ctk.Padding pad = { 0 };
 
      // 1.0f are subtracted so the groove dividing home-button and
      // the rest of the panel aligns with the right edge of the
      // launcher, this fixes LP: #630031
      lwidth  = (float) shell.get_launcher_width_foobar () - 1.0f;
      pheight = (float) shell.get_panel_height_foobar ();
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

    private void on_state_changed ()
    {
      switch (this.get_state ())
      {
        case Ctk.ActorState.STATE_NORMAL:
          glow.set_factor (0.0f);
          glow.set_invalidate_effect_cache (true);
          do_queue_redraw ();
        break;

        case Ctk.ActorState.STATE_PRELIGHT:
          glow.set_factor (0.8f);
          glow.set_invalidate_effect_cache (true);
          do_queue_redraw ();
        break;

        case Ctk.ActorState.STATE_ACTIVE:
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
      // 1.0f are subtracted so the groove dividing home-button and
      // the rest of the panel aligns with the right edge of the
      // launcher, this fixes LP: #630031
      min_width = shell.get_launcher_width_foobar () - 1.0f;
      nat_width = shell.get_launcher_width_foobar () - 1.0f;
    }

    private void on_mode_changed (ShellMode mode)
    {
      if (mode == ShellMode.MINIMIZED)
        {
          set_background_for_state (Ctk.ActorState.STATE_NORMAL, bfb_bg_normal);
          set_background_for_state (Ctk.ActorState.STATE_PRELIGHT, bfb_bg_prelight);
          set_background_for_state (Ctk.ActorState.STATE_ACTIVE, bfb_bg_active);
          search_shown = false;
        }
      else
        {
          set_background_for_state (Ctk.ActorState.STATE_NORMAL, null);
          set_background_for_state (Ctk.ActorState.STATE_PRELIGHT, null);
          set_background_for_state (Ctk.ActorState.STATE_ACTIVE, null);
          search_shown = true;
        }

      do_queue_redraw ();
    }

    private void on_clicked ()
    {
      shell.show_unity ();
      MenuManager manager = MenuManager.get_default ();
      manager.popdown_current_menu ();

      on_mode_changed (shell.get_mode ());
    }

    private bool on_motion_event (Clutter.Event event)
    {
      shell.about_to_show_places ();

      return true;
    }
  }
}

