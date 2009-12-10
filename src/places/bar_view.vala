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

  public class View : Ctk.Box
  {
    private Gee.ArrayList<Unity.Places.Bar.Model> places;

    public View ()
    {
      Unity.Places.Bar.Model place;
      int                    i;
      Ctk.Image              icon;
      int                    icon_size = 48;
      Ctk.EffectGlow         glow;
      Clutter.Color          white = {255, 255, 255, 255};

      this.homogeneous  = false;
      this.spacing      = icon_size/2;
      this.orientation  = Ctk.Orientation.HORIZONTAL; // this sucks

      this.places = new Gee.ArrayList<Unity.Places.Bar.Model> ();

      // populate places-bar with hard-coded contents for the moment
      place = new Unity.Places.Bar.Model ("Home",
                                          HOME_FILE,
                                          "Default View");
      this.places.add (place);

      place = new Unity.Places.Bar.Model ("Applications",
                                          APPS_FILE,
                                          "Programs installed locally");
      this.places.add (place);

      place = new Unity.Places.Bar.Model ("Files",
                                          FILES_FILE,
                                          "Your files stored locally");
      this.places.add (place);

      place = new Unity.Places.Bar.Model ("Music",
                                          MUSIC_FILE,
                                          "Soothing sounds and vibes");
      this.places.add (place);

      place = new Unity.Places.Bar.Model ("People",
                                          PEOPLE_FILE,
                                          "Friends, pals, mates and folks");
      this.places.add (place);

      place = new Unity.Places.Bar.Model ("Photos",
                                          PHOTOS_FILE,
                                          "Pretty pictures presented by pixels");
      this.places.add (place);

      place = new Unity.Places.Bar.Model ("Trash",
                                          TRASH_FILE,
                                          "Your piece of waste");
      this.places.add (place);

      glow = new Ctk.EffectGlow ();
      glow.set_color (white);
      glow.set_factor (9.0f);
      this.add_effect (glow);

      // create all image-actors for icons
      for (i = 0; i < this.places.size ; i++)
      {
        //icon = new Ctk.Image.from_stock (icon_size, this.places[i].icon_name);
        icon = new Ctk.Image.from_filename (icon_size, this.places[i].icon_name);
        icon.set_reactive (true);

        this.pack (icon, false, false);
        icon.enter_event.connect (this.on_enter);
        icon.leave_event.connect (this.on_leave);
      }

      this.show_all ();
    }

    public bool on_enter ()
    {
      stdout.printf ("on_enter() called\n");
      return false;
    }

    public bool on_leave ()
    {
      stdout.printf ("on_leave() called\n");
      return false;
    }

    construct
    {
    }
  }
}
