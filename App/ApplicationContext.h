#pragma once

#include <QHash>
#include <QObject>
#include <QString>

// 全局对象注册表（对应 WTC3 的 ApplicationContext）
// 只负责「名字 -> QObject*」的登记；真正的注入在窗口里做（每窗口一个 QML 引擎）。
class ApplicationContext
{
public:
    static ApplicationContext& instance();

    void                            setContext(const QString& name, QObject* object);
    QObject*                        contextObject(const QString& name) const;
    const QHash<QString, QObject*>& contextMap() const
    {
        return m_objects;
    }

private:
    ApplicationContext() = default;
    QHash<QString, QObject*> m_objects;
};
