/***
 * @Author: dawn 1964173012@qq.com
 * @Date: 2026-11-26 18:54:14
 * @LastEditors: dawn 1964173012@qq.com
 * @LastEditTime: 2026-11-26 19:00:02
 * @FilePath: /DOD-D_Serial/Model/NicModel/NicModel.cpp
 * @Description:
 * @
 * @Copyright (c) 2026 by AmsAudio Li YuMing, All Rights Reserved.
 */

#include "NicModel.h"
#include "NicSearch/NicSearch.h"
#include "Infrastructure/NicProvider.h"
#include <QDebug>

NicModel::NicModel(QObject* parent)
    : QAbstractListModel(parent)
{
    NicSearch::instance().start();
    connect(&NicSearch::instance(), &NicSearch::nicsChanged, this, &NicModel::onNicsChanged);
}

NicModel::~NicModel()
{}

QHash<int, QByteArray> NicModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole]    = "name";
    roles[AddressRole] = "address";
    return roles;
}

int NicModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_nics.size(); // Replace with actual row count
}

QVariant NicModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    switch (role) {
        case NameRole: return m_nics.at(index.row()).value("name");
        case AddressRole: return m_nics.at(index.row()).value("ipv4");
        default: return QVariant();
    }
}

void NicModel::onNicsChanged()
{
    beginResetModel();
    m_nics.clear();
    const QVector<NicInfo> current = NicSearch::instance().listWiredNicsRaw();
    for (const NicInfo& nic : current) {
        QVariantMap item;
        item.insert("id", nic.id);
        item.insert("name", nic.name);
        item.insert("ipv4", nic.ipv4);
        item.insert("netmask", nic.netmask);
        item.insert("mac", nic.mac);
        item.insert("broadcast", nic.broadcast);
        item.insert("gateway", nic.gateway);
        m_nics.append(item);
    }
    endResetModel();
}