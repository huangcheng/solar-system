#include <QtTest>
#include <QDateTime>
#include <QTimeZone>
#include <QVector3D>
#include "SunModel.h"

class TestSunModel : public QObject { Q_OBJECT
private slots:
    void directionIsUnitLength() {
        SunModel s;
        s.setUtc(QDateTime::currentDateTimeUtc());
        QVERIFY2(qAbs(s.sunDirection().length() - 1.0) < 1e-6, "not unit length");
    }
    void declinationBoundedToAxialTilt() {
        SunModel s;
        for (int month = 1; month <= 12; ++month) {
            s.setUtc(QDateTime(QDate(2024, month, 15), QTime(12, 0), QTimeZone::UTC));
            const double dec = s.subSolarLatitude();
            QVERIFY2(dec >= -23.5 && dec <= 23.5, qPrintable(QString("dec=%1").arg(dec)));
        }
    }
    void longitudeBounded() {
        SunModel s;
        s.setUtc(QDateTime::currentDateTimeUtc());
        const double lon = s.subSolarLongitude();
        QVERIFY(lon >= -180.0 && lon <= 180.0);
    }
    void juneSolsticeDeclinationPositive() {
        SunModel s;
        s.setUtc(QDateTime(QDate(2024, 6, 21), QTime(3, 0), QTimeZone::UTC));
        QVERIFY2(s.subSolarLatitude() > 22.0, qPrintable(QString("dec=%1").arg(s.subSolarLatitude())));
    }
};

QTEST_GUILESS_MAIN(TestSunModel)
#include "test_sunmodel.moc"
