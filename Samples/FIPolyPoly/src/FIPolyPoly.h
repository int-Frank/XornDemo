#ifndef FIPOLYPOLY_H
#define FIPOLYPOLY_H

#include <vector>

#include "xnModule.h"
#include "xnCommon.h"
#include "xnGeometry.h"
#include "xnModuleInitData.h"
#include "xnIRenderer.h"

#include "DgQueryPolygonPolygon.h"

typedef Dg::impl_FI2PolygonPolygon::Graph<float> Graph;
typedef Dg::FI2PolygonPolygon<float>             Query;
typedef Dg::FI2PolygonPolygon<float>::Result     Result;

struct PolygonResult
{
  xn::DgPolygon boundary;
  Dg::DynamicArray<xn::DgPolygon> polyA;
  Dg::DynamicArray<xn::DgPolygon> polyB;
  Dg::DynamicArray<xn::DgPolygon> intersection;
  Dg::DynamicArray<xn::DgPolygon> holes;
};

struct IntersectPair
{
  Graph graph;
  Result result;
  PolygonResult polygons;
};

class FIPolyPoly : public xn::Module
{
public:

  FIPolyPoly(xn::ModuleInitData *);

  void MouseDown(uint32_t modState, xn::vec2 const &) override;
  void Render(xn::IRenderer *) override;

  bool SetGeometry(std::vector<xn::PolygonLoop> const &) override;

private:

  void _DoFrame(xn::UIContext *) override;

private:

  Dg::DynamicArray<IntersectPair> m_intersects;

  bool m_showGraph;
  bool m_showSubPolygons;
};

#endif