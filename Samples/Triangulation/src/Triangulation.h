#ifndef TRIANGULATION_H
#define TRIANGULATION_H

#include <vector>

#include "a2dModule.h"
#include "a2dCommon.h"
#include "a2dRenderer.h"
#include "a2dLineProperties.h"
#include "DgSet_AVL.h"

class Triangulation : public a2d::Module
{
public:

  Triangulation(bool *pShow, a2d::Logger *pLogger);

  void Render(a2d::Renderer *pRenderer, a2d::mat33 const &T_World_View) override;
  void Clear();

  bool SetGeometry(a2d::Geometry const &) override;
  void DoFrame(a2d::UIContext *) override;

private:

  bool Update();

  class UniqueEdge
  {
  public:

    UniqueEdge() {}
    UniqueEdge(a2d::vec2 const &a, a2d::vec2 const &b);
    static bool VecLess(a2d::vec2 const &a, a2d::vec2 const &b);
    bool operator<(UniqueEdge const &a) const;

    a2d::vec2 p0;
    a2d::vec2 p1;
  };

  void SetValueBounds();

  a2d::Geometry m_geometry;
  Dg::Set_AVL<UniqueEdge> m_edgeSet;
  size_t m_vertCount;
  size_t m_faceCount;

  a2d::vec2 m_sizeCriteriaBounds;
  float m_sizeCriteria;
  float m_shapeCriteria;
  int m_LloydIterations;

  a2d::LineProperties m_edgeProperties;
};

#endif