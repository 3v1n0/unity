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

  public class View : Ctk.Box
  {
    public bool expanded = true;
    public Shell shell { get; construct;}

    Background            bground;
    HomeButton            home_button;
    MenuBar               menu_bar;
    SystemTray            system_tray;
    IndicatorBar          indicator_bar;

    public View (Shell shell)
    {
      Object (shell:shell,
              reactive:false,
              orientation:Ctk.Orientation.HORIZONTAL,
              homogeneous:false,
              spacing:0);
      system_tray.manage_stage (shell.get_stage ());
    }

    construct
    {
      START_FUNCTION ();

      /* Initialize the models */
      Indicators.IndicatorsModel.get_default();

      /* Create the background and become it's parent */
      //rect = new ThemeImage ("panel_background");
      bground = new Background ();
      set_background (bground);
      bground.show ();
      
      /* Create the views and add them to the box */
      home_button = new HomeButton (shell);
      pack (home_button, false, true);
      home_button.show ();

      menu_bar = new MenuBar ();
      pack (menu_bar, true, true);
      menu_bar.show ();

      system_tray = new SystemTray ();
      pack (system_tray, false, true);
      system_tray.show ();

      indicator_bar = new IndicatorBar ();
      pack (indicator_bar, false, true);
      indicator_bar.show ();

      END_FUNCTION ();
    }

    public int get_indicators_width ()
    {
      return (int) this.indicator_bar.get_width ();
      //this.system_tray.width + this.indicator_bar.width;
    }

    public void set_expanded (bool _expanded)
    {
      // Hide menubar
      // Put background into expanded mode
      this.expanded = _expanded;
    }

//     private override void allocate (Clutter.ActorBox        box,
//                                     Clutter.AllocationFlags flags)
//     {
//       Clutter.ActorBox child_box = { 0, 0, box.x2 - box.x1, box.y2 - box.y1 };
//       float            width;
//       float            child_width;
//
//       base.allocate (box, flags);
//
//       width = box.x2 - box.x1;
//
//       this.rect.set_clip (0, 0, width, box.y2 - box.y1);
//
//       First the background
//       child_box.y2 += 3.0f;
//       this.rect.allocate (child_box, flags);
// //
// //       Home button
// //       child_box.x1 = 0;
// //       child_box.x2 = 60;
// //       child_box.y1 = 0;
// //       child_box.y2 = PANEL_HEIGHT;
// //       this.home.allocate (child_box, flags);
//
//       this.indicator_bar.get_preferred_width (PANEL_HEIGHT,
//                                            out child_width,
//                                            out child_width);
//       child_box.x1 = width - child_width;
//       child_box.x2 = width;
//       this.indicator_bar.allocate (child_box, flags);
//     }


    public int get_panel_height ()
    {
      return Unity.Panel.PANEL_HEIGHT;
    }

    public void set_indicator_mode (bool mode)
    {
    	if (mode)
    		{
          menu_bar.hide ();
          bground.hide ();
          system_tray.hide ();
          indicator_bar.set_indicator_mode (mode);
    		}
    	else
        {
          menu_bar.show ();
          bground.show ();
          system_tray.show ();
          indicator_bar.set_indicator_mode (mode);
    		}
    	
    	//do_queue_redraw ();
    		
//       float x;
//       x = mode ? this.width - this.get_indicators_width () : 0;
//       this.rect.set_clip (x,
//                           0,
//                           mode ? this.get_indicators_width () : this.width,
//                           mode ? PANEL_HEIGHT -1 : PANEL_HEIGHT);
    }
  }
}
