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
#include <QDebug>

NicModel::NicModel(QObject* parent)
    : QAbstractListModel(parent)
{
    connect(&NicSearch::instance(), &NicSearch::nicsChanged, this, [this]() {
        qDebug() << "NICs changed";
    });
}

NicModel::~NicModel()
{
}

QHash<int, QByteArray> NicModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[AddressRole] = "address";
    return roles;
}

int NicModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 0; // Replace with actual row count
}

QVariant NicModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    switch (role) {
    case NameRole:
        return QVariant(); // Replace with actual data
    case AddressRole:
        return QVariant(); // Replace with actual data
    default:
        return QVariant();
    }
}
