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
    vsrtl::eda::RoutingComponent chipRect(QPoint(0, 0), QSize(10, 12));
    vsrtl::eda::RoutingComponent c_A(QPoint(2, 3), QSize(3, 4));
    vsrtl::eda::RoutingComponent c_B(QPoint(2, 10), QSize(7, 1));
    vsrtl::eda::RoutingComponent c_C(QPoint(6, 1), QSize(3, 6));

    vsrtl::eda::Placement p{chipRect, {c_A, c_B, c_C}};

    // Define verification routing regions
    auto rr1 = vsrtl::eda::RoutingRegion(QRect(QPoint(0, 0), QSize(2, 1)));
    auto rr2 = vsrtl::eda::RoutingRegion(QRect(QPoint(0, 1), QSize(2, 2)));
    auto rr3 = vsrtl::eda::RoutingRegion(QRect(QPoint(0, 3), QSize(2, 4)));
    auto rr4 = vsrtl::eda::RoutingRegion(QRect(QPoint(0, 7), QSize(2, 3)));
    auto rr5 = vsrtl::eda::RoutingRegion(QRect(QPoint(0, 10), QSize(2, 1)));
    auto rr6 = vsrtl::eda::RoutingRegion(QRect(QPoint(0, 11), QSize(2, 1)));

    auto rr7 = vsrtl::eda::RoutingRegion(QRect(QPoint(2, 0), QSize(3, 1)));
    auto rr8 = vsrtl::eda::RoutingRegion(QRect(QPoint(2, 1), QSize(3, 2)));
    auto rr9 = vsrtl::eda::RoutingRegion(QRect(QPoint(2, 7), QSize(3, 3)));
    auto rr10 = vsrtl::eda::RoutingRegion(QRect(QPoint(2, 11), QSize(7, 1)));

    auto rr11 = vsrtl::eda::RoutingRegion(QRect(QPoint(5, 0), QSize(1, 1)));
    auto rr12 = vsrtl::eda::RoutingRegion(QRect(QPoint(5, 1), QSize(1, 2)));
    auto rr13 = vsrtl::eda::RoutingRegion(QRect(QPoint(5, 3), QSize(1, 4)));
    auto rr14 = vsrtl::eda::RoutingRegion(QRect(QPoint(5, 7), QSize(1, 3)));

    auto rr15 = vsrtl::eda::RoutingRegion(QRect(QPoint(6, 0), QSize(3, 1)));
    auto rr16 = vsrtl::eda::RoutingRegion(QRect(QPoint(6, 7), QSize(3, 3)));

    auto rr17 = vsrtl::eda::RoutingRegion(QRect(QPoint(9, 0), QSize(1, 1)));
    auto rr18 = vsrtl::eda::RoutingRegion(QRect(QPoint(9, 1), QSize(1, 6)));
    auto rr19 = vsrtl::eda::RoutingRegion(QRect(QPoint(9, 7), QSize(1, 3)));
    auto rr20 = vsrtl::eda::RoutingRegion(QRect(QPoint(9, 10), QSize(1, 1)));
    auto rr21 = vsrtl::eda::RoutingRegion(QRect(QPoint(9, 11), QSize(1, 1)));

    // Set routing region connections
    rr1.setRegion(vsrtl::eda::Edge::Right, &rr7);
    rr1.setRegion(vsrtl::eda::Edge::Bottom, &rr2);

    rr2.setRegion(vsrtl::eda::Edge::Top, &rr1);
    rr2.setRegion(vsrtl::eda::Edge::Right, &rr8);
    rr2.setRegion(vsrtl::eda::Edge::Bottom, &rr3);

    rr3.setRegion(vsrtl::eda::Edge::Top, &rr2);
    rr3.setRegion(vsrtl::eda::Edge::Bottom, &rr4);

    rr4.setRegion(vsrtl::eda::Edge::Top, &rr3);
    rr4.setRegion(vsrtl::eda::Edge::Right, &rr9);
    rr4.setRegion(vsrtl::eda::Edge::Bottom, &rr5);

    rr5.setRegion(vsrtl::eda::Edge::Top, &rr4);
    rr5.setRegion(vsrtl::eda::Edge::Bottom, &rr6);

    rr6.setRegion(vsrtl::eda::Edge::Top, &rr5);
    rr6.setRegion(vsrtl::eda::Edge::Right, &rr10);

    rr7.setRegion(vsrtl::eda::Edge::Left, &rr1);
    rr7.setRegion(vsrtl::eda::Edge::Bottom, &rr8);
    rr7.setRegion(vsrtl::eda::Edge::Right, &rr11);

    rr8.setRegion(vsrtl::eda::Edge::Top, &rr7);
    rr8.setRegion(vsrtl::eda::Edge::Left, &rr2);
    rr8.setRegion(vsrtl::eda::Edge::Right, &rr12);

    rr9.setRegion(vsrtl::eda::Edge::Left, &rr4);
    rr9.setRegion(vsrtl::eda::Edge::Right, &rr14);

    rr10.setRegion(vsrtl::eda::Edge::Left, &rr6);
    rr10.setRegion(vsrtl::eda::Edge::Right, &rr21);

    rr11.setRegion(vsrtl::eda::Edge::Left, &rr7);
    rr11.setRegion(vsrtl::eda::Edge::Bottom, &rr12);
    rr11.setRegion(vsrtl::eda::Edge::Right, &rr15);

    rr12.setRegion(vsrtl::eda::Edge::Left, &rr8);
    rr12.setRegion(vsrtl::eda::Edge::Top, &rr11);
    rr12.setRegion(vsrtl::eda::Edge::Bottom, &rr13);

    rr13.setRegion(vsrtl::eda::Edge::Top, &rr12);
    rr13.setRegion(vsrtl::eda::Edge::Bottom, &rr14);

    rr14.setRegion(vsrtl::eda::Edge::Top, &rr13);
    rr14.setRegion(vsrtl::eda::Edge::Left, &rr9);
    rr14.setRegion(vsrtl::eda::Edge::Right, &rr16);

    rr15.setRegion(vsrtl::eda::Edge::Left, &rr11);
    rr15.setRegion(vsrtl::eda::Edge::Right, &rr17);

    rr16.setRegion(vsrtl::eda::Edge::Left, &rr14);
    rr16.setRegion(vsrtl::eda::Edge::Right, &rr19);

    rr17.setRegion(vsrtl::eda::Edge::Left, &rr15);
    rr17.setRegion(vsrtl::eda::Edge::Bottom, &rr18);

    rr18.setRegion(vsrtl::eda::Edge::Top, &rr17);
    rr18.setRegion(vsrtl::eda::Edge::Bottom, &rr19);

    rr19.setRegion(vsrtl::eda::Edge::Top, &rr18);
    rr19.setRegion(vsrtl::eda::Edge::Left, &rr16);
    rr19.setRegion(vsrtl::eda::Edge::Bottom, &rr20);

    rr20.setRegion(vsrtl::eda::Edge::Top, &rr19);
    rr20.setRegion(vsrtl::eda::Edge::Bottom, &rr21);

    rr21.setRegion(vsrtl::eda::Edge::Top, &rr20);
    rr21.setRegion(vsrtl::eda::Edge::Left, &rr10);

    std::vector<vsrtl::eda::RoutingRegion> verificationRegions;
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

    auto regions = vsrtl::eda::createConnectivityGraph(p);

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
