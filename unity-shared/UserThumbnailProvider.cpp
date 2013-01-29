/*
 * Copyright 2011 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 **
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#include <Nux/Nux.h>
#include "UnityCore/GLibWrapper.h"
#include "UserThumbnailProvider.h"
#include "ThumbnailGenerator.h"

namespace unity
{

namespace UserThumbnailProvider
{

class UserThumbnailer : public Thumbnailer
{
public:
  UserThumbnailer(std::string const& name, std::string const& command_line)
  : name(name)
  , command_line(command_line)
  {}

  std::string name;
  std::string command_line;

  virtual std::string GetName() const { return name; }

  virtual bool Run(int size, std::string const& input_file, std::string& output_file, std::string& error_hint);
};

bool UserThumbnailer::Run(int size, std::string const& input_file, std::string& output_file, std::string& error_hint)
{
  std::string tmp_command_line = command_line;

  // replace size
  size_t pos = tmp_command_line.find("%s");
  std::stringstream ss; ss << size;
  if (pos != std::string::npos) { tmp_command_line.replace(pos, 2, ss.str()); }

  // replace input file name
  pos = tmp_command_line.find("%u");
  if (pos != std::string::npos) { tmp_command_line.replace(pos, 2, input_file); }

  // replace output file name
  pos = tmp_command_line.find("%o");
  if (pos != std::string::npos) { tmp_command_line.replace(pos, 2, output_file); }

  gint exit_status = 0;
  GError* err = NULL;
  g_spawn_command_line_sync(tmp_command_line.c_str(), NULL, NULL, &exit_status, &err);
  if (err != NULL)
  {
    error_hint = err->message;
    g_error_free (err);
    return false;
  }
  else if (exit_status != 0)
  {
    std::stringstream ss;
    ss << "Failed to create thumbnail. Program exited with exit_status=" << exit_status;
    error_hint = ss.str();
    return false;
  }

  return true;
}

void Initialise()
{
  GError* err = NULL;
  GDir* thumbnailer_dir = g_dir_open("/usr/share/thumbnailers", 0, &err);
  if (err != NULL)
    return;

  const gchar* file;
  while((file = g_dir_read_name(thumbnailer_dir)) != NULL)
  {
    std::string file_name(file);
    if (file_name == "." || file_name == "..")
      continue;

    /*********************************
     * READ SETTINGS
     *********************************/  

    GKeyFile* key_file = g_key_file_new();

    err=NULL;
    if (!g_key_file_load_from_file (key_file, (std::string("/usr/share/thumbnailers/") + file_name).c_str(), G_KEY_FILE_NONE, &err))
    {
      g_key_file_free(key_file);
      g_error_free(err);
      continue;
    }

    err=NULL;
    glib::String command_line(g_key_file_get_string (key_file, "Thumbnailer Entry", "Exec", &err));
    if (err != NULL)
    {
      g_key_file_free(key_file);
      g_error_free(err);
      continue;
    }

    err=NULL;
    gsize mime_count = 0;
    gchar** mime_types = g_key_file_get_string_list (key_file, "Thumbnailer Entry", "MimeType", &mime_count, &err);
    if (err != NULL)
    {
      g_key_file_free(key_file);
      g_error_free(err);
      continue;
    }

    Thumbnailer::Ptr thumbnailer(new UserThumbnailer(file_name, command_line.Value()));
    std::list<std::string> mime_type_list;
    for (gsize i = 0; i < mime_count && mime_types[i] != NULL; i++)
    {
      mime_type_list.push_front(mime_types[i]);
    }

    ThumbnailGenerator::RegisterThumbnailer(mime_type_list, thumbnailer);

    g_strfreev(mime_types);
    g_key_file_free(key_file);
  }

  g_dir_close(thumbnailer_dir);
}

} // namespace DefaultThumbnailProvider
} // namespace unity


