#pragma once

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QString>
#include <QVector>

// Model 示例：QML 里 new 出来的数据模型（qmlRegisterType 注册）
// 用法：import DodModel 1.0 -> SerialPortModel { id: portModel }
class SerialPortModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles { NameRole = Qt::UserRole + 1, DescriptionRole };

    explicit SerialPortModel(QObject* parent = nullptr);

    int                    rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant               data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // QML 里直接调：portModel.reload()
    Q_INVOKABLE void reload();

private:
    struct PortInfo {
        QString name;
        QString description;
    };

    QVector<PortInfo> m_ports;
};
