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
  [CCode (cname = "INDICATORDIR")]
  public static const string INDICATORDIR;
  [CCode (cname = "INDICATORICONSDIR")]
  public static const string INDICATORICONSDIR;

  [CCode (cheader_filename = "unity-utils.h", cname = "LOGGER_START_PROCESS")]
  public static void LOGGER_START_PROCESS (string name);

  [CCode (cheader_filename = "unity-utils.h", cname = "LOGGER_END_PROCESS")]
  public static void LOGGER_END_PROCESS (string name);

  [CCode (cheader_filename = "unity-utils.h", cname = "START_FUNCTION")]
  public static void START_FUNCTION ();

  [CCode (cheader_filename = "unity-utils.h", cname = "END_FUNCTION")]
  public static void END_FUNCTION ();

  // I DO NOT BELIEVE I HAVE TO DO THIS! >_<
  [CCode (cname = "G_TYPE_FLOAT")]
  public static const GLib.Type G_TYPE_FLOAT;
}
