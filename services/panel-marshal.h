
#ifndef __panel_service_MARSHAL_H__
#define __panel_service_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:STRING,BOOLEAN (panel-marshal.list:1) */
extern void panel_service_VOID__STRING_BOOLEAN (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);

G_END_DECLS

#endif /* __panel_service_MARSHAL_H__ */

