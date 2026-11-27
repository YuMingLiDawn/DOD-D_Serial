#include "AmsApplication.h"

#include "ApplicationContext.h"
#include "ImageColorizer/ImageColorizer.h"
#include "Log.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlError>
#include <QQuickWidget>
#include <QUrl>
#include <QVBoxLayout>

AmsApplication::AmsApplication(QWidget* parent)
    : QWidget(parent)
    , m_quickWidget(new QQuickWidget(this))
{
    resize(800, 600);

    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickWidget->engine()->addImportPath(QStringLiteral("qrc:/"));
    m_quickWidget->engine()->addImageProvider(QStringLiteral("customImage"), ImageColorizer::instance());
    injectContextProperties();

    const QUrl qmlUrl(QStringLiteral("qrc:/Qml/Main.qml"));
    m_quickWidget->setSource(qmlUrl);

    if (m_quickWidget->status() == QQuickWidget::Error) {
        LOG_ERROR("加载 QML 失败: {}", qmlUrl.toString());
        const QList<QQmlError> errors = m_quickWidget->errors();
        for (const QQmlError& error : errors) {
            LOG_ERROR("  - {}", error.toString());
        }
    }

    auto* layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_quickWidget);
    setLayout(layout);
}

AmsApplication::~AmsApplication() = default;

void AmsApplication::injectContextProperties()
{
    QQmlContext*                    context = m_quickWidget->rootContext();
    const QHash<QString, QObject*>& objects = ApplicationContext::instance().contextMap();

    for (auto it = objects.cbegin(); it != objects.cend(); ++it) {
        context->setContextProperty(it.key(), it.value());
    }

    LOG_DEBUG("已注入全局对象 ({} 个): {}", objects.size(), objects.keys().join(QStringLiteral(", ")));
}
