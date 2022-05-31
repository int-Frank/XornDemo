#ifndef STRAIGHTSKELETON_H
#define STRAIGHTSKELETON_H

#include <vector>

#include "a2dModule.h"
#include "a2dCommon.h"
#include "a2dGeometry.h"
#include "a2dRenderer.h"
#include "a2dLineProperties.h"

class StraightSkeleton : public a2d::Module
{
public:

  StraightSkeleton(bool *pShow, a2d::Logger *pLogger);

  void Render(a2d::Renderer *pRenderer, a2d::mat33 const &T_World_View) override;
  void Clear();

  bool SetGeometry(a2d::Geometry const &) override;
  void DoFrame(a2d::UIContext *) override;

private:

  a2d::SegmentCollection m_segments;
  a2d::SegmentCollection m_boundaryConnections;
  size_t m_vertCount;
  size_t m_edgeCount;
  size_t m_faceCount;

  bool m_showBoundaryConnections;
  a2d::LineProperties m_edgeProperties;
};

#endif