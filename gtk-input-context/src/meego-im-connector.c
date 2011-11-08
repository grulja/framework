/* * This file is part of meego-im-framework *
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * All rights reserved.
 * Contact: Nokia Corporation (directui@nokia.com)
 *
 * If you have questions regarding the use of this file, please contact
 * Nokia at directui@nokia.com.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * and appearing in the file LICENSE.LGPL included in the packaging
 * of this file.
 */

#include "meego-im-connector.h"

#include <glib.h>
#include <gio/gio.h>

#define MALIIT_SERVER_NAME "org.maliit.server"
#define MALIIT_SERVER_OBJECT_PATH "/org/maliit/server/address"
#define MALIIT_SERVER_INTERFACE "org.maliit.Server.Address"
#define MALIIT_SERVER_ADDRESS_PROPERTY "address"

MeegoImConnector *meego_im_connector_new();

static gboolean
try_reconnect(MeegoImConnector *connector)
{
    meego_im_connector_run(connector);
    return FALSE; // _run is responsible for setting up a new timeout if needed
}


static void
connection_dropped(gpointer instance, MeegoImConnector *connector)
{
    if (connector->connection) {
        dbus_g_connection_unref(connector->connection);
    }
    try_reconnect(connector);
}

static char *
get_dbus_address()
{
    GDBusProxy *proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                      G_DBUS_PROXY_FLAGS_NONE,
                                                      0,
                                                      MALIIT_SERVER_NAME,
                                                      MALIIT_SERVER_OBJECT_PATH,
                                                      MALIIT_SERVER_INTERFACE,
                                                      0, 0);

    if (!proxy)
        return 0;

    GVariant *variant = g_dbus_proxy_get_cached_property(proxy, MALIIT_SERVER_ADDRESS_PROPERTY);

    if (!variant) {
        g_object_unref(proxy);
        return 0;
    }

    char* address = g_strdup(g_variant_get_string(variant, 0));

    g_variant_unref(variant);
    g_object_unref(proxy);

    return address;
}

/**
 * MeegoImConnector:
 *
 * Connects and maintains the DBusConnection, and the objects
 * that depend on it. Makes sure that MeegoIMProxy and MeegoIMDbusObj
 * has the correct DBusConnection by calling _connect on them when
 * the connection has changed.
 *
 * MeegoIMProxy is responsible for letting the connector know that the
 * connection was dropped by emitting the "connection-dropped signal".
 */
MeegoImConnector *
meego_im_connector_new()
{
    MeegoImConnector *self = g_new(MeegoImConnector, 1);

    self->connection = NULL;
    self->dbusobj = meego_imcontext_dbusobj_get_singleton();
    self->proxy = meego_im_proxy_get_singleton();

    g_signal_connect(self->proxy, "connection-dropped",
                     G_CALLBACK(connection_dropped), (gpointer)self);

    return self;
}

void
meego_im_connector_free(MeegoImConnector *self)
{
    if (self->connection) {
        dbus_g_connection_unref(self->connection);
    }
    g_free(self);
}

MeegoImConnector *
meego_im_connector_get_singleton()
{
    static MeegoImConnector *connector = NULL;
    if (!connector) {
        connector = meego_im_connector_new();
        meego_im_connector_run(connector);
    }
    return connector;
}

void
meego_im_connector_run(MeegoImConnector *self)
{
    GError *error = NULL;
    DBusGConnection *connection = NULL;

    g_return_if_fail(self != NULL);

    char *address = get_dbus_address();
    if (!address) {
        g_warning("Couldn't connect to Maliit server. Retrying...");

        g_timeout_add_seconds(2, (GSourceFunc)try_reconnect, self);
        return;
    }

    connection = dbus_g_connection_open(address, &error);
    g_free(address);

    if (error != NULL) {
        g_warning("Couldn't connect to Maliit server. Retrying...");
        g_error_free(error);

        g_timeout_add_seconds(2, (GSourceFunc)try_reconnect, self);
        return;
    }

    self->connection = connection;

    meego_im_proxy_connect(self->proxy, self->connection);
    meego_imcontext_dbusobj_connect(self->dbusobj, self->connection);
}