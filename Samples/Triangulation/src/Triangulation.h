#ifndef TRIANGULATION_H
#define TRIANGULATION_H

#include <vector>

#include "xnModule.h"
#include "xnCommon.h"
#include "xnRenderer.h"
#include "xnLineProperties.h"
#include "DgSet_AVL.h"

namespace xn
{
  class Logger;
  class Renderer;
};

class Triangulation : public xn::Module
{
public:

  Triangulation(bool *pShow, xn::Logger *pLogger);

  void Render(xn::Renderer *pRenderer, xn::mat33 const &T_World_View) override;
  void Clear();

  bool SetGeometry(xn::PolygonGroup const &) override;
  void DoFrame(xn::UIContext *) override;

private:

  bool Update();

  class UniqueEdge
  {
  public:

    UniqueEdge() {}
    UniqueEdge(xn::vec2 const &a, xn::vec2 const &b);
    static bool VecLess(xn::vec2 const &a, xn::vec2 const &b);
    bool operator<(UniqueEdge const &a) const;

    xn::vec2 p0;
    xn::vec2 p1;
  };

  void SetValueBounds();

  xn::PolygonGroup m_geometry;
  Dg::Set_AVL<UniqueEdge> m_edgeSet;
  size_t m_vertCount;
  size_t m_faceCount;

  xn::vec2 m_sizeCriteriaBounds;
  float m_sizeCriteria;
  float m_shapeCriteria;
  int m_LloydIterations;

  xn::LineProperties m_edgeProperties;
};

#endif