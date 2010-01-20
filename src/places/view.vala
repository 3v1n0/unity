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
 * Authored by Mirco "MacSlow" Müller <mirco.mueller@canonical.com>
 * Authored by Jay Taoko <jay.taoko@canonical.com>
 *
 */

namespace Unity.Places
{
  /* Margin outside of the cairo texture. We draw outside to complete the line loop
  * and we don't want the line loop to be visible in some parts of the screen. 
  * */
  private int Margin = 5;

  public class View : Ctk.Box
  {
    private Unity.Places.Bar.View       bar_view;
    private Unity.Places.File.FileView  file_view;
    private Unity.Places.Application.ApplicationView  app_view;
    private Unity.Places.Default.View   default_view;

    private Gee.ArrayList<Unity.Places.CairoDrawing.PlacesBackground> places_background_array;
    private Gee.ArrayList<Unity.Places.CairoDrawing.PlacesBackground> devices_background_array;
    Unity.Places.CairoDrawing.PlacesBackground                        TrashBackground;

    private int current_tab_index;

    /* These parameters are temporary until we get the right metrics for the places bar */

    public override void allocate (Clutter.ActorBox        box, 
                                   Clutter.AllocationFlags flags)
    {
      base.allocate (box, flags);
      Clutter.ActorBox child_box = { 0.0f, 0.0f, 0.0f, 0.0f };
      int bar_icon_size = 48; /* HARDCODED: height of icons in places bar */
      int icon_margin = 4;
      int PlacesPosition = 0; /* HARDCODED: Places X origin. Should be 0 in practice. */
      int NewPlaceWidth = (int) (box.x2 - box.x1);

      this.bar_view.IconSize = bar_icon_size;
      this.bar_view.FirstPlaceIconPosition = 100; /* HARDCODED: Offset from the left of the Places bar to the first Icon */
      this.bar_view.PlaceIconSpacing = 24;
      
      child_box.x1 = PlacesPosition;
      child_box.x2 = box.x2 - box.x1;
      child_box.y1 = this.padding.top + 8;
      child_box.y2 = child_box.y1 + bar_icon_size;

      this.bar_view.TrashPosition = (int)(box.x2 - 216 - bar_icon_size - 80); /* HARDCODED: Menu width in the places bar */
      this.bar_view.SeparatorPosition = (int)(this.bar_view.TrashPosition - 20); /* HARDCODED: Menu width in the places bar */

      this.bar_view.allocate (child_box, flags);
  
      child_box.x1 = PlacesPosition;
      child_box.x2 = box.x2 - box.x1;
      child_box.y1 = this.padding.top + 8 + bar_icon_size;
      child_box.y2 = box.y2 - child_box.y1;
      this.file_view.allocate (child_box, flags);
      this.app_view.allocate (child_box, flags);



      child_box.x1 = box.x1 + 58 /* HARDCODED: width of quicklauncher */;
      child_box.x2 = box.x2;
      child_box.y1 = box.y1 + 55 /* HARDCODED: height of Places bar */;
      child_box.y2 = box.y2;

      this.default_view.allocate (child_box, flags);

      child_box.x1 = 0;
      child_box.x2 = NewPlaceWidth;
      child_box.y1 = 0;
      child_box.y2 = 55 + Margin; /* HARDCODED: height of Places bar */;

      int spacing = this.bar_view.PlaceIconSpacing;
      int i;
      for (i = 0; i < this.places_background_array.size; i++)
        {
          if (this.places_background_array[i].PlaceWidth != NewPlaceWidth)
            {
              this.places_background_array[i].CreatePlacesBackground (NewPlaceWidth, 55,
              PlacesPosition + this.bar_view.FirstPlaceIconPosition + i*(bar_icon_size + spacing) - icon_margin,
              2*icon_margin + bar_icon_size);
            }
          this.places_background_array[i].allocate (child_box, flags);
        }

      if (this.TrashBackground.PlaceWidth != NewPlaceWidth)
        {
          this.TrashBackground.CreatePlacesBackground (NewPlaceWidth, 55,
          PlacesPosition + this.bar_view.TrashPosition - icon_margin,
          2*icon_margin + bar_icon_size);
        }
      this.TrashBackground.allocate (child_box, flags);
      
    }

    public View ()
    {
      int i;
      int NunItems;
      Unity.Places.CairoDrawing.PlacesBackground background;

      this.places_background_array = new Gee.ArrayList<Unity.Places.CairoDrawing.PlacesBackground> ();
      this.devices_background_array = new Gee.ArrayList<Unity.Places.CairoDrawing.PlacesBackground> ();

      this.current_tab_index = 0;
      this.orientation  = Ctk.Orientation.VERTICAL;
      this.bar_view     = new Unity.Places.Bar.View ();
      this.file_view    = new Unity.Places.File.FileView ();
      this.app_view    = new Unity.Places.Application.ApplicationView ();
      this.bar_view.sig_places_active_icon_index.connect(this.on_signal_active_icon);
      this.bar_view.sig_devices_active_icon_index.connect(this.on_signal_device_active_icon);
      this.bar_view.sig_trash_active_icon_index.connect(this.on_signal_trash_active_icon);

      this.bar_view.sig_activate_home_place.connect(this.on_home_place_active);
      this.bar_view.sig_activate_file_place.connect(this.on_file_place_active);
      this.bar_view.sig_activate_application_place.connect(this.on_application_place_active);


      this.default_view = new Unity.Places.Default.View ();

      NunItems = this.bar_view.get_number_of_places ();      
      for (i = 0; i < NunItems; i++)
      {
        background = new Unity.Places.CairoDrawing.PlacesBackground ();
        this.add_actor (background);
        this.places_background_array.add (background);
        background.hide ();
      }
      this.places_background_array[0].show ();

      NunItems = this.bar_view.get_number_of_devices ();      
      for (i = 0; i < NunItems; i++)
      {
        background = new Unity.Places.CairoDrawing.PlacesBackground ();
        this.add_actor (background);
        this.devices_background_array.add (background);
        background.hide ();
      }

      TrashBackground = new Unity.Places.CairoDrawing.PlacesBackground ();
      this.add_actor (TrashBackground);
      TrashBackground.hide ();


      this.add_actor (this.bar_view);
      this.add_actor (this.file_view);
      this.add_actor (this.app_view);
      this.add_actor (this.default_view);

      this.file_view.hide ();
      this.app_view.hide ();

      Ctk.Padding padding = { 0.0f, 0.0f, 0.0f, 12.0f };
      this.set_padding (padding);
    }

    public void set_size_and_position (int bar_x,
                                       int bar_y,
                                       int bar_w,
                                       int bar_h,
                                       int def_view_x,
                                       int def_view_y,
                                       int def_view_w,
                                       int def_view_h)
    {
      this.bar_view.x      = bar_x;
      this.bar_view.y      = bar_y;
      this.bar_view.width  = bar_w;
      this.bar_view.height = bar_h;

      this.default_view.x      = def_view_x;
      this.default_view.y      = def_view_y;
      this.default_view.width  = def_view_w;
      this.default_view.height = def_view_h;
    }

    construct
    {
    }

    public void on_home_place_active ()
    {
      this.default_view.show ();
      this.file_view.hide ();
      this.app_view.hide ();
    }

    public void on_file_place_active ()
    {
      this.file_view.show ();
      this.default_view.hide ();
      this.app_view.hide ();
    }

    public void on_application_place_active ()
    {
      this.file_view.hide ();
      this.default_view.hide ();
      this.app_view.show ();
    }

    public void on_signal_active_icon (int i)
    {
      if (i >= this.places_background_array.size)
        return;

      int j;
      for (j = 0; j < this.devices_background_array.size; j++)
        {
          this.devices_background_array[j].hide ();
        }
      TrashBackground.hide ();        

      this.places_background_array[this.current_tab_index].hide ();
      this.current_tab_index = i;
      this.places_background_array[this.current_tab_index].show ();

      this.file_view.hide ();
      this.default_view.hide ();
      this.app_view.hide ();
    }

    public void on_signal_device_active_icon (int i)
    {
      if (i >= this.devices_background_array.size)
        return;

      int j;
      for (j = 0; j < this.places_background_array.size; j++)
        {
          this.places_background_array[j].hide ();
        }
      TrashBackground.hide ();        

      this.devices_background_array[this.current_tab_index].hide ();
      this.current_tab_index = i;
      this.devices_background_array[this.current_tab_index].show ();

      this.file_view.hide ();
      this.default_view.hide ();
      this.app_view.hide ();
    }

    public void on_signal_trash_active_icon ()
    {
      int j;
      for (j = 0; j < this.places_background_array.size; j++)
        {
          this.places_background_array[j].hide ();
        }
      for (j = 0; j < this.devices_background_array.size; j++)
        {
          this.devices_background_array[j].hide ();
        }

      TrashBackground.show ();

      this.file_view.hide ();
      this.default_view.hide ();
      this.app_view.hide ();
    }

  }
}

