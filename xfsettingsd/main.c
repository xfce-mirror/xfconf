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

#include <X11/Xlib.h>

#include <glib.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <libxfce4util/libxfce4util.h>
#include <xfconf/xfconf.h>

#include "registry.h"

#define XF_DEBUG(str) \
    if (debug) g_print (str)

gboolean version = FALSE;
gboolean force_replace = FALSE;
gboolean running = FALSE;
gboolean debug = FALSE;

static GOptionEntry entries[] =
{
    {    "version", 'v', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &version,
        N_("Version information"),
        NULL
    },
    {    "verbose", 'V', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &version,
        N_("Verbose output"),
        NULL
    },
    {    "force", 'f', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &force_replace,
        N_("Force replace existing any xsettings daemon"),
        NULL
    },
    {    "debug", 'd', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &debug,
        N_("Start in debug mode (don't fork to the background)"),
        NULL
    },
    { NULL }
};


GdkFilterReturn
manager_event_filter (GdkXEvent *xevent,
                      GdkEvent *event,
                      gpointer data)
{
    XSettingsRegistry *registry = XSETTINGS_REGISTRY(data);

    if (xsettings_registry_process_event(registry, xevent))
    {
        gtk_main_quit();
        return GDK_FILTER_REMOVE;
    }
    else
    {
        return GDK_FILTER_CONTINUE;
    }
}

/**
 * settings_daemon_check_running:
 * @display: X11 Display object
 * @screen: X11 Screen number
 *
 * Return value: TRUE if an XSETTINGS daemon is already running
 */
gboolean
settings_daemon_check_running (Display *display, gint screen)
{
    Atom atom;
    gchar buffer[256];

    g_snprintf(buffer, sizeof(buffer), "_XSETTINGS_S%d", screen);
    atom = XInternAtom((Display *)display, buffer, False);

    if (XGetSelectionOwner((Display *)display, atom))
    {
        return TRUE;
    }
    else
        return FALSE;
}

int
main(int argc, char **argv)
{
    GError *cli_error = NULL;

    xfce_textdomain(GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    if(!gtk_init_with_args(&argc, &argv, _(""), entries, PACKAGE, &cli_error))
    {
        if (cli_error != NULL)
        {
            g_print (_("%s: %s\nTry %s --help to see a full list of available command line options.\n"), PACKAGE, cli_error->message, PACKAGE_NAME);
            g_error_free (cli_error);
            return 1;
        }
    }

    if(version)
    {
        g_print("xfsettingsd %s\n", PACKAGE_VERSION);
        return 0;
    }

    xfconf_init(NULL);

    gint screen = DefaultScreen(gdk_display);
        
    Window window = 0;

    running = settings_daemon_check_running(GDK_DISPLAY(), DefaultScreen(GDK_DISPLAY()));

    if (running)
    {
        XF_DEBUG("XSETTINGS Daemon running\n");
        if (force_replace)
        {
            XF_DEBUG("Replacing XSETTINGS daemon\n");
        }
        else
        {
            XF_DEBUG("Aborting...\n");
            return 1;
        }
    }

    if ((running && force_replace) || (!running))
    {
        XF_DEBUG("Initializing...\n");

        XfconfChannel *channel = xfconf_channel_new("xsettings");

        XSettingsRegistry *registry = xsettings_registry_new(channel, gdk_display, screen);
        
        xsettings_registry_load(registry, debug);

        xsettings_registry_notify(registry);

        gdk_window_add_filter(NULL, manager_event_filter, registry);
    }

    if(!debug) /* If not in debug mode, fork to background */
    {
        if(!fork())
        {
            gtk_main();
    
            XDestroyWindow (gdk_display, window);
            xfconf_shutdown();
        }
    }
    else
    {
        gtk_main();

        XDestroyWindow (gdk_display, window);
        xfconf_shutdown();
    }

    return 0;
}
