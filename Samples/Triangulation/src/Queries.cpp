#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/partition_2.h>
#include <CGAL/Partition_traits_2.h>
#include <CGAL/property_map.h>

#include "xnGeometry.h"

Dg::ErrorCode ConvexPartition(xn::DgPolygon const &polygon, xn::PolygonGroup *pOut)
{
  typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
  typedef CGAL::Partition_traits_2<K, CGAL::Pointer_property_map<K::Point_2>::type > Partition_traits_2;
  typedef Partition_traits_2::Point_2                         Point_2;
  typedef Partition_traits_2::Polygon_2                       Polygon_2;  // a polygon of indices
  typedef std::list<Polygon_2>                                Polygon_list;

  xn::DgPolygon ccwPolygon = polygon;
  ccwPolygon.SetWinding(Dg::Orientation::CCW);

  std::vector<K::Point_2> points;
  for (auto it = ccwPolygon.cPointsBegin(); it != ccwPolygon.cPointsEnd(); it++)
    points.push_back(K::Point_2(it->x(), it->y()));

  Polygon_list partition_polys;

  Polygon_2 CGAL_polygon;
  for (size_t i = 0; i < points.size(); i++)
    CGAL_polygon.push_back(i);

  Partition_traits_2 traits(CGAL::make_property_map(points));

  CGAL::approx_convex_partition_2(CGAL_polygon.vertices_begin(),
    CGAL_polygon.vertices_end(),
    std::back_inserter(partition_polys),
    traits);

  for (const Polygon_2 &poly : partition_polys)
  {
    xn::Polygon outPoly;
    for (Point_2 p : poly.container())
    {
      K::Point_2 point = points[p];
      outPoly.PushBack(xn::vec2((float)point.x(), (float)point.y()));
    }
    pOut->polygons.push_back(outPoly);
  }

  return Dg::ErrorCode::None;
}