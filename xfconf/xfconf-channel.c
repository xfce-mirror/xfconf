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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "xfconf-channel.h"

struct _XfconfChannel
{
    GObject parent;
    
    /* insert private instance variables here */
};

typedef struct _XfconfChannelClass
{
    GObjectClass parent;
    
    /* insert any signal members or virtual functions here */
} XfconfChannelClass;

static void xfconf_channel_class_init(XfconfChannelClass *klass);

static void xfconf_channel_init(XfconfChannel *instance);
static void xfconf_channel_finalize(GObject *obj);


G_DEFINE_TYPE(XfconfChannel, xfconf_channel, G_TYPE_OBJECT)


static void
xfconf_channel_class_init(XfconfChannelClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    
    object_class->finalize = xfconf_channel_finalize;
}

static void
xfconf_channel_init(XfconfChannel *instance)
{
    
}

static void
xfconf_channel_finalize(GObject *obj)
{
    G_OBJECT_CLASS(xfconf_channel_parent_class)->finalize(obj);
}
