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

/**
 * SECTION:unity-panel-home-button-accessible
 * @Title: UnityPanelHomeButtonAccessible
 * @short_description: Implementation of the ATK interfaces for #PanelHomeButton
 * @see_also: PanelHomeButton
 *
 * #UnityPanelHomeButtonAccessible implements the required ATK interfaces for
 * #PanelHomeButton.
 *
 */

#include <glib/gi18n-lib.h>
#include <Nux/Nux.h>
#include "PanelHomeButton.h"
#include "unity-panel-home-button-accessible.h"

#include "unitya11y.h"

/* GObject */
static void unity_panel_home_button_accessible_class_init (UnityPanelHomeButtonAccessibleClass *klass);
static void unity_panel_home_button_accessible_init       (UnityPanelHomeButtonAccessible *self);

/* AtkObject */
static void       unity_panel_home_button_accessible_initialize     (AtkObject *accessible, gpointer data);

#define UNITY_PANEL_HOME_BUTTON_ACCESSIBLE_GET_PRIVATE(obj)                      \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_PANEL_HOME_BUTTON_ACCESSIBLE, UnityPanelHomeButtonAccessiblePrivate))

G_DEFINE_TYPE (UnityPanelHomeButtonAccessible, unity_panel_home_button_accessible,  NUX_TYPE_VIEW_ACCESSIBLE)

struct _UnityPanelHomeButtonAccessiblePrivate
{
};

static void
unity_panel_home_button_accessible_class_init (UnityPanelHomeButtonAccessibleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

  /* AtkObject */
  atk_class->initialize = unity_panel_home_button_accessible_initialize;

  g_type_class_add_private (gobject_class, sizeof (UnityPanelHomeButtonAccessiblePrivate));
}

static void
unity_panel_home_button_accessible_init (UnityPanelHomeButtonAccessible *self)
{
  self->priv = UNITY_PANEL_HOME_BUTTON_ACCESSIBLE_GET_PRIVATE (self);
}

AtkObject *
unity_panel_home_button_accessible_new (nux::Object *object)
{
  AtkObject *accessible;

  g_return_val_if_fail (dynamic_cast<PanelHomeButton *>(object), NULL);

  accessible = ATK_OBJECT (g_object_new (UNITY_TYPE_PANEL_HOME_BUTTON_ACCESSIBLE, NULL));

  atk_object_initialize (accessible, object);

  return accessible;
}

static void
unity_panel_home_button_accessible_initialize (AtkObject *accessible, gpointer data)
{
  ATK_OBJECT_CLASS (unity_panel_home_button_accessible_parent_class)->initialize (accessible, data);

  accessible->role = ATK_ROLE_PUSH_BUTTON;
  atk_object_set_name (accessible, _("Home Button"));
}
