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

/* AtkObject.h */
static void          unity_launcher_icon_accessible_initialize (AtkObject *accessible,
                                                                gpointer   data);
static const gchar * unity_launcher_icon_accessible_get_name   (AtkObject *obj);


G_DEFINE_TYPE (UnityLauncherIconAccessible, unity_launcher_icon_accessible,  NUX_TYPE_OBJECT_ACCESSIBLE)

static void
unity_launcher_icon_accessible_class_init (UnityLauncherIconAccessibleClass *klass)
{
  AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

  /* AtkObject */
  atk_class->initialize = unity_launcher_icon_accessible_initialize;
  atk_class->get_name = unity_launcher_icon_accessible_get_name;
}

static void
unity_launcher_icon_accessible_init (UnityLauncherIconAccessible *launcher_icon_accessible)
{
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
  ATK_OBJECT_CLASS (unity_launcher_icon_accessible_parent_class)->initialize (accessible, data);

  accessible->role = ATK_ROLE_PUSH_BUTTON;
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
