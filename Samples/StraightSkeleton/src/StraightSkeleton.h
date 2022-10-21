#ifndef STRAIGHTSKELETON_H
#define STRAIGHTSKELETON_H

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

class StraightSkeleton : public xn::Module
{
public:

  StraightSkeleton(xn::ModuleInitData *);

  void Render(xn::Renderer *pRenderer, xn::mat33 const &T_World_View) override;
  void Clear();

  bool SetGeometry(xn::PolygonWithHoles const &) override;

private:

  void _DoFrame(xn::UIContext *) override;

  std::vector<xn::seg> m_segments;
  std::vector<xn::seg> m_boundaryConnections;
  size_t m_vertCount;
  size_t m_edgeCount;
  size_t m_faceCount;

  bool m_showBoundaryConnections;
  xn::Draw::Stroke m_edgeProperties;
};

#endif