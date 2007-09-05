/*
 *  xfconf
 *
 *  Copyright (c) 2007 Brian Tarricone <bjt23@cornell.edu>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License ONLY.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __XFCONF_CHANNEL_H__
#define __XFCONF_CHANNEL_H__

#include <glib-object.h>

#define XFCONF_TYPE_CHANNEL             (xfconf_channel_get_type())
#define XFCONF_CHANNEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), XFCONF_TYPE_CHANNEL, XfconfChannel))
#define XFCONF_IS_CHANNEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), XFCONF_TYPE_CHANNEL))
#define XFCONF_CHANNEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), XFCONF_TYPE_CHANNEL, XfconfChannelClass))
#define XFCONF_IS_CHANNEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), XFCONF_TYPE_CHANNEL))
#define XFCONF_CHANNEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), XFCONF_TYPE_CHANNEL, XfconfChannelClass))

G_BEGIN_DECLS

typedef struct _XfconfChannel         XfconfChannel;

GType xfconf_channel_get_type() G_GNUC_CONST;

XfconfChannel *xfconf_channel_new(const gchar *channel_name);

gboolean xfconf_channel_has_property(XfconfChannel *channel,
                                     const gchar *property);

void xfconf_channel_remove_property(XfconfChannel *channel,
                                    const gchar *property);

GHashTable *xfconf_channel_get_all(XfconfChannel *channel);

gchar *xfconf_channel_get_string(XfconfChannel *channel,
                                 const gchar *property,
                                 const gchar *default_value);
gchar **xfconf_channel_get_string_list(XfconfChannel *channel,
                                       const gchar *property,
                                       const gchar **default_value);
gint xfconf_channel_get_int(XfconfChannel *channel,
                            const gchar *property,
                            gint default_value);
gint64 xfconf_channel_get_int64(XfconfChannel *channel,
                                const gchar *property,
                                gint64 default_value);
gdouble xfconf_channel_get_double(XfconfChannel *channel,
                                  const gchar *property,
                                  gdouble default_value);
gboolean xfconf_channel_get_bool(XfconfChannel *channel,
                                 const gchar *property,
                                 gboolean default_value);

gboolean xfconf_channel_set_string(XfconfChannel *channel,
                                   const gchar *property,
                                   const gchar *value);
gboolean xfconf_channel_set_string_list(XfconfChannel *channel,
                                        const gchar *property,
                                        const gchar **value);
gboolean xfconf_channel_set_int(XfconfChannel *channel,
                                const gchar *property,
                                gint value);
gboolean xfconf_channel_set_int64(XfconfChannel *channel,
                                  const gchar *property,
                                  gint64 value);
gboolean xfconf_channel_set_double(XfconfChannel *channel,
                                   const gchar *property,
                                   gdouble value);
gboolean xfconf_channel_set_bool(XfconfChannel *channel,
                                 const gchar *property,
                                 gboolean value);

#if 0  /* future */

gboolean xfconf_channel_begin_transaction(XfconfChannel *channel);
gboolean xfconf_channel_commit_transaction(XfconfChannel *channel);

#endif

G_END_DECLS

#endif  /* __XFCONF_CHANNEL_H__ */
