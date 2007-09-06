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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <libxfce4util/libxfce4util.h>
#include <dbus/dbus-glib.h>

#include "xfconf-backend-perchannel-xml.h"
#include "xfconf-backend.h"
#include "xfconf-util.h"

#define CACHE_TIMEOUT  (20*60*1000)  /* 20 minutes */
#define WRITE_TIMEOUT  (5*1000)  /* 5 secionds */

struct _XfconfBackendPerchannelXml
{
    GObject parent;
    
    gchar *config_path;
    
    GTree *channels;
    
    guint save_id;
    GList *dirty_channels;
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

static void xfconf_backend_perchannel_xml_schedule_save(XfconfBackendPerchannelXml *xbpx,
                                                        const gchar *channel);

static GTree *xfconf_backend_perchannel_xml_create_channel(XfconfBackendPerchannelXml *xbpx,
                                                           const gchar *channel);
static GTree *xfconf_backend_perchannel_xml_load_channel(XfconfBackendPerchannelXml *xbpx,
                                                         const gchar *channel,
                                                         GError **error);
static gboolean xfconf_backend_perchannel_xml_flush_channel(XfconfBackendPerchannelXml *xbpx,
                                                            const gchar *channel,
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
    instance->channels = g_tree_new_full((GCompareDataFunc)g_ascii_strcasecmp,
                                         NULL,
                                         (GDestroyNotify)g_free,
                                         (GDestroyNotify)g_tree_destroy);
}

static void
xfconf_backend_perchannel_xml_finalize(GObject *obj)
{
    XfconfBackendPerchannelXml *xbpx = XFCONF_BACKEND_PERCHANNEL_XML(obj);
    
    if(xbpx->save_id) {
        g_source_remove(xbpx->save_id);
        xbpx->save_id = 0;
    }
    
    if(xbpx->dirty_channels)
        xfconf_backend_perchannel_xml_flush(XFCONF_BACKEND(xbpx), NULL);
    
    g_tree_destroy(xbpx->channels);
    g_free(xbpx->config_path);
    
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
            g_set_error(error, XFCONF_BACKEND_ERROR,
                        XFCONF_BACKEND_ERROR_WRITE_FAILURE,
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
    XfconfBackendPerchannelXml *xbpx = XFCONF_BACKEND_PERCHANNEL_XML(backend);
    GTree *properties = g_tree_lookup(xbpx->channels, channel);
    GValue *cur_val;
    
    if(!properties) {
        properties = xfconf_backend_perchannel_xml_load_channel(xbpx, channel,
                                                                NULL);
        if(!properties) {
            properties = xfconf_backend_perchannel_xml_create_channel(xbpx,
                                                                      channel);
        }
    }
    
    cur_val = g_tree_lookup(properties, property);
    if(cur_val)
        g_value_unset(cur_val);
    else {
        cur_val = g_new0(GValue, 1);
        g_tree_insert(properties, g_strdup(property), cur_val);
    }
    
    g_value_copy(value, g_value_init(cur_val, G_VALUE_TYPE(value)));
    xfconf_backend_perchannel_xml_schedule_save(xbpx, channel);
    
    return TRUE;
}

static gboolean
xfconf_backend_perchannel_xml_get(XfconfBackend *backend,
                                  const gchar *channel,
                                  const gchar *property,
                                  GValue *value,
                                  GError **error)
{
    XfconfBackendPerchannelXml *xbpx = XFCONF_BACKEND_PERCHANNEL_XML(backend);
    GTree *properties = g_tree_lookup(xbpx->channels, channel);
    GValue *cur_val;
    
    if(!properties) {
        properties = xfconf_backend_perchannel_xml_load_channel(xbpx, channel,
                                                                error);
        if(!properties)
            return FALSE;
    }
    
    cur_val = g_tree_lookup(properties, property);
    if(!cur_val) {
        if(error) {
            g_set_error(error, XFCONF_BACKEND_ERROR,
                        XFCONF_BACKEND_ERROR_PROPERTY_NOT_FOUND,
                        _("Property \"%s\" does not exist on channel \"%s\""),
                        property, channel);
        }
        return FALSE;
    }
    
    g_value_copy(cur_val, g_value_init(value, G_VALUE_TYPE(cur_val)));
    
    return TRUE;
}

static gboolean
tree_to_hash_table(gpointer key,
                   gpointer value,
                   gpointer data)
{
    GValue *value1 = g_new0(GValue, 1);
    
    g_value_copy(value, g_value_init(value1, G_VALUE_TYPE(value)));
    g_hash_table_insert((GHashTable *)data, g_strdup(key), value1);
    
    return FALSE;
}

static gboolean
xfconf_backend_perchannel_xml_get_all(XfconfBackend *backend,
                                      const gchar *channel,
                                      GHashTable *properties,
                                      GError **error)
{
    XfconfBackendPerchannelXml *xbpx = XFCONF_BACKEND_PERCHANNEL_XML(backend);
    GTree *properties1 = g_tree_lookup(xbpx->channels, channel);

    if(!properties1) {
        properties1 = xfconf_backend_perchannel_xml_load_channel(xbpx, channel,
                                                                 error);
        if(!properties1)
            return FALSE;
    }
    
    g_tree_foreach(properties1, tree_to_hash_table, properties);
    
    return TRUE;
}

static gboolean
xfconf_backend_perchannel_xml_exists(XfconfBackend *backend,
                                     const gchar *channel,
                                     const gchar *property,
                                     gboolean *exists,
                                     GError **error)
{
    XfconfBackendPerchannelXml *xbpx = XFCONF_BACKEND_PERCHANNEL_XML(backend);
    GTree *properties = g_tree_lookup(xbpx->channels, channel);
    
    if(!properties) {
        properties = xfconf_backend_perchannel_xml_load_channel(xbpx, channel,
                                                                NULL);
        if(!properties) {
            *exists = FALSE;
            return TRUE;
        }
    }
    
    if(g_tree_lookup(properties, property))
        *exists = TRUE;
    else
        *exists = FALSE;
    
    return TRUE;
}

static gboolean
xfconf_backend_perchannel_xml_remove(XfconfBackend *backend,
                                     const gchar *channel,
                                     const gchar *property,
                                     GError **error)
{
    XfconfBackendPerchannelXml *xbpx = XFCONF_BACKEND_PERCHANNEL_XML(backend);
    GTree *properties = g_tree_lookup(xbpx->channels, channel);
    
    if(!properties) {
        properties = xfconf_backend_perchannel_xml_load_channel(xbpx, channel,
                                                                error);
        if(!properties)
            return FALSE;
    }
    
    if(!g_tree_lookup(properties, property)) {
        if(error) {
            g_set_error(error, XFCONF_BACKEND_ERROR,
                        XFCONF_BACKEND_ERROR_PROPERTY_NOT_FOUND,
                        _("Property \"%s\" does not exist on channel \"%s\""),
                        property, channel);
        }
        return FALSE;
    }
    
    g_tree_remove(properties, property);
    xfconf_backend_perchannel_xml_schedule_save(xbpx, channel);
    
    return TRUE;
}

static gboolean
xfconf_backend_perchannel_xml_remove_channel(XfconfBackend *backend,
                                             const gchar *channel,
                                             GError **error)
{
    XfconfBackendPerchannelXml *xbpx = XFCONF_BACKEND_PERCHANNEL_XML(backend);
    gchar *filename;
    GList *dirty;
    
    if((dirty = g_list_find_custom(xbpx->dirty_channels, channel,
                                   (GCompareFunc)g_ascii_strcasecmp)))
    {
        xbpx->dirty_channels = g_list_remove(xbpx->dirty_channels, dirty);
        if(!xbpx->dirty_channels && xbpx->save_id) {
            g_source_remove(xbpx->save_id);
            xbpx->save_id = 0;
        }
    }
    
    g_tree_remove(xbpx->channels, channel);
    
    filename = g_strdup_printf("%s/%s.xml", xbpx->config_path, channel);
    if(unlink(filename)) {
        if(error) {
            g_set_error(error, XFCONF_BACKEND_ERROR,
                        XFCONF_BACKEND_ERROR_WRITE_FAILURE,
                        _("Unable to remove channel \"%s\": %s"),
                        channel, strerror(errno));
        }
        g_free(filename);
        return FALSE;
    }
    g_free(filename);
    
    return TRUE;
}

static gboolean
xfconf_backend_perchannel_xml_flush(XfconfBackend *backend,
                                    GError **error)
{
    XfconfBackendPerchannelXml *xbpx = XFCONF_BACKEND_PERCHANNEL_XML(backend);
    GList *l;
    
    for(l = xbpx->dirty_channels; l; l = l->next)
        xfconf_backend_perchannel_xml_flush_channel(xbpx, l->data, error);
    
    return TRUE;
}



static gboolean
xfconf_backend_perchannel_xml_save_timeout(gpointer data)
{
    XFCONF_BACKEND_PERCHANNEL_XML(data)->save_id = 0;
    xfconf_backend_perchannel_xml_flush(XFCONF_BACKEND(data), NULL);
    
    return FALSE;
}

static void
xfconf_backend_perchannel_xml_schedule_save(XfconfBackendPerchannelXml *xbpx,
                                            const gchar *channel)
{
    if(!g_list_find_custom(xbpx->dirty_channels, channel,
                           (GCompareFunc)g_ascii_strcasecmp))
    {
        gpointer orig_key = NULL, val = NULL;
        
        if(!g_tree_lookup_extended(xbpx->channels, channel, &orig_key, &val)) {
            g_warning("Attempt to schedule save for a nonexistent channel.");
            return;
        }
        
        xbpx->dirty_channels = g_list_prepend(xbpx->dirty_channels, orig_key);
    }
    
    if(xbpx->save_id)
        g_source_remove(xbpx->save_id);
    
    xbpx->save_id = g_timeout_add(WRITE_TIMEOUT,
                                  xfconf_backend_perchannel_xml_save_timeout,
                                  xbpx);
}

static GTree *
xfconf_backend_perchannel_xml_create_channel(XfconfBackendPerchannelXml *xbpx,
                                             const gchar *channel)
{
    GTree *properties;
    
    if((properties = g_tree_lookup(xbpx->channels, channel))) {
        g_warning("Attempt to create channel when one already exists.");
        return properties;
    }
    
    properties = g_tree_new_full((GCompareDataFunc)strcmp, NULL,
                                 (GDestroyNotify)g_free,
                                 (GDestroyNotify)xfconf_g_value_free);
    g_tree_insert(xbpx->channels, g_strdup(channel), properties);
    
    return properties;
}

static GTree *
xfconf_backend_perchannel_xml_load_channel(XfconfBackendPerchannelXml *xbpx,
                                           const gchar *channel,
                                           GError **error)
{
    GTree *properties = NULL;
    gchar *filename;
    
    filename = g_strdup_printf("%s/%s.xml", xbpx->config_path, channel);
    if(!g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
        if(error) {
            g_set_error(error, XFCONF_BACKEND_ERROR,
                        XFCONF_BACKEND_ERROR_CHANNEL_NOT_FOUND,
                        _("Channel \"%s\" does not exist"), channel);
        }
        g_free(filename);
        return NULL;
    }
    
//out:
    g_free(filename);
    
    return properties;
}

typedef struct
{
    FILE *fp;
    gboolean error_occurred;
} NodeWriterData;

static gboolean
tree_write_nodes(gpointer key,
                 gpointer value_p,
                 gpointer data)
{
    NodeWriterData *ndata = data;
    const gchar *property = key, *type_str = NULL;
    const GValue *value = value_p;
    gchar *value_str = NULL;
    
    /*
     * I know the suggested config value scheme implies some kind of hierarchical
     * list of properties, but I'm lazy, so we're just going to store them flat.
     */
    
    switch(G_VALUE_TYPE(value)) {
        case G_TYPE_STRING:
            value_str = g_markup_escape_text(g_value_get_string(value), -1);
            type_str = "string";
            break;
        
        case G_TYPE_INT:
            value_str = g_strdup_printf("%d", g_value_get_int(value));
            type_str = "int";
            break;
        
        case G_TYPE_INT64:
            value_str = g_strdup_printf("%" G_GINT64_FORMAT,
                                        g_value_get_int64(value));
            type_str = "int64";
            break;
        
        case G_TYPE_DOUBLE:
            value_str = g_strdup_printf("%f", g_value_get_double(value));
            type_str = "double";
            break;
        
        case G_TYPE_BOOLEAN:
            value_str = g_strdup(g_value_get_boolean(value) ? "true" : "false");
            type_str = "bool";
            break;
        
        default:
            if(G_VALUE_TYPE(value) == dbus_g_type_get_collection("GPtrArray",
                                                                 G_TYPE_STRING))
            {
                type_str = "strlist";
            } else {
                g_warning("Unknown value type %d (\"%s\"), skipping",
                          (int)G_VALUE_TYPE(value), G_VALUE_TYPE_NAME(value));
                ndata->error_occurred = TRUE;
                return TRUE;
            }
            break;
    }
    
    if(fprintf(ndata->fp, "  <property name=\"%s\" type=\"%s\"",
               property, type_str) < 0)
    {
        ndata->error_occurred = TRUE;
        goto out;
    }
    
    if(value_str) {
        if(fprintf(ndata->fp, " value=\"%s\"/>\n", value_str) < 0) {
            ndata->error_occurred = TRUE;
            goto out;
        }
    } else {
        GPtrArray *arr;
        gint i;
        
        if(fputs(">\n", ndata->fp) == EOF) {
            ndata->error_occurred = TRUE;
            goto out;
        }
        
        arr = g_value_get_boxed(value);
        for(i = 0; i < arr->len; ++i) {
            value_str = g_markup_escape_text(arr->pdata[i], -1);
            if(fprintf(ndata->fp, "    <string>%s</string\n", value_str) < 0) {
                ndata->error_occurred = TRUE;
                goto out;
            }
            g_free(value_str);
        }
        value_str = NULL;
        
        if(fputs("  </property>\n", ndata->fp) == EOF) {
            ndata->error_occurred = TRUE;
            goto out;
        }
    }
    
out:
    g_free(value_str);
    
    return ndata->error_occurred;
}
    

static gboolean
xfconf_backend_perchannel_xml_flush_channel(XfconfBackendPerchannelXml *xbpx,
                                            const gchar *channel,
                                            GError **error)
{
    gboolean ret = FALSE;
    GTree *properties = g_tree_lookup(xbpx->channels, channel);
    gchar *filename = NULL, *filename_tmp = NULL;
    FILE *fp = NULL;
    NodeWriterData ndata;
    
    if(!properties) {
        if(error) {
            g_set_error(error, XFCONF_BACKEND_ERROR,
                        XFCONF_BACKEND_ERROR_CHANNEL_NOT_FOUND,
                        _("Channel \"%s\" does not exist"), channel);
        }
        return FALSE;
    }
    
    filename = g_strdup_printf("%s/%s.xml", xbpx->config_path, channel);
    filename_tmp = g_strconcat(filename, ".new", NULL);
    
    fp = fopen(filename_tmp, "w");
    if(!fp)
        goto out;
    
    if(fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n", fp) == EOF
       || fprintf(fp, "<channel name=\"%s\" version=\"0.1\">\n", channel) < 0)
    {
        goto out;
    }
    
    ndata.fp = fp;
    ndata.error_occurred = FALSE;
    g_tree_foreach(properties, tree_write_nodes, &ndata);
    if(ndata.error_occurred)
        goto out;
    
    if(fputs("</channel>\n", fp) == EOF)
        goto out;
    
    if(fclose(fp)) {
        fp = NULL;
        goto out;
    }
    fp = NULL;
    
    if(rename(filename_tmp, filename))
        goto out;
    
    ret = TRUE;
    
out:
    if(!ret && error && !*error) {
        g_set_error(error, XFCONF_BACKEND_ERROR,
                    XFCONF_BACKEND_ERROR_WRITE_FAILURE,
                    _("Unable to write channel \"%s\": %s"),
                    channel, strerror(errno));
    }
    
    if(fp)
        fclose(fp);
    
    g_free(filename);
    g_free(filename_tmp);
    
    return ret;
}
