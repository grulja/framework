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

#ifndef MIMSERVER_H
#define MIMSERVER_H

#include <QObject>
#include <tr1/memory>

class MInputContextConnection;
class MImServerPrivate;
class QWidget;

/* MImServer: The Maliit Input Method Server
 *
 * Consumers of MImServer are responsible for creating a QApplication (for the mainloop),
 * and an MInputContextConnection for communication with clients, and for starting the mainloop.
 * Everything else is handled by the server.
 *
 * Note: For X11, MImServer MUST be used together with MImXApplication.
 */
class MImServer : public QObject
{
    Q_OBJECT
public:
    explicit MImServer(std::tr1::shared_ptr<MInputContextConnection> icConnection, QObject *parent = 0);
    ~MImServer();

private:
    void connectComponents();
    QWidget *pluginsWidget();

    Q_DISABLE_COPY(MImServer)
    Q_DECLARE_PRIVATE(MImServer)
    const QScopedPointer<MImServerPrivate> d_ptr;
};

#endif // MIMSERVER_H