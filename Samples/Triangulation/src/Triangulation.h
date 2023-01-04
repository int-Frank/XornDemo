#ifndef TRIANGULATION_H
#define TRIANGULATION_H

#include <vector>

#include "xnModule.h"
#include "xnCommon.h"
#include "xnIScene.h"
#include "DgSet_AVL.h"
#include "xnModuleInitData.h"

class Triangulation : public xn::Module
{
public:

  Triangulation(xn::ModuleInitData *pData);

  void Clear();

  bool SetGeometry(std::vector<xn::PolygonLoop> const &) override;

private:

  void _DoFrame(xn::UIContext *, xn::IScene *pScene) override;
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

  xn::PolygonWithHoles m_polygon;
  Dg::Set_AVL<UniqueEdge> m_edgeSet;
  size_t m_vertCount;
  size_t m_faceCount;

  xn::vec2 m_sizeCriteriaBounds;
  float m_sizeCriteria;
  float m_shapeCriteria;
  int m_LloydIterations;

  //xn::Draw::Stroke m_edgeProperties;
};

#endif