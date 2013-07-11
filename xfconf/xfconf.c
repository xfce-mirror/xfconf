/*
 *  xfconf
 *
 *  Copyright (c) 2007 Brian Tarricone <bjt23@cornell.edu>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; version 2
 *  of the License ONLY.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <glib-object.h>

#include <dbus/dbus-glib.h>

#include "xfconf.h"
#include "common/xfconf-marshal.h"
#include "xfconf-private.h"
#include "common/xfconf-alias.h"

static guint xfconf_refcnt = 0;
static DBusGConnection *dbus_conn = NULL;
static DBusGProxy *dbus_proxy = NULL;
static GHashTable *named_structs = NULL;


/* private api */

DBusGConnection *
_xfconf_get_dbus_g_connection(void)
{
    if(!xfconf_refcnt) {
        g_critical("xfconf_init() must be called before attempting to use libxfconf!");
        return NULL;
    }

    return dbus_conn;
}

DBusGProxy *
_xfconf_get_dbus_g_proxy(void)
{
    if(!xfconf_refcnt) {
        g_critical("xfconf_init() must be called before attempting to use libxfconf!");
        return NULL;
    }

    return dbus_proxy;
}

XfconfNamedStruct *
_xfconf_named_struct_lookup(const gchar *struct_name)
{
    return named_structs ? g_hash_table_lookup(named_structs, struct_name) : NULL;
}

static void
_xfconf_named_struct_free(XfconfNamedStruct *ns)
{
    g_free(ns->member_types);
    g_slice_free(XfconfNamedStruct, ns);
}



static void
xfconf_static_dbus_init(void)
{
    static gboolean static_dbus_inited = FALSE;

    if(!static_dbus_inited) {
        dbus_g_error_domain_register(XFCONF_ERROR, "org.xfce.Xfconf.Error",
                                     XFCONF_TYPE_ERROR);

        dbus_g_object_register_marshaller(_xfconf_marshal_VOID__STRING_STRING_BOXED,
                                          G_TYPE_NONE,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_VALUE,
                                          G_TYPE_INVALID);
        dbus_g_object_register_marshaller(_xfconf_marshal_VOID__STRING_STRING,
                                          G_TYPE_NONE,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_INVALID);

        static_dbus_inited = TRUE;
    }
}



/* public api */

/**
 * xfconf_init:
 * @error: An error return.
 *
 * Initializes the Xfconf library.  Can be called multiple times with no
 * adverse effects.
 *
 * Returns: %TRUE if the library was initialized succesfully, %FALSE on
 *          error.  If there is an error @error will be set.
 **/
gboolean
xfconf_init(GError **error)
{
    if(xfconf_refcnt) {
        ++xfconf_refcnt;
        return TRUE;
    }

#if !GLIB_CHECK_VERSION(2,36,0)
    g_type_init();
#endif

    xfconf_static_dbus_init();

    dbus_conn = dbus_g_bus_get(DBUS_BUS_SESSION, error);
    if(!dbus_conn)
        return FALSE;

    dbus_proxy = dbus_g_proxy_new_for_name(dbus_conn,
                                           "org.xfce.Xfconf",
                                           "/org/xfce/Xfconf",
                                           "org.xfce.Xfconf");

    dbus_g_proxy_add_signal(dbus_proxy, "PropertyChanged",
                            G_TYPE_STRING, G_TYPE_STRING, G_TYPE_VALUE,
                            G_TYPE_INVALID);
    dbus_g_proxy_add_signal(dbus_proxy, "PropertyRemoved",
                            G_TYPE_STRING, G_TYPE_STRING,
                            G_TYPE_INVALID);

    ++xfconf_refcnt;
    return TRUE;
}

/**
 * xfconf_shutdown:
 *
 * Shuts down and frees any resources consumed by the Xfconf library.
 * If xfconf_init() is called multiple times, xfconf_shutdown() must be
 * called an equal number of times to shut down the library.
 **/
void
xfconf_shutdown(void)
{
    if(xfconf_refcnt <= 0) {
        return;
    }

    if(xfconf_refcnt > 1) {
        --xfconf_refcnt;
        return;
    }

    _xfconf_channel_shutdown();
    _xfconf_g_bindings_shutdown();

    if(named_structs) {
        g_hash_table_destroy(named_structs);
        named_structs = NULL;
    }

    g_object_unref(G_OBJECT(dbus_proxy));
    dbus_proxy = NULL;

    dbus_g_connection_unref(dbus_conn);
    dbus_conn = NULL;

    --xfconf_refcnt;
}

/**
 * xfconf_named_struct_register:
 * @struct_name: The unique name of the struct to register.
 * @n_members: The number of data members in the struct.
 * @member_types: An array of the #GType<!-- -->s of the struct members.
 *
 * Registers a named struct for use with xfconf_channel_get_named_struct()
 * and xfconf_channel_set_named_struct().
 **/
void
xfconf_named_struct_register(const gchar *struct_name,
                             guint n_members,
                             const GType *member_types)
{
    XfconfNamedStruct *ns;

    g_return_if_fail(struct_name && *struct_name && n_members && member_types);

    /* lazy initialize the hash table */
    if(named_structs == NULL)
        named_structs = g_hash_table_new_full(g_str_hash, g_str_equal,
                                              (GDestroyNotify)g_free,
                                              (GDestroyNotify)_xfconf_named_struct_free);

    if(G_UNLIKELY(g_hash_table_lookup(named_structs, struct_name)))
        g_critical("The struct '%s' is already registered", struct_name);
    else {
        ns = g_slice_new(XfconfNamedStruct);
        ns->n_members = n_members;
        ns->member_types = g_new(GType, n_members);
        memcpy(ns->member_types, member_types, sizeof(GType) * n_members);

        g_hash_table_insert(named_structs, g_strdup(struct_name), ns);
    }
}

#if 0
/**
 * xfconf_array_new:
 * @n_preallocs: Number of entries to preallocate space for.
 *
 * Convenience function to greate a new #GArray to hold
 * #GValue<!-- -->s.  Normal #GArray functions may be used on
 * the returned array.  For convenience, see also xfconf_array_free().
 *
 * Returns: A new #GArray.
 **/
GArray *
xfconf_array_new(gint n_preallocs)
{
    return g_array_sized_new(FALSE, TRUE, sizeof(GValue), n_preallocs);
}
#endif

/**
 * xfconf_array_free:
 * @arr: A #GPtrArray of #GValue<!-- -->s.
 *
 * Properly frees a #GPtrArray structure containing a list of
 * #GValue<!-- -->s.  This will also cause the contents of the
 * values to be freed as well.
 **/
void
xfconf_array_free(GPtrArray *arr)
{
    guint i;
    
    if(!arr)
        return;
    
    for(i = 0; i < arr->len; ++i) {
        GValue *val = g_ptr_array_index(arr, i);
        g_value_unset(val);
        g_free(val);
    }
    
    g_ptr_array_free(arr, TRUE);
}



#define __XFCONF_C__
#include "common/xfconf-aliasdef.c"
