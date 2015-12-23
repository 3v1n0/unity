/*
 * Copyright (C) 2015 Canonical Ltd
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
 * SECTION:unity-expander-view-accessible
 * @Title: UnityExpanderViewAccessible
 * @short_description: Implementation of the ATK interfaces for #ExpanderView
 * @see_also: ExpanderView
 *
 * #UnityExpanderViewAccessible implements the required ATK interfaces for
 * #ExpanderView, mainly exposing the text as his name, as this
 * #object is mainly used as a label
 *
 */

#include <NuxCore/Logger.h>
#include <glib/gi18n.h>

#include "unity-expander-view-accessible.h"

#include "ExpanderView.h"
#include "StaticCairoText.h"

DECLARE_LOGGER(logger, "unity.a11y.ExpanderView");

using namespace unity;

/* GObject */
static void unity_expander_view_accessible_class_init(UnityExpanderViewAccessibleClass* klass);
static void unity_expander_view_accessible_init(UnityExpanderViewAccessible* self);
static void unity_expander_view_accessible_dispose(GObject* object);

/* AtkObject.h */
static void         unity_expander_view_accessible_initialize(AtkObject* accessible,
                                                             gpointer   data);
static const gchar* unity_expander_view_accessible_get_name(AtkObject* obj);
static void  on_focus_changed_cb(nux::Area* area,
                                 bool has_focus,
                                 nux::KeyNavDirection direction,
                                 AtkObject* accessible);
static void  on_expanded_changed_cb(bool is_expanded,
                                    AtkObject* accessible);
static void  on_name_changed_cb(std::string name,
                                AtkObject* accessible);

G_DEFINE_TYPE(UnityExpanderViewAccessible, unity_expander_view_accessible,  NUX_TYPE_VIEW_ACCESSIBLE);


#define UNITY_EXPANDER_VIEW_ACCESSIBLE_GET_PRIVATE(obj)            \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_EXPANDER_VIEW_ACCESSIBLE, \
                                UnityExpanderViewAccessiblePrivate))

struct _UnityExpanderViewAccessiblePrivate
{
  gchar* name;
};


static void
unity_expander_view_accessible_class_init(UnityExpanderViewAccessibleClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  gobject_class->dispose = unity_expander_view_accessible_dispose;

  /* AtkObject */
  atk_class->initialize = unity_expander_view_accessible_initialize;
  atk_class->get_name = unity_expander_view_accessible_get_name;

  g_type_class_add_private(gobject_class, sizeof(UnityExpanderViewAccessiblePrivate));
}

static void
unity_expander_view_accessible_init(UnityExpanderViewAccessible* self)
{
  UnityExpanderViewAccessiblePrivate* priv =
    UNITY_EXPANDER_VIEW_ACCESSIBLE_GET_PRIVATE(self);

  self->priv = priv;
  self->priv->name = NULL;
}

static void
unity_expander_view_accessible_dispose(GObject* object)
{
  UnityExpanderViewAccessible* self = UNITY_EXPANDER_VIEW_ACCESSIBLE(object);

  if (self->priv->name != NULL)
  {
    g_free(self->priv->name);
    self->priv->name = NULL;
  }

  G_OBJECT_CLASS(unity_expander_view_accessible_parent_class)->dispose(object);
}

AtkObject*
unity_expander_view_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<ExpanderView*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(UNITY_TYPE_EXPANDER_VIEW_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
unity_expander_view_accessible_initialize(AtkObject* accessible,
                                         gpointer data)
{
  nux::Object* object = NULL;
  ExpanderView* view = NULL;

  ATK_OBJECT_CLASS(unity_expander_view_accessible_parent_class)->initialize(accessible, data);

  object = (nux::Object*)data;
  view = static_cast<ExpanderView*>(object);
  view->key_nav_focus_change.connect(sigc::bind(sigc::ptr_fun(on_focus_changed_cb), accessible));
  view->expanded.changed.connect(sigc::bind(sigc::ptr_fun(on_expanded_changed_cb), accessible));
  view->label.changed.connect(sigc::bind(sigc::ptr_fun(on_name_changed_cb), accessible));

  atk_object_set_role(accessible, ATK_ROLE_PANEL);
}

static const gchar*
unity_expander_view_accessible_get_name(AtkObject* obj)
{
  g_return_val_if_fail(UNITY_IS_EXPANDER_VIEW_ACCESSIBLE(obj), NULL);
  UnityExpanderViewAccessible* self = UNITY_EXPANDER_VIEW_ACCESSIBLE(obj);

  if (self->priv->name != NULL)
  {
    g_free(self->priv->name);
    self->priv->name = NULL;
  }

  self->priv->name = g_strdup(ATK_OBJECT_CLASS(unity_expander_view_accessible_parent_class)->get_name(obj));
  if (self->priv->name == NULL)
  {
    ExpanderView* view = NULL;

    view = dynamic_cast<ExpanderView*>(nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj)));

    if (view == NULL) /* state is defunct */
      return NULL;

    if (view->expanded)
      self->priv->name = g_strdup_printf(_("%s: expanded"), view->label().c_str());
    else
      self->priv->name = g_strdup_printf(_("%s: collapsed"), view->label().c_str());
  }

  return self->priv->name;
}

static void
on_focus_changed_cb(nux::Area* area,
                    bool has_focus,
                    nux::KeyNavDirection direction,
                    AtkObject* accessible)
{
  g_return_if_fail(UNITY_IS_EXPANDER_VIEW_ACCESSIBLE(accessible));

  LOG_WARN(logger) << "has_focus = " << has_focus;
  g_signal_emit_by_name(accessible, "focus-event", has_focus);
}

static void
on_expanded_changed_cb(bool is_expanded,
                     AtkObject* accessible)
{
  g_return_if_fail(UNITY_IS_EXPANDER_VIEW_ACCESSIBLE(accessible));

  g_object_notify(G_OBJECT(accessible), "accessible-name");
}

static void
on_name_changed_cb(std::string name,
                   AtkObject* accessible)
{
  g_return_if_fail(UNITY_IS_EXPANDER_VIEW_ACCESSIBLE(accessible));

  g_object_notify(G_OBJECT(accessible), "accessible-name");
}
