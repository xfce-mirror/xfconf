/*
 *  AppName
 *
 *  Copyright (c) YEAR Your Name <your@email>
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

/* insert member function declarations here */

G_END_DECLS

#endif  /* __XFCONF_CHANNEL_H__ */
