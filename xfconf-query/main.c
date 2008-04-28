/*
 *  Copyright (c) 2008 Stephan Arts <stephan@xfce.org>
 *  Copyright (c) 2008 Brian Tarricone <bjt23@cornell.edu>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <glib.h>

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include "xfconf-gvaluefuncs.h"
#include "xfconf-common-private.h"
#include "xfconf/xfconf.h"

static gboolean version = FALSE;
static gboolean list = FALSE;
static gboolean verbose = FALSE;
static gboolean create = FALSE;
static gboolean remove = FALSE;
static gchar *channel_name = NULL;
static gchar *property_name = NULL;
static gchar **set_value = NULL;
static gchar **type = NULL;

static void
xfconf_query_get_propname_size (gpointer key, gpointer value, gpointer user_data)
{
    gint *size = user_data;
    gchar *property_name = (gchar *)key;

    if (strlen(property_name) > *size)
        *size = strlen(property_name);

}

static void
xfconf_query_list_contents (gpointer key, gpointer value, gpointer user_data)
{
    gint i;
    gint size = *(gint *)user_data;
    gint property_name_size = 0, value_size = 0;
    gchar *property_name = (gchar *)key;
    GValue *property_value = (GValue *)value;

    if (verbose)
    {
        g_print("%s%n", property_name, &property_name_size);
        for(i = property_name_size; i < (size+2); ++i)
        {
            g_print(" ");
        }
        const gchar *str_val = _xfconf_string_from_gvalue(property_value);
        g_print("%s%n", str_val, &value_size);
        g_print("\n");
    }
    else
    {
        g_print ("%s\n", property_name);
    }
}

static GOptionEntry entries[] =
{
    {    "version", 'V', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &version,
        N_("Version information"),
        NULL
    },
    {    "channel", 'c', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &channel_name,
        N_("pick the channel"),
        NULL
    },
    {    "property", 'p', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &property_name,
        N_("pick the property"),
        NULL
    },
    {    "set", 's', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING_ARRAY, &set_value,
        N_("set (change the value)"),
        NULL
    },
    {    "list", 'l', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &list,
        N_("List properties"),
        NULL
    },
    {    "verbose", 'v', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &verbose,
        N_("Verbose output"),
        NULL
    },
    {    "create", 'n', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &create,
        N_("Create new entry"),
        NULL
    },
    {    "type", 't', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING_ARRAY, &type,
       N_("Specify the property value type"),
       NULL
    },
    {    "remove", 'r', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &remove,
        N_("Remove property"),
        NULL
    },
    { NULL }
};

int 
main(int argc, char **argv)
{
    GError *cli_error = NULL;
    XfconfChannel *channel = NULL;
    gboolean prop_exists;

    g_type_init();
    xfconf_init(NULL);
    
    GOptionContext *context = g_option_context_new("- xfconf commandline utility");

    g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);

    if(!g_option_context_parse(context, &argc, &argv, &cli_error))
    {
        g_print("option parsing failed: %s\n", cli_error->message);
        return 1;
    }

    if(version)
    {
        g_print("xfconf-query  %s\n", PACKAGE_VERSION);
        return 0;
    }

    /** Check if the channel is specified */
    if(!channel_name)
    {
        g_print("No channel specified, aborting...\n");
        return 1;
    }

    /** Check if the property is specified */
    if(!property_name && !list)
    {
        g_print("No property specified, aborting...\n");
        return 1;
    }

    if (create && remove)
    {
        g_print("--create and --remove options can not be used together,\naborting...\n");
        return 1;
    }

    channel = xfconf_channel_new(channel_name);

    if (property_name)
    {
        /** Remove property */
        if (remove)
        {
            xfconf_channel_remove_property(channel, property_name);
        }
        /** Read value */
        else if(set_value == NULL || set_value[0] == NULL)
        {
            GValue value = { 0, };

            if(!xfconf_channel_get_property(channel, property_name, &value))
            {
                g_print(_("Property \"%s\" doesn't exist on channel \"%s\"."),
                        property_name, channel_name);
                return 1;
            }
            
            if(XFCONF_TYPE_G_VALUE_ARRAY != G_VALUE_TYPE(&value))
            {
                gchar *str_val = _xfconf_string_from_gvalue(&value);
                g_print("%s\n", str_val ? str_val : _("(unknown)"));
                g_free(str_val);
                g_value_unset(&value);
            }
            else
            {
                GPtrArray *arr = g_value_get_boxed(&value);
                gint i;

                g_print(_("Value is an array with %d items:\n\n"), arr->len);

                for(i = 0; i < arr->len; ++i)
                {
                    GValue *item_value = g_ptr_array_index(arr, i);

                    if(item_value)
                    {
                        gchar *str_val = _xfconf_string_from_gvalue(item_value);
                        g_print("%s\n", str_val ? str_val : _("(unknown)"));
                        g_free(str_val);
                    }
                }
            }
        }
        /* Write value */
        else {
            GValue value = { 0, };
            gint i;

            prop_exists = xfconf_channel_has_property(channel, property_name);
            if(!prop_exists && !create)
            {
                g_printerr(_("Property \"%s\" does not exist on channel \"%s\".  If you would\n"
                             "like to create a new property, use the --create option.\n"),
                             property_name, channel_name);
                return 1;
            }

            if(!prop_exists && (!type || !type[0]))
            {
                g_printerr(_("When creating a new property, you must specify the value type.\n"));
                return 1;
            }

            if(prop_exists && !(type && type[0]))
            {
                /* only care what the existing type is if the user
                 * didn't specify one */
                if(!xfconf_channel_get_property(channel, property_name, &value))
                {
                    g_printerr(_("Failed to get existing type for value.\n"));
                    return 1;
                }
            }

            if(!set_value[1])
            {
                /* not an array */
                GType gtype = G_TYPE_INVALID;

                if(prop_exists)
                    gtype = G_VALUE_TYPE(&value);

                if(type && type[0])
                    gtype = _xfconf_gtype_from_string(type[0]);

                if(G_TYPE_INVALID == gtype || G_TYPE_NONE == gtype)
                {
                    g_printerr(_("Unable to determine type of value.\n"));
                    return 1;
                }

                if(XFCONF_TYPE_G_VALUE_ARRAY == gtype)
                {
                    g_printerr(_("You must specify a value type to change an array into a single value.\n"));
                    return 1;
                }

                if(G_VALUE_TYPE(&value))
                    g_value_unset(&value);

                g_value_init(&value, gtype);
                if(!_xfconf_gvalue_from_string(&value, set_value[0]))
                {
                    g_printerr(_("Unable to convert \"%s\" to type \"%s\"\n"),
                                   set_value[0], g_type_name(gtype));
                    return 1;
                }

                if(!xfconf_channel_set_property(channel, property_name, &value))
                {
                    g_printerr(_("Failed to set property.\n"));
                    return 1;
                }
            }
            else
            {
                /* array property */
                GPtrArray *arr_old = NULL, *arr_new;
                gint new_values = 0, new_types = 0;

                for(new_values = 0; set_value[new_values]; ++new_values)
                    ;
                if(type && type[0])
                {
                    for(new_types = 0; type[new_types]; ++new_types)
                        ;
                }
                else if(G_VALUE_TYPE(&value) == XFCONF_TYPE_G_VALUE_ARRAY)
                {
                    arr_old = g_value_get_boxed(&value);
                    new_types = arr_old->len;
                }

                if(new_values != new_types)
                {
                    g_printerr(_("Have %d new values, but could only determine %d types.\n"),
                               new_values, new_types);
                    return 1;
                }

                arr_new = g_ptr_array_sized_new(new_values);

                for(i = 0; set_value[i]; ++i) {
                    GType gtype = G_TYPE_INVALID;
                    GValue *value_new;

                    if(arr_old)
                    {
                        GValue *value_old = g_ptr_array_index(arr_old, i);
                        gtype = G_VALUE_TYPE(value_old);
                    }
                    else
                        gtype = _xfconf_gtype_from_string(type[i]);

                    if(G_TYPE_INVALID == gtype || G_TYPE_NONE == gtype)
                    {
                        g_printerr(_("Unable to determine type of value at index %d.\n"),
                                   i);
                        return 1;
                    }

                    value_new = g_new0(GValue, 1);
                    g_value_init(value_new, gtype);
                    if(!_xfconf_gvalue_from_string(value_new, set_value[i]))
                    {
                        g_printerr(_("Unable to convert \"%s\" to type \"%s\"\n"),
                                   set_value[i], g_type_name(gtype));
                        return 1;
                    }
                    g_ptr_array_add(arr_new, value_new);
                }

                if(G_VALUE_TYPE(&value))
                    g_value_unset(&value);

                g_value_init(&value, XFCONF_TYPE_G_VALUE_ARRAY);
                g_value_set_boxed(&value, arr_new);

                if(!xfconf_channel_set_property(channel, property_name, &value))
                {
                    g_printerr(_("Failed to set property.\n"));
                    return 1;
                }
            }
        }
    }

    if (list)
    {
        GHashTable *channel_contents = xfconf_channel_get_all(channel);
        if (channel_contents)
        {
            gint size = 0;
            g_hash_table_foreach (channel_contents, (GHFunc)xfconf_query_get_propname_size, &size);
            g_hash_table_foreach (channel_contents, (GHFunc)xfconf_query_list_contents, &size);
        }
        else
        {
            g_print(_("Channel '%s' contains no properties\n"), channel_name);
        }
    }

    return 0;
}
