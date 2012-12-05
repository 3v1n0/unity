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

  LAST_SIGNAL
};


static guint32 _service_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(TestSignals, test_signals, G_TYPE_OBJECT);

static void
test_signals_class_init(TestSignalsClass* klass)
{
  GObjectClass* obj_class = G_OBJECT_CLASS(klass);

  /* Signals */
  _service_signals[SIGNAL_0] =
    g_signal_new("signal0",
                 G_OBJECT_CLASS_TYPE(obj_class),
                 G_SIGNAL_RUN_LAST,
                 0,
                 NULL, NULL, NULL,
                 G_TYPE_NONE, 0);

  _service_signals[SIGNAL_1] =
    g_signal_new("signal1",
                 G_OBJECT_CLASS_TYPE(obj_class),
                 G_SIGNAL_RUN_LAST,
                 0,
                 NULL, NULL, NULL,
                 G_TYPE_NONE, 1, G_TYPE_STRING);

  _service_signals[SIGNAL_2] =
    g_signal_new("signal2",
                 G_OBJECT_CLASS_TYPE(obj_class),
                 G_SIGNAL_RUN_LAST,
                 0,
                 NULL, NULL, NULL,
                 G_TYPE_NONE, 2,
                 G_TYPE_STRING, G_TYPE_INT);

  _service_signals[SIGNAL_3] =
    g_signal_new("signal3",
                 G_OBJECT_CLASS_TYPE(obj_class),
                 G_SIGNAL_RUN_LAST,
                 0,
                 NULL, NULL, NULL,
                 G_TYPE_NONE, 3,
                 G_TYPE_STRING, G_TYPE_INT, G_TYPE_FLOAT);

  _service_signals[SIGNAL_4] =
    g_signal_new("signal4",
                 G_OBJECT_CLASS_TYPE(obj_class),
                 G_SIGNAL_RUN_LAST,
                 0,
                 NULL, NULL, NULL,
                 G_TYPE_NONE, 4,
                 G_TYPE_STRING, G_TYPE_INT, G_TYPE_FLOAT, G_TYPE_DOUBLE);

  _service_signals[SIGNAL_5] =
    g_signal_new("signal5",
                 G_OBJECT_CLASS_TYPE(obj_class),
                 G_SIGNAL_RUN_LAST,
                 0,
                 NULL, NULL, NULL,
                 G_TYPE_NONE, 5,
                 G_TYPE_STRING, G_TYPE_INT, G_TYPE_FLOAT,
                 G_TYPE_DOUBLE, G_TYPE_BOOLEAN);

  _service_signals[SIGNAL_6] =
    g_signal_new("signal6",
                 G_OBJECT_CLASS_TYPE(obj_class),
                 G_SIGNAL_RUN_LAST,
                 0,
                 NULL, NULL, NULL,
                 G_TYPE_BOOLEAN, 6,
                 G_TYPE_STRING, G_TYPE_INT, G_TYPE_FLOAT,
                 G_TYPE_DOUBLE, G_TYPE_BOOLEAN, G_TYPE_CHAR);
}

static void
test_signals_init(TestSignals* self)
{
}
