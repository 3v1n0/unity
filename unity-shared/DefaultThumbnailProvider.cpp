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
#include "ThumbnailGenerator.h"
#include "UnityCore/GLibWrapper.h"
#include "DefaultThumbnailProvider.h"

namespace unity
{

namespace DefaultThumbnailProvider
{

class DefaultThumbnailer : public Thumbnailer
{
public:
  DefaultThumbnailer(std::string const& name)
  : name(name)
  {}
  virtual ~DefaultThumbnailer() {}

  std::string name;

  virtual std::string GetName() const { return name; }

  virtual bool Run(int size, std::string const& input_file, std::string& output_file, std::string& error_hint);
};


bool DefaultThumbnailer::Run(int size, std::string const& input_file, std::string& output_file, std::string& error_hint)
{
  glib::Object<GFile> file(::g_file_new_for_uri(input_file.c_str()));

  GError *err = NULL;
  glib::Object<GFileInfo> file_info(g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_ICON, G_FILE_QUERY_INFO_NONE, NULL, &err));
  if (err != NULL)
  {
    error_hint = err->message;
    g_error_free (err);
    return "";
  }

  GIcon* icon = g_file_info_get_icon(file_info); // [transfer none]
  output_file = g_icon_to_string(icon);

  return true;
}

void Initialise()
{
  Thumbnailer::Ptr thumbnailer(new DefaultThumbnailer("default"));
  std::list<std::string> mime_types;
  mime_types.push_back("*");
  ThumbnailGenerator::RegisterThumbnailer(mime_types, thumbnailer);
}

} // namespace DefaultThumbnailProvider
} // namespace unity


