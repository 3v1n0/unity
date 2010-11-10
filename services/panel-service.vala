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

using Gee;

[DBus (name = "com.canonical.Unity.Panel.Service")]
public class Service : Object
{
  const string INDICATORICONSDIR = "/usr/share/libindicator/icons";
  const string INDICATORDIR = "/usr/lib/indicators/4/";
  enum IndicatorsColumn
  {
    NAME,
    ENTRY_MODEL,
    EXPAND
  }

  enum EntryColumn
  {
    ID,
    LABEL,
    ICON_HINT,
    ICON_DATA,
    LABEL_VISIBLE,
    ICON_VISIBLE
  }

  public static HashMap<string, int> indicator_order = null;
  public HashMap<string, Indicator.Object> indicator_map;
  public HashMap<Indicator.Object, Dee.SharedModel> entry_models;
  public HashMap<string, unowned Indicator.ObjectEntry> entry_map;

  private Dee.SharedModel indicators_model;

  private unowned Indicator.ObjectEntry last_entry;
  private unowned Gtk.Menu              last_menu;
  private int                           last_x;
  private int                           last_y;

  public signal void entry_activated (string entry_id);

  public Service ()
  {
    Object ();
  }

  construct
  {
    indicator_map = new Gee.HashMap<string, Indicator.Object> ();
    indicator_order = new Gee.HashMap<string, int> ();
    entry_models = new Gee.HashMap<Indicator.Object, Dee.SharedModel> ();
    entry_map = new Gee.HashMap<string, unowned Indicator.ObjectEntry> ();

    indicators_model = new Dee.SharedModel ("com.canonical.Unity.Panel.Service.Indicators",
                                            3,
                                            typeof (string),
                                            typeof (string),
                                            typeof (bool));

    load_indicators ();

    indicators_model.connect ();
  }

  /*
   * Public Methods
   */
  public void show_entry (string entry_id, uint timestamp, int x, int y)
  {
    unowned Indicator.ObjectEntry entry = entry_map[entry_id];

    if (last_menu is Gtk.Menu)
      {
        last_x = 0;
        last_y = 0;
        last_entry = null;

        last_menu.notify["visible"].disconnect (menu_vis_changed);
        last_menu.popdown ();
        last_menu = null;
      }

    if (entry != null && entry.menu is Gtk.Menu)
      {
        last_entry = entry;
        last_x = x;
        last_y = y;
        last_menu = entry.menu;
        last_menu.popup (null, null, position_menu, 0, timestamp);

        entry_activated (entry_id);

        last_menu.notify["visible"].connect (menu_vis_changed);

        debug ("MENU SHOWN: %ld, %d %d", timestamp, x, y);
      }
  }

  public void scroll_object (string object_id, int delta, uint direction)
  {
    Indicator.Object obj = indicator_map[object_id];
    Signal.emit_by_name (obj, "scroll", delta, direction);
  }

  /*
   * Private Methods
   */
  private void menu_vis_changed (Object object, ParamSpec pspec)
  {
    Gtk.Menu menu = object as Gtk.Menu;
    bool visible = (menu.get_flags () & Gtk.WidgetFlags.VISIBLE) != 0;

    if (visible == false)
      {
        if (last_menu == menu)
          {
            last_menu = null;
            last_entry = null;
            entry_activated ("");
          }
        menu.notify["visible"].disconnect (menu_vis_changed);
      }
  }

  private void position_menu (Gtk.Menu m, out int x, out int y, out bool push)
  {
    x = last_x;
    y = last_y;
    push = true;
  }

  private void load_indicator (string filename, string leaf)
  {
    Indicator.Object o;

    o = new Indicator.Object.from_file (filename);

    if (o is Indicator.Object)
      {
        indicator_map[leaf] = o;

        string model_name = "com.canonical.Unity.Panel.Service.%s".printf (leaf[3:-3]);

        Dee.SharedModel entry_model = new Dee.SharedModel (model_name,
                                         6,
                                         typeof (string),
                                         typeof (string),
                                         typeof (uint),
                                         typeof (string),
                                         typeof (bool),
                                         typeof (bool));
        entry_models[o] = entry_model;
        o.entry_added.connect (on_entry_added);
        o.entry_removed.connect (on_entry_removed);

        unowned GLib.List<Indicator.ObjectEntry> list = o.get_entries ();

        for (int i = 0; i < list.length (); i++)
          {
            unowned Indicator.ObjectEntry entry = (Indicator.ObjectEntry) list.nth_data (i);

            on_entry_added (o, entry);
          }

        entry_model.connect ();

        indicators_model.append (IndicatorsColumn.NAME, leaf,
                                 IndicatorsColumn.ENTRY_MODEL, model_name,
                                 IndicatorsColumn.EXPAND, leaf == "libappmenu.so" ? true : false, -1);

      }
    else
      {
        warning ("Unable to load %s\n", filename);
      }
  }

  private unowned Dee.ModelIter? iter_for_entry_id (Dee.Model model,
                                                    string    id)
  {
    unowned Dee.ModelIter? ret = null;

    unowned Dee.ModelIter iter = model.get_first_iter ();
    while (iter != null && !model.is_last (iter))
      {
        if (model.get_string (iter, EntryColumn.ID) == id)
          {
            ret = iter;
            break;
          }
        iter = model.next (iter);
      }

    return ret;
  }

  private void on_entry_removed (Indicator.Object indicator_object,
                                 Indicator.ObjectEntry entry)
  {
    Dee.SharedModel? model = entry_models[indicator_object];
    string id = "%p".printf (entry);

    entry_map.unset (id);

    unowned Dee.ModelIter? iter = iter_for_entry_id (model, id);

    if (iter != null)
      {
        model.remove (iter);
      }
  }

  private void on_entry_added (Indicator.Object      object,
                               Indicator.ObjectEntry entry)
  {
    Dee.SharedModel model = entry_models[object];

    string id = "%p".printf (entry);
    string label = "";
    uint   icon_hint = 0;
    string icon_data = "";
    bool   label_visible = false;
    bool   icon_visible = false;


    entry_map[id] = entry;

    if (entry.label is Gtk.Label)
      {
        label_visible = (entry.label.get_flags () & Gtk.WidgetFlags.VISIBLE) != 0;
        label = entry.label.label;

        unowned Gtk.Label gtk_label = entry.label;

        gtk_label.notify["label"].connect (() => {
        unowned Dee.ModelIter? iter = iter_for_entry_id (model, id);

          if (iter != null)
            model.set (iter, EntryColumn.LABEL, gtk_label.label, -1);
        });

        gtk_label.notify["visible"].connect (() => {
          unowned Dee.ModelIter? iter = iter_for_entry_id (model, id);

          if (iter != null)
            model.set (iter,
                       EntryColumn.LABEL_VISIBLE,
                      (gtk_label.get_flags () & Gtk.WidgetFlags.VISIBLE) !=0,
                      -1);
        });
      }

    if (entry.image is Gtk.Image)
      {
        icon_visible = (entry.image.get_flags () & Gtk.WidgetFlags.VISIBLE) != 0;
        icon_hint = entry.image.storage_type;
        icon_data = get_icon_data_for_image (entry.image);

        unowned Gtk.Image image = entry.image;

        image.notify["storage-type"].connect (() => { refresh_icon (model, id, image); });
        image.notify["icon-name"].connect (() => { refresh_icon (model, id, image); });

        image.notify["pixbuf"].connect (() => { refresh_icon (model, id, image); });
        image.notify["stock"].connect (() => { refresh_icon (model, id, image); });
        image.notify["gicon"].connect (() => { refresh_icon (model, id, image); });
        image.notify["visible"].connect (() => { refresh_icon (model, id, image); });
      }

    model.append (EntryColumn.ID, id,
                  EntryColumn.LABEL, label,
                  EntryColumn.ICON_HINT, icon_hint,
                  EntryColumn.ICON_DATA, icon_data,
                  EntryColumn.LABEL_VISIBLE, label_visible,
                  EntryColumn.ICON_VISIBLE, icon_visible,
                  -1);
  }

  private void refresh_icon (Dee.SharedModel model,
                             string          id,
                             Gtk.Image       image)
  {
    unowned Dee.ModelIter? iter = iter_for_entry_id (model, id);
    string                 i_data = get_icon_data_for_image (image);
    bool visible = (image.get_flags () & Gtk.WidgetFlags.VISIBLE) != 0;

    if (iter != null)
      model.set (iter,
                 EntryColumn.ICON_HINT, image.storage_type,
                 EntryColumn.ICON_DATA, i_data,
                 EntryColumn.ICON_VISIBLE, visible,
                 -1);
  }

  private string get_icon_data_for_image (Gtk.Image image)
  {
    string ret = "";

    switch (image.storage_type)
      {
      case Gtk.ImageType.PIXBUF:
        ret = image_to_string (image);
        break;

      case Gtk.ImageType.STOCK:
        ret = image.stock;
        break;

      case Gtk.ImageType.ICON_NAME:
        ret = image.icon_name;
        break;

      case Gtk.ImageType.GICON:
        ret = image.gicon.to_string ();
        break;

      default:
        message ("GtkImageType '%d' not supported", image.storage_type);
        break;
      }
    return ret;
  }

  private string image_to_string (Gtk.Image image)
  {
    string      png_data = "";
    Gdk.Pixbuf? pixbuf = image.pixbuf;

    if (pixbuf is Gdk.Pixbuf)
      {
        try {
          char[] buffer;

          if (pixbuf.save_to_buffer (out buffer, "png"))
            {
              png_data = Base64.encode ((uchar[])buffer);
            }
        } catch (Error error) {
          warning ("Unable to convert Pixbuf into string: %s", error.message);
        }
      }

    return png_data;
  }

  private void load_indicators ()
  {
      string skip_list;

      /* Static order of indicators. We always want the session indicators
       * to be on the far right end of the panel. That is why it the session
       * indicator is the last one set in indicator_order.
       */
      indicator_order.set ("libappmenu.so", 1);
      indicator_order.set ("libapplication.so", 2);
      indicator_order.set ("libsoundmenu.so", 3);
      indicator_order.set ("libmessaging.so", 4);
      indicator_order.set ("libdatetime.so", 5);
      indicator_order.set ("libme.so", 6);
      indicator_order.set ("libsession.so", 7);

      /* Indicators we don't want to load */
      skip_list = Environment.get_variable ("UNITY_PANEL_INDICATORS_SKIP");
      if (skip_list == null)
        skip_list = "";

      if (skip_list == "all")
        {
          message ("Skipping all indicator loading");
          return;
        }

    /* We need to look for icons in an specific location */
    Gtk.IconTheme.get_default ().append_search_path (INDICATORICONSDIR);

    /* Start loading 'em in. .so are located in  INDICATORDIR*/
    var dir = File.new_for_path (INDICATORDIR);
    try
      {
        var e = dir.enumerate_children (FILE_ATTRIBUTE_STANDARD_NAME, 0,null);

        ArrayList<string> sos = new ArrayList<string> ();

        FileInfo file_info;
        while ((file_info = e.next_file (null)) != null)
          {
            string leaf = file_info.get_name ();

            if (leaf in skip_list)
              continue;

            if (leaf[leaf.length-3:leaf.length] == ".so")
              {
                sos.add (leaf);
              }
          }

        /* Order the so's before we load them */
        sos.sort ((CompareFunc)indicator_sort_func);

        foreach (string leaf in sos)
          load_indicator (INDICATORDIR + leaf, leaf);
      }
    catch (Error error)
      {
        print ("Unable to read indicators: %s\n", error.message);
      }
  }

  public static int indicator_sort_func (string a, string b)
  {
      return indicator_order[a] - indicator_order[b];
  }
}


public class Main
{
  const string GETTEXT_PACKAGE = "unity";
  const string LOCALE_DIR = "/usr/share/locale/en_GB";
  

  public static int main (string[] args)
  {
    Gtk.init (ref args);

    Gdk.window_add_filter (null, on_gdk_event);

    GLib.Intl.textdomain (GETTEXT_PACKAGE);
    GLib.Intl.bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
    GLib.Intl.bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

    try {
      var conn = DBus.Bus.get (DBus.BusType.SESSION);

      dynamic DBus.Object bus = conn.get_object ("org.freedesktop.DBus",
                                                 "/org/freedesktop/DBus",
                                                 "org.freedesktop.DBus");


      uint request_name_result = bus.request_name ("com.canonical.Unity.Panel.Service",
                                                   (uint) 0);

      if (request_name_result == DBus.RequestNameReply.PRIMARY_OWNER)
        {
          var service = new Service ();
          conn.register_object ("/com/canonical/Unity/Panel/Service", service);
        }
      else
        {
          warning ("Another instance of the panel service is running");
          return 1;
        }

    } catch (Error e) {
        warning (@"Unable to register service: $(e.message)");
        return 1;
    }

    Gtk.main ();

    return 0;
  }

  static Gdk.FilterReturn on_gdk_event (Gdk.XEvent xevent, Gdk.Event event)
  {
    print ("GDK EVENT\n");
    return Gdk.FilterReturn.CONTINUE;
  }
}
