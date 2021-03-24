/*
 *  Copyright (c) 2021 Jan Ziak <0xe2.0x9a.0x9b@xfce.org>
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

#include "xfconf-binding.h"
#include "xfconf-channel.h"

using namespace xfce4::xfconf;

Channel::Channel(XfconfChannel *c) : channel(c) {}

Channel::~Channel() {}

std::shared_ptr<Channel>
Channel::get(const std::string &channel_name)
{
    XfconfChannel *c = xfconf_channel_get(channel_name.c_str());
    return std::shared_ptr<Channel>(new Channel(c));
}

gulong
Channel::property_bind(const std::string &xfconf_property,
                       GType xfconf_property_type,
                       gpointer object,
                       const std::string &object_property)
{
    return xfconf_g_property_bind(channel, xfconf_property.c_str(),
        xfconf_property_type, object, object_property.c_str());
}

gulong
Channel::property_bind_gdkcolor(const std::string &xfconf_property,
                                gpointer object,
                                const std::string &object_property)
{
    return xfconf_g_property_bind_gdkcolor(channel, xfconf_property.c_str(),
        object, object_property.c_str());
}

gulong
Channel::property_bind_gdkrgba(const std::string &xfconf_property,
                               gpointer object,
                               const std::string &object_property)
{
    return xfconf_g_property_bind_gdkrgba(channel, xfconf_property.c_str(),
        object, object_property.c_str());
}

void
Channel::property_unbind_by_property(const std::string &xfconf_property,
                                     gpointer object,
                                     const std::string &object_property)
{
    xfconf_g_property_unbind_by_property(channel, xfconf_property.c_str(),
        object, object_property.c_str());
}

void
Channel::property_unbind_all()
{
    xfconf_g_property_unbind_all(channel);
}
