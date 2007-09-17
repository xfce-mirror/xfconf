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
#include "xfconf-util.h"

#define FILE_VERSION_MAJOR  "1"
#define FILE_VERSION_MINOR  "0"

#define CONFIG_DIR_STEM  "xfce4/xfconf/" XFCONF_BACKEND_PERCHANNEL_XML_TYPE_ID "/"
#define CONFIG_FILE_FMT  CONFIG_DIR_STEM "%s.xml"
#define CACHE_TIMEOUT    (20*60*1000)  /* 20 minutes */
#define WRITE_TIMEOUT    (5*1000)  /* 5 secionds */

#define MAX_PREF_PATH  4096

struct _XfconfBackendPerchannelXml
{
    GObject parent;
    
    gchar *config_save_path;
    
    GTree *channels;
    
    guint save_id;
    GList *dirty_channels;
};

typedef struct _XfconfBackendPerchannelXmlClass
{
    GObjectClass parent;
} XfconfBackendPerchannelXmlClass;

typedef struct
{
    GValue *value;
    gboolean locked;
} XfconfProperty;

typedef enum
{
    ELEM_NONE = 0,
    ELEM_CHANNEL,
    ELEM_PROPERTY,
    ELEM_STRING,
} XmlParserElem;

/* FIXME: due to the hierarchical nature of the file, i need to use a
 * stack for strlist_property and strlist_value because a more than one
 * strlist property can be open at once */
typedef struct
{
    XfconfBackendPerchannelXml *xbpx;
    GTree *properties;
    gboolean is_system_file;
    XmlParserElem cur_elem;
    gchar *channel_name;
    gboolean channel_locked;
    gchar cur_path[MAX_PREF_PATH];
    gchar *cur_text;
    gchar *strlist_property;
    GValue *strlist_value;
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
    iface->remove_channel = xfconf_backend_perchannel_xml_remove_channel;
    iface->flush = xfconf_backend_perchannel_xml_flush;
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
            g_set_error(error, XFCONF_BACKEND_ERROR,
                        XFCONF_BACKEND_ERROR_WRITE_FAILURE,
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
    GTree *properties = g_tree_lookup(xbpx->channels, channel);
    XfconfProperty *cur_prop;
    
    if(!properties) {
        properties = xfconf_backend_perchannel_xml_load_channel(xbpx, channel,
                                                                NULL);
        if(!properties) {
            properties = xfconf_backend_perchannel_xml_create_channel(xbpx,
                                                                      channel);
        }
    }
    
    cur_prop = g_tree_lookup(properties, property);
    if(cur_prop) {
        if(G_IS_VALUE(cur_prop->value))
            g_value_unset(cur_prop->value);
    } else {
        cur_prop = g_new0(XfconfProperty, 1);
        cur_prop->value = g_new0(GValue, 1);
        g_tree_insert(properties, g_ascii_strdown(property, -1), cur_prop);
    }
    
    g_value_copy(value, g_value_init(cur_prop->value, G_VALUE_TYPE(value)));
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
    XfconfProperty *cur_prop;
    
    TRACE("entering");
    
    if(!properties) {
        properties = xfconf_backend_perchannel_xml_load_channel(xbpx, channel,
                                                                error);
        if(!properties)
            return FALSE;
    }
    
    cur_prop = g_tree_lookup(properties, property);
    DBG("cur_prop:%p, cur_prop->value:%p, G_IS_VALUE:%d", cur_prop, cur_prop ? cur_prop->value : NULL, cur_prop && cur_prop->value ? G_IS_VALUE(cur_prop->value) : 0);
    if(!cur_prop || !cur_prop->value || !G_IS_VALUE(cur_prop->value)) {
        if(error) {
            g_set_error(error, XFCONF_BACKEND_ERROR,
                        XFCONF_BACKEND_ERROR_PROPERTY_NOT_FOUND,
                        _("Property \"%s\" does not exist on channel \"%s\""),
                        property, channel);
        }
        return FALSE;
    }
    
    g_value_copy(cur_prop->value, g_value_init(value,
                                               G_VALUE_TYPE(cur_prop->value)));
    
    return TRUE;
}

static gboolean
tree_to_hash_table(gpointer key,
                   gpointer value,
                   gpointer data)
{
    GValue *value1;
    XfconfProperty *prop = value;
    
    if(G_IS_VALUE(prop->value)) {
        value1 = g_new0(GValue, 1);
        g_value_copy(prop->value,
                     g_value_init(value1, G_VALUE_TYPE(prop->value)));
        g_hash_table_insert((GHashTable *)data, g_strdup(key), value1);
    }
    
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
    
    filename = g_strdup_printf("%s/%s.xml", xbpx->config_save_path, channel);
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



static void
xfconf_property_free(XfconfProperty *property)
{
    if(G_IS_VALUE(property->value))
        g_value_unset(property->value);
    g_free(property->value);
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

static GTree *
xfconf_backend_perchannel_xml_create_channel(XfconfBackendPerchannelXml *xbpx,
                                             const gchar *channel)
{
    GTree *properties;
    
    if((properties = g_tree_lookup(xbpx->channels, channel))) {
        g_warning("Attempt to create channel when one already exists.");
        return properties;
    }
    
    properties = g_tree_new_full((GCompareDataFunc)g_ascii_strcasecmp, NULL,
                                 (GDestroyNotify)g_free,
                                 (GDestroyNotify)xfconf_property_free);
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
    gchar fullpath[MAX_PREF_PATH], *p;
    gint maj_ver_len;
    XfconfProperty *prop = NULL;
    
    TRACE("entering (%s)", element_name);
    
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
                maj_ver_len = p ? p - version : strlen(version);
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
                
                /* FIXME: validate name for valid chars */
                state->channel_name = g_strdup(name);
                
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
                        
                        g_tree_destroy(state->properties);
                        state->properties = g_tree_new_full((GCompareDataFunc)g_ascii_strcasecmp,
                                                            NULL,
                                                            (GDestroyNotify)g_free,
                                                            (GDestroyNotify)xfconf_property_free);
                    }
                }
                
                state->cur_elem = ELEM_CHANNEL;
            }
            break;
        
        case ELEM_CHANNEL:
        case ELEM_PROPERTY:
            if(!strcmp(element_name, "property")) {
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
                g_strlcpy(fullpath, state->cur_path, MAX_PREF_PATH);
                g_strlcat(fullpath, "/", MAX_PREF_PATH);
                g_strlcat(fullpath, name, MAX_PREF_PATH);
                
                /* check if property is locked in a previous file */
                prop = g_tree_lookup(state->properties, fullpath);
                if(prop) {
                    if(prop->locked) {
                        /* we just want to skip over this property, but not
                         * throw an error */
                        state->cur_elem = ELEM_PROPERTY;
                        g_strlcpy(state->cur_path, fullpath, MAX_PREF_PATH);
                        return;
                    } else {
                        /* clear out the old data */
                        g_value_unset(prop->value);
                    }
                } else {
                    prop = g_new0(XfconfProperty, 1);
                    prop->value = g_new0(GValue, 1);
                    g_tree_insert(state->properties, g_strdup(fullpath), prop);
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
                        g_tree_remove(state->properties, fullpath);
                        return;
                    }
                    
                    if(unlocked && *unlocked)
                        prop->locked = !xfconf_user_is_in_list(unlocked);
                    else if(locked && *locked)
                        prop->locked = xfconf_user_is_in_list(locked);
                }
                
                /* parse types and values */
                if(!strcmp(type, "string")) {
                    g_value_init(prop->value, G_TYPE_STRING);
                    g_value_set_string(prop->value, value);
                } else if(!strcmp(type, "strlist")) {
                    GPtrArray *arr = g_ptr_array_new();
                    g_value_init(prop->value,
                                 dbus_g_type_get_collection("GPtrArray",
                                                            G_TYPE_STRING));
                    g_value_set_boxed(prop->value, arr);
                    /* FIXME: use stacks here */
                    state->strlist_property = g_strdup(fullpath);
                    state->strlist_value = prop->value;
                } else if(!strcmp(type, "int")) {
                    g_value_init(prop->value, G_TYPE_INT);
                    g_value_set_int(prop->value, atoi(value));
                } else if(!strcmp(type, "int64")) {
                    g_value_init(prop->value, G_TYPE_INT64);
                    g_value_set_int64(prop->value, g_ascii_strtoll(value, NULL,
                                                                   0));
                } else if(!strcmp(type, "double")) {
                    g_value_init(prop->value, G_TYPE_DOUBLE);
                    g_value_set_double(prop->value, g_ascii_strtod(value,
                                                                   NULL));
                } else if(!strcmp(type, "bool")) {
                    g_value_init(prop->value, G_TYPE_BOOLEAN);
                    if(!g_ascii_strcasecmp(value, "true"))
                        g_value_set_boolean(prop->value, TRUE);
                    else
                        g_value_set_boolean(prop->value, FALSE);
                } else {
                    g_tree_remove(state->properties, fullpath);
                    
                    if(strcmp(type, "empty")) {
                        if(error) {
                            g_set_error(error, G_MARKUP_ERROR,
                                        G_MARKUP_ERROR_INVALID_CONTENT,
                                        "Invalid property type \"%s\"", type);
                        }
                        return;
                    }
                }
                
                g_strlcpy(state->cur_path, fullpath, MAX_PREF_PATH);
                state->cur_elem = ELEM_PROPERTY;
            } else if(ELEM_PROPERTY == state->cur_elem
                      && state->strlist_property  /* FIXME: use stack */
                      && state->strlist_value  /* FIXME: use stack */
                      && !strcmp(element_name, "string"))
            {
                state->cur_elem = ELEM_STRING;
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
        
        case ELEM_STRING:
            if(error) {
                g_set_error(error, G_MARKUP_ERROR,
                            G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                            "No other elements are allowed inside a string element.");
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
    GPtrArray *arr;
    
    TRACE("entering (%s)", element_name);
    
    switch(state->cur_elem) {
        case ELEM_CHANNEL:
            state->cur_elem = ELEM_NONE;
            break;
        
        case ELEM_PROPERTY:
            /* FIXME: use stacks here */
            state->strlist_property = NULL;
            state->strlist_value = NULL;
            
            DBG("closing property elem, cur_path is \"%s\"", state->cur_path);
            
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
        
        case ELEM_STRING:
            /* FIXME: use stack */
            arr = g_value_get_boxed(state->strlist_value);
            g_ptr_array_add(arr, state->cur_text);
            state->cur_text = NULL;
            break;
        
        case ELEM_NONE:
            /* this really can't happen */
            break;
    }
}

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
    
    TRACE("entering");
    
    if(ELEM_STRING != state->cur_elem) {
        /* check to make sure it's not just whitespace */
        if(check_is_whitespace(text, text_len))
            return;
        
        if(error) {
            g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                        "Content only allowed in <string> elements");
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

static gboolean
xfconf_backend_perchannel_xml_merge_file(XfconfBackendPerchannelXml *xbpx,
                                         const gchar *filename,
                                         GTree **properties)
{
    gboolean ret = FALSE;
    gchar *file_contents = NULL;
    GMarkupParseContext *context = NULL;
    GMarkupParser parser = {
        xfconf_backend_perchannel_xml_start_elem,
        xfconf_backend_perchannel_xml_end_elem,
        xfconf_backend_perchannel_xml_text_elem,
        NULL,
    };
    XmlParserState state;
    int fd = -1;
    struct stat st;
    GError *error = NULL;
#ifdef HAVE_MMAP
    void *addr = NULL;
#endif
    
    TRACE("entering (%s)", filename);
    
    state.properties = *properties;
    
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
    
    state.xbpx = xbpx;
    state.cur_elem = ELEM_NONE;
    state.channel_name = NULL;
    memset(state.cur_path, 0, MAX_PREF_PATH);
    
    context = g_markup_parse_context_new(&parser, 0, &state, NULL);
    if(!g_markup_parse_context_parse(context, file_contents, st.st_size, &error)
       || !g_markup_parse_context_end_parse(context, &error))
    {
        g_warning("Error parsing xfconf config file \"%s\": %s", filename,
                  error->message);
        g_error_free(error);
        goto out;
    }
    
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

static GTree *
xfconf_backend_perchannel_xml_load_channel(XfconfBackendPerchannelXml *xbpx,
                                           const gchar *channel,
                                           GError **error)
{
    GTree *properties = NULL;
    gchar *filename_stem = NULL, **filenames = NULL;
    GList *system_files = NULL, *user_files = NULL, *l;
    gint i;
    
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
            g_set_error(error, XFCONF_BACKEND_ERROR,
                        XFCONF_BACKEND_ERROR_CHANNEL_NOT_FOUND,
                        _("Channel \"%s\" does not exist"), channel);
        }
        goto out;
    }
    
    properties = g_tree_new_full((GCompareDataFunc)g_ascii_strcasecmp,
                                 NULL,
                                 (GDestroyNotify)g_free,
                                 (GDestroyNotify)xfconf_property_free);
    
    /* FIXME: handle locking */
    for(l = system_files; l; l = l->next)
        xfconf_backend_perchannel_xml_merge_file(xbpx, l->data, &properties);
    for(l = user_files; l; l = l->next)
        xfconf_backend_perchannel_xml_merge_file(xbpx, l->data, &properties);
    
    g_tree_insert(xbpx->channels, g_ascii_strdown(channel, -1), properties);
    
out:
    
    g_list_foreach(user_files, (GFunc)g_free, NULL);
    g_list_free(user_files);
    g_list_foreach(system_files, (GFunc)g_free, NULL);
    g_list_free(user_files);
    
    return properties;
}

typedef struct
{
    FILE *fp;
    gboolean error_occurred;
    gchar cur_path[MAX_PREF_PATH];
} NodeWriterData;

static inline gint
count_slashes(const gchar *str,
              gchar *leading,
              gsize leading_len)
{
    gint slashes = 0;
    gchar *p = (gchar *)str;
    
    while(*p) {
        if('/' == *p)
            ++slashes;
        ++p;
    }
    
    if(slashes * 2 > leading_len - 1)
        slashes = (leading_len - 1) / 2;
    memset(leading, ' ', slashes * 2);
    leading[slashes * 2] = 0;
    
    return slashes;
}

static gboolean
tree_write_nodes(gpointer key,
                 gpointer value_p,
                 gpointer data)
{
    NodeWriterData *ndata = data;
    const gchar *property = key, *type_str = NULL;
    const GValue *value = value_p;
    gchar *value_str = NULL, *p, *short_prop_name;
    gint n_tabs;
    gchar leading[128];
    
    /* this is less than ideal.  there's no easy way to 'peek' at the next
     * value in the tree to know if there are sub-properties or not, so we
     * defer closing the current property tag until we know if there are
     * are sub-properties or not. */
    
    n_tabs = count_slashes(ndata->cur_path, leading, sizeof(leading));
    
    if(!g_str_has_prefix(property, ndata->cur_path)) {
        /* new property is not a sub-property of the last property.  first
         * close as many previous properties as needed */
        while((p = g_strrstr(ndata->cur_path, "/"))
              && !g_str_has_prefix(property, ndata->cur_path))
        {
            DBG("cur_path:%s", ndata->cur_path);
            fprintf(ndata->fp, "%s</property>\n", leading);
            leading[n_tabs * 2 - 1] = 0;
            leading[n_tabs * 2 - 2] = 0;
            --n_tabs;
            *p = 0;
        }
        
        fprintf(ndata->fp, "%s</property>\n", leading);
        leading[n_tabs * 2 - 1] = 0;
        leading[n_tabs * 2 - 2] = 0;
        --n_tabs;
        p = g_strrstr(ndata->cur_path, "/");
        if(p)
            *p = 0;
    }
        
    /* open new branches if needed */
    for(p = (gchar *)property + strlen(ndata->cur_path); p && *p;) {
        gchar *q = strstr(p+1, "/");
        
        DBG("p:%s, q:%s", p, q);
        
        if(q) {
            ++n_tabs;
            leading[n_tabs * 2 - 2] = ' ';
            leading[n_tabs * 2 - 1] = ' ';
            leading[n_tabs * 2] = 0;
            
            fputs(leading, ndata->fp);
            fputs("<property name=\"", ndata->fp);
            fwrite(p+1, 1, q-p-1, ndata->fp);
            fputs("\" type=\"empty\">\n", ndata->fp);
        }
        
        p = q;
    }
    
    g_strlcpy(ndata->cur_path, property, MAX_PREF_PATH);
    n_tabs = count_slashes(property, leading, sizeof(leading));
    short_prop_name = g_strrstr(property, "/");
    g_assert(short_prop_name);
    ++short_prop_name;
    
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
    
    if(fprintf(ndata->fp, "%s<property name=\"%s\" type=\"%s\"",
               leading, short_prop_name, type_str) < 0)
    {
        ndata->error_occurred = TRUE;
        goto out;
    }
    
    if(value_str) {
        if(fprintf(ndata->fp, " value=\"%s\">\n", value_str) < 0) {
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
            if(fprintf(ndata->fp, "%s  <string>%s</string>\n",
                       leading, value_str) < 0)
            {
                ndata->error_occurred = TRUE;
                goto out;
            }
            g_free(value_str);
        }
        value_str = NULL;
        
        /*
        if(fprintf(ndata->fp, "%s</property>\n", leading) == EOF) {
            ndata->error_occurred = TRUE;
            goto out;
        }
        */
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
    gchar *filename = NULL, *filename_tmp = NULL, leading[128], *p;
    FILE *fp = NULL;
    NodeWriterData ndata;
    gint n_tabs;
    
    if(!properties) {
        if(error) {
            g_set_error(error, XFCONF_BACKEND_ERROR,
                        XFCONF_BACKEND_ERROR_CHANNEL_NOT_FOUND,
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
    
    ndata.fp = fp;
    ndata.error_occurred = FALSE;
    ndata.cur_path[0] = 0;
    g_tree_foreach(properties, tree_write_nodes, &ndata);
    if(ndata.error_occurred)
        goto out;
    
    /* close any open <property> tags */
    n_tabs = count_slashes(ndata.cur_path, leading, sizeof(leading));
    while((p = g_strrstr(ndata.cur_path, "/"))) {
        fprintf(fp, "%s</property>\n", leading);
        leading[n_tabs * 2 - 1] = 0;
        leading[n_tabs * 2 - 2] = 0;
        --n_tabs;
        *p = 0;
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
