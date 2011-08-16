#ifndef _SERVICE_LENS_H_
#define _SERVICE_LENS_H_

#include <dee.h>

G_BEGIN_DECLS

#define SERVICE_TYPE_LENS (service_lens_get_type ())

#define SERVICE_LENS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
  SERVICE_TYPE_LENS, ServiceLens))

#define SERVICE_LENS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
  SERVICE_TYPE_LENS, ServiceLensClass))

#define SERVICE_IS_LENS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
  SERVICE_TYPE_LENS))

#define SERVICE_IS_LENS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
  SERVICE_TYPE_LENS))

#define ServiceLens_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
  SERVICE_TYPE_LENS, ServiceLensClass))

typedef struct _ServiceLens        ServiceLens;
typedef struct _ServiceLensClass   ServiceLensClass;
typedef struct _ServiceLensPrivate ServiceLensPrivate;

struct _ServiceLens
{
  GObject parent;

  ServiceLensPrivate *priv;
};

struct _ServiceLensClass
{
  GObjectClass parent_class;
};

GType service_lens_get_type(void) G_GNUC_CONST;

ServiceLens* service_lens_new(void);

G_END_DECLS

#endif /* _SERVICE_LENS_H_ */
