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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "xfconf-channel.h"
#include "xfconf-dbus-bindings.h"
#include "xfconf-private.h"
#include "xfconf-marshal.h"

struct _XfconfChannel
{
    GObject parent;
    
    gchar *channel_name;
};

typedef struct _XfconfChannelClass
{
    GObjectClass parent;
    
    /*< signals >*/
    void (*property_changed)(XfconfChannel *channel,
                             const gchar *property);
} XfconfChannelClass;

enum
{
    SIG_PROPERTY_CHANGED = 0,
    N_SIGS,
};

enum
{
    PROP0 = 0,
    PROP_CHANNEL_NAME,
};

static void xfconf_channel_class_init(XfconfChannelClass *klass);

static void xfconf_channel_init(XfconfChannel *instance);
static void xfconf_channel_set_property(GObject *object,
                                        guint property_id,
                                        const GValue *value,
                                        GParamSpec *pspec);
static void xfconf_channel_get_property(GObject *object,
                                        guint property_id,
                                        GValue *value,
                                        GParamSpec *pspec);
static void xfconf_channel_finalize(GObject *obj);

static void xfconf_channel_property_changed(DBusGProxy *proxy,
                                            const gchar *channel_name,
                                            const gchar *property,
                                            gpointer user_data);

static guint signals[N_SIGS] = { 0, };


G_DEFINE_TYPE(XfconfChannel, xfconf_channel, G_TYPE_OBJECT)


static void
xfconf_channel_class_init(XfconfChannelClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    
    object_class->set_property = xfconf_channel_set_property;
    object_class->get_property = xfconf_channel_get_property;
    object_class->finalize = xfconf_channel_finalize;
    
    /**
     * XfconfChannel::property-changed:
     * @channel: An #XfconfChannel.
     * @property: A property name.
     * @user_data: The user_data passed to g_signal_connect().
     *
     * Emitted when a property on @channel has changed.
     **/
    signals[SIG_PROPERTY_CHANGED] = g_signal_new("property-changed",
                                                 XFCONF_TYPE_CHANNEL,
                                                 G_SIGNAL_RUN_LAST,
                                                 G_STRUCT_OFFSET(XfconfChannelClass,
                                                                 property_changed),
                                                 NULL,
                                                 NULL,
                                                 g_cclosure_marshal_VOID__STRING,
                                                 G_TYPE_NONE,
                                                 1, G_TYPE_STRING);
    
    /**
     * XfconfChannel::channel-name:
     *
     * The string identifier used for this channel.
     **/
    g_object_class_install_property(object_class, PROP_CHANNEL_NAME,
                                    g_param_spec_string("channel-name",
                                                        "Channel Name",
                                                        "The name of the channel",
                                                        NULL,
                                                        G_PARAM_READWRITE
                                                        | G_PARAM_CONSTRUCT_ONLY));
    
    dbus_g_object_register_marshaller(xfconf_marshal_VOID__STRING_STRING,
                                      G_TYPE_NONE, G_TYPE_STRING, G_TYPE_STRING,
                                      G_TYPE_INVALID);
}

static void
xfconf_channel_init(XfconfChannel *instance)
{
    
}

static void
xfconf_channel_set_property(GObject *object,
                            guint property_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
    switch(property_id) {
        case PROP_CHANNEL_NAME:
            XFCONF_CHANNEL(object)->channel_name = g_strdup(g_value_get_string(value));
            break;
        
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
xfconf_channel_get_property(GObject *object,
                            guint property_id,
                            GValue *value,
                            GParamSpec *pspec)
{
    switch(property_id) {
        case PROP_CHANNEL_NAME:
            g_value_set_string(value, XFCONF_CHANNEL(object)->channel_name);
            break;
        
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
xfconf_channel_finalize(GObject *obj)
{
    XfconfChannel *channel = XFCONF_CHANNEL(obj);
    DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();
    
    dbus_g_proxy_disconnect_signal(proxy, "Changed",
                                   G_CALLBACK(xfconf_channel_property_changed),
                                   channel);
    
    g_free(channel->channel_name);
    
    G_OBJECT_CLASS(xfconf_channel_parent_class)->finalize(obj);
}



static void
xfconf_channel_property_changed(DBusGProxy *proxy,
                                const gchar *channel_name,
                                const gchar *property,
                                gpointer user_data)
{
    /* FIXME: optimise this by keeping track of all channels in a big hashtable
     * and using only a single instance of this callback for the class */
    XfconfChannel *channel = XFCONF_CHANNEL(user_data);
    
    if(strcmp(channel_name, channel->channel_name))
        return;
    
    g_signal_emit(G_OBJECT(channel), signals[SIG_PROPERTY_CHANGED], 0,
                  property);
}


/**
 * xfconf_channel_new:
 * @channel_name: A channel name.
 *
 * Creates a new channel using @name as the channel's identifier.
 * Note that this function does no checking to see if the channel
 * exists or is new.
 *
 * Returns: A new #XfconfChannel.
 **/
XfconfChannel *
xfconf_channel_new(const gchar *channel_name)
{
    DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();
    XfconfChannel *channel = g_object_new(XFCONF_TYPE_CHANNEL,
                                          "channel-name", channel_name,
                                          NULL);
    
    dbus_g_proxy_connect_signal(proxy, "Changed",
                                G_CALLBACK(xfconf_channel_property_changed),
                                channel, NULL);
    
    return channel;
}

gboolean
xfconf_channel_get_string(XfconfChannel *channel,
                          const gchar *property,
                          gchar **value)
{
    DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();
    GValue val;
    
    g_return_val_if_fail(XFCONF_IS_CHANNEL(channel) && property && value, FALSE);
    
    g_value_init(&val, G_TYPE_STRING);
    if(!xfconf_client_get(proxy, channel->channel_name, property, &val, NULL))
        return FALSE;
    
    *value = g_value_dup_string(&val);
    g_value_unset(&val);
    
    return TRUE;
}

gboolean
xfconf_channel_get_string_list(XfconfChannel *channel,
                               const gchar *property,
                               gchar ***value)
{
    DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();
    GValue val;
    GPtrArray *arr;
    gint i;
    
    g_return_val_if_fail(XFCONF_IS_CHANNEL(channel) && property && value, FALSE);
    
    g_value_init(&val, dbus_g_type_get_collection("GPtrArray", G_TYPE_STRING));
    if(!xfconf_client_get(proxy, channel->channel_name, property, &val, NULL))
        return FALSE;
    
    arr = g_value_get_boxed(&val);
    
    *value = g_new(gchar *, arr->len + 1);
    for(i = 0; i < arr->len; ++i)
        (*value)[i] = g_strdup(arr->pdata[i]);
    (*value)[arr->len] = NULL;
    
    g_value_unset(&val);
    
    return TRUE;
}

gboolean
xfconf_channel_get_int(XfconfChannel *channel,
                       const gchar *property,
                       gint *value)
{
    DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();
    GValue val;
    
    g_return_val_if_fail(XFCONF_IS_CHANNEL(channel) && property && value, FALSE);
    
    g_value_init(&val, G_TYPE_INT);
    if(!xfconf_client_get(proxy, channel->channel_name, property, &val, NULL))
        return FALSE;
    
    *value = g_value_get_int(&val);
    g_value_unset(&val);
    
    return TRUE;
}

gboolean
xfconf_channel_get_int64(XfconfChannel *channel,
                         const gchar *property,
                         gint64 *value)
{
    DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();
    GValue val;
    
    g_return_val_if_fail(XFCONF_IS_CHANNEL(channel) && property && value, FALSE);
    
    g_value_init(&val, G_TYPE_INT64);
    if(!xfconf_client_get(proxy, channel->channel_name, property, &val, NULL))
        return FALSE;
    
    *value = g_value_get_int64(&val);
    g_value_unset(&val);
    
    return TRUE;
}

gboolean
xfconf_channel_get_double(XfconfChannel *channel,
                          const gchar *property,
                          gdouble *value)
{
    DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();
    GValue val;
    
    g_return_val_if_fail(XFCONF_IS_CHANNEL(channel) && property && value, FALSE);
    
    g_value_init(&val, G_TYPE_DOUBLE);
    if(!xfconf_client_get(proxy, channel->channel_name, property, &val, NULL))
        return FALSE;
    
    *value = g_value_get_double(&val);
    g_value_unset(&val);
    
    return TRUE;
}

gboolean
xfconf_channel_get_bool(XfconfChannel *channel,
                        const gchar *property,
                        gboolean *value)
{
    DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();
    GValue val;
    
    g_return_val_if_fail(XFCONF_IS_CHANNEL(channel) && property && value, FALSE);
    
    g_value_init(&val, G_TYPE_BOOLEAN);
    if(!xfconf_client_get(proxy, channel->channel_name, property, &val, NULL))
        return FALSE;
    
    *value = g_value_get_boolean(&val);
    g_value_unset(&val);
    
    return TRUE;
}

gboolean
xfconf_channel_set_string(XfconfChannel *channel,
                          const gchar *property,
                          const gchar *value)
{
    DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();
    GValue val;
    gboolean ret;
    
    g_return_val_if_fail(XFCONF_IS_CHANNEL(channel) && property && value, FALSE);
    
    g_value_init(&val, G_TYPE_STRING);
    g_value_set_string(&val, value);
    
    ret = xfconf_client_set(proxy, channel->channel_name, property, &val, NULL);
    
    g_value_unset(&val);
    
    return ret;
}

gboolean
xfconf_channel_set_string_list(XfconfChannel *channel,
                               const gchar *property,
                               const gchar **value)
{
    DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();
    GPtrArray *arr;
    GValue val;
    gboolean ret;
    gint i, count = 0;
    
    g_return_val_if_fail(XFCONF_IS_CHANNEL(channel) && property && value, FALSE);
    
    while(value[count])
        ++count;
    
    arr = g_ptr_array_sized_new(count);
    for(i = 0; i < count; ++i)
        g_ptr_array_add(arr, (gpointer)value[i]);
    
    g_value_init(&val, dbus_g_type_get_collection("GPtrArray", G_TYPE_STRING));
    g_value_set_boxed(&val, arr);
    
    ret = xfconf_client_set(proxy, channel->channel_name, property, &val, NULL);
    
    g_value_unset(&val);
    g_ptr_array_free(arr, TRUE);
    
    return ret;
}

gboolean
xfconf_channel_set_int(XfconfChannel *channel,
                       const gchar *property,
                       gint value)
{
    DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();
    GValue val;
    gboolean ret;
    
    g_return_val_if_fail(XFCONF_IS_CHANNEL(channel) && property, FALSE);
    
    g_value_init(&val, G_TYPE_INT);
    g_value_set_int(&val, value);
    
    ret = xfconf_client_set(proxy, channel->channel_name, property, &val, NULL);
    
    g_value_unset(&val);
    
    return ret;
}

gboolean
xfconf_channel_set_int64(XfconfChannel *channel,
                         const gchar *property,
                         gint64 value)
{
    DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();
    GValue val;
    gboolean ret;
    
    g_return_val_if_fail(XFCONF_IS_CHANNEL(channel) && property, FALSE);
    
    g_value_init(&val, G_TYPE_INT64);
    g_value_set_int64(&val, value);
    
    ret = xfconf_client_set(proxy, channel->channel_name, property, &val, NULL);
    
    g_value_unset(&val);
    
    return ret;
}

gboolean
xfconf_channel_set_double(XfconfChannel *channel,
                          const gchar *property,
                          gdouble value)
{
    DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();
    GValue val;
    gboolean ret;
    
    g_return_val_if_fail(XFCONF_IS_CHANNEL(channel) && property, FALSE);
    
    g_value_init(&val, G_TYPE_DOUBLE);
    g_value_set_double(&val, value);
    
    ret = xfconf_client_set(proxy, channel->channel_name, property, &val, NULL);
    
    g_value_unset(&val);
    
    return ret;
}

gboolean
xfconf_channel_set_bool(XfconfChannel *channel,
                        const gchar *property,
                        gboolean value)
{
    DBusGProxy *proxy = _xfconf_get_dbus_g_proxy();
    GValue val;
    gboolean ret;
    
    g_return_val_if_fail(XFCONF_IS_CHANNEL(channel) && property, FALSE);
    
    g_value_init(&val, G_TYPE_BOOLEAN);
    g_value_set_boolean(&val, value);
    
    ret = xfconf_client_set(proxy, channel->channel_name, property, &val, NULL);
    
    g_value_unset(&val);
    
    return ret;
}
