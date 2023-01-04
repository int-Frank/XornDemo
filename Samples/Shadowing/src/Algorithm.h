#ifndef ALGORITHM_H
#define ALGORITHM_H

#include "xnGeometry.h"

class VisibilityBuilder
{
public:

  VisibilityBuilder();
  ~VisibilityBuilder();

  void SetRegion(std::vector<xn::PolygonLoop> const &loops);
  bool TryBuildVisibilityPolygon(xn::vec2 const &source, xn::DgPolygon *pOut);

private:

  class PIMPL;
  PIMPL *m_pimpl;
};

#endif