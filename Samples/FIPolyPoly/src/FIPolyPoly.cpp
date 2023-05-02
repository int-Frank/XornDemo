
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
}

void FIPolyPoly::MouseDown(uint32_t modState, xn::vec2 const &)
{
}

void FIPolyPoly::Render(xn::IRenderer *pRenderer)
{
  for (auto const & intersect : m_intersects)
    DrawGraph(intersect.graph, pRenderer);
}

bool FIPolyPoly::SetGeometry(std::vector<xn::PolygonLoop> const &loops)
{
  m_intersects.clear();

  for (size_t i = 0; i < loops.size(); i++)
  {
    for (size_t j = i + 1; j < loops.size(); j++)
    {
      Graph graph;
      GraphBuilder builder;
      auto code = builder.Execute(loops[i], loops[j], &graph);

      if (code == Dg::QueryCode::Fail)
        M_LOG_ERROR("Failed to build graph %i, %i", i, j);

      if (code != Dg::QueryCode::Intersecting)
        continue;

      IntersectPair intersect;
      intersect.graph = graph;

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
}
