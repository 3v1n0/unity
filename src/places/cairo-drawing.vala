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
        /* Clear */
        /*cairoctx.set_operator (Cairo.Operator.CLEAR);
        cairoctx.paint ();
        cairoctx.set_operator (Cairo.Operator.OVER);*/

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
      //this.add_effect (effect_glow);
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
}

