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

    /* Margin outside of the cairo texture. We draw outside to complete the line loop
     * and we don't want the line loop to be visible in some parts of the screen. 
     * */
    private int Margin = 20;

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

      cairotxt = new Clutter.CairoTexture(PlaceW, PlaceH);
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
    private Gee.ArrayList<PlacesBackground> background_array;
    private int current_tab_index;

    /* These parameters are temporary until we get the right metrics for the places bar */

    public override void allocate (Clutter.ActorBox        box, 
                                   Clutter.AllocationFlags flags)
    {
      base.allocate (box, flags);
      Clutter.ActorBox child_box = { 0.0f, 0.0f, 0.0f, 0.0f };
      int bar_icon_size = 48;
      int icon_margin = 4;
      int offset = 94; /* offset to first icon */
      int NewPlaceWidth = (int) (box.x2 - box.x1);

      child_box.x1 = offset; //this.padding.left;
      child_box.x2 = box.x2 - box.x1 - this.padding.left - this.padding.right;
      child_box.y1 = this.padding.top + 8;
      child_box.y2 = child_box.y1 + bar_icon_size;
      this.bar_view.allocate (child_box, flags);

      child_box.y1 = this.padding.top + bar_icon_size;
      child_box.y2 = box.y2 - box.y1 - this.padding.top -this.padding.bottom - bar_icon_size;

      this.default_view.allocate (child_box, flags);

      child_box.x1 = 0;
      child_box.x2 = NewPlaceWidth;
      child_box.y1 = 0;
      child_box.y2 = 55;

      int spacing = this.bar_view.spacing;
      int i;
      for (i = 0; i < this.background_array.size; i++)
        {
          if (this.background_array[i].PlaceWidth != NewPlaceWidth)
            {
              this.background_array[i].CreatePlacesBackground (NewPlaceWidth, 55,
              offset + i*(bar_icon_size + spacing) - icon_margin,
              2*icon_margin + bar_icon_size);
            }
          this.background_array[i].allocate (child_box, flags);
        }
    }

    public View ()
    {
      int i;
      int NunItems;
      PlacesBackground placesbackground;

      this.background_array = new Gee.ArrayList<PlacesBackground> ();

      this.current_tab_index = 0;
      this.orientation  = Ctk.Orientation.VERTICAL;
      this.bar_view     = new Unity.Places.Bar.View ();
      this.bar_view.sig_active_icon_index.connect(this.on_signal_active_icon);
      
      this.default_view = new Unity.Places.Default.View ();

      NunItems = this.bar_view.get_number_of_items ();      
      for (i = 0; i < NunItems; i++)
      {
        placesbackground = new PlacesBackground ();
        this.add_actor (placesbackground);
        this.background_array.add (placesbackground);
        placesbackground.hide ();
      }
      this.background_array[0].show ();

      this.add_actor (this.bar_view);
      this.add_actor (this.default_view);

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

    public void on_signal_active_icon (int i)
    {
      if (i >= this.background_array.size)
        return;

      this.background_array[this.current_tab_index].hide ();
      this.current_tab_index = i;
      this.background_array[this.current_tab_index].show ();
    }
  }
}

