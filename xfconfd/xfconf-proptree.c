/*
 *  xfconf
 *
 *  Copyright (c) 2007-2008 Brian Tarricone <bjt23@cornell.edu>
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

#include <libxfce4util/libxfce4util.h>

#include "xfconf-proptree.h"

GNode *
xfconf_proptree_lookup_node(GNode *proptree,
                            const gchar *name)
{
    GNode *found_node = NULL;
    gchar **parts;
    GNode *parent, *node;
    gint i;

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

XfconfProperty *
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
GNode *
xfconf_proptree_add_property(GNode *proptree,
                             const gchar *name,
                             const GValue *value,
                             gboolean locked)
{
    GNode *parent = NULL;
    gchar tmp[MAX_PROP_PATH];
    gchar *p;
    XfconfProperty *prop;

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

    prop = g_slice_new0(XfconfProperty);
    prop->name = g_strdup(strrchr(name, '/')+1);
    if(value) {
        g_value_init(&prop->value, G_VALUE_TYPE(value));
        g_value_copy(value, &prop->value);
    }
    prop->locked = locked;

    return g_node_append_data(parent, prop);
}

gboolean
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

gboolean
proptree_free_node_data(GNode *node,
                        gpointer data)
{
    xfconf_property_free((XfconfProperty *)node->data);
    return FALSE;
}

void
xfconf_proptree_destroy(GNode *proptree)
{
    if(G_LIKELY(proptree)) {
        g_node_traverse(proptree, G_IN_ORDER, G_TRAVERSE_ALL, -1,
                        proptree_free_node_data, NULL);
        g_node_destroy(proptree);
    }
}



void
xfconf_property_free(XfconfProperty *property)
{
    g_free(property->name);
    if(G_IS_VALUE(&property->value))
        g_value_unset(&property->value);
    g_slice_free(XfconfProperty, property);
}


