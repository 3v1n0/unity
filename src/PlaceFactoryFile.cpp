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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */
#include "config.h"

#include <algorithm>

#include "PlaceFactoryFile.h"

#include "PlaceRemote.h"

static void on_directory_enumeration_ready (GObject      *source,
                                            GAsyncResult *result,
                                            gpointer      user_data);

PlaceFactoryFile::PlaceFactoryFile (const char *directory)
{
  /* Use the default lookup location */
  if (directory == NULL)
    _directory = g_build_filename (DATADIR, "unity", "places", NULL);
  else
    _directory = g_strdup (directory);

  /* Set off the async listing of the directory */
  _dir = g_file_new_for_path (_directory);
  if (_dir == NULL)
  {
    g_warning ("Unable to create GFile object for places directory %s", _directory);
    return;
  }

  if (!g_getenv ("UNITY_PLACES_DISABLE"))
    g_file_enumerate_children_async (_dir,
                                     G_FILE_ATTRIBUTE_STANDARD_NAME,
                                     G_FILE_QUERY_INFO_NONE,
                                     0,
                                     NULL,
                                     on_directory_enumeration_ready,
                                     this);
}

PlaceFactoryFile::~PlaceFactoryFile ()
{
  std::vector<Place*>::iterator it;
  
  for (it = _places.begin(); it != _places.end(); it++)
  {
    Place *place = static_cast<Place *> (*it);
    delete place;
  }

  _places.erase (_places.begin (), _places.end ());

  g_free (_directory);
  g_object_unref (_dir);
}

std::vector<Place *>&
PlaceFactoryFile::GetPlaces ()
{
  return _places;
}

void
PlaceFactoryFile::ForceRefresh ()
{
  g_debug ("%s: Unsupported on this backend", G_STRFUNC);
}

void
PlaceFactoryFile::AddProperties (GVariantBuilder *builder)
{
}

void
PlaceFactoryFile::OnDirectoryEnumerationReady (GObject      *source,
                                               GAsyncResult *result)
{
  GFileEnumerator *enumerator;
  GError          *error = NULL;
  GFileInfo       *info;

  enumerator = g_file_enumerate_children_finish (_dir,
                                                 result,
                                                 &error);
  if (error)
  {
    g_warning ("Unable to enumerate contents of %s: %s",
               _directory,
               error->message);
    g_error_free (error);
    return;
  }

  while ((info = g_file_enumerator_next_file (enumerator, NULL, &error)))
  {
    const char *name;

    name = g_file_info_get_name (info);
    if (g_str_has_suffix (name, ".place"))
    {
      PlaceRemote *place;
      gchar       *path;

      path = g_build_filename (_directory, name, NULL);

      place = new PlaceRemote (path);
      if (place->IsValid ())
      {
        _places.push_back (place);
      }
      else
        delete place;

      g_free (path);
    }

    g_object_unref (info);
  }

  if (error)
  {
    g_warning ("Unable to read files from %s: %s",
               _directory,
               error->message);

    g_error_free (error);
    g_object_unref (enumerator);
    return;
  }

  // Sort them
  std::sort (_places.begin (), _places.end (), DoSortThemMister);

  // Signal their creation
  std::vector<Place *>::iterator it, eit = _places.end ();
  for (it = _places.begin (); it != eit; ++it)
  {
    place_added.emit (*it);
    g_debug ("%s", static_cast<PlaceRemote *> (*it)->GetDBusPath ());
  }

  read_directory = true;

  g_object_unref (enumerator);
}

bool
PlaceFactoryFile::DoSortThemMister (Place *aa, Place *bb)
{
#define FIRST "/com/canonical/unity/applicationsplace"
#define SECOND "/com/canonical/unity/filesplace"

  PlaceRemote *a = static_cast<PlaceRemote *> (aa);
  PlaceRemote *b = static_cast<PlaceRemote *> (bb);

  if (g_strcmp0 (a->GetDBusPath (), FIRST) == 0)
    return true;
  else if (g_strcmp0 (b->GetDBusPath (), FIRST) == 0)
    return false;
  else if (g_strcmp0 (a->GetDBusPath (), SECOND) == 0)
    return true;
  else if (g_strcmp0 (b->GetDBusPath (), SECOND) == 0)
    return false;
  else
    return g_strcmp0 (a->GetDBusPath (), b->GetDBusPath ()) == 0;
}

/*
 * C to C++ glue
 */
static void
on_directory_enumeration_ready (GObject      *source,
                                GAsyncResult *result,
                                gpointer      user_data)
{
  PlaceFactoryFile *self = static_cast<PlaceFactoryFile *> (user_data);
  self->OnDirectoryEnumerationReady (source, result);
}
