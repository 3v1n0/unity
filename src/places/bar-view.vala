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

namespace Unity.Places.Bar
{
  const string APPS_FILE = Unity.PKGDATADIR + "/applications.png";
  const string FILES_FILE = Unity.PKGDATADIR + "/files.png";
  const string GADGETS_FILE = Unity.PKGDATADIR + "/gadgets.png";
  const string HOME_FILE = Unity.PKGDATADIR + "/home.png";
  const string MUSIC_FILE = Unity.PKGDATADIR + "/music.png";
  const string PEOPLE_FILE = Unity.PKGDATADIR + "/people.png";
  const string PHOTOS_FILE = Unity.PKGDATADIR + "/photos.png";
  const string SOFTWARECENTER_FILE = Unity.PKGDATADIR + "/software_centre.png";
  const string TRASH_FILE = Unity.PKGDATADIR + "/trash.png";
  const string VIDEOS_FILE = Unity.PKGDATADIR + "/videos.png";
  const string WEB_FILE = Unity.PKGDATADIR + "/web.png";


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

      if (cairotxt != null)
        this.remove_actor (cairotxt);

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


  public class PlaceIcon
  {
    private Unity.Places.Bar.Model model;
    public Ctk.Image view;

    public PlaceIcon (int width, /*int height,*/ string name, string icon_name, string tooltip)
    {
      model = new Unity.Places.Bar.Model (name, icon_name, tooltip);
      view = new Ctk.Image.from_filename (width, icon_name);
      view.set_reactive (true);
    }
  }

  public class View : Ctk.Box
  {
    public Places.Model model { get; construct; }

    public int IconSize = 0;
    public int FirstPlaceIconPosition = 0;
    public int PlaceIconSpacing = 0;
    public int SeparatorPosition = 0;

    public int FirstDevicePosition = 0;
    public int DeviceIconSpacing = 0;

    public int TrashPosition = 0;

    private Gee.ArrayList<PlaceIcon> PlacesIconArray;
    private Gee.ArrayList<PlaceIcon> DevicesIconArray;
    private PlaceIcon                TrashIcon;

    private PlacesVSeparator Separator;

    public signal void sig_places_active_icon_index (int i);
    public signal void sig_devices_active_icon_index (int i);
    public signal void sig_trash_active_icon_index ();

    public override void allocate (Clutter.ActorBox        box,
                                   Clutter.AllocationFlags flags)
    {
      Clutter.ActorBox        base_box = {0, 0, 0, 0};
      base_box.x1 = box.x1;
      base_box.x2 = box.x2;
      base_box.y1 = box.y1;
      base_box.y2 = box.y2;
      base.allocate (base_box, flags);

      int i;

      base_box.x1 = FirstPlaceIconPosition;
      for (i = 0; i < this.PlacesIconArray.size; i++)
      {
        base_box.x2 = base_box.x1 + IconSize;
        base_box.y1 = 0;
        base_box.y2 = IconSize;

        this.PlacesIconArray[i].view.allocate (base_box, flags);

        base_box.x1 += IconSize + PlaceIconSpacing;
      }

      base_box.x1 = FirstDevicePosition;
      for (i = 0; i < this.DevicesIconArray.size; i++)
      {
        base_box.x2 = base_box.x1 + IconSize;
        base_box.y1 = 0;
        base_box.y2 = IconSize;

        this.DevicesIconArray[i].view.allocate (base_box, flags);

        base_box.x1 += IconSize + PlaceIconSpacing;
      }

      base_box.x1 = TrashPosition;
      base_box.x2 = base_box.x1 + IconSize;
      base_box.y1 = 0;
      base_box.y2 = IconSize;

      this.TrashIcon.view.allocate (base_box, flags);

      if (this.Separator.Height != 48)
        {
          this.Separator.CreateSeparator (5, 48);
        }

      base_box.x1 = SeparatorPosition;
      base_box.x2 = SeparatorPosition+5;
      base_box.y1 = 4;
      base_box.y2 = 44;
      this.Separator.allocate (base_box, flags);
    }

    public View (Places.Model model)
    {
      PlaceIcon place;
      int                    i;
      int                    icon_size = 48;
      Ctk.EffectGlow         glow;
      Clutter.Color          white = {255, 255, 255, 255};

      Object (model:model);

      this.homogeneous  = false;
      this.orientation  = Ctk.Orientation.HORIZONTAL; // this sucks

      this.PlacesIconArray = new Gee.ArrayList<PlaceIcon> ();
      this.DevicesIconArray = new Gee.ArrayList<PlaceIcon> ();

      // populate places-bar with hard-coded contents for the moment
      place = new PlaceIcon (icon_size, "Home",
                                        HOME_FILE,
                                        "Default View");
      this.PlacesIconArray.add (place);

      place = new PlaceIcon (icon_size, "Files",
                                        FILES_FILE,
                                        "Your files stored locally");
      this.PlacesIconArray.add (place);

      place = new PlaceIcon (icon_size, "Applications",
                                        APPS_FILE,
                                        "Programs installed locally");
      this.PlacesIconArray.add (place);

      place = new PlaceIcon (icon_size, "Music",
                                        MUSIC_FILE,
                                        "Soothing sounds and vibes");
      this.PlacesIconArray.add (place);

      place = new PlaceIcon (icon_size, "People",
                                        PEOPLE_FILE,
                                        "Friends, pals, mates and folks");
      this.PlacesIconArray.add (place);

      place = new PlaceIcon (icon_size, "Photos",
                                        PHOTOS_FILE,
                                        "Pretty pictures presented by pixels");
      this.PlacesIconArray.add (place);

      /*place = new PlaceIcon (icon_size, "Trash",
                                        TRASH_FILE,
                                        "Your piece of waste");
      this.PlacesIconArray.add (place);*/

      /* create all image-actors for icons */
      for (i = 0; i < this.PlacesIconArray.size ; i++)
      {
        glow = new Ctk.EffectGlow ();
        glow.set_color (white);
        glow.set_factor (1.0f);
        glow.set_margin (6);
        this.PlacesIconArray[i].view.add_effect (glow);

        this.pack (this.PlacesIconArray[i].view, false, false);
        this.PlacesIconArray[i].view.enter_event.connect (this.on_enter);
        this.PlacesIconArray[i].view.leave_event.connect (this.on_leave);
        this.PlacesIconArray[i].view.button_press_event.connect (this.on_button_press);
      }

      for (i = 0; i < this.DevicesIconArray.size ; i++)
      {
        glow = new Ctk.EffectGlow ();
        glow.set_color (white);
        glow.set_factor (1.0f);
        glow.set_margin (6);
        this.DevicesIconArray[i].view.add_effect (glow);

        this.pack (this.DevicesIconArray[i].view, false, false);
        this.DevicesIconArray[i].view.enter_event.connect (this.on_enter);
        this.DevicesIconArray[i].view.leave_event.connect (this.on_leave);
        this.DevicesIconArray[i].view.button_press_event.connect (this.on_button_press);
      }

      {
        TrashIcon = new PlaceIcon (icon_size, "Trash",
                                            TRASH_FILE,
                                            "Your piece of waste");
        glow = new Ctk.EffectGlow ();
        glow.set_color (white);
        glow.set_factor (1.0f);
        glow.set_margin (6);
        this.TrashIcon.view.add_effect (glow);

        this.pack (this.TrashIcon.view, false, false);
        this.TrashIcon.view.enter_event.connect (this.on_enter);
        this.TrashIcon.view.leave_event.connect (this.on_leave);
        this.TrashIcon.view.button_press_event.connect (this.on_button_press);
      }

      Separator = new PlacesVSeparator ();
      this.pack (this.Separator, false, false);

      this.show_all ();
    }

    public int get_number_of_places ()
    {
      return this.PlacesIconArray.size;
    }

    public int get_number_of_devices ()
    {
      return this.DevicesIconArray.size;
    }


    public bool on_enter ()
    {
      /* stdout.printf ("on_enter() called\n"); */
      return false;
    }

    public bool on_leave ()
    {
      /* stdout.printf ("on_leave() called\n"); */
      return false;
    }

    public bool on_button_press (Clutter.Event event)
    {
      int i;
      Clutter.Actor actor;
      actor = event.button.source;

      for (i = 0; i < this.PlacesIconArray.size; i++)
        {
          if (actor == this.PlacesIconArray[i].view)
            {
              sig_places_active_icon_index (i);
              return false;
            }
        }

      for (i = 0; i < this.DevicesIconArray.size; i++)
        {
          if (actor == this.DevicesIconArray[i].view)
            {
              sig_devices_active_icon_index (i);
              return false;
            }
        }

      if (actor == this.TrashIcon.view)
        sig_trash_active_icon_index ();

      return false;
    }

    construct
    {
    }
  }
}
