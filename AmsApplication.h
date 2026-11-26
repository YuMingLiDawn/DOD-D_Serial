#pragma once

#include <QWidget>

class QQuickWidget;

// 底层 QWidget 主界面：内部只承载一个 QQuickWidget，由它渲染 Qml/Main.qml
class AmsApplication : public QWidget
{
    Q_OBJECT

public:
    explicit AmsApplication(QWidget* parent = nullptr);
    ~AmsApplication() override;

private:
    // 把 ApplicationContext 里登记的全局对象逐个注入本窗口的 QML 上下文
    // （每个 QQuickWidget 都有自己的 QQmlEngine，所以必须逐窗口注入）
    void injectContextProperties();

    QQuickWidget* m_quickWidget = nullptr;
};
