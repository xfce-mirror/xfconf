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

#include "xfconf-backend.h"

GType
xfconf_backend_get_type()
{
    static GType backend_type = 0;
    
    if(!backend_type) {
        static const GTypeInfo backend_info = {
            sizeof(XfconfBackendInterface),
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            0,
            0,
            NULL,
        };
        
        backend_type = g_type_register_static(G_TYPE_INTERFACE,
                                              "XfconfBackend");
        g_type_interface_add_prerequisite(backend_type, G_TYPE_OBJECT);
    }
    
    return backend_type;
}


gboolean
xfconf_backend_initialize(XfconfBackend *backend,
                          GError **error)
{
    XfconfBackendInterface *iface = XFCONF_BACKEND_GET_INTERFACE(backend);
    
    g_return_val_if_fail(iface && iface->initialize, FALSE);
    
    return iface->initialize(backend, error);
}

gboolean
xfconf_backend_set(XfconfBackend *backend,
                   const gchar *channel,
                   const gchar *property,
                   const GValue *value,
                   GError **error)
{
    XfconfBackendInterface *iface = XFCONF_BACKEND_GET_INTERFACE(backend);
    
    g_return_val_if_fail(iface && iface->set && channel && property && value
                         && (!error || *error), FALSE);
    
    return iface->set(backend, channel, property, value, error);
}
    
gboolean
xfconf_backend_get(XfconfBackend *backend,
                   const gchar *channel,
                   const gchar *property,
                   GValue *value,
                   GError **error)
{
    XfconfBackendInterface *iface = XFCONF_BACKEND_GET_INTERFACE(backend);
    
    g_return_val_if_fail(iface && iface->get && channel && property && value
                         && (!error || *error), FALSE);
    
    return iface->get(backend, channel, property, value, error);
}
