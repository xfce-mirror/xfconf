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

#include <stdio.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <glib.h>

#include <libxfce4util/libxfce4util.h>

#ifdef ENABLE_NLS
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#endif

#include "xfconf-gvaluefuncs.h"
#include "xfconf-common-private.h"
#include "xfconf/xfconf.h"

static gboolean version = FALSE;
static gboolean list = FALSE;
static gboolean verbose = FALSE;
static gboolean create = FALSE;
static gboolean reset = FALSE;
static gboolean recursive = FALSE;
static gboolean force_array = FALSE;
static gboolean monitor = FALSE;
static gchar *channel_name = NULL;
static gchar *property_name = NULL;
static gchar **set_value = NULL;
static gchar **type = NULL;
static gchar *import_file = NULL;
static gchar *export_file = NULL;

static void
xfconf_query_monitor (XfconfChannel *channel, const gchar *changed_property, GValue *property_value)
{
    gchar *string;
    
    if (property_name && !g_str_has_prefix (changed_property, property_name))
        return;

    if (property_value && G_IS_VALUE (property_value))
    {
        if (verbose)
        {
           string = _xfconf_string_from_gvalue (property_value);
           g_print (_("Property '%s' changed: %s\n"), changed_property, string);
           g_free (string);
       }
       else
       {
           g_print (_("Property '%s' changed\n"), changed_property);
       }
    }
    else
    {
        g_print (_("Property '%s' removed\n"), changed_property);
    }
}

static gboolean
xfconf_query_import_channel (XfconfChannel *channel, gint fd, GError **error)
{
    if (error != NULL)
    {
        g_set_error (error, G_FILE_ERROR, 1, _("Export method not yet implemented"));
    }
    else
    {
        g_warning ("--import: Method not implemented");
    }
    return FALSE;
}

static gboolean
xfconf_query_export_channel (XfconfChannel *channel, gint fd, GError **error)
{
    if (error != NULL)
    {
        g_set_error (error, G_FILE_ERROR, 1, _("Export not yet implemented"));
    }
    else
    {
        g_warning ("--export: Method not implemented");
    }
    return FALSE;
}

static void
xfconf_query_get_propname_size (gpointer key, gpointer value, gpointer user_data)
{
    gint *size = user_data;
    gchar *propname = (gchar *)key;

    if ((gint) strlen(propname) > *size)
        *size = strlen(propname);

}

static void
xfconf_query_list_sorted (gpointer key, gpointer value, gpointer user_data)
{
  GSList **listp = user_data;
  
  *listp = g_slist_insert_sorted (*listp, key, (GCompareFunc) g_utf8_collate);
}

static void
xfconf_query_list_contents (GSList *sorted_contents, GHashTable *channel_contents, gint size)
{
    GSList *li;
    gchar *format = verbose ? g_strdup_printf ("%%-%ds%%s\n", size + 2) : NULL;
    GValue *property_value;
    gchar *string;
    
    for (li = sorted_contents; li != NULL; li = li->next)
    {
        if (verbose)
        {
            property_value = g_hash_table_lookup (channel_contents, li->data);

            if (XFCONF_TYPE_G_VALUE_ARRAY != G_VALUE_TYPE (property_value))
            {
                string = _xfconf_string_from_gvalue (property_value);
            }
            else
            {
                string = g_strdup ("<<UNSUPPORTED>>");
            }

            g_print (format, (gchar *) li->data, string);
            g_free (string);
        }
        else
        {
            g_print ("%s\n", (gchar *) li->data);
        }
    }
    
    g_free (format);
}

static GOptionEntry entries[] =
{
     {   "version", 'V', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &version,
        N_("Version information"),
        NULL
    },
    {    "channel", 'c', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &channel_name,
        N_("The channel to query/modify"),
        NULL
    },
    {    "property", 'p', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &property_name,
        N_("The property to query/modify"),
        NULL
    },
    {    "set", 's', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING_ARRAY, &set_value,
        N_("The new value to set for the property"),
        NULL
    },
    {    "list", 'l', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &list,
        N_("List properties (or channels if -c is not specified)"),
        NULL
    },
    {    "verbose", 'v', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &verbose,
        N_("Verbose output"),
        NULL
    },
    {    "create", 'n', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &create,
        N_("Create a new property if it does not already exist"),
        NULL
    },
    {    "type", 't', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING_ARRAY, &type,
       N_("Specify the property value type"),
       NULL
    },
    {    "reset", 'r', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &reset,
        N_("Reset property"),
        NULL
    },
    {    "recursive", 'R', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &recursive,
        N_("Recursive (use with -r)"),
        NULL
    },
    {   "force-array", 'a', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &force_array,
        N_("Force array even if only one element"),
        NULL
    },
/*
    {   "export", 'x', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &export_file,
        N_("Export channel to file"),
        NULL,
    },
    {   "import", 'i', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &import_file,
        N_("Import channel from file"),
        NULL,
    },
*/
    {   "monitor", 'm', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &monitor,
        N_("Monitor a channel for property changes"),
        NULL,
    },
    { NULL }
};

int 
main(int argc, char **argv)
{
    GError *cli_error = NULL;
    GError *error= NULL;
    XfconfChannel *channel = NULL;
    gboolean prop_exists;
    gint fd = -1;
    GOptionContext *context;

#ifdef ENABLE_NLS
    setlocale (LC_ALL, "");
#endif
    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    g_type_init();
    if(!xfconf_init(&error))
    {
        g_critical("Failed to init libxfconf: %s\n", error->message);
        g_error_free(error);
        return 1;
    }

    context = g_option_context_new("- xfconf commandline utility");

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
        gchar **channels;
        gint i;

        g_print("Channels:\n");

        channels = xfconf_list_channels();
        if(G_LIKELY(channels)) {
            for(i = 0; channels[i]; ++i)
                g_print("  %s\n", channels[i]);
            g_strfreev(channels);
        } else
            return 1;

        return 0;
    }

    /** Check if the property is specified */
    if(!property_name && !list && !export_file && !import_file && !monitor)
    {
        g_print("No property specified, aborting...\n");
        return 1;
    }

    if (create && reset)
    {
        g_print("--create and --reset options can not be used together,\naborting...\n");
        return 1;
    }

    if ((create || reset) && (list))
    {
        g_print("--create and --reset options can not be used together with\n --list\naborting...\n");
        return 1;
    }

    if (import_file && export_file)
    {
        g_print("--import and --export options can not be used together,\naborting...\n");
        return 1;
    }

    if ((import_file || export_file) && (list || property_name || create || reset))
    {
        g_print("--import and --export options can not be used together with\n --create, --reset, --property and --list,\naborting...\n");
        return 1;
    }

    channel = xfconf_channel_new(channel_name);
    
    if (monitor)
    {
        GMainLoop *loop;

        g_signal_connect (G_OBJECT (channel), "property-changed", G_CALLBACK (xfconf_query_monitor), NULL);
        
        g_print ("\n");
        g_print (_("Start monitoring channel '%s':"), channel_name);
        g_print ("\n------------------------------------------------\n\n");
        
        loop = g_main_loop_new (NULL, TRUE);
        g_main_loop_run (loop);
        g_main_loop_unref (loop);        
    }

    if (property_name && !list)
    {
        /** Reset property */
        if (reset)
        {
            xfconf_channel_reset_property(channel, property_name, recursive);
        }
        /** Read value */
        else if(set_value == NULL || set_value[0] == NULL)
        {
            GValue value = { 0, };

            if(!xfconf_channel_get_property(channel, property_name, &value))
            {
                g_print(_("Property \"%s\" does not exist on channel \"%s\".\n"),
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
                guint i;

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
                g_printerr(_("Property \"%s\" does not exist on channel \"%s\".  If a new\n"
                             "property should be created, use the --create option.\n"),
                             property_name, channel_name);
                return 1;
            }

            if(!prop_exists && (!type || !type[0]))
            {
                g_printerr(_("When creating a new property, the value type must be specified.\n"));
                return 1;
            }

            if(prop_exists && !(type && type[0]))
            {
                /* only care what the existing type is if the user
                 * didn't specify one */
                if(!xfconf_channel_get_property(channel, property_name, &value))
                {
                    g_printerr(_("Failed to get the existing type for the value.\n"));
                    return 1;
                }
            }

            if(!set_value[1] && !force_array)
            {
                /* not an array */
                GType gtype = G_TYPE_INVALID;

                if(prop_exists)
                    gtype = G_VALUE_TYPE(&value);

                if(type && type[0])
                    gtype = _xfconf_gtype_from_string(type[0]);

                if(G_TYPE_INVALID == gtype || G_TYPE_NONE == gtype)
                {
                    g_printerr(_("Unable to determine the type of the value.\n"));
                    return 1;
                }

                if(XFCONF_TYPE_G_VALUE_ARRAY == gtype)
                {
                    g_printerr(_("A value type must be specified to change an array into a single value.\n"));
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
                    g_printerr(_("There are %d new values, but only %d types could be determined.\n"),
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
        GHashTable *channel_contents = xfconf_channel_get_properties(channel, property_name);
        if (channel_contents)
        {
            gint size = 0;
            GSList *sorted_contents = NULL;

            if (verbose)
                g_hash_table_foreach (channel_contents, (GHFunc)xfconf_query_get_propname_size, &size);
            
            g_hash_table_foreach (channel_contents, (GHFunc)xfconf_query_list_sorted, &sorted_contents);
            
            xfconf_query_list_contents (sorted_contents, channel_contents, size);
            
            g_slist_free (sorted_contents);
            g_hash_table_destroy (channel_contents);
        }
        else
        {
            g_print(_("Channel '%s' contains no properties\n"), channel_name);
        }
    }

    if (export_file)
    {
        if (!strcmp(export_file, "-"))
        {
            /* Use stdout */
            fd = fileno (stdout);
        }
        else
        {
            fd = open (export_file, O_CREAT | O_EXCL | O_WRONLY, 0);
            if (fd < 0)
            {
                g_printerr (_("Could not create export file \"%s\": %s\n"), export_file, strerror (errno));
                return 1;
            }
        }

        if (!xfconf_query_export_channel (channel, fd, &error))
        {
            if (fd != fileno (stdout))
                close (fd);
            g_printerr (_("Could not create export file \"%s\": %s\n"), export_file, error->message);
            g_error_free (error);
            return 1;
        }
        if (fd != fileno (stdout))
            close (fd);
    }

    if (import_file)
    {
        if (!strcmp(import_file, "-"))
        {
            /* Use stdin */
            fd = fileno (stdin);
        }
        else
        {
            fd = open (import_file, O_RDONLY, 0);
            if (fd < 0)
            {
                g_printerr (_("Could not open import file \"%s\": %s\n"), import_file, strerror (errno));
                return 1;
            }
        }

        if (!xfconf_query_import_channel (channel, fd, &error))
        {
            if (fd != fileno (stdin))
                close (fd);
            g_printerr (_("Could not parse import file \"%s\": %s\n"), import_file, error->message);
            g_error_free (error);
            return 1;
        }
        if (fd != fileno (stdin))
            close (fd);
    }

    return 0;
}
