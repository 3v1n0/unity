/*
 * GObject Class to allow for extensive testing of our Signal wrapper
 */

#ifndef _TestSignals_H_
#define _TestSignals_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define TEST_TYPE_SIGNALS (test_signals_get_type ())

#define TestSignals(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
	TEST_TYPE_SIGNALS, TestSignals))

#define TestSignals_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
	TEST_TYPE_SIGNALS, TestSignalsClass))

#define TEST_IS_SIGNALS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
	TEST_TYPE_SIGNALS))

#define TEST_IS_SIGNALS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
	TEST_TYPE_SIGNALS))

#define TestSignals_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
	TEST_TYPE_SIGNALS, TestSignalsClass))

typedef struct _TestSignals        TestSignals;
typedef struct _TestSignalsClass   TestSignalsClass;

struct _TestSignals
{
  GObject parent;
};

struct _TestSignalsClass
{
  GObjectClass parent_class;
};

GType test_signals_get_type(void) G_GNUC_CONST;

G_END_DECLS

#endif /* _TestSignals_H_ */
