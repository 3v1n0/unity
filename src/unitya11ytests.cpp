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
 * Authored by: Alejandro Piñeiro Iglesias <apinheiro@igalia.com>
 */

#include "unitya11ytests.h"

#include <glib.h>

#include "unitya11y.h"
#include "unity-util-accessible.h"

/* nux accessible objects */
#include "nux-view-accessible.h"
#include "nux-base-window-accessible.h"
#include "nux-layout-accessible.h"

/* unity accessible objects */
#include "Launcher.h"
#include "LauncherIcon.h"
#include "SimpleLauncherIcon.h"
#include "PanelView.h"
#include "PlacesView.h"
#include "unity-launcher-accessible.h"
#include "unity-launcher-icon-accessible.h"
#include "unity-panel-view-accessible.h"
#include "unity-panel-home-button-accessible.h"

/*
 * This unit test checks if the destroy management is working:
 *
 * - If the state of a accessibility object is properly updated after
 *   the object destruction
 *
 */
static gboolean
a11y_unit_test_destroy_management (void)
{
  QuicklistView *quicklist = NULL;
  AtkObject *accessible = NULL;
  nux::Object *base_object = NULL;
  AtkStateSet *state_set = NULL;

  quicklist = new QuicklistView ();
  quicklist->SinkReference ();
  accessible = unity_a11y_get_accessible (quicklist);

  base_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (accessible));
  if (base_object != quicklist)
    {
      g_debug ("[a11y] destroy management unit test: base object"
               " different to the original one");
      return FALSE;
    }

  if (quicklist->UnReference () == false)
    {
      g_debug ("[a11y] destroy management unit test: base object not destroyed");
      return FALSE;
    }

  base_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (accessible));
  if (base_object != NULL)
    {
      g_debug ("[a11y] destroy management unit test: base object"
               " not NULL after base object destruction");
      return FALSE;
    }

  state_set = atk_object_ref_state_set (accessible);
  if (!atk_state_set_contains_state (state_set, ATK_STATE_DEFUNCT))
    {
      g_debug ("[a11y] destroy management unit test: accessible object"
               " doesn't include DEFUNCT state");
      return FALSE;
    }

  g_object_unref (state_set);
  g_object_unref (accessible);

  return TRUE;
}

/**
 * This unit test checks if the hash table destroy management is working:
 *
 * - If the hash table removes properly the accessible object once it
 *   is destroyed.
 */
static gboolean
a11y_unit_test_hash_table_destroy_management (void)
{
  QuicklistView *quicklist = NULL;
  nux::Layout *layout = NULL;
  AtkObject *accessible = NULL;
  guint prev_hash_size = 0;
  guint hash_size = 0;

  /* test the hash table management with the accessible destroy */

  prev_hash_size = g_hash_table_size (_unity_a11y_get_accessible_table ());

  layout = new nux::Layout ();
  layout->SinkReference ();
  accessible = unity_a11y_get_accessible (layout);

  if (accessible == NULL)
    {
      g_debug ("[a11y] hash table destroy management unit test: error creating"
               " the accessible object (accessible == NULL)");
      return FALSE;
    }

  hash_size = g_hash_table_size (_unity_a11y_get_accessible_table ());

  if ((hash_size - prev_hash_size) != 1 )
    {
      g_debug ("[a11y] hash table destroy management unit test: accessible object"
               " not added to the hash table after his creation");
      return FALSE;
    }

  prev_hash_size = g_hash_table_size (_unity_a11y_get_accessible_table ());

  g_object_unref (accessible);

  hash_size = g_hash_table_size (_unity_a11y_get_accessible_table ());

  if ((prev_hash_size - hash_size) != 1 )
    {
      g_debug ("[a11y] hash table destroy management unit test: accessible object"
               " not removed from the hash table after his destruction");
      return FALSE;
    }

  layout->UnReference ();

  /* Test the hash table management after the object destroy */

  prev_hash_size = g_hash_table_size (_unity_a11y_get_accessible_table ());

  quicklist = new QuicklistView ();
  quicklist->SinkReference ();
  accessible = unity_a11y_get_accessible (quicklist);

  if (accessible == NULL)
    {
      g_debug ("[a11y] hash table destroy management unit test: error creating"
               " the accessible object (accessible == NULL)");
      return FALSE;
    }

  hash_size = g_hash_table_size (_unity_a11y_get_accessible_table ());

  if ((hash_size - prev_hash_size) != 1 )
    {
      g_debug ("[a11y] hash table destroy management unit test: accessible object"
               " not added to the hash table after his creation");
      return FALSE;
    }

  prev_hash_size = g_hash_table_size (_unity_a11y_get_accessible_table ());

  if (quicklist->UnReference () == false)
    {
      g_debug ("[a11y] hash table destroy management unit test: base object not destroyed");
      return FALSE;
    }

  hash_size = g_hash_table_size (_unity_a11y_get_accessible_table ());

  if ((prev_hash_size - hash_size) != 1 )
    {
      g_debug ("[a11y] hash table destroy management unit test: accessible object"
               " not removed from the hash table after base object destruction");
      return FALSE;
    }

  return TRUE;
}

/**
 * This unit test checks if the launcher connection process works
 */
static gboolean
a11y_unit_test_launcher_connection (void)
{
  Launcher *launcher = NULL;
  nux::BaseWindow *window = NULL;
  AtkObject *launcher_accessible = NULL;
  LauncherIcon *launcher_icon = NULL;
  AtkObject *launcher_icon_accessible = NULL;

  window = new nux::BaseWindow (TEXT(""));
  launcher = new Launcher (window, NULL);
  launcher->SinkReference ();
  launcher_accessible = unity_a11y_get_accessible (launcher);

  if (!UNITY_IS_LAUNCHER_ACCESSIBLE (launcher_accessible))
    {
      g_debug ("[a11y] wrong launcher accessible type");
      return FALSE;
    }
  else
    {
      g_debug ("[a11y] Launcher accessible created correctly");
    }

  launcher_icon = new SimpleLauncherIcon (launcher);
  launcher_icon->SinkReference ();
  launcher_icon_accessible = unity_a11y_get_accessible (launcher_icon);

  if (!UNITY_IS_LAUNCHER_ICON_ACCESSIBLE (launcher_icon_accessible))
    {
      g_debug ("[a11y] wrong launcher icon accessible type");
      return FALSE;
    }
  else
    {
      g_debug ("[a11y] LauncherIcon accessible created correctly");
    }

  launcher->UnReference ();
  launcher_icon->UnReference ();

  return TRUE;
}

/* public */

void
unity_run_a11y_unit_tests (void)
{
  if (a11y_unit_test_destroy_management ())
    g_debug ("[a11y] destroy management unit test: SUCCESS");
  else
    g_debug ("[a11y] destroy management unit test: FAIL");

  if (a11y_unit_test_hash_table_destroy_management ())
    g_debug ("[a11y] hash table destroy management unit test: SUCCESS");
  else
    g_debug ("[a11y] hash table destroy management unit test: FAIL");

  if (a11y_unit_test_launcher_connection ())
    g_debug ("[a11y] launcher connection: SUCCESS");
  else
    g_debug ("[a11y] launcher connection: FAIL");

}
