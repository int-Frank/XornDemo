#ifndef STRAIGHTSKELETON_H
#define STRAIGHTSKELETON_H

#include <vector>

#include "xnModule.h"
#include "xnCommon.h"
#include "xnGeometry.h"
#include "xnModuleInitData.h"
#include "xnIRenderer.h"
#include "xnLogger.h"

class StraightSkeleton : public xn::Module
{
public:

  StraightSkeleton(xn::ModuleInitData *);

  void Clear();
  bool SetGeometry(std::vector<xn::PolygonLoop> const &) override;
  void Render(xn::IRenderer *) override;

private:

  void _DoFrame(xn::UIContext *) override;

  std::vector<xn::seg> m_segments;
  std::vector<xn::seg> m_boundaryConnections;
  size_t m_vertCount;
  size_t m_edgeCount;
  size_t m_faceCount;

  bool m_showBoundaryConnections;
};

#endif