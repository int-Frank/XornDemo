
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
  return (a << 32) | (b & 0xFFFFFFFFull);
}

static void GetVertexIDs(ID edge, ID *pa, ID *pb)
{
  *pa = edge >> 32;
  *pb = edge & 0xFFFFFFFFull;
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

  struct TempData
  {
    vec2 point;
    ID id;
    float distanceSq;
  };

  struct Ray
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


  bool TempListContains(ID id);
  bool IsConnected(ID, Ray const &);
  bool GetClosestIntersect(ray2 const &ray, vec2 *pPoint, ID *edgeID);
  Side GetSide(ID id, ray2 const &ray);
  bool TurnRaysIntoPolygon(xn::vec2 const &source, xn::DgPolygon *pOut);

private:

  static ID const s_InvalidID;
  static ID const s_FirstValidID;

  Dg::Map_AVL<ID, Vertex> m_regionVerts;
  Dg::Map_AVL<float, Ray> m_rays;
  TempData *m_pTempData;
  uint32_t m_tempDataSize;
};

ID const VisibilityBuilder::PIMPL::s_InvalidID = 0;
ID const VisibilityBuilder::PIMPL::s_FirstValidID = 1;

//----------------------------------------------------------------
// VisibilityBuilder::PIMPL
//----------------------------------------------------------------

VisibilityBuilder::PIMPL::PIMPL()
  : m_pTempData(nullptr)
  , m_tempDataSize(0)
{

}

VisibilityBuilder::PIMPL::~PIMPL()
{
  delete[] m_pTempData;
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

  delete[] m_pTempData;
  m_pTempData = new TempData[m_regionVerts.size()]{};
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
    // If already processed, continue
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
    m_pTempData[0].id = it->first;
    m_pTempData[0].point = it->second.point;
    m_pTempData[0].distanceSq = lenSq;
    m_tempDataSize = 1;

    // Find all other vertices that are on the ray
    for (auto it2 = it; it2 != m_regionVerts.end_rand(); it2++)
    {
      if (it2->second.processed)
        continue;

      Dg::CP2PointRay<float> query;
      auto result = query(it2->second.point, ray);

      if (result.u == 0.f)
        continue;

      float d = Dg::MagSq(result.cp - it2->second.point);
      if (d > epsilon)
        continue;

      it2->second.processed = true;
      vec2 v2 = it2->second.point - source;
      float lenSq2 = Dg::MagSq(v2);
      if (Dg::IsZero(lenSq2))
        continue;

      m_pTempData[m_tempDataSize].id = it2->first;
      m_pTempData[m_tempDataSize].point = it2->second.point;
      m_pTempData[m_tempDataSize].distanceSq = lenSq2;
      m_tempDataSize++;
    }

    // Sort the ray-vertex list based on distance to the source.
    std::sort(m_pTempData, m_pTempData + m_tempDataSize,
      [](TempData const &a, TempData const &b) {return a.distanceSq < b.distanceSq; });

    // Next, find the closest boundary intersection with the ray.
    // If none in found, this means an already processed vertex is the
    // boundary.

    {
      vec2 endPoint = {};
      ID edgeID = s_InvalidID;

      // If we find a valid back point, cull all temp points beyond this point.
      if (GetClosestIntersect(ray, &endPoint, &edgeID))
      {
        float backDistSq = Dg::MagSq(endPoint - source);
        uint32_t oldTempSize = m_tempDataSize;
        for (uint32_t i = 0; i < m_tempDataSize; i++)
        {
          if (backDistSq < m_pTempData[i].distanceSq)
          {
            m_tempDataSize = i;
            break;
          }
        }

        // Add in the back point we just found if we still have verts on this ray.
        if (m_tempDataSize > 0)
        {
          m_pTempData[m_tempDataSize].point = endPoint;
          m_pTempData[m_tempDataSize].id = edgeID;
          m_pTempData[m_tempDataSize].distanceSq = backDistSq;
          m_tempDataSize++;
        }
      }
    }

    // No vertex can be seen.
    if (m_tempDataSize == 0)
      continue;

    // Now check all edges from all ray-verts are on the same side. 
    // Otherwise they are going to cut off the line of sight.
    uint32_t side = SideNone;
    for (uint32_t i = 0; i < m_tempDataSize; i++)
    {
      ID id = m_pTempData[i].id;
      side = side | GetSide(id, ray);

      if (side == SideBoth)
      {
        m_tempDataSize = i + 1;
        break;
      }
    }

    Ray r = {};
    r.point0 = m_pTempData[0].point;
    r.ID0 = m_pTempData[0].id;
    r.ID1 = s_InvalidID;

    // Emit furtherest vertex if we have more than one visible vertex on the ray.
    if (m_tempDataSize > 1)
    {
      r.ID1 = m_pTempData[m_tempDataSize - 1].id;
      r.point1 = m_pTempData[m_tempDataSize - 1].point;
    }

    // We now have the start and end point!
    float angle = atan2(v.y(), v.x());
    m_rays[angle] = r;
  }

  return TurnRaysIntoPolygon(source, pOut);
}

bool VisibilityBuilder::PIMPL::TempListContains(ID id)
{
  for (uint32_t i = 0; i < m_tempDataSize; i++)
  {
    if (m_pTempData[i].id == id)
      return true;
  }
  return false;
}

bool VisibilityBuilder::PIMPL::GetClosestIntersect(ray2 const &ray, vec2 *pPoint, ID *pEdgeID)
{
  float ur = FLT_MAX;
  for (auto it = m_regionVerts.begin_rand(); it != m_regionVerts.end_rand(); it++)
  {
    if (TempListContains(it->first) || TempListContains(it->second.nextID))
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

VisibilityBuilder::PIMPL::Side VisibilityBuilder::PIMPL::GetSide(ID id, ray2 const &ray)
{
  // We have two IDs; id is an edge intersection.
  if (id > 0xFFFFFFFFull)
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

bool VisibilityBuilder::PIMPL::IsConnected(ID id, Ray const &node)
{
  ID a, b, c, d;
  c = m_regionVerts.at(node.ID0).nextID;
  d = m_regionVerts.at(node.ID0).prevID;
  GetVertexIDs(node.ID1, &a, &b);

  bool connected = false;
  connected = connected || (a != s_InvalidID) && a == id;
  connected = connected || (b != s_InvalidID) && b == id;
  connected = connected || c == id;
  connected = connected || d == id;
  return connected;
}

bool VisibilityBuilder::PIMPL::TurnRaysIntoPolygon(xn::vec2 const &source, xn::DgPolygon *pOut)
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