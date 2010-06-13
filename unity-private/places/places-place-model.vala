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
  /**
   * PlaceModel:
   *
   * Contains the loaded Place objects. Abstract class so views can be
   * tested with fake model
   **/
  public abstract class PlaceModel : Gee.ArrayList<Place>
  {
  }


  /**
   * PlaceFileModel:
   *
   * Reads in .place files and creates the offline model, ready for representing
   * to the user without actually initing any of the places
   **/
  public class PlaceFileModel : PlaceModel
  {
    /* Properties */
    public string directory { get; construct; }
    public bool   async     { get; construct; }

    /* Constructors */
    public PlaceFileModel ()
    {
      Object (directory:PKGDATADIR + "/places", async:true);
    }

    /* Allows loading places files from a non-install directory */
    public PlaceFileModel.with_directory (string _directory)
    {
      Object (directory: _directory, async:false);
    }

    construct
    {
      /* Start asyncly reading in place files */
      if (async)
        load_place_files.begin ();
      else
        load_place_files_sync ();
    }

    /* Private Methods */

    /*
     * Read in all the .place files from the directory and convert them to
     * Place objects, with the initial set of Entries, if that information
     * was included inside the .place file
     */
    private async void load_place_files ()
    {
      var dir = File.new_for_path (directory);
      try {
        var e = yield dir.enumerate_children_async (FILE_ATTRIBUTE_STANDARD_NAME,
                                                  0,
                                                  Priority.DEFAULT, null);
        while (true)
          {
            var files = yield e.next_files_async (10, Priority.DEFAULT, null);
            if (files == null)
              break;

            foreach (var info in files)
              {
                if (info.get_name ().has_suffix (".place") == false)
                  continue;

                var place = load_place (Path.build_filename (directory,
                                                             info.get_name ()));
                if (place is Place)
                  {
                    place.connect ();
                    add (place);
                  }
              }
          }

      } catch (Error error) {
        warning (@"Unable to read place files from directory '$directory': %s",
                 error.message);
      }
    }

    private void load_place_files_sync ()
    {
      var dir = File.new_for_path (directory);
      try {
          var e = dir.enumerate_children (FILE_ATTRIBUTE_STANDARD_NAME, 0, null);
          FileInfo info;
          while ((info = e.next_file (null)) != null)
            {
              string leaf = info.get_name ();
              if (leaf.has_suffix (".place") == false)
                continue;

              var place = load_place (Path.build_filename (directory, leaf));
              if (place is Place)
                add (place);
            }

      } catch (Error error) {
        warning (@"Unable to read place files from directory '$directory': %s",
                 error.message);
      }
    }

    /*
     * Reads in the place file (which is a key file) and creates and inits
     * a UnityPlacesPlace object to represent it.
     */
    private Place? load_place (string path)
    {
      var file = new KeyFile ();
      try {
        file.load_from_file (path, KeyFileFlags.NONE);

        return Place.new_from_keyfile (file, path);

      } catch (Error e) {
        warning (@"Unable to load place file '$path': %s", e.message);
        return null;
      }
    }
  }
}
