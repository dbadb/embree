// ======================================================================== //
// Copyright 2009-2013 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "heuristic_strand_partition.h"

namespace embree
{
  const StrandSplit StrandSplit::find(size_t threadIndex, BezierRefList& prims)
  {
    /* first try to split two hair strands */
    BezierRefList::block_iterator_unsafe i = prims;
    Vec3fa axis0 = normalize(i->p3 - i->p0);
    float bestCos = 1.0f;
    Bezier1 bestI = *i;

    for (i; i; i++) {
      Vec3fa axisi = i->p3 - i->p0;
      float leni = length(axisi);
      if (leni == 0.0f) continue;
      axisi /= leni;
      float cos = abs(dot(axisi,axis0));
      if (cos < bestCos) { bestCos = cos; bestI = *i; }
    }
    Vec3fa axis1 = normalize(bestI.p3-bestI.p0);

    /* partition the two strands */
    size_t num0 = 0, num1 = 0;
    //NAABBox3fa naabb0(one,inf);
    //NAABBox3fa naabb1(one,inf);

    BBox3fa lbounds = empty, rbounds = empty;
    const LinearSpace3fa space0 = frame(axis0).transposed();
    const LinearSpace3fa space1 = frame(axis1).transposed();
    
    for (BezierRefList::block_iterator_unsafe i = prims; i; i++) 
    {
      Bezier1& prim = *i;
      const Vec3fa axisi = normalize(prim.p3-prim.p0);
      const float cos0 = abs(dot(axisi,axis0));
      const float cos1 = abs(dot(axisi,axis1));
      
      if (cos0 > cos1) { num0++; lbounds.extend(prim.bounds(space0)); }
      else             { num1++; rbounds.extend(prim.bounds(space1)); }
    }
    
    StrandSplit split;				
    split.axis0 = axis0;
    split.axis1 = axis1;
    if (num0 == 0 || num1 == 0) 
      split.cost = inf;
    else
      split.cost = float(num0)*halfArea(lbounds) + float(num1)*halfArea(rbounds);
    return split;
  }

  void StrandSplit::split(size_t threadIndex, PrimRefBlockAlloc<Bezier1>& alloc, BezierRefList& prims, 
			  BezierRefList& lprims_o, PrimInfo& linfo_o, BezierRefList& rprims_o, PrimInfo& rinfo_o) const 
  {
    BezierRefList::item* lblock = lprims_o.insert(alloc.malloc(threadIndex));
    BezierRefList::item* rblock = rprims_o.insert(alloc.malloc(threadIndex));
    
    while (BezierRefList::item* block = prims.take()) 
    {
      for (size_t i=0; i<block->size(); i++) 
      {
        const Bezier1& prim = block->at(i); 
	const Vec3fa axisi = normalize(prim.p3-prim.p0);
	const float cos0 = abs(dot(axisi,axis0));
	const float cos1 = abs(dot(axisi,axis1));

        if (cos0 > cos1) 
        {
          linfo_o.add(prim.bounds(),prim.center());
          if (likely(lblock->insert(prim))) continue; 
          lblock = lprims_o.insert(alloc.malloc(threadIndex));
          lblock->insert(prim);
        } 
        else 
        {
          rinfo_o.add(prim.bounds(),prim.center());
          if (likely(rblock->insert(prim))) continue;
          rblock = rprims_o.insert(alloc.malloc(threadIndex));
          rblock->insert(prim);
        }
      }
      alloc.free(threadIndex,block);
    }
  }
}
