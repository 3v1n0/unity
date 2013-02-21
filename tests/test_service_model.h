#ifndef _SERVICE_MODEL_H_
#define _SERVICE_MODEL_H_

#include <dee.h>

G_BEGIN_DECLS

#define SERVICE_TYPE_MODEL (service_model_get_type ())

#define SERVICE_MODEL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
  SERVICE_TYPE_MODEL, ServiceModel))

#define SERVICE_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
  SERVICE_TYPE_MODEL, ServiceModelClass))

#define SERVICE_IS_MODEL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
  SERVICE_TYPE_MODEL))

#define SERVICE_IS_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
  SERVICE_TYPE_MODEL))

#define ServiceModel_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
  SERVICE_TYPE_MODEL, ServiceModelClass))

typedef struct _ServiceModel         ServiceModel;
typedef struct _ServiceModelClass    ServiceModelClass;
typedef struct _ServiceModelPrivate  ServiceModelPrivate;


struct _ServiceModel
{
  GObject parent;

  ServiceModelPrivate *priv;
};

struct _ServiceModelClass
{
  GObjectClass parent_class;
};

GType service_model_get_type(void) G_GNUC_CONST;

ServiceModel* service_model_new(void);

G_END_DECLS

#endif /* _SERVICE_MODEL_H_ */
