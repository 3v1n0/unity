#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "panel-service.h"

G_DEFINE_TYPE (PanelService, panel_service, G_TYPE_OBJECT);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PANEL_TYPE_SERVICE, PanelServicePrivate))

struct _PanelServicePrivate
{
  gint margin;
};

/* Globals */
enum
{
  ENTRY_ACTIVATED = 0,

  LAST_SIGNAL
};

static guint32 _service_signals[LAST_SIGNAL] = { 0 };

/* Forwards */

/*
 * GObject stuff
 */
static void
panel_service_class_init (PanelServiceClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  /* Signals */
  _service_signals[ENTRY_ACTIVATED] =
    g_signal_new ("entry-activated",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);
  g_type_class_add_private (obj_class, sizeof (PanelServicePrivate));
}


static void
panel_service_init (PanelService *self)
{
  PanelServicePrivate *priv;
  priv = GET_PRIVATE (self);
}

PanelService *
panel_service_new ()
{
  return g_object_new (PANEL_TYPE_SERVICE, NULL);
}
