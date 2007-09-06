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
#include <libxfce4util/libxfce4util.h>

#include "xfconf-daemon.h"
#include "xfconf-backend-factory.h"
#include "xfconf-backend.h"
#include "xfconf-marshal.h"
#include "xfconf-util.h"

static gboolean xfconf_set_property(XfconfDaemon *xfconfd,
                                    const gchar *channel,
                                    const gchar *property,
                                    const GValue *value,
                                    GError **error);
static gboolean xfconf_get_property(XfconfDaemon *xfconfd,
                                    const gchar *channel,
                                    const gchar *property,
                                    GValue *value,
                                    GError **error);
static gboolean xfconf_get_all_properties(XfconfDaemon *xfconfd,
                                          const gchar *channel,
                                          GHashTable **properties,
                                          GError **error);
static gboolean xfconf_property_exists(XfconfDaemon *xfconfd,
                                       const gchar *channel,
                                       const gchar *property,
                                       gboolean *exists,
                                       GError **error);
static gboolean xfconf_remove_property(XfconfDaemon *xfconfd,
                                       const gchar *channel,
                                       const gchar *property,
                                       GError **error);
static gboolean xfconf_remove_channel(XfconfDaemon *xfconfd,
                                      const gchar *channel,
                                      GError **error);
static gboolean xfconf_gui_show_list(XfconfDaemon *xfconfd,
                                     const gchar *display,
                                     GError **error);
static gboolean xfconf_gui_show_plugin(XfconfDaemon *xfconfd,
                                       const gchar *display,
                                       const gchar *name,
                                       GError **error);

#include "xfconf-dbus-server.h"

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
    
    g_signal_new("property-changed", XFCONF_TYPE_DAEMON, G_SIGNAL_RUN_LAST, 0,
                 NULL, NULL, xfconf_marshal_VOID__STRING_STRING, G_TYPE_NONE,
                 2, G_TYPE_STRING, G_TYPE_STRING);
    
    dbus_g_object_type_install_info(G_TYPE_FROM_CLASS(klass),
                                    &dbus_glib_xfconf_object_info);
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
        xfconf_backend_flush(xfconfd->backend, NULL);
        g_object_unref(G_OBJECT(xfconfd->backend));
    }
    
    if(xfconfd->dbus_conn)
        dbus_g_connection_unref(xfconfd->dbus_conn);
    
    G_OBJECT_CLASS(xfconf_daemon_parent_class)->finalize(obj);
}



static gboolean
xfconf_set_property(XfconfDaemon *xfconfd,
                    const gchar *channel,
                    const gchar *property,
                    const GValue *value,
                    GError **error)
{
    return xfconf_backend_set(xfconfd->backend, channel, property, value, error);
}

static gboolean
xfconf_get_property(XfconfDaemon *xfconfd,
                    const gchar *channel,
                    const gchar *property,
                    GValue *value,
                    GError **error)
{
    /* FIXME: presumably, |value| leaks.  how do we fix this?  perhaps
     * using the org.freedesktop.DBus.GLib.Async annotation? */
    return xfconf_backend_get(xfconfd->backend, channel, property, value, error);
}

static gboolean
xfconf_get_all_properties(XfconfDaemon *xfconfd,
                          const gchar *channel,
                          GHashTable **properties,
                          GError **error)
{
    gboolean ret;
    
    g_return_val_if_fail(properties && !*properties, FALSE);
    
    *properties = g_hash_table_new_full(g_str_hash, g_str_equal,
                                        (GDestroyNotify)g_free,
                                        (GDestroyNotify)xfconf_g_value_free);
    
    ret = xfconf_backend_get_all(xfconfd->backend, channel, *properties, error);
    if(!ret) {
        g_hash_table_destroy(*properties);
        *properties = NULL;
    }
    
    /* FIXME: presumably, |*properties| leaks.  how do we fix this?  perhaps
     * using the org.freedesktop.DBus.GLib.Async annotation? */
    
    return ret;
}

static gboolean
xfconf_property_exists(XfconfDaemon *xfconfd,
                       const gchar *channel,
                       const gchar *property,
                       gboolean *exists,
                       GError **error)
{
    return xfconf_backend_exists(xfconfd->backend, channel, property, exists, error);
}

static gboolean
xfconf_remove_property(XfconfDaemon *xfconfd,
                       const gchar *channel,
                       const gchar *property,
                       GError **error)
{
    return xfconf_backend_remove(xfconfd->backend, channel, property, error);
}

static gboolean
xfconf_remove_channel(XfconfDaemon *xfconfd,
                      const gchar *channel,
                      GError **error)
{
    return xfconf_backend_remove_channel(xfconfd->backend, channel, error);
}

static gboolean
xfconf_gui_show_list(XfconfDaemon *xfconfd,
                     const gchar *display,
                     GError **error)
{
    gchar *command;
    gboolean ret;
    
    if(display)
        command = g_strdup_printf("env DISPLAY='%s' %s/xfconf-settings-show",
                                  display, BINDIR);
    else
        command = g_strdup(BINDIR "/xfconf-settings-show");
    
    ret = g_spawn_command_line_async(command, error);
    
    g_free(command);
    
    return ret;
}

static gboolean
xfconf_gui_show_plugin(XfconfDaemon *xfconfd,
                       const gchar *display,
                       const gchar *name,
                       GError **error)
{
    gchar *command;
    gboolean ret;
    
    g_return_val_if_fail(name, FALSE);
    
    if(display)
        command = g_strdup_printf("env DISPLAY='%s' %s/xfconf-settings-show '%s'",
                                  display, BINDIR, name);
    else
        command = g_strdup_printf("%s/xfconf-settings-show '%s'", BINDIR, name);
    
    ret = g_spawn_command_line_async(command, error);
    
    g_free(command);
    
    return ret;
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
            g_set_error(error, DBUS_GERROR, DBUS_GERROR_FAILED,
                        _("Another Xfconf daemon is already running"));
        }
        
        return FALSE;
    }
    
    return TRUE;
}

static gboolean
xfconf_daemon_load_config(XfconfDaemon *xfconfd,
                          const gchar *backend_id,
                          GError **error)
{
    xfconfd->backend = xfconf_backend_factory_get_backend(backend_id, error);
    if(!xfconfd->backend)
        return FALSE;
    
    return TRUE;
}



XfconfDaemon *
xfconf_daemon_new_unique(const gchar *backend_id,
                         GError **error)
{
    XfconfDaemon *xfconfd;
    
    g_return_val_if_fail(backend_id && *backend_id, NULL);
    
    xfconfd = g_object_new(XFCONF_TYPE_DAEMON, NULL);
    
    if(!xfconf_daemon_start(xfconfd, error)
       || !xfconf_daemon_load_config(xfconfd, backend_id, error))
    {
        g_object_unref(G_OBJECT(xfconfd));
        return NULL;
    }
    
    return xfconfd;
}
