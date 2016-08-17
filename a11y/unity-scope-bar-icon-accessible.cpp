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
 * Authored by: Luke Yelavich <luke.yelavich@canonical.com>
 */

/**
 * SECTION:unity-scope_bar_icon-accessible
 * @Title: UnityScopeBarIconAccessible
 * @short_description: Implementation of the ATK interfaces for #ScopeBarIcon
 * @see_also: ScopeBarIcon
 *
 * #UnityScopeBarIconAccessible implements the required ATK interfaces for
 * #ScopeBarIcon, mainly exposing the text as his name, as this
 * #object is mainly used as a label
 *
 */

#include <glib/gi18n.h>

#include "unity-scope-bar-icon-accessible.h"

#include "unitya11y.h"
#include "ScopeBarIcon.h"

using namespace unity::dash;

/* GObject */
static void unity_scope_bar_icon_accessible_class_init(UnityScopeBarIconAccessibleClass* klass);
static void unity_scope_bar_icon_accessible_init(UnityScopeBarIconAccessible* self);
static void unity_scope_bar_icon_accessible_dispose(GObject* object);

/* AtkObject.h */
static void         unity_scope_bar_icon_accessible_initialize(AtkObject* accessible,
                                                                    gpointer   data);
static const gchar* unity_scope_bar_icon_accessible_get_name(AtkObject* obj);
static void         on_focus_changed_cb(nux::Area* area,
                                        bool has_focus,
                                        nux::KeyNavDirection direction,
                                        AtkObject* accessible);
static void on_active_changed_cb(bool is_active,
                                 AtkObject* accessible);

G_DEFINE_TYPE(UnityScopeBarIconAccessible, unity_scope_bar_icon_accessible,  NUX_TYPE_VIEW_ACCESSIBLE);

#define UNITY_SCOPE_BAR_ICON_ACCESSIBLE_GET_PRIVATE(obj)           \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_SCOPE_BAR_ICON_ACCESSIBLE, \
                                UnityScopeBarIconAccessiblePrivate))

struct _UnityScopeBarIconAccessiblePrivate
{
  gchar* name;
};

static void
unity_scope_bar_icon_accessible_class_init(UnityScopeBarIconAccessibleClass* klass)
{
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->dispose = unity_scope_bar_icon_accessible_dispose;

  /* AtkObject */
  atk_class->get_name = unity_scope_bar_icon_accessible_get_name;
  atk_class->initialize = unity_scope_bar_icon_accessible_initialize;

  g_type_class_add_private(gobject_class, sizeof(UnityScopeBarIconAccessiblePrivate));
}

static void
unity_scope_bar_icon_accessible_init(UnityScopeBarIconAccessible* self)
{
  UnityScopeBarIconAccessiblePrivate* priv =
  UNITY_SCOPE_BAR_ICON_ACCESSIBLE_GET_PRIVATE(self);

  self->priv = priv;
  self->priv->name = NULL;
}

static void
unity_scope_bar_icon_accessible_dispose(GObject* object)
{
  UnityScopeBarIconAccessible* self = UNITY_SCOPE_BAR_ICON_ACCESSIBLE(object);

  if (self->priv->name != NULL)
  {
    g_free(self->priv->name);
    self->priv->name = NULL;
  }

  G_OBJECT_CLASS(unity_scope_bar_icon_accessible_parent_class)->dispose(object);
}

AtkObject*
unity_scope_bar_icon_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<ScopeBarIcon*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(UNITY_TYPE_SCOPE_BAR_ICON_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
unity_scope_bar_icon_accessible_initialize(AtkObject* accessible,
                                                gpointer data)
{
  nux::Object* nux_object = NULL;
  ScopeBarIcon* icon = NULL;

  ATK_OBJECT_CLASS(unity_scope_bar_icon_accessible_parent_class)->initialize(accessible, data);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(accessible));
  icon = dynamic_cast<ScopeBarIcon*>(nux_object);

  if (icon == NULL)
    return;

  icon->key_nav_focus_change.connect(sigc::bind(sigc::ptr_fun(on_focus_changed_cb), accessible));

  icon->active.changed.connect(sigc::bind(sigc::ptr_fun(on_active_changed_cb), accessible));

  atk_object_set_role(accessible, ATK_ROLE_PUSH_BUTTON);
}

static const gchar*
unity_scope_bar_icon_accessible_get_name(AtkObject* obj)
{
  g_return_val_if_fail(UNITY_IS_SCOPE_BAR_ICON_ACCESSIBLE(obj), NULL);
  UnityScopeBarIconAccessible* self = UNITY_SCOPE_BAR_ICON_ACCESSIBLE(obj);

  if (self->priv->name)
  {
    g_free(self->priv->name);
    self->priv->name = NULL;
  }

  self->priv->name = g_strdup(ATK_OBJECT_CLASS(unity_scope_bar_icon_accessible_parent_class)->get_name(obj));
  if (self->priv->name == NULL)
  {
    ScopeBarIcon* icon = NULL;

    icon = dynamic_cast<ScopeBarIcon*>(nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj)));
    if (icon != NULL)
    {
      if (icon->active())
        self->priv->name = g_strdup_printf(_("%s: selected"), icon->name().c_str());
      else
        self->priv->name = g_strdup(icon->name().c_str());
    }
  }

  return self->priv->name;
}

static void
on_focus_changed_cb(nux::Area* area,
                    bool has_focus,
                    nux::KeyNavDirection direction,
                    AtkObject* accessible)
{
  g_return_if_fail(UNITY_IS_SCOPE_BAR_ICON_ACCESSIBLE(accessible));

  g_signal_emit_by_name(accessible, "focus-event", has_focus);
}

static void
on_active_changed_cb(bool is_active,
                     AtkObject* accessible)
{
  g_return_if_fail(UNITY_IS_SCOPE_BAR_ICON_ACCESSIBLE(accessible));

  g_object_notify(G_OBJECT(accessible), "accessible-name");
}
