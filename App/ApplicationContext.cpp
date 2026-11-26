#include "ApplicationContext.h"

#include "Log.h"

ApplicationContext& ApplicationContext::instance()
{
    static ApplicationContext context;
    return context;
}

void ApplicationContext::setContext(const QString& name, QObject* object)
{
    if (name.isEmpty() || object == nullptr) {
        LOG_WARN("忽略无效登记: name={}, obj={}", name, object == nullptr ? "null" : "valid");
        return;
    }
    // 同名重复登记时以最后一次为准，便于测试/替换实现
    m_objects.insert(name, object);
}

QObject* ApplicationContext::contextObject(const QString& name) const
{
    QObject* object = m_objects.value(name, nullptr);
    if (object == nullptr) {
        LOG_WARN("找不到上下文对象: {}", name);
    }
    return object;
}
