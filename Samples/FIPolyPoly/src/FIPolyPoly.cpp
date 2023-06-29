
#include <windows.h>

#include "FIPolyPoly.h"
#include "xnPluginAPI.h"
#include "xnVersion.h"
#include "xnLogger.h"
#include "xnModule.h"
#include "xnIRenderer.h"
#include "DgRNG_Local.h"

using namespace xn;

DEFINE_STANDARD_EXPORTS
DEFINE_DLLMAIN

typedef Dg::impl_FI2PolygonPolygon::PolygonsToGraph<float> PolysToGraph;
typedef Dg::impl_FI2PolygonPolygon::GraphBuilder<float> GraphBuilder;
typedef Dg::impl_FI2PolygonPolygon::Node<float> Node;

Module *xnPlugin_CreateModule(ModuleInitData *pData)
{
  return new FIPolyPoly(pData);
}

char const *xnPlugin_GetModuleName()
{
  return "FIPolyPoly";
}

class ColourGenerator
{
public :

  ColourGenerator(uint32_t seed = 0)
  {
    if (seed != 0)
      m_rng.SetSeed(seed);
  }

  xn::Colour NextColour()
  {
    xn::Colour clr;
    clr.rgba.r = m_rng.GetUintRange(100, 255) << 1;
    clr.rgba.g = m_rng.GetUintRange(100, 255) << 1;
    clr.rgba.b = m_rng.GetUintRange(100, 255) << 1;
    clr.rgba.a = 255;

    return clr;
  }

private:

  Dg::RNG_Local m_rng;
};

FIPolyPoly::FIPolyPoly(xn::ModuleInitData *pData)
  : Module(pData)
  , m_showGraph(true)
  , m_showSubPolygons(true)
{

}

static void DrawGraph(Graph const &graph, xn::IRenderer *pRenderer)
{
  float thickness = 3.f;

  ColourGenerator clrGen(42);
  for (auto it_node = graph.nodes.cbegin(); it_node != graph.nodes.cend(); it_node++)
  {
    xn::vec2 p0 = it_node->vertex;
    std::vector<xn::seg> lines;
    for (auto it_neighbour = it_node->neighbours.cbegin(); it_neighbour != it_node->neighbours.cend(); it_neighbour++)
    {
      xn::vec2 p1 = graph.nodes[it_neighbour->id].vertex;
      p1 = (p0 + p1) * 0.5f;

      lines.push_back(xn::seg(p0, p1));
    }
    pRenderer->DrawLineGroup(lines.data(), lines.size(), thickness, clrGen.NextColour(), 0);
  }

  Dg::DynamicArray<xn::vec2> nodes;
  for (auto it_node = graph.nodes.cbegin(); it_node != graph.nodes.cend(); it_node++)
    nodes.push_back(it_node->vertex);

  pRenderer->DrawFilledCircleGroup(nodes.data(), nodes.size(), 10.f, 0xFFFF00FF, 0);
}

static xn::DgPolygon ToPolygon(Dg::DynamicArray<Dg::Vector2<float>> const & verts, Dg::DynamicArray<uint32_t> const &indices)
{
  xn::DgPolygon polygon;
  for (size_t i = 0; i < indices.size(); i++)
    polygon.PushBack(verts[indices[i]]);
  return polygon;
}

static PolygonResult ToPolygons(Result const &input)
{
  PolygonResult result;
  result.boundary = ToPolygon(input.vertices, input.boundary);

  for (size_t i = 0; i < input.polyA.size(); i++)
    result.polyA.push_back(ToPolygon(input.vertices, input.polyA[i]));

  for (size_t i = 0; i < input.polyB.size(); i++)
    result.polyB.push_back(ToPolygon(input.vertices, input.polyB[i]));

  for (size_t i = 0; i < input.intersection.size(); i++)
    result.intersection.push_back(ToPolygon(input.vertices, input.intersection[i]));

  for (size_t i = 0; i < input.holes.size(); i++)
    result.holes.push_back(ToPolygon(input.vertices, input.holes[i]));

  return result;
}

static void DrawPolygons(PolygonResult const &result, xn::IRenderer *pRenderer)
{
  pRenderer->DrawPolygon(result.boundary, 3.f, 0xFFFFFFFF, 0);

  for (size_t i = 0; i < result.polyA.size(); i++)
    pRenderer->DrawFilledPolygon(result.polyA[i], 0xFF0000FF, 0);

  for (size_t i = 0; i < result.polyB.size(); i++)
    pRenderer->DrawFilledPolygon(result.polyB[i], 0xFFFF0000, 0);

  for (size_t i = 0; i < result.intersection.size(); i++)
    pRenderer->DrawFilledPolygon(result.intersection[i], 0xFF00FF00, 0);

  for (size_t i = 0; i < result.holes.size(); i++)
    pRenderer->DrawFilledPolygon(result.holes[i], 0xFF000000, 0);
}

void FIPolyPoly::MouseDown(uint32_t modState, xn::vec2 const &)
{
}

void FIPolyPoly::Render(xn::IRenderer *pRenderer)
{
  if (m_showGraph)
  {
    for (auto const &intersect : m_intersects)
      DrawGraph(intersect.graph, pRenderer);
  }

  if (m_showSubPolygons)
  {
    for (auto const &intersect : m_intersects)
      DrawPolygons(intersect.polygons, pRenderer);
  }
}

bool FIPolyPoly::SetGeometry(std::vector<xn::PolygonLoop> const &loops)
{
  /*for (auto const &loop : loops)
  {
    M_LOG_DEBUG("Loop");
    for (auto it = loop.cPointsBegin(); it != loop.cPointsEnd(); it++)
      M_LOG_DEBUG("%f, %f", it->x(), it->y());
    M_LOG_DEBUG("\n");
  }*/

  m_intersects.clear();

  for (size_t i = 0; i < loops.size(); i++)
  {
    for (size_t j = i + 1; j < loops.size(); j++)
    {
      Graph graph;
      PolysToGraph polysToGraph;

      auto code = polysToGraph.Execute(loops[i], loops[j], &graph);

      if (code == Dg::QueryCode::Fail)
      {
        M_LOG_ERROR("Failed to add polygons to graph %i, %i", i, j);
        continue;
      }

      GraphBuilder builder;
      code = builder.Execute(&graph);

      if (code == Dg::QueryCode::Fail)
      {
        M_LOG_ERROR("Failed to build graph %i, %i", i, j);
        continue;
      }

      if (code != Dg::QueryCode::Intersecting)
        continue;

      IntersectPair intersect;
      intersect.graph = graph;

      Query query;
      intersect.result = query(loops[i], loops[j]);

      intersect.polygons = ToPolygons(intersect.result);

      m_intersects.push_back(intersect);
    }
  }

  return true;
}

void FIPolyPoly::_DoFrame(xn::UIContext *pContext)
{
  if (pContext->Button("What is this?##FIPolyPoly"))
    pContext->OpenPopup("Description##FIPolyPoly");
  if (pContext->BeginPopup("Description##FIPolyPoly"))
  {
    float wrap_width = 400.f;
    pContext->PushTextWrapPos(pContext->GetCursorPos().x() + wrap_width);
    pContext->Text("Find Intersection of two polygons.", wrap_width);
    pContext->PopTextWrapPos();
    pContext->EndPopup();
  }

  pContext->Separator();

  pContext->Checkbox("Show graph##FIPolyPoly", &m_showGraph);
  pContext->Checkbox("Show subs##FIPolyPoly", &m_showSubPolygons);
}
