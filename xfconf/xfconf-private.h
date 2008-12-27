/*
 *  xfce-power-manager
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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __XFCONF_PRIVATE_H__
#define __XFCONF_PRIVATE_H__

#include <dbus/dbus-glib.h>

typedef struct
{
    guint n_members;
    GType *member_types;
} XfconfNamedStruct;

DBusGConnection *_xfconf_get_dbus_g_connection();
DBusGProxy *_xfconf_get_dbus_g_proxy();

XfconfNamedStruct *_xfconf_named_struct_lookup(const gchar *struct_name);

void _xfconf_channel_shutdown();

void _xfconf_g_bindings_init();
void _xfconf_g_bindings_shutdown();

#endif  /* __XFCONF_PRIVATE_H__ */
