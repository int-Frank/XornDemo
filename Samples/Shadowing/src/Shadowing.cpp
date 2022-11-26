
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
  , m_source(0.f, 0.f)
  , m_hasSource(true)
  , m_mouseDown(false)
{

}

bool Shadowing::SetGeometry(PolygonWithHoles const &polygon)
{
  m_shadowBuilder.SetRegionPolygon(polygon);
  m_shadowBuilder.TryBuildRayMap(m_source, &m_shadow);
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
}

void Shadowing::MouseDown(MouseInput button, vec2 const &p)
{
  if (button == MouseInput::LeftButton)
  {
    m_source = p;
    m_mouseDown = true;
    m_shadowBuilder.TryBuildRayMap(m_source, &m_shadow);
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
    m_shadowBuilder.TryBuildRayMap(m_source, &m_shadow);
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

  for (auto it = m_shadow.cEdgesBegin(); it != m_shadow.cEdgesEnd(); it++)
  {
    seg s = it.ToSegment();

    vec3 p0(s.GetP0().x(), s.GetP0().y(), 1.f);
    vec3 p1(s.GetP1().x(), s.GetP1().y(), 1.f);
    p0 = p0 * T_World_View;
    p1 = p1 * T_World_View;
    pRenderer->DrawLine(seg(vec2(p0.x(), p0.y()), vec2(p1.x(), p1.y())), strokeShadow);
    pRenderer->DrawFilledNGon(p0, 32, 5.f, fillSource);
  }
}