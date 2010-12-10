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

#include "PlaceFactoryFile.h"

static void on_directory_enumeration_ready (GObject      *source,
                                            GAsyncResult *result,
                                            gpointer      user_data);

PlaceFactoryFile::PlaceFactoryFile (const char *directory)
{
  /* Use the default lookup location */
  if (directory == NULL)
    _directory = g_build_filename (PKGDATADIR, "places", NULL);
  else
    _directory = g_strdup (directory);

  /* Set off the async listing of the directory */
  _dir = g_file_new_for_path (_directory);
  if (_dir == NULL)
  {
    g_warning ("Unable to create GFile object for places directory %s", _directory);
    return;
  }

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
      g_print ("Found Place: %s\n", name);
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

  g_object_unref (enumerator);
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
