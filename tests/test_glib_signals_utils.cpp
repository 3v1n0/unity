#include "test_glib_signals_utils.h"

enum
{
  SIGNAL_0,
  SIGNAL_1,
  SIGNAL_2,
  SIGNAL_3,
  SIGNAL_4,
  SIGNAL_5,
  SIGNAL_6,
  SIGNAL_7,

  LAST_SIGNAL
};


static guint32 _service_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (TestSignals, test_signals, G_TYPE_OBJECT);

static void
test_signals_class_init (TestSignalsClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  /* Signals */
  _service_signals[SIGNAL_1] =
    g_signal_new ("signal-1",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
test_signals_init (TestSignals *self)
{
}
