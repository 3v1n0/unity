/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Rodrigo Moya <rodrigo.moya@canonical.com>
 */

#include <gio/gio.h>
#include <atk-bridge.h>

#include "panel-a11y.h"
#include "panel-util-accessible.h"

static gboolean a11y_initialized = FALSE;

void
panel_a11y_init (void)
{
  if (a11y_initialized)
    return;

  /* Restore environment to load AT bridge */
  g_unsetenv ("NO_AT_BRIDGE");
  g_unsetenv ("NO_GAIL");

  /* Load PanelUtilAccessible class */
  g_type_class_unref (g_type_class_ref (PANEL_TYPE_UTIL_ACCESSIBLE));
  atk_bridge_adaptor_init(NULL, NULL);

  a11y_initialized = TRUE;
}
