#ifndef _SERVICE_PANEL_H_
#define _SERVICE_PANEL_H_

#include <glib-object.h>
G_BEGIN_DECLS

#define SERVICE_TYPE_PANEL (service_panel_get_type ())

#define SERVICE_PANEL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
  SERVICE_TYPE_PANEL, ServicePanel))

#define SERVICE_PANEL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
  SERVICE_TYPE_PANEL, ServicePanelClass))

#define SERVICE_IS_PANEL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
  SERVICE_TYPE_PANEL))

#define SERVICE_IS_PANEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
  SERVICE_TYPE_PANEL))

#define ServicePanel_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
  SERVICE_TYPE_PANEL, ServicePanelClass))

typedef struct _ServicePanel        ServicePanel;
typedef struct _ServicePanelClass   ServicePanelClass;
typedef struct _ServicePanelPrivate ServicePanelPrivate;

struct _ServicePanel
{
  GObject parent;

  ServicePanelPrivate *priv;
};

struct _ServicePanelClass
{
  GObjectClass parent_class;
};

GType service_panel_get_type(void) G_GNUC_CONST;

ServicePanel* service_panel_new(void);

G_END_DECLS

#endif /* _SERVICE_PANEL_H_ */
