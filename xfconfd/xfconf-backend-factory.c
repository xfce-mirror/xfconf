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

/* i'm not sure i like this method.  perhaps each backend could be a
 * GTypeModule.  i also want the ability to multiplex multiple backends.
 * for example, i'd like to write a MCS backend that can read the old MCS
 * config files to ease migration to the new system, but of course the 'new'
 * backend should be the one that gets written to all the time.
 */

#ifdef XFCONF_BACKEND_PERCHANNEL_XML
#include "xfconf-backend-perchannel-xml.h"
#endif

static GQuark error_quark = 0;
static GHashTable *backends = NULL;

static void
xfconf_backend_factory_ensure_backends()
{
    if(backends)
        return;
    
    backends = g_hash_table_new(g_str_hash, g_str_equal);
    
#ifdef XFCONF_BACKEND_PERCHANNEL_XML
    g_hash_table_insert(backends, XFCONF_BACKEND_PERCHANNEL_XML_TYPE_ID,
                        XFCONF_TYPE_BACKEND_PERCHANNEL_XML);
#endif
}


XfconfBackend *
xfconf_backend_factory_get_backend(const gchar *type,
                                   GError **error)
{
    XfconfBackend *backend = NULL;
    GType backend_gtype;
    
    xfconf_backend_factory_ensure_backends();
    
    backend_gtype = g_hash_table_lookup(backends, type);
    if(G_TYPE_NONE == backend_type) {
        if(error) {
            g_set_error(error, XFCONF_BACKEND_ERROR, 0,
                        _("Unable to find Xfconf backend of type \"%s\""),
                        type);
        }
        return NULL;
    }
    
    backend = g_object_new(backend_gtype, NULL);
    if(!xfconf_backend_initialize(backend, error)) {
        g_object_unref(G_OBJECT(backend));
        return NULL;
    }
    
    return backend;
}
