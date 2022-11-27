
#include <windows.h>

#include "Shadowing.h"
#include "xnPluginAPI.h"
#include "xnVersion.h"
#include "xnLogger.h"

using namespace xn;

DEFINE_STANDARD_EXPORTS
DEFINE_DLLMAIN

Module *xnPlugin_CreateModule(xn::ModuleInitData *pData)
{
  return new Shadowing(pData);
}

char const *xnPlugin_GetModuleName()
{
  return "Shadowing";
}

Shadowing::Shadowing(xn::ModuleInitData *pData)
  : Module(pData)
  , m_visibilityBuilder()
  , m_visibleRegion()
  , m_source(0.f, 0.f)
  , m_mouseDown(false)
  , m_showVertices(false)
{

}

bool Shadowing::SetGeometry(PolygonWithHoles const &polygon)
{
  m_visibilityBuilder.SetRegion(polygon);
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

void Shadowing::MouseDown(MouseInput button, vec2 const &p)
{
  if (button == MouseInput::LeftButton)
  {
    m_source = p;
    m_mouseDown = true;
    m_visibilityBuilder.TryBuildVisibilityPolygon(m_source, &m_visibleRegion);
  }
}

void Shadowing::MouseUp(MouseInput button)
{
  if (button == MouseInput::LeftButton)
    m_mouseDown = false;
}

void Shadowing::MouseMove(vec2 const &p)
{
  if (m_mouseDown)
  {
    m_source = p;
    m_visibilityBuilder.TryBuildVisibilityPolygon(m_source, &m_visibleRegion);
  }
}

void Shadowing::Render(Renderer *pRenderer, mat33 const &T_World_View)
{
  vec3 source3(m_source.x(), m_source.y(), 1.f);
  source3 = source3 * T_World_View;
  vec2 source(source3.x(), source3.y());

  xn::Colour clrSource(0xFFFFFFFF);
  xn::Colour clrPolygon(0xFFC51BC6);
  xn::Draw::Fill fillSource(clrSource);
  xn::Draw::Stroke strokeShadow(clrPolygon, 3.f);

  pRenderer->DrawFilledNGon(source, 32, 5.f, fillSource);

  for (auto it = m_visibleRegion.cEdgesBegin(); it != m_visibleRegion.cEdgesEnd(); it++)
  {
    seg s = it.ToSegment();

    vec3 p0(s.GetP0().x(), s.GetP0().y(), 1.f);
    vec3 p1(s.GetP1().x(), s.GetP1().y(), 1.f);
    p0 = p0 * T_World_View;
    p1 = p1 * T_World_View;
    pRenderer->DrawLine(seg(vec2(p0.x(), p0.y()), vec2(p1.x(), p1.y())), strokeShadow);
    
    if (m_showVertices)
      pRenderer->DrawFilledNGon(p0, 32, 5.f, fillSource);
  }
}