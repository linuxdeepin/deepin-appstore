/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp com.deepin.lastore.updater.xml -p lastore_updater_interface -i dbus/dbusvariant/app_update_info.h -i dbus/dbusvariant/locale_mirror_source.h -c LastoreUpdaterInterface
 *
 * qdbusxml2cpp is Copyright (C) 2016 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef LASTORE_UPDATER_INTERFACE_H
#define LASTORE_UPDATER_INTERFACE_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>
#include "dbus/dbusvariant/app_update_info.h"
#include "dbus/dbusvariant/locale_mirror_source.h"

/*
 * Proxy class for interface com.deepin.lastore.Updater
 */
class LastoreUpdaterInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "com.deepin.lastore.Updater"; }

public:
    LastoreUpdaterInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~LastoreUpdaterInterface();

    Q_PROPERTY(bool AutoCheckUpdates READ autoCheckUpdates)
    inline bool autoCheckUpdates() const
    { return qvariant_cast< bool >(property("AutoCheckUpdates")); }

    Q_PROPERTY(bool AutoDownloadUpdates READ autoDownloadUpdates)
    inline bool autoDownloadUpdates() const
    { return qvariant_cast< bool >(property("AutoDownloadUpdates")); }

    Q_PROPERTY(QString MirrorSource READ mirrorSource)
    inline QString mirrorSource() const
    { return qvariant_cast< QString >(property("MirrorSource")); }

    Q_PROPERTY(QStringList UpdatableApps READ updatableApps)
    inline QStringList updatableApps() const
    { return qvariant_cast< QStringList >(property("UpdatableApps")); }

    Q_PROPERTY(QStringList UpdatablePackages READ updatablePackages)
    inline QStringList updatablePackages() const
    { return qvariant_cast< QStringList >(property("UpdatablePackages")); }

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<AppUpdateInfoList> ApplicationUpdateInfos(const QString &in0)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(in0);
        return asyncCallWithArgumentList(QStringLiteral("ApplicationUpdateInfos"), argumentList);
    }

    inline QDBusPendingReply<LocaleMirrorSourceList> ListMirrorSources(const QString &in0)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(in0);
        return asyncCallWithArgumentList(QStringLiteral("ListMirrorSources"), argumentList);
    }

    inline QDBusPendingReply<> SetAutoCheckUpdates(bool in0)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(in0);
        return asyncCallWithArgumentList(QStringLiteral("SetAutoCheckUpdates"), argumentList);
    }

    inline QDBusPendingReply<> SetAutoDownloadUpdates(bool in0)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(in0);
        return asyncCallWithArgumentList(QStringLiteral("SetAutoDownloadUpdates"), argumentList);
    }

    inline QDBusPendingReply<> SetMirrorSource(const QString &in0)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(in0);
        return asyncCallWithArgumentList(QStringLiteral("SetMirrorSource"), argumentList);
    }

Q_SIGNALS: // SIGNALS
};

namespace com {
  namespace deepin {
    namespace lastore {
      typedef ::LastoreUpdaterInterface Updater;
    }
  }
}
#endif