
#include <windows.h>

#include "Shadowing.h"
#include "xnPluginAPI.h"
#include "xnVersion.h"

using namespace xn;

DEFINE_STANDARD_EXPORTS
DEFINE_DLLMAIN

Module *xnPlugin_CreateModule(ModuleInitData *pData)
{
  return new Shadowing(pData);
}

char const *xnPlugin_GetModuleName()
{
  return "Shadowing";
}

Shadowing::Shadowing(ModuleInitData *pData)
  : Module(pData)
  , m_visibilityBuilder()
  , m_visibleRegion()
  , m_source(0.f, 0.f)
  , m_mouseDown(false)
  , m_showVertices(false)
{

}

bool Shadowing::SetGeometry(std::vector<PolygonLoop> const &loops)
{
  m_visibilityBuilder.SetRegion(loops);
  m_visibilityBuilder.TryBuildVisibilityPolygon(m_source, &m_visibleRegion);
  return true;
}

void Shadowing::_DoFrame(UIContext *pContext)
{
  if (pContext->Button("What is this?##Shadowing"))
    pContext->OpenPopup("Description##Shadowing");
  if (pContext->BeginPopup("Description##Shadowing"))
  {
    float wrap_width = 400.f;
    pContext->PushTextWrapPos(pContext->GetCursorPos().x() + wrap_width);
    pContext->Text("2D Shadow algorithm.", wrap_width);
    pContext->PopTextWrapPos();
    pContext->EndPopup();
  }
  pContext->Separator();

  pContext->Checkbox("Show vertices##Shadowing", &m_showVertices);

  static float stepSize = 1.f;
  pContext->InputFloat("Step size##Shadowing", &stepSize, 1.f, 10.f);
  if (pContext->InputFloat("x##Shadowing", &m_source.x(), stepSize, stepSize))
    m_visibilityBuilder.TryBuildVisibilityPolygon(m_source, &m_visibleRegion);
  if (pContext->InputFloat("y##Shadowing", &m_source.y(), stepSize, stepSize))
    m_visibilityBuilder.TryBuildVisibilityPolygon(m_source, &m_visibleRegion);

}

void Shadowing::Render(IRenderer *pRenderer)
{
  pRenderer->DrawFilledPolygon(m_visibleRegion, 0xFFCCCCCC, 0);
  pRenderer->DrawFilledCircle(m_source, 10.f, 0xFFFF00FF, 0);
}

void Shadowing::MouseDown(uint32_t modState, vec2 const &p)
{
  m_source = p;
  m_mouseDown = true;
  m_visibilityBuilder.TryBuildVisibilityPolygon(m_source, &m_visibleRegion);
}

void Shadowing::MouseUp(uint32_t modState)
{
  m_mouseDown = false;
}

void Shadowing::MouseMove(uint32_t modState, vec2 const &p)
{
  if (m_mouseDown)
  {
    m_source = p;
    m_visibilityBuilder.TryBuildVisibilityPolygon(m_source, &m_visibleRegion);
  }
}