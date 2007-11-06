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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "xfconf/xfconf-types.h"
#include "xfconf-alias.h"

#include <gobject/gvaluecollector.h>


static void
ushort_value_init(GValue *value)
{
    value->data[0].v_int = 0;
}

static void
ushort_value_copy(const GValue *src_value,
                  GValue *dest_value)
{
    dest_value->data[0].v_int = src_value->data[0].v_int;
}

static gchar *
ushort_value_collect(GValue *value,
                     guint n_collect_values,
                     GTypeCValue *collect_values,
                     guint collect_flags)
{
    value->data[0].v_int = collect_values[0].v_int;
    return NULL;
}

static gchar *
ushort_value_lcopy(const GValue *value,
                   guint n_collect_values,
                   GTypeCValue *collect_values,
                   guint collect_flags)
{
    guint16 *uint16_p = collect_values[0].v_pointer;

    if(!uint16_p) {
        return g_strdup_printf("value location for `%s' passed as NULL",
                               G_VALUE_TYPE_NAME(value));
    }

    *uint16_p = value->data[0].v_int;

    return NULL;
}


GType
xfconf_uint16_get_type()
{
    static GType uint16_type = 0;
    GTypeFundamentalInfo finfo = { 0 };
    GTypeInfo info = { 0, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL, };
    static const GTypeValueTable value_table = {
        ushort_value_init,
        NULL,
        ushort_value_copy,
        NULL,
        "i",
        ushort_value_collect,
        "p",
        ushort_value_lcopy
    };

    if(!uint16_type) {
        info.value_table = &value_table;
        uint16_type = g_type_register_fundamental(g_type_fundamental_next(),
                                                  "XfconfUint16", &info,
                                                   &finfo, 0);
    }

    return uint16_type;
}

/**
 * xfconf_g_value_get_uint16:
 * @value: A #GValue.
 *
 * Retrieves a 16-bit unsigned value from @value.
 *
 * Returns: A guint16.
 **/
guint16
xfconf_g_value_get_uint16(const GValue *value)
{
    g_return_val_if_fail(G_VALUE_HOLDS(value, XFCONF_TYPE_UINT16), 0);
    return (guint16)value->data[0].v_int;
}

/**
 * xfconf_g_value_set_uint16:
 * @value: A #GValue.
 * @v_uint16: A guint16.
 *
 * Sets @value using an unsigned 16-bit integer.
 **/
void
xfconf_g_value_set_uint16(GValue *value,
                          guint16 v_uint16)
{
    g_return_if_fail(G_VALUE_HOLDS(value, XFCONF_TYPE_UINT16));
    value->data[0].v_int = v_uint16;
}

GType
xfconf_int16_get_type()
{
    static GType int16_type = 0;
    GTypeFundamentalInfo finfo = { 0 };
    GTypeInfo info = { 0, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL, };
    static const GTypeValueTable value_table = {
        ushort_value_init,
        NULL,
        ushort_value_copy,
        NULL,
        "i",
        ushort_value_collect,
        "p",
        ushort_value_lcopy
    };

    if(!int16_type) {
        info.value_table = &value_table;
        int16_type = g_type_register_fundamental(g_type_fundamental_next(),
                                                 "XfconfInt16", &info,
                                                  &finfo, 0);
    }

    return int16_type;
}

/**
 * xfconf_g_value_get_int16:
 * @value: A #GValue.
 *
 * Retrieves a 16-bit signed value from @value.
 *
 * Returns: A gint16.
 **/
gint16
xfconf_g_value_get_int16(const GValue *value)
{
    g_return_val_if_fail(G_VALUE_HOLDS(value, XFCONF_TYPE_INT16), 0);
    return (gint16)value->data[0].v_int;
}

/**
 * xfconf_g_value_set_int16:
 * @value: A #GValue.
 * @v_int16: A gint16.
 *
 * Sets @value using a signed 16-bit integer.
 **/
void
xfconf_g_value_set_int16(GValue *value,
                         gint16 v_int16)
{
    g_return_if_fail(G_VALUE_HOLDS(value, XFCONF_TYPE_INT16));
    value->data[0].v_int = v_int16;
}



#define __XFCONF_TYPES_C__
#include "xfconf-aliasdef.c"
