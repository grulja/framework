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

#include "mimpluginmanager.h"
#include "mimpluginmanager_p.h"

#include <MGConfItem>
#include <MKeyboardStateTracker>

#include "minputmethodplugin.h"
#include "minputcontextdbusconnection.h"
#include "mtoolbarmanager.h"

#include <QDir>
#include <QPluginLoader>
#include <QDebug>


namespace
{
    const int DeleteInputMethodTimeout = 60000;

    const QString DefaultPluginLocation("/usr/lib/meego-im-plugins/");

    const QString ConfigRoot          = "/meegotouch/inputmethods/";
    const QString MImPluginPaths    = ConfigRoot + "paths";
    const QString MImPluginActive   = ConfigRoot + "activeplugins";
    const QString MImPluginDisabled = ConfigRoot + "disabledpluginfiles";

    const QString PluginRoot          = "/meegotouch/inputmethods/plugins/";
    const QString MImHandlerToPlugin  = PluginRoot + "handler";
    const QString MImAccesoryEnabled  = "/meegotouch/inputmethods/accessoryenabled";
}



MIMPluginManagerPrivate::MIMPluginManagerPrivate(MInputContextConnection *connection,
                                                     MIMPluginManager *p)
    : parent(p),
      mICConnection(connection),
      handlerToPluginConf(0),
      imAccessoryEnabledConf(0)
{
    deleteImTimer.setSingleShot(true);
    deleteImTimer.setInterval(DeleteInputMethodTimeout);
}


MIMPluginManagerPrivate::~MIMPluginManagerPrivate()
{
    delete mICConnection;
}


void MIMPluginManagerPrivate::loadPlugins()
{
    foreach (QString path, paths) {
        QDir pluginsDir(path);

        foreach (const QString &fileName, pluginsDir.entryList(QDir::Files)) {
            if (blacklist.contains(fileName)) {
                qWarning() << __PRETTY_FUNCTION__ << fileName << "is on the blacklist, skipped.";
                continue;
            }

            QPluginLoader load(pluginsDir.absoluteFilePath(fileName));
            QObject *pluginInstance = load.instance();

            if (!pluginInstance) {
                qWarning() << __PRETTY_FUNCTION__ << "Error loading plugin from "
                           << path << fileName << load.errorString();
                continue;
            }

            MInputMethodPlugin *plugin = qobject_cast<MInputMethodPlugin *>(pluginInstance);

            if (plugin) {
                if (!plugin->supportedStates().isEmpty()) {
                    PluginDescription desc = { 0, PluginState(), M::SwitchUndefined };
                    plugins[plugin] = desc;
                    if (active.contains(plugin->name())) {
                        activatePlugin(plugin);
                    }
                } else {
                    qWarning() << __PRETTY_FUNCTION__
                               << "Plugin does not support any state: " << fileName;
                }
            } else
                qWarning() << __PRETTY_FUNCTION__
                           << "Plugin is not MInputMethodPlugin: " << fileName;
        } // end foreach file in path
    } // end foreach path in paths
}


bool MIMPluginManagerPrivate::activatePlugin(const QString &name)
{
    foreach (MInputMethodPlugin *plugin, activePlugins) {
        if (plugin->name() == name) {
            return true;
        }
    }

    foreach (MInputMethodPlugin *plugin, plugins.keys()) {
        if (plugin->name() == name) {
            activatePlugin(plugin);
            return true;
        }
    }
    return false;
}

void MIMPluginManagerPrivate::activatePlugin(MInputMethodPlugin *plugin)
{
    if (!plugin || activePlugins.contains(plugin)) {
        return;
    }

    MInputMethodBase *inputMethod = 0;

    activePlugins.insert(plugin);
    if (!plugins[plugin].inputMethod) {
        inputMethod = plugin->createInputMethod(mICConnection);
        bool connected = false;

        plugins[plugin].inputMethod = inputMethod;
        if (inputMethod) {
            connected = QObject::connect(inputMethod, SIGNAL(regionUpdated(const QRegion &)),
                                         parent, SIGNAL(regionUpdated(const QRegion &)));

            connected = QObject::connect(inputMethod,
                                         SIGNAL(inputMethodAreaUpdated(const QRegion &)),
                                         mICConnection,
                                         SLOT(updateInputMethodArea(const QRegion &)))
                        && connected;

            connected = QObject::connect(inputMethod,
                                         SIGNAL(pluginSwitchRequired(M::InputMethodSwitchDirection)),
                                         parent,
                                         SLOT(switchPlugin(M::InputMethodSwitchDirection)))
                        && connected;

            connected = QObject::connect(inputMethod,
                                         SIGNAL(pluginSwitchRequired(const QString&)),
                                         parent,
                                         SLOT(switchPlugin(const QString&)))
                        && connected;
        }
        if (!connected) {
            qWarning() << __PRETTY_FUNCTION__ << "Plugin" << plugin->name()
                       << "Unable to connect plugin's signals with IC connection's slots";
        }
    } else {
        inputMethod = plugins[plugin].inputMethod;
    }

    mICConnection->addTarget(inputMethod); // redirect incoming requests

    return;
}


void MIMPluginManagerPrivate::addHandlerMap(MIMHandlerState state, const QString &pluginName)
{
    foreach (MInputMethodPlugin *plugin, plugins.keys()) {
        if (plugin->name() == pluginName) {
            handlerToPlugin[state] = plugin;
            break;
        }
    }
}


void MIMPluginManagerPrivate::setActiveHandlers(const QSet<MIMHandlerState> &states)
{
    QSet<MInputMethodPlugin *> activatedPlugins;
    MInputMethodBase *inputMethod = 0;

    //clear all cached states before activating new one
    for (Plugins::iterator iterator = plugins.begin();
         iterator != plugins.end();
         ++iterator) {
        iterator->state.clear();
    }

    //activate new plugins
    foreach (MIMHandlerState state, states) {
        HandlerMap::const_iterator iterator = handlerToPlugin.find(state);
        MInputMethodPlugin *plugin = 0;

        if (iterator != handlerToPlugin.end()) {
            plugin = iterator.value();
            if (!activePlugins.contains(plugin)) {
                activatePlugin(plugin);
            }
            inputMethod = plugins[plugin].inputMethod;
            if (inputMethod) {
                plugins[plugin].state << state;
                activatedPlugins.insert(plugin);
            }
        }
    }

    // notify plugins about new states
    foreach (MInputMethodPlugin *plugin, activatedPlugins) {
        plugins[plugin].inputMethod->setState(plugins[plugin].state);
    }

    // deactivate unnecessary plugins
    QSet<MInputMethodPlugin *> previousActive = activePlugins;
    foreach (MInputMethodPlugin *plugin, previousActive) {
        if (!activatedPlugins.contains(plugin)) {
            deactivatePlugin(plugin);  //activePlugins is modified here
        }
    }
}


QSet<MIMHandlerState> MIMPluginManagerPrivate::activeHandlers() const
{
    QSet<MIMHandlerState> handlers;
    foreach (MInputMethodPlugin *plugin, activePlugins) {
        handlers << handlerToPlugin.key(plugin);
    }
    return handlers;
}


void MIMPluginManagerPrivate::deleteInactiveIM()
{
    Plugins::iterator iterator;

    for (iterator = plugins.begin(); iterator != plugins.end(); ++iterator) {
        if (!activePlugins.contains(iterator.key())) {
            delete iterator->inputMethod;
            iterator->inputMethod = 0;
        }
    }
}


void MIMPluginManagerPrivate::deactivatePlugin(MInputMethodPlugin *plugin)
{
    if (!activePlugins.contains(plugin))
        return;

    activePlugins.remove(plugin);
    MInputMethodBase *inputMethod = plugins[plugin].inputMethod;

    if (!inputMethod)
        return;

    plugins[plugin].state = PluginState();
    inputMethod->hide();
    inputMethod->reset();
    mICConnection->removeTarget(inputMethod);
}


void MIMPluginManagerPrivate::convertAndFilterHandlers(const QStringList &handlerNames,
                                                         QSet<MIMHandlerState> *handlers)
{
    bool ok = false;
    bool disableOnscreenKbd = false;

    foreach (const QString &name, handlerNames) {
        int handlerNumber = (MIMHandlerState)name.toInt(&ok);
        if (ok && handlerNumber >= OnScreen && handlerNumber <= Accessory) {
            if (!disableOnscreenKbd) {
                disableOnscreenKbd = handlerNumber != OnScreen;
            }
            handlers->insert((MIMHandlerState)handlerNumber);
        }
    }

    if (disableOnscreenKbd) {
        handlers->remove(OnScreen);
    }
}


void MIMPluginManagerPrivate::replacePlugin(M::InputMethodSwitchDirection direction,
                                            Plugins::iterator initiator,
                                            Plugins::iterator replacement)
{
    PluginState state = initiator->state;
    MInputMethodBase *switchedTo = 0;

    deactivatePlugin(initiator.key());
    activatePlugin(replacement.key());
    switchedTo = replacement->inputMethod;;
    replacement->state = state;
    switchedTo->setState(state);
    if (replacement->lastSwitchDirection == direction) {
        switchedTo->switchContext(direction, false);
    }
    initiator->lastSwitchDirection = direction;
    switchedTo->show();
}


bool MIMPluginManagerPrivate::switchPlugin(M::InputMethodSwitchDirection direction,
                                           MInputMethodBase *initiator)
{
    if (direction == M::SwitchUndefined) {
        return true; //do nothing for this direction
    }

    //Find plugin initiated this switch
    Plugins::iterator iterator(plugins.begin());

    for (; iterator != plugins.end(); ++iterator) {
        if (iterator->inputMethod == initiator) {
            break;
        }
    }

    if (iterator == plugins.end()) {
        return false;
    }

    Plugins::iterator source = iterator;

    //find next inactive plugin and activate it
    for (int n = 0; n < plugins.size() - 1; ++n) {
        if (direction == M::SwitchForward) {
            ++iterator;
            if (iterator == plugins.end()) {
                iterator = plugins.begin();
            }
        } else { // Backward
            if (iterator == plugins.begin()) {
                iterator = plugins.end();
            }
            --iterator;
        }

        if (doSwitchPlugin(direction, source, iterator)) {
            return true;
        }
    }

    return false;
}

bool MIMPluginManagerPrivate::switchPlugin(const QString &name, MInputMethodBase *initiator)
{
    //Find plugin initiated this switch
    Plugins::iterator iterator(plugins.begin());

    for (; iterator != plugins.end(); ++iterator) {
        if (iterator->inputMethod == initiator) {
            break;
        }
    }

    if (iterator == plugins.end()) {
        return false;
    }

    Plugins::iterator source = iterator;

    // find plugin specified by name
    for (iterator = plugins.begin(); iterator != plugins.end(); ++iterator) {
        if (iterator.key()->name() == name) {
            break;
        }
    }

    if (iterator == plugins.end()) {
        return false;
    }

    if (source == iterator) {
        return true;
    }

    return doSwitchPlugin(M::SwitchUndefined, source, iterator);
}

bool MIMPluginManagerPrivate::doSwitchPlugin(M::InputMethodSwitchDirection direction,
                                           Plugins::iterator source,
                                           Plugins::iterator replacement)
{
    if (!activePlugins.contains(replacement.key())) {
        const QSet<MIMHandlerState> intersect(replacement.key()->supportedStates()
                & source->state);
        // switch to other plugin if it could handle any state
        // handled by current plugin just now
        if (intersect == source->state) {
            changeHandlerMap(source.key(), replacement.key(), replacement.key()->supportedStates());
            replacePlugin(direction, source, replacement);
            return true;
        }
    }

    return false;
}

void MIMPluginManagerPrivate::changeHandlerMap(MInputMethodPlugin *origin,
                                               MInputMethodPlugin *replacement,
                                               QSet<MIMHandlerState> states)
{
    QList<QString> handlers = handlerToPluginConf->listEntries();

    foreach (MIMHandlerState state, states) {
        HandlerMap::iterator iterator = handlerToPlugin.find(state);
        if (iterator != handlerToPlugin.end() && *iterator == origin) {
            MGConfItem gconf(MImHandlerToPlugin + QString("/%1").arg(int(state)));
            gconf.set(replacement->name());
            *iterator = replacement; //for unit tests
        }
    }
}

QStringList MIMPluginManagerPrivate::loadedPluginsNames() const
{
    QStringList result;

    foreach (MInputMethodPlugin *plugin, plugins.keys()) {
        result.append(plugin->name());
    }

    return result;
}


QStringList MIMPluginManagerPrivate::activePluginsNames() const
{
    QStringList result;

    foreach (MInputMethodPlugin *plugin, activePlugins) {
        result.append(plugin->name());
    }

    return result;
}


QStringList MIMPluginManagerPrivate::activeInputMethodsNames() const
{
    QStringList result;

    for (Plugins::const_iterator iterator = plugins.begin();
            iterator != plugins.end(); ++iterator) {
        if (iterator->inputMethod) {
            result.append(iterator.key()->name());
        }
    }

    return result;
}


///////////////
// actual class

MIMPluginManager::MIMPluginManager()
    : QObject(),
      d(new MIMPluginManagerPrivate(new MInputContextDBusConnection, this))
{
    MToolbarManager::createInstance();

    d->paths     = MGConfItem(MImPluginPaths).value(QStringList(DefaultPluginLocation)).toStringList();
    d->blacklist = MGConfItem(MImPluginDisabled).value().toStringList();
    d->active    = MGConfItem(MImPluginActive).value().toStringList();

    d->loadPlugins();

    d->handlerToPluginConf = new MGConfItem(MImHandlerToPlugin, this);
    connect(d->handlerToPluginConf, SIGNAL(valueChanged()), this, SLOT(reloadHandlerMap()));

    reloadHandlerMap();

    if (MKeyboardStateTracker::instance()->isPresent()) {
        connect(MKeyboardStateTracker::instance(), SIGNAL(stateChanged()), this, SLOT(updateInputSource()));
    }

    d->imAccessoryEnabledConf = new MGConfItem(MImAccesoryEnabled, this);
    connect(d->imAccessoryEnabledConf, SIGNAL(valueChanged()), this, SLOT(updateInputSource()));

    updateInputSource();

    connect(&d->deleteImTimer, SIGNAL(timeout()), this, SLOT(deleteInactiveIM()));
}


MIMPluginManager::~MIMPluginManager()
{
    delete d;
    d = 0;

    MToolbarManager::destroyInstance();
}


void MIMPluginManager::reloadHandlerMap()
{
    QList<QString> handlers = d->handlerToPluginConf->listEntries();

    foreach (const QString &handlerName, handlers) {
        QStringList path = handlerName.split("/");
        QString pluginName = MGConfItem(handlerName).value().toString();
        d->addHandlerMap((MIMHandlerState)path.last().toInt(), pluginName);
    }
}


void MIMPluginManager::deleteInactiveIM()
{
    d->deleteInactiveIM();
}


QStringList MIMPluginManager::loadedPluginsNames() const
{
    return d->loadedPluginsNames();
}


QStringList MIMPluginManager::activePluginsNames() const
{
    return d->activePluginsNames();
}


QStringList MIMPluginManager::activeInputMethodsNames() const
{
    return d->activeInputMethodsNames();
}


void MIMPluginManager::setDeleteIMTimeout(int timeout)
{
    d->deleteImTimer.setInterval(timeout);
}

void MIMPluginManager::updateInputSource()
{
    // Hardware and Accessory can work together.
    // OnScreen is mutually exclusive to Hardware and Accessory.
    QSet<MIMHandlerState> handlers = d->activeHandlers();
    if (MKeyboardStateTracker::instance()->isOpen()) {
        // hw keyboard is on
        handlers.remove(OnScreen);
        handlers.insert(Hardware);
    } else {
        // hw keyboard is off
        handlers.remove(Hardware);
        handlers.insert(OnScreen);
    }

    if (d->imAccessoryEnabledConf->value().toBool()) {
        handlers.remove(OnScreen);
        handlers.insert(Accessory);
    } else {
        handlers.remove(Accessory);
    }

    if (!handlers.isEmpty()) {
        d->setActiveHandlers(handlers);
        d->deleteImTimer.start();
    }
}

void MIMPluginManager::switchPlugin(M::InputMethodSwitchDirection direction)
{
    MInputMethodBase *initiator = qobject_cast<MInputMethodBase*>(sender());

    if (initiator) {
        if (d->switchPlugin(direction, initiator)) {
            d->deleteImTimer.start();
        } else {
            initiator->switchContext(direction, true);
        }
    }
}

void MIMPluginManager::switchPlugin(const QString &name)
{
    MInputMethodBase *initiator = qobject_cast<MInputMethodBase*>(sender());

    if (initiator) {
        if (d->switchPlugin(name, initiator)) {
            d->deleteImTimer.start();
        }
    }
}