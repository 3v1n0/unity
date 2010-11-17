/*
 * Copyright 2010 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */


#include "config.h"

#include "panel-service.h"

static void TestAllocation   (void);

void
TestPanelServiceCreateSuite ()
{
#define _DOMAIN "/Unit/PanelService"

  g_test_add_func (_DOMAIN"/Allocation", TestAllocation);
}

static void
TestAllocation ()
{
  PanelService *service;

  service = panel_service_get_default ();
  g_assert (PANEL_IS_SERVICE (service));

  g_object_unref (service);
}
