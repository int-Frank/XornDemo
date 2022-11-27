
#include <stdint.h>
#include <cmath>
#include <algorithm>

#include "Algorithm.h"

#include "DgMap_AVL.h"
#include "DgQueryPointRay.h"
#include "DgQuerySegmentRay.h"

#include "xnGeometry.h"
#include "DgRay.h"

using namespace xn;

typedef Dg::Ray2<float> ray2;
typedef uint64_t ID;

static ID GetEdgeID(ID a, ID b)
{
  if (a > b)
  {
    ID temp = a;
    a = b;
    b = temp;
  }

  return (a << 32) | (b & 0xFFFFFFFFull);
}

static void GetVertexIDs(ID edge, ID *pa, ID *pb)
{
  *pa = edge >> 32;
  *pb = edge & 0xFFFFFFFFull;
}

static bool IsEdgeID(ID id)
{
  return id > 0xFFFFFFFFull;
}

class VisibilityBuilder::PIMPL
{
  enum Side : uint32_t
  {
    SideNone = 0,
    SideLeft = 1,
    SideRight = 2,
    SideBoth = SideLeft | SideRight
  };

  struct RayVertex
  {
    vec2 point;
    ID id;
    float distanceSq;
  };

  struct VisibilityRay
  {
    vec2 point0;
    vec2 point1;
    ID ID0;
    ID ID1;
  };

  struct Vertex
  {
    ID prevID;
    ID nextID;
    vec2 point;
    bool processed;
  };

public:

  PIMPL();
  ~PIMPL();

  void SetRegion(PolygonWithHoles const &polygon);
  bool TryBuildVisibilityPolygon(vec2 const &source, DgPolygon *pOut);

private:

  void FindAllVertsOnRay(ray2 const &, Dg::Map_AVL<ID, Vertex>::iterator_rand seed, float epsilon);
  void ClipRayVertsAgainstBoundary(ray2 const &);
  bool RayVertsContain(ID id) const;
  bool IsConnected(ID, VisibilityRay const &) const;
  bool GetClosestIntersect(ray2 const &ray, vec2 *pPoint, ID *edgeID) const;
  Side GetSide(ID id, ray2 const &ray) const;
  bool TurnRaysIntoPolygon(xn::vec2 const &source, xn::DgPolygon *pOut) const;

private:

  static ID const s_InvalidID;
  static ID const s_FirstValidID;

  Dg::Map_AVL<ID, Vertex> m_regionVerts;
  Dg::Map_AVL<float, VisibilityRay> m_rays;
  RayVertex *m_pRayVerts;
  uint32_t m_rayVertsSize;
};

ID const VisibilityBuilder::PIMPL::s_InvalidID = 0;
ID const VisibilityBuilder::PIMPL::s_FirstValidID = 1;

//----------------------------------------------------------------
// VisibilityBuilder::PIMPL
//----------------------------------------------------------------

VisibilityBuilder::PIMPL::PIMPL()
  : m_pRayVerts(nullptr)
  , m_rayVertsSize(0)
{

}

VisibilityBuilder::PIMPL::~PIMPL()
{
  delete[] m_pRayVerts;
}

void VisibilityBuilder::PIMPL::SetRegion(PolygonWithHoles const &polygon)
{
  m_regionVerts.clear();
  ID id = s_FirstValidID;
  for (auto poly_it = polygon.loops.cbegin(); poly_it != polygon.loops.cend(); poly_it++)
  {
    uint32_t size = (uint32_t)poly_it->Size();
    ID localID = 0;
    for (auto vert_it = poly_it->cPointsBegin(); vert_it != poly_it->cPointsEnd(); vert_it++)
    {
      ID localNextID = (localID + 1) % size;
      ID localPrevID = (localID + size - 1) % size;

      Vertex v;
      v.point = *vert_it;
      v.nextID = id + localNextID;
      v.prevID = id + localPrevID;
      v.processed = false;

      m_regionVerts.insert(Dg::Pair<ID, Vertex>(id + localID, v));

      localID++;
    }
    id += size;
  }

  delete[] m_pRayVerts;
  m_pRayVerts = new RayVertex[m_regionVerts.size()]{};
}

bool VisibilityBuilder::PIMPL::TryBuildVisibilityPolygon(vec2 const &source, DgPolygon *pOut)
{
  float epsilon = Dg::Constants<float>::EPSILON;

  pOut->Clear();
  m_rays.clear();

  for (auto it = m_regionVerts.begin_rand(); it != m_regionVerts.end_rand(); it++)
    it->second.processed = false;

  for (auto it = m_regionVerts.begin_rand(); it != m_regionVerts.end_rand(); it++)
  {
    if (it->second.processed)
      continue;
    it->second.processed = true;

    // Build the source ray
    vec2 v = it->second.point - source;
    float lenSq = Dg::MagSq(v);
    if (Dg::IsZero(lenSq))
      continue;

    v /= sqrt(lenSq);
    ray2 ray(source, v);

    // We are going to build up a list of vertices which lie on this ray.
    // From here, we will sort them by distance to the source, and then try to
    // work out where this ray starts and finishes.

    // Add the first vertex to the vertex list.
    m_pRayVerts[0].id = it->first;
    m_pRayVerts[0].point = it->second.point;
    m_pRayVerts[0].distanceSq = lenSq;
    m_rayVertsSize = 1;

    auto seed = it;
    seed++;
    FindAllVertsOnRay(ray, seed, epsilon);

    // Sort the ray-vertex list based on distance to the source.
    std::sort(m_pRayVerts, m_pRayVerts + m_rayVertsSize,
      [](RayVertex const &a, RayVertex const &b) {return a.distanceSq < b.distanceSq; });

    // Next, find the closest boundary intersection with the ray.
    // If none in found, this means the last vertex in the list is the
    // furtherest point.
    ClipRayVertsAgainstBoundary(ray);

    // No vertex can be seen.
    if (m_rayVertsSize == 0)
      continue;

    // Now check all edges from all ray-verts are on the same side. 
    // Otherwise they are going to cut off the line of sight.
    uint32_t side = SideNone;
    for (uint32_t i = 0; i < m_rayVertsSize; i++)
    {
      ID id = m_pRayVerts[i].id;
      side = side | GetSide(id, ray);

      if (side == SideBoth)
      {
        m_rayVertsSize = i + 1;
        break;
      }
    }

    VisibilityRay r = {};
    r.point0 = m_pRayVerts[0].point;
    r.ID0 = m_pRayVerts[0].id;
    r.ID1 = s_InvalidID;

    // Emit furtherest vertex if we have more than one visible vertex on the ray.
    if (m_rayVertsSize > 1)
    {
      r.ID1 = m_pRayVerts[m_rayVertsSize - 1].id;
      r.point1 = m_pRayVerts[m_rayVertsSize - 1].point;
    }

    // We now have the near and far point of the ray!
    float angle = atan2(v.y(), v.x());
    m_rays[angle] = r;
  }

  return TurnRaysIntoPolygon(source, pOut);
}

void VisibilityBuilder::PIMPL::FindAllVertsOnRay(ray2 const &ray, Dg::Map_AVL<ID, Vertex>::iterator_rand seed, float epsilon)
{
  for (auto it = seed; it != m_regionVerts.end_rand(); it++)
  {
    if (it->second.processed)
      continue;

    Dg::CP2PointRay<float> query;
    auto result = query(it->second.point, ray);

    if (result.u == 0.f)
      continue;

    float d = Dg::MagSq(result.cp - it->second.point);
    if (d > epsilon)
      continue;

    it->second.processed = true;
    vec2 v2 = it->second.point - ray.Origin();
    float lenSq2 = Dg::MagSq(v2);

    m_pRayVerts[m_rayVertsSize].id = it->first;
    m_pRayVerts[m_rayVertsSize].point = it->second.point;
    m_pRayVerts[m_rayVertsSize].distanceSq = lenSq2;
    m_rayVertsSize++;
  }
}

void VisibilityBuilder::PIMPL::ClipRayVertsAgainstBoundary(ray2 const &r)
{
  vec2 endPoint = {};
  ID edgeID = s_InvalidID;

  // If we find a valid back point, cull all temp points beyond this point.
  if (GetClosestIntersect(r, &endPoint, &edgeID))
  {
    float backDistSq = Dg::MagSq(endPoint - r.Origin());
    for (uint32_t i = 0; i < m_rayVertsSize; i++)
    {
      if (backDistSq < m_pRayVerts[i].distanceSq)
      {
        m_rayVertsSize = i;
        break;
      }
    }

    // Add in the back point we just found if we still have verts on this ray.
    if (m_rayVertsSize > 0)
    {
      m_pRayVerts[m_rayVertsSize].point = endPoint;
      m_pRayVerts[m_rayVertsSize].id = edgeID;
      m_pRayVerts[m_rayVertsSize].distanceSq = backDistSq;
      m_rayVertsSize++;
    }
  }
}

bool VisibilityBuilder::PIMPL::RayVertsContain(ID id) const
{
  for (uint32_t i = 0; i < m_rayVertsSize; i++)
  {
    if (m_pRayVerts[i].id == id)
      return true;
  }
  return false;
}

bool VisibilityBuilder::PIMPL::GetClosestIntersect(ray2 const &ray, vec2 *pPoint, ID *pEdgeID) const
{
  float ur = FLT_MAX;
  for (auto it = m_regionVerts.cbegin_rand(); it != m_regionVerts.cend_rand(); it++)
  {
    if (RayVertsContain(it->first) || RayVertsContain(it->second.nextID))
      continue;

    vec2 p1 = m_regionVerts.at(it->second.nextID).point;
    seg s(it->second.point, p1);

    Dg::FI2SegmentRay<float> query;
    auto result = query(s, ray);

    if (result.code != Dg::QueryCode::Intersecting)
      continue;

    if (result.pointResult.ur < ur)
    {
      ur = result.pointResult.ur;
      *pEdgeID = GetEdgeID(it->first, it->second.nextID);
      *pPoint = result.pointResult.point;
    }
  }
  return ur != FLT_MAX;
}

VisibilityBuilder::PIMPL::Side VisibilityBuilder::PIMPL::GetSide(ID id, ray2 const &ray) const
{
  // We have two IDs; id is an edge intersection.
  if (IsEdgeID(id))
    return SideBoth;

  float epsilon = Dg::Constants<float>::EPSILON;
  vec2 next = m_regionVerts.at(m_regionVerts.at(id).nextID).point - ray.Origin();
  vec2 prev = m_regionVerts.at(m_regionVerts.at(id).prevID).point - ray.Origin();

  float edgeNext = Dg::PerpDot(ray.Direction(), next);
  float edgePrev = Dg::PerpDot(ray.Direction(), prev);

  uint32_t sideNext = SideNone;
  uint32_t sidePrev = SideNone;

  if (edgeNext > epsilon) sideNext = SideLeft;
  else if (edgeNext < -epsilon) sideNext = SideRight;
  if (edgePrev > epsilon) sidePrev = SideLeft;
  else if (edgePrev < -epsilon) sidePrev = SideRight;

  return Side(sideNext | sidePrev);
}

bool VisibilityBuilder::PIMPL::IsConnected(ID id, VisibilityRay const &ray) const
{
  // Three cases:
  //   - ray is a single vertex
  //   - ray is a vertex -> edge ray
  //   - ray is a vertex -> vertex ray

  ID a = m_regionVerts.at(ray.ID0).nextID;
  ID b = m_regionVerts.at(ray.ID0).prevID;
  bool connected = false;

  connected = connected || a == id;
  connected = connected || b == id;

  if (ray.ID1 != s_InvalidID)
  {
    if (IsEdgeID(ray.ID1))
    {
      GetVertexIDs(ray.ID1, &a, &b);
    }
    else
    {
      a = m_regionVerts.at(ray.ID1).nextID;
      b = m_regionVerts.at(ray.ID1).prevID;
    }
    connected = connected || a == id;
    connected = connected || b == id;
  }

  return connected;
}

bool VisibilityBuilder::PIMPL::TurnRaysIntoPolygon(xn::vec2 const &source, xn::DgPolygon *pOut) const
{
  if (m_rays.size() < 3)
    return false;

  auto it = m_rays.cbegin();
  for (size_t i = 0; i < m_rays.size(); i++)
  {
    auto itPrev = it;
    it++;
    if (it == m_rays.cend())
      it = m_rays.cbegin();

    if (it->second.ID1 == s_InvalidID)
    {
      pOut->PushBack(it->second.point0);
    }
    else if (IsConnected(it->second.ID0, itPrev->second))
    {
      pOut->PushBack(it->second.point0);
      pOut->PushBack(it->second.point1);
    }
    else
    {
      pOut->PushBack(it->second.point1);
      pOut->PushBack(it->second.point0);
    }
  }
}

//----------------------------------------------------------------
// VisibilityBuilder
//----------------------------------------------------------------

VisibilityBuilder::VisibilityBuilder()
  : m_pimpl(new PIMPL())
{

}

VisibilityBuilder::~VisibilityBuilder()
{
  delete m_pimpl;
}

void VisibilityBuilder::SetRegion(xn::PolygonWithHoles const &polygon)
{
  m_pimpl->SetRegion(polygon);
}

bool VisibilityBuilder::TryBuildVisibilityPolygon(xn::vec2 const &source, xn::DgPolygon *pOut)
{
  return m_pimpl->TryBuildVisibilityPolygon(source, pOut);
}