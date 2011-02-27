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

/**
 * SECTION:nux-launcher_icon-accessible
 * @Title: UnityLauncherIconAccessible
 * @short_description: Implementation of the ATK interfaces for #nux::LauncherIcon
 * @see_also: nux::LauncherIcon
 *
 * #UnityLauncherIconAccessible implements the required ATK interfaces of
 * nux::LauncherIcon, exposing the common elements on each basic individual
 * element (position, extents, etc)
 *
 */

#include "unity-launcher-icon-accessible.h"
#include "LauncherIcon.h"

/* GObject */
static void unity_launcher_icon_accessible_class_init (UnityLauncherIconAccessibleClass *klass);
static void unity_launcher_icon_accessible_init       (UnityLauncherIconAccessible *launcher_icon_accessible);
static void unity_launcher_icon_accessible_finalize   (GObject *object);


/* AtkObject.h */
static void          unity_launcher_icon_accessible_initialize (AtkObject *accessible,
                                                                gpointer   data);
static AtkStateSet*  unity_launcher_icon_accessible_ref_state_set (AtkObject *obj);
static const gchar * unity_launcher_icon_accessible_get_name   (AtkObject *obj);

/* private/utility methods*/
static void check_selected (UnityLauncherIconAccessible *self);
static void on_selection_change_cb (UnityLauncherIconAccessible *icon_accessible);


G_DEFINE_TYPE (UnityLauncherIconAccessible, unity_launcher_icon_accessible,  NUX_TYPE_OBJECT_ACCESSIBLE)

#define UNITY_LAUNCHER_ICON_ACCESSIBLE_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_LAUNCHER_ICON_ACCESSIBLE, \
                                UnityLauncherIconAccessiblePrivate))

struct _UnityLauncherIconAccessiblePrivate
{
  /* Cached values (used to avoid extra notifications) */
  gboolean selected;

  sigc::connection on_selection_change_connection;
};

static void
unity_launcher_icon_accessible_class_init (UnityLauncherIconAccessibleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

  gobject_class->finalize = unity_launcher_icon_accessible_finalize;

  /* AtkObject */
  atk_class->initialize = unity_launcher_icon_accessible_initialize;
  atk_class->get_name = unity_launcher_icon_accessible_get_name;
  atk_class->ref_state_set = unity_launcher_icon_accessible_ref_state_set;

  g_type_class_add_private (gobject_class, sizeof (UnityLauncherIconAccessiblePrivate));
}

static void
unity_launcher_icon_accessible_init (UnityLauncherIconAccessible *launcher_icon_accessible)
{
  UnityLauncherIconAccessiblePrivate *priv =
    UNITY_LAUNCHER_ICON_ACCESSIBLE_GET_PRIVATE (launcher_icon_accessible);

  launcher_icon_accessible->priv = priv;
}

static void
unity_launcher_icon_accessible_finalize (GObject *object)
{
  UnityLauncherIconAccessible *self = UNITY_LAUNCHER_ICON_ACCESSIBLE (object);

  self->priv->on_selection_change_connection.disconnect ();

  G_OBJECT_CLASS (unity_launcher_icon_accessible_parent_class)->finalize (object);
}


AtkObject*
unity_launcher_icon_accessible_new (nux::Object *object)
{
  AtkObject *accessible = NULL;

  g_return_val_if_fail (dynamic_cast<LauncherIcon *>(object), NULL);

  accessible = ATK_OBJECT (g_object_new (UNITY_TYPE_LAUNCHER_ICON_ACCESSIBLE, NULL));

  atk_object_initialize (accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
unity_launcher_icon_accessible_initialize (AtkObject *accessible,
                                           gpointer data)
{
  LauncherIcon *icon = NULL;
  Launcher *launcher = NULL;
  UnityLauncherIconAccessible *self = NULL;

  ATK_OBJECT_CLASS (unity_launcher_icon_accessible_parent_class)->initialize (accessible, data);
  self = UNITY_LAUNCHER_ICON_ACCESSIBLE (accessible);

  accessible->role = ATK_ROLE_PUSH_BUTTON;

  icon = (LauncherIcon*)data;
  launcher = icon->GetLauncher ();

  if (launcher != NULL)
    {
      self->priv->on_selection_change_connection =
        launcher->selection_change.connect (sigc::bind (sigc::ptr_fun (on_selection_change_cb), self));
    }

  /* Check the cached selected state and notify the first selection.
   * Ie: it is required to ensure a first notification
   */
  check_selected (UNITY_LAUNCHER_ICON_ACCESSIBLE (accessible));
}


static const gchar *
unity_launcher_icon_accessible_get_name (AtkObject *obj)
{
  const gchar *name;

  g_return_val_if_fail (UNITY_IS_LAUNCHER_ICON_ACCESSIBLE (obj), NULL);

  name = ATK_OBJECT_CLASS (unity_launcher_icon_accessible_parent_class)->get_name (obj);
  if (name == NULL)
    {
      LauncherIcon *icon = NULL;

      icon = dynamic_cast<LauncherIcon *>(nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (obj)));

      if (icon == NULL) /* State is defunct */
        name = NULL;
      else
        name = icon->GetTooltipText().GetTCharPtr ();
    }

  return name;
}

static AtkStateSet*
unity_launcher_icon_accessible_ref_state_set (AtkObject *obj)
{
  AtkStateSet *state_set = NULL;
  UnityLauncherIconAccessible *self = NULL;
  nux::Object *nux_object = NULL;

  g_return_val_if_fail (UNITY_IS_LAUNCHER_ICON_ACCESSIBLE (obj), NULL);
  self = UNITY_LAUNCHER_ICON_ACCESSIBLE (obj);

  state_set = ATK_OBJECT_CLASS (unity_launcher_icon_accessible_parent_class)->ref_state_set (obj);

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (obj));

  if (nux_object == NULL) /* actor is defunct */
    return state_set;

  if (self->priv->selected)
    atk_state_set_add_state (state_set, ATK_STATE_SELECTED);

  return state_set;
}

/* private methods */

/*
 * Checks if the current item is selected, and notify a selection
 * change if the selection has changed
 */
static void
check_selected (UnityLauncherIconAccessible *self)
{
  LauncherIcon *icon = NULL;
  LauncherIcon *selected_icon = NULL;
  Launcher *launcher = NULL;
  nux::Object *nux_object = NULL;
  gboolean found = FALSE;

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (self));
  icon = dynamic_cast<LauncherIcon *>(nux_object);
  launcher = icon->GetLauncher ();

  if (launcher == NULL)
    return;

  selected_icon = launcher->GetSelectedMenuIcon ();

  if (icon == selected_icon)
    found = TRUE;

  if (found != self->priv->selected)
    {
      self->priv->selected = found;
      atk_object_notify_state_change (ATK_OBJECT (self),
                                      ATK_STATE_SELECTED,
                                      found);
      g_debug ("[LAUNCHER-ICON]: selected state changed (%p:%i)", self, found);
    }
}

static void
on_selection_change_cb (UnityLauncherIconAccessible *icon_accessible)
{
  check_selected (icon_accessible);
}

