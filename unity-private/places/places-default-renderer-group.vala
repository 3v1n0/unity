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
    private Ctk.HBox      title_box;
    private Ctk.Image     icon;
    private Ctk.Text      text;
    private Ctk.Image     expander;
    private Clutter.Actor sep;
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

      vbox = new Ctk.VBox (SPACING);
      vbox.spacing = SPACING;
      vbox.homogeneous = false;
      add_actor (vbox);
      vbox.show ();

      title_box = new Ctk.HBox (5);
      vbox.pack (title_box, false, false);
      title_box.show ();
      title_box.reactive = true;

      icon = new Ctk.Image (22);
      icon.set_from_filename (Config.PKGDATADIR + "/favourites.png");
      title_box.pack (icon, false, false);
      icon.show ();

      text = new Ctk.Text (display_name);
      title_box.pack (text, true, true);
      text.show ();

      expander = new Ctk.Image (22);
      expander.set_from_filename (Config.PKGDATADIR + "/maximize_up.png");
      expander.opacity = 0;
      title_box.pack (expander, false, true);
      expander.show ();

      sep = new Clutter.Rectangle.with_color ({ 255, 255, 255, 255 });
      sep.height = 1;
      vbox.pack (sep, false, false);
      sep.show ();

      title_box.button_release_event.connect (() => {
        if (n_results <= renderer.get_n_cols () || allow_expand == false)
          return true;

        if (bin_state == ExpandingBinState.UNEXPANDED)
          {
            bin_state = ExpandingBinState.EXPANDED;
            expander.set_from_filename (Config.PKGDATADIR + "/minimize_up.png");
          }
        else
          {
            bin_state = ExpandingBinState.UNEXPANDED;
            expander.set_from_filename (Config.PKGDATADIR + "/maximize_up.png");
          }
        return true;
      });
      title_box.motion_event.connect (() => {
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
          title_box.hide ();
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
          unexpanded_height = title_box.height + 1.0f + child.height + h;
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
        button.about_to_show ();

      button.activated.connect ((u, m) => { activated (u, m); });

      add_to_n_results (1);

      if (bin_state == ExpandingBinState.CLOSED)
        {
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

      add_to_n_results (0);
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

