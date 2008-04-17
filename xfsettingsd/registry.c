/*
 * Copyright © 2001 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Red Hat not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Red Hat makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * RED HAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL RED HAT
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Owen Taylor, Red Hat, Inc.
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
 *
 *          Stephan Arts <stephan@xfce.org>: adapted to the "xfconf" concept
 */

#include <config.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>

#include <glib.h>

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <string.h>

#include <xfconf/xfconf.h>

#include "registry.h"

#define XSETTINGS_PAD(n,m) ((n + m - 1) & (~(m-1)))

#define XSETTINGS_DEBUG(str) \
    if (debug) g_print ("%s", str)
#define XSETTINGS_DEBUG_CREATE(str) \
    if (debug) g_print ("Creating Property: '%s'\n", str)
#define XSETTINGS_DEBUG_LOAD(str) \
    if (debug) g_print ("Loading Property: '%s'\n", str)

G_DEFINE_TYPE(XSettingsRegistry, xsettings_registry, G_TYPE_OBJECT);

struct _XSettingsRegistryPriv
{
    XSettingsRegistryEntry **properties;

    gint serial;
    gint last_change_serial;

    /* props */
    XfconfChannel *channel;
    gint screen;
    Display *display;
    Window window;
    Atom xsettings_atom;
};

static void xsettings_registry_set_property(GObject*, guint, const GValue*, GParamSpec*);
static void xsettings_registry_get_property(GObject*, guint, GValue*, GParamSpec*);

static void
cb_xsettings_registry_channel_property_changed(XfconfChannel *channel, const gchar *property_name, XSettingsRegistry *registry); 
static Bool
timestamp_predicate (Display *display, XEvent  *xevent, XPointer arg);

static XSettingsRegistryEntry *
xsettings_registry_entry_new_string(const gchar *name, const gchar *value);
static XSettingsRegistryEntry *
xsettings_registry_entry_new_int(const gchar *name, gint value);
static XSettingsRegistryEntry *
xsettings_registry_entry_new_bool(const gchar *name, gboolean value);

#define XSETTINGS_REGISTRY_SIZE 24

enum {
	XSETTINGS_REGISTRY_PROPERTY_CHANNEL = 1,
    XSETTINGS_REGISTRY_PROPERTY_DISPLAY,
    XSETTINGS_REGISTRY_PROPERTY_SCREEN,
    XSETTINGS_REGISTRY_PROPERTY_XSETTINGS_ATOM,
    XSETTINGS_REGISTRY_PROPERTY_WINDOW
};

void
xsettings_registry_class_init(XSettingsRegistryClass *reg_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(reg_class);
	GParamSpec *pspec;

	object_class->set_property = xsettings_registry_set_property;
	object_class->get_property = xsettings_registry_get_property;


	pspec = g_param_spec_object("channel", NULL, NULL, XFCONF_TYPE_CHANNEL, G_PARAM_READABLE|G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(object_class, XSETTINGS_REGISTRY_PROPERTY_CHANNEL, pspec);

	pspec = g_param_spec_int("screen", NULL, NULL, -1, 65535, -1, G_PARAM_READABLE|G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(object_class, XSETTINGS_REGISTRY_PROPERTY_SCREEN, pspec);

	pspec = g_param_spec_pointer("display", NULL, NULL, G_PARAM_READABLE|G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(object_class, XSETTINGS_REGISTRY_PROPERTY_DISPLAY, pspec);

	pspec = g_param_spec_long("xsettings_atom", NULL, NULL, G_MINLONG, G_MAXLONG, 0, G_PARAM_READABLE|G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(object_class, XSETTINGS_REGISTRY_PROPERTY_XSETTINGS_ATOM, pspec);

	pspec = g_param_spec_long("window", NULL, NULL, G_MINLONG, G_MAXLONG, 0, G_PARAM_READABLE|G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(object_class, XSETTINGS_REGISTRY_PROPERTY_WINDOW, pspec);

}

void
xsettings_registry_init(XSettingsRegistry *registry)
{
    registry->priv = g_new0(XSettingsRegistryPriv, 1);
    registry->priv->properties = g_new0(XSettingsRegistryEntry *, XSETTINGS_REGISTRY_SIZE);

    gint i = XSETTINGS_REGISTRY_SIZE;
    /* Net settings */
    registry->priv->properties[--i] = xsettings_registry_entry_new_int("Net/DoubleClickTime", 250);
    registry->priv->properties[--i] = xsettings_registry_entry_new_int("Net/DoubleClickDistance", 5);
    registry->priv->properties[--i] = xsettings_registry_entry_new_int("Net/DndDragThreshold", 8);
    registry->priv->properties[--i] = xsettings_registry_entry_new_bool("Net/CursorBlink", TRUE);
    registry->priv->properties[--i] = xsettings_registry_entry_new_int("Net/CursorBlinkTime", 1200);
    registry->priv->properties[--i] = xsettings_registry_entry_new_string("Net/ThemeName", "Default");
    registry->priv->properties[--i] = xsettings_registry_entry_new_string("Net/IconThemeName", "hicolor");
    /* Xft settings */
    registry->priv->properties[--i] = xsettings_registry_entry_new_int("Xft/Antialias", -1);
    registry->priv->properties[--i] = xsettings_registry_entry_new_int("Xft/Hinting", -1);
    registry->priv->properties[--i] = xsettings_registry_entry_new_string("Xft/HintStyle", "hintnone");
    registry->priv->properties[--i] = xsettings_registry_entry_new_string("Xft/RGBA", "none");
    registry->priv->properties[--i] = xsettings_registry_entry_new_int("Xft/DPI", -1);
    /* GTK settings */
    registry->priv->properties[--i] = xsettings_registry_entry_new_bool("Gtk/CanChangeAccels", FALSE);
    registry->priv->properties[--i] = xsettings_registry_entry_new_string("Gtk/ColorPalette", 
                    "black:white:gray50:red:purple:blue:light "
                    "blue:green:yellow:orange:lavender:brown:goldenrod4:dodger "
                    "blue:pink:light green:gray10:gray30:gray75:gray90");
    registry->priv->properties[--i] = xsettings_registry_entry_new_string("Gtk/FontName", "Sans 10");
    registry->priv->properties[--i] = xsettings_registry_entry_new_string("Gtk/IconSizes", NULL);
    registry->priv->properties[--i] = xsettings_registry_entry_new_string("Gtk/KeyThemeName", NULL);
    registry->priv->properties[--i] = xsettings_registry_entry_new_string("Gtk/ToolbarStyle", "Icons");
    registry->priv->properties[--i] = xsettings_registry_entry_new_int("Gtk/ToolbarIconSize", 3);
    registry->priv->properties[--i] = xsettings_registry_entry_new_string("Gtk/IMPreeditStyle", "");
    registry->priv->properties[--i] = xsettings_registry_entry_new_string("Gtk/IMStatusStyle", "");
    registry->priv->properties[--i] = xsettings_registry_entry_new_bool("Gtk/MenuImages", TRUE);
    registry->priv->properties[--i] = xsettings_registry_entry_new_bool("Gtk/ButtonImages", TRUE);
    registry->priv->properties[--i] = xsettings_registry_entry_new_string("Gtk/MenuBarAccel", "F10");

#ifdef DEBUG
    if (i != 0)
        g_critical ("XSETTINGS_REGISTRY_SIZE != number of registry items");
#endif

}

static void
cb_xsettings_registry_channel_property_changed(XfconfChannel *channel, const gchar *name, XSettingsRegistry *registry)
{
    gint i;

    for (i = 0; i < XSETTINGS_REGISTRY_SIZE; ++i)
    {
        XSettingsRegistryEntry *entry = registry->priv->properties[i];
        if (!strcmp(entry->name, &name[1]))
        {
            switch (G_VALUE_TYPE(entry->value))
            {
                case G_TYPE_INT:
                    g_value_set_int(entry->value, xfconf_channel_get_int(channel, name, g_value_get_int(entry->value)));
                    break;
                case G_TYPE_STRING:
                    g_value_set_string(entry->value, xfconf_channel_get_string(channel, name, g_value_get_string(entry->value)));
                    break;
                case G_TYPE_BOOLEAN:
                    g_value_set_boolean(entry->value, xfconf_channel_get_bool(channel, name, g_value_get_boolean(entry->value)));
                    break;
            }
        }
    }
    xsettings_registry_notify(registry);
}

void
xsettings_registry_notify(XSettingsRegistry *registry)
{
    registry->priv->last_change_serial = registry->priv->serial;

    XSettingsRegistryEntry *entry = NULL;

    gint buf_len = 12;
    gint i;

    /** Calculate buffer size */
    for(i = 0; i < XSETTINGS_REGISTRY_SIZE; ++i)
    {
        entry = registry->priv->properties[i];
        buf_len += 8 + XSETTINGS_PAD(strlen(entry->name), 4);
        switch (G_VALUE_TYPE(entry->value))
        {
            case G_TYPE_INT:
            case G_TYPE_BOOLEAN:
                buf_len += 4;
                break;
            case G_TYPE_STRING:
                {
                    buf_len += 4;
                    const gchar *value = g_value_get_string(entry->value);
                    if(value)
                    {
                        buf_len += XSETTINGS_PAD(strlen(value), 4);
                    }

                }
                break;
            case G_TYPE_UINT64:
                buf_len += 8;
                break;
        }
    }

    guchar *buffer = NULL;
    guchar *pos = buffer = g_new0(guchar, buf_len);

    *(CARD32 *)pos = LSBFirst;
    pos +=4;

    *(CARD32 *)pos = registry->priv->serial++;
    pos += 4;

    *(CARD32 *)pos = XSETTINGS_REGISTRY_SIZE;
    pos += 4;

    /** Fill the buffer */
    for(i = 0; i < XSETTINGS_REGISTRY_SIZE; ++i)
    {
        entry = registry->priv->properties[i];

        gint name_len = XSETTINGS_PAD(strlen(entry->name), 4);
        gint value_len = 0;

        switch (G_VALUE_TYPE(entry->value))
        {
            case G_TYPE_INT:
            case G_TYPE_BOOLEAN:
                *pos++ = 0;
                break;
            case G_TYPE_STRING:
                *pos++ = 1; // String 
                {
                    const gchar *value = g_value_get_string(entry->value);
                    if(value)
                    {
                        value_len = XSETTINGS_PAD(strlen(value), 4);
                    }
                    else
                    {
                        value_len = 0;
                    }
                }
                break;
            case G_TYPE_UINT64: /* Color is a 64-bits value */
                *pos++ = 2;
                break;
        }
        *pos++ = 0;

        gint str_length = strlen(entry->name);
        *(CARD16 *)pos = str_length;
        pos += 2;
        memcpy (pos, entry->name, str_length);
        name_len -= str_length;
        pos += str_length;

        while(name_len > 0)
        {
            *(pos++) = 0;
            name_len--;
        }
        
        *(CARD32 *)pos = registry->priv->last_change_serial; 
        pos+= 4;

        switch (G_VALUE_TYPE(entry->value))
        {
            case G_TYPE_STRING:
                {
                    const gchar *val = g_value_get_string(entry->value);

                    if (val)
                    {
                        *(CARD32 *)pos = strlen(val);
                        pos += 4;

                        memcpy (pos, val, strlen(val));
                        pos += strlen(val);
                        value_len -= strlen(val);
                    }
                    else
                    {
                        *(CARD32 *)pos = 0;
                        pos += 4;
                    }
                }
                while(value_len > 0)
                {
                    *(pos++) = 0;
                    value_len--;
                }
                break;
            case G_TYPE_INT:
                *(CARD32 *)pos = g_value_get_int(entry->value);
                pos += 4;
                break;
            case G_TYPE_BOOLEAN:
                *(CARD32 *)pos = g_value_get_boolean(entry->value);
                pos += 4;
                break;
            case G_TYPE_UINT64:

                pos += 8;
                break;
        }

    }

    XChangeProperty(registry->priv->display,
                    registry->priv->window,
                    registry->priv->xsettings_atom,
                    registry->priv->xsettings_atom,
                    8, PropModeReplace, buffer, buf_len);

    registry->priv->last_change_serial = registry->priv->serial;
}

static XSettingsRegistryEntry *
xsettings_registry_entry_new_string(const gchar *name, const gchar *value)
{
    XSettingsRegistryEntry *entry = g_new0(XSettingsRegistryEntry, 1);
    entry->name = g_strdup(name);

    entry->value = g_new0(GValue, 1);
    entry->value = g_value_init(entry->value, G_TYPE_STRING);
    g_value_set_string(entry->value, value);

    return entry;
}

static XSettingsRegistryEntry *
xsettings_registry_entry_new_int(const gchar *name, gint value)
{
    XSettingsRegistryEntry *entry = g_new0(XSettingsRegistryEntry, 1);
    entry->name = g_strdup(name);
    entry->value = g_new0(GValue, 1);
    entry->value = g_value_init(entry->value, G_TYPE_INT);
    g_value_set_int(entry->value, value);

    return entry;
}

static XSettingsRegistryEntry *
xsettings_registry_entry_new_bool(const gchar *name, gboolean value)
{
    XSettingsRegistryEntry *entry = g_new0(XSettingsRegistryEntry, 1);
    entry->name = g_strdup(name);

    entry->value = g_new0(GValue, 1);
    entry->value = g_value_init(entry->value, G_TYPE_BOOLEAN);
    g_value_set_boolean(entry->value, value);

    return entry;
}

XSettingsRegistry *
xsettings_registry_new (XfconfChannel *channel, Display *dpy, gint screen)
{
    Atom xsettings_atom = XInternAtom(dpy, "_XSETTINGS_SETTINGS", True);

    Window window = 0;

    window = XCreateSimpleWindow (dpy,
					 RootWindow (dpy, screen),
					 0, 0, 10, 10, 0,
					 WhitePixel (dpy, screen),
					 WhitePixel (dpy, screen));
    if (!window)
    {
        g_critical( "no window");
        return NULL;
    }

    GObject *object = g_object_new(XSETTINGS_REGISTRY_TYPE,
                                   "channel", channel,
                                   "display", dpy,
                                   "screen", screen,
                                   "xsettings_atom", xsettings_atom,
                                   "window", window,
                                   NULL);
    gchar buffer[256];
    unsigned char c = 'a';
    TimeStampInfo info;
    XEvent xevent;

    g_snprintf(buffer, sizeof(buffer), "_XSETTINGS_S%d", screen);
    Atom selection_atom = XInternAtom(dpy, buffer, True);
    Atom manager_atom = XInternAtom(dpy, "MANAGER", True);

    info.timestamp_prop_atom = XInternAtom(dpy, "_TIMESTAMP_PROP", False);
    info.window = window;

    XSelectInput (dpy, window, PropertyChangeMask);

    XChangeProperty (dpy, window,
       info.timestamp_prop_atom, info.timestamp_prop_atom,
       8, PropModeReplace, &c, 1);

    XIfEvent (dpy, &xevent,
              timestamp_predicate, (XPointer)&info); 

    XSetSelectionOwner (dpy, selection_atom,
                        window, xevent.xproperty.time);

    if (XGetSelectionOwner (dpy, selection_atom) ==
        window)
    {
        XClientMessageEvent xev;

        xev.type = ClientMessage;
        xev.window = RootWindow (dpy, screen);
        xev.message_type = manager_atom;
        xev.format = 32;
        xev.data.l[0] = xevent.xproperty.time;
        xev.data.l[1] = selection_atom;
        xev.data.l[2] = window;
        xev.data.l[3] = 0;	/* manager specific data */
        xev.data.l[4] = 0;	/* manager specific data */

        XSendEvent (dpy, RootWindow (dpy, screen),
          False, StructureNotifyMask, (XEvent *)&xev);
    }
    else
    {
        g_debug("fail");
    }

    return XSETTINGS_REGISTRY(object);
}

static void
xsettings_registry_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *p_spec)
{
	switch(property_id)
	{
		case XSETTINGS_REGISTRY_PROPERTY_CHANNEL:
            if (XSETTINGS_REGISTRY(object)->priv->channel)
            {
                XfconfChannel *channel = XSETTINGS_REGISTRY(object)->priv->channel;

                g_signal_handlers_disconnect_by_func(G_OBJECT(channel), (GCallback)cb_xsettings_registry_channel_property_changed, object);
                XSETTINGS_REGISTRY(object)->priv->channel = NULL;
            }

			XSETTINGS_REGISTRY(object)->priv->channel = g_value_get_object(value);

            if (XSETTINGS_REGISTRY(object)->priv->channel)
            {
                XfconfChannel *channel = XSETTINGS_REGISTRY(object)->priv->channel;

                g_signal_connect(G_OBJECT(channel), "property-changed", (GCallback)cb_xsettings_registry_channel_property_changed, object);
            }
		    break;
		case XSETTINGS_REGISTRY_PROPERTY_SCREEN:
            XSETTINGS_REGISTRY(object)->priv->screen = g_value_get_int(value);
            break;
		case XSETTINGS_REGISTRY_PROPERTY_DISPLAY:
            XSETTINGS_REGISTRY(object)->priv->display = g_value_get_pointer(value);
            break;
        case XSETTINGS_REGISTRY_PROPERTY_XSETTINGS_ATOM:
            XSETTINGS_REGISTRY(object)->priv->xsettings_atom = g_value_get_long(value);
            break;
        case XSETTINGS_REGISTRY_PROPERTY_WINDOW:
            XSETTINGS_REGISTRY(object)->priv->window = g_value_get_long(value);
            break;
	}

}

static void
xsettings_registry_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *p_spec)
{
	switch(property_id)
	{
		case XSETTINGS_REGISTRY_PROPERTY_CHANNEL:
			g_value_set_object(value, XSETTINGS_REGISTRY(object)->priv->channel);
		break;
		case XSETTINGS_REGISTRY_PROPERTY_SCREEN:
            g_value_set_int(value, XSETTINGS_REGISTRY(object)->priv->screen);
        break;
		case XSETTINGS_REGISTRY_PROPERTY_DISPLAY:
            g_value_set_pointer(value, XSETTINGS_REGISTRY(object)->priv->display);
        break;
        case XSETTINGS_REGISTRY_PROPERTY_XSETTINGS_ATOM:
            g_value_set_long(value, XSETTINGS_REGISTRY(object)->priv->xsettings_atom);
        break;
        case XSETTINGS_REGISTRY_PROPERTY_WINDOW:
            g_value_set_long(value, XSETTINGS_REGISTRY(object)->priv->window);
        break;
	}

}

static Bool
timestamp_predicate (Display *display,
		     XEvent  *xevent,
		     XPointer arg)
{
  TimeStampInfo *info = (TimeStampInfo *)arg;

  if (xevent->type == PropertyNotify &&
      xevent->xproperty.window == info->window &&
      xevent->xproperty.atom == info->timestamp_prop_atom)
    return True;

  return False;
}

gboolean
xsettings_registry_load(XSettingsRegistry *registry, gboolean debug)
{
    gint i;
    XfconfChannel *channel = registry->priv->channel;

    for(i = 0; i < XSETTINGS_REGISTRY_SIZE ; ++i)
    {
        XSettingsRegistryEntry *entry = registry->priv->properties[i];

        gchar *name = g_strconcat("/", entry->name, NULL);

        if (xfconf_channel_has_property(channel, name) == TRUE)
        {
            XSETTINGS_DEBUG_LOAD(entry->name);
            switch (G_VALUE_TYPE(entry->value))
            {
                case G_TYPE_INT:
                    g_value_set_int(entry->value, xfconf_channel_get_int(channel, name, g_value_get_int(entry->value)));
                    break;
                case G_TYPE_STRING:
                    g_value_set_string(entry->value, xfconf_channel_get_string(channel, name, g_value_get_string(entry->value)));
                    break;
                case G_TYPE_BOOLEAN:
                    g_value_set_boolean(entry->value, xfconf_channel_get_bool(channel, name, g_value_get_boolean(entry->value)));
                    break;
            }
        }
        else
        {
            XSETTINGS_DEBUG_CREATE(entry->name);

            if(xfconf_channel_set_property(channel, name, entry->value))
            {
                XSETTINGS_DEBUG("... OK\n");
            }
            else
            {
                XSETTINGS_DEBUG("... FAIL\n");
            }
        }

        g_free(name);

    }

    return TRUE;
}
