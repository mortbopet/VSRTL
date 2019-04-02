#include <QtTest/QTest>
#include "vsrtl_placeroute.h"

class tst_connectivityGraph : public QObject {
    Q_OBJECT private slots : void testcreateConnectivityGraph();
};

void tst_connectivityGraph::testcreateConnectivityGraph() {
    /*
     * Test is based on the figure presented in figure 5.18 of
     * VLSI Physical Design: From Graph Partitioning to Timing Closure, with the center (abnormally) shaped object being
     * defined as a rectangle
     */
    // Define components
    vsrtl::pr::RoutingComponent chipRect(QPoint(0, 0), QSize(10, 12));
    vsrtl::pr::RoutingComponent c_A(QPoint(2, 3), QSize(3, 4));
    vsrtl::pr::RoutingComponent c_B(QPoint(2, 10), QSize(7, 1));
    vsrtl::pr::RoutingComponent c_C(QPoint(6, 1), QSize(3, 6));

    vsrtl::pr::Placement p{chipRect, {c_A, c_B, c_C}};

    // Define verification routing regions
    auto rr1 = vsrtl::pr::RoutingRegion(QRect(QPoint(0, 0), QSize(2, 1)));
    auto rr2 = vsrtl::pr::RoutingRegion(QRect(QPoint(0, 1), QSize(2, 2)));
    auto rr3 = vsrtl::pr::RoutingRegion(QRect(QPoint(0, 3), QSize(2, 4)));
    auto rr4 = vsrtl::pr::RoutingRegion(QRect(QPoint(0, 7), QSize(2, 3)));
    auto rr5 = vsrtl::pr::RoutingRegion(QRect(QPoint(0, 10), QSize(2, 1)));
    auto rr6 = vsrtl::pr::RoutingRegion(QRect(QPoint(0, 11), QSize(2, 1)));

    auto rr7 = vsrtl::pr::RoutingRegion(QRect(QPoint(2, 0), QSize(3, 1)));
    auto rr8 = vsrtl::pr::RoutingRegion(QRect(QPoint(2, 1), QSize(3, 2)));
    auto rr9 = vsrtl::pr::RoutingRegion(QRect(QPoint(2, 7), QSize(3, 3)));
    auto rr10 = vsrtl::pr::RoutingRegion(QRect(QPoint(2, 11), QSize(7, 1)));

    auto rr11 = vsrtl::pr::RoutingRegion(QRect(QPoint(5, 0), QSize(1, 1)));
    auto rr12 = vsrtl::pr::RoutingRegion(QRect(QPoint(5, 1), QSize(1, 2)));
    auto rr13 = vsrtl::pr::RoutingRegion(QRect(QPoint(5, 3), QSize(1, 4)));
    auto rr14 = vsrtl::pr::RoutingRegion(QRect(QPoint(5, 7), QSize(1, 3)));

    auto rr15 = vsrtl::pr::RoutingRegion(QRect(QPoint(6, 0), QSize(3, 1)));
    auto rr16 = vsrtl::pr::RoutingRegion(QRect(QPoint(6, 7), QSize(3, 3)));

    auto rr17 = vsrtl::pr::RoutingRegion(QRect(QPoint(9, 0), QSize(1, 1)));
    auto rr18 = vsrtl::pr::RoutingRegion(QRect(QPoint(9, 1), QSize(1, 6)));
    auto rr19 = vsrtl::pr::RoutingRegion(QRect(QPoint(9, 7), QSize(1, 3)));
    auto rr20 = vsrtl::pr::RoutingRegion(QRect(QPoint(9, 10), QSize(1, 1)));
    auto rr21 = vsrtl::pr::RoutingRegion(QRect(QPoint(9, 11), QSize(1, 1)));

    // Set routing region connections
    rr1.right = &rr7;
    rr1.bottom = &rr2;

    rr2.top = &rr1;
    rr2.right = &rr8;
    rr2.bottom = &rr3;

    rr3.top = &rr2;
    rr3.bottom = &rr4;

    rr4.top = &rr3;
    rr4.right = &rr9;
    rr4.bottom = &rr5;

    rr5.top = &rr4;
    rr5.bottom = &rr6;

    rr6.top = &rr5;
    rr6.right = &rr10;

    rr7.left = &rr1;
    rr7.bottom = &rr8;
    rr7.right = &rr11;

    rr8.top = &rr7;
    rr8.left = &rr2;
    rr8.right = &rr12;

    rr9.left = &rr4;
    rr9.right = &rr14;

    rr10.left = &rr6;
    rr10.right = &rr21;

    rr11.left = &rr7;
    rr11.bottom = &rr12;
    rr11.right = &rr15;

    rr12.left = &rr8;
    rr12.top = &rr11;
    rr12.bottom = &rr13;

    rr13.top = &rr12;
    rr13.bottom = &rr14;

    rr14.top = &rr13;
    rr14.left = &rr9;
    rr14.right = &rr16;

    rr15.left = &rr11;
    rr15.right = &rr17;

    rr16.left = &rr14;
    rr16.right = &rr19;

    rr17.left = &rr15;
    rr17.bottom = &rr18;

    rr18.top = &rr17;
    rr18.bottom = &rr19;

    rr19.top = &rr18;
    rr19.left = &rr16;
    rr19.bottom = &rr20;

    rr20.top = &rr19;
    rr20.bottom = &rr21;

    rr21.top = &rr20;
    rr21.left = &rr10;

    std::vector<vsrtl::pr::RoutingRegion> verificationRegions;
    verificationRegions.push_back(rr1);
    verificationRegions.push_back(rr2);
    verificationRegions.push_back(rr3);
    verificationRegions.push_back(rr4);
    verificationRegions.push_back(rr5);
    verificationRegions.push_back(rr6);
    verificationRegions.push_back(rr7);
    verificationRegions.push_back(rr8);
    verificationRegions.push_back(rr9);
    verificationRegions.push_back(rr10);
    verificationRegions.push_back(rr11);
    verificationRegions.push_back(rr12);
    verificationRegions.push_back(rr13);
    verificationRegions.push_back(rr14);
    verificationRegions.push_back(rr15);
    verificationRegions.push_back(rr16);
    verificationRegions.push_back(rr17);
    verificationRegions.push_back(rr18);
    verificationRegions.push_back(rr19);
    verificationRegions.push_back(rr20);
    verificationRegions.push_back(rr21);

    auto regions = vsrtl::pr::createConnectivityGraph(p);

    QVERIFY(regions.size() == verificationRegions.size());

    // Verify that computed regions are equal to expected regions
    for (auto it = verificationRegions.begin(); it != verificationRegions.end();) {
        auto it_res = std::find_if(regions.begin(), regions.end(), [it](const auto& r) { return *r.get() == *it; });
        if (it_res != regions.end()) {
            it = verificationRegions.erase(it);
        } else {
            it++;
        }
    }
    QVERIFY(verificationRegions.size() == 0);
}

QTEST_APPLESS_MAIN(tst_connectivityGraph)
#include "tst_connectivitygraph.moc"
