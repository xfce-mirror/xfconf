/*
 *  Copyright (c) 2008 Stephan Arts <stephan@xfce.org>
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

#include <glib.h>

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include <xfconf/xfconf.h>

static gboolean version = FALSE;
static gboolean verbose = FALSE;
static gchar *channel_name = NULL;
static gchar *property_name = NULL;
static gchar **value = NULL;

static GOptionEntry entries[] =
{
    {    "version", 'v', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &version,
        N_("Version information"),
        NULL
    },
    {    "verbose", 'V', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &verbose,
        N_("Verbose output"),
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
    {    "set", 's', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING_ARRAY, &value,
        N_("set (change the value)"),
        NULL
    },
    { NULL }
};

int 
main(int argc, char **argv)
{
    GError *cli_error = NULL;
    XfconfChannel *channel = NULL;

    g_type_init();
    xfconf_init(NULL);
    
    GOptionContext *context = g_option_context_new("- xfconf commandline utility");

    g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);

    if(!g_option_context_parse(context, &argc, &argv, &cli_error))
    {
        g_print("option parsing failed: %s\n", cli_error->message);
        return 1;
    }

    /** Check if the channel is specified */
    if(!channel_name)
    {
        g_print("No channel specified, aborting...\n");
        return 1;
    }

    /** Check if the property is specified */
    if(!property_name)
    {
        g_print("No property specified, aborting...\n");
        return 1;
    }

    channel = xfconf_channel_new(channel_name);

    /** Check if the value is specified */
    if(!value)
    {
        /**
         * Read the property value
         */
        if (xfconf_channel_has_property(channel, property_name))
        {
            GValue tmp_value = {0, };
            xfconf_channel_get_property(channel, property_name, &tmp_value);

            GType prop_type = G_VALUE_TYPE(&tmp_value);
            switch(prop_type)
            {
                case G_TYPE_INT:
                    {
                        gint val = g_value_get_int(&tmp_value);
                        g_print("%d\n", val);
                    }
                    break;
                case G_TYPE_STRING:
                    {
                        const gchar *val = g_value_get_string(&tmp_value);
                        g_print("%s\n", val);
                    }
                    break;
                case G_TYPE_BOOLEAN:
                    {
                        gboolean val = g_value_get_boolean(&tmp_value);
                        g_print("%d\n", val);
                    }
                    break;
            }
            g_value_unset(&tmp_value);
        }
        else
        {
            g_print("[ERROR] Property doesn't exist\n");
            return 1;
        }
    }
    else
    {
        if (value[0] != NULL)
        {
            if (xfconf_channel_has_property(channel, property_name))
            {
                GValue tmp_value = {0, };
                xfconf_channel_get_property(channel, property_name, &tmp_value);

                GType prop_type = G_VALUE_TYPE(&tmp_value);
                if (value[1] != NULL)
                {
                    if(!g_strcasecmp(value[0], "int") && (prop_type == G_TYPE_INT))
                    {
                        gint i_val = atoi(value[1]);
                        xfconf_channel_set_int(channel, property_name, i_val);
                        return 0;
                    }
                    if(!g_strcasecmp(value[0], "string") && (prop_type == G_TYPE_STRING))
                    {
                        xfconf_channel_set_string(channel, property_name, value[1]);
                        return 0;

                    }
                    if(!g_strcasecmp(value[0], "bool") && (prop_type == G_TYPE_BOOLEAN))
                    {
                        if(!g_strcasecmp(value[1], "True") || !g_strcasecmp(value[1], "1"))
                        {
                            xfconf_channel_set_bool(channel, property_name, TRUE);
                            return 0;
                        }
                        if(!g_strcasecmp(value[1], "False") || !g_strcasecmp(value[1], "0"))
                        {
                            xfconf_channel_set_bool(channel, property_name, FALSE);
                            return 0;
                        }
                        return 1;

                    }
                }
                else
                {
                    switch (prop_type)
                    {
                        case G_TYPE_INT:
                            {
                            gint i_val = atoi(value[0]);
                            xfconf_channel_set_int(channel, property_name, i_val);
                            }
                        break;
                        case G_TYPE_STRING:
                            xfconf_channel_set_string(channel, property_name, value[0]);
                        break;
                        default:
                        break;
                    }

                }
                g_value_unset(&tmp_value);
                    
            }
            else
            {
                g_print("value not found");
            }
        }
        else
        {
            return 1;
        }
    }

    return 0;
}
