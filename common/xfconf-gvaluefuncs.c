/*
 *  xfconf
 *
 *  Copyright (c) 2008 Brian Tarricone <bjt23@cornell.edu>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; version 2
 *  of the License ONLY.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef GETTEXT_PACKAGE
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include <dbus/dbus-glib.h>

#include "xfconf-gvaluefuncs.h"
#include "xfconf/xfconf-types.h"
#include "xfconf-common-private.h"

#ifdef CHAR_MIN
#define XFCONF_MINCHAR  CHAR_MIN
#else 
#define XFCONF_MINCHAR  (-128)
#endif

#ifdef CHAR_MAX
#define XFCONF_MAXCHAR  CHAR_MAX
#else
#define XFCONF_MAXCHAR  (127)
#endif

#ifdef UCHAR_MAX
#define XFCONF_MAXUCHAR  UCHAR_MAX
#else
#define XFCONF_MAXUCHAR  (255)
#endif

GType
_xfconf_gtype_from_string(const gchar *type)
{
    /* note: move the most frequently used types to the top */
    if(!strcmp(type, "empty"))
        return G_TYPE_NONE;
    else if(!strcmp(type, "string"))
        return G_TYPE_STRING;
    else if(!strcmp(type, "int"))
        return G_TYPE_INT;
    else if(!strcmp(type, "double"))
        return G_TYPE_DOUBLE;
    else if(!strcmp(type, "bool"))
        return G_TYPE_BOOLEAN;
    else if(!strcmp(type, "array"))
        return XFCONF_TYPE_G_VALUE_ARRAY;
    else if(!strcmp(type, "uint"))
        return G_TYPE_UINT;
    else if(!strcmp(type, "uchar"))
        return G_TYPE_UCHAR;
    else if(!strcmp(type, "char"))
        return G_TYPE_CHAR;
    else if(!strcmp(type, "uint16"))
        return XFCONF_TYPE_UINT16;
    else if(!strcmp(type, "int16"))
        return XFCONF_TYPE_INT16;
    else if(!strcmp(type, "uint64"))
        return G_TYPE_UINT64;
    else if(!strcmp(type, "int64"))
        return G_TYPE_INT64;
    else if(!strcmp(type, "float"))
        return G_TYPE_FLOAT;

    return G_TYPE_INVALID;
}

const gchar *
_xfconf_string_from_gtype(GType gtype)
{
    switch(gtype) {
        case G_TYPE_STRING:
            return "string";
        case G_TYPE_UCHAR:
            return "uchar";
        case G_TYPE_CHAR:
            return "char";
        case G_TYPE_UINT:
            return "uint";
        case G_TYPE_INT:
            return "int";
        case G_TYPE_UINT64:
            return "uint64";
        case G_TYPE_INT64:
            return "int64";
        case G_TYPE_FLOAT:
            return "float";
        case G_TYPE_DOUBLE:
            return "double";
        case G_TYPE_BOOLEAN:
            return "bool";
        default:
            if(gtype == XFCONF_TYPE_UINT16)
                return "uint16";
            else if(gtype == XFCONF_TYPE_INT16)
                return "int16";
            else if(gtype == XFCONF_TYPE_G_VALUE_ARRAY)
                return "array";
            break;
    }

    g_warning("GType %s doesn't map to an Xfconf type",
              g_type_name(gtype));
    return NULL;
}

gboolean
_xfconf_gvalue_from_string(GValue *value,
                           const gchar *str)
{
#define CHECK_CONVERT_STATUS() \
    if(*str == 0 || *endptr != 0) \
        return FALSE
#define CHECK_CONVERT_VALUE(val, minval, maxval) \
    if((val) < (minval) || (val) > (maxval)) \
        return FALSE
    
#define REAL_HANDLE_INT(minval, maxval, convertfunc, setfunc) \
    G_STMT_START{ \
        errno = 0; \
        intval = convertfunc(str, &endptr, 0); \
        if(0 == intval && ERANGE == errno) \
            return FALSE; \
        CHECK_CONVERT_STATUS(); \
        CHECK_CONVERT_VALUE(intval, minval, maxval); \
        setfunc(value, intval); \
        return TRUE; \
    }G_STMT_END

#define HANDLE_UINT(minval, maxval, setfunc)  REAL_HANDLE_INT(minval, maxval, strtoul, setfunc)
#define HANDLE_INT(minval, maxval, setfunc)  REAL_HANDLE_INT(minval, maxval, strtol, setfunc)
    
    guint64 uintval;
    gint64 intval;
    gdouble dval;
    gchar *endptr = NULL;
    
    switch(G_VALUE_TYPE(value)) {
        case G_TYPE_STRING:
            g_value_set_string(value, str);
            return TRUE;
        
        case G_TYPE_UCHAR:
            HANDLE_UINT(0, XFCONF_MAXUCHAR, g_value_set_uchar);
        case G_TYPE_CHAR:
#if GLIB_CHECK_VERSION (2, 32, 0)
            HANDLE_INT(G_MININT8, G_MAXINT8, g_value_set_schar);
#else
            HANDLE_INT(XFCONF_MINCHAR, XFCONF_MAXCHAR, g_value_set_char);
#endif
        case G_TYPE_UINT:
            HANDLE_UINT(0, G_MAXUINT, g_value_set_uint);
        case G_TYPE_INT:
            HANDLE_INT(G_MININT, G_MAXINT, g_value_set_int);
        
        case G_TYPE_UINT64:
            errno = 0;
            uintval = g_ascii_strtoull(str, &endptr, 0);
            if(0 == uintval && ERANGE == errno)
                return FALSE;
            CHECK_CONVERT_STATUS();
            g_value_set_uint64(value, uintval);
            return TRUE;
        
        case G_TYPE_INT64:
            errno = 0;
            intval = g_ascii_strtoll(str, &endptr, 0);
            if(0 == intval && ERANGE == errno)
                return FALSE;
            CHECK_CONVERT_STATUS();
            g_value_set_int64(value, intval);
            return TRUE;
        
        case G_TYPE_FLOAT:
            errno = 0;
            dval = g_ascii_strtod(str, &endptr);
            if(0.0 == dval && ERANGE == errno)
                return FALSE;
            CHECK_CONVERT_STATUS();
            if(dval < G_MINFLOAT || dval > G_MAXFLOAT)
                return FALSE;
            g_value_set_float(value, (gfloat)dval);
            return TRUE;
        
        case G_TYPE_DOUBLE:
            errno = 0;
            dval = g_ascii_strtod(str, &endptr);
            if(0.0 == dval && ERANGE == errno)
                return FALSE;
            CHECK_CONVERT_STATUS();
            g_value_set_double(value, dval);
            return TRUE;
        
        case G_TYPE_BOOLEAN:
            if(!strcmp(str, "true")) {
                g_value_set_boolean(value, TRUE);
                return TRUE;
            } else if(!strcmp(str, "false")) {
                g_value_set_boolean(value, FALSE);
                return TRUE;
            } else
                return FALSE;
        
        default:
            if(XFCONF_TYPE_UINT16 == G_VALUE_TYPE(value)) {
                HANDLE_INT(0, G_MAXUSHORT, xfconf_g_value_set_uint16);
                return TRUE;
            } else if(XFCONF_TYPE_INT16 == G_VALUE_TYPE(value)) {
                HANDLE_INT(G_MINSHORT, G_MAXSHORT, xfconf_g_value_set_int16);
                return TRUE;
            } else if(XFCONF_TYPE_G_VALUE_ARRAY == G_VALUE_TYPE(value)) {
                GPtrArray *arr = g_ptr_array_sized_new(1);
                g_value_take_boxed(value, arr);
                return TRUE;
            }
            return FALSE;
    }

#undef CHECK_CONVERT_STATUS
#undef CHECK_CONVERT_VALUE
#undef REAL_HANDLE_INT
#undef HANDLE_INT
#undef HANDLE_UINT
}

gchar *
_xfconf_string_from_gvalue(GValue *val)
{
    g_return_val_if_fail(val && G_VALUE_TYPE(val), NULL);

    switch(G_VALUE_TYPE(val)) {
        case G_TYPE_STRING:
            return g_value_dup_string(val);
        case G_TYPE_UCHAR:
            return g_strdup_printf("%u", (guint)g_value_get_uchar(val));
        case G_TYPE_CHAR:
#if GLIB_CHECK_VERSION (2, 32, 0)
            return g_strdup_printf("%d", g_value_get_schar(val));
#else
            return g_strdup_printf("%d", (gint)g_value_get_char(val));
#endif
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
            return g_strdup(g_value_get_boolean(val) ? "true" : "false");
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

gboolean
_xfconf_gvalue_is_equal(const GValue *value1,
                        const GValue *value2)
{
    if(G_UNLIKELY(!value1 && !value2))
        return TRUE;
    if(G_UNLIKELY(!value1 || !value2))
        return FALSE;
    if(G_VALUE_TYPE(value1) != G_VALUE_TYPE(value2))
        return FALSE;
    if(G_VALUE_TYPE(value1) == G_TYPE_INVALID
       || G_VALUE_TYPE(value1) == G_TYPE_NONE)
    {
        return TRUE;
    }

    switch(G_VALUE_TYPE(value1)) {
#define HANDLE_CMP_GV(TYPE, getter) \
        case G_TYPE_ ## TYPE: \
            return g_value_get_ ## getter(value1) == g_value_get_ ## getter(value2)

#if GLIB_CHECK_VERSION (2, 32, 0)
        HANDLE_CMP_GV(CHAR, schar);
#else
        HANDLE_CMP_GV(CHAR, char);
#endif
        HANDLE_CMP_GV(UCHAR, uchar);
        HANDLE_CMP_GV(BOOLEAN, boolean);
        HANDLE_CMP_GV(INT, int);
        HANDLE_CMP_GV(UINT, uint);
        HANDLE_CMP_GV(INT64, int64);
        HANDLE_CMP_GV(UINT64, uint64);
        HANDLE_CMP_GV(FLOAT, float);
        HANDLE_CMP_GV(DOUBLE, double);

        case G_TYPE_STRING:
            return !g_strcmp0(g_value_get_string(value1), g_value_get_string(value2));

        default:
            if(G_VALUE_TYPE(value1) == XFCONF_TYPE_INT16)
                return xfconf_g_value_get_int16(value1) == xfconf_g_value_get_uint16(value2);
            else if(G_VALUE_TYPE(value1) == XFCONF_TYPE_UINT16)
                return xfconf_g_value_get_uint16(value1) == xfconf_g_value_get_uint16(value2);
            break;
#undef HANDLE_CMP_GV
    }

    return FALSE;
}

void
_xfconf_gvalue_free(GValue *value)
{
    if(!value)
        return;
    g_value_unset(value);
    g_free(value);
}
