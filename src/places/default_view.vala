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
  public class ActivityWidget : Ctk.Box
  {
    private Ctk.Image    icon;
    private Clutter.Text primary_label;
    private Clutter.Text secondary_label;

    public ActivityWidget (int    spacing,
                           int    size,
                           string icon_name,
                           string primary_text,
                           string secondary_text)
    {
      Clutter.Color color = {255, 255, 255, 255};

      this.homogeneous = false;
      this.orientation = Ctk.Orientation.VERTICAL;
      this.set_reactive (true);
      //this.padding = pad;
      this.spacing = spacing;
      //this.width   = size + 32;
      this.icon    = new Ctk.Image.from_stock (size, icon_name);
      this.icon.set_reactive (true);

      this.primary_label = new Clutter.Text ();
      this.primary_label.set_reactive (true);
      this.primary_label.width = size;
      this.primary_label.set_markup (primary_text);
      this.primary_label.justify = true;
      this.primary_label.color = color;
      this.primary_label.ellipsize = Pango.EllipsizeMode.NONE;
      this.primary_label.line_wrap = true;
      this.primary_label.line_alignment = Pango.Alignment.CENTER;

      this.secondary_label = new Clutter.Text ();
      this.secondary_label.set_reactive (true);
      this.secondary_label.width = size + spacing;
      //this.secondary_label.get_layout().set_height (-2);
      this.secondary_label.text = secondary_text;
      this.secondary_label.justify = false;
      this.secondary_label.color = color;
      //this.secondary_label.ellipsize = Pango.EllipsizeMode.END;
      this.secondary_label.line_wrap = true;
      this.secondary_label.line_alignment = Pango.Alignment.CENTER;
      this.secondary_label.line_wrap_mode = Pango.WrapMode.WORD_CHAR;
      this.enter_event.connect (this.on_enter);
      this.leave_event.connect (this.on_leave);
      this.secondary_label.opacity = 0;

      this.pack (this.icon, false, false);
      this.pack (this.primary_label, false, false);
      this.pack (this.secondary_label, false, false);
    }

    construct
    {
    }

    public bool on_enter ()
    {
      this.secondary_label.opacity = 255;
      return false;
    }

    public bool on_leave ()
    {
      this.secondary_label.opacity = 0;
      return false;
    }
     public bool on_clicked ()
    {
      stdout.printf ("on_clicked() called\n");
      return false;
    }
     
  }

  public class View : Ctk.IconView
  {
    private Gee.ArrayList<Unity.Places.Default.Model> activities;

    public View ()
    {
      Unity.Places.Default.Model activity;
      int                        i;
      ActivityWidget             widget;
      int                        widget_size = 128;

      this.activities = new Gee.ArrayList<Unity.Places.Default.Model> ();

      // populate defaultview with hard-coded contents for the moment
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

      // create image-actors now
      for (i = 0; i < this.activities.size; i++)
      {
        widget = new ActivityWidget (0,
                                     widget_size,
                                     this.activities[i].icon_name,
                                     this.activities[i].primary_text,
                                     this.activities[i].secondary_text);
        this.add_actor (widget);
      }

      this.show_all ();
    }

    construct
    {
    }
  }
}
