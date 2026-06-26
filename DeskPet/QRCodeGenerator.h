#ifndef QRCODEGENERATOR_H
#define QRCODEGENERATOR_H

#include <QPixmap>

class QRCodeGenerator
{
public:
    static QPixmap generateQRCode(const QString &text, int size = 300);

private:
    QRCodeGenerator() = default;
};

#endif // QRCODEGENERATOR_H
