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
static gboolean export = FALSE;
static gchar *export_filename = NULL;
static gboolean import = FALSE;
static gchar *import_filename = NULL;
static gboolean
xfconf_query_parse_option(const gchar *option_name,
                          const gchar *value,
                          gpointer data,
                          GError **error)
{
    if (g_strcmp0(option_name, "-v") == 0 || g_strcmp0(option_name, "--verbose") == 0) {
        verbose++;
    } else if (g_strcmp0(option_name, "--export") == 0) {
        export = TRUE;
        export_filename = g_strdup(value);
    } else if (g_strcmp0(option_name, "--import") == 0) {
        import = TRUE;
        import_filename = g_strdup(value);
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
    const GType gtypes[] = {
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
        G_TYPE_DOUBLE,
        G_TYPE_INVALID,
    };
    print_func(_("Possible types:"));
    print_func("\n");
    for (guint n = 0; gtypes[n] != G_TYPE_INVALID; n++) {
        print_func("%s\n", _xfconf_string_from_gtype(gtypes[n]));
    }
}

static gchar *
xfconf_query_g_strescape(gchar *str)
{
    /*
     * Escape spaces too, in order to build space-separated arguments: g_strcompress()
     * will restore them on its own. On the other hand, don't escape double quotes and
     * non-ASCII characters below, to keep the output as unchanged and human-readable
     * as possible.
     */
    const gchar *excluded =
        "\""
        "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f"
        "\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f"
        "\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf"
        "\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf"
        "\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf"
        "\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf"
        "\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef"
        "\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff";
    gchar *escaped = g_strescape(str, excluded);
    GString *string = g_string_new_take(escaped);
    g_string_replace(string, " ", "\\040", 0);
    return g_string_free_and_steal(string);
}

static gchar *
xfconf_query_export_string_from_gvalue(GValue *value)
{
    gchar *string;

    if (G_VALUE_TYPE(value) != G_TYPE_PTR_ARRAY) {
        gchar *value_str = _xfconf_string_from_gvalue(value);
        gchar *escaped = xfconf_query_g_strescape(value_str);
        const gchar *type_str = _xfconf_string_from_gtype(G_VALUE_TYPE(value));
        string = g_strdup_printf("0 %s %s", type_str, escaped);
        g_free(escaped);
        g_free(value_str);
    } else {
        GPtrArray *arr = g_value_get_boxed(value);
        gchar **strv = g_new0(gchar *, arr->len + 1);

        for (guint i = 0; i < arr->len; ++i) {
            GValue *item_value = g_ptr_array_index(arr, i);
            gchar *value_str = _xfconf_string_from_gvalue(item_value);
            gchar *escaped = xfconf_query_g_strescape(value_str);
            const gchar *type_str = _xfconf_string_from_gtype(G_VALUE_TYPE(item_value));
            strv[i] = g_strdup_printf("%s %s", type_str, escaped);
            g_free(escaped);
            g_free(value_str);
        }
        gchar *value_str = g_strjoinv(" ", strv);
        string = g_strdup_printf("%d %s", arr->len, value_str);
        g_free(value_str);
        g_strfreev(strv);
    }

    return string;
}

/*
 * One line per property to be exported, in the following format:
 *   channel property n type_0 escaped_value_0 … type_(n-1) escaped_value_(n-1)
 * where n is the array size (n == 0 for non-array properties).
 */
static gchar *
xfconf_query_export_channel(const gchar *_channel_name,
                            const gchar *_property_name)
{
    XfconfChannel *channel = xfconf_channel_get(_channel_name);
    GHashTable *channel_contents = xfconf_channel_get_properties(channel, _property_name);
    GHashTableIter iter;
    gpointer property, value;
    GString *string = g_string_new(NULL);

    g_hash_table_iter_init(&iter, channel_contents);
    while (g_hash_table_iter_next(&iter, &property, &value)) {
        gchar *value_str = xfconf_query_export_string_from_gvalue(value);
        g_string_append_printf(string, "%s %s %s\n", _channel_name, (gchar *)property, value_str);
        g_free(value_str);
    }

    g_hash_table_destroy(channel_contents);
    return g_string_free_and_steal(string);
}

static gboolean
xfconf_query_export(void)
{
    GError *error = NULL;
    gchar *contents = NULL;

    if (channel_name != NULL) {
        contents = xfconf_query_export_channel(channel_name, property_name);
    } else {
        gchar **channels = xfconf_list_channels();
        if (channels != NULL) {
            GString *string = g_string_new(NULL);
            for (guint i = 0; channels[i] != NULL; i++) {
                gchar *channel_export = xfconf_query_export_channel(channels[i], property_name);
                g_string_append(string, channel_export);
                g_free(channel_export);
            }
            contents = g_string_free_and_steal(string);
            g_strfreev(channels);
        }
    }

    if (contents != NULL) {
        if (export_filename != NULL) {
            if (!g_file_set_contents(export_filename, contents, -1, &error)) {
                xfconf_query_printerr(_("Failed to write contents: %s"), error->message);
                g_error_free(error);
                g_free(contents);
                g_free(export_filename);
                return FALSE;
            }
        } else {
            g_print("%s", contents);
        }
        g_free(contents);
    }

    g_free(export_filename);
    return TRUE;
}

static GHashTable *
xfconf_query_import_check(const gchar *contents)
{
    GHashTable *channels = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)g_hash_table_destroy);
    gchar **lines = g_strsplit(contents, "\n", 0);
    for (guint i = 0; lines[i] != NULL; i++) {
        if (xfce_str_is_empty(lines[i])) {
            continue;
        }

        gchar **words = g_strsplit(lines[i], " ", 0);
        guint n_words = g_strv_length(words);
        guint64 array_size;

        /* expected line format: channel property n type_0 escaped_value_0 … type_(n-1) escaped_value_(n-1) */
        if (n_words < 5
            || !g_ascii_string_to_unsigned(words[2], 10, 0, G_MAXUINT, &array_size, NULL)
            || n_words != 3 + 2 * (MAX(1, array_size)))
        {
            xfconf_query_printerr(_("Aborted on line %d: corrupted format"), i + 1);
            g_clear_pointer(&channels, g_hash_table_destroy);
            g_strfreev(words);
            break;
        }

        XfconfChannel *channel = xfconf_channel_get(words[0]);
        GHashTable *properties = g_hash_table_lookup(channels, channel);
        if (properties == NULL) {
            properties = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)_xfconf_gvalue_free);
            g_hash_table_insert(channels, channel, properties);
        }

        if (array_size == 0) {
            /* non-array property value */
            GType gtype = _xfconf_gtype_from_string(words[3]);
            if (gtype == G_TYPE_INVALID || gtype == G_TYPE_NONE) {
                xfconf_query_printerr(_("Aborted on line %d: wrong type %s"), i + 1, words[3]);
                g_clear_pointer(&channels, g_hash_table_destroy);
                g_strfreev(words);
                break;
            } else {
                gchar *str_value = g_strcompress(words[4]);
                GValue *value = g_new0(GValue, 1);
                g_value_init(value, gtype);
                if (!_xfconf_gvalue_from_string(value, str_value)) {
                    xfconf_query_printerr(_("Aborted on line %d: type and value do not match"), i + 1);
                    g_clear_pointer(&channels, g_hash_table_destroy);
                    _xfconf_gvalue_free(value);
                    g_free(str_value);
                    g_strfreev(words);
                    break;
                } else if (!g_hash_table_insert(properties, g_strdup(words[1]), value)) {
                    xfconf_query_printerr(_("Aborted on line %d: duplicate property %s"), i + 1, words[1]);
                    g_clear_pointer(&channels, g_hash_table_destroy);
                    g_free(str_value);
                    g_strfreev(words);
                    break;
                }
                g_free(str_value);
            }
        } else {
            /* array property value */
            GPtrArray *array = g_ptr_array_new_full(array_size, (GDestroyNotify)_xfconf_gvalue_free);
            for (guint j = 0; j < array_size; j++) {
                const gchar *str_type = words[2 + 2 * j + 1];
                GType gtype = _xfconf_gtype_from_string(str_type);
                if (gtype == G_TYPE_INVALID || gtype == G_TYPE_NONE) {
                    xfconf_query_printerr(_("Aborted on line %d: wrong type %s"), i + 1, str_type);
                    g_clear_pointer(&array, g_ptr_array_unref);
                    break;
                } else {
                    gchar *str_value = g_strcompress(words[2 + 2 * j + 2]);
                    GValue *value = g_new0(GValue, 1);
                    g_value_init(value, gtype);
                    if (!_xfconf_gvalue_from_string(value, str_value)) {
                        xfconf_query_printerr(_("Aborted on line %d: type and value do not match"), i + 1);
                        g_ptr_array_add(array, value);
                        g_clear_pointer(&array, g_ptr_array_unref);
                        g_free(str_value);
                        break;
                    }
                    g_ptr_array_add(array, value);
                    g_free(str_value);
                }
            }

            if (array != NULL) {
                GValue *value = g_new0(GValue, 1);
                g_value_init(value, G_TYPE_PTR_ARRAY);
                g_value_set_boxed(value, array);
                if (!g_hash_table_insert(properties, g_strdup(words[1]), value)) {
                    xfconf_query_printerr(_("Aborted on line %d: duplicate property %s"), i + 1, words[1]);
                    g_clear_pointer(&channels, g_hash_table_destroy);
                    g_strfreev(words);
                    break;
                }
            } else {
                g_clear_pointer(&channels, g_hash_table_destroy);
                g_strfreev(words);
                break;
            }
        }

        g_strfreev(words);
    }

    g_strfreev(lines);
    return channels;
}

static gboolean
xfconf_query_import(void)
{
    GFile *file = g_file_new_for_path(import_filename != NULL ? import_filename : "/dev/stdin");
    g_free(import_filename);
    GError *error = NULL;
    gchar *contents;
    if (!g_file_load_contents(file, NULL, &contents, NULL, NULL, &error)) {
        xfconf_query_printerr(_("Failed to read contents: %s"), error->message);
        g_error_free(error);
        g_object_unref(file);
        return FALSE;
    }

    g_object_unref(file);
    GHashTable *channels = xfconf_query_import_check(contents);
    if (channels == NULL) {
        g_free(contents);
        return FALSE;
    }

    g_free(contents);
    gboolean success = TRUE;
    GHashTableIter channel_iter;
    gpointer channel, properties;
    g_hash_table_iter_init(&channel_iter, channels);
    while (g_hash_table_iter_next(&channel_iter, &channel, &properties)) {
        GHashTableIter property_iter;
        gpointer property, value;
        g_hash_table_iter_init(&property_iter, properties);
        while (g_hash_table_iter_next(&property_iter, &property, &value)) {
            if (!xfconf_channel_set_property(channel, property, value)) {
                gchar *_channel_name;
                g_object_get(channel, "channel-name", &_channel_name, NULL);
                xfconf_query_printerr(_("Failed to set property %s in channel %s"), property, _channel_name);
                success = FALSE;
                g_free(_channel_name);
            }
        }
    }

    g_hash_table_destroy(channels);
    return success;
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
    { "export", '\0', G_OPTION_FLAG_IN_MAIN | G_OPTION_FLAG_OPTIONAL_ARG | G_OPTION_FLAG_FILENAME, G_OPTION_ARG_CALLBACK, xfconf_query_parse_option,
      N_("Export all or part of the settings to an optional file or standard output, in a format suitable for the --import option"),
      NULL },
    { "import", '\0', G_OPTION_FLAG_IN_MAIN | G_OPTION_FLAG_OPTIONAL_ARG | G_OPTION_FLAG_FILENAME, G_OPTION_ARG_CALLBACK, xfconf_query_parse_option,
      N_("Import settings exported using the --export option from an optional file or standard input"),
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
    if (!property_name && !list && !monitor && !export && !import) {
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

    if (export && import) {
        xfconf_query_printerr(_("--export and --import options can not be used together"));
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

    if (export) {
        gboolean success = xfconf_query_export();
        xfconf_shutdown();
        return success ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    if (import) {
        gboolean success = xfconf_query_import();
        xfconf_shutdown();
        return success ? EXIT_SUCCESS : EXIT_FAILURE;
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
        if (g_hash_table_size(channel_contents) > 0) {
            gint *size = g_new0(gint, 2);
            GSList *sorted_contents = NULL;

            if (verbose) {
                g_hash_table_foreach(channel_contents, (GHFunc)xfconf_query_get_propname_size, size);
            }

            g_hash_table_foreach(channel_contents, (GHFunc)xfconf_query_list_sorted, &sorted_contents);

            xfconf_query_list_contents(sorted_contents, channel_contents, size[0], size[1]);

            g_free(size);
            g_slist_free(sorted_contents);
        } else {
            g_print(_("Channel \"%s\" contains no properties"), channel_name);
            g_print(".\n");
        }
        g_hash_table_destroy(channel_contents);
    }

    xfconf_shutdown();

    return EXIT_SUCCESS;
}
