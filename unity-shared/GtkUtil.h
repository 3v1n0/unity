// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2013 Canonical Ltd
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
* Authored by: Sam Spilsbury <smspillaz@gmail.com>
*/

#ifndef GTK_UTIL_H
#define GTK_UTIL_H

#include <glib-2.0/glib-object.h>
#include <gtk/gtk.h>
#include <config.h>

namespace unity
{
namespace gtk
{
inline void UnreferenceIconInfo(GtkIconInfo *info)
{
#ifdef HAVE_GTK_3_8
  g_object_clear(info);
#else
  if (info)
    gtk_icon_info_free(info);
#endif
}
}
}

#endif
