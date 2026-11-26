#include "SerialPortModel.h"

#include "Log.h"

SerialPortModel::SerialPortModel(QObject* parent)
    : QAbstractListModel(parent)
{
    reload();
}

int SerialPortModel::rowCount(const QModelIndex& parent) const
{
    // 列表模型：有父索引就没有子行
    return parent.isValid() ? 0 : m_ports.size();
}

QVariant SerialPortModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_ports.size()) {
        return {};
    }
    const PortInfo& port = m_ports.at(index.row());
    switch (role) {
        case NameRole: return port.name;
        case DescriptionRole: return port.description;
        default: return {};
    }
}

QHash<int, QByteArray> SerialPortModel::roleNames() const
{
    // QML delegate 里用 model.name / model.description
    return {{NameRole, "name"}, {DescriptionRole, "description"}};
}

void SerialPortModel::reload()
{
    // 真实实现：从 SerialService 拿当前串口列表
    beginResetModel();
    m_ports = {{QStringLiteral("ttyUSB0"), QStringLiteral("USB-Serial")}, {QStringLiteral("ttyS0"), QStringLiteral("板载串口")}};
    endResetModel();

    LOG_INFO("[SerialPortModel] reload 完成, {} 项", m_ports.size());
}
