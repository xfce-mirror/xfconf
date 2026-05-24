/*
 *  Copyright (c) 2008 Stephan Arts <stephan@xfce.org>
 *  Copyright (c) 2008 Brian Tarricone <bjt23@cornell.edu>
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

#ifdef HAVE_XFCE_REVISION_H
#include "xfce-revision.h"
#endif

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
#include <stdio.h>

#include "common/xfconf-common-private.h"
#include "common/xfconf-gvaluefuncs.h"
#include "xfconf/xfconf.h"

static gboolean version = FALSE;
static gboolean list = FALSE;
static gboolean create = FALSE;
static gboolean reset = FALSE;
static gboolean recursive = FALSE;
static gboolean force_array = FALSE;
static gboolean monitor = FALSE;
static gboolean toggle = FALSE;
static gboolean list_types = FALSE;
static gchar *channel_name = NULL;
static gchar *property_name = NULL;
static gchar **set_value = NULL;
static gchar **type = NULL;
static guint verbose = 0;
static gboolean
xfconf_query_parse_option(const gchar *option_name,
                          const gchar *value,
                          gpointer data,
                          GError **error)
{
    if (g_strcmp0(option_name, "-v") == 0 || g_strcmp0(option_name, "--verbose") == 0) {
        verbose++;
    }

    return TRUE;
}

static gchar *
xfconf_query_string_from_gvalue(GValue *value)
{
    if (G_VALUE_TYPE(value) != G_TYPE_PTR_ARRAY) {
        return _xfconf_string_from_gvalue(value);
    } else {
        GPtrArray *arr = g_value_get_boxed(value);
        gchar **strv = g_new0(gchar *, arr->len + 1);

        for (guint i = 0; i < arr->len; ++i) {
            GValue *item_value = g_ptr_array_index(arr, i);
            strv[i] = _xfconf_string_from_gvalue(item_value);
        }
        gchar *temp = g_strjoinv(",", strv);
        gchar *string = g_strdup_printf("[%s]", temp);
        g_free(temp);
        g_strfreev(strv);

        return string;
    }
}

static void
xfconf_query_monitor(XfconfChannel *channel, const gchar *changed_property, GValue *property_value)
{
    if (property_name && !g_str_has_prefix(changed_property, property_name)) {
        return;
    }

    if (property_value && G_IS_VALUE(property_value)) {
        // TRANSLATORS: This refers to: "A property has been set"
        const gchar *prefix = _("set");
        if (verbose > 0) {
            gchar *string = xfconf_query_string_from_gvalue(property_value);
            if (verbose > 1) {
                const gchar *str_type = _xfconf_string_from_gtype(G_VALUE_TYPE(property_value));
                g_print("%s: %s [%s] (%s)\n", prefix, changed_property, str_type, string);
            } else {
                g_print("%s: %s (%s)\n", prefix, changed_property, string);
            }
            g_free(string);
        } else {
            g_print("%s: %s\n", prefix, changed_property);
        }
    } else {
        // TRANSLATORS: This refers to: "A property has been reset"
        g_print("%s: %s\n", _("reset"), changed_property);
    }
}

static void
xfconf_query_get_propname_size(gpointer key, gpointer value, gpointer user_data)
{
    gint *size = user_data;

    size[0] = MAX(size[0], (gint)strlen(key));
    if (verbose > 1) {
        const gchar *str_type = _xfconf_string_from_gtype(G_VALUE_TYPE(value));
        size[1] = MAX(size[1], (gint)strlen(str_type));
    }
}

static void
xfconf_query_list_sorted(gpointer key, gpointer value, gpointer user_data)
{
    GSList **listp = user_data;

    *listp = g_slist_insert_sorted(*listp, key, (GCompareFunc)g_utf8_collate);
}

static void
xfconf_query_list_contents(GSList *sorted_contents, GHashTable *channel_contents, gint prop_size, gint type_size)
{
    GSList *li;
    gchar *format = NULL;
    GValue *property_value;
    gchar *string;
    if (verbose == 1) {
        format = g_strdup_printf("%%-%ds%%s\n", prop_size + 2);
    } else if (verbose > 1) {
        format = g_strdup_printf("%%-%ds%%-%ds%%s\n", prop_size + 2, type_size + 2);
    }

    for (li = sorted_contents; li != NULL; li = li->next) {
        if (verbose > 0) {
            property_value = g_hash_table_lookup(channel_contents, li->data);
            string = xfconf_query_string_from_gvalue(property_value);
            if (verbose > 1) {
                const gchar *str_type = _xfconf_string_from_gtype(G_VALUE_TYPE(property_value));
                g_print(format, (gchar *)li->data, str_type, string);
            } else {
                g_print(format, (gchar *)li->data, string);
            }
            g_free(string);
        } else {
            g_print("%s\n", (gchar *)li->data);
        }
    }

    g_free(format);
}

static void
xfconf_query_printerr(const gchar *message,
                      ...)
{
    va_list args;
    gchar *str;

    va_start(args, message);
    str = g_strdup_vprintf(message, args);
    va_end(args);

    g_printerr("%s.\n", str);
    g_free(str);
}

static gint
xfconf_query_compare_func(gconstpointer a,
                          gconstpointer b,
                          gpointer user_data)
{
    gchar **s = (gchar **)a;
    gchar **t = (gchar **)b;
    return g_strcmp0(*s, *t);
}

static void
xfconf_query_list_types(gboolean on_stderr)
{
    void (*print_func)(const gchar *, ...) = on_stderr ? g_printerr : g_print;
    GType gtypes[] = {
        G_TYPE_STRING,
        G_TYPE_BOOLEAN,
        G_TYPE_UCHAR,
        G_TYPE_CHAR,
        XFCONF_TYPE_UINT16,
        XFCONF_TYPE_INT16,
        G_TYPE_UINT,
        G_TYPE_INT,
        G_TYPE_UINT64,
        G_TYPE_INT64,
        G_TYPE_FLOAT,
        G_TYPE_DOUBLE,
        G_TYPE_INVALID,
    };
    print_func(_("Possible types:"));
    print_func("\n");
    for (guint n = 0; gtypes[n] != G_TYPE_INVALID; n++) {
        print_func("%s\n", _xfconf_string_from_gtype(gtypes[n]));
    }
}

static GOptionEntry entries[] = {
    { "version", 'V', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &version,
      N_("Version information"),
      NULL },
    { "channel", 'c', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &channel_name,
      N_("The channel to query/modify"),
      NULL },
    { "property", 'p', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &property_name,
      N_("The property to query/modify"),
      NULL },
    { "set", 's', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING_ARRAY, &set_value,
      N_("The new value to set for the property (multiple uses for an array)"),
      NULL },
    { "list", 'l', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &list,
      N_("List properties (or channels if -c is not specified)"),
      NULL },
    { "verbose", 'v', G_OPTION_FLAG_IN_MAIN | G_OPTION_FLAG_OPTIONAL_ARG | G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, xfconf_query_parse_option,
      N_("Print also the value in combination with -l or -m (use twice to print also the type)"),
      NULL },
    { "type", 't', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING_ARRAY, &type,
      N_("Specify the property value type (multiple uses for an array)"),
      NULL },
    { "reset", 'r', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &reset,
      N_("Reset property"),
      NULL },
    { "recursive", 'R', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &recursive,
      N_("Recursive (use with -r)"),
      NULL },
    { "force-array", 'a', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &force_array,
      N_("Force array even if only one element"),
      NULL },
    { "toggle", 'T', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &toggle,
      N_("Invert an existing boolean property"),
      NULL },
    { "monitor", 'm', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &monitor,
      N_("Monitor a channel for property changes"),
      NULL },
    { "list-types", '\0', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &list_types,
      N_("List types that can be specified using the --type option"),
      NULL },
    /* deprecated */
    { "create", 'n', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &create,
      N_("Create a new property if it does not already exist"),
      NULL },
    { NULL }
};

int
main(int argc, char **argv)
{
    GError *error = NULL;
    XfconfChannel *channel = NULL;
    gboolean prop_exists;
    GOptionContext *context;

    xfce_textdomain(GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    context = g_option_context_new(_("- Xfconf commandline utility"));
    g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);

    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        xfconf_query_printerr(_("Option parsing failed: %s"), error->message);
        g_error_free(error);
        g_option_context_free(context);
        return EXIT_FAILURE;
    }
    g_option_context_free(context);

    if (version) {
        g_print("xfconf-query");
        g_print(" %s\n\n", VERSION_FULL);
        g_print("%s\n", "Copyright (c) 2008-" COPYRIGHT_YEAR);
        g_print("\t%s\n\n", _("The Xfce development team. All rights reserved."));
        g_print(_("Please report bugs to <%s>."), PACKAGE_BUGREPORT);
        g_print("\n");

        return EXIT_SUCCESS;
    }

    if (list_types) {
        xfconf_query_list_types(FALSE);
        return EXIT_SUCCESS;
    }

    /** Check if the property is specified */
    if (!property_name && !list && !monitor) {
        xfconf_query_printerr(_("No property specified"));
        return EXIT_FAILURE;
    }

    if (property_name != NULL && !g_str_has_prefix(property_name, "/")) {
        xfconf_query_printerr(_("The property name should start with '/'"));
        return EXIT_FAILURE;
    }

    if (set_value != NULL && reset) {
        xfconf_query_printerr(_("--set and --reset options can not be used together"));
        return EXIT_FAILURE;
    }

    if ((set_value != NULL || reset) && (list)) {
        xfconf_query_printerr(_("--set and --reset options can not be used together with --list"));
        return EXIT_FAILURE;
    }

    if (create) {
        g_warning("--create option is deprecated and will be removed in a future release; --set now automatically creates the property if necessary");
    }

    if (!xfconf_init(&error)) {
        xfconf_query_printerr(_("Failed to init libxfconf: %s"), error->message);
        g_error_free(error);
        return EXIT_FAILURE;
    }

    /** Check if the channel is specified */
    if (!channel_name) {
        gchar **channels;
        gint i;

        g_print("%s\n", _("Channels:"));

        channels = xfconf_list_channels();
        if (G_LIKELY(channels)) {
            g_qsort_with_data(channels, g_strv_length(channels), sizeof(gchar *), xfconf_query_compare_func, NULL);
            for (i = 0; channels[i]; ++i) {
                g_print("  %s\n", channels[i]);
            }
            g_strfreev(channels);
        } else {
            xfconf_shutdown();
            return EXIT_FAILURE;
        }
        xfconf_shutdown();
        return EXIT_SUCCESS;
    }

    channel = xfconf_channel_new(channel_name);

    if (monitor) {
        GMainLoop *loop;

        g_signal_connect(G_OBJECT(channel), "property-changed", G_CALLBACK(xfconf_query_monitor), NULL);

        g_print(_("Start monitoring channel \"%s\":"), channel_name);
        g_print("\n\n");

        loop = g_main_loop_new(NULL, TRUE);
        g_main_loop_run(loop);
        g_main_loop_unref(loop);
    }

    if (property_name && !list) {
        if (reset) {
            /** Reset property */
            xfconf_channel_reset_property(channel, property_name, recursive);
        } else if (set_value == NULL || set_value[0] == NULL) {
            /** Read value */
            GValue value = G_VALUE_INIT;

            if (!xfconf_channel_get_property(channel, property_name, &value)) {
                xfconf_query_printerr(_("Property \"%s\" does not exist on channel \"%s\""),
                                      property_name, channel_name);
                xfconf_shutdown();
                return EXIT_FAILURE;
            }

            if (toggle) {
                if (G_VALUE_HOLDS_BOOLEAN(&value)) {
                    if (xfconf_channel_set_bool(channel, property_name, !g_value_get_boolean(&value))) {
                        xfconf_shutdown();
                        return EXIT_SUCCESS;
                    } else {
                        xfconf_query_printerr(_("Failed to set property"));
                    }
                } else {
                    xfconf_query_printerr(_("--toggle only works with boolean values"));
                }
                xfconf_shutdown();
                return EXIT_FAILURE;
            }

            if (G_TYPE_PTR_ARRAY != G_VALUE_TYPE(&value)) {
                gchar *str_val = _xfconf_string_from_gvalue(&value);
                if (verbose > 0) {
                    const gchar *str_type = _xfconf_string_from_gtype(G_VALUE_TYPE(&value));
                    g_print("[%s] %s\n", str_type, str_val ? str_val : _("(unknown)"));
                } else {
                    g_print("%s\n", str_val ? str_val : _("(unknown)"));
                }
                g_free(str_val);
                g_value_unset(&value);
            } else {
                GPtrArray *arr = g_value_get_boxed(&value);
                guint i;

                g_print(_("Value is an array with %d items:"), arr->len);
                g_print("\n\n");

                for (i = 0; i < arr->len; ++i) {
                    GValue *item_value = g_ptr_array_index(arr, i);

                    if (item_value) {
                        gchar *str_val = _xfconf_string_from_gvalue(item_value);
                        if (verbose > 0) {
                            const gchar *str_type = _xfconf_string_from_gtype(G_VALUE_TYPE(item_value));
                            g_print("[%s] %s\n", str_type, str_val ? str_val : _("(unknown)"));
                        } else {
                            g_print("%s\n", str_val ? str_val : _("(unknown)"));
                        }
                        g_free(str_val);
                    }
                }
            }
        } else {
            /* Write value */
            GValue value = G_VALUE_INIT;
            gint i;

            prop_exists = xfconf_channel_has_property(channel, property_name);
            if (!prop_exists && (!type || !type[0])) {
                xfconf_query_printerr(_("When creating a new property, the value type must be specified"));
                xfconf_query_list_types(TRUE);
                xfconf_shutdown();
                return EXIT_FAILURE;
            }

            if (prop_exists && !(type && type[0])) {
                /* only care what the existing type is if the user
                 * didn't specify one */
                if (!xfconf_channel_get_property(channel, property_name, &value)) {
                    xfconf_query_printerr(_("Failed to get the existing type for the value"));
                    xfconf_shutdown();
                    return EXIT_FAILURE;
                }
            }

            if (!set_value[1] && !force_array) {
                /* not an array */
                GType gtype = G_TYPE_INVALID;

                if (prop_exists) {
                    gtype = G_VALUE_TYPE(&value);
                }

                if (type && type[0]) {
                    gtype = _xfconf_gtype_from_string(type[0]);
                }

                if (G_TYPE_INVALID == gtype || G_TYPE_NONE == gtype) {
                    xfconf_query_printerr(_("Unable to determine the type of the value"));
                    xfconf_query_list_types(TRUE);
                    xfconf_shutdown();
                    return EXIT_FAILURE;
                }

                if (G_TYPE_PTR_ARRAY == gtype) {
                    xfconf_query_printerr(_("A value type must be specified to change an array into a single value"));
                    xfconf_query_list_types(TRUE);
                    xfconf_shutdown();
                    return EXIT_FAILURE;
                }

                if (G_VALUE_TYPE(&value)) {
                    g_value_unset(&value);
                }

                g_value_init(&value, gtype);
                if (!_xfconf_gvalue_from_string(&value, set_value[0])) {
                    xfconf_query_printerr(_("Unable to convert \"%s\" to type \"%s\""),
                                          set_value[0], g_type_name(gtype));
                    xfconf_query_list_types(TRUE);
                    xfconf_shutdown();
                    return EXIT_FAILURE;
                }

                if (!xfconf_channel_set_property(channel, property_name, &value)) {
                    xfconf_query_printerr(_("Failed to set property"));
                    xfconf_shutdown();
                    return EXIT_FAILURE;
                }
            } else {
                /* array property */
                GPtrArray *arr_old = NULL, *arr_new;
                gint new_values = 0, new_types = 0;

                for (new_values = 0; set_value[new_values]; ++new_values)
                    ;
                if (type && type[0]) {
                    for (new_types = 0; type[new_types]; ++new_types)
                        ;
                } else if (G_VALUE_TYPE(&value) == G_TYPE_PTR_ARRAY) {
                    arr_old = g_value_get_boxed(&value);
                    new_types = arr_old->len;
                }

                if (new_values != new_types) {
                    xfconf_query_printerr(_("There are %d new values, but only %d types could be determined"),
                                          new_values, new_types);
                    xfconf_shutdown();
                    return EXIT_FAILURE;
                }

                arr_new = g_ptr_array_sized_new(new_values);

                for (i = 0; set_value[i]; ++i) {
                    GType gtype = G_TYPE_INVALID;
                    GValue *value_new;

                    if (arr_old) {
                        GValue *value_old = g_ptr_array_index(arr_old, i);
                        gtype = G_VALUE_TYPE(value_old);
                    } else {
                        gtype = _xfconf_gtype_from_string(type[i]);
                    }

                    if (G_TYPE_INVALID == gtype || G_TYPE_NONE == gtype) {
                        xfconf_query_printerr(_("Unable to determine type of value at index %d"), i);
                        xfconf_query_list_types(TRUE);
                        xfconf_shutdown();
                        return EXIT_FAILURE;
                    }

                    value_new = g_new0(GValue, 1);
                    g_value_init(value_new, gtype);
                    if (!_xfconf_gvalue_from_string(value_new, set_value[i])) {
                        xfconf_query_printerr(_("Unable to convert \"%s\" to type \"%s\""),
                                              set_value[i], g_type_name(gtype));
                        xfconf_query_list_types(TRUE);
                        xfconf_shutdown();
                        return EXIT_FAILURE;
                    }
                    g_ptr_array_add(arr_new, value_new);
                }

                if (G_VALUE_TYPE(&value)) {
                    g_value_unset(&value);
                }

                g_value_init(&value, G_TYPE_PTR_ARRAY);
                g_value_set_boxed(&value, arr_new);

                if (!xfconf_channel_set_property(channel, property_name, &value)) {
                    xfconf_query_printerr(_("Failed to set property"));
                    xfconf_shutdown();
                    return EXIT_FAILURE;
                }
            }
        }
    }

    if (list) {
        GHashTable *channel_contents = xfconf_channel_get_properties(channel, property_name);
        if (channel_contents) {
            gint *size = g_new0(gint, 2);
            GSList *sorted_contents = NULL;

            if (verbose) {
                g_hash_table_foreach(channel_contents, (GHFunc)xfconf_query_get_propname_size, size);
            }

            g_hash_table_foreach(channel_contents, (GHFunc)xfconf_query_list_sorted, &sorted_contents);

            xfconf_query_list_contents(sorted_contents, channel_contents, size[0], size[1]);

            g_free(size);
            g_slist_free(sorted_contents);
            g_hash_table_destroy(channel_contents);
        } else {
            g_print(_("Channel \"%s\" contains no properties"), channel_name);
            g_print(".\n");
        }
    }

    xfconf_shutdown();

    return EXIT_SUCCESS;
}
