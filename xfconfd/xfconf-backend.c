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

#include <dbus/dbus-glib.h>

#include "xfconf-backend.h"


static void xfconf_backend_base_init(gpointer g_class);

static inline gboolean xfconf_property_is_valid(const gchar *property,
                                                GError **error);
static inline gboolean xfconf_channel_is_valid(const gchar *channel,
                                               GError **error);

/**
 * XfconfBackendInterface:
 *
 * An interface for implementing pluggable configuration store backends
 * into the Xfconf Daemon.
 *
 * See the #XfconfBackend function documentation for a description of what
 * each virtual function in #XfconfBackendInterface should do.
 **/


/**
 * XfconfBackend:
 *
 * An instance of a class implementing a #XfconfBackendInterface.
 **/


GType
xfconf_backend_get_type()
{
    static GType backend_type = 0;
    
    if(!backend_type) {
        static const GTypeInfo backend_info = {
            sizeof(XfconfBackendInterface),
            xfconf_backend_base_init,
            NULL,
            NULL,
            NULL,
            NULL,
            0,
            0,
            NULL,
        };
        
        backend_type = g_type_register_static(G_TYPE_INTERFACE,"XfconfBackend",
                                              &backend_info, 0);
        g_type_interface_add_prerequisite(backend_type, G_TYPE_OBJECT);
    }
    
    return backend_type;
}



static void
xfconf_backend_base_init(gpointer g_class)
{
    static gboolean _inited = FALSE;
    
    if(!_inited) {
        _inited = TRUE;
    }
}



static inline gboolean
xfconf_property_is_valid(const gchar *property,
                         GError **error)
{
    const gchar *p = property;

    if(!p || *p != '/') {
        if(error) {
            g_set_error(error, XFCONF_ERROR, XFCONF_ERROR_INVALID_PROPERTY,
                        _("Property names must start with a '/' character"));
        }
        return FALSE;
    }

    p++;
    while(*p) {
        if(!(*p >= 'A' && *p <= 'Z') && !(*p >= 'a' && *p <= 'z')
           && *p != '_' && *p != '-' && *p != '/')
        {
            if(error) {
                g_set_error(error, XFCONF_ERROR,
                            XFCONF_ERROR_INVALID_PROPERTY,
                            _("Property names can only include the ASCII characters A-Z, a-z, 0-9, '_', and '-', as well as '/' as a separator"));
            }
            return FALSE;
        }

        if('/' == *p && '/' == *(p-1)) {
            if(error) {
                g_set_error(error, XFCONF_ERROR,
                            XFCONF_ERROR_INVALID_PROPERTY,
                            _("Property names cannot have two or more '/' characters in a row"));
            }
            return FALSE;
        }

        p++;
    }

    return TRUE;
}

static inline gboolean
xfconf_channel_is_valid(const gchar *channel,
                        GError **error)
{
    const gchar *p = channel;

    if(!p || !*p) {
        if(error) {
            g_set_error(error, XFCONF_ERROR, XFCONF_ERROR_INVALID_CHANNEL,
                        _("Channel name cannot be an empty string"));
        }
        return FALSE;
    }

    p++;
    while(*p) {
        if(!(*p >= 'A' && *p <= 'Z') && !(*p >= 'a' && *p <= 'z')
           && *p != '_' && *p != '-')
        {
            if(error) {
                g_set_error(error, XFCONF_ERROR,
                            XFCONF_ERROR_INVALID_CHANNEL,
                            _("Channel names can only include the ASCII characters A-Z, a-z, 0-9, '_', and '-'"));
            }
            return FALSE;
        }
        p++;
    }

    return TRUE;
}



/**
 * xfconf_backend_initialize:
 * @backend: The #XfconfBackend.
 * @error: An error return.
 *
 * Does any pre-initialization that the backend needs to function.
 *
 * Return value: The backend should return %TRUE if initialization
 *               was successful, or %FALSE otherwise.  On %FALSE,
 *               @error should be set to a description of the failure.
 **/
gboolean
xfconf_backend_initialize(XfconfBackend *backend,
                          GError **error)
{
    XfconfBackendInterface *iface = XFCONF_BACKEND_GET_INTERFACE(backend);
    
    xfconf_backend_return_val_if_fail(iface && iface->initialize
                                      && (!error || !*error), FALSE);
    
    return iface->initialize(backend, error);
}

/**
 * xfconf_backend_set:
 * @backend: The #XfconfBackend.
 * @channel: A channel name.
 * @property: A property name.
 * @value: A value.
 * @error: An error return.
 *
 * Sets the variant @value for @property on @channel.
 *
 * Return value: The backend should return %TRUE if the operation
 *               was successful, or %FALSE otherwise.  On %FALSE,
 *               @error should be set to a description of the failure.
 **/
gboolean
xfconf_backend_set(XfconfBackend *backend,
                   const gchar *channel,
                   const gchar *property,
                   const GValue *value,
                   GError **error)
{
    XfconfBackendInterface *iface = XFCONF_BACKEND_GET_INTERFACE(backend);
    
    xfconf_backend_return_val_if_fail(iface && iface->set && channel && *channel
                                      && property && *property
                                      && value && (!error || !*error), FALSE);
    if(!xfconf_channel_is_valid(channel, error))
        return FALSE;
    if(!xfconf_property_is_valid(property, error))
        return FALSE;
    
    return iface->set(backend, channel, property, value, error);
}

/**
 * xfconf_backend_get:
 * @backend: The #XfconfBackend.
 * @channel: A channel name.
 * @property: A property name.
 * @value: A #GValue return.
 * @error: An error return.
 *
 * Gets the value of @property on @channel and stores it in @value.
 *
 * Return value: The backend should return %TRUE if the operation
 *               was successful, or %FALSE otherwise.  On %FALSE,
 *               @error should be set to a description of the failure.
 **/
gboolean
xfconf_backend_get(XfconfBackend *backend,
                   const gchar *channel,
                   const gchar *property,
                   GValue *value,
                   GError **error)
{
    XfconfBackendInterface *iface = XFCONF_BACKEND_GET_INTERFACE(backend);
    
    xfconf_backend_return_val_if_fail(iface && iface->get && channel && *channel
                                      && property && *property
                                      && value && (!error || !*error), FALSE);
    if(!xfconf_channel_is_valid(channel, error))
        return FALSE;
    if(!xfconf_property_is_valid(property, error))
        return FALSE;
    
    return iface->get(backend, channel, property, value, error);
}

/**
 * xfconf_backend_get_all:
 * @backend: The #XfconfBackend.
 * @channel: A channel name.
 * @properties: A #GHashTable.
 * @error: An error return.
 *
 * Gets all properties and values on @channel and stores them in
 * @properties, which is already initialized to hold #gchar* keys and
 * #GValue<!-- -->* values.
 *
 * Return value: The backend should return %TRUE if the operation
 *               was successful, or %FALSE otherwise.  On %FALSE,
 *               @error should be set to a description of the failure.
 **/
gboolean
xfconf_backend_get_all(XfconfBackend *backend,
                       const gchar *channel,
                       GHashTable *properties,
                       GError **error)
{
    XfconfBackendInterface *iface = XFCONF_BACKEND_GET_INTERFACE(backend);
    
    xfconf_backend_return_val_if_fail(iface && iface->get_all && channel
                                      && *channel && properties
                                      && (!error || !*error), FALSE);
    if(!xfconf_channel_is_valid(channel, error))
        return FALSE;

    return iface->get_all(backend, channel, properties, error);
}

/**
 * xfconf_backend_exists:
 * @backend: The #XfconfBackend.
 * @channel: A channel name.
 * @property: A property name.
 * @exists: A boolean return.
 * @error: An error return.
 *
 * Checks to see if @property exists on @channel, and stores %TRUE or
 * %FALSE in @exists.
 *
 * Return value: The backend should return %TRUE if the operation
 *               was successful, or %FALSE otherwise.  On %FALSE,
 *               @error should be set to a description of the failure.
 **/
gboolean
xfconf_backend_exists(XfconfBackend *backend,
                      const gchar *channel,
                      const gchar *property,
                      gboolean *exists,
                      GError **error)
{
    XfconfBackendInterface *iface = XFCONF_BACKEND_GET_INTERFACE(backend);
    
    xfconf_backend_return_val_if_fail(iface && iface->exists && channel
                                      && *channel && property && *property
                                      && exists
                                      && (!error || !*error), FALSE);
    if(!xfconf_channel_is_valid(channel, error))
        return FALSE;
    if(!xfconf_property_is_valid(property, error))
        return FALSE;
 
    return iface->exists(backend, channel, property, exists, error);
}

/**
 * xfconf_backend_remove:
 * @backend: The #XfconfBackend.
 * @channel: A channel name.
 * @property: A property name.
 * @error: An error return.
 *
 * Removes the property identified by @property from @channel.
 *
 * Return value: The backend should return %TRUE if the operation
 *               was successful, or %FALSE otherwise.  On %FALSE,
 *               @error should be set to a description of the failure.
 **/
gboolean
xfconf_backend_remove(XfconfBackend *backend,
                      const gchar *channel,
                      const gchar *property,
                      GError **error)
{
    XfconfBackendInterface *iface = XFCONF_BACKEND_GET_INTERFACE(backend);
    
    xfconf_backend_return_val_if_fail(iface && iface->remove && channel
                                      && *channel && property && *property
                                      && (!error || !*error), FALSE);
    if(!xfconf_channel_is_valid(channel, error))
        return FALSE;
    if(!xfconf_property_is_valid(property, error))
        return FALSE;
    
    return iface->remove(backend, channel, property, error);
}

/**
 * xfconf_backend_remove_channel:
 * @backend: An #XfconfBackend.
 * @channel: A channel name.
 * @error: An error return.
 *
 * Removes @channel (and al its properties) from the confguration store.
 *
 * Return value: The backend should return %TRUE if the operation
 *               was successful, or %FALSE otherwise.  On %FALSE,
 *               @error should be set to a description of the failure.
 **/ 
gboolean
xfconf_backend_remove_channel(XfconfBackend *backend,
                              const gchar *channel,
                              GError **error)
{
    XfconfBackendInterface *iface = XFCONF_BACKEND_GET_INTERFACE(backend);
    
    xfconf_backend_return_val_if_fail(iface && iface->remove_channel && channel
                                      && *channel
                                      && (!error || !*error), FALSE);
    if(!xfconf_channel_is_valid(channel, error))
        return FALSE;

    return iface->remove_channel(backend, channel, error);
}

/**
 * xfconf_backend_flush
 * @backend: The #XfconfBackend.
 * @error: An error return.
 *
 * For backends that support persistent storage, ensures that all
 * configuration data stored in memory is saved to persistent storage.
 *
 * Return value: The backend should return %TRUE if the operation
 *               was successful, or %FALSE otherwise.  On %FALSE,
 *               @error should be set to a description of the failure.
 **/
gboolean
xfconf_backend_flush(XfconfBackend *backend,
                     GError **error)
{
    XfconfBackendInterface *iface = XFCONF_BACKEND_GET_INTERFACE(backend);
    
    xfconf_backend_return_val_if_fail(iface && iface->flush
                                      && (!error || !*error), FALSE);
    
    return iface->flush(backend, error);
}

/**
 * xfconf_backend_register_property_changed_func:
 * @backend: The #XfconfBackend.
 * @func: A function of type #XfconfPropertyChangeFunc.
 * @user_data: Arbitrary caller-supplied data.
 *
 * Registers a function to be called when a property changes.  The
 * backend implementation should keep a pointer to @func and @user_data
 * and call @func when a property in the configuration store changes.
 **/
void
xfconf_backend_register_property_changed_func(XfconfBackend *backend,
                                              XfconfPropertyChangedFunc func,
                                              gpointer user_data)
{
    XfconfBackendInterface *iface = XFCONF_BACKEND_GET_INTERFACE(backend);

    g_return_if_fail(iface);
    if(!iface->register_property_changed_func)
        return;

    iface->register_property_changed_func(backend, func, user_data);
}
