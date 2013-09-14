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

#include <glib.h>
#include <gio/gio.h>
#include <gmodule.h>
#include <stdio.h>
#include <atk-bridge.h>

#include "unitya11y.h"
#include "unitya11ytests.h"
#include "unity-util-accessible.h"

/* nux accessible objects */
#include "nux-view-accessible.h"
#include "nux-base-window-accessible.h"
#include "nux-layout-accessible.h"
#include "nux-text-entry-accessible.h"

/* unity accessible objects */
#include "Launcher.h"
#include "LauncherIcon.h"
#include "SimpleLauncherIcon.h"
#include "PanelView.h"
#include "DashView.h"
#include "PlacesGroup.h"
#include "QuicklistView.h"
#include "QuicklistMenuItem.h"
#include "SwitcherView.h"
#include "unity-launcher-accessible.h"
#include "unity-launcher-icon-accessible.h"
#include "unity-panel-view-accessible.h"
#include "unity-dash-view-accessible.h"
#include "unity-search-bar-accessible.h"
#include "unity-sctext-accessible.h"
#include "unity-rvgrid-accessible.h"
#include "unity-places-group-accessible.h"
#include "unity-quicklist-accessible.h"
#include "unity-quicklist-menu-item-accessible.h"
#include "unity-switcher-accessible.h"

using namespace unity;
using namespace unity::dash;
using namespace unity::launcher;

static GHashTable* accessible_table = NULL;
/* FIXME: remove accessible objects when not required anymore */

static gboolean a11y_initialized = FALSE;

static void
unity_a11y_restore_environment(void)
{
  g_unsetenv("NO_AT_BRIDGE");
  g_unsetenv("NO_GAIL");
}

static void
load_unity_atk_util(nux::WindowThread* wt)
{
  unity_util_accessible_set_window_thread(wt);
  g_type_class_unref(g_type_class_ref(UNITY_TYPE_UTIL_ACCESSIBLE));
}

/*
 * In order to avoid the atk-bridge loading and the GAIL
 * initialization during the gtk_init, it is required to set some
 * environment vars.
 *
 */
void
unity_a11y_preset_environment(void)
{
  g_setenv("NO_AT_BRIDGE", "1", TRUE);
  g_setenv("NO_GAIL", "1", TRUE);
}

/*
 * Initializes the accessibility (ATK) support on Unity
 *
 */
void
unity_a11y_init(nux::WindowThread* wt)
{
  if (a11y_initialized)
    return;

  unity_a11y_restore_environment();
  load_unity_atk_util(wt);
  atk_bridge_adaptor_init(NULL, NULL);
  atk_get_root();

  a11y_initialized = TRUE;

// NOTE: we run the unit tests manually while developing by
// uncommenting this. Take a look at the explanation in the
// unitya11ytests.h header for more information

//  unity_run_a11y_unit_tests ();
}

/*
 * Finalize the issues related with accessibility.
 *
 * It mainly cleans the resources related with accessibility
 */
void
unity_a11y_finalize(void)
{
  if (accessible_table != NULL)
  {
    g_hash_table_unref(accessible_table);
    accessible_table = NULL;
  }
  a11y_initialized = FALSE;
}


/*
 * Creates the accessible object for a nux::Area object
 *
 * Method factory, equivalent to
 * atk_object_factory_creeate_accessible, but required because
 * AtkObjectFactory gives only support for GObject classes.
 *
 * FIXME: this should be a temporal method. The best way to implement
 * that would be add a ->get_accessible method on the nux::View
 * subclasses itself.
 *
 * WARNING: as a reason the previous comment is true. Take into
 * account that you should be careful with the order in which you add
 * those defines. The order will be from more specific classes to more
 * abstracted classes.
 *
 */

static AtkObject*
unity_a11y_create_accessible(nux::Object* object)
{
  /* UNITY classes*/
  if (object->Type().IsDerivedFromType(Launcher::StaticObjectType))
    return unity_launcher_accessible_new(object);

  if (object->Type().IsDerivedFromType(LauncherIcon::StaticObjectType))
    return unity_launcher_icon_accessible_new(object);

  if (object->Type().IsDerivedFromType(PanelView::StaticObjectType))
    return unity_panel_view_accessible_new(object);

  if (object->Type().IsDerivedFromType(DashView::StaticObjectType))
    return unity_dash_view_accessible_new(object);

  if (object->Type().IsDerivedFromType(PlacesGroup::StaticObjectType))
    return unity_places_group_accessible_new(object);

  if (object->Type().IsDerivedFromType(QuicklistView::StaticObjectType))
    return unity_quicklist_accessible_new(object);

  if (object->Type().IsDerivedFromType(QuicklistMenuItem::StaticObjectType))
    return unity_quicklist_menu_item_accessible_new(object);

  if (object->Type().IsDerivedFromType(StaticCairoText::StaticObjectType))
    return unity_sctext_accessible_new(object);

  if (object->Type().IsDerivedFromType(unity::dash::ResultViewGrid::StaticObjectType))
    return unity_rvgrid_accessible_new(object);

  if (object->Type().IsDerivedFromType(unity::SearchBar::StaticObjectType))
    return unity_search_bar_accessible_new(object);

  if (object->Type().IsDerivedFromType(unity::switcher::SwitcherView::StaticObjectType))
    return unity_switcher_accessible_new(object);

  /* NUX classes  */
  if (object->Type().IsDerivedFromType(nux::TextEntry::StaticObjectType))
    return nux_text_entry_accessible_new(object);

  if (object->Type().IsDerivedFromType(nux::BaseWindow::StaticObjectType))
    return nux_base_window_accessible_new(object);

  if (object->Type().IsDerivedFromType(nux::View::StaticObjectType))
    return nux_view_accessible_new(object);

  if (object->Type().IsDerivedFromType(nux::Layout::StaticObjectType))
    return nux_layout_accessible_new(object);

  if (object->Type().IsDerivedFromType(nux::Area::StaticObjectType))
    return nux_area_accessible_new(object);

  return nux_object_accessible_new(object);
}

static void
on_object_destroy_cb(nux::Object* base_object,
                     AtkObject* accessible_object)
{
  /* NOTE: the pair key:value (base_object:accessible_object) could be
     already removed on on_accessible_destroy_cb. That just means that
     g_hash_table_remove would return FALSE. We don't add a
     debug/warning message to avoid being too verbose */

  g_hash_table_remove(accessible_table, base_object);
}

static void
on_accessible_destroy_cb(gpointer data,
                         GObject* where_the_object_was)
{
  /* NOTE: the pair key:value (base_object:accessible_object) could be
     already removed on on_object_destroy_cb. That just means that
     g_hash_table_remove would return FALSE. We don't add a
     debug/warning message to avoid being too verbose */

  g_hash_table_remove(accessible_table, data);
}

/*
 * Returns the accessible object of a nux::View object
 *
 * This method tries to:
 *   * Check if area already has a accessibility object
 *   * If this is the case, return that
 *   * If not, create it and return the object
 *
 * FIXME: this should be a temporal method. The best way to implement
 * that would be add a ->get_accessible method on the nux::View
 * subclasses itself.
 *
 */
AtkObject*
unity_a11y_get_accessible(nux::Object* object)
{
  AtkObject* accessible_object = NULL;

  g_return_val_if_fail(object != NULL, NULL);

  if (accessible_table == NULL)
  {
    accessible_table = g_hash_table_new(g_direct_hash, g_direct_equal);
  }

  accessible_object = ATK_OBJECT(g_hash_table_lookup(accessible_table, object));
  if (accessible_object == NULL)
  {
    accessible_object = unity_a11y_create_accessible(object);

    g_hash_table_insert(accessible_table, object, accessible_object);

    /* there are two reasons the object should be removed from the
     * table: base object destroyed or accessible object
     * destroyed
     */
    g_object_weak_ref(G_OBJECT(accessible_object),
                      on_accessible_destroy_cb,
                      object);

    object->OnDestroyed.connect(sigc::bind(sigc::ptr_fun(on_object_destroy_cb),
                                           accessible_object));
  }

  return accessible_object;
}

/*
 * Returns if the accessibility support is properly initialized
 */
gboolean unity_a11y_initialized(void)
{
  return a11y_initialized;
}

/* Returns the accessible_table. Just for unit testing purposes, you
   should not require to use it */
GHashTable* _unity_a11y_get_accessible_table()
{
  return accessible_table;
}
