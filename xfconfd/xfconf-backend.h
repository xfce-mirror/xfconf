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

#ifndef __XFCONF_BACKEND_H__
#define __XFCONF_BACKEND_H__

#include <glib-object.h>

#define XFCONF_TYPE_BACKEND                (xfconf_backend_get_type())
#define XFCONF_BACKEND(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj), XFCONF_TYPE_BACKEND, XfconfBackend))
#define XFCONF_IS_BACKEND(obj)             (G_TYPE_CHECK_INSTANCE_TYPE((obj), XFCONF_TYPE_BACKEND))
#define XFCONF_BACKEND_GET_INTERFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE((obj), XFCONF_TYPE_BACKEND, XfconfBackendInterface))

#define XFCONF_BACKEND_ERROR               (xfconf_backend_get_error_quark())

G_BEGIN_DECLS

typedef struct _XfconfBackend           XfconfBackend;
typedef struct _XfconfBackendInterface  XfconfBackendInterface;

struct _XfconfBackendInterface
{
    GTypeInterface parent;
    
    /**
     * XfconfBackendInterface::initialize:
     * @backend: The #XfconfBackend.
     * @error: An error return.
     *
     * Does any pre-initialization that the backend needs to function.
     *
     * Return value: The backend should return %TRUE if initialization
     *               was successful, or %FALSE otherwise.  On %FALSE,
     *               @error should be set to a description of the failure.
     **/
    gboolean (*initialize)(XfconfBackend *backend,
                           GError **error);
    
    /**
     * XfconfBackendInterface::set:
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
    gboolean (*set)(XfconfBackend *backend,
                    const gchar *channel,
                    const gchar *property,
                    const GValue *value,
                    GError **error);
    
    /**
     * XfconfBackendInterface::get:
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
    gboolean (*get)(XfconfBackend *backend,
                    const gchar *channel,
                    const gchar *property,
                    GValue *value,
                    GError **error);
    
    /**
     * XfconfBackendInterface::get_all:
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
    gboolean (*get_all)(XfconfBackend *backend,
                        const gchar *channel,
                        GHashTable *properties,
                        GError **error);
    
    /**
     * XfconfBackendInterface::exists:
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
    gboolean (*exists)(XfconfBackend *backend,
                       const gchar *channel,
                       const gchar *property,
                       gboolean *exists,
                       GError **error);
    
    /**
     * XfconfBackendInterface::remove:
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
    gboolean (*remove)(XfconfBackend *backend,
                       const gchar *channel,
                       const gchar *property,
                       GError **error);
    
    /**
     * XfconfBackendInterface::flush
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
    gboolean (*flush)(XfconfBackend *backend,
                      GError **error);
    
    /*< reserved for future expansion >*/
    void (*_xb_reserved0)();
    void (*_xb_reserved1)();
    void (*_xb_reserved2)();
    void (*_xb_reserved3)();
};

GQuark xfconf_backend_get_error_quark();

GType xfconf_backend_get_type() G_GNUC_CONST;

gboolean xfconf_backend_initialize(XfconfBackend *backend,
                                   GError **error);

gboolean xfconf_backend_set(XfconfBackend *backend,
                            const gchar *channel,
                            const gchar *property,
                            const GValue *value,
                            GError **error);

gboolean xfconf_backend_get(XfconfBackend *backend,
                            const gchar *channel,
                            const gchar *property,
                            GValue *value,
                            GError **error);

gboolean xfconf_backend_get_all(XfconfBackend *backend,
                                const gchar *channel,
                                GHashTable *properties,
                                GError **error);

gboolean xfconf_backend_exists(XfconfBackend *backend,
                               const gchar *channel,
                               const gchar *property,
                               gboolean *exists,
                               GError **error);

gboolean xfconf_backend_remove(XfconfBackend *backend,
                               const gchar *channel,
                               const gchar *property,
                               GError **error);

gboolean xfconf_backend_flush(XfconfBackend *backend,
                              GError **error);

G_END_DECLS

#endif  /* __XFCONF_BACKEND_H__ */
