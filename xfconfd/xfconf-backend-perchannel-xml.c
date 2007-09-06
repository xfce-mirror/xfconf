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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4util/libxfce4util.h>

#include "xfconf-backend-perchannel-xml.h"
#include "xfconf-backend.h"

struct _XfconfBackendPerchannelXml
{
    GObject parent;
    
    gchar *config_path;
};

typedef struct _XfconfBackendPerchannelXmlClass
{
    GObjectClass parent;
} XfconfBackendPerchannelXmlClass;

static void xfconf_backend_perchannel_xml_class_init(XfconfBackendPerchannelXmlClass *klass);

static void xfconf_backend_perchannel_xml_init(XfconfBackendPerchannelXml *instance);
static void xfconf_backend_perchannel_xml_finalize(GObject *obj);

static void xfconf_backend_perchannel_xml_backend_init(XfconfBackendInterface *iface);

static gboolean xfconf_backend_perchannel_xml_initialize(XfconfBackend *backend,
                                                         GError **error);
static gboolean xfconf_backend_perchannel_xml_set(XfconfBackend *backend,
                                                  const gchar *channel,
                                                  const gchar *property,
                                                  const GValue *value,
                                                  GError **error);
static gboolean xfconf_backend_perchannel_xml_get(XfconfBackend *backend,
                                                  const gchar *channel,
                                                  const gchar *property,
                                                  GValue *value,
                                                  GError **error);
static gboolean xfconf_backend_perchannel_xml_get_all(XfconfBackend *backend,
                                                      const gchar *channel,
                                                      GHashTable *properties,
                                                      GError **error);
static gboolean xfconf_backend_perchannel_xml_exists(XfconfBackend *backend,
                                                     const gchar *channel,
                                                     const gchar *property,
                                                     gboolean *exists,
                                                     GError **error);
static gboolean xfconf_backend_perchannel_xml_remove(XfconfBackend *backend,
                                                     const gchar *channel,
                                                     const gchar *property,
                                                     GError **error);
static gboolean xfconf_backend_perchannel_xml_remove_channel(XfconfBackend *backend,
                                                             const gchar *channel,
                                                             GError **error);
static gboolean xfconf_backend_perchannel_xml_flush(XfconfBackend *backend,
                                                    GError **error);


G_DEFINE_TYPE_WITH_CODE(XfconfBackendPerchannelXml, xfconf_backend_perchannel_xml, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(XFCONF_TYPE_BACKEND,
                                              xfconf_backend_perchannel_xml_backend_init))


static void
xfconf_backend_perchannel_xml_class_init(XfconfBackendPerchannelXmlClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    
    object_class->finalize = xfconf_backend_perchannel_xml_finalize;
}

static void
xfconf_backend_perchannel_xml_init(XfconfBackendPerchannelXml *instance)
{
    
}

static void
xfconf_backend_perchannel_xml_finalize(GObject *obj)
{
    G_OBJECT_CLASS(xfconf_backend_perchannel_xml_parent_class)->finalize(obj);
}

static void
xfconf_backend_perchannel_xml_backend_init(XfconfBackendInterface *iface)
{
    iface->initialize = xfconf_backend_perchannel_xml_initialize;
    iface->set = xfconf_backend_perchannel_xml_set;
    iface->get = xfconf_backend_perchannel_xml_get;
    iface->get_all = xfconf_backend_perchannel_xml_get_all;
    iface->exists = xfconf_backend_perchannel_xml_exists;
    iface->remove = xfconf_backend_perchannel_xml_remove;
    iface->remove_channel = xfconf_backend_perchannel_xml_remove_channel;
    iface->flush = xfconf_backend_perchannel_xml_flush;
}

static gboolean
xfconf_backend_perchannel_xml_initialize(XfconfBackend *backend,
                                         GError **error)
{
    XfconfBackendPerchannelXml *backend_px = XFCONF_BACKEND_PERCHANNEL_XML(backend);
    gchar *path = xfce_resource_save_location(XFCE_RESOURCE_CONFIG,
                                              "xfce4/xfconf/" XFCONF_BACKEND_PERCHANNEL_XML_TYPE_ID "/",
                                              TRUE);
    
    if(!path || !g_file_test(path, G_FILE_TEST_IS_DIR)) {
        if(error) {
            g_set_error(error, G_FILE_ERROR, 0,
                        _("Unable to create configuration directory"));
        }
        g_free(path);
        return FALSE;
    }
    
    backend_px->config_path = path;
    
    return TRUE;
}

static gboolean
xfconf_backend_perchannel_xml_set(XfconfBackend *backend,
                                  const gchar *channel,
                                  const gchar *property,
                                  const GValue *value,
                                  GError **error)
{
    return FALSE;
}

static gboolean
xfconf_backend_perchannel_xml_get(XfconfBackend *backend,
                                  const gchar *channel,
                                  const gchar *property,
                                  GValue *value,
                                  GError **error)
{
    return FALSE;
}

static gboolean
xfconf_backend_perchannel_xml_get_all(XfconfBackend *backend,
                                      const gchar *channel,
                                      GHashTable *properties,
                                      GError **error)
{
    return FALSE;
}

static gboolean
xfconf_backend_perchannel_xml_exists(XfconfBackend *backend,
                                     const gchar *channel,
                                     const gchar *property,
                                     gboolean *exists,
                                     GError **error)
{
    return FALSE;
}

static gboolean
xfconf_backend_perchannel_xml_remove(XfconfBackend *backend,
                                     const gchar *channel,
                                     const gchar *property,
                                     GError **error)
{
    return FALSE;
}

static gboolean
xfconf_backend_perchannel_xml_remove_channel(XfconfBackend *backend,
                                             const gchar *channel,
                                             GError **error)
{
    return FALSE;
}

static gboolean
xfconf_backend_perchannel_xml_flush(XfconfBackend *backend,
                                    GError **error)
{
    return TRUE;
}
