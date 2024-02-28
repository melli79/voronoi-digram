//
// Created by Melchior Grützmann on 2024-02-26.
//

#ifndef VORONOIGRAPH_H
#define VORONOIGRAPH_H

#include <QWidget>
#include <boost/polygon/voronoi.hpp>
#include <vector>

namespace bp = boost::polygon;

struct Point {
    typedef double  coordinate_type;
    double x=0.0, y=0.0;

    [[nodiscard]]
    inline double get(bp::orientation_2d const& orient) {
        return (orient == bp::HORIZONTAL) ? x : y;
    }};

struct Segment {
    typedef Point::coordinate_type coordinate_type;
    typedef Point point_type;
    Point p0;
    Point p1;
    Segment (double x1, double y1, double x2, double y2) : p0{x1, y1}, p1{x2, y2} {
    }
};

struct Rect {
    double x0, y0, dx, dy;

    static Rect of(double x0, double y0, double x1, double y1) {
        return { x0,y0, x1-x0, y1-y0 };
    }

    [[nodiscard]] double px(double x) const {
        return (x-x0)*dx;
    }

    [[nodiscard]] double py(double y) const {
        return (y-y0)*dy;
    }
};

class VoronoiGraph :public QWidget {
    Q_OBJECT

public:
    explicit VoronoiGraph(QWidget* parent =nullptr);
    ~VoronoiGraph() override;

    void upscale(int width, int height);

    void paintEvent(QPaintEvent*) override;

    // void resizeEvent(QResizeEvent* event) override;

    typedef boost::polygon::voronoi_diagram<double> VDiagram;

public slots:
    void doUpscale();

protected:
    Point retrieve_point(boost::polygon::voronoi_cell<double> const &cell) const;
    std::vector<Point> clip_infinite_edge(boost::polygon::voronoi_edge<double> const &edge) const;

private:
    std::vector<Point> basePoints;
    bool upscaling = false;
    Rect scale;
    int lastWidth = 0;
    std::vector<Point> points;
    VDiagram vd;
};


#endif //VORONOIGRAPH_H
