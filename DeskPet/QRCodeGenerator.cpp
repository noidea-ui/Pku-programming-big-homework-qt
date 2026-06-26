#include "QRCodeGenerator.h"

#include <QImage>
#include <QPainter>

extern "C" {
#include <qrencode.h>
}

QPixmap QRCodeGenerator::generateQRCode(const QString &text, int size)
{
    const QByteArray utf8 = text.toUtf8();
    QRcode *qr = QRcode_encodeString8bit(utf8.constData(), 0, QR_ECLEVEL_M);
    if (!qr) {
        return {};
    }

    const int w = qr->width;
    const int margin = 2;               // modules of quiet zone
    const int total = w + 2 * margin;
    const int modulePx = qMax(1, size / total);
    const int imgSize = total * modulePx;

    QImage image(imgSize, imgSize, QImage::Format_RGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);

    for (int y = 0; y < w; ++y) {
        for (int x = 0; x < w; ++x) {
            if (qr->data[y * w + x] & 0x01) {
                painter.fillRect(
                    QRectF((x + margin) * modulePx, (y + margin) * modulePx,
                           modulePx, modulePx),
                    Qt::black);
            }
        }
    }

    painter.end();
    QRcode_free(qr);
    return QPixmap::fromImage(image);
}
