// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "bvh.h"

#if defined(EMBREE_GEOMETRY_SUBDIV)

#include "bvh_refit.h"
#include "bvh_builder.h"

#include "../builders/primrefgen.h"
#include "../builders/bvh_builder_sah.h"

#include "../algorithms/parallel_for_for.h"
#include "../algorithms/parallel_for_for_prefix_sum.h"

#include "../subdiv/bezier_curve.h"
#include "../geometry/subdivpatch1cached_intersector1.h"

#include "../geometry/subdivpatch1cached.h"
#include "../geometry/grid_soa.h"

namespace embree
{
  namespace isa
  {
    typedef FastAllocator::ThreadLocal2 Allocator;

    template<int N, bool mblur>
    struct BVHNSubdivPatch1CachedBuilderBinnedSAHClass : public Builder, public BVHNRefitter<N>::LeafBoundsInterface
    {
      ALIGNED_STRUCT;

      typedef BVHN<N> BVH;
      typedef typename BVH::NodeRef NodeRef;
      typedef typename BVHN<N>::Allocator BVH_Allocator;

      static const size_t MBLUR = mblur?2:1;

      BVH* bvh;
      std::unique_ptr<BVHNRefitter<N>> refitter;
      Scene* scene;
      size_t numTimeSteps;
      mvector<PrimRef> prims; 
      mvector<BBox3fa> bounds; 
      ParallelForForPrefixSumState<PrimInfo> pstate;
      size_t numSubdivEnableDisableEvents;
      bool cached;

      BVHNSubdivPatch1CachedBuilderBinnedSAHClass (BVH* bvh, Scene* scene, bool cached)
        : bvh(bvh), refitter(nullptr), scene(scene), numTimeSteps(0), prims(scene->device), bounds(scene->device), numSubdivEnableDisableEvents(0), cached(cached) {}
      
      virtual const BBox3fa leafBounds (NodeRef& ref) const
      {
        if (ref == BVH::emptyNode) return BBox3fa(empty);
        size_t num; SubdivPatch1Cached *sptr = (SubdivPatch1Cached*)ref.leaf(num);
        const size_t index = ((size_t)sptr - (size_t)bvh->subdiv_patches.data()) / sizeof(SubdivPatch1Cached);
        return prims[index].bounds(); 
      }

      bool initializeHalfEdges(size_t& numPrimitives)
      {
        /* initialize all half edge structures */
        bool fastUpdateMode = true;
        numPrimitives = scene->getNumPrimitives<SubdivMesh,MBLUR>();
        if (numPrimitives > 0 || scene->isInterpolatable()) 
        {
          Scene::Iterator<SubdivMesh,MBLUR> iter(scene,scene->isInterpolatable());
          fastUpdateMode = parallel_reduce(size_t(0),iter.size(),true,[&](const range<size_t>& range)
          {
            bool fastUpdate = true;
            for (size_t i=range.begin(); i<range.end(); i++)
            {
              if (!iter[i]) continue;
              fastUpdate &= !iter[i]->vertexIndices.isModified(); 
              fastUpdate &= !iter[i]->faceVertices.isModified();
              fastUpdate &= !iter[i]->holes.isModified();
              fastUpdate &= !iter[i]->edge_creases.isModified();
              fastUpdate &= !iter[i]->edge_crease_weights.isModified();
              fastUpdate &= !iter[i]->vertex_creases.isModified();
              fastUpdate &= !iter[i]->vertex_crease_weights.isModified(); 
              fastUpdate &= iter[i]->levels.isModified();
              iter[i]->initializeHalfEdgeStructures();
              //iter[i]->patch_eval_trees.resize(iter[i]->size()*numTimeSteps);
            }
            return fastUpdate;
          }, [](const bool a, const bool b) { return a && b; });
        }

        /* only enable fast mode if no subdiv mesh got enabled or disabled since last run */
        fastUpdateMode &= numSubdivEnableDisableEvents == scene->numSubdivEnableDisableEvents;
        numSubdivEnableDisableEvents = scene->numSubdivEnableDisableEvents;
        return fastUpdateMode;
      }

      size_t countSubPatches()
      {
        Scene::Iterator<SubdivMesh,MBLUR> iter(scene);
        pstate.init(iter,size_t(1024));

        PrimInfo pinfo = parallel_for_for_prefix_sum( pstate, iter, PrimInfo(empty), [&](SubdivMesh* mesh, const range<size_t>& r, size_t k, const PrimInfo& base) -> PrimInfo
        { 
          size_t s = 0;
          for (size_t f=r.begin(); f!=r.end(); ++f) 
          {          
            if (!mesh->valid(f)) continue;
            s += patch_eval_subdivision_count (mesh->getHalfEdge(f)); 
          }
          return PrimInfo(s,empty,empty);
        }, [](const PrimInfo& a, const PrimInfo& b) -> PrimInfo { return PrimInfo(a.size()+b.size(),empty,empty); });

        return pinfo.size();
      }

      void rebuild(size_t numPrimitives)
      {
        SubdivPatch1Cached* const subdiv_patches = (SubdivPatch1Cached*) bvh->subdiv_patches.data();
        bvh->alloc.reset();

        Scene::Iterator<SubdivMesh,MBLUR> iter(scene);
        PrimInfo pinfo = parallel_for_for_prefix_sum( pstate, iter, PrimInfo(empty), [&](SubdivMesh* mesh, const range<size_t>& r, size_t k, const PrimInfo& base) -> PrimInfo
        {
          PrimInfo s(empty);
          for (size_t f=r.begin(); f!=r.end(); ++f) 
          {
            if (!mesh->valid(f)) continue;
            
            BVH_Allocator alloc(bvh);
            //for (size_t t=0; t<numTimeSteps; t++)
            //mesh->patch_eval_trees[f*numTimeSteps+t] = Patch3fa::create(alloc, mesh->getHalfEdge(f), mesh->getVertexBuffer(t).getPtr(), mesh->getVertexBuffer(t).getStride());

            patch_eval_subdivision(mesh->getHalfEdge(f),[&](const Vec2f uv[4], const int subdiv[4], const float edge_level[4], int subPatch)
            {
              const size_t patchIndex = base.size()+s.size();
              assert(patchIndex < numPrimitives);

              for (size_t t=0; t<numTimeSteps; t++)
              {
                SubdivPatch1Base& patch = subdiv_patches[numTimeSteps*patchIndex+t];
                new (&patch) SubdivPatch1Cached(mesh->id,unsigned(f),subPatch,mesh,t,uv,edge_level,subdiv,VSIZEX);
              }
              
              if (cached)
              {
                for (size_t t=0; t<numTimeSteps; t++)
                {
                  SubdivPatch1Base& patch = subdiv_patches[numTimeSteps*patchIndex+t];
                  BBox3fa bound = evalGridBounds(patch,0,patch.grid_u_res-1,0,patch.grid_v_res-1,patch.grid_u_res,patch.grid_v_res,mesh);
                  bounds[numTimeSteps*patchIndex+t] = bound;
                  if (t != 0) continue;
                  prims[patchIndex] = PrimRef(bound,patchIndex);
                  s.add(bound);
                }
              }
              else
              {
                BBox3fa mybounds[RTC_MAX_TIME_STEPS];
                SubdivPatch1Base& patch = subdiv_patches[numTimeSteps*patchIndex];
                patch.root_ref.data = (int64_t) GridSOA::create(&patch,(unsigned)numTimeSteps,scene,alloc,mybounds);

                for (size_t t=0; t<numTimeSteps; t++)
                {
                  BBox3fa bound = mybounds[t];
                  bounds[numTimeSteps*patchIndex+t] = bound;
                  if (t != 0) continue;
                  prims[patchIndex] = PrimRef(bound,patchIndex);
                  s.add(bound);
                }
              }
            });
          }
          return s;
        }, [](const PrimInfo& a, const PrimInfo& b) -> PrimInfo { return PrimInfo::merge(a, b); });

        auto virtualprogress = BuildProgressMonitorFromClosure([&] (size_t dn) { 
            //bvh->scene->progressMonitor(double(dn)); // FIXME: triggers GCC compiler bug
          });
        
        /* build normal BVH over patches */
        if (!mblur)
        {
          auto createLeaf = [&] (const BVHBuilderBinnedSAH::BuildRecord& current, Allocator* alloc) -> int {
            size_t items MAYBE_UNUSED = current.pinfo.size();
            assert(items == 1);
            const size_t patchIndex = prims[current.prims.begin()].ID();
            *current.parent = bvh->encodeLeaf((char*)&subdiv_patches[patchIndex],1);
            return 0;
          };
          
          BVHNBuilder<N>::build(bvh,createLeaf,virtualprogress,prims.data(),pinfo,N,1,1,1.0f,1.0f);
        }

        /* build MBlur BVH over patches */
        else
        {
          //BVHNBuilderMblur<N>::build(bvh,createLeaf,virtualprogress,prims.data(),pinfo,N,1,1,1.0f,1.0f);

           /* allocate buffers */
          const size_t numTimeSegments = scene->numTimeSteps-1; assert(scene->numTimeSteps > 1);
          //bvh->alloc.init_estimate(numPrimitives*sizeof(PrimRef)*numTimeSegments);
          NodeRef* roots = (NodeRef*) bvh->alloc.threadLocal2()->alloc0.malloc(sizeof(NodeRef)*numTimeSegments,BVH::byteNodeAlignment);
          
          /* build BVH for each timestep */
          BBox3fa box = empty;
          size_t num_bvh_primitives = 0;
          for (size_t t=0; t<numTimeSegments; t++)
          {
            auto createLeaf = [&] (const BVHBuilderBinnedSAH::BuildRecord& current, Allocator* alloc) -> std::pair<BBox3fa,BBox3fa> {
              size_t items MAYBE_UNUSED = current.pinfo.size();
              assert(items == 1);
              const size_t patchIndex = prims[current.prims.begin()].ID();
              *current.parent = bvh->encodeLeaf((char*)&subdiv_patches[numTimeSteps*patchIndex+0],1);
              const BBox3fa bounds0 = bounds[numTimeSteps*patchIndex+t+0];
              const BBox3fa bounds1 = bounds[numTimeSteps*patchIndex+t+1];
              return std::make_pair(bounds0,bounds1);
            };

            /* call BVH builder */
            NodeRef root; std::pair<BBox3fa,BBox3fa> tbounds;
            std::tie(root, tbounds) = BVHNBuilderMblur<N>::build(bvh,createLeaf,bvh->scene->progressInterface,prims.data(),pinfo,N,1,1,1.0f,1.0f);
            roots[t] = root;
            box.extend(merge(tbounds.first,tbounds.second));
            num_bvh_primitives = max(num_bvh_primitives,pinfo.size());
          }
          bvh->set(NodeRef((size_t)roots),box,num_bvh_primitives);
          bvh->msmblur = true;
        }
      }

      void cachedUpdate(size_t numPrimitives)
      {
        SubdivPatch1Cached* const subdiv_patches = (SubdivPatch1Cached*) bvh->subdiv_patches.data();

        Scene::Iterator<SubdivMesh,MBLUR> iter(scene);
        parallel_for_for_prefix_sum( pstate, iter, PrimInfo(empty), [&](SubdivMesh* mesh, const range<size_t>& r, size_t k, const PrimInfo& base) -> PrimInfo
        {
          PrimInfo s(empty);
          for (size_t f=r.begin(); f!=r.end(); ++f) 
          {
            if (!mesh->valid(f)) continue;
            
            patch_eval_subdivision(mesh->getHalfEdge(f),[&](const Vec2f uv[4], const int subdiv[4], const float edge_level[4], int subPatch)
            {
              const size_t patchIndex = base.size()+s.size();
              assert(patchIndex < numPrimitives);
              SubdivPatch1Base& patch = subdiv_patches[patchIndex];
              BBox3fa bound = empty;
              
              bool grid_changed = patch.updateEdgeLevels(edge_level,subdiv,mesh,VSIZEX);
              if (grid_changed) {
                patch.resetRootRef();
                bound = evalGridBounds(patch,0,patch.grid_u_res-1,0,patch.grid_v_res-1,patch.grid_u_res,patch.grid_v_res,mesh);
              }
              else {
                bound = bounds[patchIndex];
              }

              bounds[patchIndex] = bound;
              prims[patchIndex] = PrimRef(bound,patchIndex);
              s.add(bound);
            });
          }
          return s;
        }, [](const PrimInfo& a, const PrimInfo& b) -> PrimInfo { return PrimInfo::merge(a, b); });

        /* refit BVH over patches */
        if (!refitter)
          refitter.reset(new BVHNRefitter<N>(bvh,*(typename BVHNRefitter<N>::LeafBoundsInterface*)this));
        
        refitter->refit();
      }

      void build(size_t, size_t) 
      {
        numTimeSteps = mblur ? scene->numTimeSteps : 1;

        /* initialize all half edge structures */
        size_t numPatches;
        bool fastUpdateMode = initializeHalfEdges(numPatches);
        //static size_t counter = 0; if ((++counter) % 16 == 0) fastUpdateMode = false;

        /* skip build for empty scene */
        if (numPatches == 0) {
          prims.resize(numPatches);
          bounds.resize(numPatches);
          bvh->set(BVH::emptyNode,empty,0);
          return;
        }

        double t0 = bvh->preBuild(TOSTRING(isa) "::BVH" + toString(N) + "SubdivPatch1" + (mblur ? "MBlur" : "") + (cached ? "Cached" : "") + "BuilderBinnedSAH");
        
        /* calculate number of primitives (some patches need initial subdivision) */
        size_t numSubPatches = countSubPatches();
        prims.resize(numSubPatches);
        bounds.resize(numSubPatches*numTimeSteps);
        
        /* exit if there are no primitives to process */
        if (numSubPatches == 0) {
          bvh->set(BVH::emptyNode,empty,0);
          bvh->postBuild(t0);
          return;
        }
        
        /* Allocate memory for gregory and b-spline patches */
        bvh->subdiv_patches.resize(sizeof(SubdivPatch1Cached) * numSubPatches * numTimeSteps);

        /* switch between fast and slow mode */
        if (mblur) rebuild(numSubPatches);
        else if (cached && fastUpdateMode) cachedUpdate(numSubPatches);
        else rebuild(numSubPatches);
        
	/* clear temporary data for static geometry */
	if (scene->isStatic()) {
          prims.clear();
          bvh->shrink();
        }
        bvh->cleanup();
        bvh->postBuild(t0);        
      }
      
      void clear() {
        prims.clear();
      }
    };
    
    /* entry functions for the scene builder */
    Builder* BVH4SubdivPatch1CachedBuilderBinnedSAH(void* bvh, Scene* scene, size_t mode) { return new BVHNSubdivPatch1CachedBuilderBinnedSAHClass<4,false>((BVH4*)bvh,scene,mode); }
    Builder* BVH4SubdivPatch1MBlurCachedBuilderBinnedSAH(void* bvh, Scene* scene, size_t mode) { return new BVHNSubdivPatch1CachedBuilderBinnedSAHClass<4,true>((BVH4*)bvh,scene,mode); }
  }
}
#endif
