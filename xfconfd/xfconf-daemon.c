/*
 *  xfconfd
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <dbus/dbus-glib-lowlevel.h>

#include "xfconf-daemon.h"
#include "xfconfd-dbus-server.h"
#include "xfconf-backend-factory.h"
#include "xfconf-backend.h"

struct _XfconfDaemon
{
    GObject parent;
    
    DBusGConnection *dbus_conn;
    
    XfconfBackend *backend;
};

typedef struct _XfconfDaemonClass
{
    GObjectClass parent;
} XfconfDaemonClass;

static void xfconf_daemon_class_init(XfconfDaemonClass *klass);

static void xfconf_daemon_init(XfconfDaemon *instance);
static void xfconf_daemon_finalize(GObject *obj);


G_DEFINE_TYPE(XfconfDaemon, xfconf_daemon, G_TYPE_OBJECT)


static void
xfconf_daemon_class_init(XfconfDaemonClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    
    object_class->finalize = xfconf_daemon_finalize;
    
    dbus_g_object_type_install_info(G_TYPE_FROM_CLASS(klass),
                                    &dbus_glib_xfconfd_object_info);
}

static void
xfconf_daemon_init(XfconfDaemon *instance)
{
    
}

static void
xfconf_daemon_finalize(GObject *obj)
{
    XfconfDaemon *xfconfd = XFCONF_DAEMON(obj);
    
    if(xfconfd->backend) {
        xfconf_backend_flush(xfconfd->backend);
        g_object_unref(G_OBJECT(xfconfd->backend));
    }
    
    if(xfconfd->dbus_conn)
        dbus_g_connection_unref(xfconfd->dbus_conn);
    
    G_OBJECT_CLASS(xfconf_daemon_parent_class)->finalize(obj);
}



static gboolean
xfconf_daemon_start(XfconfDaemon *xfconfd,
                    GError **error)
{
    int ret;
    DBusError derror;
    
    xfconfd->dbus_conn = dbus_g_bus_get(DBUS_BUS_SESSION, error);
    if(G_UNLIKELY(!xfconfd->dbus_conn))
        return FALSE;
    
    dbus_g_connection_register_g_object(xfconfd->dbus_conn,
                                        "/org/xfce/Xfconf",
                                        G_OBJECT(xfconfd));
    
    dbus_error_init(&derror);
    ret = dbus_bus_request_name(dbus_g_connection_get_connection(xfconfd->dbus_conn),
                                "org.xfce.Xfconf",
                                DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                &derror);
    if(DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
        if(dbus_error_is_set(&derror)) {
            if(error)
                dbus_set_g_error(error, &derror);
            dbus_error_free(&derror);
        } else if(error) {
            g_error_set(error, DBUS_GERROR, DBUS_GERROR_FAILED,
                        _("Another Xfconf daemon is already running"));
        }
        
        return FALSE;
    }
    
    return TRUE;
}

static gboolean
xfconf_daemon_load_config(XfconfDaemon *xfconfd,
                          GError **error)
{
    xfconfd->backend = xfconf_backend_factory_get_backend(NULL, error);
    if(!xfconfd->backend)
        return FALSE;
    
    return TRUE;
}



XfconfDaemon *
xfconf_daemon_new_unique(GError **error)
{
    XfconfDaemon *xfconfd = g_object_new(XFCONF_TYPE_DAEMON, NULL);
    
    if(!xfconf_daemon_start(xfconfd, error)
       || !xfconf_daemon_load_config(xfconfd, error))
    {
        g_object_unref(G_OBJECT(xfconfd));
        return NULL;
    }
    
    return xfconfd;
}
