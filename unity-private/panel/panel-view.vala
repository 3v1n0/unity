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
using Unity.Panel.Indicators;

namespace Unity.Panel
{
  static const int PANEL_HEIGHT = 24;
  static const bool search_entry_has_focus = false;

  public class View : Ctk.Bin
  {
    public Ctk.EffectCache cache;

    public bool expanded = true;
    public Shell shell { get; construct;}

    Ctk.HBox              hbox;
    Background            bground;
    HomeButton            home_button;
    WindowButtons         window_buttons;
    MenuBar               menu_bar;
    SystemTray            system_tray;
    IndicatorBar          indicator_bar;

    public View (Shell shell)
    {
      Object (shell:shell,
              reactive:true);
      system_tray.manage_stage (shell.get_stage ());
    }

    construct
    {
      START_FUNCTION ();

      /* Initialize the models */
      Indicators.IndicatorsModel.get_default();

      hbox = new Ctk.HBox (0);
      hbox.homogeneous = false;
      add_actor (hbox);
      hbox.show ();

      /* Create the background and become it's parent */
      //rect = new ThemeImage ("panel_background");
      bground = new Background ();
      set_background (bground);
      bground.show ();

      /* Create the views and add them to the box */
      home_button = new HomeButton (shell);
      hbox.pack (home_button, false, true);
      home_button.show ();

      window_buttons = new WindowButtons ();
      hbox.pack (window_buttons, false, true);
      window_buttons.show ();

      menu_bar = new MenuBar ();
      hbox.pack (menu_bar, true, true);
      menu_bar.show ();

      system_tray = new SystemTray ();
      hbox.pack (system_tray, false, true);
      system_tray.show ();

      indicator_bar = new IndicatorBar ();
      hbox.pack (indicator_bar, false, true);
      indicator_bar.show ();

      button_release_event.connect (on_button_release_event);

      cache = new Ctk.EffectCache ();
      add_effect (cache);
      cache.update_texture_cache ();
      hbox.queue_redraw.connect (() => { cache.update_texture_cache (); });

      END_FUNCTION ();
    }

    private bool on_button_release_event (Clutter.Event e)
    {
      MenuManager manager = MenuManager.get_default ();
      manager.popdown_current_menu ();

      return false;
    }

    public int get_indicators_width ()
    {
      return (int) indicator_bar.get_width ();
      //system_tray.width + indicator_bar.width;
    }

    public void set_expanded (bool _expanded)
    {
      // Hide menubar
      // Put background into expanded mode
      expanded = _expanded;
    }

    public int get_panel_height ()
    {
      return Unity.Panel.PANEL_HEIGHT;
    }

    public void set_indicator_mode (bool mode)
    {
      if (mode)
        {
          Timeout.add (0, () => {
            cache.invalidate_texture_cache ();
            do_queue_redraw ();
            return false;
          });

         if (menu_bar.indicator_object_view is Clutter.Actor)
            menu_bar.indicator_object_view.hide ();
          window_buttons.hide ();
          bground.hide ();
          system_tray.hide ();
          reactive = false;
          /*
          var glow = new Ctk.EffectGlow ();
          glow.set_color ({ 255, 255, 255, 150 });
          glow.set_factor (1.0f);
          glow.set_margin (5);
          indicator_bar.add_effect (glow);
          */

          do_queue_redraw ();
        }
      else
        {
          if (menu_bar.indicator_object_view is Clutter.Actor)
            menu_bar.indicator_object_view.show ();
          window_buttons.show ();
          bground.show ();
          system_tray.show ();
          reactive = true;

          indicator_bar.remove_all_effects ();

          Timeout.add (0, () => {
            cache.update_texture_cache ();
            do_queue_redraw ();
            return false;
          });      }
    }
  }
}
