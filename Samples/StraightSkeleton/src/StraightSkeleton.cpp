
#include <windows.h>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/create_straight_skeleton_from_polygon_with_holes_2.h>
#include <boost/shared_ptr.hpp>

#include "StraightSkeleton.h"
#include "xnGeometricQueries.h"
#include "xnPluginAPI.h"
#include "xnVersion.h"
#include "xnLogger.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef K::Point_2                    Point;
typedef CGAL::Polygon_2<K>            Polygon2;
typedef CGAL::Polygon_with_holes_2<K> Polygon_with_holes;
typedef CGAL::Straight_skeleton_2<K>  Ss;

typedef boost::shared_ptr<Ss> SsPtr;

using namespace xn;

DEFINE_STANDARD_EXPORTS
DEFINE_DLLMAIN

vec2 ToDgVec(Point const &p)
{
  return vec2((float)p.x(), (float)p.y());
}

Module *xnPlugin_CreateModule(xn::ModuleInitData *pData)
{
  return new StraightSkeleton(pData);
}

char const *xnPlugin_GetModuleName()
{
  return "Straight Skeleton";
}

StraightSkeleton::StraightSkeleton(xn::ModuleInitData *pData)
  : Module(pData)
  , m_segments()
  , m_vertCount(0)
  , m_edgeCount(0)
  , m_faceCount(0)
  , m_showBoundaryConnections(false)
  , m_edgeProperties(0xFFFF00FF, 2.f)
{

}

void StraightSkeleton::Clear()
{
  m_segments.segments.clear();
  m_boundaryConnections.segments.clear();
  m_vertCount = 0;
  m_edgeCount = 0;
  m_faceCount = 0;
}

bool StraightSkeleton::SetGeometry(PolygonGroup const &geometry)
{
  Clear();
  if (geometry.polygons.size() == 0)
    return true;

  Polygon2 outer;
  for (auto it = geometry.polygons[0].cPointsBegin(); it != geometry.polygons[0].cPointsEnd(); it++)
    outer.push_back(Point(it->x(), it->y()));
  
  if (!outer.is_counterclockwise_oriented())
  {
    M_LOG_ERROR("Failed to create straight skeleton: output polygon is not CCW");
    return false;
  }

  Polygon_with_holes poly(outer);

  for (size_t i = 1; i < geometry.polygons.size(); i++)
  {
    Polygon2 hole;
    for (auto it = geometry.polygons[i].cPointsBegin(); it != geometry.polygons[i].cPointsEnd(); it++)
      hole.push_back(Point(it->x(), it->y()));

    if (!hole.is_clockwise_oriented())
    {
      M_LOG_ERROR("Failed to create straight skeleton: hole polygon is not CW");
      return false;
    }

    poly.add_hole(hole);
  }

  SsPtr iss = CGAL::create_interior_straight_skeleton_2(poly);

  if (iss == nullptr)
  {
    M_LOG_ERROR("Failed to create straight skeleton");
    return false;
  }

  typedef typename Ss::Vertex_const_handle     Vertex_const_handle;
  typedef typename Ss::Halfedge_const_handle   Halfedge_const_handle;
  typedef typename Ss::Halfedge_const_iterator Halfedge_const_iterator;

  Halfedge_const_handle null_halfedge;
  Vertex_const_handle   null_vertex;

  m_vertCount = iss->size_of_vertices();
  m_edgeCount = iss->size_of_halfedges();
  m_faceCount = iss->size_of_faces();

  for (Halfedge_const_iterator it = iss->halfedges_begin(); it != iss->halfedges_end(); ++it)
  {
    if (!it->is_bisector())
      continue;

    vec2 p0 = ToDgVec(it->opposite()->vertex()->point());
    vec2 p1 = ToDgVec(it->vertex()->point());

    seg s(p0, p1);
    bool intersectsBoundary = false;

    for (auto const p : geometry.polygons)
    {
      if (IntersectsBoundary(p, s) == Dg::QueryCode::Intersecting)
      {
        intersectsBoundary = true;
        break;
      }
    }
    if (intersectsBoundary)
      m_boundaryConnections.segments.push_back(seg(p0, p1));
    else
      m_segments.segments.push_back(seg(p0, p1));
  }

  return true;
}

void StraightSkeleton::DoFrame(UIContext *pContext)
{
  pContext->BeginWindow(xnPlugin_GetModuleName(), m_pShow);

  if (pContext->Button("What is this?##StraightSkeleton"))
    pContext->OpenPopup("Description##StraightSkeleton");
  if (pContext->BeginPopup("Description##StraightSkeleton"))
  {
    float wrap_width = 400.f;
    pContext->PushTextWrapPos(pContext->GetCursorPos().x() + wrap_width);
    pContext->Text("A straight skeleton is a method of representing a polygon by a topological skeleton. It is similar in some ways to the medial axis but differs in that the skeleton is composed of straight line segments, while the medial axis of a polygon may involve parabolic curves. However, both are homotopy-equivalent to the underlying polygon.", wrap_width);
    pContext->PopTextWrapPos();
    pContext->EndPopup();
  }
  pContext->Text("Vertices: %u", m_vertCount);
  pContext->Text("Edges: %u", m_edgeCount);
  pContext->Text("Faces: %u", m_faceCount);
  pContext->Checkbox("Show boundary connections##StraightSkeleton", &m_showBoundaryConnections);

  pContext->EndWindow();
}

void StraightSkeleton::Render(Renderer *pRenderer, mat33 const &T_World_View)
{
  SegmentCollection transformed = m_segments.GetTransformed(T_World_View);
  for (auto const &edge : transformed.segments)
    pRenderer->DrawLine(edge, m_edgeProperties);

  if (m_showBoundaryConnections)
  {
    transformed = m_boundaryConnections.GetTransformed(T_World_View);
    for (auto const &edge : transformed.segments)
      pRenderer->DrawLine(edge, m_edgeProperties);
  }
}