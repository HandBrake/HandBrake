
#ifndef __ghb_marshal_MARSHAL_H__
#define __ghb_marshal_MARSHAL_H__

#include    <glib-object.h>

G_BEGIN_DECLS

/* VOID:STRING (/home/jstebbins/Source/hb/HandBrake/build.dbg/../gtk/src/marshalers.list:1) */
#define ghb_marshal_VOID__STRING    g_cclosure_marshal_VOID__STRING

/* VOID:STRING,STRING (/home/jstebbins/Source/hb/HandBrake/build.dbg/../gtk/src/marshalers.list:2) */
extern void ghb_marshal_VOID__STRING_STRING (GClosure     *closure,
                                             GValue       *return_value,
                                             guint         n_param_values,
                                             const GValue *param_values,
                                             gpointer      invocation_hint,
                                             gpointer      marshal_data);

/* BOOLEAN:BOXED (/home/jstebbins/Source/hb/HandBrake/build.dbg/../gtk/src/marshalers.list:3) */
extern void ghb_marshal_BOOLEAN__BOXED (GClosure     *closure,
                                        GValue       *return_value,
                                        guint         n_param_values,
                                        const GValue *param_values,
                                        gpointer      invocation_hint,
                                        gpointer      marshal_data);

G_END_DECLS

#endif /* __ghb_marshal_MARSHAL_H__ */

