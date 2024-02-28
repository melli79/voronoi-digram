//
// Created by Melchior Grützmann on 2024-02-26.
//

#include "VoronoiGraph.h"

#include <QApplication>
#include <QtGui>
#include <random>

// namespace bp = boost::polygon;
// using boost::polygon::voronoi_builder;
typedef VoronoiGraph::VDiagram  VDiagram;

template <>
struct boost::polygon::geometry_concept<Point> { typedef point_concept type; };

template <>
struct boost::polygon::point_traits<Point> {
    typedef double coordinate_type;

    static inline double get(const Point& point, orientation_2d const& orient) {
        return (orient == HORIZONTAL) ? point.x : point.y;
    }
};

template <>
struct boost::polygon::geometry_concept<Segment> { typedef segment_concept type; };

template <>
struct boost::polygon::segment_traits<Segment> {
    typedef int coordinate_type;
    typedef Point point_type;

    static inline Point get(const Segment& segment, direction_1d const& dir) {
        return dir.to_int() ? segment.p1 : segment.p0;
    }
};

#include "voronoi-utils.h"


typedef std::mt19937_64  Radom;
typedef std::uniform_real_distribution<>  Uniform;


std::vector<Point> createBasePoints() {
    std::vector<Point> result;
    const unsigned N = 10;
    auto random = Radom(std::random_device()());
    auto u01 = Uniform(0.0, 1.0);
    for (unsigned n=0; n<N; ++n)
        result.push_back({u01(random), u01(random)});
    return result;
}

VoronoiGraph::VoronoiGraph(QWidget* parent) :QWidget(parent), basePoints(createBasePoints()) {
    setMinimumSize(400, 300);
    setWindowTitle("Voronoi cells");
    setAttribute(Qt::WA_StaticContents);
}

VoronoiGraph::~VoronoiGraph() = default;

void VoronoiGraph::upscale(int width, int height) {
    points.clear();
    double dx = 0.9*std::min(width, height);
    this->scale = { -(width/dx-1.0)/2, 1.0+(height/dx-1.0)/2, dx, -dx };
    for (auto const& p : basePoints) {
        points.push_back({scale.px(p.x), scale.py(p.y)});
    }
    std::vector<Segment> empty;
    vd.clear();
    construct_voronoi(points.begin(), points.end(), empty.begin(), empty.end(), &vd);
    this->lastWidth = width;
}

Point VoronoiGraph::retrieve_point(VDiagram::cell_type const& cell) const {
    auto index = cell.source_index();
    auto category = cell.source_category();
    assert (category == bp::SOURCE_CATEGORY_SINGLE_POINT);
    return points[index];
    // index -= points.size();
    // if (category == bp::SOURCE_CATEGORY_SEGMENT_START_POINT) {
    //     return boost::polygon::low(segments[index]);
    // } else {
    //     return boost::polygon::high(segments[index]);
    // }
}

std::vector<Point> VoronoiGraph::clip_infinite_edge(VDiagram::edge_type const& edge) const {
    const VDiagram::cell_type& cell1 = *edge.cell();
    const VDiagram::cell_type& cell2 = *edge.twin()->cell();
    Point origin, direction;
    assert (cell1.contains_point() && cell2.contains_point());
        Point p1 = retrieve_point(cell1);
        Point p2 = retrieve_point(cell2);
        origin.x = (p1.x + p2.x)/2;
        origin.y = (p1.y + p2.y)/2;
        direction.x = p1.y - p2.y;
        direction.y = p2.x - p1.x;
    // } else {
    //     // Infinite edges could not be created by two segment sites.
    //     origin = cell1.contains_segment() ?
    //                  retrieve_point(cell2) :
    //                  retrieve_point(cell1);
    //     Segment segment = cell1.contains_segment() ?
    //                                retrieve_segment(cell1) :
    //                                retrieve_segment(cell2);
    //     double dx = boost::polygon::high(segment).x() - boost::polygon::low(segment).x();
    //     double dy = boost::polygon::high(segment).y() - boost::polygon::low(segment).y();
    //     if ((boost::polygon::low(segment) == origin) ^ cell1.contains_point()) {
    //         direction.x(dy);
    //         direction.y(-dx);
    //     } else {
    //         direction.x(-dy);
    //         direction.y(dx);
    //     }
    // }
    double side = scale.dx;
    double koef = side / std::max(fabs(direction.x), fabs(direction.y));
    std::vector<Point> clipped_edge;
    if (edge.vertex0() == nullptr) {
        clipped_edge.push_back({
            origin.x - direction.x * koef,
            origin.y - direction.y * koef});
    } else {
        clipped_edge.push_back({edge.vertex0()->x(), edge.vertex0()->y()});
    }
    if (edge.vertex1() == nullptr) {
        clipped_edge.push_back({
            origin.x + direction.x * koef,
            origin.y + direction.y * koef});
    } else {
        clipped_edge.push_back({
            edge.vertex1()->x(), edge.vertex1()->y()});
    }
    return clipped_edge;
}

// std::vector<Point> VoronoiGraph::sample_curved_edge(VDiagram::edge_type const& edge) const {
//     double max_dist = 1E-3 * scale.dx;
//     Point point = edge.cell()->contains_point() ?
//                            retrieve_point(*edge.cell()) :
//                            retrieve_point(*edge.twin()->cell());
//     Segment segment = edge.cell()->contains_point() ?
//                                retrieve_segment(*edge.twin()->cell()) :
//                                retrieve_segment(*edge.cell());
//     return voronoi_utils<double>::discretize(point, segment, max_dist);
// }

void VoronoiGraph::paintEvent(QPaintEvent*) {
    if (upscaling)
        return;
    if (width()!=lastWidth) {
        upscaling = true;
        QMetaObject::invokeMethod(this, &VoronoiGraph::doUpscale, Qt::QueuedConnection);
        // QTimer::singleShot(100, this, &VoronoiGraph::doUpscale);
        return;
    }

    auto p = QPainter(this);
    p.setPen(QPen(QBrush(Qt::blue), 3.0));
    for (Point const& pt : points) {
        // qDebug() << QString("(%1, %2),  ").arg(pt.x).arg(pt.y);
        p.drawEllipse(int(lround(pt.x))-1, int(lround(pt.y))-1, 3,3);
    }
    p.setPen(QPen(QBrush(Qt::darkGreen), 2.0));
    for (auto edge : vd.edges()) {
        std::vector<Point> linestring;
        if (edge.is_finite()) {
            linestring.push_back({edge.vertex0()->x(), edge.vertex0()->y()});
            linestring.push_back({edge.vertex1()->x(), edge.vertex1()->y()});
        } else
            linestring = clip_infinite_edge(edge);
        Point const& p0 = linestring[0];
        int lastX = int(lround(p0.x));  int lastY = int(lround(p0.y));
        for (auto const& pt : linestring) {
            int x = int(lround(pt.x)),  y = int(lround(pt.y));
            p.drawLine(lastX,lastY, x,y);
            lastX = x;  lastY = y;
        }
    }
    p.end();
}

void VoronoiGraph::keyPressEvent(QKeyEvent* event) {
    if (event->key()==Qt::Key_Escape) {
        QApplication::exit();
    }
    if (!event->text().isEmpty()) {
        basePoints = createBasePoints();
        QMetaObject::invokeMethod(this, &VoronoiGraph::doUpscale, Qt::QueuedConnection);
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void VoronoiGraph::doUpscale() {
    upscale(width(), height());
    upscaling = false;
    update();
}

// void VoronoiGraph::resizeEvent(QResizeEvent* event) {
//     if (upscaling) {
//         QWidget::resizeEvent(event);
//         return;
//     }
//     upscaling = true;
//     QTimer::singleShot(500, this, &VoronoiGraph::doUpscale);
//     QWidget::resizeEvent(event);
// }
