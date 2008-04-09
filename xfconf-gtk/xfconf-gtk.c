/*
 *  xfconf
 *
 *  Copyright (c) 2007 Brian Tarricone <bjt23@cornell.edu>
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

#ifdef GETTEXT_PACKAGE
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include "xfconf-gtk.h"
#include "xfconf-gvaluefuncs.h"

typedef struct
{
    GtkWidget *widget;
    XfconfChannel *channel;
    gchar *property;
    GType property_type;
    GCallback xfconf_callback;
    GCallback gtk_callback;
} XfconfGtkBinding;


static void
xfconf_gtk_channel_freed(gpointer data,
                         GObject *where_the_object_was)
{
    XfconfGtkBinding *binding = data;
    binding->channel = NULL;
    g_object_set_data(G_OBJECT(binding->widget), "--xfconf-gtk-binding",
                      NULL);
}

static void
xfconf_gtk_binding_free(XfconfGtkBinding *binding)
{
    if(binding->channel) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(binding->channel),
                                             binding->xfconf_callback,
                                             binding);
        g_object_weak_unref(G_OBJECT(binding->channel),
                            xfconf_gtk_channel_freed, binding);
    }

    if(binding->widget) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(binding->widget),
                                             binding->gtk_callback,
                                             binding);
    }

    g_free(binding->property);
    g_free(binding);
}



static gchar *
xfconf_gtk_string_from_gvalue(GValue *val)
{
    g_return_val_if_fail(val && G_VALUE_TYPE(val), NULL);

    switch(G_VALUE_TYPE(val)) {
        case G_TYPE_STRING:
            return g_value_dup_string(val);
        case G_TYPE_UCHAR:
            return g_strdup_printf("%u", (guint)g_value_get_uchar(val));
        case G_TYPE_CHAR:
            return g_strdup_printf("%d", (gint)g_value_get_char(val));
        case G_TYPE_UINT:
            return g_strdup_printf("%u", g_value_get_uint(val));
        case G_TYPE_INT:
            return g_strdup_printf("%d", g_value_get_int(val));
        case G_TYPE_UINT64:
            return g_strdup_printf("%" G_GUINT64_FORMAT,
                                   g_value_get_uint64(val));
        case G_TYPE_INT64:
            return g_strdup_printf("%" G_GINT64_FORMAT,
                                   g_value_get_int64(val));
        case G_TYPE_FLOAT:
            return g_strdup_printf("%f", (gdouble)g_value_get_float(val));
        case G_TYPE_DOUBLE:
            return g_strdup_printf("%f", g_value_get_double(val));
        case G_TYPE_BOOLEAN:
            return g_strdup(g_value_get_boolean(val) ? _("true")
                                                     : _("false"));
        default:
            if(G_VALUE_TYPE(val) == XFCONF_TYPE_UINT16) {
                return g_strdup_printf("%u",
                                       (guint)xfconf_g_value_get_uint16(val));
            } else if(G_VALUE_TYPE(val) == XFCONF_TYPE_INT16) {
                return g_strdup_printf("%d",
                                       (gint)xfconf_g_value_get_int16(val));
            }
            break;
    }

    g_warning("Unable to convert GValue to string");
    return NULL;
}

static gboolean
xfconf_gtk_update_property_from_string(XfconfGtkBinding *binding,
                                       const gchar *value)
{
    gboolean ret = FALSE;
    GValue val = { 0, };

    g_value_init(&val, binding->property_type);
    
    if(_xfconf_gvalue_from_string(&val, value)) {
        ret = xfconf_channel_set_property(binding->channel,
                                          binding->property,
                                          &val);
    } else {
        g_warning("Unable to convert string \"%s\" to GType \"%s\"",
                  value, G_VALUE_TYPE_NAME(&val));
    }

    g_value_unset(&val);

    return ret;
}

static void
xfconf_gtk_editable_binding(XfconfChannel *channel,
                            const gchar *property,
                            gpointer user_data)
{
    XfconfGtkBinding *binding = user_data;
    GValue val = { 0, };
    gchar *cur_val, *new_val;
    gint strpos = 0;

    if(strcmp(property, binding->property))
        return;

    if(!xfconf_channel_get_property(channel, property, &val))
        return;

    new_val = xfconf_gtk_string_from_gvalue(&val);
    g_value_unset(&val);
    if(!new_val)
        return;

    cur_val = gtk_editable_get_chars(GTK_EDITABLE(binding->widget), 0, -1);
    if(!strcmp(cur_val, new_val)) {
        g_free(cur_val);
        g_free(new_val);
        return;
    }
    g_free(cur_val);
    
    g_signal_handlers_block_by_func(G_OBJECT(binding->widget),
                                    binding->gtk_callback, binding);

    gtk_editable_delete_text(GTK_EDITABLE(binding->widget), 0, -1);
    gtk_editable_insert_text(GTK_EDITABLE(binding->widget), new_val,
                             strlen(new_val), &strpos);

    g_signal_handlers_unblock_by_func(G_OBJECT(binding->widget),
                                      binding->gtk_callback, binding);

    g_free(new_val);
}

static void
xfconf_gtk_editable_changed(GtkEditable *editable,
                            gpointer user_data)
{
    XfconfGtkBinding *binding = user_data;
    gchar *str;

    str = gtk_editable_get_chars(editable, 0, -1);
    xfconf_gtk_update_property_from_string(binding, str);
    g_free(str);
}



/**
 * xfconf_gtk_editable_bind_property:
 * @editable: A widget implementing #GtkEditable.
 * @channel: An #XfconfChannel.
 * @property: A string property name.
 * @property_type: The #GType of @property.
 *
 * Binds @editable to @property on @channel such that @editable will
 * always display the value of @property, even if that value changes
 * via any other means.  If @widget is editable, the binding will also
 * cause @property to be updated in the Xfconf configuration store.
 *
 * Note: #GtkEntry and #GtkSpinButton (and perhaps others) both
 *       implement the #GtkEditable interface and can be used with
 *       this function.
 **/
void
xfconf_gtk_editable_bind_property(GtkEditable *editable,
                                  XfconfChannel *channel,
                                  const gchar *property,
                                  GType property_type)
{
    XfconfGtkBinding *binding;
    GtkWidget *widget;

    g_return_if_fail(GTK_IS_EDITABLE(editable) && XFCONF_IS_CHANNEL(channel)
                     && property && *property);

    widget = GTK_WIDGET(editable);

    binding = g_new0(XfconfGtkBinding, 1);
    binding->widget = widget;
    binding->channel = channel;
    binding->property = g_strdup(property);
    binding->property_type = property_type;
    binding->xfconf_callback = G_CALLBACK(xfconf_gtk_editable_binding);
    binding->gtk_callback = G_CALLBACK(xfconf_gtk_editable_changed);

    /* set initial entry value */
    xfconf_gtk_editable_binding(channel, property, binding);

    g_signal_connect(G_OBJECT(widget), "changed",
                     binding->gtk_callback, binding);
    g_signal_connect(G_OBJECT(channel), "property-changed",
                     binding->xfconf_callback, binding);

    g_object_set_data_full(G_OBJECT(widget), "--xfconf-gtk-binding",
                           binding,
                           (GDestroyNotify)xfconf_gtk_binding_free);

    g_object_weak_ref(G_OBJECT(channel), xfconf_gtk_channel_freed,
                      binding);
}

/**
 * xfconf_gtk_widget_unbind:
 * @widget: A #GtkWidget.
 *
 * Causes a widget previously bound to an Xfconf property
 * (via xfconf_gtk_widget_bind_property()) to no longer be bound.
 **/
void
xfconf_gtk_widget_unbind(GtkWidget *widget)
{
    g_object_set_data(G_OBJECT(widget), "--xfconf-gtk-binding", NULL);
}
