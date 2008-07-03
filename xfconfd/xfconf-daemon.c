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
#include "xfconf-gvaluefuncs.h"
#include "xfconf/xfconf-errors.h"

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
    
    GList *backends;
};

typedef struct _XfconfDaemonClass
{
    GObjectClass parent;
} XfconfDaemonClass;

enum
{
    SIG_PROPERTY_CHANGED = 0,
    SIG_PROPERTY_REMOVED,
    N_SIGS,
};

static void xfconf_daemon_class_init(XfconfDaemonClass *klass);

static void xfconf_daemon_init(XfconfDaemon *instance);
static void xfconf_daemon_finalize(GObject *obj);

static guint signals[N_SIGS] = { 0, };


G_DEFINE_TYPE(XfconfDaemon, xfconf_daemon, G_TYPE_OBJECT)


static void
xfconf_daemon_class_init(XfconfDaemonClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    
    object_class->finalize = xfconf_daemon_finalize;
    
    signals[SIG_PROPERTY_CHANGED] = g_signal_new("property-changed",
                                                 XFCONF_TYPE_DAEMON,
                                                 G_SIGNAL_RUN_LAST,
                                                 0,
                                                 NULL, NULL,
                                                 xfconf_marshal_VOID__STRING_STRING_BOXED,
                                                 G_TYPE_NONE,
                                                 3, G_TYPE_STRING,
                                                 G_TYPE_STRING,
                                                 G_TYPE_VALUE);

    signals[SIG_PROPERTY_REMOVED] = g_signal_new("property-removed",
                                                 XFCONF_TYPE_DAEMON,
                                                 G_SIGNAL_RUN_LAST,
                                                 0,
                                                 NULL, NULL,
                                                 xfconf_marshal_VOID__STRING_STRING,
                                                 G_TYPE_NONE,
                                                 2, G_TYPE_STRING,
                                                 G_TYPE_STRING);

    dbus_g_object_type_install_info(G_TYPE_FROM_CLASS(klass),
                                    &dbus_glib_xfconf_object_info);
    dbus_g_error_domain_register(XFCONF_ERROR, "org.xfce.Xfconf.Error",
                                 XFCONF_TYPE_ERROR);
}

static void
xfconf_daemon_init(XfconfDaemon *instance)
{
    
}

static void
xfconf_daemon_finalize(GObject *obj)
{
    XfconfDaemon *xfconfd = XFCONF_DAEMON(obj);
    GList *l;
    
    for(l = xfconfd->backends; l; l = l->next) {
        xfconf_backend_register_property_changed_func(l->data, NULL, NULL);
        xfconf_backend_flush(l->data, NULL);
        g_object_unref(l->data);
    }
    g_list_free(xfconfd->backends);
    
    if(xfconfd->dbus_conn)
        dbus_g_connection_unref(xfconfd->dbus_conn);
    
    G_OBJECT_CLASS(xfconf_daemon_parent_class)->finalize(obj);
}

typedef struct
{
    XfconfDaemon *xfconfd;
    XfconfBackend *backend;
    gchar *channel;
    gchar *property;
} XfconfPropChangedData;

static gboolean
xfconf_daemon_emit_property_changed_idled(gpointer data)
{
    XfconfPropChangedData *pdata = data;
    GValue value = { 0, };

    xfconf_backend_get(pdata->backend, pdata->channel, pdata->property,
                       &value, NULL);

    if(G_VALUE_TYPE(&value)) {
        g_signal_emit(G_OBJECT(pdata->xfconfd), signals[SIG_PROPERTY_CHANGED],
                      0, pdata->channel, pdata->property, &value);
        g_value_unset(&value);
    } else {
        g_signal_emit(G_OBJECT(pdata->xfconfd), signals[SIG_PROPERTY_REMOVED],
                      0, pdata->channel, pdata->property);
    }

    g_object_unref(G_OBJECT(pdata->backend));
    g_free(pdata->channel);
    g_free(pdata->property);
    g_object_unref(G_OBJECT(pdata->xfconfd));
    g_slice_free(XfconfPropChangedData, pdata);

    return FALSE;
}

static void
xfconf_daemon_backend_property_changed(XfconfBackend *backend,
                                       const gchar *channel,
                                       const gchar *property,
                                       gpointer user_data)
{
    XfconfPropChangedData *pdata = g_slice_new0(XfconfPropChangedData);

    pdata->xfconfd = g_object_ref(G_OBJECT(user_data));
    pdata->backend = g_object_ref(G_OBJECT(backend));
    pdata->channel = g_strdup(channel);
    pdata->property = g_strdup(property);

    g_idle_add(xfconf_daemon_emit_property_changed_idled, pdata);
}

static gboolean
xfconf_set_property(XfconfDaemon *xfconfd,
                    const gchar *channel,
                    const gchar *property,
                    const GValue *value,
                    GError **error)
{
    /* only write to first backend */
    return xfconf_backend_set(xfconfd->backends->data, channel, property,
                              value, error);
}

static gboolean
xfconf_get_property(XfconfDaemon *xfconfd,
                    const gchar *channel,
                    const gchar *property,
                    GValue *value,
                    GError **error)
{
    GList *l;

    /* FIXME: presumably, |value| leaks.  how do we fix this?  perhaps
     * using the org.freedesktop.DBus.GLib.Async annotation? */
    
    /* check each backend until we find a value */
    for(l = xfconfd->backends; l; l = l->next) {
        if(xfconf_backend_get(l->data, channel, property, value, error))
            return TRUE;
        else if(l->next && error && *error) {
            g_error_free(*error);
            *error = NULL;
        }
    }
    
    return FALSE;
}

static gboolean
xfconf_get_all_properties(XfconfDaemon *xfconfd,
                          const gchar *channel,
                          GHashTable **properties,
                          GError **error)
{
    gboolean ret = FALSE;
    GList *l;
    
    g_return_val_if_fail(properties && !*properties, FALSE);
    
    *properties = g_hash_table_new_full(g_str_hash, g_str_equal,
                                        (GDestroyNotify)g_free,
                                        (GDestroyNotify)_xfconf_gvalue_free);
    
    /* get all properties from all backends.  if they all fail, return FALSE */
    for(l = xfconfd->backends; l; l = l->next) {
        if(xfconf_backend_get_all(l->data, channel, *properties, error))
            ret = TRUE;
        else if(l->next && error && *error) {
            g_error_free(*error);
            *error = NULL;
        }
    }
    
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
    gboolean ret = FALSE, exists_tmp = FALSE;
    GList *l;
    
    /* if at least one backend returns TRUE (regardles if |*exists| gets set
     * to TRUE or FALSE), we'll return TRUE from this function */
    
    for(l = xfconfd->backends; l; l = l->next) {
        if(xfconf_backend_exists(l->data, channel, property, &exists_tmp,
                                 error))
        {
            ret = TRUE;
            *exists = exists_tmp;
            if(*exists)
                return TRUE;
        } else if(l->next && error && *error) {
            g_error_free(*error);
            *error = NULL;
        }
    }
    
    return ret;
}

static gboolean
xfconf_remove_property(XfconfDaemon *xfconfd,
                       const gchar *channel,
                       const gchar *property,
                       GError **error)
{
    gboolean ret = FALSE;
    GList *l;
    
    /* while technically all backends but the first should be opened read-only,
     * we need to remove from all backends so the property doesn't reappear
     * later */
    
    for(l = xfconfd->backends; l; l = l->next) {
        if(xfconf_backend_remove(l->data, channel, property, error))
            ret = TRUE;
        else if(l->next && error && *error) {
            g_error_free(*error);
            *error = NULL;
        }
    }
    
    return ret;
}

static gboolean
xfconf_remove_channel(XfconfDaemon *xfconfd,
                      const gchar *channel,
                      GError **error)
{
    gboolean ret = FALSE;
    GList *l;
    
    /* while technically all backends but the first should be opened read-only,
     * we need to remove from all backends so the channel doesn't reappear
     * later */
    
    for(l = xfconfd->backends; l; l = l->next) {
        if(xfconf_backend_remove_channel(l->data, channel, error))
            ret = TRUE;
        else if(l->next && error && *error) {
            g_error_free(*error);
            *error = NULL;
        }
    }
    
    return ret;
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
                          gchar * const *backend_ids,
                          GError **error)
{
    gint i;
    
    for(i = 0; backend_ids[i]; ++i) {
        GError *error1 = NULL;
        XfconfBackend *backend = xfconf_backend_factory_get_backend(backend_ids[i],
                                                                    &error1);
        if(!backend) {
            g_warning("Unable to start backend \"%s\": %s", backend_ids[i],
                      error1->message);
            g_error_free(error1);
            error1 = NULL;
        } else {
            xfconfd->backends = g_list_prepend(xfconfd->backends, backend);
            xfconf_backend_register_property_changed_func(backend,
                                                          xfconf_daemon_backend_property_changed,
                                                          xfconfd);
        }
    }
                                           
    if(!xfconfd->backends) {
        if(error) {
            g_set_error(error, XFCONF_ERROR, XFCONF_ERROR_NO_BACKEND,
                        _("No backends could be started"));
        }
        return FALSE;
    }
    
    xfconfd->backends = g_list_reverse(xfconfd->backends);
    
    return TRUE;
}



XfconfDaemon *
xfconf_daemon_new_unique(gchar * const *backend_ids,
                         GError **error)
{
    XfconfDaemon *xfconfd;
    
    g_return_val_if_fail(backend_ids && backend_ids[0], NULL);
    
    xfconfd = g_object_new(XFCONF_TYPE_DAEMON, NULL);
    
    if(!xfconf_daemon_start(xfconfd, error)
       || !xfconf_daemon_load_config(xfconfd, backend_ids, error))
    {
        g_object_unref(G_OBJECT(xfconfd));
        return NULL;
    }
    
    return xfconfd;
}
