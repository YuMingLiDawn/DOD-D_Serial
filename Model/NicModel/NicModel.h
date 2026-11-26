/***
 * @Author: dawn 1964173012@qq.com
 * @Date: 2026-11-26 18:54:16
 * @LastEditors: dawn 1964173012@qq.com
 * @LastEditTime: 2026-11-26 18:59:21
 * @FilePath: /DOD-D_Serial/Model/NicModel/NicModel.h
 * @Description:
 * @
 * @Copyright (c) 2026 by AmsAudio Li YuMing, All Rights Reserved.
 */

#pragma once
#include <QObject>
#include <QAbstractListModel>

class NicModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit NicModel(QObject* parent = nullptr);
    ~NicModel();

    enum Roles {
        NameRole = Qt::UserRole + 1,
        AddressRole
    };

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
};