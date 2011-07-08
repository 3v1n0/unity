
#include	<glib-object.h>

#include "test_glib_signals_utils_marshal.h"

#ifdef G_ENABLE_DEBUG
#define g_marshal_value_peek_boolean(v)  g_value_get_boolean (v)
#define g_marshal_value_peek_char(v)     g_value_get_char (v)
#define g_marshal_value_peek_uchar(v)    g_value_get_uchar (v)
#define g_marshal_value_peek_int(v)      g_value_get_int (v)
#define g_marshal_value_peek_uint(v)     g_value_get_uint (v)
#define g_marshal_value_peek_long(v)     g_value_get_long (v)
#define g_marshal_value_peek_ulong(v)    g_value_get_ulong (v)
#define g_marshal_value_peek_int64(v)    g_value_get_int64 (v)
#define g_marshal_value_peek_uint64(v)   g_value_get_uint64 (v)
#define g_marshal_value_peek_enum(v)     g_value_get_enum (v)
#define g_marshal_value_peek_flags(v)    g_value_get_flags (v)
#define g_marshal_value_peek_float(v)    g_value_get_float (v)
#define g_marshal_value_peek_double(v)   g_value_get_double (v)
#define g_marshal_value_peek_string(v)   (char*) g_value_get_string (v)
#define g_marshal_value_peek_param(v)    g_value_get_param (v)
#define g_marshal_value_peek_boxed(v)    g_value_get_boxed (v)
#define g_marshal_value_peek_pointer(v)  g_value_get_pointer (v)
#define g_marshal_value_peek_object(v)   g_value_get_object (v)
#define g_marshal_value_peek_variant(v)  g_value_get_variant (v)
#else /* !G_ENABLE_DEBUG */
/* WARNING: This code accesses GValues directly, which is UNSUPPORTED API.
 *          Do not access GValues directly in your code. Instead, use the
 *          g_value_get_*() functions
 */
#define g_marshal_value_peek_boolean(v)  (v)->data[0].v_int
#define g_marshal_value_peek_char(v)     (v)->data[0].v_int
#define g_marshal_value_peek_uchar(v)    (v)->data[0].v_uint
#define g_marshal_value_peek_int(v)      (v)->data[0].v_int
#define g_marshal_value_peek_uint(v)     (v)->data[0].v_uint
#define g_marshal_value_peek_long(v)     (v)->data[0].v_long
#define g_marshal_value_peek_ulong(v)    (v)->data[0].v_ulong
#define g_marshal_value_peek_int64(v)    (v)->data[0].v_int64
#define g_marshal_value_peek_uint64(v)   (v)->data[0].v_uint64
#define g_marshal_value_peek_enum(v)     (v)->data[0].v_long
#define g_marshal_value_peek_flags(v)    (v)->data[0].v_ulong
#define g_marshal_value_peek_float(v)    (v)->data[0].v_float
#define g_marshal_value_peek_double(v)   (v)->data[0].v_double
#define g_marshal_value_peek_string(v)   (v)->data[0].v_pointer
#define g_marshal_value_peek_param(v)    (v)->data[0].v_pointer
#define g_marshal_value_peek_boxed(v)    (v)->data[0].v_pointer
#define g_marshal_value_peek_pointer(v)  (v)->data[0].v_pointer
#define g_marshal_value_peek_object(v)   (v)->data[0].v_pointer
#define g_marshal_value_peek_variant(v)  (v)->data[0].v_pointer
#endif /* !G_ENABLE_DEBUG */


/* VOID:STRING,INT (./test_glib_signals_utils_marshal.list:1) */
void
test_signals_VOID__STRING_INT (GClosure     *closure,
                               GValue       *return_value G_GNUC_UNUSED,
                               guint         n_param_values,
                               const GValue *param_values,
                               gpointer      invocation_hint G_GNUC_UNUSED,
                               gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING_INT) (gpointer     data1,
                                                 gpointer     arg_1,
                                                 gint         arg_2,
                                                 gpointer     data2);
  register GMarshalFunc_VOID__STRING_INT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING_INT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            g_marshal_value_peek_int (param_values + 2),
            data2);
}

/* VOID:STRING,INT,FLOAT (./test_glib_signals_utils_marshal.list:2) */
void
test_signals_VOID__STRING_INT_FLOAT (GClosure     *closure,
                                     GValue       *return_value G_GNUC_UNUSED,
                                     guint         n_param_values,
                                     const GValue *param_values,
                                     gpointer      invocation_hint G_GNUC_UNUSED,
                                     gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING_INT_FLOAT) (gpointer     data1,
                                                       gpointer     arg_1,
                                                       gint         arg_2,
                                                       gfloat       arg_3,
                                                       gpointer     data2);
  register GMarshalFunc_VOID__STRING_INT_FLOAT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING_INT_FLOAT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            g_marshal_value_peek_int (param_values + 2),
            g_marshal_value_peek_float (param_values + 3),
            data2);
}

/* VOID:STRING,INT,FLOAT,DOUBLE (./test_glib_signals_utils_marshal.list:3) */
void
test_signals_VOID__STRING_INT_FLOAT_DOUBLE (GClosure     *closure,
                                            GValue       *return_value G_GNUC_UNUSED,
                                            guint         n_param_values,
                                            const GValue *param_values,
                                            gpointer      invocation_hint G_GNUC_UNUSED,
                                            gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING_INT_FLOAT_DOUBLE) (gpointer     data1,
                                                              gpointer     arg_1,
                                                              gint         arg_2,
                                                              gfloat       arg_3,
                                                              gdouble      arg_4,
                                                              gpointer     data2);
  register GMarshalFunc_VOID__STRING_INT_FLOAT_DOUBLE callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 5);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING_INT_FLOAT_DOUBLE) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            g_marshal_value_peek_int (param_values + 2),
            g_marshal_value_peek_float (param_values + 3),
            g_marshal_value_peek_double (param_values + 4),
            data2);
}

/* VOID:STRING,INT,FLOAT,DOUBLE,BOOL (./test_glib_signals_utils_marshal.list:4) */
void
test_signals_VOID__STRING_INT_FLOAT_DOUBLE_BOOLEAN (GClosure     *closure,
                                                    GValue       *return_value G_GNUC_UNUSED,
                                                    guint         n_param_values,
                                                    const GValue *param_values,
                                                    gpointer      invocation_hint G_GNUC_UNUSED,
                                                    gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING_INT_FLOAT_DOUBLE_BOOLEAN) (gpointer     data1,
                                                                      gpointer     arg_1,
                                                                      gint         arg_2,
                                                                      gfloat       arg_3,
                                                                      gdouble      arg_4,
                                                                      gboolean     arg_5,
                                                                      gpointer     data2);
  register GMarshalFunc_VOID__STRING_INT_FLOAT_DOUBLE_BOOLEAN callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 6);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING_INT_FLOAT_DOUBLE_BOOLEAN) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            g_marshal_value_peek_int (param_values + 2),
            g_marshal_value_peek_float (param_values + 3),
            g_marshal_value_peek_double (param_values + 4),
            g_marshal_value_peek_boolean (param_values + 5),
            data2);
}

/* BOOL:STRING,INT,FLOAT,DOUBLE,BOOL,CHAR (./test_glib_signals_utils_marshal.list:5) */
void
test_signals_BOOLEAN__STRING_INT_FLOAT_DOUBLE_BOOLEAN_CHAR (GClosure     *closure,
                                                            GValue       *return_value G_GNUC_UNUSED,
                                                            guint         n_param_values,
                                                            const GValue *param_values,
                                                            gpointer      invocation_hint G_GNUC_UNUSED,
                                                            gpointer      marshal_data)
{
  typedef gboolean (*GMarshalFunc_BOOLEAN__STRING_INT_FLOAT_DOUBLE_BOOLEAN_CHAR) (gpointer     data1,
                                                                                  gpointer     arg_1,
                                                                                  gint         arg_2,
                                                                                  gfloat       arg_3,
                                                                                  gdouble      arg_4,
                                                                                  gboolean     arg_5,
                                                                                  gchar        arg_6,
                                                                                  gpointer     data2);
  register GMarshalFunc_BOOLEAN__STRING_INT_FLOAT_DOUBLE_BOOLEAN_CHAR callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gboolean v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 7);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__STRING_INT_FLOAT_DOUBLE_BOOLEAN_CHAR) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_string (param_values + 1),
                       g_marshal_value_peek_int (param_values + 2),
                       g_marshal_value_peek_float (param_values + 3),
                       g_marshal_value_peek_double (param_values + 4),
                       g_marshal_value_peek_boolean (param_values + 5),
                       g_marshal_value_peek_char (param_values + 6),
                       data2);

  g_value_set_boolean (return_value, v_return);
}

/* BOOL:STRING,INT,FLOAT,DOUBLE,BOOL,CHAR,UINT (./test_glib_signals_utils_marshal.list:6) */
void
test_signals_BOOLEAN__STRING_INT_FLOAT_DOUBLE_BOOLEAN_CHAR_UINT (GClosure     *closure,
                                                                 GValue       *return_value G_GNUC_UNUSED,
                                                                 guint         n_param_values,
                                                                 const GValue *param_values,
                                                                 gpointer      invocation_hint G_GNUC_UNUSED,
                                                                 gpointer      marshal_data)
{
  typedef gboolean (*GMarshalFunc_BOOLEAN__STRING_INT_FLOAT_DOUBLE_BOOLEAN_CHAR_UINT) (gpointer     data1,
                                                                                       gpointer     arg_1,
                                                                                       gint         arg_2,
                                                                                       gfloat       arg_3,
                                                                                       gdouble      arg_4,
                                                                                       gboolean     arg_5,
                                                                                       gchar        arg_6,
                                                                                       guint        arg_7,
                                                                                       gpointer     data2);
  register GMarshalFunc_BOOLEAN__STRING_INT_FLOAT_DOUBLE_BOOLEAN_CHAR_UINT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gboolean v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 8);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__STRING_INT_FLOAT_DOUBLE_BOOLEAN_CHAR_UINT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_string (param_values + 1),
                       g_marshal_value_peek_int (param_values + 2),
                       g_marshal_value_peek_float (param_values + 3),
                       g_marshal_value_peek_double (param_values + 4),
                       g_marshal_value_peek_boolean (param_values + 5),
                       g_marshal_value_peek_char (param_values + 6),
                       g_marshal_value_peek_uint (param_values + 7),
                       data2);

  g_value_set_boolean (return_value, v_return);
}

