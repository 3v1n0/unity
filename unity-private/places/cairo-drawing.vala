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
 * Authored by Jay Taoko <jay.taoko@canonical.com>
 *
 */

namespace Unity.Places.CairoDrawing
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
    private int MenuW = 100;

    private int MenuBottom; /* PlaceY + MenuH */

    private int TabH = 50;

    /* The squirl area width */
    private int SquirlW = 66;

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
      int radius = 8;
      cairoctx.line_to (PlaceX + PlaceW + Margin, PlaceY + MenuH);
      cairoctx.line_to (PlaceX + PlaceW - MenuW + radius, PlaceY + MenuH);

      int arc_center_x = PlaceX + PlaceW - MenuW + radius;
      int arc_center_y = PlaceY + MenuH + radius;

      cairoctx.arc_negative ( (double)arc_center_x, (double)arc_center_y, radius, 3.0*Math.PI/2.0, Math.PI);

      cairoctx.line_to (PlaceX + PlaceW - MenuW, PlaceY + PlaceH - radius);

      arc_center_x = PlaceX + PlaceW - MenuW - radius;
      arc_center_y = PlaceY + PlaceH - radius;
      cairoctx.arc ( (double)arc_center_x, (double)arc_center_y, radius, 0.0, Math.PI/2.0);
    }

    void DrawTab(Cairo.Context cairoctx, TabRect tab)
    {
      int radius = 5;
      int radius_small = 5;
      int arc_center_x;
      int arc_center_y;

      cairoctx.line_to (tab.right + radius_small, PlaceBottom );

      arc_center_x = tab.right + radius_small;
      arc_center_y = PlaceBottom - radius_small;
      cairoctx.arc ( (double)arc_center_x, (double)arc_center_y, radius_small, Math.PI/2.0, Math.PI);

      cairoctx.line_to (tab.right, tab.top + radius);

      arc_center_x = tab.right - radius;
      arc_center_y = tab.top + radius;
      cairoctx.arc_negative ( (double)arc_center_x, (double)arc_center_y, radius, 0.0, 3.0*Math.PI/2.0);

      cairoctx.line_to (tab.left + radius, tab.top);

      arc_center_x = tab.left + radius;
      arc_center_y = tab.top + radius;
      cairoctx.arc_negative ( (double)arc_center_x, (double)arc_center_y, radius, 3.0*Math.PI/2.0, Math.PI);

      cairoctx.line_to (tab.left, PlaceBottom - radius_small);

      arc_center_x = tab.left - radius_small;
      arc_center_y = PlaceBottom - radius_small;
      cairoctx.arc ( (double)arc_center_x, (double)arc_center_y, radius_small, 0.0, Math.PI/2.0);
    }

    void DrawSquirl(Cairo.Context cairoctx)
    {
      int radius_small = 5;
      int arc_center_x;
      int arc_center_y;

      cairoctx.line_to (PlaceX + SquirlW + radius_small, PlaceBottom);

      arc_center_x = PlaceX + SquirlW + radius_small;
      arc_center_y = PlaceBottom - radius_small;
      cairoctx.arc ( (double)arc_center_x, (double)arc_center_y, radius_small, Math.PI/2.0, Math.PI);

      cairoctx.line_to (PlaceX + SquirlW, MenuBottom + radius_small );

      arc_center_x =  PlaceX + SquirlW - radius_small;
      arc_center_y = MenuBottom + radius_small;
      cairoctx.arc_negative ( (double)arc_center_x, (double)arc_center_y, radius_small, 0, 3.0*Math.PI/2.0);

      cairoctx.line_to (PlaceX - Margin, MenuBottom);
    }

    public PlacesBackground ()
    {
      PlaceWidth = 0;
    }

    public void create_places_background (int WindowWidth,
      int WindowHeight,
      int TabPositionX,
      int TabWidth,
      int menu_width)
    {
      PlaceWidth = WindowWidth;
      PlaceW = WindowWidth;
      PlaceBottom = PlaceY + PlaceH;
      MenuBottom = PlaceY + MenuH;

      MenuW = menu_width;

      if (this.get_child () is Clutter.Actor)
      {
        this.remove_actor (this.get_child ());
      }

      cairotxt = new Clutter.CairoTexture(PlaceW, PlaceH + Margin);
      Cairo.Context cairoctx = cairotxt.create();
      {
        cairoctx.set_operator (Cairo.Operator.CLEAR);
        cairoctx.paint ();
        cairoctx.set_operator (Cairo.Operator.OVER);
        cairoctx.translate (0.5f, 0.5f);

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

        cairoctx.clip ();

        Cairo.Surface surface = new Cairo.ImageSurface.from_png (Unity.PKGDATADIR + "/dash_background.png");
        Cairo.Pattern pattern = new Cairo.Pattern.for_surface (surface);
        pattern.set_extend (Cairo.Extend.REPEAT);
        cairoctx.set_source (pattern);

        cairoctx.paint_with_alpha (0.1);
      }

      cairotxt.set_opacity (0xFF);
      this.add_actor (cairotxt);

      /* Remove all effects set on this actor */
      this.remove_all_effects ();

      /* Create a new effect and add it to this actor */
      /* The new effect will use the newly created drawing as a base */
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


  public class PlacesVSeparator : Ctk.Bin
  {
    public int Width = 0;
    public int Height = 0;

    public Clutter.CairoTexture cairotxt;
    public PlacesVSeparator ()
    {
    }

    public void CreateSeparator (int W, int H)
    {
      Width = W;
      Height = H;

      if (this.get_child () is Clutter.Actor)
      {
        this.remove_actor (this.get_child ());
      }

      cairotxt = new Clutter.CairoTexture(Width, Height);
      Cairo.Context cairoctx = cairotxt.create();
      {
        cairoctx.set_source_rgba (1, 1, 1, 1.0);
        cairoctx.set_line_width (1.0);

        cairoctx.move_to (Width/2.0, 0);
        cairoctx.line_to (Width/2.0, Height);

        cairoctx.stroke ();
        cairoctx.set_source_rgba (1, 1, 1, 0.15);
      }

      cairotxt.set_opacity (0xFF);
      this.add_actor (cairotxt);

      /* Remove all effects set on this actor */
      this.remove_all_effects ();

      /* Create a new effect and add it to this actor */
      /* The new effect will use the newly created drawing as a base */
      Ctk.EffectGlow effect_glow = new Ctk.EffectGlow ();
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

  public class PlacesHSeparator : Ctk.Bin
  {
    public int Width = 0;
    public int Height = 0;

    public Clutter.CairoTexture cairotxt;
    public PlacesHSeparator ()
    {
    }

    public void CreateSeparator (int W, int H)
    {
      Width = W;
      Height = H;

      if (this.get_child () is Clutter.Actor)
      {
        this.remove_actor (this.get_child ());
      }

      cairotxt = new Clutter.CairoTexture(Width, Height);
      Cairo.Context cairoctx = cairotxt.create();
      {
        cairoctx.set_source_rgba (1, 1, 1, 1.0);
        cairoctx.set_line_width (1.0);

        cairoctx.move_to (0, Height/2.0);
        cairoctx.line_to (Width, Height/2.0);

        cairoctx.stroke ();
        cairoctx.set_source_rgba (1, 1, 1, 0.15);
      }

      cairotxt.set_opacity (0xFF);
      this.add_actor (cairotxt);

      /* Remove all effects set on this actor */
      this.remove_all_effects ();

      /* Create a new effect and add it to this actor */
      /* The new effect will use the newly created drawing as a base */

      Ctk.EffectGlow effect_glow = new Ctk.EffectGlow ();
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

  public class RectangleBox : Ctk.Bin
  {
    public int Width = 0;
    public int Height = 0;

    public Clutter.CairoTexture cairotxt;
    public RectangleBox ()
    {
    }

    public void CreateRectangleBox (int W, int H)
    {
      Width = W;
      Height = H;

      if (this.get_child () is Clutter.Actor)
      {
        this.remove_actor (this.get_child ());
      }

      cairotxt = new Clutter.CairoTexture(Width, Height);
      Cairo.Context cairoctx = cairotxt.create();
      {
        cairoctx.set_operator (Cairo.Operator.CLEAR);
        cairoctx.paint ();
        cairoctx.set_operator (Cairo.Operator.OVER);
        cairoctx.translate (0.5f, 0.5f);

        cairoctx.set_source_rgba (1, 1, 1, 1.0);
        cairoctx.set_line_width (1.0);

        cairoctx.move_to (1, 1);
        cairoctx.line_to (Width-1, 1);
        cairoctx.line_to (Width-1, Height-1);
        cairoctx.line_to (1, Height-1);
        cairoctx.line_to (1, 1);
        cairoctx.stroke ();
      }

      cairotxt.set_opacity (0xFF);
      this.add_actor (cairotxt);

      /* Remove all effects set on this actor */
      this.remove_all_effects ();

      /* Create a new effect and add it to this actor */
      /* The new effect will use the newly created drawing as a base */
      Ctk.EffectGlow effect_glow = new Ctk.EffectGlow ();
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

  public class EntryBackground : Ctk.Bin
  {
    public int Width = 0;
    public int Height = 0;

    public Clutter.CairoTexture cairotxt;
    public EntryBackground ()
    {
      cairotxt = new Clutter.CairoTexture (120, 24);
      add_actor (cairotxt);
    }

    public void create_search_entry_background (int W, int H)
    {
      Width = W;
      Height = H;

      Cairo.Context cairoctx = cairotxt.create();
      {
        cairoctx.set_operator (Cairo.Operator.CLEAR);
        cairoctx.paint ();
        cairoctx.set_operator (Cairo.Operator.OVER);
        cairoctx.translate (-0.5f, -0.5f);

        cairoctx.set_source_rgba (1.0, 1.0, 1.0, 0.9);
        cairoctx.set_line_width (1.0);

        cairoctx.move_to (10, 1);
        cairoctx.line_to (Width-10, 1);

        cairoctx.curve_to ((double)(Width-5), 1.0, (double)(Width-1), 5.0, (double)(Width-1), (double)(Height/2.0));
        cairoctx.curve_to ((double)(Width-1), (double)(Height/2.0 + 5.0), (double)(Width-5), (double)(Height-2), (double)(Width-10), (double)(Height-2));

        cairoctx.line_to (10, Height-2);

        cairoctx.curve_to (5.0, (double)(Height-2), 1.0, (double)(Height/2.0 + 5.0), 1.0, (double)(Height/2.0));
        cairoctx.curve_to (1.0, 5.0, 5.0, 1.0, 10.0, 1.0);


        cairoctx.fill ();
      }
      cairotxt.set_opacity (0xFF);
    }

    /*private override void allocate (Clutter.ActorBox        box,
                                    Clutter.AllocationFlags flags)
    {
      base.allocate (box, flags);
      create_search_entry_background ((int)(box.x2 - box.x1), (int)(box.y2 - box.y1));
    }*/

    construct
    {
    }
  }
}

