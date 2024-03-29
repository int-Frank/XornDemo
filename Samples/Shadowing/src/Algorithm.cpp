
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
typedef uint32_t VertexID;

class ID
{
  static const VertexID s_InvalidID;
public:

  ID()
    : m_id0(s_InvalidID)
    , m_id1(s_InvalidID)
  { }

  ID(VertexID id)
    : m_id0(id)
    , m_id1(s_InvalidID)
  { }

  ID(VertexID id0, VertexID id1)
    : m_id0(id0)
    , m_id1(id1)
  {
    // Standardise the way we store vertex ids for edges; lowest id first.
    if (id0 > id1)
    {
      m_id0 = id1;
      m_id1 = id0;
    }
  }

  bool operator==(ID const &other) const
  {
    return (m_id0 == other.m_id0) && (m_id1 == other.m_id1);
  }

  void SetVertex(VertexID id)
  {
    m_id0 = id;
    m_id1 = s_InvalidID;
  }

  // Vertex
  VertexID GetFirst() const { return m_id0; }

  // Second vertex that makes the edge, if this is an edge ID.
  VertexID GetSecond() const { return m_id1; }
  
  bool IsValid() const { return (m_id0 != s_InvalidID) || (m_id1 != s_InvalidID); }
  bool IsVertexID() const { return m_id1 == s_InvalidID; }
  bool IsEdgeID() const { return m_id1 != s_InvalidID; }

private:

  VertexID m_id0;
  VertexID m_id1;
};

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
    ID id;
    vec2 point;
    float distanceSq;
  };

  struct VisibilityRay
  {
    VertexID sourceID; // The ray source will always be a single vertex.
    ID backID;         // The ray can either end at another vertex, or at along an edge.
    vec2 backPoint;    // As the back point can be an edge intersect, we also store it here.
  };

  struct Vertex
  {
    uint32_t prevVertex;
    uint32_t nextVertex;
    vec2 point;
  };

public:

  PIMPL();
  ~PIMPL();

  void SetRegion(std::vector<xn::PolygonLoop> const &loops);
  bool TryBuildVisibilityPolygon(vec2 const &source, DgPolygon *pOut);

private:

  void FindAllVertsOnRay(ray2 const &, VertexID startIndex, float epsilon);
  void ClipRayAgainstBoundary(ray2 const &);
  bool RayVertsContain(ID id) const;
  bool IsConnected(VertexID, VisibilityRay const &) const;
  bool GetClosestIntersect(ray2 const &ray, vec2 *pPoint, ID *edgeID) const;
  Side GetSide(ID id, ray2 const &ray) const;
  bool TurnRaysIntoPolygon(xn::DgPolygon *pOut) const;

private:

  std::vector<Vertex> m_regionVerts;
  Dg::Map_AVL<float, VisibilityRay> m_rays;
  std::vector<bool> m_processedFlags;
  RayVertex *m_pRayVerts;
  uint32_t m_rayVertsSize;
};

VertexID const ID::s_InvalidID = 0xFFFFFFFF;

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

void VisibilityBuilder::PIMPL::SetRegion(std::vector<xn::PolygonLoop> const &loops)
{
  m_regionVerts.clear();
  for (auto poly_it = loops.cbegin(); poly_it != loops.cend(); poly_it++)
  {
    uint32_t size = (uint32_t)poly_it->Size();
    uint32_t currentBase = (uint32_t)m_regionVerts.size();
    VertexID localID = 0;
    for (auto vert_it = poly_it->cPointsBegin(); vert_it != poly_it->cPointsEnd(); vert_it++)
    {
      VertexID localNextID = (localID + 1) % size;
      VertexID localPrevID = (localID + size - 1) % size;

      Vertex v;
      v.point = *vert_it;
      v.nextVertex = currentBase + localNextID;
      v.prevVertex = currentBase + localPrevID;

      m_regionVerts.push_back(v);

      localID++;
    }
  }

  delete[] m_pRayVerts;
  m_pRayVerts = new RayVertex[m_regionVerts.size()]{};

  m_processedFlags.resize(m_regionVerts.size());
}

bool VisibilityBuilder::PIMPL::TryBuildVisibilityPolygon(vec2 const &source, DgPolygon *pOut)
{
  float epsilon = Dg::Constants<float>::EPSILON;

  pOut->Clear();
  m_rays.clear();

  std::fill(m_processedFlags.begin(), m_processedFlags.end(), false);

  for (VertexID vertIndex = 0; vertIndex < m_regionVerts.size(); vertIndex++)
  {
    if (m_processedFlags[vertIndex])
      continue;
    m_processedFlags[vertIndex] = true;

    auto &vert = m_regionVerts[vertIndex];

    // Build the source ray
    vec2 v = vert.point - source;
    float lenSq = Dg::MagSq(v);
    if (Dg::IsZero(lenSq))
      continue;

    v /= sqrt(lenSq);
    ray2 ray(source, v);

    // We are going to build up a list of vertices which lie on this ray.
    // From here, we will sort them by distance to the source, and then try to
    // work out where this ray starts and finishes.

    // Add the first vertex to the vertex list.
    m_pRayVerts[0].id = ID(vertIndex);
    m_pRayVerts[0].point = vert.point;
    m_pRayVerts[0].distanceSq = lenSq;
    m_rayVertsSize = 1;

    FindAllVertsOnRay(ray, vertIndex + 1, epsilon);

    // Sort the ray-vertex list based on distance to the source.
    std::sort(m_pRayVerts, m_pRayVerts + m_rayVertsSize,
      [](RayVertex const &a, RayVertex const &b) {return a.distanceSq < b.distanceSq; });

    // Next, find the closest boundary intersection with the ray.
    // If none in found, this means the last vertex in the list is the
    // furtherest point.
    ClipRayAgainstBoundary(ray);

    // No vertex can be seen.
    if (m_rayVertsSize == 0)
      continue;

    // Check if any of the edges connected to the ray-verts are going to cut off
    // the line of sight.
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

    VisibilityRay r;
    r.sourceID = m_pRayVerts[0].id.GetFirst();

    // Emit furtherest vertex if we have more than one visible vertex on the ray.
    if (m_rayVertsSize > 1)
    {
      r.backID = m_pRayVerts[m_rayVertsSize - 1].id;
      r.backPoint = m_pRayVerts[m_rayVertsSize - 1].point;
    }

    // We now have the near and far point of the ray!
    float angle = atan2(v.y(), v.x());
    m_rays[angle] = r;
  }

  return TurnRaysIntoPolygon(pOut);
}

void VisibilityBuilder::PIMPL::FindAllVertsOnRay(ray2 const &ray, VertexID startVertex, float epsilon)
{
  for (VertexID vertIndex = startVertex; vertIndex < m_regionVerts.size(); vertIndex++)
  {
    if (m_processedFlags[vertIndex])
      continue;

    auto &vert = m_regionVerts[vertIndex];

    Dg::CP2PointRay<float> query;
    auto result = query(vert.point, ray);

    if (result.u == 0.f)
      continue;

    float d = Dg::MagSq(result.cp - vert.point);
    if (d > epsilon)
      continue;

    m_processedFlags[vertIndex] = true;
    float lenSq = Dg::MagSq(vert.point - ray.Origin());

    m_pRayVerts[m_rayVertsSize].id.SetVertex(vertIndex);
    m_pRayVerts[m_rayVertsSize].point = vert.point;
    m_pRayVerts[m_rayVertsSize].distanceSq = lenSq;
    m_rayVertsSize++;
  }
}

void VisibilityBuilder::PIMPL::ClipRayAgainstBoundary(ray2 const &ray)
{
  vec2 endPoint = {};
  ID edgeID;

  // If we find a valid back point, cull all temp points beyond this point.
  if (GetClosestIntersect(ray, &endPoint, &edgeID))
  {
    float backDistSq = Dg::MagSq(endPoint - ray.Origin());
    for (uint32_t i = 0; i < m_rayVertsSize; i++)
    {
      if (backDistSq < m_pRayVerts[i].distanceSq)
      {
        m_rayVertsSize = i;
        break;
      }
    }

    // Add in the back point if we haven't clipped the entire ray.
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
  for (VertexID vertIndex = 0; vertIndex < m_regionVerts.size(); vertIndex++)
  {
    auto &vert = m_regionVerts[vertIndex];
    if (RayVertsContain(vertIndex) || RayVertsContain(vert.nextVertex))
      continue;

    vec2 p1 = m_regionVerts[vert.nextVertex].point;
    seg s(vert.point, p1);

    Dg::FI2SegmentRay<float> query;
    auto result = query(s, ray);

    if (result.code != Dg::QueryCode::Intersecting)
      continue;

    if (result.pointResult.ur < ur)
    {
      ur = result.pointResult.ur;
      *pEdgeID = ID(vertIndex, vert.nextVertex);
      *pPoint = result.pointResult.point;
    }
  }
  return ur != FLT_MAX;
}

VisibilityBuilder::PIMPL::Side VisibilityBuilder::PIMPL::GetSide(ID id, ray2 const &ray) const
{
  // We have two IDs; id is an edge intersection.
  if (id.IsEdgeID())
    return SideBoth;

  float epsilon = Dg::Constants<float>::EPSILON;
  auto &vert = m_regionVerts[id.GetFirst()];
  vec2 next = m_regionVerts[vert.nextVertex].point - ray.Origin();
  vec2 prev = m_regionVerts[vert.prevVertex].point - ray.Origin();

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

bool VisibilityBuilder::PIMPL::IsConnected(VertexID id, VisibilityRay const &ray) const
{
  // Three cases:
  //   - ray is a single vertex
  //   - ray is a vertex -> edge ray
  //   - ray is a vertex -> vertex ray

  VertexID a = m_regionVerts[ray.sourceID].nextVertex;
  VertexID b = m_regionVerts[ray.sourceID].prevVertex;
  bool connected = false;

  connected = connected || a == id;
  connected = connected || b == id;

  if (ray.backID.IsValid())
  {
    if (ray.backID.IsEdgeID())
    {
      a = ray.backID.GetFirst();
      b = ray.backID.GetSecond();
    }
    else
    {
      auto &const vert = m_regionVerts[ray.backID.GetFirst()];
      a = vert.nextVertex;
      b = vert.prevVertex;
    }
    connected = connected || a == id;
    connected = connected || b == id;
  }

  return connected;
}

bool VisibilityBuilder::PIMPL::TurnRaysIntoPolygon(xn::DgPolygon *pOut) const
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
    
    vec2 source = m_regionVerts[it->second.sourceID].point;
    if (!it->second.backID.IsValid())
    {
      pOut->PushBack(source);
    }
    else if (IsConnected(it->second.sourceID, itPrev->second))
    {
      pOut->PushBack(source);
      pOut->PushBack(it->second.backPoint);
    }
    else
    {
      pOut->PushBack(it->second.backPoint);
      pOut->PushBack(source);
    }
  }
  return true;
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

void VisibilityBuilder::SetRegion(std::vector<xn::PolygonLoop> const &loops)
{
  m_pimpl->SetRegion(loops);
}

bool VisibilityBuilder::TryBuildVisibilityPolygon(xn::vec2 const &source, xn::DgPolygon *pOut)
{
  return m_pimpl->TryBuildVisibilityPolygon(source, pOut);
}