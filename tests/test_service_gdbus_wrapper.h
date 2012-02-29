#ifndef _SERVICE_GDBUS_WRAPPER_H_
#define _SERVICE_GDBUS_WRAPPER_H_

#include <glib-object.h>
G_BEGIN_DECLS

#define SERVICE_TYPE_GDBUS_WRAPPER (service_gdbus_wrapper_get_type ())

#define SERVICE_GDBUS_WRAPPER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
  SERVICE_TYPE_GDBUS_WRAPPER, ServiceGDBusWrapper))

#define SERVICE_GDBUS_WRAPPER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
  SERVICE_TYPE_GDBUS_WRAPPER, ServiceGDBusWrapperClass))

#define SERVICE_IS_GDBUS_WRAPPER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
  SERVICE_TYPE_GDBUS_WRAPPER))

#define SERVICE_IS_GDBUS_WRAPPER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
  SERVICE_TYPE_GDBUS_WRAPPER))

#define ServiceGDBusWrapper_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
  SERVICE_TYPE_GDBUS_WRAPPER, ServiceGDBusWrapperClass))

typedef struct _ServiceGDBusWrapper        ServiceGDBusWrapper;
typedef struct _ServiceGDBusWrapperClass   ServiceGDBusWrapperClass;
typedef struct _ServiceGDBusWrapperPrivate ServiceGDBusWrapperPrivate;

struct _ServiceGDBusWrapper
{
  GObject parent;

  ServiceGDBusWrapperPrivate *priv;
};

struct _ServiceGDBusWrapperClass
{
  GObjectClass parent_class;
};

GType service_gdbus_wrapper_get_type(void) G_GNUC_CONST;

ServiceGDBusWrapper* service_gdbus_wrapper_new(void);

G_END_DECLS

#endif /* _SERVICE_GDBUS_WRAPPER_H_ */
