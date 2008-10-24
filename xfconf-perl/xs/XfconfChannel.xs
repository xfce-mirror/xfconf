/* NOTE: THIS FILE WAS POSSIBLY AUTO-GENERATED! */

/*
 * Copyright (c) 2008 Brian Tarricone <bjt23@cornell.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "xfconfperl.h"

#include <common/xfconf-gvaluefuncs.h>
#include <common/xfconf-common-private.h>


/* pulled in from xfconf/common/xfconf-gvaluefuncs.c, linked into
 * our perl shared lib in a really evil way */
extern GType _xfconf_gtype_from_string(const gchar *type);

static void
xfconf_perl_ghashtable_to_hv(gpointer key,
                             gpointer valuep,
                             gpointer data)
{
    GValue *value = (GValue *)valuep;
    HV *hv = (HV *)data;
    SV *val_sv = gperl_sv_from_value(value);

    hv_store(hv, (const char *)key, strlen(key), val_sv, 0);
}

MODULE = Xfce4::Xfconf::Channel    PACKAGE = Xfce4::Xfconf::Channel    PREFIX = xfconf_channel_

XfconfChannel *
xfconf_channel_new(class, channel_name)
        const gchar * channel_name
    C_ARGS:
        channel_name

gboolean
xfconf_channel_has_property(channel, property)
        XfconfChannel * channel
        const gchar * property

void
xfconf_channel_reset_property(channel, property_base, recursive=FALSE)
        XfconfChannel * channel
        const gchar * property_base
		gboolean recursive

HV *
xfconf_channel_get_properties(channel, property_base)
        XfconfChannel * channel
        const gchar * property_base
    PREINIT:
        GHashTable *properties = NULL;
    CODE:
        properties = xfconf_channel_get_properties(channel, property_base);
        if(!properties)
            RETVAL = (HV *)&PL_sv_undef;
        else {
            RETVAL = newHV();
            g_hash_table_foreach(properties,
                                 xfconf_perl_ghashtable_to_hv,
                                 RETVAL);
            sv_2mortal((SV *)RETVAL);
            g_hash_table_destroy(properties);
        }
	

gint32
xfconf_channel_get_int(channel, property, default_value=0)
        XfconfChannel * channel
        const gchar * property
        gint32 default_value

gboolean
xfconf_channel_set_int(channel, property, value)
        XfconfChannel * channel
        const gchar * property
        gint32 value

guint32
xfconf_channel_get_uint(channel, property, default_value=0)
        XfconfChannel * channel
        const gchar * property
        guint32 default_value

gboolean
xfconf_channel_set_uint(channel, property, value)
        XfconfChannel * channel
        const gchar * property
        guint32 value

guint64
xfconf_channel_get_uint64(channel, property, default_value=0)
        XfconfChannel * channel
        const gchar * property
        guint64 default_value

gboolean
xfconf_channel_set_uint64(channel, property, value)
        XfconfChannel * channel
        const gchar * property
        guint64 value

gdouble
xfconf_channel_get_double(channel, property, default_value=0.0)
        XfconfChannel * channel
        const gchar * property
        gdouble default_value

gboolean
xfconf_channel_set_double(channel, property, value)
        XfconfChannel * channel
        const gchar * property
        gdouble value

gboolean
xfconf_channel_get_bool(channel, property, default_value=FALSE)
        XfconfChannel * channel
        const gchar * property
        gboolean default_value

gboolean
xfconf_channel_set_bool(channel, property, value)
        XfconfChannel * channel
        const gchar * property
        gboolean value

gchar *
xfconf_channel_get_string(channel, property, default_value=NULL)
        XfconfChannel * channel
        const gchar * property
        const gchar * default_value

gboolean
xfconf_channel_set_string(channel, property, value)
        XfconfChannel * channel
        const gchar * property
        const gchar * value

AV *
xfconf_channel_get_string_list(channel, property)
        XfconfChannel * channel
        const gchar * property
    PREINIT:
        gchar **strs;
        gint i;
    CODE:
        strs = xfconf_channel_get_string_list(channel, property);
        if(!strs)
            RETVAL = (AV *)&PL_sv_undef;
        else {
            RETVAL = newAV();
            sv_2mortal((SV *)RETVAL);
            for(i = 0; strs[i]; ++i)
                av_store(RETVAL, i, newSVGChar(strs[i]));
            g_strfreev(strs);
        }

SV *
xfconf_channel_get_property(channel, property)
        XfconfChannel * channel
        const gchar * property
    PREINIT:
        GValue value = { 0, };
    CODE:
        if(!xfconf_channel_get_property(channel, property, &value))
            RETVAL = &PL_sv_undef;
        else {
            RETVAL = gperl_sv_from_value(&value);
            g_value_unset(&value);
        }

gboolean
xfconf_channel_set_property(channel, property, value)
        XfconfChannel * channel
        const gchar * property
        SV * value
    PREINIT:
        GValue val = { 0, };
    CODE:
        /* FIXME: figure out a non-lame way to implement this */
        RETVAL = FALSE;

void
xfconf_channel_get_array(channel, property)
        XfconfChannel * channel
        const gchar * property
    PREINIT:
        GPtrArray *arr;
        gint i;
    PPCODE:
        arr = xfconf_channel_get_arrayv(channel, property);
        if(arr) {
            for(i = 0; i < arr->len; ++i) {
                GValue *value = g_ptr_array_index(arr, i);
                XPUSHs(sv_2mortal(gperl_sv_from_value(value)));
            }
            xfconf_array_free(arr);
        }
         
gboolean
xfconf_channel_set_array(channel, property, ...)
        XfconfChannel * channel
        const gchar * property
    C_ARGS:
        property
    PREINIT:
        GPtrArray *arr;
        gint num_elements, i;
    CODE:
        RETVAL = FALSE;
        num_elements = items - 2;

        arr = g_ptr_array_sized_new(num_elements);
        for(i = 0; i < num_elements; ++i) {
            SV *val_pair_r = (SV *)ST(i+2);
            AV *val_pair;
            SV **type_sv, **val_sv;
            const gchar *type_str;
            GType gtype = G_TYPE_INVALID;
            GValue *value;

            if(!SvROK((SV *)val_pair_r) || SvTYPE(SvRV(val_pair_r)) != SVt_PVAV) {
                xfconf_array_free(arr);
                croak("Xfce4::Xfconf::set_array(): Value is not an array reference at index %d", i);
            }

            val_pair = (AV *)SvRV(val_pair_r);
            if(av_len(val_pair) != 1) {
                xfconf_array_free(arr);
                croak("Xfce4::Xfconf::set_array(): each value pair should have 2 elements (got %d)", av_len(val_pair)+1);
            }

            type_sv = av_fetch(val_pair, 0, 0);
            type_str = SvGChar(*type_sv);
            if(!type_str) {
                xfconf_array_free(arr);
                croak("Xfce4::Xfconf::set_array(): first element of value pair %d is not a string", i);
            }
            gtype = _xfconf_gtype_from_string(type_str);
            if(gtype == G_TYPE_INVALID || gtype == XFCONF_TYPE_G_VALUE_ARRAY) {
                xfconf_array_free(arr);
                croak("Xfce4::Xfconf::set_array(): value type of element at index %d (%s) is not valid", i, type_str);
            }

            val_sv = av_fetch(val_pair, 1, 0);
            if(!val_sv || !*val_sv || !SvOK(*val_sv)) {
                xfconf_array_free(arr);
                croak("Xfce4::Xfconf::set_array(): second element of value pair %d is not a valid value", i);
            }

            value = g_new0(GValue, 1);
            g_value_init(value, gtype);
            gperl_value_from_sv(value, *val_sv);

            g_ptr_array_add(arr, value);
        }

        RETVAL = xfconf_channel_set_arrayv(channel, property, arr);
        xfconf_array_free(arr);
