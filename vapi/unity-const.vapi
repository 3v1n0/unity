/* -*- Mode: vala; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- *//*
 * Copyright (C) 2009 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * idst under the terms of the GNU General Public License version 3 as
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
 * Authored by Gordon Allott <gord.allott@canonical.com>
 *
 */

namespace Unity {
  [CCode (cname = "DATADIR")]
  public static const string DATADIR;
  [CCode (cname = "PKGDATADIR")]
  public static const string PKGDATADIR;
  
  [CCode (cheader_filename = "unity-utils.h", cname = "LOGGER_START_PROCESS")]
  public static void LOGGER_START_PROCESS (string name);
  
  [CCode (cheader_filename = "unity-utils.h", cname = "LOGGER_END_PROCESS")]
  public static void LOGGER_END_PROCESS (string name);
  
  [CCode (cheader_filename = "unity-utils.h", cname = "START_FUNCTION")]
  public static void START_FUNCTION ();
  
  [CCode (cheader_filename = "unity-utils.h", cname = "END_FUNCTION")]
  public static void END_FUNCTION ();

  [CCode (cheader_filename = "gdk/gdkx.h", cname = "gdk_x11_visual_get_xvisual")]
  public static unowned X.Visual _x11_visual_get_xvisual (Gdk.Visual visual);

  [CCode (cheader_filename = "unity-utils.h", cname = "XVisualIDFromVisual")]
  public static ulong _x_visual_id_from_visual (X.Visual visual);
}
