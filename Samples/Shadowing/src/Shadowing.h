#ifndef SHADOWING_H
#define SHADOWING_H

#include <vector>

#include "xnModule.h"
#include "xnCommon.h"
#include "xnGeometry.h"
#include "xnRenderer.h"
#include "xnDraw.h"
#include "xnModuleInitData.h"

namespace xn
{
  class Logger;
  class Renderer;
};

class Shadowing : public xn::Module
{
public:

  Shadowing(xn::ModuleInitData *);

  void Render(xn::Renderer *pRenderer, xn::mat33 const &T_World_View) override;
  void Clear();

  void MouseDown(xn::MouseInput, xn::vec2 const &);

  bool SetGeometry(xn::PolygonWithHoles const &) override;

private:

  void _DoFrame(xn::UIContext *) override;

private:

  Dg::Polygon2<float> m_shadow;
  xn::vec2 m_source;
  bool m_hasSource;
};

#endif