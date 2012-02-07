#ifndef _SERVICE_HUD_H_
#define _SERVICE_HUD_H_

#include <glib-object.h>
G_BEGIN_DECLS

#define SERVICE_TYPE_HUD (service_hud_get_type ())

#define SERVICE_HUD(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
  SERVICE_TYPE_HUD, ServiceHud))

#define SERVICE_HUD_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
  SERVICE_TYPE_HUD, ServiceHudClass))

#define SERVICE_IS_HUD(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
  SERVICE_TYPE_HUD))

#define SERVICE_IS_HUD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
  SERVICE_TYPE_HUD))

#define ServiceHud_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
  SERVICE_TYPE_HUD, ServiceHudClass))

typedef struct _ServiceHud        ServiceHud;
typedef struct _ServiceHudClass   ServiceHudClass;
typedef struct _ServiceHudPrivate ServiceHudPrivate;

struct _ServiceHud
{
  GObject parent;

  ServiceHudPrivate *priv;
};

struct _ServiceHudClass
{
  GObjectClass parent_class;
};

GType service_hud_get_type(void) G_GNUC_CONST;

ServiceHud* service_hud_new(void);

G_END_DECLS

#endif /* _SERVICE_HUD_H_ */
