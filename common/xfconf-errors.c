/*
 *  xfconf
 *
 *  Copyright (c) 2007 Brian Tarricone <bjt23@cornell.edu>
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

#include "xfconf/xfconf-errors.h"
#include "xfconf-alias.h"

static GQuark xfconf_error_quark = 0;

/**
 * XFCONF_ERROR:
 *
 * The #GError error domain for Xfconf.
 **/


/**
 * XFCONF_TYPE_ERROR:
 *
 * An enum GType for Xfconf errors.
 **/


/**
 * XfconfError:
 *
 * An enumeration listing the different kinds of errors under the
 * XFCONF_ERROR domain.
 **/

GQuark
xfconf_get_error_quark(void)
{
    if(!xfconf_error_quark)
        xfconf_error_quark = g_quark_from_static_string("xfconf-error-quark");
    
    return xfconf_error_quark;
}

/* unfortunately glib-mkenums can't generate types that are compatible with
 * dbus error names -- the 'nick' value is used, which can have dashes in it,
 * which dbus doesn't like. */
GType
xfconf_error_get_type(void)
{
    static GType type = 0;
    
    if(!type) {
        static const GEnumValue values[] = {
            { XFCONF_ERROR_UNKNOWN, "XFCONF_ERROR_UNKNOWN", "Unknown" },
            { XFCONF_ERROR_CHANNEL_NOT_FOUND, "XFCONF_ERROR_CHANNEL_NOT_FOUND", "ChannelNotFound" },
            { XFCONF_ERROR_PROPERTY_NOT_FOUND, "XFCONF_ERROR_PROPERTY_NOT_FOUND", "PropertyNotFound" },
            { XFCONF_ERROR_READ_FAILURE, "XFCONF_ERROR_READ_FAILURE", "ReadFailure" },
            { XFCONF_ERROR_WRITE_FAILURE, "XFCONF_ERROR_WRITE_FAILURE", "WriteFailure" },
            { XFCONF_ERROR_PERMISSION_DENIED, "XFCONF_ERROR_PERMISSION_DENIED", "PermissionDenied" },
            { XFCONF_ERROR_INTERNAL_ERROR, "XFCONF_ERROR_INTERNAL_ERROR", "InternalError" },
            { XFCONF_ERROR_NO_BACKEND, "XFCONF_ERROR_NO_BACKEND", "NoBackend" },
            { XFCONF_ERROR_INVALID_PROPERTY, "XFCONF_ERROR_INVALID_PROPERTY", "InvalidProperty" },
            { XFCONF_ERROR_INVALID_CHANNEL, "XFCONF_ERROR_INVALID_CHANNEL", "InvalidChannel" },
            { 0, NULL, NULL }
        };
        
        type = g_enum_register_static("XfconfError", values);
    }
    
    return type;
}



#define __XFCONF_ERRORS_C__
#include "xfconf-aliasdef.c"
