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

MODULE = Xfce4::Xfconf::Binding   PACKAGE = Xfce4::Xfconf::Binding  PREFIX = xfconf_g_property_
PROTOTYPES: ENABLE

gulong
xfconf_g_property_bind(channel, xfconf_property, xfconf_property_type, object, object_property)
        XfconfChannel * channel
        const gchar *xfconf_property
        const gchar *xfconf_property_type
        GObject *object
        const gchar *object_property
    CODE:
        RETVAL = 0;

        if(!g_ascii_strcasecmp(xfconf_property_type, "gdkcolor")) {
            RETVAL = xfconf_g_property_bind_gdkcolor(channel, xfconf_property,
                                                     object, object_property);
        } else {
            GType xfconf_gtype = _xfconf_gtype_from_string(xfconf_property_type);
            if(xfconf_gtype == G_TYPE_INVALID)
                croak("Xfce4::Xfconf::Binding::bind(): can't determine xfconf property type from \"%s\"", xfconf_property_type);
            if(xfconf_gtype == G_TYPE_NONE || xfconf_gtype == XFCONF_TYPE_G_VALUE_ARRAY)
                croak("Xfce4::Xfconf::Binding::bind(): invalid xfconf property type \"%s\" for binding", xfconf_property_type);
            RETVAL = xfconf_g_property_bind(channel, xfconf_property, xfconf_gtype,
                                            object, object_property);
        }

void
xfconf_g_property_unbind(...)
    CODE:
        if(items != 1 && items != 4)
            croak("Usage: Xfce4::Xfconf::Binding::unbind(id) or ::unbind(channel) or ::unbind(object) or ::unbind(channel, xfconf_property, object, object_property)");

        if(items == 1) {
            GObject *channel_or_object;

            if((channel_or_object = SvGObject_ornull(ST(0))))
                xfconf_g_property_unbind_all(channel_or_object);
            else if(SvIOK(ST(0)))
                xfconf_g_property_unbind(SvIV(ST(0)));
        } else if(items == 4) {
            XfconfChannel *channel = SvXfconfChannel(ST(0));
            const gchar *xfconf_property = SvGChar(ST(1));
            GObject *object = SvGObject(ST(2));
            const gchar *object_property = SvGChar(ST(3));

            xfconf_g_property_unbind_by_property(channel, xfconf_property, object, object_property);
        }
