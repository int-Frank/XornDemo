
#include <algorithm>
#include <windows.h>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Delaunay_mesher_2.h>
#include <CGAL/Delaunay_mesh_face_base_2.h>
#include <CGAL/Delaunay_mesh_vertex_base_2.h>
#include <CGAL/Delaunay_mesh_size_criteria_2.h>
#include <CGAL/lloyd_optimize_mesh_2.h>

#include "Triangulation.h"
#include "a2dGeometricQueries.h"
#include "a2dPlugin_API.h"
#include "a2dVersion.h"
#include "a2dLogger.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Delaunay_mesh_vertex_base_2<K>                Vb;
typedef CGAL::Delaunay_mesh_face_base_2<K>                  Fb;
typedef CGAL::Triangulation_data_structure_2<Vb, Fb>        Tds;
typedef CGAL::Constrained_Delaunay_triangulation_2<K, Tds>  CDT;
typedef CGAL::Delaunay_mesh_size_criteria_2<CDT>            Criteria;
typedef CGAL::Delaunay_mesher_2<CDT, Criteria>              Mesher;
typedef CDT::Vertex_handle Vertex_handle;
typedef CDT::Point Point;

using namespace a2d;

DEFINE_STANDARD_EXPORTS
DEFINE_DLLMAIN

Dg::ErrorCode ConvexPartition(DgPolygon const &, Geometry *pOut);

Module * a2dPlugin_CreateModule(bool *pShow, Logger *pLogger)
{
  return new Triangulation(pShow, pLogger);
}

char const *a2dPlugin_GetModuleName()
{
  return "Triangulation";
}

template<typename T>
static vec2 ToDgVec(T const &p)
{
  return vec2((float)p.x(), (float)p.y());
}

static std::vector<Point> GenerateSeeds(Geometry const &geom)
{
  std::vector<Point> result;

  for (size_t i = 1; i < geom.polygons.size(); i++)
  {
    ::a2d::Polygon const &poly = geom.polygons[i];
    Geometry partition;
    ConvexPartition(poly, &partition);
    if (partition.polygons.size() == 0)
      continue;

    ::a2d::Polygon const &subPoly = partition.polygons[0];
    vec2 centroid(0.f, 0.f);
    for (auto it = subPoly.cPointsBegin(); it != subPoly.cPointsEnd(); it++)
      centroid += *it;
    centroid /= (float)subPoly.Size();
    result.push_back(Point(centroid.x(), centroid.y()));
  }

  return result;
}

Triangulation::UniqueEdge::UniqueEdge(vec2 const &a, vec2 const &b)
{
  if (VecLess(a, b))
  {
    p0 = a;
    p1 = b;
  }
  else
  {
    p1 = a;
    p0 = b;
  }
}

bool Triangulation::UniqueEdge::VecLess(vec2 const &a, vec2 const &b)
{
  if (a.x() < b.x()) return true;
  if (a.x() > b.x()) return false;
  if (a.y() < b.y()) return true;
  if (a.y() > b.y()) return false;
  return false;
}

bool Triangulation::UniqueEdge::operator<(UniqueEdge const &a) const
{
  if (p0.x() < a.p0.x()) return true;
  if (p0.x() > a.p0.x()) return false;
  if (p0.y() < a.p0.y()) return true;
  if (p0.y() > a.p0.y()) return false;

  if (p1.x() < a.p1.x()) return true;
  if (p1.x() > a.p1.x()) return false;
  if (p1.y() < a.p1.y()) return true;
  if (p1.y() > a.p1.y()) return false;

  return false;
}

Triangulation::Triangulation(bool *pShow, Logger *pLogger)
  : Module(pShow, pLogger)
  , m_edgeSet()
  , m_vertCount(0)
  , m_faceCount(0)
  , m_sizeCriteriaBounds(1.f, 1.f)
  , m_sizeCriteria(1.f)
  , m_shapeCriteria(0.125f)
  , m_LloydIterations(0)
  , m_edgeProperties(0xFFFF00FF, 1.f)
{

}

void Triangulation::Clear()
{
  m_edgeSet.clear();
  m_vertCount = 0;
  m_faceCount = 0;
}

void Triangulation::SetValueBounds()
{
  float const divisorMin = 20.f;
  float const divisorMax = 5.f;

  vec2 minBounds(FLT_MAX, FLT_MAX);
  vec2 maxBounds(-FLT_MAX, -FLT_MAX);

  for (auto const &poly : m_geometry.polygons)
  {
    for (auto it = poly.cPointsBegin(); it != poly.cPointsEnd(); it++)
    {
      vec2 p = *it;

      for (int a = 0; a < 2; a++)
      {
        if (p[a] < minBounds[a]) minBounds[a] = p[a];
        if (p[a] > maxBounds[a]) maxBounds[a] = p[a];
      }
    }
  }

  vec2 range = maxBounds - minBounds;
  float maxDimension = max(range.x(), range.y());
  m_sizeCriteriaBounds.x() = maxDimension / divisorMin;
  m_sizeCriteriaBounds.y() = maxDimension / divisorMax;

  if (m_sizeCriteria < m_sizeCriteriaBounds.x() || m_sizeCriteria > m_sizeCriteriaBounds.y())
    m_sizeCriteria = (m_sizeCriteriaBounds.x() + m_sizeCriteriaBounds.y()) / 2.f;
}

bool Triangulation::SetGeometry(Geometry const &geometry)
{
  m_geometry = geometry;
  return Update();
}

bool Triangulation::Update()
{
  Clear();

  SetValueBounds();

  std::vector<Point> seeds = GenerateSeeds(m_geometry);

  CDT cdt;

  for (auto const &poly : m_geometry.polygons)
  {
    std::vector<Vertex_handle> vertices;
    for (auto it = poly.cPointsBegin(); it != poly.cPointsEnd(); it++)
    {
      vec2 p = *it;
      Point a(p.x(), p.y());
      vertices.push_back(cdt.insert(a));
    }

    for (size_t a = 0; a < vertices.size(); a++)
    {
      size_t b = (a + 1) % vertices.size();
      cdt.insert(vertices[a], vertices[b]);
    }
  }

  Mesher mesher(cdt);
  mesher.set_criteria(Criteria(m_shapeCriteria, m_sizeCriteria));
  mesher.set_seeds(seeds.begin(), seeds.end());
  mesher.refine_mesh();

  if (m_LloydIterations > 0)
    CGAL::lloyd_optimize_mesh_2(cdt, CGAL::parameters::max_iteration_number = m_LloydIterations);

  m_vertCount = cdt.number_of_vertices();
  m_faceCount = cdt.number_of_faces();

  for (auto it = cdt.faces_begin(); it != cdt.faces_end(); it++)
  {
    if (!it->is_in_domain())
      continue;

    for (int a = 0; a < 3; a++)
    {
      int b = (a + 1) % 3;
      vec2 p0 = ToDgVec(it->vertex(a)->point());
      vec2 p1 = ToDgVec(it->vertex(b)->point());
      m_edgeSet.insert(UniqueEdge(p0, p1));
    }
  }

  return true;
}

void Triangulation::DoFrame(a2d::UIContext *pContext)
{
  pContext->BeginWindow(a2dPlugin_GetModuleName(), m_pShow);

  if (pContext->Button("What is this?##Triangulation"))
    pContext->OpenPopup("Description##Triangulation");
  if (pContext->BeginPopup("Description##Triangulation"))
  {
    float wrap_width = 400.f;
    pContext->PushTextWrapPos(pContext->GetCursorPos().x() + wrap_width);
    pContext->Text("This module demonstrates Shewchuk's algorithm to construct conforming triangulations and 2D meshes.", wrap_width);
    pContext->PopTextWrapPos();
    pContext->EndPopup();
  }
  pContext->Text("Vertices: %u", m_vertCount);
  pContext->Text("Faces: %u", m_faceCount);
  pContext->Separator();
  if (pContext->SliderFloat("Triangle size", &m_sizeCriteria, m_sizeCriteriaBounds.x(), m_sizeCriteriaBounds.y()))
    Update();

  if (pContext->SliderFloat("Triangle shape", &m_shapeCriteria, 0.01f, 0.3f))
    Update();

  if (pContext->SliderInt("Lloyd iterations", &m_LloydIterations, 0, 50))
    Update();

  pContext->EndWindow();
}

void Triangulation::Render(Renderer *pRenderer, mat33 const &T_World_View)
{
  for (auto it = m_edgeSet.cbegin_rand(); it != m_edgeSet.cend_rand(); it++)
  {
    vec3 p0(it->p0.x(), it->p0.y(), 1.f);
    vec3 p1(it->p1.x(), it->p1.y(), 1.f);
    p0 = p0 * T_World_View;
    p1 = p1 * T_World_View;
    pRenderer->DrawLine(seg(vec2(p0.x(), p0.y()), vec2(p1.x(), p1.y())), m_edgeProperties);
  }
}