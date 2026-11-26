/***
 * @Author: dawn 1964173012@qq.com
 * @Date: 2026-03-18
 * @LastEditors: dawn 1964173012@qq.com
 * @LastEditTime: 2026-03-18
 * @FilePath: /DOD-Mix24/code/Utility/ImageColorizer/ImageColorizer.h
 * @Description: 图片着色器，支持 PNG/JPG/SVG 等格式的颜色替换
 * @
 * @Copyright (c) 2026 by AmsAudio Li YuMing, All Rights Reserved.
 */

#pragma once

#include "Log.h" // 统一日志（换日志库只改 Utility/Log.*，这里不直接依赖 glog/spdlog）

#include <QCoreApplication>
#include <QFile>
#include <QHash>
#include <QIODevice>
#include <QImage>
#include <QMutex>
#include <QPainter>
#include <QQuickImageProvider>
#include <QRegularExpression>
#include <QSvgRenderer>

class ImageColorizer : public QQuickImageProvider
{
public:
    static ImageColorizer* instance()
    {
        static ImageColorizer inst;
        return &inst;
    }

    ImageColorizer()
        : QQuickImageProvider(QQuickImageProvider::Image)
    {}

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override
    {
        QMutexLocker locker(&m_mutex);

        // 格式: "image://customImage/icon.svg?color=red;path=res/id"
        // 或者: "icon.svg?color=red" (兼容简化格式)
        QString imagePath;
        QString colorStr; // 空表示不替换颜色
        QString pathPrefix;
        QSize   sizeFromParam;
        bool    hasColorParam = false;

        // 去掉 "image://customImage/" 前缀
        QString data = id;
        if (data.startsWith("image://customImage/")) {
            data = data.mid(QString("image://customImage/").length());
        }

        // 分割主路径和参数部分
        QString args;
        int     sepIndex = data.indexOf('?');
        if (sepIndex > 0) {
            imagePath = data.left(sepIndex);
            args      = data.mid(sepIndex + 1);
        }
        else {
            imagePath = data;
            args      = "";
        }

        // 解析参数: color=red;path=xxx;size=128*128
        // 同时兼容 '&' 分隔：color=...&path=...&size=...
        QStringList params = args.split(QRegularExpression("[;&]"), Qt::SkipEmptyParts);
        for (const QString& param : params) {
            QStringList kv = param.split('=');
            if (kv.size() == 2) {
                QString key = kv[0].trimmed();
                QString val = kv[1].trimmed();
                if (key == "color") {
                    colorStr      = val;
                    hasColorParam = true;
                }
                else if (key == "path") {
                    pathPrefix = val;
                }
                else if (key == "size") {
                    sizeFromParam = parseSizeParam(val);
                }
            }
        }

        if (imagePath.isEmpty()) {
            LOG_WARN("ImageColorizer: 空图片路径, id={}", id);
            return QImage();
        }

        // 解析目标尺寸
        QSize targetSize = sizeFromParam.isValid() ? sizeFromParam : requestedSize;
        if (!targetSize.isValid() || targetSize.isEmpty()) {
            targetSize = QSize(64, 64);
        }

        // 基础路径: 由 path 参数决定（默认 res/id），具体解析见 resolveImagePath
        QImage image;

        // 去掉前导 /
        if (imagePath.startsWith('/')) {
            imagePath = imagePath.mid(1);
        }

        // 检查是否为 SVG 文件
        bool isSvg = imagePath.endsWith(".svg", Qt::CaseInsensitive);

        // 资源路径：path 参数（默认 res/id）；先找 exe 同级磁盘目录，再回退 qrc
        const QString relPath  = (pathPrefix.isEmpty() ? QStringLiteral("res/id") : pathPrefix) + QLatin1Char('/') + imagePath;
        const QString fullPath = resolveImagePath(relPath);

        if (isSvg) {
            // SVG 渲染方式
            if (hasColorParam && !colorStr.isEmpty()) {
                QColor tintColor = parseColor(colorStr);
                if (tintColor.isValid()) {
                    image = renderSvgWithColor(fullPath, tintColor, targetSize);
                }
                else {
                    image = renderSvg(fullPath, fullPath, targetSize);
                }
            }
            else {
                image = renderSvg(fullPath, fullPath, targetSize);
            }
        }
        else {
            // PNG/JPG 等位图加载
            image.load(fullPath);
            if (!image.isNull()) {
                LOG_DEBUG("ImageColorizer: 已加载位图 {}", fullPath);
            }

            if (!image.isNull() && targetSize.isValid()) {
                image = image.scaled(targetSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            }
        }

        if (image.isNull()) {
            LOG_WARN("ImageColorizer: 返回空图片 path={} isSvg={} requested={}x{} target={}x{}", fullPath, isSvg, requestedSize.width(), requestedSize.height(), targetSize.width(),
                     targetSize.height());
            return QImage();
        }

        if (size) {
            *size = image.size();
        }

        // PNG/JPG 着色调处理
        if (!isSvg && hasColorParam && !colorStr.isEmpty()) {
            QColor tintColor = parseColor(colorStr);
            if (tintColor.isValid()) {
                image = tintImage(image, tintColor);
            }
        }

        return image;
    }

    // SVG 渲染方法
    QImage renderSvg(const QString& path1, const QString& /*path2*/, const QSize& targetSize)
    {
        QSvgRenderer renderer;

        QFile file(path1);
        if (!file.open(QIODevice::ReadOnly)) {
            LOG_WARN("ImageColorizer: SVG 打开失败: {}", path1);
            return QImage();
        }

        QByteArray svgData = file.readAll();
        file.close();

        if (!renderer.load(svgData)) {
            LOG_WARN("ImageColorizer: SVG 解析失败: {}", path1);
            return QImage();
        }

        LOG_TRACE("ImageColorizer: SVG 已加载: {}", path1);

        QSize finalSize   = targetSize;
        QSize defaultSize = renderer.defaultSize();
        if (!defaultSize.isValid() || defaultSize.isEmpty()) {
            defaultSize = QSize(128, 128);
        }

        // Handle edge cases like 20x0 / 0x20: keep aspect ratio from SVG default size.
        if (finalSize.width() <= 0 && finalSize.height() <= 0) {
            finalSize = defaultSize;
        }
        else if (finalSize.width() <= 0) {
            int w = (defaultSize.height() > 0) ? (finalSize.height() * defaultSize.width()) / defaultSize.height() : finalSize.height();
            finalSize.setWidth(w > 0 ? w : finalSize.height());
        }
        else if (finalSize.height() <= 0) {
            int h = (defaultSize.width() > 0) ? (finalSize.width() * defaultSize.height()) / defaultSize.width() : finalSize.width();
            finalSize.setHeight(h > 0 ? h : finalSize.width());
        }

        if (finalSize.width() <= 0 || finalSize.height() <= 0) {
            finalSize = defaultSize;
        }

        QImage image(finalSize, QImage::Format_ARGB32);
        if (image.isNull()) {
            LOG_WARN("ImageColorizer: SVG 渲染失败(分配图片) path={} size={}x{}", path1, finalSize.width(), finalSize.height());
            return QImage();
        }
        image.fill(Qt::transparent);

        QPainter painter;
        if (!painter.begin(&image)) {
            LOG_WARN("ImageColorizer: SVG 渲染失败(painter 未激活) path={} size={}x{}", path1, finalSize.width(), finalSize.height());
            return QImage();
        }
        renderer.render(&painter);
        painter.end();
        return image;
    }

    // 解析 SVG 并替换颜色后渲染
    QImage renderSvgWithColor(const QString& svgPath, const QColor& fillColor, const QSize& targetSize)
    {
        QByteArray svgData;
        QString    loadedPath;

        // 直接从完整路径加载
        QFile file(svgPath);
        if (file.open(QIODevice::ReadOnly)) {
            svgData    = file.readAll();
            loadedPath = svgPath;
            file.close();
        }

        if (svgData.isEmpty()) {
            LOG_WARN("ImageColorizer: SVG 读取失败(空数据): {}", svgPath);
            return QImage();
        }

        // DLOG(INFO) << QString("ImageColorizer: SVG file loaded: %1").arg(loadedPath).toStdString();

        // 替换 SVG 中的 fill 和 stroke 颜色
        QString svgString = QString::fromUtf8(svgData);

        // 替换 fill="..." 和 stroke="..." 为目标颜色
        QString colorHex = fillColor.name();
        svgString.replace(QRegularExpression(R"(fill="[^"]*")"), QString(R"(fill="%1")").arg(colorHex));
        svgString.replace(QRegularExpression(R"(stroke="[^"]*")"), QString(R"(stroke="%1")").arg(colorHex));

        QSvgRenderer renderer(svgString.toUtf8());

        if (!renderer.isValid()) {
            LOG_WARN("ImageColorizer: 着色后 SVG 无效: {}", svgPath);
            return QImage();
        }

        QSize finalSize   = targetSize;
        QSize defaultSize = renderer.defaultSize();
        if (!defaultSize.isValid() || defaultSize.isEmpty()) {
            defaultSize = QSize(128, 128);
        }

        // Handle edge cases like 20x0 / 0x20: keep aspect ratio from SVG default size.
        if (finalSize.width() <= 0 && finalSize.height() <= 0) {
            finalSize = defaultSize;
        }
        else if (finalSize.width() <= 0) {
            int w = (defaultSize.height() > 0) ? (finalSize.height() * defaultSize.width()) / defaultSize.height() : finalSize.height();
            finalSize.setWidth(w > 0 ? w : finalSize.height());
        }
        else if (finalSize.height() <= 0) {
            int h = (defaultSize.width() > 0) ? (finalSize.width() * defaultSize.height()) / defaultSize.width() : finalSize.width();
            finalSize.setHeight(h > 0 ? h : finalSize.width());
        }

        if (finalSize.width() <= 0 || finalSize.height() <= 0) {
            finalSize = defaultSize;
        }

        QImage image(finalSize, QImage::Format_ARGB32);
        if (image.isNull()) {
            LOG_WARN("ImageColorizer: 着色渲染失败(分配图片) path={} size={}x{}", svgPath, finalSize.width(), finalSize.height());
            return QImage();
        }
        image.fill(Qt::transparent);

        QPainter painter;
        if (!painter.begin(&image)) {
            LOG_WARN("ImageColorizer: 着色渲染失败(painter 未激活) path={} size={}x{}", svgPath, finalSize.width(), finalSize.height());
            return QImage();
        }
        renderer.render(&painter);
        painter.end();

        return image;
    }

private:
    // 先找 exe 同级磁盘目录（用户可直接替换图标），找不到再用 qrc（图标与 QML 一起内嵌）
    QString resolveImagePath(const QString& relPath) const
    {
        const QString diskPath = QCoreApplication::applicationDirPath() + QLatin1Char('/') + relPath;
        if (QFile::exists(diskPath)) {
            return diskPath;
        }
        const QString qrcPath = QStringLiteral(":/") + relPath;
        if (QFile::exists(qrcPath)) {
            return qrcPath;
        }
        return diskPath;
    }

    QSize parseSizeParam(const QString& sizeStr) const
    {
        QString s = sizeStr.trimmed().toLower();
        if (s.isEmpty()) {
            return QSize();
        }

        // 兼容: 128*128 / 128x128
        QRegularExpression      re2D("^(\\d+)\\s*[*x]\\s*(\\d+)$");
        QRegularExpressionMatch m2D = re2D.match(s);
        if (m2D.hasMatch()) {
            int w = m2D.captured(1).toInt();
            int h = m2D.captured(2).toInt();
            if (w > 0 && h > 0) {
                return QSize(w, h);
            }
            return QSize();
        }

        // 兼容: 128 -> 128x128
        QRegularExpression      re1D("^(\\d+)$");
        QRegularExpressionMatch m1D = re1D.match(s);
        if (m1D.hasMatch()) {
            int v = m1D.captured(1).toInt();
            if (v > 0) {
                return QSize(v, v);
            }
        }

        return QSize();
    }

    QColor parseColor(const QString& colorStr) const
    {
        if (colorStr.isEmpty()) {
            return QColor();
        }

        // 颜色名称映射
        static const QHash<QString, QColor> colorMap = {
            {"red", QColor(255, 0, 0)},      {"green", QColor(0, 255, 0)},    {"blue", QColor(0, 0, 255)},      {"white", QColor(255, 255, 255)}, {"black", QColor(0, 0, 0)},
            {"yellow", QColor(255, 255, 0)}, {"cyan", QColor(0, 255, 255)},   {"magenta", QColor(255, 0, 255)}, {"gray", QColor(128, 128, 128)},  {"grey", QColor(128, 128, 128)},
            {"orange", QColor(255, 165, 0)}, {"purple", QColor(128, 0, 128)}, {"pink", QColor(255, 192, 203)},  {"brown", QColor(165, 42, 42)},
        };

        if (colorMap.contains(colorStr.toLower())) {
            return colorMap[colorStr.toLower()];
        }

        // 尝试解析颜色
        QString colorInput = colorStr;
        // 如果没有 # 前缀但看起来像十六进制 (6位)，则添加 #
        if (!colorInput.startsWith('#') && QRegularExpression("^[0-9A-Fa-f]{6}$").match(colorInput).hasMatch()) {
            colorInput = "#" + colorInput;
        }
        QColor color(colorInput);
        if (color.isValid()) {
            return color;
        }

        return QColor();
    }

    QImage tintImage(const QImage& source, const QColor& tintColor) const
    {
        QImage result = source.convertToFormat(QImage::Format_ARGB32);

        int r = tintColor.red();
        int g = tintColor.green();
        int b = tintColor.blue();

        for (int y = 0; y < result.height(); ++y) {
            QRgb* line = reinterpret_cast<QRgb*>(result.scanLine(y));
            for (int x = 0; x < result.width(); ++x) {
                QRgb pixel = line[x];
                int  a     = qAlpha(pixel);

                if (a > 0) {
                    // 将像素颜色与目标颜色混合
                    line[x] = qRgba(r, g, b, a);
                }
            }
        }

        return result;
    }

    mutable QMutex m_mutex;
};
