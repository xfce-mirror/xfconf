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

#ifndef __XFCONF_BACKEND_PERCHANNEL_XML_H__
#define __XFCONF_BACKEND_PERCHANNEL_XML_H__

#include <glib-object.h>

#define XFCONF_BACKEND_PERCHANNEL_XML_TYPE_ID "xfce-perchannel-xml"

G_BEGIN_DECLS

#define XFCONF_TYPE_BACKEND_PERCHANNEL_XML (xfconf_backend_perchannel_xml_get_type())
G_DECLARE_FINAL_TYPE(XfconfBackendPerchannelXml, xfconf_backend_perchannel_xml, XFCONF, BACKEND_PERCHANNEL_XML, GObject)

G_END_DECLS

#endif /* __XFCONF_BACKEND_PERCHANNEL_XML_H__ */
