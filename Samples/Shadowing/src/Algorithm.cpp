
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
typedef uint32_t VertexID; // Polygon vertex id.

class ShadowBuilder::PIMPL
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
    VertexID id;
    float distanceSq;
  };

  struct Node
  {
    vec2 point0;
    VertexID ID0;
    vec2 point1;
    VertexID ID1; // InvalidID -> point1 does not exist, NoID -> point1 is not a vertex
  };

  struct Vertex
  {
    VertexID prevID;
    VertexID nextID;
    vec2 point;
    bool processed;
  };

public:

  void SetRegionPolygon(PolygonWithHoles const &polygon);
  bool TryBuildRayMap(vec2 const &source, DgPolygon *pOut);

private:

  static bool IgnoreListContains(VertexID id, TempData const *pIgnoreList, uint32_t sizeIgnoreList);

  bool GetClosestIntersect(ray2 const &ray, vec2 *pPoint, TempData const *pIgnoreList, uint32_t sizeIgnoreList);
  Side GetSide(VertexID id, ray2 const &ray);
  bool GetCoverage(xn::vec2 const &source, xn::DgPolygon *pOut);

private:

  static VertexID const s_InvalidID;
  static VertexID const s_EdgeID;

  Dg::Map_AVL<VertexID, Vertex> m_verts;
  Dg::Map_AVL<float, Node> m_shadowNodes;
};


VertexID const ShadowBuilder::PIMPL::s_InvalidID = 0xFFFFFFFF;
VertexID const ShadowBuilder::PIMPL::s_EdgeID = 0xFFFFFFFF - 1;

//----------------------------------------------------------------
// ShadowBuilder::PIMPL
//----------------------------------------------------------------

void ShadowBuilder::PIMPL::SetRegionPolygon(PolygonWithHoles const &polygon)
{
  m_verts.clear();
  VertexID id = 0;
  for (auto poly_it = polygon.loops.cbegin(); poly_it != polygon.loops.cend(); poly_it++)
  {
    uint32_t size = (uint32_t)poly_it->Size();
    VertexID localID = 0;
    for (auto vert_it = poly_it->cPointsBegin(); vert_it != poly_it->cPointsEnd(); vert_it++)
    {
      VertexID localNextID = (localID + 1) % size;
      VertexID localPrevID = (localID + size - 1) % size;

      Vertex v;
      v.point = *vert_it;
      v.nextID = id + localNextID;
      v.prevID = id + localPrevID;
      v.processed = false;

      m_verts.insert(Dg::Pair<VertexID, Vertex>(id + localID, v));

      localID++;
    }
    id += size;
  }
}

bool ShadowBuilder::PIMPL::TryBuildRayMap(vec2 const &source, DgPolygon *pOut)
{
  float epsilon = Dg::Constants<float>::EPSILON;

  pOut->Clear();
  m_shadowNodes.clear();
  for (auto it = m_verts.begin_rand(); it != m_verts.end_rand(); it++)
    it->second.processed = false;

  TempData *pTempData = new TempData[m_verts.size()]{};

  for (auto it = m_verts.begin_rand(); it != m_verts.end_rand(); it++)
  {
    // If already processed, continue
    if (it->second.processed)
      continue;
    it->second.processed = true;

    // Build the source ray
    vec2 v = it->second.point - source;
    float degrees = atan2(v.y(), v.x()) / Dg::Constants<float>::PI * 180.f;
    float lenSq = Dg::MagSq(v);
    if (Dg::IsZero(lenSq))
      continue;

    v /= sqrt(lenSq);
    ray2 ray(source, v);

    // Add the first vertex to the list
    pTempData[0].id = it->first;
    pTempData[0].point = it->second.point;
    pTempData[0].distanceSq = lenSq;
    uint32_t tempSize = 1;

    // Find all other vertices that are on the ray
    for (auto it2 = it; it2 != m_verts.end_rand(); it2++)
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

      pTempData[tempSize].id = it2->first;
      pTempData[tempSize].point = it2->second.point;
      pTempData[tempSize].distanceSq = lenSq2;
      tempSize++;
    }

    // Sort the ray-vertex list based on distance to the source.
    std::sort(pTempData, pTempData + tempSize,
      [](TempData const &a, TempData const &b) {return a.distanceSq < b.distanceSq; });

    // Find the closest boundary intersection with the ray.
    // If none is found, this means an already processed vertex is the
    // boundary.

    // If we find a valid back point, cull all temp points beyond this point.
    {
      vec2 endPoint = {};

      if (GetClosestIntersect(ray, &endPoint, pTempData, tempSize))
      {
        float backDistSq = Dg::MagSq(endPoint - source);
        uint32_t oldTempSize = tempSize;
        for (uint32_t i = 0; i < tempSize; i++)
        {
          if (backDistSq < pTempData[i].distanceSq)
          {
            tempSize = i;
            break;
          }
        }

        if (tempSize > 0)
        {
          pTempData[tempSize].point = endPoint;
          pTempData[tempSize].id = s_EdgeID;
          pTempData[tempSize].distanceSq = backDistSq;
          tempSize++;
        }
      }
    }

    // No vertex can be seen.
    if (tempSize == 0)
      continue;

    // Now check all edges from all ray-verts are on the same side. 
    // Otherwise they are going to cut off the line of sight.
    uint32_t side = SideNone;
    for (uint32_t i = 0; i < tempSize; i++)
    {
      VertexID id = pTempData[i].id;
      side = side | GetSide(id, ray);

      if (side == SideBoth)
      {
        tempSize = i + 1;
        break;
      }
    }

    Node node = {};
    node.point0 = pTempData[0].point;
    node.ID0 = pTempData[0].id;
    node.ID1 = s_InvalidID;

    // Emit Second vertex if we have more than one visible vertex on the ray.
    if (tempSize > 1)
    {
      node.ID1 = pTempData[tempSize - 1].id;
      node.point1 = pTempData[tempSize - 1].point;
    }

    // We now have the start and end point!
    float angle = atan2(v.y(), v.x());
    m_shadowNodes[angle] = node;
  }
  delete[] pTempData;

  return GetCoverage(source, pOut);
}

bool ShadowBuilder::PIMPL::IgnoreListContains(VertexID id, TempData const *pIgnoreList, uint32_t sizeIgnoreList)
{
  for (uint32_t i = 0; i < sizeIgnoreList; i++)
  {
    if (pIgnoreList[i].id == id)
      return true;
  }
  return false;
}

bool ShadowBuilder::PIMPL::GetClosestIntersect(ray2 const &ray, vec2 *pPoint, TempData const *pIgnoreList, uint32_t sizeIgnoreList)
{
  float ur = FLT_MAX;
  for (auto it = m_verts.begin_rand(); it != m_verts.end_rand(); it++)
  {
    if (IgnoreListContains(it->first, pIgnoreList, sizeIgnoreList) ||
        IgnoreListContains(it->second.nextID, pIgnoreList, sizeIgnoreList))
      continue;

    vec2 p1 = m_verts.at(it->second.nextID).point;
    seg s(it->second.point, p1);

    Dg::FI2SegmentRay<float> query;
    auto result = query(s, ray);

    if (result.code != Dg::QueryCode::Intersecting)
      continue;

    if (result.pointResult.ur < ur)
    {
      ur = result.pointResult.ur;
      *pPoint = result.pointResult.point;
    }
  }
  return ur != FLT_MAX;
}

ShadowBuilder::PIMPL::Side ShadowBuilder::PIMPL::GetSide(VertexID id, ray2 const &ray)
{
  if (id == s_EdgeID)
    return SideBoth;

  float epsilon = Dg::Constants<float>::EPSILON;
  vec2 next = m_verts.at(m_verts.at(id).nextID).point - ray.Origin();
  vec2 prev = m_verts.at(m_verts.at(id).prevID).point - ray.Origin();

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

bool ShadowBuilder::PIMPL::GetCoverage(xn::vec2 const &source, xn::DgPolygon *pOut)
{
  if (m_shadowNodes.size() < 3)
    return false;

  auto it = m_shadowNodes.cbegin();
  float closestDistSq = FLT_MAX;

  for (auto it2 = m_shadowNodes.cbegin(); it2 != m_shadowNodes.cend(); it2++)
  {
    float distSq = Dg::MagSq((source - it2->second.point0));
    if (distSq < closestDistSq)
    {
      it = it2;
      closestDistSq = distSq;
    }
  }

  bool lastWasVertex = true;
  for (size_t i = 0; i < m_shadowNodes.size(); i++)
  {
    auto itPrev = it;
    it++;
    if (it == m_shadowNodes.cend())
      it = m_shadowNodes.cbegin();

    if (it->second.ID1 == s_InvalidID)
    {
      lastWasVertex = true;
      pOut->PushBack(it->second.point0);
      continue;
    }

    if (!lastWasVertex)
    {
      pOut->PushBack(it->second.point0);
      lastWasVertex = true;
      if (it->second.ID1 != s_InvalidID)
      {
        pOut->PushBack(it->second.point1);
        lastWasVertex = false;
      }
    }
    else 
    {
      if (itPrev->second.ID0 == it->second.ID0)
      {
        pOut->PushBack(it->second.point0);
        lastWasVertex = true;
        if (it->second.ID1 != s_InvalidID)
        {
          pOut->PushBack(it->second.point1);
          lastWasVertex = false;
        }
      }
      else
      {
        pOut->PushBack(it->second.point1);
        pOut->PushBack(it->second.point0);
        lastWasVertex = false;
      }
    }
  }
}

//----------------------------------------------------------------
// ShadowBuilder
//----------------------------------------------------------------

ShadowBuilder::ShadowBuilder()
  : m_pimpl(new PIMPL())
{

}

ShadowBuilder::~ShadowBuilder()
{
  delete m_pimpl;
}

void ShadowBuilder::SetRegionPolygon(xn::PolygonWithHoles const &polygon)
{
  m_pimpl->SetRegionPolygon(polygon);
}

bool ShadowBuilder::TryBuildRayMap(xn::vec2 const &source, xn::DgPolygon *pOut)
{
  return m_pimpl->TryBuildRayMap(source, pOut);
}