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

using Unity.Launcher;

namespace Unity.Places
{
  public class TrashController : ScrollerChildController
  {
    public static const string ICON = "/usr/share/unity/trash.png";

    private GLib.File? trash_dir;
    private uint n_items = 0;

    public TrashController ()
    {
      Object (child:new ScrollerChild ());
    }

    construct
    {
      name = _("Trash");
      load_icon_from_icon_name (ICON);

      try {
        trash_dir = GLib.File.new_for_uri ("trash://");

      } catch (Error e) {
        warning (@"Unable to monitor trash: $(e.message)");
      }

      child.group_type = ScrollerChild.GroupType.SYSTEM;
    }

    public override void activate ()
    {
      try {
        Gtk.show_uri (null, "trash://", Clutter.get_current_event_time ());
      } catch (Error e) {
        warning (@"Unable to show Trash: $(e.message)");
      }
    }

    public override QuicklistController? get_menu_controller ()
    {
      return new ApplicationQuicklistController (this);
    }

    public override void get_menu_actions (ScrollerChildController.menu_cb callback)
    {
      n_items = 0;

      Dbusmenu.Menuitem root = new Dbusmenu.Menuitem ();
      root.set_root (true);

      try {
          var e = trash_dir.enumerate_children (FILE_ATTRIBUTE_STANDARD_NAME, 0, null);
          FileInfo info;
          while ((info = e.next_file (null)) != null)
            n_items++;

      } catch (Error error) {
        warning (@"Unable to enumerate trash: %s",  error.message);
      }

      if (n_items != 0)
        {
          Dbusmenu.Menuitem item;
          /* i18n: This is the number of items in the Trash folder */
          string label = ngettext("%d item", "%d items", n_items);

          item = new Dbusmenu.Menuitem ();
          item.property_set (Dbusmenu.MENUITEM_PROP_LABEL, label.printf (n_items));
          item.property_set_bool (Dbusmenu.MENUITEM_PROP_ENABLED, false);
          item.property_set_bool (Dbusmenu.MENUITEM_PROP_VISIBLE, true);
          root.child_append (item);
        }

      callback (root);
    }

    public override void get_menu_navigation (ScrollerChildController.menu_cb callback)
    {
      Dbusmenu.Menuitem root = new Dbusmenu.Menuitem ();
      root.set_root (true);

      Dbusmenu.Menuitem item;

      item = new Dbusmenu.Menuitem ();
      item.property_set (Dbusmenu.MENUITEM_PROP_LABEL, _("Open"));
      item.property_set_bool (Dbusmenu.MENUITEM_PROP_ENABLED, true);
      item.property_set_bool (Dbusmenu.MENUITEM_PROP_VISIBLE, true);
      root.child_append (item);
      item.item_activated.connect (() => {
        try {
          Gtk.show_uri (null, "trash://", Clutter.get_current_event_time ());
        } catch (Error e) {
          warning (@"Unable to show Trash: $(e.message)");
        }
      });

      if (n_items != 0)
        {
          item = new Dbusmenu.Menuitem ();
          item.property_set (Dbusmenu.MENUITEM_PROP_LABEL, _("Empty Trash"));
          item.property_set_bool (Dbusmenu.MENUITEM_PROP_ENABLED, true);
          item.property_set_bool (Dbusmenu.MENUITEM_PROP_VISIBLE, true);
          root.child_append (item);

          item.item_activated.connect (() => {

          /* FIXME: Popping up this dialog inside mutter doesn't work, so we
           * need to figure out something else for this action. Right now,
           * we'll at least honor the click and empty the trash

            var dialog = new Gtk.MessageDialog (null,
                                                0,
                                                Gtk.MessageType.WARNING,
                                                Gtk.ButtonsType.CANCEL,
                                                _("Empty everything from the Deleted items folder?"));
            dialog.format_secondary_text (_("If you choose to empty the Deleted Items folder, all items in it will be permanently lost. Please note that you can also delete them separately."));
            dialog.add_button (_("Empty Deleted Items"), Gtk.ResponseType.OK);

            dialog.response.connect ((response_id) => {
              if (response_id == Gtk.ResponseType.OK)
                recursively_delete_contents.begin (trash_dir);

              dialog.destroy ();
            });

            dialog.show ();

            return false;
          */

            recursively_delete_contents.begin (trash_dir);
          });
        }

      callback (root);
    }

    private async void recursively_delete_contents (GLib.File dir)
    {
      try {
        var e = yield dir.enumerate_children_async (FILE_ATTRIBUTE_STANDARD_NAME + "," + FILE_ATTRIBUTE_STANDARD_TYPE ,
                                                  FileQueryInfoFlags.NOFOLLOW_SYMLINKS,
                                                  Priority.DEFAULT, null);
        while (true)
          {
            var files = yield e.next_files_async (10, Priority.DEFAULT, null);
            if (files == null)
              break;

            foreach (var info in files)
              {
                var child = dir.get_child (info.get_name ());

                if (info.get_file_type () == FileType.DIRECTORY)
                  yield recursively_delete_contents (child);

                try {
                  child.delete (null);
                } catch (Error error_) {
                  warning (@"Unable to delete file '$(child.get_basename ()): $(error_.message)");
                }
              }
          }

      } catch (Error error) {
        warning (@"Unable to read place files from directory '$(dir.get_basename ())': %s",
                 error.message);
      }
    }

    public override bool can_drag ()
    {
      return true;
    }
  }
}
