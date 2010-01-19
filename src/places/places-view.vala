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
 *
 */

namespace Unity.Places
{
  /* Margin outside of the cairo texture. We draw outside to complete the line loop
  * and we don't want the line loop to be visible in some parts of the screen.
  * */
  private int Margin = 5;

  public class PlacesBackground : Ctk.Bin
  {
    public Clutter.CairoTexture cairotxt;

    private Ctk.EffectGlow effect_glow;

    /* (X, Y) origine of Places Bar: The top-left corner of the screen */
    private int PlaceX = 0;
    private int PlaceY = 0;

    /* Places Bar width and height. The width is set the the width of the window. */
    private int PlaceW = 760;
    private int PlaceH = 55;

    private int PlaceBottom; /* PlaceY + PlaceH */

    /* Menu area width and height */
    private int MenuH = 22;
    private int MenuW = 216;

    private int Rounding = 16;
    private int RoundingSmall = 8;

    private int MenuBottom; /* PlaceY + MenuH */

    private int TabH = 50;

    /* The squirl area width */
    private int SquirlW = 70;

    public struct TabRect
    {
      int left;
      int right;
      int top;
      int bottom;
    }

    public int PlaceWidth;

    void DrawAroundMenu (Cairo.Context cairoctx)
    {
      cairoctx.line_to (PlaceX + PlaceW + Margin, PlaceY + MenuH);
      cairoctx.line_to (PlaceX + PlaceW - MenuW + Rounding, PlaceY + MenuH);
      cairoctx.curve_to (
        PlaceX + PlaceW - MenuW, PlaceY + MenuH,
        PlaceX + PlaceW - MenuW, PlaceY + MenuH,
        PlaceX + PlaceW - MenuW, PlaceY + MenuH + Rounding);
      cairoctx.line_to (PlaceX + PlaceW - MenuW, PlaceY + PlaceH - Rounding);
      cairoctx.curve_to (
        PlaceX + PlaceW - MenuW, PlaceY + PlaceH,
        PlaceX + PlaceW - MenuW, PlaceY + PlaceH,
        PlaceX + PlaceW - MenuW - Rounding, PlaceY + PlaceH);
    }

    void DrawTab(Cairo.Context cairoctx, TabRect tab)
    {
      cairoctx.line_to (tab.right + RoundingSmall, PlaceBottom );
      cairoctx.curve_to (
        tab.right, PlaceBottom,
        tab.right, PlaceBottom,
        tab.right, PlaceBottom - RoundingSmall);
      cairoctx.line_to (tab.right, tab.top + Rounding);
      cairoctx.curve_to (
        tab.right, tab.top,
        tab.right, tab.top,
        tab.right - Rounding, tab.top);
      cairoctx.line_to (tab.left + Rounding, tab.top);
      cairoctx.curve_to (
      tab.left, tab.top,
        tab.left, tab.top,
        tab.left, tab.top + Rounding);
      cairoctx.line_to (tab.left, PlaceBottom - RoundingSmall);
      cairoctx.curve_to (
        tab.left, PlaceBottom,
        tab.left, PlaceBottom,
        tab.left - RoundingSmall, PlaceBottom);
    }

    void DrawSquirl(Cairo.Context cairoctx)
    {
      cairoctx.line_to (PlaceX + SquirlW + RoundingSmall, PlaceBottom);
      cairoctx.curve_to (
        PlaceX + SquirlW, PlaceBottom,
        PlaceX + SquirlW, PlaceBottom,
        PlaceX + SquirlW, PlaceBottom - RoundingSmall);
      cairoctx.line_to (PlaceX + SquirlW, MenuBottom + Rounding );
      cairoctx.curve_to (
        PlaceX + SquirlW, MenuBottom,
        PlaceX + SquirlW, MenuBottom,
        PlaceX + SquirlW - Rounding, MenuBottom);
      cairoctx.line_to (PlaceX - Margin, MenuBottom);
    }

    public PlacesBackground ()
    {
      PlaceWidth = 0;
    }

    public void CreatePlacesBackground (int WindowWidth,
      int WindowHeight,
      int TabPositionX,
      int TabWidth)
    {
      PlaceWidth = WindowWidth;
      PlaceW = WindowWidth;
      PlaceBottom = PlaceY + PlaceH;
      MenuBottom = PlaceY + MenuH;

      if (cairotxt != null)
        this.remove_actor (cairotxt);

      cairotxt = new Clutter.CairoTexture(PlaceW, PlaceH + Margin);
      Cairo.Context cairoctx = cairotxt.create();
      {
        cairoctx.set_source_rgba (1, 1, 1, 1.0);
        cairoctx.set_line_width (1.0);

        cairoctx.move_to (PlaceX - Margin, PlaceY - Margin);
        cairoctx.line_to (PlaceX + PlaceW + Margin, PlaceY - Margin);

        DrawAroundMenu (cairoctx);

        TabRect tab = TabRect();
        tab.left = TabPositionX;
        tab.right = tab.left + TabWidth;
        tab.top = PlaceY + PlaceH - TabH;
        tab.bottom = PlaceY + PlaceH;

        DrawTab(cairoctx, tab);

        DrawSquirl (cairoctx);

        /* close the path */
        cairoctx.line_to (PlaceX - Margin, PlaceY - Margin);

        cairoctx.stroke_preserve ();
        cairoctx.set_source_rgba (1, 1, 1, 0.15);
        cairoctx.fill ();
      }

      cairotxt.set_opacity (0xFF);
      this.add_actor (cairotxt);

      effect_glow = new Ctk.EffectGlow ();
      Clutter.Color c = Clutter.Color ()
      {
        red = 255,
        green = 255,
        blue = 255,
        alpha = 255
      };

      effect_glow.set_color (c);
      effect_glow.set_factor (1.0f);
      effect_glow.set_margin (5);
      this.add_effect (effect_glow);
    }

    construct
    {
    }
  }

  public class View : Ctk.Box
  {
    private Unity.Places.Bar.View     bar_view;
    private Unity.Places.Default.View default_view;
    private Gee.ArrayList<PlacesBackground> PlacesBackgroundArray;
    private Gee.ArrayList<PlacesBackground> DevicesBackgroundArray;
    PlacesBackground                        TrashBackground;

    private int current_tab_index;

    public Model model { get; construct; }

    public View (Model model)
    {
      int i;
      int NunItems;
      PlacesBackground background;

      Object (model:model);

      this.PlacesBackgroundArray = new Gee.ArrayList<PlacesBackground> ();
      this.DevicesBackgroundArray = new Gee.ArrayList<PlacesBackground> ();

      this.current_tab_index = 0;
      this.orientation  = Ctk.Orientation.VERTICAL;

      this.bar_view     = new Unity.Places.Bar.View (this.model);
      this.bar_view.sig_places_active_icon_index.connect(this.on_signal_active_icon);
      this.bar_view.sig_devices_active_icon_index.connect(this.on_signal_device_active_icon);
      this.bar_view.sig_trash_active_icon_index.connect(this.on_signal_trash_active_icon);

      this.default_view = new Unity.Places.Default.View ();

      NunItems = this.bar_view.get_number_of_places ();
      for (i = 0; i < NunItems; i++)
      {
        background = new PlacesBackground ();
        this.add_actor (background);
        this.PlacesBackgroundArray.add (background);
        background.hide ();
      }
      this.PlacesBackgroundArray[0].show ();

      NunItems = this.bar_view.get_number_of_devices ();
      for (i = 0; i < NunItems; i++)
      {
        background = new PlacesBackground ();
        this.add_actor (background);
        this.DevicesBackgroundArray.add (background);
        background.hide ();
      }

      TrashBackground = new PlacesBackground ();
      this.add_actor (TrashBackground);
      TrashBackground.hide ();


      this.add_actor (this.bar_view);
      this.add_actor (this.default_view);

      Ctk.Padding padding = { 0.0f, 0.0f, 0.0f, 12.0f };
      this.set_padding (padding);
    }

    /* These parameters are temporary until we get the right metrics for
     * the places bar
     */
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
      for (i = 0; i < this.PlacesBackgroundArray.size; i++)
        {
          if (this.PlacesBackgroundArray[i].PlaceWidth != NewPlaceWidth)
            {
              this.PlacesBackgroundArray[i].CreatePlacesBackground (NewPlaceWidth, 55,
              PlacesPosition + this.bar_view.FirstPlaceIconPosition + i*(bar_icon_size + spacing) - icon_margin,
              2*icon_margin + bar_icon_size);
            }
          this.PlacesBackgroundArray[i].allocate (child_box, flags);
        }

      if (this.TrashBackground.PlaceWidth != NewPlaceWidth)
        {
          this.TrashBackground.CreatePlacesBackground (NewPlaceWidth, 55,
          PlacesPosition + this.bar_view.TrashPosition - icon_margin,
          2*icon_margin + bar_icon_size);
        }
      this.TrashBackground.allocate (child_box, flags);

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

    public void on_signal_active_icon (int i)
    {
      if (i >= this.PlacesBackgroundArray.size)
        return;

      int j;
      for (j = 0; j < this.DevicesBackgroundArray.size; j++)
        {
          this.DevicesBackgroundArray[j].hide ();
        }
      TrashBackground.hide ();

      this.PlacesBackgroundArray[this.current_tab_index].hide ();
      this.current_tab_index = i;
      this.PlacesBackgroundArray[this.current_tab_index].show ();
    }

    public void on_signal_device_active_icon (int i)
    {
      if (i >= this.DevicesBackgroundArray.size)
        return;

      int j;
      for (j = 0; j < this.PlacesBackgroundArray.size; j++)
        {
          this.PlacesBackgroundArray[j].hide ();
        }
      TrashBackground.hide ();

      this.DevicesBackgroundArray[this.current_tab_index].hide ();
      this.current_tab_index = i;
      this.DevicesBackgroundArray[this.current_tab_index].show ();
    }

    public void on_signal_trash_active_icon ()
    {
      int j;
      for (j = 0; j < this.PlacesBackgroundArray.size; j++)
        {
          this.PlacesBackgroundArray[j].hide ();
        }
      for (j = 0; j < this.DevicesBackgroundArray.size; j++)
        {
          this.DevicesBackgroundArray[j].hide ();
        }

      TrashBackground.show ();
    }

  }
}

