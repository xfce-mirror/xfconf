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

#include <stdlib.h>
#include <glib.h>

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include "xfconf-gvaluefuncs.h"
#include "xfconf/xfconf.h"

static gboolean version = FALSE;
static gboolean verbose = FALSE;
static gchar *channel_name = NULL;
static gchar *property_name = NULL;
static gchar **set_value = NULL;

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
    {    "set", 's', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING_ARRAY, &set_value,
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
    if(!property_name)
    {
        g_print("No property specified, aborting...\n");
        return 1;
    }

    channel = xfconf_channel_new(channel_name);

    if (xfconf_channel_has_property(channel, property_name))
    {
        GValue value = {0, };
        xfconf_channel_get_property(channel, property_name, &value);

        /** Read value */
        if(set_value == NULL)
        {
            const gchar *str_val = _xfconf_string_from_gvalue(&value);
            g_print("%s\n", str_val);
        }
        /* Write value */
        else
        {
            if(_xfconf_gvalue_from_string(&value, set_value[0]))
            {
                xfconf_channel_set_property(channel, property_name, &value);
            }
            else
            {
                g_print("ERROR: Could not convert value\n");
            }
        }
        g_value_unset(&value);
    }

    return 0;
}
