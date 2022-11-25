
#include <stdint.h>
#include <cmath>
#include <algorithm>

#include "DgMap_AVL.h"
#include "DgQueryPointRay.h"
#include "DgQuerySegmentRay.h"

#include "xnGeometry.h"
#include "DgRay.h"

using namespace xn;

typedef Dg::Ray2<float> ray2;
typedef uint32_t VertexID; // Polygon vertex id.
static VertexID const InvalidID = 0xFFFFFFFF;
static VertexID const EdgeID = 0xFFFFFFFF - 1;

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

class ShadowBuilder
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

public:

  ShadowBuilder()
  {

  }

  ~ShadowBuilder()
  {

  }

  static bool IgnoreListContains(VertexID id, TempData const *pIgnoreList, uint32_t sizeIgnoreList)
  {
    for (uint32_t i = 0; i < sizeIgnoreList; i++)
    {
      if (pIgnoreList[i].id == id)
        return true;
    }
    return false;
  }

  bool GetClosestIntersect(ray2 const &ray, vec2 *pPoint, TempData const *pIgnoreList, uint32_t sizeIgnoreList)
  {
    float closestDistSq = FLT_MAX;
    for (auto it = m_verts.begin_rand(); it != m_verts.end_rand(); it++)
    {
      if (IgnoreListContains(it->first, pIgnoreList, sizeIgnoreList))
        continue;

      vec2 p1 = m_verts.at(it->second.nextID).point;
      seg s(it->second.point, p1);

      Dg::FI2SegmentRay<float> query;
      auto result = query(s, ray);

      if (result.code != Dg::QueryCode::Intersecting)
        continue;

      float distSq = Dg::MagSq(result.pointResult.point - ray.Origin());
      if (distSq < closestDistSq)
      {
        closestDistSq = distSq;
        *pPoint = result.pointResult.point;
      }
    }
    return closestDistSq != FLT_MAX;
  }

  bool TryBuildRayMap(vec2 const &source, DgPolygon *pOut)
  {
    float epsilon = Dg::Constants<float>::EPSILON;

    TempData *pTempData = new TempData[m_verts.size()]{};
    uint32_t sizeIDs = 0;

    for (auto it = m_verts.begin_rand(); it != m_verts.end_rand(); it++)
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

      // Add the first vertex to the list
      pTempData[0].id = it->first;
      pTempData[0].point = it->second.point;
      pTempData[0].distanceSq = lenSq;
      sizeIDs++;

      // Find all other vertices that are on the ray
      uint32_t tempSize = 0;
      for (auto it2 = it; it2 != m_verts.end_rand(); it2++)
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
        vec2 v2 = it2->second.point - source;
        float lenSq2 = Dg::MagSq(v2);
        if (Dg::IsZero(lenSq2))
          continue;

        pTempData[tempSize].id = it->first;
        pTempData[tempSize].point = it->second.point;
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
            pTempData[tempSize].id = EdgeID;
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
          tempSize = i;
          break;
        }
      }

      Node node = {};
      node.point0 = pTempData[0].point;
      node.ID0 = pTempData[0].id;
      node.ID1 = InvalidID;

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

    return GetCoverage(pOut);
  }

  void SetRegionPolygon(PolygonWithHoles const &polygon)
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

        m_verts.insert(Dg::Pair<VertexID, Vertex>(id + localID, v));

        localID++;
      }
      id += size;
    }
  }

private:

  Side GetSide(VertexID id, ray2 const &ray)
  {
    if (id == EdgeID)
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
    if (edgePrev > epsilon) sideNext = SideLeft;
    else if (edgePrev < -epsilon) sideNext = SideRight;

    return Side(sideNext | sidePrev);
  }

  bool GetCoverage(xn::DgPolygon *pOut)
  {
    if (m_shadowNodes.size() < 3)
      return false;

    for (auto it = m_shadowNodes.cbegin(); it != m_shadowNodes.cend(); it++)
    {
      if (it->second.ID1 == InvalidID)
      {
        pOut->PushBack(it->second.point0);
        continue;
      }

      // Find a matching
      auto itPrev = it;
      if (itPrev == m_shadowNodes.cbegin())
      {
        itPrev = m_shadowNodes.cend();
        itPrev--;
      }

      if (it->second.ID0 == itPrev->second.ID0 || it->second.ID0 == itPrev->second.ID1)
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

private:

  Dg::Map_AVL<VertexID, Vertex> m_verts;
  Dg::Map_AVL<float, Node> m_shadowNodes;
};