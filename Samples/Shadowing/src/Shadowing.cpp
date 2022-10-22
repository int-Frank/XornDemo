
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
{

}

void Shadowing::Clear()
{

}

bool Shadowing::SetGeometry(PolygonWithHoles const &polygon)
{
  Clear();
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

void Shadowing::Render(Renderer *pRenderer, mat33 const &T_World_View)
{

}