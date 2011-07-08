
#ifndef __test_signals_MARSHAL_H__
#define __test_signals_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:STRING,INT (./test_glib_signals_utils_marshal.list:1) */
 void test_signals_VOID__STRING_INT (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);

/* VOID:STRING,INT,FLOAT (./test_glib_signals_utils_marshal.list:2) */
 void test_signals_VOID__STRING_INT_FLOAT (GClosure     *closure,
                                                 GValue       *return_value,
                                                 guint         n_param_values,
                                                 const GValue *param_values,
                                                 gpointer      invocation_hint,
                                                 gpointer      marshal_data);

/* VOID:STRING,INT,FLOAT,DOUBLE (./test_glib_signals_utils_marshal.list:3) */
 void test_signals_VOID__STRING_INT_FLOAT_DOUBLE (GClosure     *closure,
                                                        GValue       *return_value,
                                                        guint         n_param_values,
                                                        const GValue *param_values,
                                                        gpointer      invocation_hint,
                                                        gpointer      marshal_data);

/* VOID:STRING,INT,FLOAT,DOUBLE,BOOL (./test_glib_signals_utils_marshal.list:4) */
 void test_signals_VOID__STRING_INT_FLOAT_DOUBLE_BOOLEAN (GClosure     *closure,
                                                                GValue       *return_value,
                                                                guint         n_param_values,
                                                                const GValue *param_values,
                                                                gpointer      invocation_hint,
                                                                gpointer      marshal_data);
#define test_signals_VOID__STRING_INT_FLOAT_DOUBLE_BOOL	test_signals_VOID__STRING_INT_FLOAT_DOUBLE_BOOLEAN

/* BOOL:STRING,INT,FLOAT,DOUBLE,BOOL,CHAR (./test_glib_signals_utils_marshal.list:5) */
 void test_signals_BOOLEAN__STRING_INT_FLOAT_DOUBLE_BOOLEAN_CHAR (GClosure     *closure,
                                                                        GValue       *return_value,
                                                                        guint         n_param_values,
                                                                        const GValue *param_values,
                                                                        gpointer      invocation_hint,
                                                                        gpointer      marshal_data);
#define test_signals_BOOL__STRING_INT_FLOAT_DOUBLE_BOOL_CHAR	test_signals_BOOLEAN__STRING_INT_FLOAT_DOUBLE_BOOLEAN_CHAR

/* BOOL:STRING,INT,FLOAT,DOUBLE,BOOL,CHAR,UINT (./test_glib_signals_utils_marshal.list:6) */
 void test_signals_BOOLEAN__STRING_INT_FLOAT_DOUBLE_BOOLEAN_CHAR_UINT (GClosure     *closure,
                                                                             GValue       *return_value,
                                                                             guint         n_param_values,
                                                                             const GValue *param_values,
                                                                             gpointer      invocation_hint,
                                                                             gpointer      marshal_data);
#define test_signals_BOOL__STRING_INT_FLOAT_DOUBLE_BOOL_CHAR_UINT	test_signals_BOOLEAN__STRING_INT_FLOAT_DOUBLE_BOOLEAN_CHAR_UINT

G_END_DECLS

#endif /* __test_signals_MARSHAL_H__ */

