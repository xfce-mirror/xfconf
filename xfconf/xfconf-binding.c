/*
 *  xfconf
 *
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "xfconf-binding.h"
#include "xfconf-alias.h"

typedef struct
{
    XfconfChannel *channel;
    gchar *xfconf_property;
    GType xfconf_property_type;

    GObject *object;
    gchar *object_property;
    GType object_property_type;
} XfconfGBinding;

static void xfconf_g_binding_channel_destroyed(gpointer data,
                                               GObject *where_the_object_was);
static void xfconf_g_binding_object_destroyed(gpointer data,
                                              GObject *where_the_object_was);
static void xfconf_g_binding_channel_property_changed(XfconfChannel *channel,
                                                      const gchar *property,
                                                      gpointer user_data);
static void xfconf_g_binding_object_property_changed(GObject *object,
                                                     GParamSpec *pspec,
                                                     gpointer user_data);

static void
xfconf_g_binding_free(XfconfGBinding *binding)
{
    if(G_UNLIKELY(!binding))
        return;

    if(binding->object) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(binding->object),
                                            G_CALLBACK(xfconf_g_binding_object_property_changed),
                                             binding);
        g_object_weak_unref(G_OBJECT(binding->object),
                            xfconf_g_binding_object_destroyed,
                            binding);
    }

    if(binding->channel) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(binding->channel),
                                             G_CALLBACK(xfconf_g_binding_channel_property_changed),
                                             binding);
        g_object_weak_unref(G_OBJECT(binding->channel),
                            xfconf_g_binding_channel_destroyed,
                            binding);
    }

    g_free(binding->xfconf_property);
    g_free(binding->object_property);
    g_free(binding);
}

static void
xfconf_g_binding_channel_destroyed(gpointer data,
                                   GObject *where_the_object_was)
{
    XfconfGBinding *binding = data;
    binding->channel = NULL;
    xfconf_g_binding_free(binding);
}

static void
xfconf_g_binding_object_destroyed(gpointer data,
                                  GObject *where_the_object_was)
{
    XfconfGBinding *binding = data;
    GList *bindings = g_object_steal_data(G_OBJECT(binding->channel),
                                          "--xfconf-g-bindings");

    bindings = g_list_remove(bindings, binding);
    if(bindings) {
        g_object_set_data_full(G_OBJECT(binding->channel),
                               "--xfconf-g-bindings",
                               bindings, (GDestroyNotify)g_list_free);
    }

    binding->object = NULL;
    xfconf_g_binding_free(binding);
}

static void
xfconf_g_binding_channel_property_changed(XfconfChannel *channel,
                                          const gchar *property,
                                          gpointer user_data)
{
    XfconfGBinding *binding = user_data;
    GValue src_val = { 0, }, dst_val = { 0, };

    if(!xfconf_channel_get_property(channel, property, &src_val))
        return;

    g_value_init(&dst_val, binding->object_property_type);

    if(g_value_transform(&src_val, &dst_val)) {
        g_signal_handlers_block_by_func(binding->object,
                                        G_CALLBACK(xfconf_g_binding_object_property_changed),
                                        binding);
        g_object_set_property(binding->object, binding->object_property,
                              &dst_val);
        g_signal_handlers_unblock_by_func(binding->object,
                                          G_CALLBACK(xfconf_g_binding_object_property_changed),
                                          binding);
    }

    g_value_unset(&src_val);
    g_value_unset(&dst_val);
}

static void
xfconf_g_binding_object_property_changed(GObject *object,
                                         GParamSpec *pspec,
                                         gpointer user_data)
{
    XfconfGBinding *binding = user_data;
    GValue src_val = { 0, }, dst_val = { 0, };

    /* this can do auto-conversion for us, but we can't easily tell if
     * the conversion worked */
    g_value_init(&src_val, G_PARAM_SPEC_VALUE_TYPE(pspec));
    g_object_get_property(object, g_param_spec_get_name(pspec), &src_val);
    
    g_value_init(&dst_val, binding->xfconf_property_type);

    if(g_value_transform(&src_val, &dst_val)) {
        g_signal_handlers_block_by_func(G_OBJECT(binding->channel),
                                        G_CALLBACK(xfconf_g_binding_channel_property_changed),
                                        binding);
        xfconf_channel_set_property(binding->channel,
                                    binding->xfconf_property,
                                    &dst_val);
        g_signal_handlers_unblock_by_func(G_OBJECT(binding->channel),
                                          G_CALLBACK(xfconf_g_binding_channel_property_changed),
                                          binding);
    }

    g_value_unset(&src_val);
    g_value_unset(&dst_val);
}

/**
 * xfconf_g_property_bind:
 * @channel: An #XfconfChannel.
 * @xfconf_property: A property on @channel.
 * @xfconf_property_type: The type of @xfconf_property.
 * @object: A #GObject.
 * @object_property: A valid property on @object.
 *
 * Binds an Xfconf property to a #GObject property.  If the property
 * is changed via either the #GObject or Xfconf, the corresponding
 * property will also be updated.
 *
 * Note that @xfconf_property_type is required since @xfconf_property
 * may or may not already exist in the Xfconf store.  The type of
 * @object_property will be determined automatically.  If the two
 * types do not match, a conversion will be attempted.
 **/
void
xfconf_g_property_bind(XfconfChannel *channel,
                       const gchar *xfconf_property,
                       GType xfconf_property_type,
                       GObject *object,
                       const gchar *object_property)
{
    XfconfGBinding *binding;
    GParamSpec *pspec;
    gchar buf[1024];
    GList *bindings;

    g_return_if_fail(XFCONF_IS_CHANNEL(channel)
                     && xfconf_property && *xfconf_property
                     && xfconf_property_type != G_TYPE_NONE
                     && xfconf_property_type != G_TYPE_INVALID
                     && G_IS_OBJECT(object)
                     && object_property && *object_property);

    pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(object),
                                         object_property);
    if(!pspec) {
        g_warning("Property \"%s\" is not valid for GObject type \"%s\"",
                  object_property, G_OBJECT_TYPE_NAME(object));
        return;
    }

    if(!g_value_type_transformable(xfconf_property_type,
                                   G_PARAM_SPEC_VALUE_TYPE(pspec)))
    {
        g_warning("Converting from type \"%s\" to type \"%s\" is not supported",
                  g_type_name(xfconf_property_type),
                  g_type_name(G_PARAM_SPEC_VALUE_TYPE(pspec)));
        return;
    }

    if(!g_value_type_transformable(G_PARAM_SPEC_VALUE_TYPE(pspec),
                                   xfconf_property_type))
    {
        g_warning("Converting from type \"%s\" to type \"%s\" is not supported",
                  g_type_name(G_PARAM_SPEC_VALUE_TYPE(pspec)),
                  g_type_name(xfconf_property_type));
        return;
    }


    binding = g_new0(XfconfGBinding, 1);
    binding->channel = channel;
    binding->xfconf_property = g_strdup(xfconf_property);
    binding->xfconf_property_type = xfconf_property_type;
    binding->object = object;
    binding->object_property = g_strdup(object_property);
    binding->object_property_type = G_PARAM_SPEC_VALUE_TYPE(pspec);

    g_object_weak_ref(G_OBJECT(channel),
                      xfconf_g_binding_channel_destroyed,
                      binding);
    g_object_weak_ref(G_OBJECT(object),
                      xfconf_g_binding_object_destroyed,
                      binding);
    
    g_snprintf(buf, sizeof(buf), "property-changed::%s", xfconf_property);
    g_signal_connect(G_OBJECT(channel), buf,
                     G_CALLBACK(xfconf_g_binding_channel_property_changed),
                     binding);

    g_snprintf(buf, sizeof(buf), "notify::%s", object_property);
    g_signal_connect(G_OBJECT(object), buf,
                     G_CALLBACK(xfconf_g_binding_object_property_changed),
                     binding);

    bindings = g_object_get_data(G_OBJECT(channel), "--xfconf-g-bindings");
    if(bindings)
        bindings = g_list_append(bindings, binding);
    else {
        bindings = g_list_append(bindings, binding);
        g_object_set_data_full(G_OBJECT(channel), "--xfconf-g-bindings",
                               bindings, (GDestroyNotify)g_list_free);
    }

    xfconf_g_binding_channel_property_changed(channel, xfconf_property,
                                              binding);
}

/**
 * xfconf_g_property_unbind:
 * @channel: An #XfconfChannel.
 * @xfconf_property: A bound property on @channel.
 * @object: A #GObject.
 * @object_property: A bound property on @object.
 *
 * Causes an Xfconf channel previously bound to a #GObject property
 * (see xfconf_g_property_bind()) to no longer be bound.
 **/
void
xfconf_g_property_unbind(XfconfChannel *channel,
                         const gchar *xfconf_property,
                         GObject *object,
                         const gchar *object_property)
{
    GList *bindings = g_object_steal_data(G_OBJECT(channel),
                                          "--xfconf-g-bindings");
    GList *l;

    for(l = bindings; l; l = l->next) {
        XfconfGBinding *binding = l->data;

        if(object == binding->object
           && !strcmp(xfconf_property, binding->xfconf_property)
           && !strcmp(object_property, binding->object_property))
        {
            bindings = g_list_delete_link(bindings, l);
            xfconf_g_binding_free(binding);
            break;
        }
    }

    if(bindings) {
        g_object_set_data_full(G_OBJECT(channel), "--xfconf-g-bindings",
                               bindings, (GDestroyNotify)g_list_free);
    }
}



#define __XFCONF_BINDING_C__
#include "common/xfconf-aliasdef.c"
