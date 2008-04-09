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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
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
            HANDLE_INT(XFCONF_MINCHAR, XFCONF_MAXCHAR, g_value_set_char);
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

void
_xfconf_gvalue_free(GValue *value)
{
    if(!value)
        return;
    g_value_unset(value);
    g_free(value);
}
