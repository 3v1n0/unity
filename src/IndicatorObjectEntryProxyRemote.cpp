// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#include "IndicatorObjectEntryProxyRemote.h"

#include <gtk/gtk.h>

IndicatorObjectEntryProxyRemote::IndicatorObjectEntryProxyRemote ()
: _dirty (false),
  _active (false),
  _id (NULL),
  _label (NULL),
  _image_type (0),
  _image_data (NULL)
{
  label_visible = false;
  label_sensitive = true;
  icon_visible = false;
  icon_sensitive = true;
}


IndicatorObjectEntryProxyRemote::~IndicatorObjectEntryProxyRemote ()
{
  g_free (_id);
  g_free (_label);
  g_free (_image_data);
}

const char *
IndicatorObjectEntryProxyRemote::GetLabel ()
{
  return _label;
}

GdkPixbuf *
IndicatorObjectEntryProxyRemote::GetPixbuf ()
{
  GdkPixbuf *ret = NULL;
  
  if (_image_type == GTK_IMAGE_PIXBUF)
    {
      guchar       *decoded;
      GInputStream *stream;
      gsize         len = 0;
     
      decoded = g_base64_decode (_image_data, &len);
      stream = g_memory_input_stream_new_from_data (decoded, len, NULL);

      ret = gdk_pixbuf_new_from_stream (stream, NULL, NULL);

      g_free (decoded);
      g_input_stream_close (stream, NULL, NULL);
    }
  else if (_image_type == GTK_IMAGE_STOCK
           || _image_type == GTK_IMAGE_ICON_NAME)
    {
      ret = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                      _image_data,
                                      22,
                                      (GtkIconLookupFlags)0,
                                      NULL);
    }
  else if (_image_type == GTK_IMAGE_GICON)
    {
      GtkIconInfo *info;
      GIcon       *icon;

      icon = g_icon_new_for_string (_image_data, NULL);
      info = gtk_icon_theme_lookup_by_gicon (gtk_icon_theme_get_default (),
                                             icon,
                                             22,
                                             (GtkIconLookupFlags)0);
      if (info)
        ret = gtk_icon_info_load_icon (info, NULL);

      gtk_icon_info_free (info);
      g_object_unref (icon);
    }

  return ret;
}

void
IndicatorObjectEntryProxyRemote::SetActive (bool active)
{
  if (_active == active)
    return;

  _active = active;

  active_changed.emit (active);
  updated.emit ();
}

bool
IndicatorObjectEntryProxyRemote::GetActive ()
{
  return _active;
}

void
IndicatorObjectEntryProxyRemote::Refresh (const char *__id,
                                          const char *__label,
                                          bool        __label_sensitive,
                                          bool        __label_visible,
                                          guint32     __image_type,
                                          const char *__image_data,
                                          bool        __image_sensitive,
                                          bool        __image_visible)
{
  g_free (_id);
  g_free (_label);
  g_free (_image_data);

  _id = NULL;
  _label = NULL;
  _image_data = NULL;

  _id = g_strdup (__id);
  _label = g_strdup (__label);
  label_sensitive = __label_sensitive;
  label_visible = __label_visible;
  _image_type = __image_type;
  if (_image_type)
    _image_data = g_strdup (__image_data);
  icon_sensitive = __image_sensitive;
  icon_visible = __image_visible;

  updated.emit ();
}

const char *
IndicatorObjectEntryProxyRemote::GetId ()
{
  return _id;
}

void
IndicatorObjectEntryProxyRemote::ShowMenu (int x, int y, guint32 timestamp, guint32 button)
{
  OnShowMenuRequest.emit (_id, x, y, timestamp, button);
}

void
IndicatorObjectEntryProxyRemote::Scroll (int delta)
{
  OnScroll.emit(_id, delta);
}