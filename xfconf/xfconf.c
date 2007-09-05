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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <dbus/dbus-glib.h>

#include "xfconf.h"


static guint xfconf_refcnt = 0;
static DBusGConnection *dbus_conn = NULL;
static DBusGProxy *dbus_proxy = NULL;
static DBusGProxy *gui_dbus_proxy = NULL;


DBusGConnection *
_xfconf_get_dbus_g_connection()
{
    if(!xfconf_refcnt) {
        g_critical("xfconf_init() must be called before attempting to use libxfconf!");
        return NULL;
    }
    
    return dbus_conn;
}

DBusGProxy *
_xfconf_get_dbus_g_proxy()
{
    if(!xfconf_refcnt) {
        g_critical("xfconf_init() must be called before attempting to use libxfconf!");
        return NULL;
    }
    
    return dbus_proxy;
}

DBusGProxy *
_xfconf_get_gui_dbus_g_proxy()
{
    if(!xfconf_refcnt) {
        g_critical("xfconf_init() must be called before attempting to use libxfconf!");
        return NULL;
    }
    
    return gui_dbus_proxy;
}

/**
 * xfconf_init:
 * @error: An error return.
 *
 * Initializes the Xfconf library.  Can be called multiple times with no
 * adverse effects.
 *
 * Returns: %TRUE if the library was initialized succesfully, %FALSE on
 *          error.  If there is an error @error will be set.
 **/
gboolean
xfconf_init(GError **error)
{
    if(xfconf_refcnt) {
        ++xfconf_refcnt;
        return TRUE;
    }
    
    dbus_conn = dbus_g_bus_get(DBUS_BUS_SESSION, error);
    if(!dbus_conn)
        return FALSE;
    
    dbus_proxy = dbus_g_proxy_new_for_name(dbus_conn,
                                           "org.xfce.Xfconf",
                                           "/org/xfce/Xfconf",
                                           "org.xfce.Xfconf");
    
    gui_dbus_proxy = dbus_g_proxy_new_for_name(dbus_conn,
                                               "org.xfce.Xfconf",
                                               "/org/xfce/Xfconf",
                                               "org.xfce.Xfconf.GUI");
    
    ++xfconf_refcnt;
    return TRUE;
}

/**
 * xfconf_shutdown:
 *
 * Shuts down and frees any resources consumed by the Xfconf library.
 * If xfconf_init() is called multiple times, xfconf_shutdown() must be
 * called an equal number of times to shut down the library.
 **/
void
xfconf_shutdown()
{
    if(--xfconf_refcnt)
        return;
    
    g_object_unref(G_OBJECT(dbus_proxy));
    dbus_proxy = NULL;
    
    g_object_unref(G_OBJECT(gui_dbus_proxy));
    gui_dbus_proxy = NULL;
    
    dbus_g_connection_unref(dbus_conn);
    dbus_conn = NULL;
}
