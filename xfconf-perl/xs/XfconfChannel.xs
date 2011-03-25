/*
 * Copyright (c) 2008 Brian Tarricone <bjt23@cornell.edu>
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

#include "xfconfperl.h"

#include <common/xfconf-gvaluefuncs.h>
#include <common/xfconf-common-private.h>


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
PROTOTYPES: ENABLE

XfconfChannel *
xfconf_channel_new(class, channel_name)
        const gchar * channel_name
    C_ARGS:
        channel_name

XfconfChannel *
xfconf_channel_new_with_property_base(class, channel_name, property_base)
        const gchar * channel_name
        const gchar * property_base
    C_ARGS:
        channel_name, 
        property_base

gboolean
xfconf_channel_has_property(channel, property)
        XfconfChannel * channel
        const gchar * property

gboolean
xfconf_channel_is_property_locked(channel, property)
        XfconfChannel * channel
        const gchar * property

void
xfconf_channel_reset_property(channel, property_base, recursive=FALSE)
        XfconfChannel * channel
        const gchar * property_base
		gboolean recursive

HV *
xfconf_channel_get_properties(channel, property_base=NULL)
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
        ST(0) = (SV *)RETVAL;  /* why isn't xsubpp doing this for us? */
	
void
xfconf_channel_get_property(channel, property, default_value=NULL)
        XfconfChannel * channel
        const gchar * property
        SV * default_value
    PREINIT:
        GValue val = { 0, };
    PPCODE:
        if(!xfconf_channel_get_property(channel, property, &val)) {
            XPUSHs((default_value ? default_value : &PL_sv_undef));
        } else if(G_VALUE_TYPE(&val) == XFCONF_TYPE_G_VALUE_ARRAY) {
            GPtrArray *arr = g_value_get_boxed(&val);
            gint i;

            EXTEND(SP, arr->len);
            for(i = 0; i < arr->len; ++i) {
                GValue *arrval = g_ptr_array_index(arr, i);
                PUSHs(sv_2mortal(gperl_sv_from_value(arrval)));
            }
            g_value_unset(&val);
        } else {
            XPUSHs(gperl_sv_from_value(&val));
            g_value_unset(&val);
        }

gboolean
_set_property(channel, property, value, arraytypes=NULL)
        XfconfChannel * channel
        const gchar * property
        SV * value
        SV * arraytypes
    ALIAS:
        set_uchar = 0
        set_char = 1
        set_uint16 = 2
        set_int16 = 3
        set_uint = 4
        set_int = 5
        set_uint64 = 6
        set_int64 = 7
        set_float = 8
        set_double = 9
        set_bool = 10
        set_string = 11
        set_array = 12
    PREINIT:
        GType gtype = G_TYPE_INVALID;
        GValue val = { 0 , };
    CODE:
        RETVAL = FALSE;
        switch(ix) {
            case 0:  gtype = G_TYPE_UCHAR; break;
            case 1:  gtype = G_TYPE_CHAR; break;
            case 2:  gtype = XFCONF_TYPE_UINT16; break;
            case 3:  gtype = XFCONF_TYPE_INT16; break;
            case 4:  gtype = G_TYPE_UINT; break;
            case 5:  gtype = G_TYPE_INT; break;
            case 6:  gtype = G_TYPE_UINT64; break;
            case 7:  gtype = G_TYPE_INT64; break;
            case 8:  gtype = G_TYPE_FLOAT; break;
            case 9:  gtype = G_TYPE_DOUBLE; break;
            case 10: gtype = G_TYPE_BOOLEAN; break;
            case 11: gtype = G_TYPE_STRING; break;
            case 12: gtype = XFCONF_TYPE_G_VALUE_ARRAY; break;
        }

        if(gtype != XFCONF_TYPE_G_VALUE_ARRAY) {
            g_value_init(&val, gtype);
            gperl_value_from_sv(&val, value);
            RETVAL = xfconf_channel_set_property(channel, property, &val);
            g_value_unset(&val);
        } else {
            GPtrArray *arr;
            AV *av_values, *av_types;
            gint num_elements, i;

            if(!SvROK(value) || (arraytypes && !SvROK(arraytypes)))
                croak("Usage: Xfce4::Xfconf::Channel::set_array($property, \\@values, \\@types=NULL)");

            av_values = (AV *)SvRV(value);
            av_types = arraytypes ? (AV *)SvRV(arraytypes) : NULL;

            if(av_len(av_values) != av_len(av_types))
                croak("Xfce4::Xfconf::Channel::set_array(): values array is not the same size as the types array");

            num_elements = av_len(av_values) + 1;
            arr = g_ptr_array_sized_new(num_elements);
            for(i = 0; i < num_elements; ++i) {
                SV **val_sv = av_fetch(av_values, i, 0);
                GValue *arrval;

                gtype = G_TYPE_INVALID;

                if(!val_sv || !*val_sv || !SvOK(*val_sv) || SvROK(*val_sv)) {
                    xfconf_array_free(arr);
                    croak("Xfce4::Xfconf::Channel::set_array(): invalid value at index %d", i);
                }

                if(av_types) {
                    SV **type_sv = av_fetch(av_types, i, 0);
                    if(type_sv && *type_sv && SvOK(*type_sv))
                        gtype = _xfconf_gtype_from_string(SvGChar(*type_sv));
                }

                if(gtype == G_TYPE_INVALID) {
                    if(av_types)
                        warn("Xfce4::Xfconf::Channel::set_array(): unable to determine type at index %d; guessing", i);
                    if(SvNOKp(*val_sv))
                        gtype = G_TYPE_DOUBLE;
                    else if(SvIOKp(*val_sv))
                        gtype = G_TYPE_INT;
                    else
                        gtype = G_TYPE_STRING;
                }

                if(gtype == G_TYPE_NONE || gtype == XFCONF_TYPE_G_VALUE_ARRAY) {
                    xfconf_array_free(arr);
                    croak("Xfce4::Xfconf::Channel::set_array(): value cannot be of type 'empty' or 'array' at index %d", i);
                }

                arrval = g_new0(GValue, 1);
                g_value_init(arrval, gtype);
                gperl_value_from_sv(arrval, *val_sv);

                g_ptr_array_add(arr, arrval);
            }

            RETVAL = xfconf_channel_set_arrayv(channel, property, arr);
            xfconf_array_free(arr);
        }
        /* why isn't xsubpp doing this for us? */
        ST(0) = sv_2mortal(boolSV(RETVAL));
