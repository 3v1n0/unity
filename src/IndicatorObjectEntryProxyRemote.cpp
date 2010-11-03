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

enum
{
  COL_ID,
  COL_LABEL,
  COL_ICON_HINT,
  COL_ICON_DATA,
  COL_LABEL_VISIBLE,
  COL_ICON_VISIBLE,
  COL_LABEL_SENSITIVE,
  COL_ICON_SENSITIVE
};

IndicatorObjectEntryProxyRemote::IndicatorObjectEntryProxyRemote (DeeModel     *model,
                                                                  DeeModelIter *iter)
: _model (model),
  _iter (iter),
  _active (false)
{
  label_visible = false;
  label_sensitive = true;
  icon_visible = false;
  icon_sensitive = true;
  _active = false;

  Refresh ();
}


IndicatorObjectEntryProxyRemote::~IndicatorObjectEntryProxyRemote ()
{

}

const char *
IndicatorObjectEntryProxyRemote::GetLabel ()
{
  return dee_model_get_string (_model, _iter, COL_LABEL);
}

GdkPixbuf *
IndicatorObjectEntryProxyRemote::GetPixbuf ()
{
  GdkPixbuf *ret = NULL;
  guint32 icon_hint = dee_model_get_uint (_model, _iter, COL_ICON_HINT);

  if (icon_hint == GTK_IMAGE_PIXBUF)
    {
      guchar       *decoded;
      GInputStream *stream;
      gsize         len = 0;
     
      decoded = g_base64_decode (dee_model_get_string (_model, _iter, COL_ICON_DATA), &len);
      stream = g_memory_input_stream_new_from_data (decoded, len, NULL);

      ret = gdk_pixbuf_new_from_stream (stream, NULL, NULL);

      g_free (decoded);
      g_input_stream_close (stream, NULL, NULL);
    }
  else if (icon_hint == GTK_IMAGE_STOCK
           || icon_hint == GTK_IMAGE_ICON_NAME)
    {
      ret = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                      dee_model_get_string (_model, _iter, COL_ICON_DATA),
                                      22,
                                      (GtkIconLookupFlags)0,
                                      NULL);
    }
  else if (icon_hint == GTK_IMAGE_GICON)
    {
      GtkIconInfo *info;
      GIcon       *icon;

      icon = g_icon_new_for_string (dee_model_get_string (_model, _iter, COL_ICON_DATA), NULL);
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

  Updated.emit ();
}

bool
IndicatorObjectEntryProxyRemote::GetActive ()
{
  return _active;
}

void
IndicatorObjectEntryProxyRemote::Refresh ()
{
  label_visible = dee_model_get_bool (_model, _iter, COL_LABEL_VISIBLE);
  //label_sensitive = dee_model_get_bool (_model, _iter, COL_LABEL_SENSITIVE); FIXME: Re-enable these when the service supports them
  icon_visible = dee_model_get_bool (_model, _iter, COL_ICON_VISIBLE);
  //icon_sensitive = dee_model_get_bool (_model, _iter, COL_ICON_SENSITIVE);

  Updated.emit ();
}

const char *
IndicatorObjectEntryProxyRemote::GetId ()
{
  return dee_model_get_string (_model, _iter, COL_ID);
}

void
IndicatorObjectEntryProxyRemote::ShowMenu (int x, int y, guint32 timestamp)
{
  OnShowMenuRequest.emit (dee_model_get_string (_model, _iter, COL_ID), x, y, timestamp);  
}
