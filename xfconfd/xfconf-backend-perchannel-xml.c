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

#include <stdio.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <libxfce4util/libxfce4util.h>
#include <dbus/dbus-glib.h>

#include "xfconf-backend-perchannel-xml.h"
#include "xfconf-backend.h"
#include "xfconf-locking-utils.h"
#include "xfconf-gvaluefuncs.h"
#include "xfconf/xfconf-types.h"
#include "xfconf-common-private.h"

#define FILE_VERSION_MAJOR  "1"
#define FILE_VERSION_MINOR  "0"

#define PROP_NAME_IS_VALID(name) ( (name) && (name)[0] == '/' && (name)[1] != 0 && !strstr((name), "//") )

#define CONFIG_DIR_STEM  "xfce4/xfconf/" XFCONF_BACKEND_PERCHANNEL_XML_TYPE_ID "/"
#define CONFIG_FILE_FMT  CONFIG_DIR_STEM "%s.xml"
#define CACHE_TIMEOUT    (20*60*1000)  /* 20 minutes */
#define WRITE_TIMEOUT    (5*1000)  /* 5 secionds */
#define MAX_PROP_PATH    (4096)

struct _XfconfBackendPerchannelXml
{
    GObject parent;

    gchar *config_save_path;

    GTree *channels;

    guint save_id;
    GList *dirty_channels;

    XfconfPropertyChangedFunc prop_changed_func;
    gpointer prop_changed_data;
};

typedef struct _XfconfBackendPerchannelXmlClass
{
    GObjectClass parent;
} XfconfBackendPerchannelXmlClass;

typedef struct
{
    gchar *name;
    GValue value;
    gboolean locked;
} XfconfProperty;

typedef enum
{
    ELEM_NONE = 0,
    ELEM_CHANNEL,
    ELEM_PROPERTY,
    ELEM_VALUE,
} XmlParserElem;

/* FIXME: due to the hierarchical nature of the file, i need to use a
 * stack for list_property and list_value because  more than one array
 * property can be open at once.  the current xml file writer always
 * puts the <value> elements right after the opening <property>, but it's
 * possible someone could edit the file so that's not the case anymore. */
typedef struct
{
    XfconfBackendPerchannelXml *xbpx;
    GNode *properties;
    gboolean is_system_file;
    XmlParserElem cur_elem;
    gboolean channel_locked;
    gchar cur_path[MAX_PROP_PATH];
    gchar *list_property;
    GValue *list_value;
} XmlParserState;

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
                                                      const gchar *property_base,
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
                                                     gboolean recursive,
                                                     GError **error);
static gboolean xfconf_backend_perchannel_xml_flush(XfconfBackend *backend,
                                                    GError **error);
static void xfconf_backend_perchannel_xml_register_property_changed_func(XfconfBackend *backend,
                                                                         XfconfPropertyChangedFunc func,
                                                                         gpointer user_data);

static void xfconf_backend_perchannel_xml_schedule_save(XfconfBackendPerchannelXml *xbpx,
                                                        const gchar *channel);

static GNode *xfconf_backend_perchannel_xml_create_channel(XfconfBackendPerchannelXml *xbpx,
                                                           const gchar *channel);
static GNode *xfconf_backend_perchannel_xml_load_channel(XfconfBackendPerchannelXml *xbpx,
                                                         const gchar *channel,
                                                         GError **error);
static gboolean xfconf_backend_perchannel_xml_flush_channel(XfconfBackendPerchannelXml *xbpx,
                                                            const gchar *channel,
                                                            GError **error);

static GNode *xfconf_proptree_add_property(GNode *proptree,
                                           const gchar *name,
                                           const GValue *value,
                                           gboolean locked);
static XfconfProperty *xfconf_proptree_lookup(GNode *proptree,
                                              const gchar *name);
static GNode *xfconf_proptree_lookup_node(GNode *proptree,
                                          const gchar *name);
static gboolean xfconf_proptree_remove(GNode *proptree,
                                       const gchar *name);
static void xfconf_proptree_destroy(GNode *proptree);

static void xfconf_property_free(XfconfProperty *property);


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
                                         (GDestroyNotify)xfconf_proptree_destroy);
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

    g_free(xbpx->config_save_path);

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
    iface->flush = xfconf_backend_perchannel_xml_flush;
    iface->register_property_changed_func = xfconf_backend_perchannel_xml_register_property_changed_func;
}

static gboolean
xfconf_backend_perchannel_xml_initialize(XfconfBackend *backend,
                                         GError **error)
{
    XfconfBackendPerchannelXml *backend_px = XFCONF_BACKEND_PERCHANNEL_XML(backend);
    gchar *path = xfce_resource_save_location(XFCE_RESOURCE_CONFIG,
                                              CONFIG_DIR_STEM,
                                              TRUE);

    if(!path || !g_file_test(path, G_FILE_TEST_IS_DIR)) {
        if(error) {
            g_set_error(error, XFCONF_ERROR,
                        XFCONF_ERROR_WRITE_FAILURE,
                        _("Unable to create configuration directory"));
        }
        g_free(path);
        return FALSE;
    }

    backend_px->config_save_path = path;

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
    GNode *properties = g_tree_lookup(xbpx->channels, channel);
    XfconfProperty *cur_prop;

    if(!properties) {
        properties = xfconf_backend_perchannel_xml_load_channel(xbpx, channel,
#ifdef XFCONF_ENABLE_CHECKS
                                                                error);
#else
                                                                NULL);
#endif
        if(!properties) {
#ifdef XFCONF_ENABLE_CHECKS
            if(error && *error) {
                g_error_free(*error);
                *error = NULL;
            }
#endif
            properties = xfconf_backend_perchannel_xml_create_channel(xbpx,
                                                                      channel);
        }
    }

    cur_prop = xfconf_proptree_lookup(properties, property);
    if(cur_prop) {
        if(cur_prop->locked) {
            if(error) {
                g_set_error(error, XFCONF_ERROR,
                            XFCONF_ERROR_PERMISSION_DENIED,
                            _("You don't have permission to modify property \"%s\" on channel \"%s\""),
                            property, channel);
            }
            return FALSE;
        }

        if(G_IS_VALUE(&cur_prop->value))
            g_value_unset(&cur_prop->value);
        g_value_copy(value, g_value_init(&cur_prop->value,
                                         G_VALUE_TYPE(value)));

        /* FIXME: this will trigger if the value is replaced by the same
         * value */
        if(xbpx->prop_changed_func)
            xbpx->prop_changed_func(backend, channel, property, xbpx->prop_changed_data);
    } else {
        xfconf_proptree_add_property(properties, property, value, FALSE);
        if(xbpx->prop_changed_func)
            xbpx->prop_changed_func(backend, channel, property, xbpx->prop_changed_data);
    }

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
    GNode *properties = g_tree_lookup(xbpx->channels, channel);
    XfconfProperty *cur_prop;

    TRACE("entering");

    if(!properties) {
        properties = xfconf_backend_perchannel_xml_load_channel(xbpx, channel,
                                                                error);
        if(!properties)
            return FALSE;
    }

    cur_prop = xfconf_proptree_lookup(properties, property);
    if(!cur_prop || !G_IS_VALUE(&cur_prop->value)) {
        if(error) {
            g_set_error(error, XFCONF_ERROR,
                        XFCONF_ERROR_PROPERTY_NOT_FOUND,
                        _("Property \"%s\" does not exist on channel \"%s\""),
                        property, channel);
        }
        return FALSE;
    }

    g_value_copy(&cur_prop->value, g_value_init(value,
                                                G_VALUE_TYPE(&cur_prop->value)));

    return TRUE;
}

static void
xfconf_proptree_node_to_hash_table(GNode *node,
                                   GHashTable *props_hash,
                                   gchar cur_path[MAX_PROP_PATH])
{
    XfconfProperty *prop = node->data;

    if(G_VALUE_TYPE(&prop->value)) {
        GValue *value = g_new0(GValue, 1);

        g_value_copy(&prop->value, g_value_init(value,
                                                G_VALUE_TYPE(&prop->value)));
        g_hash_table_insert(props_hash,
                            g_strconcat(cur_path, "/", prop->name, NULL),
                            value);
    }

    if(node->children) {
        GNode *cur;
        gchar *p;

        g_strlcat(cur_path, "/", MAX_PROP_PATH);
        g_strlcat(cur_path, prop->name, MAX_PROP_PATH);

        for(cur = g_node_first_child(node);
            cur;
            cur = g_node_next_sibling(cur))
        {
            xfconf_proptree_node_to_hash_table(cur, props_hash, cur_path);
        }

        p = strrchr(cur_path, '/');
        if(p)
            *p = 0;
    }
}

static gboolean
xfconf_backend_perchannel_xml_get_all(XfconfBackend *backend,
                                      const gchar *channel,
                                      const gchar *property_base,
                                      GHashTable *properties,
                                      GError **error)
{
    XfconfBackendPerchannelXml *xbpx = XFCONF_BACKEND_PERCHANNEL_XML(backend);
    GNode *props_tree = g_tree_lookup(xbpx->channels, channel), *cur;
    gchar cur_path[MAX_PROP_PATH];

    if(!props_tree) {
        props_tree = xfconf_backend_perchannel_xml_load_channel(xbpx, channel,
                                                                error);
        if(!props_tree)
            return FALSE;
    }

    if(property_base[0] && property_base[1]) {
        /* it's not "" or "/" */
        cur = xfconf_proptree_lookup_node(props_tree, property_base);
        if(!cur) {
            if(error) {
                g_set_error(error, XFCONF_ERROR, XFCONF_ERROR_PROPERTY_NOT_FOUND,
                             _("Property \"%s\" does not exist on channel \"%s\""),
                             property_base, channel);
            }
            return FALSE;
        }

        g_strlcpy(cur_path, property_base, sizeof(cur_path));
        xfconf_proptree_node_to_hash_table(cur, properties, cur_path);
    } else {
        /* need to hit each child directly to avoid having a
         * double '/' at the beginning of each prop */
        for(cur = g_node_first_child(props_tree);
            cur;
            cur = g_node_next_sibling(props_tree))
        {
            cur_path[0] = 0;
            xfconf_proptree_node_to_hash_table(cur, properties, cur_path);
        }
    }


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
    GNode *properties = g_tree_lookup(xbpx->channels, channel);
    XfconfProperty *prop;

    if(!properties) {
        properties = xfconf_backend_perchannel_xml_load_channel(xbpx, channel,
#ifdef XFCONF_ENABLE_CHECKS
                                                                error);
#else
                                                                NULL);
#endif
        if(!properties) {
#ifdef XFCONF_ENABLE_CHECKS
            if(error && *error) {
                g_error_free(*error);
                *error = NULL;
            }
#endif

            *exists = FALSE;
            return TRUE;
        }
    }

    prop = xfconf_proptree_lookup(properties, property);
    *exists = (prop && G_VALUE_TYPE(&prop->value) ? TRUE : FALSE);

    return TRUE;
}

typedef struct
{
    XfconfBackend *backend;
    const gchar *channel;
} PropChangeData;

static void nodes_do_propchange_remove(GNode *node,
                                       gpointer data)
{
    XfconfBackendPerchannelXml *xbpx = XFCONF_BACKEND_PERCHANNEL_XML(data);
    PropChangeData *pdata = data;
    XfconfProperty *prop = node->data;

    if(!G_VALUE_TYPE(&prop->value))
        return;

    xbpx->prop_changed_func(pdata->backend, pdata->channel,
                            prop->name, xbpx->prop_changed_data);
}

static gboolean
do_remove_channel(XfconfBackend *backend,
                  const gchar *channel,
                  GNode *properties,
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

    if(xbpx->prop_changed_func) {
        PropChangeData pdata;

        pdata.backend = backend;
        pdata.channel = channel;
        g_node_children_foreach(properties, G_TRAVERSE_ALL,
                                nodes_do_propchange_remove, &pdata);
    }
    g_tree_remove(xbpx->channels, channel);

    filename = g_strdup_printf("%s/%s.xml", xbpx->config_save_path, channel);
    if(unlink(filename)) {
        if(error) {
            g_set_error(error, XFCONF_ERROR,
                        XFCONF_ERROR_WRITE_FAILURE,
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
xfconf_backend_perchannel_xml_remove(XfconfBackend *backend,
                                     const gchar *channel,
                                     const gchar *property,
                                     gboolean recursive,
                                     GError **error)
{
    XfconfBackendPerchannelXml *xbpx = XFCONF_BACKEND_PERCHANNEL_XML(backend);
    GNode *properties = g_tree_lookup(xbpx->channels, channel);

    if(!properties) {
        properties = xfconf_backend_perchannel_xml_load_channel(xbpx, channel,
                                                                error);
        if(!properties)
            return FALSE;
    }

    if(!recursive) {
        if(!xfconf_proptree_remove(properties, property)) {
            if(error) {
                g_set_error(error, XFCONF_ERROR,
                            XFCONF_ERROR_PROPERTY_NOT_FOUND,
                            _("Property \"%s\" does not exist on channel \"%s\""),
                            property, channel);
            }
            return FALSE;
        }

        if(xbpx->prop_changed_func)
            xbpx->prop_changed_func(backend, channel, property, xbpx->prop_changed_data);
    } else {
        GNode *top;
        XfconfProperty *prop;
        
        if(property[0] && property[1]) {
            PropChangeData pdata;

            /* it's not "" or "/" */
            top = xfconf_proptree_lookup_node(properties, property);
            if(!top) {
                if(error) {
                    g_set_error(error, XFCONF_ERROR,
                                XFCONF_ERROR_PROPERTY_NOT_FOUND,
                                _("Property \"%s\" does not exist on channel \"%s\""),
                                property, channel);
                }
                return FALSE;
            }

            g_node_unlink(top);
            prop = top->data;
            if(G_VALUE_TYPE(&prop->value) && xbpx->prop_changed_func)
                xbpx->prop_changed_func(backend, channel, property, xbpx->prop_changed_data);

            pdata.backend = backend;
            pdata.channel = channel;
            g_node_children_foreach(top, G_TRAVERSE_ALL,
                                    nodes_do_propchange_remove, &pdata);

            xfconf_proptree_destroy(top);
        } else {
            /* remove the entire channel */
            return do_remove_channel(backend, channel, properties, error);
        }
    }

    xfconf_backend_perchannel_xml_schedule_save(xbpx, channel);

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

    TRACE("exiting, flushed all channels");

    return TRUE;
}

static void
xfconf_backend_perchannel_xml_register_property_changed_func(XfconfBackend *backend,
                                                             XfconfPropertyChangedFunc func,
                                                             gpointer user_data)
{
    XfconfBackendPerchannelXml *xbpx = XFCONF_BACKEND_PERCHANNEL_XML(backend);

    xbpx->prop_changed_func = func;
    xbpx->prop_changed_data = user_data;
}



static GNode *
xfconf_proptree_lookup_node(GNode *proptree,
                            const gchar *name)
{
    GNode *found_node = NULL;
    gchar **parts;
    GNode *parent, *node;
    gint i;

    g_return_val_if_fail(PROP_NAME_IS_VALID(name), NULL);

    parts = g_strsplit_set(name+1, "/", -1);
    parent = proptree;

    for(i = 0; parts[i]; ++i) {
        for(node = g_node_first_child(parent);
            node;
            node = g_node_next_sibling(node))
        {
            if(!strcmp(((XfconfProperty *)node->data)->name, parts[i])) {
                if(!parts[i+1])
                    found_node = node;
                else
                    parent = node;
                break;
            }
        }

        if(found_node || !node)
            break;
    }

    g_strfreev(parts);

    return found_node;
}

static XfconfProperty *
xfconf_proptree_lookup(GNode *proptree,
                       const gchar *name)
{
    GNode *node;
    XfconfProperty *prop = NULL;

    node = xfconf_proptree_lookup_node(proptree, name);
    if(node)
        prop = node->data;

    return prop;
}

/* here we assume the entry does not already exist */
static GNode *
xfconf_proptree_add_property(GNode *proptree,
                             const gchar *name,
                             const GValue *value,
                             gboolean locked)
{
    GNode *parent = NULL;
    gchar tmp[MAX_PROP_PATH];
    gchar *p;
    XfconfProperty *prop;

    g_return_val_if_fail(PROP_NAME_IS_VALID(name), NULL);

    g_strlcpy(tmp, name, MAX_PROP_PATH);
    p = g_strrstr(tmp, "/");
    if(p == tmp)
        parent = proptree;
    else {
        *p = 0;
        parent = xfconf_proptree_lookup_node(proptree, tmp);
        if(!parent)
            parent = xfconf_proptree_add_property(proptree, tmp, NULL, FALSE);
    }

    prop = g_new0(XfconfProperty, 1);
    prop->name = g_strdup(strrchr(name, '/')+1);
    if(value) {
        g_value_init(&prop->value, G_VALUE_TYPE(value));
        g_value_copy(value, &prop->value);
    }
    prop->locked = locked;

    return g_node_append_data(parent, prop);
}

static gboolean
xfconf_proptree_remove(GNode *proptree,
                       const gchar *name)
{
    GNode *node = xfconf_proptree_lookup_node(proptree, name);

    if(node) {
        XfconfProperty *prop = node->data;

        if(G_IS_VALUE(&prop->value)) {
            if(node->children) {
                /* don't remove the children; just blank out the value */
                DBG("unsetting value at \"%s\"", prop->name);
                g_value_unset(&prop->value);
            } else {
                GNode *parent = node->parent;

                g_node_unlink(node);
                xfconf_proptree_destroy(node);

                /* remove parents without values until we find the root node or 
                 * a parent with a value or any children */
                while(parent) {
                    prop = parent->data;
                    if(!G_IS_VALUE(&prop->value) && !parent->children && strcmp(prop->name, "/") != 0) {
                        GNode *tmp = parent;
                        parent = parent->parent;

                        DBG("unlinking node at \"%s\"", prop->name);

                        g_node_unlink(tmp);
                        xfconf_proptree_destroy(tmp);
                    } else
                        parent = NULL;
                }
            }

            return TRUE;
        }
    }

    return FALSE;
}

static gboolean
proptree_free_node_data(GNode *node,
                        gpointer data)
{
    xfconf_property_free((XfconfProperty *)node->data);
    return FALSE;
}

static void
xfconf_proptree_destroy(GNode *proptree)
{
    if(G_LIKELY(proptree)) {
        g_node_traverse(proptree, G_IN_ORDER, G_TRAVERSE_ALL, -1,
                        proptree_free_node_data, NULL);
        g_node_destroy(proptree);
    }
}



static void
xfconf_property_free(XfconfProperty *property)
{
    g_free(property->name);
    if(G_IS_VALUE(&property->value))
        g_value_unset(&property->value);
    g_free(property);
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

static GNode *
xfconf_backend_perchannel_xml_create_channel(XfconfBackendPerchannelXml *xbpx,
                                             const gchar *channel)
{
    GNode *properties;
    XfconfProperty *prop;

    if((properties = g_tree_lookup(xbpx->channels, channel))) {
        g_warning("Attempt to create channel when one already exists.");
        return properties;
    }

    prop = g_new0(XfconfProperty, 1);
    prop->name = g_strdup("/");
    properties = g_node_new(prop);
    g_tree_insert(xbpx->channels, g_ascii_strdown(channel, -1), properties);

    return properties;
}

static void
xfconf_backend_perchannel_xml_start_elem(GMarkupParseContext *context,
                                         const gchar *element_name,
                                         const gchar **attribute_names,
                                         const gchar **attribute_values,
                                         gpointer user_data,
                                         GError **error)
{
    XmlParserState *state = user_data;
    gint i;
    const gchar *name = NULL, *type = NULL, *value = NULL;
    const gchar *version = NULL, *locked = NULL, *unlocked = NULL;
    gchar fullpath[MAX_PROP_PATH], *p;
    gint maj_ver_len;
    XfconfProperty *prop = NULL;

    switch(state->cur_elem) {
        case ELEM_NONE:
            if(strcmp(element_name, "channel")) {
                if(error) {
                    g_set_error(error, G_MARKUP_ERROR,
                                G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                                "Element <%s> not valid at top level",
                                element_name);
                }
                return;
            } else {
                for(i = 0; attribute_names[i]; ++i) {
                    if(!strcmp(attribute_names[i], "name"))
                        name = attribute_values[i];
                    else if(!strcmp(attribute_names[i], "version"))
                        version = attribute_values[i];
                    else if(!strcmp(attribute_names[i], "locked"))
                        locked = attribute_values[i];
                    else if(!strcmp(attribute_names[i], "unlocked"))
                        unlocked = attribute_values[i];
                    else {
                        if(error) {
                            g_set_error(error, G_MARKUP_ERROR,
                                        G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
                                        "Unknown attribute in <%s>: %s",
                                        element_name, attribute_names[i]);
                        }
                        return;
                    }
                }

                if(!name || !*name || !version || !*version) {
                    if(error) {
                        g_set_error(error, G_MARKUP_ERROR,
                                    G_MARKUP_ERROR_EMPTY,
                                    "Element <channel> requires both name and version attributes");
                    }
                    return;
                }

                /* compare versions */
                p = strstr(version, ".");
                maj_ver_len = p ? p - version : (gint) strlen(version);
                if(maj_ver_len != strlen(FILE_VERSION_MAJOR)
                   || strncmp(version, FILE_VERSION_MAJOR, maj_ver_len))
                {
                    if(error) {
                        g_set_error(error, G_MARKUP_ERROR,
                                    G_MARKUP_ERROR_INVALID_CONTENT,
                                    "On-disk file version %s is not compatible with our file version %s.%s",
                                    version, FILE_VERSION_MAJOR,
                                    FILE_VERSION_MINOR);
                    }
                    return;
                }

                if((locked && *locked) || (unlocked && *unlocked)) {
                    if(!state->is_system_file) {
                        if(error) {
                            g_set_error(error, G_MARKUP_ERROR,
                                        G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
                                        "Attribute \"locked\" not allowed in <%s> for non-system files",
                                        element_name);
                        }
                        return;
                    }

                    if(unlocked && *unlocked)
                        state->channel_locked = !xfconf_user_is_in_list(unlocked);
                    else if(locked && *locked)
                        state->channel_locked = xfconf_user_is_in_list(locked);

                    if(state->channel_locked) {
                        XfconfProperty *prop;

                        /* if this channel is locked now, we want to throw
                         * out all existing properties.  it's possible that
                         * there's more than one file that describes this
                         * channel in the various system config dirs, and this
                         * file here is not the first one we've examined.
                         * i'm not 100% sure this should be the correct
                         * behavior: another option would be to go back and
                         * mark all previously-read properties as locked,
                         * and just stop processing properties for this
                         * channel after this file is done.
                         */

                        xfconf_proptree_destroy(state->properties);

                        prop = g_new0(XfconfProperty, 1);
                        prop->name = g_strdup("/");
                        state->properties = g_node_new(prop);
                    }
                }

                state->cur_elem = ELEM_CHANNEL;
            }
            break;

        case ELEM_CHANNEL:
        case ELEM_PROPERTY:
            if(!strcmp(element_name, "property")) {
                GType value_type;

                for(i = 0; attribute_names[i]; ++i) {
                    if(!strcmp(attribute_names[i], "name"))
                        name = attribute_values[i];
                    else if(!strcmp(attribute_names[i], "type"))
                        type = attribute_values[i];
                    else if(!strcmp(attribute_names[i], "value"))
                        value = attribute_values[i];
                    else if(!strcmp(attribute_names[i], "locked"))
                        locked = attribute_values[i];
                    else if(!strcmp(attribute_names[i], "unlocked"))
                        unlocked = attribute_values[i];
                    else {
                        if(error) {
                            g_set_error(error, G_MARKUP_ERROR,
                                        G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
                                        "Unknown attribute in <%s>: %s",
                                        element_name, attribute_names[i]);
                        }
                        return;
                    }
                }

                if(!name || !*name || !type || !*type) {
                    if(error) {
                        g_set_error(error, G_MARKUP_ERROR,
                                    G_MARKUP_ERROR_EMPTY,
                                    "Element <property> requires both name and type attributes");
                    }
                    return;
                }

                /* FIXME: name validation! */
                g_strlcpy(fullpath, state->cur_path, MAX_PROP_PATH);
                g_strlcat(fullpath, "/", MAX_PROP_PATH);
                g_strlcat(fullpath, name, MAX_PROP_PATH);

                /* check if property is locked in a previous file */
                prop = xfconf_proptree_lookup(state->properties, fullpath);
                if(prop) {
                    if(prop->locked) {
                        /* we just want to skip over this property, but not
                         * throw an error */
                        state->cur_elem = ELEM_PROPERTY;
                        g_strlcpy(state->cur_path, fullpath, MAX_PROP_PATH);
                        return;
                    } else {
                        /* clear out the old data */
                        g_value_unset(&prop->value);
                    }
                } else {
                    GNode *prop_node = xfconf_proptree_add_property(state->properties,
                                                                    fullpath,
                                                                    NULL,
                                                                    FALSE);
                    prop = prop_node->data;
                }

                if(state->channel_locked)
                    prop->locked = TRUE;
                else if((locked && *locked) || (unlocked && *unlocked)) {
                    if(!state->is_system_file) {
                        if(error) {
                            g_set_error(error, G_MARKUP_ERROR,
                                        G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
                                        "Attribute \"locked\" not allowed in <%s> for non-system files",
                                        element_name);
                        }
                        xfconf_proptree_remove(state->properties, fullpath);
                        return;
                    }

                    if(unlocked && *unlocked)
                        prop->locked = !xfconf_user_is_in_list(unlocked);
                    else if(locked && *locked)
                        prop->locked = xfconf_user_is_in_list(locked);
                }

                /* parse types and values */
                value_type = _xfconf_gtype_from_string(type);
                if(G_TYPE_INVALID == value_type) {
                    if(error) {
                        g_set_error(error, G_MARKUP_ERROR,
                                    G_MARKUP_ERROR_INVALID_CONTENT,
                                    _("Invalid type for <%s>: \"%s\""),
                                    element_name, type);
                    }
                    return;
                }

                if(G_TYPE_NONE != value_type) {
                    g_value_init(&prop->value, value_type);
                    if(!_xfconf_gvalue_from_string(&prop->value, value)) {
                        if(error) {
                            g_set_error(error, G_MARKUP_ERROR,
                                        G_MARKUP_ERROR_INVALID_CONTENT,
                                        _("Unable to parse value of type \"%s\" from \"%s\""),
                                        g_type_name(value_type), value);
                        }
                        return;
                    }

                    if(XFCONF_TYPE_G_VALUE_ARRAY == value_type) {
                        /* FIXME: use stacks here */
                        state->list_property = g_strdup(fullpath);
                        state->list_value = &prop->value;
                    }

                    if(prop)
                        DBG("property '%s' has value type %s", fullpath, G_VALUE_TYPE_NAME(&prop->value));
                } else
                    DBG("empty property (branch)");

                g_strlcpy(state->cur_path, fullpath, MAX_PROP_PATH);
                state->cur_elem = ELEM_PROPERTY;
            } else if(ELEM_PROPERTY == state->cur_elem
                      && state->list_property  /* FIXME: use stack */
                      && state->list_value  /* FIXME: use stack */
                      && !strcmp(element_name, "value"))
            {
                GPtrArray *arr;
                GValue *val;
                GType value_type = G_TYPE_INVALID;

                for(i = 0; attribute_names[i]; ++i) {
                    if(!strcmp(attribute_names[i], "type"))
                        type = attribute_values[i];
                    else if(!strcmp(attribute_names[i], "value"))
                        value = attribute_values[i];
                    else {
                        if(error) {
                            g_set_error(error, G_MARKUP_ERROR,
                                        G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
                                        "Unknown attribute in <%s>: %s",
                                        element_name, attribute_names[i]);
                        }
                        return;
                    }
                }

                value_type = _xfconf_gtype_from_string(type);
                if(XFCONF_TYPE_G_VALUE_ARRAY == value_type) {
                    if(error) {
                        g_set_error(error, G_MARKUP_ERROR,
                                    G_MARKUP_ERROR_INVALID_CONTENT,
                                    _("The type attribute of <value> cannot be an array"));
                    }
                    return;
                } else if(G_TYPE_INVALID == value_type
                          || G_TYPE_NONE == value_type)
                {
                    if(error) {
                        g_set_error(error, G_MARKUP_ERROR,
                                    G_MARKUP_ERROR_INVALID_CONTENT,
                                    _("Invalid type for <%s>: \"%s\""),
                                    element_name, type);
                    }
                    return;
                }

                val = g_new0(GValue, 1);
                g_value_init(val, value_type);
                if(!_xfconf_gvalue_from_string(val, value)) {
                    if(error) {
                        g_set_error(error, G_MARKUP_ERROR,
                                    G_MARKUP_ERROR_INVALID_CONTENT,
                                    _("Unable to parse value of type \"%s\" from \"%s\""),
                                    g_type_name(value_type), value);
                    }
                    g_value_unset(val);
                    g_free(val);
                    return;
                }

                arr = g_value_get_boxed(state->list_value);
                g_ptr_array_add(arr, val);

                state->cur_elem = ELEM_VALUE;
            } else {
                if(error) {
                    g_set_error(error, G_MARKUP_ERROR,
                                G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                                "Element <%s> not allowed inside <%s>",
                                element_name, ELEM_CHANNEL == state->cur_elem
                                              ? "channel" : "property");
                }
                return;
            }
            break;

        case ELEM_VALUE:
            if(error) {
                g_set_error(error, G_MARKUP_ERROR,
                            G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                            "No other elements are allowed inside a <value> element.");
            }
            return;
    }
}

static void
xfconf_backend_perchannel_xml_end_elem(GMarkupParseContext *context,
                                       const gchar *element_name,
                                       gpointer user_data,
                                       GError **error)
{
    XmlParserState *state = user_data;
    gchar *p;

    switch(state->cur_elem) {
        case ELEM_CHANNEL:
            state->cur_elem = ELEM_NONE;
            state->cur_path[0] = 0;
            break;

        case ELEM_PROPERTY:
            /* FIXME: use stacks here */
            state->list_property = NULL;
            state->list_value = NULL;

            p = g_strrstr(state->cur_path, "/");

            if(p) {
                *p = 0;
                if(!*(state->cur_path))
                    state->cur_elem = ELEM_CHANNEL;
                else
                    state->cur_elem = ELEM_PROPERTY;
            } else {
                g_warning("Missing '/' char: I don't think this should happen");
                state->cur_elem = ELEM_CHANNEL;
            }
            break;

        case ELEM_VALUE:
            state->cur_elem = ELEM_PROPERTY;
            break;

        case ELEM_NONE:
            /* this really can't happen */
            break;
    }
}

#if 0
static inline gboolean
check_is_whitespace(const gchar *str,
                    gsize str_len)
{
    gint i;

    for(i = 0; i < str_len; ++i) {
        if(str[i] != ' ' && str[i] != '\t' && str[i] != '\r' && str[i] != '\n'
           && str[i] != 0)
        {
            return FALSE;
        }
    }

    return TRUE;
}

static void
xfconf_backend_perchannel_xml_text_elem(GMarkupParseContext *context,
                                        const gchar *text,
                                        gsize text_len,
                                        gpointer user_data,
                                        GError **error)
{
    XmlParserState *state = user_data;

    if(ELEM_VALUE != state->cur_elem) {
        /* check to make sure it's not just whitespace */
        if(check_is_whitespace(text, text_len))
            return;

        if(error) {
            g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                        "Content only allowed in <value> elements");
        }
        return;
    }

    if(!state->cur_text)
        state->cur_text = g_strndup(text, text_len);
    else {
        gint cur_len = strlen(state->cur_text);
        state->cur_text = g_realloc(state->cur_text, cur_len + text_len + 1);
        memcpy(state->cur_text + cur_len, text, text_len);
        state->cur_text[cur_len + text_len] = 0;
    }
}
#endif

static gboolean
xfconf_backend_perchannel_xml_merge_file(XfconfBackendPerchannelXml *xbpx,
                                         const gchar *filename,
                                         gboolean is_system_file,
                                         GNode **properties,
                                         gboolean *channel_locked,
                                         GError **error)
{
    gboolean ret = FALSE;
    gchar *file_contents = NULL;
    GMarkupParseContext *context = NULL;
    GMarkupParser parser = {
        xfconf_backend_perchannel_xml_start_elem,
        xfconf_backend_perchannel_xml_end_elem,
        /* xfconf_backend_perchannel_xml_text_elem, */
        NULL,
    };
    XmlParserState state;
    int fd = -1;
    struct stat st;
#ifdef HAVE_MMAP
    void *addr = NULL;
#endif

    TRACE("entering (%s)", filename);

    memset(&state, 0, sizeof(XmlParserState));
    state.properties = *properties;
    state.xbpx = xbpx;
    state.cur_elem = ELEM_NONE;
    state.channel_locked = FALSE;
    state.is_system_file = is_system_file;

    fd = open(filename, O_RDONLY, 0);
    if(fd < 0)
        goto out;

    if(fstat(fd, &st))
        goto out;

#ifdef HAVE_MMAP
    addr = mmap(NULL, st.st_size, PROT_READ, MAP_FILE | MAP_SHARED, fd, 0);
    if(addr != MAP_FAILED)
        file_contents = addr;
#endif

    if(!file_contents) {
        file_contents = g_malloc(st.st_size);
        if(read(fd, file_contents, st.st_size) != st.st_size)
            goto out;
    }

    context = g_markup_parse_context_new(&parser, 0, &state, NULL);
    if(!g_markup_parse_context_parse(context, file_contents, st.st_size, error)
       || !g_markup_parse_context_end_parse(context, error))
    {
        g_warning("Error parsing xfconf config file \"%s\": %s", filename,
                  error && *error ? (*error)->message : "(?)");
        goto out;
    }

    *channel_locked = state.channel_locked;
    ret = TRUE;

out:
    TRACE("exiting");

    *properties = state.properties;

    if(context)
        g_markup_parse_context_free(context);

#ifdef HAVE_MMAP
    if(addr) {
        munmap(addr, st.st_size);
        file_contents = NULL;
    }
#endif

    g_free(file_contents);

    if(fd >= 0)
        close(fd);

    return ret;
}

static GNode *
xfconf_backend_perchannel_xml_load_channel(XfconfBackendPerchannelXml *xbpx,
                                           const gchar *channel,
                                           GError **error)
{
    GNode *properties = NULL;
    gchar *filename_stem = NULL, **filenames = NULL;
    GList *system_files = NULL, *user_files = NULL, *l;
    gint i;
    XfconfProperty *prop;
    gboolean channel_locked = FALSE;

    TRACE("entering");

    filename_stem = g_strdup_printf(CONFIG_FILE_FMT, channel);
    filenames = xfce_resource_lookup_all(XFCE_RESOURCE_CONFIG, filename_stem);
    g_free(filename_stem);

    for(i = 0; filenames[i]; ++i) {
        if(!access(filenames[i], W_OK))  /* we can write, it's ours */
            user_files = g_list_append(user_files, filenames[i]);
        else if(!access(filenames[i], R_OK))  /* we can read, it's system */
            system_files = g_list_append(system_files, filenames[i]);
        else  /* we can't even read, so skip it */
            g_free(filenames[i]);
    }
    g_free(filenames);

    if(!system_files && !user_files) {
        if(error) {
            g_set_error(error, XFCONF_ERROR,
                        XFCONF_ERROR_CHANNEL_NOT_FOUND,
                        _("Channel \"%s\" does not exist"), channel);
        }
        goto out;
    }

    prop = g_new0(XfconfProperty, 1);
    prop->name = g_strdup("/");
    properties = g_node_new(prop);

    for(l = system_files; l && !channel_locked; l = l->next) {
        xfconf_backend_perchannel_xml_merge_file(xbpx, l->data, TRUE,
                                                 &properties, &channel_locked,
                                                 error);
    }
    for(l = user_files; l && !channel_locked; l = l->next) {
        xfconf_backend_perchannel_xml_merge_file(xbpx, l->data, FALSE,
                                                 &properties, &channel_locked,
                                                 error);
    }

    g_tree_insert(xbpx->channels, g_ascii_strdown(channel, -1), properties);

out:

    g_list_foreach(user_files, (GFunc)g_free, NULL);
    g_list_free(user_files);
    g_list_foreach(system_files, (GFunc)g_free, NULL);
    g_list_free(system_files);

    return properties;
}

static gboolean
xfconf_format_xml_tag(GString *elem_str,
                      GValue *value,
                      gboolean is_array_value,
                      gchar spaces[MAX_PROP_PATH],
                      gboolean *is_array)
{
    gchar *tmp;

    switch(G_VALUE_TYPE(value)) {
        case G_TYPE_STRING:
            tmp = g_markup_escape_text(g_value_get_string(value), -1);
            g_string_append_printf(elem_str, " type=\"string\" value=\"%s\"",
                                   tmp);
            g_free(tmp);
            break;

        case G_TYPE_UCHAR:
            g_string_append_printf(elem_str, " type=\"uchar\" value=\"%hhu\"",
                                   g_value_get_uchar(value));
            break;

        case G_TYPE_CHAR:
            g_string_append_printf(elem_str, " type=\"char\" value=\"%hhd\"",
                                   g_value_get_uchar(value));
            break;

        case G_TYPE_UINT:
            g_string_append_printf(elem_str, " type=\"uint\" value=\"%u\"",
                                   g_value_get_uint(value));
            break;

        case G_TYPE_INT:
            g_string_append_printf(elem_str, " type=\"int\" value=\"%d\"",
                                   g_value_get_int(value));
            break;

        case G_TYPE_UINT64:
            g_string_append_printf(elem_str, " type=\"uint64\" value=\"%" G_GUINT64_FORMAT "\"",
                                   g_value_get_uint64(value));
            break;

        case G_TYPE_INT64:
            g_string_append_printf(elem_str, " type=\"int64\" value=\"%" G_GINT64_FORMAT "\"",
                                   g_value_get_int64(value));
            break;

        case G_TYPE_FLOAT:
            g_string_append_printf(elem_str, " type=\"float\" value=\"%f\"",
                                   (gdouble)g_value_get_float(value));
            break;

        case G_TYPE_DOUBLE:
            g_string_append_printf(elem_str, " type=\"double\" value=\"%f\"",
                                   g_value_get_double(value));
            break;

        case G_TYPE_BOOLEAN:
            g_string_append_printf(elem_str, " type=\"bool\" value=\"%s\"",
                                   g_value_get_boolean(value) ? "true" : "false");
            break;

        default:
            if(G_VALUE_TYPE(value) == G_TYPE_STRV) {
                gchar **strlist;
                gint i;

                /* we shouldn't get here anymore, i think */
                g_critical("Got G_TYPE_STRV.  Shouldn't happen anymore, right?");

                if(is_array_value)
                    return FALSE;

                g_string_append(elem_str, " type=\"array\">\n");

                strlist = g_value_get_boxed(value);
                for(i = 0; strlist[i]; ++i) {
                    gchar *value_str = g_markup_escape_text(strlist[i], -1);
                    g_string_append_printf(elem_str,
                                           "%s  <value type=\"string\" value=\"%s\"/>\n",
                                           spaces, value_str);
                    g_free(value_str);
                }

                *is_array = TRUE;
            } else if(XFCONF_TYPE_G_VALUE_ARRAY == G_VALUE_TYPE(value)) {
                GPtrArray *arr;
                guint i;
                
                if(is_array_value)
                    return FALSE;

                g_string_append(elem_str, " type=\"array\">\n");

                arr = g_value_get_boxed(value);
                for(i = 0; i < arr->len; ++i) {
                    GValue *value1 = g_ptr_array_index(arr, i);
                    gboolean dummy;

                    g_string_append_printf(elem_str, "%s  <value ", spaces);
                    if(!xfconf_format_xml_tag(elem_str, value1, TRUE, spaces,
                                              &dummy))
                    {
                        return FALSE;
                    }
                    g_string_append(elem_str, "/>\n");
                }

                *is_array = TRUE;
            } else {
                if(is_array_value)
                    return FALSE;

                if(G_VALUE_TYPE(value) != 0) {
                    g_warning("Unknown value type %d (\"%s\"), treating as branch",
                              (int)G_VALUE_TYPE(value), G_VALUE_TYPE_NAME(value));
                    g_value_unset(value);
                }

                is_array = FALSE;
                g_string_append(elem_str, " type=\"empty\"");
            }
            break;
    }

    return TRUE;
}

static gboolean
xfconf_backend_perchannel_xml_write_node(XfconfBackendPerchannelXml *xbpx,
                                         FILE *fp,
                                         GNode *node,
                                         gint depth,
                                         GError **error)
{
    XfconfProperty *prop = node->data;
    GValue *value = &prop->value;
    GString *elem_str;
    gchar spaces[MAX_PROP_PATH];
    GNode *child;
    gchar *escaped_name;
    gboolean is_array = FALSE;
    
    if(depth * 2 > (gint) sizeof(spaces) + 1)
        depth = sizeof(spaces) / 2 - 1;

    memset(spaces, ' ', depth * 2);
    spaces[depth * 2] = 0;

    escaped_name = g_markup_escape_text(prop->name, strlen(prop->name));

    elem_str = g_string_sized_new(128);
    g_string_append_printf(elem_str, "%s<property name=\"%s\"", spaces,
                           escaped_name);

    g_free (escaped_name);

    if(!xfconf_format_xml_tag(elem_str, value, FALSE, spaces, &is_array)) {
        /* _flush_channel() will handle |error| */
        g_string_free(elem_str, TRUE);
        return FALSE;
    }

    child = g_node_first_child(node);
    if(!is_array) {
        if(child)
            g_string_append(elem_str, ">\n");
        else
            g_string_append(elem_str, "/>\n");
    }

    if(fputs(elem_str->str, fp) == EOF) {
        /* _flush_channel() will handle |error| */
        g_string_free(elem_str, TRUE);
        return FALSE;
    }
    g_string_free(elem_str, TRUE);

    for(; child; child = g_node_next_sibling(child)) {
        if(!xfconf_backend_perchannel_xml_write_node(xbpx, fp, child,
                                                     depth + 1, error))
        {
            /* _flush_channel() will handle |error| */
            return FALSE;
        }
    }

    if(is_array || g_node_first_child(node)) {
        if(fputs(spaces, fp) == EOF || fputs("</property>\n", fp) == EOF) {
            /* _flush_channel() will handle |error| */
            return FALSE;
        }
    }

    return TRUE;
}

static gboolean
xfconf_backend_perchannel_xml_flush_channel(XfconfBackendPerchannelXml *xbpx,
                                            const gchar *channel,
                                            GError **error)
{
    gboolean ret = FALSE;
    GNode *properties = g_tree_lookup(xbpx->channels, channel), *child;
    gchar *filename = NULL, *filename_tmp = NULL;
    FILE *fp = NULL;

    if(!properties) {
        if(error) {
            g_set_error(error, XFCONF_ERROR,
                        XFCONF_ERROR_CHANNEL_NOT_FOUND,
                        _("Channel \"%s\" does not exist"), channel);
        }
        return FALSE;
    }

    filename = g_strdup_printf("%s/%s.xml", xbpx->config_save_path, channel);
    filename_tmp = g_strconcat(filename, ".new", NULL);

    fp = fopen(filename_tmp, "w");
    if(!fp)
        goto out;

    if(fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n", fp) == EOF
       || fprintf(fp, "<channel name=\"%s\" version=\"%s.%s\">\n", channel,
                  FILE_VERSION_MAJOR, FILE_VERSION_MINOR) < 0)
    {
        goto out;
    }

    for(child = g_node_first_child(properties);
        child;
        child = g_node_next_sibling(child))
    {
        if(!xfconf_backend_perchannel_xml_write_node(xbpx, fp, child, 1, error))
            goto out;
    }

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
        g_set_error(error, XFCONF_ERROR,
                    XFCONF_ERROR_WRITE_FAILURE,
                    _("Unable to write channel \"%s\": %s"),
                    channel, strerror(errno));
    }

    if(fp)
        fclose(fp);

    g_free(filename);
    g_free(filename_tmp);

    return ret;
}
