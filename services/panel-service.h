/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef _PANEL_SERVICE_H_
#define _PANEL_SERVICE_H_

#include <glib-object.h>
#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>

G_BEGIN_DECLS

#define PANEL_TYPE_SERVICE (panel_service_get_type ())

#define PANEL_SERVICE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
        PANEL_TYPE_SERVICE, PanelService))

#define PANEL_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
        PANEL_TYPE_SERVICE, PanelServiceClass))

#define PANEL_IS_SERVICE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
        PANEL_TYPE_SERVICE))

#define PANEL_IS_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
        PANEL_TYPE_SERVICE))

#define PANEL_SERVICE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
        PANEL_TYPE_SERVICE, PanelServiceClass))

typedef struct _PanelService        PanelService;
typedef struct _PanelServiceClass   PanelServiceClass;
typedef struct _PanelServicePrivate PanelServicePrivate;

struct _PanelService
{
  GObject              parent;

  PanelServicePrivate *priv;
};

struct _PanelServiceClass
{
  GObjectClass   parent_class;

  /*< private >*/
  void (*_view_padding1) (void);
  void (*_view_padding2) (void);
  void (*_view_padding3) (void);
  void (*_view_padding4) (void);
  void (*_view_padding5) (void);
  void (*_view_padding6) (void);
};

GType             panel_service_get_type      (void) G_GNUC_CONST;

PanelService    * panel_service_get_default   ();
PanelService    * panel_service_get_default_with_indicators (GList *indicators);

guint             panel_service_get_n_indicators (PanelService *self);

IndicatorObject * panel_service_get_indicator_nth (PanelService *self, guint position);
IndicatorObject * panel_service_get_indicator (PanelService *self, const gchar *indicator_id);

void              panel_service_add_indicator (PanelService *self, IndicatorObject *indicator);

void              panel_service_remove_indicator (PanelService *self, IndicatorObject *indicator);

void              panel_service_clear_indicators (PanelService *self);

GVariant        * panel_service_sync          (PanelService *self);

GVariant        * panel_service_sync_one      (PanelService *self,
                                               const gchar  *indicator_id);
void              panel_service_sync_geometry (PanelService *self,
                                               const gchar  *indicator_id,
                                               const gchar  *entry_id,
                                               gint          x,
                                               gint          y,
                                               gint          width,
                                               gint          height);

void              panel_service_show_entry    (PanelService *self,
                                               const gchar  *entry_id,
                                               guint32       xid,
                                               gint32        x,
                                               gint32        y,
                                               guint32       button);

void              panel_service_show_entries  (PanelService *self,
                                               gchar       **entries,
                                               guint32       xid,
                                               gint32        x,
                                               gint32        y);

void              panel_service_show_app_menu (PanelService *self,
                                               guint32       xid,
                                               gint32        x,
                                               gint32        y);

void              panel_service_secondary_activate_entry (PanelService *self,
                                               const gchar  *entry_id);

void              panel_service_scroll_entry   (PanelService *self,
                                                const gchar  *entry_id,
                                                gint32       delta);

G_END_DECLS

#endif /* _PANEL_SERVICE_H_ */
