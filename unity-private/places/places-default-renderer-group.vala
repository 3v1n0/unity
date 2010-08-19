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

namespace Unity.Places
{
  public class DefaultRendererGroup : ExpandingBin
  {
    static const float PADDING = 24.0f;
    static const int   SPACING = 0;
    static const int   OMG_FOOTEL_SUCKS_CANT_HANDLE_MANY_TEXTURES = 100;

    public uint      group_id       { get; construct; }
    public string    group_renderer { get; construct; }
    public string    display_name   { get; construct; }
    public string    icon_hint      { get; construct; }
    public Dee.Model results        { get; construct; }

    private Ctk.VBox      vbox;
    private Ctk.Button    title_button;
    private Ctk.HBox      title_box;
    private Ctk.Image     icon;
    private Ctk.Text      text;
    private Expander      expander;
    private Ctk.Button    sep;
    private Ctk.IconView  renderer;

    private MoreResultsButton? more_results_button;

    /* Some caching to speed up lookups */
    private uint          n_results = 0;
    private bool          dirty = false;

    private bool          allow_expand = true;

    public signal void activated (string uri, string mimetype);

    public DefaultRendererGroup (uint      group_id,
                                 string    group_renderer,
                                 string    display_name,
                                 string    icon_hint,
                                 Dee.Model results)
    {
      Object (group_id:group_id,
              group_renderer:group_renderer,
              display_name:display_name,
              icon_hint:icon_hint,
              results:results);
    }

    construct
    {
      padding = { 0.0f, 0.0f, PADDING, 0.0f};
      hide ();

      if (group_renderer == "UnityLinkGroupRenderer")
        allow_expand = false;
      else if (group_renderer == "UnityFolderGroupRenderer")
        bin_state = ExpandingBinState.EXPANDED;

      vbox = new Ctk.VBox (SPACING);
      vbox.spacing = SPACING;
      vbox.homogeneous = false;
      add_actor (vbox);
      vbox.show ();
  
      title_button = new Ctk.Button (Ctk.Orientation.HORIZONTAL);
      title_button.padding = { 4.0f, 6.0f, 4.0f, 6.0f };
      vbox.pack (title_button, false, false);
      title_button.show ();
      var title_bg = new StripeTexture (null);
      title_button.set_background_for_state (Ctk.ActorState.STATE_PRELIGHT,
                                             title_bg);
      title_button.notify["state"].connect (() => {
        if (title_button.state == Ctk.ActorState.STATE_PRELIGHT)
          sep.opacity = 0;
        else
          sep.opacity = 255;

        unowned GLib.SList<unowned Ctk.Effect> effects = title_button.get_effects ();
        foreach (unowned Ctk.Effect effect in effects)
          effect.set_invalidate_effect_cache (true);
      });
      var glow = new Ctk.EffectGlow ();
      glow.set_factor (1.0f);
      glow.set_margin (3);
      title_button.add_effect (glow);
      
      title_box = new Ctk.HBox (5);
      title_button.add_actor (title_box);
      title_box.show ();

      icon = new Ctk.Image (22);
      icon.set_from_filename (Config.PKGDATADIR + "/favourites.png");
      title_box.pack (icon, false, false);
      icon.show ();

      text = new Ctk.Text (display_name);
      text.set_markup ("<big>" + display_name + "</big>");
      title_box.pack (text, false, false);
      text.show ();

      expander = new Expander ();
      expander.opacity = 0;
      title_box.pack (expander, false, true);
      expander.show ();

      sep = new Ctk.Button (Ctk.Orientation.HORIZONTAL);
      var rect = new Clutter.Rectangle.with_color ({ 255, 255, 255, 100 });
      sep.add_actor (rect);
      rect.height = 1.0f;
      vbox.pack (sep, false, false);
      sep.show ();
      glow = new Ctk.EffectGlow ();
      glow.set_factor (1.0f);
      glow.set_margin (5);
      sep.add_effect (glow);

      title_button.clicked.connect (() => {
        if (n_results <= renderer.get_n_cols () || allow_expand == false)
          return;

        if (bin_state == ExpandingBinState.UNEXPANDED)
          {
            bin_state = ExpandingBinState.EXPANDED;
            expander.expanding_state = Expander.State.EXPANDED;
          }
        else
          {
            bin_state = ExpandingBinState.UNEXPANDED;
            expander.expanding_state = Expander.State.UNEXPANDED;
          }
      });
      title_button.motion_event.connect (() => {
        if (dirty && allow_expand)
          {
            var children = renderer.get_children ();
            foreach (Clutter.Actor child in children)
              {
                Tile tile = child as Tile;
                tile.about_to_show ();
              }
              dirty = false;
          }

        return false;
      });

      renderer = new Ctk.IconView ();
      renderer.padding = { 12.0f, 0.0f, 0.0f, 0.0f };
      renderer.spacing = 24;
      vbox.pack (renderer, true, true);
      renderer.show ();
      renderer.set ("auto-fade-children", true);
      renderer.notify["n-cols"].connect (on_n_cols_changed);

      if (group_renderer == "UnityLinkGroupRenderer")
        {
          allow_expand = false;

          more_results_button = new MoreResultsButton ();
          more_results_button.padding = { PADDING/4,
                                          PADDING/2,
                                          PADDING/4,
                                          PADDING/2 };
          vbox.pack (more_results_button, false, false);
          more_results_button.show ();

          more_results_button.clicked.connect (() =>
            {
              /* FIXME:!!!
               * This is not the way we'll end up doing this. This is a stop-gap
               * until we have better support for signalling things through
               * a place
               */
              Controller cont = Testing.ObjectRegistry.get_default ().lookup ("UnityPlacesController")[0] as Controller;
              PlaceSearchBar search_bar = (Testing.ObjectRegistry.get_default ().lookup ("UnityPlacesSearchBar"))[0] as PlaceSearchBar;

              if (cont is Controller && search_bar is PlaceSearchBar)
                {
                  string text = search_bar.get_search_text ();
                  cont.activate_entry (display_name);
                  search_bar.search (text);
                }
            });
        }
      else if (group_renderer == "UnityFolderGroupRenderer")
        {
          title_button.hide ();
          sep.hide ();
          bin_state = ExpandingBinState.EXPANDED;
        }

      unowned Dee.ModelIter iter = results.get_first_iter ();
      while (!results.is_last (iter))
        {
          if (interesting (iter))
            on_result_added (iter);

          iter = results.next (iter);
        }

      results.row_added.connect (on_result_added);
      results.row_removed.connect (on_result_removed);

    }

    private override void allocate (Clutter.ActorBox        box,
                                    Clutter.AllocationFlags flags)
    {
      base.allocate (box, flags);

      /* Update the unexpanded height if necessary */
      /* FIXME: Can we please add some nice methods to CluTK which allow
       * doing something like $clutk_container.get_nth_child (), and so
       * bypass the stupid get_children stuff. In any case, cache the result
       */
      var children = renderer.get_children ();
      var child = children.nth_data (0) as Clutter.Actor;
      if (child is Clutter.Actor &&
          child.height != unexpanded_height)
        {
          var h = more_results_button != null ? more_results_button.height : 0;
          unexpanded_height = title_button.height + 1.0f + child.height + h;
        }
    }

    /*
     * Private Methods
     */
    private void on_result_added (Dee.ModelIter iter)
    {
      if (!interesting (iter))
        return;

      if (n_results == OMG_FOOTEL_SUCKS_CANT_HANDLE_MANY_TEXTURES)
        return;
      var button = new Tile (iter,
                             results.get_string (iter, 0),
                             results.get_string (iter, 1),
                             results.get_string (iter, 3),
                             results.get_string (iter, 4),
                             results.get_string (iter, 5));
      renderer.add_actor (button);
      button.show ();

      if (bin_state == ExpandingBinState.EXPANDED)
        {
          button.about_to_show ();
        }

      button.activated.connect ((u, m) => { activated (u, m); });

      add_to_n_results (1);

      if (bin_state == ExpandingBinState.CLOSED)
        {
          if (group_renderer == "UnityFolderGroupRenderer")
            bin_state = ExpandingBinState.EXPANDED;
          else
            bin_state = ExpandingBinState.UNEXPANDED;
          show ();
        }

      dirty = true;
    }

    private void on_result_removed (Dee.ModelIter iter)
    {
     if (!interesting (iter))
        return;

      var children = renderer.get_children ();
      foreach (Clutter.Actor actor in children)
        {
          Tile tile = actor as Tile;

          if (tile.iter == iter)
            {
              actor.destroy ();
              add_to_n_results (-1);
              break;
            }
        }

      if (n_results < 1)
        {
          bin_state = ExpandingBinState.CLOSED;
        }
    }

    private bool interesting (Dee.ModelIter iter)
    {
      return (results.get_uint (iter, 2) == group_id);
    }

    private void add_to_n_results (int i)
    {
      n_results += i;

      if (n_results > renderer.get_n_cols ())
        {
          if (allow_expand)
            {
              expander.animate (Clutter.AnimationMode.EASE_IN_SINE, 200,
                                "opacity", 255);
            }
          else if (more_results_button != null)
            {
              more_results_button.count = n_results - renderer.get_n_cols ();
            }
        }
      else
        {
          expander.animate (Clutter.AnimationMode.EASE_OUT_SINE, 200,
                                "opacity", 0);

          if (more_results_button != null)
            {
              more_results_button.count = 0;
            }
        }
    }

    private void on_n_cols_changed ()
    {
      var n_cols = renderer.get_n_cols ();

      if (bin_state == ExpandingBinState.UNEXPANDED)
        {
          var children = renderer.get_children ();
          int i = 0;

          foreach (Clutter.Actor child in children)
            {
              Tile tile = child as Tile;
              if (i < n_cols)
                {
                  tile.about_to_show ();
                  i++;
                }
              else
                break;
            }
        }
      else
        {
          var children = renderer.get_children ();
   
          foreach (Clutter.Actor child in children)
            {
              Tile tile = child as Tile;
              tile.about_to_show ();
            }
        }

      add_to_n_results (0);
    }
  }

  public class Expander : Ctk.Bin
  {
    public enum State
    {
      EXPANDED,
      UNEXPANDED
    }

    private float arrow_size = 12.0f;
    private float arrow_quart = 3.0f;

    private CairoCanvas arrow;

    public State expanding_state {
      get { return _state; }
      set {
        if (_state != value)
          {
            _state = value;
            arrow.update ();
          }
      }
    }

    private State _state = State.UNEXPANDED;

    public Expander ()
    {
      Object ();
    }

    construct
    {
      arrow = new CairoCanvas (paint_arrow);
      add_actor (arrow);
      arrow.show ();
    }

    private override void allocate (Clutter.ActorBox box,
                                    Clutter.AllocationFlags flags)
    {
      Clutter.ActorBox child_box = Clutter.ActorBox ();

      base.allocate (box, flags);

      child_box.x1 = ((box.x2 - box.x1)/2.0f) - arrow_quart *2;
      child_box.x2 = child_box.x1 + arrow_size;
      child_box.y1 = ((box.y2 - box.y1)/2.0f) - arrow_quart *2;
      child_box.y2 = child_box.y1 + arrow_size;

      arrow.allocate (child_box, flags);
    }

    private override void get_preferred_height (float for_width,
                                           out float mheight,
                                           out float nheight)
    {
      mheight = arrow_size;
      nheight = arrow_size;
    }

    private override void get_preferred_width (float for_height,
                                          out float mwidth,
                                          out float nwidth)
    {
      mwidth = arrow_size;
      nwidth = arrow_size;
    }

    private void paint_arrow (Cairo.Context cr, int width, int height)
    {
      cr.set_operator (Cairo.Operator.CLEAR);
      cr.paint ();

      cr.set_operator (Cairo.Operator.OVER);
      cr.set_line_width (1.0f);
      cr.translate (-0.5, -0.5);
      cr.set_source_rgba (1.0f, 1.0f, 1.0f, 1.0f);

      if (_state == State.UNEXPANDED)
        {
          cr.move_to (arrow_quart, arrow_quart);
          cr.line_to (arrow_size - arrow_quart, arrow_size/2.0f);
          cr.line_to (arrow_quart, arrow_size - arrow_quart);
          cr.close_path ();
        }
      else
        {
          cr.move_to (arrow_quart, arrow_quart);
          cr.line_to (arrow_size - arrow_quart, arrow_quart);
          cr.line_to (arrow_size /2.0f, arrow_size - arrow_quart);
          cr.close_path ();
        }

      cr.fill ();
    }
  }

  public class MoreResultsButton : Ctk.Button
  {
    public uint count {
      get { return _count; }
      set {
        _count = value;
        text.text = _("See %u more results").printf (_count);

        animate (Clutter.AnimationMode.EASE_OUT_QUAD, 200,
                            "opacity", count == 0 ? 0:255);
      }
    }

    private uint _count = 0;
    private Ctk.Text text;

    public MoreResultsButton ()
    {
      Object (orientation:Ctk.Orientation.HORIZONTAL);
    }

    construct
    {
      var bg = new CairoCanvas (paint_bg);
      set_background (bg);

      text = new Ctk.Text ("");
      add_actor (text);
      text.show ();
    }

    private override void get_preferred_height (float for_width,
                                                out float mheight,
                                                out float nheight)
    {
      if (count == 0)
        {
          mheight = 0.0f;
          nheight = 0.0f;
        }
      else
        {
          mheight = 28 + padding.top + padding.bottom;
          nheight = mheight;
        }
    }

    private void paint_bg (Cairo.Context cr, int width, int height)
    {
      var vpad = padding.top;
      var hpad = padding.left;
      float nwidth;
      text.get_preferred_width (height, null, out nwidth);

      cr.translate (0.5, 0.5);
      cr.rectangle (0.0,
                    vpad,
                    hpad + nwidth + hpad,
                    height - vpad - vpad);
      cr.set_source_rgba (1.0, 1.0, 1.0, 0.2);
      cr.fill_preserve ();

      cr.set_line_width (1.5);
      cr.set_source_rgba (1.0, 1.0, 1.0, 0.5);
      cr.stroke ();
    }
  }

  public class Tile : Ctk.Button
  {
    static const int ICON_SIZE = 48;
    static const string DEFAULT_ICON = "text-x-preview";

    public unowned Dee.ModelIter iter { get; construct; }

    public string  display_name { get; construct; }
    public string? icon_hint    { get; construct; }
    public string  uri          { get; construct; }
    public string? mimetype     { get; construct; }
    public string? comment      { get; construct; }

    private bool shown = false;

    public signal void activated (string uri, string mimetype);

    public Tile (Dee.ModelIter iter,
                 string        uri,
                 string?       icon_hint,
                 string?       mimetype,
                 string        display_name,
                 string?       comment)
    {
      Object (orientation:Ctk.Orientation.VERTICAL,
              iter:iter,
              display_name:display_name,
              icon_hint:icon_hint,
              uri:uri,
              mimetype:mimetype,
              comment:comment);
    }

    construct
    {
      unowned Ctk.Text text = get_text ();
      text.ellipsize = Pango.EllipsizeMode.MIDDLE;
    }

    public void about_to_show ()
    {
      if (shown)
        return;
      shown = true;

      Timeout.add (0, () => {
        set_label (display_name);
        set_icon ();
        return false;
      });
    }

    private override void get_preferred_width (float for_height,
                                               out float mwidth,
                                               out float nwidth)
    {
      mwidth = 150.0f;
      nwidth = 150.0f;
    }

    private override void clicked ()
    {
      activated (uri, mimetype);
    }
    
    private void set_icon ()
    {
      var cache = PixbufCache.get_default ();

      if (icon_hint != null && icon_hint != "")
        {
          cache.set_image_from_gicon_string (get_image (), icon_hint, ICON_SIZE);
        }
      else if (mimetype != null && mimetype != "")
        {
          var icon = GLib.g_content_type_get_icon (mimetype);
          cache.set_image_from_gicon_string (get_image (), icon.to_string(), ICON_SIZE);
        }
      else
        {
          cache.set_image_from_icon_name (get_image (), DEFAULT_ICON, ICON_SIZE);
        }
    }
  }
}

