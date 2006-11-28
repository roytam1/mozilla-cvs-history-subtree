
#ifndef __gtkmozembed_MARSHAL_H__
#define __gtkmozembed_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* BOOL:STRING (gtkmozembedmarshal.list:1) */
extern void gtkmozembed_BOOLEAN__STRING (GClosure     *closure,
                                         GValue       *return_value,
                                         guint         n_param_values,
                                         const GValue *param_values,
                                         gpointer      invocation_hint,
                                         gpointer      marshal_data);
#define gtkmozembed_BOOL__STRING	gtkmozembed_BOOLEAN__STRING

/* BOOL:STRING,STRING (gtkmozembedmarshal.list:2) */
extern void gtkmozembed_BOOLEAN__STRING_STRING (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);
#define gtkmozembed_BOOL__STRING_STRING	gtkmozembed_BOOLEAN__STRING_STRING

/* BOOL:STRING,STRING,POINTER (gtkmozembedmarshal.list:3) */
extern void gtkmozembed_BOOLEAN__STRING_STRING_POINTER (GClosure     *closure,
                                                        GValue       *return_value,
                                                        guint         n_param_values,
                                                        const GValue *param_values,
                                                        gpointer      invocation_hint,
                                                        gpointer      marshal_data);
#define gtkmozembed_BOOL__STRING_STRING_POINTER	gtkmozembed_BOOLEAN__STRING_STRING_POINTER

/* BOOL:STRING,STRING,POINTER,INT (gtkmozembedmarshal.list:4) */
extern void gtkmozembed_BOOLEAN__STRING_STRING_POINTER_INT (GClosure     *closure,
                                                            GValue       *return_value,
                                                            guint         n_param_values,
                                                            const GValue *param_values,
                                                            gpointer      invocation_hint,
                                                            gpointer      marshal_data);
#define gtkmozembed_BOOL__STRING_STRING_POINTER_INT	gtkmozembed_BOOLEAN__STRING_STRING_POINTER_INT

/* BOOL:STRING,STRING,POINTER,POINTER,STRING,POINTER (gtkmozembedmarshal.list:5) */
extern void gtkmozembed_BOOLEAN__STRING_STRING_POINTER_POINTER_STRING_POINTER (GClosure     *closure,
                                                                               GValue       *return_value,
                                                                               guint         n_param_values,
                                                                               const GValue *param_values,
                                                                               gpointer      invocation_hint,
                                                                               gpointer      marshal_data);
#define gtkmozembed_BOOL__STRING_STRING_POINTER_POINTER_STRING_POINTER	gtkmozembed_BOOLEAN__STRING_STRING_POINTER_POINTER_STRING_POINTER

/* BOOL:STRING,STRING,POINTER,STRING,POINTER (gtkmozembedmarshal.list:6) */
extern void gtkmozembed_BOOLEAN__STRING_STRING_POINTER_STRING_POINTER (GClosure     *closure,
                                                                       GValue       *return_value,
                                                                       guint         n_param_values,
                                                                       const GValue *param_values,
                                                                       gpointer      invocation_hint,
                                                                       gpointer      marshal_data);
#define gtkmozembed_BOOL__STRING_STRING_POINTER_STRING_POINTER	gtkmozembed_BOOLEAN__STRING_STRING_POINTER_STRING_POINTER

/* BOOL:STRING,STRING,STRING,POINTER (gtkmozembedmarshal.list:7) */
extern void gtkmozembed_BOOLEAN__STRING_STRING_STRING_POINTER (GClosure     *closure,
                                                               GValue       *return_value,
                                                               guint         n_param_values,
                                                               const GValue *param_values,
                                                               gpointer      invocation_hint,
                                                               gpointer      marshal_data);
#define gtkmozembed_BOOL__STRING_STRING_STRING_POINTER	gtkmozembed_BOOLEAN__STRING_STRING_STRING_POINTER

/* INT:STRING,INT,INT,INT,INT,INT (gtkmozembedmarshal.list:8) */
extern void gtkmozembed_INT__STRING_INT_INT_INT_INT_INT (GClosure     *closure,
                                                         GValue       *return_value,
                                                         guint         n_param_values,
                                                         const GValue *param_values,
                                                         gpointer      invocation_hint,
                                                         gpointer      marshal_data);

/* INT:STRING,STRING,INT,INT,INT,INT (gtkmozembedmarshal.list:9) */
extern void gtkmozembed_INT__STRING_STRING_INT_INT_INT_INT (GClosure     *closure,
                                                            GValue       *return_value,
                                                            guint         n_param_values,
                                                            const GValue *param_values,
                                                            gpointer      invocation_hint,
                                                            gpointer      marshal_data);

/* INT:STRING,STRING,UINT,STRING,STRING,STRING,STRING,POINTER (gtkmozembedmarshal.list:10) */
extern void gtkmozembed_INT__STRING_STRING_UINT_STRING_STRING_STRING_STRING_POINTER (GClosure     *closure,
                                                                                     GValue       *return_value,
                                                                                     guint         n_param_values,
                                                                                     const GValue *param_values,
                                                                                     gpointer      invocation_hint,
                                                                                     gpointer      marshal_data);

/* INT:VOID (gtkmozembedmarshal.list:11) */
extern void gtkmozembed_INT__VOID (GClosure     *closure,
                                   GValue       *return_value,
                                   guint         n_param_values,
                                   const GValue *param_values,
                                   gpointer      invocation_hint,
                                   gpointer      marshal_data);

/* STRING:STRING,STRING (gtkmozembedmarshal.list:12) */
extern void gtkmozembed_STRING__STRING_STRING (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);

/* VOID:BOOL (gtkmozembedmarshal.list:13) */
#define gtkmozembed_VOID__BOOLEAN	g_cclosure_marshal_VOID__BOOLEAN
#define gtkmozembed_VOID__BOOL	gtkmozembed_VOID__BOOLEAN

/* VOID:INT,INT,BOOL (gtkmozembedmarshal.list:14) */
extern void gtkmozembed_VOID__INT_INT_BOOLEAN (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);
#define gtkmozembed_VOID__INT_INT_BOOL	gtkmozembed_VOID__INT_INT_BOOLEAN

/* VOID:INT,STRING (gtkmozembedmarshal.list:15) */
extern void gtkmozembed_VOID__INT_STRING (GClosure     *closure,
                                          GValue       *return_value,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint,
                                          gpointer      marshal_data);

/* VOID:INT,STRING,STRING (gtkmozembedmarshal.list:16) */
extern void gtkmozembed_VOID__INT_STRING_STRING (GClosure     *closure,
                                                 GValue       *return_value,
                                                 guint         n_param_values,
                                                 const GValue *param_values,
                                                 gpointer      invocation_hint,
                                                 gpointer      marshal_data);

/* VOID:INT,UINT (gtkmozembedmarshal.list:17) */
extern void gtkmozembed_VOID__INT_UINT (GClosure     *closure,
                                        GValue       *return_value,
                                        guint         n_param_values,
                                        const GValue *param_values,
                                        gpointer      invocation_hint,
                                        gpointer      marshal_data);

/* VOID:POINTER,INT,POINTER (gtkmozembedmarshal.list:18) */
extern void gtkmozembed_VOID__POINTER_INT_POINTER (GClosure     *closure,
                                                   GValue       *return_value,
                                                   guint         n_param_values,
                                                   const GValue *param_values,
                                                   gpointer      invocation_hint,
                                                   gpointer      marshal_data);

/* VOID:POINTER,INT,STRING,STRING,STRING,STRING,STRING,BOOLEAN,INT (gtkmozembedmarshal.list:19) */
extern void gtkmozembed_VOID__POINTER_INT_STRING_STRING_STRING_STRING_STRING_BOOLEAN_INT (GClosure     *closure,
                                                                                          GValue       *return_value,
                                                                                          guint         n_param_values,
                                                                                          const GValue *param_values,
                                                                                          gpointer      invocation_hint,
                                                                                          gpointer      marshal_data);

/* VOID:POINTER,STRING,BOOL,BOOL (gtkmozembedmarshal.list:20) */
extern void gtkmozembed_VOID__POINTER_STRING_BOOLEAN_BOOLEAN (GClosure     *closure,
                                                              GValue       *return_value,
                                                              guint         n_param_values,
                                                              const GValue *param_values,
                                                              gpointer      invocation_hint,
                                                              gpointer      marshal_data);
#define gtkmozembed_VOID__POINTER_STRING_BOOL_BOOL	gtkmozembed_VOID__POINTER_STRING_BOOLEAN_BOOLEAN

/* VOID:STRING,INT,INT (gtkmozembedmarshal.list:21) */
extern void gtkmozembed_VOID__STRING_INT_INT (GClosure     *closure,
                                              GValue       *return_value,
                                              guint         n_param_values,
                                              const GValue *param_values,
                                              gpointer      invocation_hint,
                                              gpointer      marshal_data);

/* VOID:STRING,INT,UINT (gtkmozembedmarshal.list:22) */
extern void gtkmozembed_VOID__STRING_INT_UINT (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);

/* VOID:STRING,STRING (gtkmozembedmarshal.list:23) */
extern void gtkmozembed_VOID__STRING_STRING (GClosure     *closure,
                                             GValue       *return_value,
                                             guint         n_param_values,
                                             const GValue *param_values,
                                             gpointer      invocation_hint,
                                             gpointer      marshal_data);

/* VOID:STRING,STRING,POINTER (gtkmozembedmarshal.list:24) */
extern void gtkmozembed_VOID__STRING_STRING_POINTER (GClosure     *closure,
                                                     GValue       *return_value,
                                                     guint         n_param_values,
                                                     const GValue *param_values,
                                                     gpointer      invocation_hint,
                                                     gpointer      marshal_data);

/* VOID:STRING,STRING,STRING,ULONG,INT (gtkmozembedmarshal.list:25) */
extern void gtkmozembed_VOID__STRING_STRING_STRING_ULONG_INT (GClosure     *closure,
                                                              GValue       *return_value,
                                                              guint         n_param_values,
                                                              const GValue *param_values,
                                                              gpointer      invocation_hint,
                                                              gpointer      marshal_data);

/* VOID:STRING,STRING,STRING,POINTER (gtkmozembedmarshal.list:26) */
extern void gtkmozembed_VOID__STRING_STRING_STRING_POINTER (GClosure     *closure,
                                                            GValue       *return_value,
                                                            guint         n_param_values,
                                                            const GValue *param_values,
                                                            gpointer      invocation_hint,
                                                            gpointer      marshal_data);

/* VOID:UINT,INT,INT,STRING,STRING,STRING,STRING (gtkmozembedmarshal.list:27) */
extern void gtkmozembed_VOID__UINT_INT_INT_STRING_STRING_STRING_STRING (GClosure     *closure,
                                                                        GValue       *return_value,
                                                                        guint         n_param_values,
                                                                        const GValue *param_values,
                                                                        gpointer      invocation_hint,
                                                                        gpointer      marshal_data);

/* VOID:ULONG,ULONG,ULONG (gtkmozembedmarshal.list:28) */
extern void gtkmozembed_VOID__ULONG_ULONG_ULONG (GClosure     *closure,
                                                 GValue       *return_value,
                                                 guint         n_param_values,
                                                 const GValue *param_values,
                                                 gpointer      invocation_hint,
                                                 gpointer      marshal_data);

/* BOOL:POINTER,UINT (gtkmozembedmarshal.list:29) */
extern void gtkmozembed_BOOLEAN__POINTER_UINT (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);
#define gtkmozembed_BOOL__POINTER_UINT	gtkmozembed_BOOLEAN__POINTER_UINT

/* VOID:POINTER (gtkmozembedmarshal.list:30) */
#define gtkmozembed_VOID__POINTER	g_cclosure_marshal_VOID__POINTER

/* BOOL:STRING,STRING,POINTER (gtkmozembedmarshal.list:31) */

G_END_DECLS

#endif /* __gtkmozembed_MARSHAL_H__ */

