#ifndef SHADOWING_H
#define SHADOWING_H

#include <vector>

#include "xnModule.h"
#include "xnCommon.h"
#include "xnGeometry.h"
#include "xnModuleInitData.h"
#include "xnIRenderer.h"
#include "xnLogger.h"

#include "Algorithm.h"

class Shadowing : public xn::Module
{
public:

  Shadowing(xn::ModuleInitData *);

  void MouseDown(uint32_t modState, xn::vec2 const &) override;
  void MouseUp(uint32_t modState) override;
  void MouseMove(uint32_t modState, xn::vec2 const &) override;
  void Render(xn::IRenderer *) override;

  bool SetGeometry(std::vector<xn::PolygonLoop> const &) override;

private:

  void _DoFrame(xn::UIContext *) override;

private:

  VisibilityBuilder m_visibilityBuilder;
  Dg::Polygon2<float> m_visibleRegion;
  xn::vec2 m_source;
  bool m_mouseDown;
  bool m_showVertices;
};

#endif