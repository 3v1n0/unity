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
#include "TextureThumbnailProvider.h"

namespace unity
{

namespace TextureThumbnailProvider
{

class GdkTextureThumbnailer : public Thumbnailer
{
public:
  GdkTextureThumbnailer(std::string const& name)
  : name(name)
  {}

  std::string name;

  virtual std::string GetName() const { return name; }

  virtual bool Run(int size, std::string const& input_file, std::string& output_file, std::string& error_hint);
};


bool GdkTextureThumbnailer::Run(int size, std::string const& input_file, std::string& output_file, std::string& error_hint)
{
  GFileInputStream       *stream;
  GError                 *error = NULL;
  GFile                  *file;

  /* try to open the source file for reading */
  file = g_file_new_for_uri (input_file.c_str());
  stream = g_file_read (file, NULL, &error);
  g_object_unref (file);

  if (error != NULL)
  {
    error_hint = error->message;
    g_error_free (error);
    return false;
  }

  glib::Object<GdkPixbuf> source_pixbuf(::gdk_pixbuf_new_from_stream_at_scale(G_INPUT_STREAM (stream),
                                                       -1,
                                                       size,
                                                       TRUE,
                                                       NULL,
                                                       NULL));

  // glib::Object<GdkPixbuf> source_pixbuf(gdk_pixbuf_new_from_stream (G_INPUT_STREAM (stream), 
  //                                             NULL, &error));
  g_object_unref (stream);

  if (error != NULL)
  {
    error_hint = error->message;
    g_error_free (error);
    return false;
  }

  /**************************
   * Generate Scaled buffer *
   **************************/

  gdouble    hratio;
  gdouble    wratio;
  gint       dest_width = size;
  gint       dest_height = size;
  gint       source_width;
  gint       source_height;

  /* determine the source pixbuf dimensions */
  source_width = gdk_pixbuf_get_width (source_pixbuf);
  source_height = gdk_pixbuf_get_height (source_pixbuf);
  
  /* return the same pixbuf if no scaling is required */
  if (source_width <= dest_width && source_height <= dest_height)
  {
    gdk_pixbuf_save(source_pixbuf, output_file.c_str(), "png", &error, NULL);
    if (error != NULL)
    {
      error_hint = error->message;
      g_error_free (error);
      return false;
    }
  }

  /* determine which axis needs to be scaled down more */
  wratio = (gdouble) source_width / (gdouble) dest_width;
  hratio = (gdouble) source_height / (gdouble) dest_height;

  /* adjust the other axis */
  if (hratio > wratio)
    dest_width = rint (source_width / hratio);
  else
    dest_height = rint (source_height / wratio);


  /* scale the pixbuf down to the desired size */
  glib::Object<GdkPixbuf> thumbnail_buffer(gdk_pixbuf_scale_simple (source_pixbuf, 
                                                    MAX (dest_width, 1), MAX (dest_height, 1), 
                                                    GDK_INTERP_BILINEAR));

  gdk_pixbuf_save(thumbnail_buffer, output_file.c_str(), "png", &error, NULL);
  if (error != NULL)
  {
    error_hint = error->message;
    g_error_free (error);
    return false;
  }
  return true;
}

void Initialise()
{
  GSList            *formats;
  GSList            *fp;
  GStrv              format_types;
  guint              n;
  std::list<std::string> mime_types;

  /* get a list of all formats supported by GdkPixbuf */
  formats = gdk_pixbuf_get_formats ();

  /* iterate over all formats */
  for (fp = formats; fp != NULL; fp = fp->next)
  {
    /* ignore the disabled ones */
    if (!gdk_pixbuf_format_is_disabled ((GdkPixbufFormat*)fp->data))
    {
      /* get a list of MIME types supported by this format */
      format_types = gdk_pixbuf_format_get_mime_types ((GdkPixbufFormat*)fp->data);

      /* put them all in the unqiue MIME type hash table */
      for (n = 0; format_types != NULL && format_types[n] != NULL; ++n)
      {
        mime_types.push_back(format_types[n]);
      }

      /* free the string array */
      g_strfreev (format_types);
    }
  }


  Thumbnailer::Ptr thumbnailer(new GdkTextureThumbnailer("gdk_pixelbuffer"));
  ThumbnailGenerator::RegisterThumbnailer(mime_types, thumbnailer);
}

} // namespace TextureThumbnailProvider
} // namespace unity

