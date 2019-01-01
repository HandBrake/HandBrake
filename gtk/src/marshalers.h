/*
 * marshalers.h
 * Copyright (C) John Stebbins 2008-2019 <stebbins@stebbins>
 *
 * marshalers.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * marshalers.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with callbacks.h.  If not, write to:
 *  The Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor
 *  Boston, MA  02110-1301, USA.
 */

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

