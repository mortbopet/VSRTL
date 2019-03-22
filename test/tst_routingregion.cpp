#include <QtTest/QTest>
#include "vsrtl_placeroute.h"

class tst_routingregion : public QObject {
    Q_OBJECT private slots : void testDefineRoutingRegions();
};

void tst_routingregion::testDefineRoutingRegions() {
    /*
     * Test is based on the figure presented in figure 5.18 of
     * VLSI Physical Design: From Graph Partitioning to Timing Closure, with the center (abnormally) shaped object being
     * defined as a rectangle
     */
    // Define components
    QRectF chipRect(QPoint(0, 0), QPoint(10, 12));
    QRectF c_A(QPoint(2, 3), QPoint(5, 7));
    QRectF c_B(QPoint(2, 10), QPoint(9, 11));
    QRectF c_C(QPoint(6, 1), QPoint(9, 7));

    vsrtl::Placement p{chipRect, {c_A, c_B, c_C}};

    // Define verification routing regions
    QList<vsrtl::RoutingRegion> verificationRegions;
    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(0, 0), QSizeF(2, 1)));
    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(0, 1), QSizeF(2, 2)));
    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(0, 3), QSizeF(2, 4)));
    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(0, 7), QSizeF(2, 3)));
    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(0, 10), QSizeF(2, 1)));
    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(0, 11), QSizeF(2, 1)));

    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(2, 0), QSizeF(3, 1)));
    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(2, 1), QSizeF(3, 2)));
    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(2, 7), QSizeF(3, 3)));
    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(2, 11), QSizeF(7, 1)));

    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(5, 0), QSizeF(1, 1)));
    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(5, 1), QSizeF(1, 2)));
    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(5, 3), QSizeF(1, 4)));
    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(5, 7), QSizeF(1, 3)));

    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(6, 0), QSizeF(3, 1)));
    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(6, 7), QSizeF(3, 3)));

    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(9, 0), QSizeF(1, 1)));
    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(9, 1), QSizeF(1, 6)));
    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(9, 7), QSizeF(1, 3)));
    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(9, 10), QSizeF(1, 1)));
    verificationRegions << vsrtl::RoutingRegion(QRectF(QPointF(9, 11), QSizeF(1, 1)));

    const auto regions = vsrtl::defineRoutingRegions(p);

    QVERIFY(regions.size() == verificationRegions.size());

    // Verify that computed regions are equal to expected regions
    for (const auto& region : regions) {
        verificationRegions.removeOne(region);
    }
    QVERIFY(verificationRegions.size() == 0);
}

QTEST_APPLESS_MAIN(tst_routingregion)
#include "tst_routingregion.moc"
