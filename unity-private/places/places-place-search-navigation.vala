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

using Unity;

namespace Unity.Places
{
  [DBus (name = "com.canonical.Unity.PlaceBrowser")]
  public interface PlaceBrowserRemote : Object
  {
    public struct State
    {
      bool sensitive;
      string tooltip;
    }

    public abstract async State[] get_state () throws DBus.Error;
    public abstract async State[] go_back () throws DBus.Error;
    public abstract async State[] go_forward () throws DBus.Error;
  }

  public class PlaceSearchNavigation : Ctk.Box
  {
    static const int SPACING = 1;
    static const int ARROW_SIZE = 5;

    private CairoCanvas back;
    private CairoCanvas forward;

    private bool back_sensitive = false;
    private bool forward_sensitive = false;

    private PlaceEntry? active_entry = null;

    private Ctk.EffectGlow glow;

    private PlaceBrowserRemote remote;

    public PlaceSearchNavigation ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL,
              homogeneous:false,
              spacing:SPACING);
    }

    construct
    {
      back = new CairoCanvas (draw_back_arrow);
      back.reactive = true;
      back.button_release_event.connect (() => {
        if (remote != null && back_sensitive)
          {
            go_back.begin ();
          }

        return true;
      });
      pack (back, true, true);
      back.show ();

      forward = new CairoCanvas (draw_forward_arrow);
      forward.button_release_event.connect (() => {
        if (remote != null && forward_sensitive)
          {
            go_forward.begin ();
          }
        return true;
      });
      pack (forward, true, true);
      forward.show ();

      glow = new Ctk.EffectGlow ();
      glow.set_color ({ 255, 255, 255, 255 });
      glow.set_factor (1.0f);
      glow.set_margin (5);
      add_effect (glow);
    }

    public void set_active_entry (PlaceEntry entry)
    {
      active_entry = null;
      remote = null;

      if (entry.hints == null)
        {
          /* FIXME: Erm, I'm assuming we are meant to support _something_ */
          back_sensitive = false;
          forward_sensitive = false;
        }
      else
        {
          var path = entry.hints["UnityPlaceBrowserPath"];

          if (path != null)
            {
              try {
                var connection = DBus.Bus.get (DBus.BusType.SESSION);
                remote = (PlaceBrowserRemote)connection.get_object (entry.parent.dbus_name,
                                                 path,
                                                 "com.canonical.Unity.PlaceBrowser");
                refresh_states.begin ();

              } catch (Error e) {
                warning (@"Unable to connect to $path: %s",
                         e.message);
                back_sensitive = false;
                forward_sensitive = false;
              }
            }
          else
            {
              back_sensitive = false;
              forward_sensitive = false;
            }
        }

      back.update ();
      forward.update ();
      queue_relayout ();
      glow.set_invalidate_effect_cache (true);
    }

    private async void refresh_states ()
    {
      try {
        var states = yield remote.get_state ();
        back_sensitive = states[0].sensitive;
        forward_sensitive = states[1].sensitive;

      } catch (DBus.Error e) {
        warning (@"Unable to refresh browser navigation state: $(e.message)");
      }
    } 

    private async void go_forward ()
    {
      try {
        var states = yield remote.go_forward ();
        back_sensitive = states[0].sensitive;
        forward_sensitive = states[1].sensitive;
      } catch (DBus.Error e) {
        warning (@"Unable to go forward in browser view: $(e.message)");
      }
    }

    private async void go_back ()
    {
      try {       
        var states = yield remote.go_back ();
        back_sensitive = states[0].sensitive;
        forward_sensitive = states[1].sensitive;
      } catch (DBus.Error e) {
        warning (@"Unable to go back in browser view: $(e.message)");
      }
    }

    private override void get_preferred_width (float for_height,
                                               out float mwidth,
                                               out float nwidth)
    {
      mwidth = 54.0f;
      nwidth = 54.0f;
    }

    private void draw_back_arrow (Cairo.Context cr,
                                  int           width,
                                  int           height)
    {
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      cr.set_operator (Cairo.Operator.OVER);
      cr.set_line_width (1.0f);
      cr.translate (-0.5f, -0.5f);

      cr.set_source_rgba (1.0f, 1.0f, 1.0f, back_sensitive ? 1.0f : 0.125f);

      cr.move_to (width/2 - ARROW_SIZE, height/2);
      cr.line_to (width/2 + ARROW_SIZE, height/2 - ARROW_SIZE);
      cr.line_to (width/2 + ARROW_SIZE, height/2 + ARROW_SIZE);
      cr.close_path ();

      cr.fill ();
    }

    private void draw_forward_arrow (Cairo.Context cr,
                                     int           width,
                                     int           height)
    {
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      cr.set_operator (Cairo.Operator.OVER);
      cr.set_line_width (1.0f);
      cr.translate (0.5f, -0.5f);

      cr.set_source_rgba (1.0f, 1.0f, 1.0f, forward_sensitive ? 1.0f : 0.125f);

      cr.move_to (width/2 + ARROW_SIZE, height/2);
      cr.line_to (width/2 - ARROW_SIZE, height/2 - ARROW_SIZE);
      cr.line_to (width/2 - ARROW_SIZE, height/2 + ARROW_SIZE);
      cr.close_path ();

      cr.fill ();
    }
    /*
     * Public Methods
     */
  }
}
