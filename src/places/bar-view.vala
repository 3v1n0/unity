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
 *             Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

namespace Unity.Places.Bar
{
  const float PANEL_HEIGHT    = 24;
  const int   ICON_SIZE       = 48;
  const float ICON_VIEW_WIDTH = 80.0f;
  const float ICON_VIEW_Y1    = 8.0f;
  const float QL_PAD          = 12.0f;

  const string TRASH_FILE = Unity.PKGDATADIR + "/trash.png";

  public class View : Ctk.Box
  {
    public Places.Model model { get; construct; }

    private Gee.ArrayList<PlaceIcon> places_icons;
    private PlaceIcon                trash_icon;

    private Unity.Places.CairoDrawing.PlacesVSeparator separator;
    private Unity.Places.CairoDrawing.PlacesBackground bg;

    public View (Places.Model model)
    {
      Ctk.EffectGlow         glow;
      Clutter.Color          white = {255, 255, 255, 255};

      Object (model:model);

      /* We do our own allocation, but this doesn't hurt */
      this.homogeneous  = false;
      this.orientation  = Ctk.Orientation.HORIZONTAL;

      /* The background of the entire places bar */
      this.bg = new Unity.Places.CairoDrawing.PlacesBackground ();
      this.bg.set_parent (this);
      this.bg.show ();

      /* This'll be populated in an idle by the model */
      this.places_icons = new Gee.ArrayList<PlaceIcon> ();
      this.model.place_added.connect (this.on_place_added);

      /* Create trash icon */
      this.trash_icon = new PlaceIcon (ICON_SIZE,
                                       "Trash",
                                       TRASH_FILE,
                                       "Your piece of waste");
      glow = new Ctk.EffectGlow ();
      glow.set_color (white);
      glow.set_factor (1.0f);
      glow.set_margin (6);
      this.trash_icon.add_effect (glow);

      this.pack (this.trash_icon, false, false);
      this.trash_icon.button_release_event.connect (this.on_button_release);

      /* Create the separator */
      this.separator = new Unity.Places.CairoDrawing.PlacesVSeparator ();
      this.pack (this.separator, false, false);

      this.show_all ();
    }

    private void on_place_added (Place place)
    {
      Clutter.Color white = {255, 255, 255, 255};

      var icon = new PlaceIcon.from_place (ICON_SIZE, place);
      this.places_icons.add (icon);

      var glow = new Ctk.EffectGlow ();
      glow.set_color (white);
      glow.set_factor (1.0f);
      glow.set_margin (6);
      icon.add_effect (glow);

      this.pack (icon, false, false);
      icon.button_release_event.connect (this.on_button_release);

      if (this.places_icons.size == 1)
        {
          Clutter.Actor stage = icon.get_stage ();
          this.bg.create_places_background ((int)stage.width,
                                            (int)stage.height,
                                            (int)(this.padding.left + QL_PAD),
                                            (int)ICON_VIEW_WIDTH);

          icon.place.active = true;
        }
    }

    public override void map ()
    {
      base.map ();
      this.bg.map ();
    }

    public override void unmap ()
    {
      base.unmap ();
      this.bg.unmap ();
    }

    public override void paint ()
    {
     this.bg.paint ();
     base.paint ();
    }

    public override void allocate (Clutter.ActorBox        box,
                                   Clutter.AllocationFlags flags)
    {
      Clutter.ActorBox        child_box = {0, 0, 0, 0};
      child_box.x1 = 0.0f;
      child_box.x2 = box.x2 - box.x1;
      child_box.y1 = 0.0f;
      child_box.y2 = box.y2 - box.y1;

      this.bg.allocate (child_box, flags);

      /* Allocate the places icons */
      var n_places = 0;
      var lpadding = this.padding.left + QL_PAD;

      child_box.y1 = ICON_VIEW_Y1;
      child_box.y2 = child_box.y1 + ICON_SIZE;

      foreach (PlaceIcon place in this.places_icons)
        {
          child_box.x1 = lpadding +  (ICON_VIEW_WIDTH * n_places);
          child_box.x2 = child_box.x1 +  ICON_VIEW_WIDTH;

          place.allocate (child_box, flags);

          n_places++;
        }

      /* Allocate the Trash */
      child_box.x1 = box.x2 - box.x1 - 266 - ICON_VIEW_WIDTH;
      child_box.x2 = child_box.x1 + ICON_VIEW_WIDTH;
      this.trash_icon.allocate (child_box, flags);

      /* Allocate the seperator */
      child_box.x1 -= 12.0f;
      child_box.x2 = child_box.x1 + 5;
      child_box.y1 = 10;
      child_box.y2 = ICON_SIZE;
      this.separator.allocate (child_box, flags);
    }

    public bool on_button_release (Clutter.Event event)
    {
      Clutter.Actor actor;
      actor = event.button.source;

      if (actor is PlaceIcon)
        {
          PlaceIcon     icon = actor as PlaceIcon;

          /* Do something with the click */
          if (actor == this.trash_icon)
            {
              try
                {
                  Process.spawn_command_line_async ("xdg-open trash:///");
                }
              catch (SpawnError e)
                {
                  warning ("Unable to show Trash: %s", e.message);
                }
            }
          else if (icon.place is Place)
            {
              Clutter.Actor stage = actor.get_stage ();

              /* Update the background */
              this.bg.create_places_background ((int)stage.width,
                                                (int)stage.height,
                                                (int)actor.x,
                                                80);

              /* Set the place as active, unset the others */
              foreach (PlaceIcon picon in this.places_icons)
                {
                  if (picon.place is Place)
                    picon.place.active = (picon == icon) ? true : false;
                }
            }

          return true;
        }

      return false;
    }
  }

  public class PlaceIcon : Ctk.Image
  {
    public Place? place { get; set; }

    public PlaceIcon (int       width,
                      string name,
                      string    icon_name,
                      string    tooltip)
    {
      Object (size:width);

      this.set_from_filename (icon_name);
      this.reactive = true;
    }

    public PlaceIcon.from_place (int size, Place place)
    {
      this(size, place.name, place.icon_name, "");
      this.place = place;
    }
  }
}
