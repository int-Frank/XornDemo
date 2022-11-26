#ifndef ALGORITHM_H
#define ALGORITHM_H

#include "xnGeometry.h"

class ShadowBuilder
{
public:

  ShadowBuilder();
  ~ShadowBuilder();

  void SetRegionPolygon(xn::PolygonWithHoles const &polygon);
  bool TryBuildRayMap(xn::vec2 const &source, xn::DgPolygon *pOut);

private:

  class PIMPL;
  PIMPL *m_pimpl;
};

#endif