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

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#define XFCONF_TYPE_BACKEND                (xfconf_backend_get_type())
#define XFCONF_BACKEND(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj), XFCONF_TYPE_BACKEND, XfconfBackend))
#define XFCONF_IS_BACKEND(obj)             (G_TYPE_CHECK_INSTANCE_TYPE((obj), XFCONF_TYPE_BACKEND))
#define XFCONF_BACKEND_GET_INTERFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE((obj), XFCONF_TYPE_BACKEND, XfconfBackendInterface))

#define XFCONF_TYPE_BACKEND_ERROR          (xfconf_backend_error_get_type())
#define XFCONF_BACKEND_ERROR               (xfconf_backend_get_error_quark())

#define xfconf_backend_return_val_if_fail(cond, val)  G_STMT_START{ \
    if(!(cond)) { \
        if(error) { \
            g_set_error(error, XFCONF_BACKEND_ERROR, \
                        XFCONF_BACKEND_ERROR_INTERNAL_ERROR, \
                        _("An internal error occurred; this is probably a bug")); \
        } \
        g_return_val_if_fail((cond), (val)); \
        return (val);  /* ensure return even if G_DISABLE_CHECKS */ \
    } \
}G_STMT_END

G_BEGIN_DECLS

typedef enum
{
    XFCONF_BACKEND_ERROR_UNKNOWN = 0,
    XFCONF_BACKEND_ERROR_CHANNEL_NOT_FOUND,
    XFCONF_BACKEND_ERROR_PROPERTY_NOT_FOUND,
    XFCONF_BACKEND_ERROR_READ_FAILURE,
    XFCONF_BACKEND_ERROR_WRITE_FAILURE,
    XFCONF_BACKEND_ERROR_INTERNAL_ERROR,
} XfconfBackendError;

typedef struct _XfconfBackend           XfconfBackend;
typedef struct _XfconfBackendInterface  XfconfBackendInterface;

struct _XfconfBackendInterface
{
    GTypeInterface parent;
    
    gboolean (*initialize)(XfconfBackend *backend,
                           GError **error);
    
    gboolean (*set)(XfconfBackend *backend,
                    const gchar *channel,
                    const gchar *property,
                    const GValue *value,
                    GError **error);
    
    gboolean (*get)(XfconfBackend *backend,
                    const gchar *channel,
                    const gchar *property,
                    GValue *value,
                    GError **error);
    
    gboolean (*get_all)(XfconfBackend *backend,
                        const gchar *channel,
                        GHashTable *properties,
                        GError **error);
    
    gboolean (*exists)(XfconfBackend *backend,
                       const gchar *channel,
                       const gchar *property,
                       gboolean *exists,
                       GError **error);
    
    gboolean (*remove)(XfconfBackend *backend,
                       const gchar *channel,
                       const gchar *property,
                       GError **error);
    
    gboolean (*remove_channel)(XfconfBackend *backend,
                               const gchar *channel,
                               GError **error);
    
    gboolean (*flush)(XfconfBackend *backend,
                      GError **error);
    
    /*< reserved for future expansion >*/
    void (*_xb_reserved0)();
    void (*_xb_reserved1)();
    void (*_xb_reserved2)();
    void (*_xb_reserved3)();
};

GType xfconf_backend_error_get_type() G_GNUC_CONST;
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

gboolean xfconf_backend_remove_channel(XfconfBackend *backend,
                                       const gchar *channel,
                                       GError **error);
    
gboolean xfconf_backend_flush(XfconfBackend *backend,
                              GError **error);

G_END_DECLS

#endif  /* __XFCONF_BACKEND_H__ */
