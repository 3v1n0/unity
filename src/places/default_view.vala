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

namespace Unity.Places.Default
{
  public class View : Ctk.IconView
  {
    private Gee.ArrayList<Unity.Places.Default.Model> activities;

    public View ()
    {
      Unity.Places.Default.Model activity;

      this.activities = new Gee.ArrayList<Unity.Places.Default.Model> ();

      activity = new Unity.Places.Default.Model ("applications-internet",
                                                 "<b>Web</b>",
                                                 "Search, Suft & Download");
      this.activities.add (activity);

      activity = new Unity.Places.Default.Model ("rhythmbox",
                                                 "<b>Music</b>",
                                                 "Jukebox, Radio & Podcasts");
      this.activities.add (activity);

      activity = new Unity.Places.Default.Model ("applications-multimedia",
                                                 "<b>Videos</b>",
                                                 "YouTube & More");
      this.activities.add (activity);

      activity = new Unity.Places.Default.Model ("applications-graphics",
                                                 "<b>Photos</b>",
                                                 "Organize, Edit & Share");
      this.activities.add (activity);

      activity = new Unity.Places.Default.Model ("applications-office",
                                                 "<b>Work</b>",
                                                 "Office Documents, Spreadsheets & Presentations");
      this.activities.add (activity);

      activity = new Unity.Places.Default.Model ("evolution",
                                                 "<b>Email</b>",
                                                 "Read & Write Email");
      this.activities.add (activity);

      activity = new Unity.Places.Default.Model ("empathy",
                                                 "<b>Chat</b>",
                                                 "AIM, Yahoo, Skype & MSN");
      this.activities.add (activity);

      activity = new Unity.Places.Default.Model ("softwarecenter",
                                                 "<b>Get New Apps</b>",
                                                 "Ubuntu Software Center");
      this.activities.add (activity);
    }

    construct
    {
    }
  }
}
