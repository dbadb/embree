// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "../builders/bvh_builder_sah.h"

namespace embree
{
  namespace isa
  {
    template<int N>
      struct BVHNBuilderVirtual
      {
        typedef BVHN<N> BVH;
        typedef typename BVH::NodeRef NodeRef;
        typedef FastAllocator::ThreadLocal2 Allocator;
      
        struct BVHNBuilderV {
          NodeRef build(FastAllocator* allocator, BuildProgressMonitor& progress, PrimRef* prims, const PrimInfo& pinfo, GeneralBVHBuilder::Settings settings);
          virtual NodeRef createLeaf (const BVHBuilderBinnedSAH::BuildRecord& current, Allocator* alloc) = 0;
        };

        template<typename CreateLeafFunc>
        struct BVHNBuilderT : public BVHNBuilderV
        {
          BVHNBuilderT (CreateLeafFunc createLeafFunc)
            : createLeafFunc(createLeafFunc) {}

          NodeRef createLeaf (const BVHBuilderBinnedSAH::BuildRecord& current, Allocator* alloc) {
            return createLeafFunc(current,alloc);
          }

        private:
          CreateLeafFunc createLeafFunc;
        };

        template<typename CreateLeafFunc>
        static NodeRef build(FastAllocator* allocator, CreateLeafFunc createLeaf, BuildProgressMonitor& progress, PrimRef* prims, const PrimInfo& pinfo, GeneralBVHBuilder::Settings settings) {
          return BVHNBuilderT<CreateLeafFunc>(createLeaf).build(allocator,progress,prims,pinfo,settings);
        }
      };

    template<int N>
      struct BVHNBuilderQuantizedVirtual
      {
        typedef BVHN<N> BVH;
        typedef typename BVH::NodeRef NodeRef;
        typedef FastAllocator::ThreadLocal2 Allocator;
      
        struct BVHNBuilderV {
          NodeRef build(FastAllocator* allocator, BuildProgressMonitor& progress, PrimRef* prims, const PrimInfo& pinfo, GeneralBVHBuilder::Settings settings);
          virtual NodeRef createLeaf (const BVHBuilderBinnedSAH::BuildRecord& current, Allocator* alloc) = 0;
        };

        template<typename CreateLeafFunc>
        struct BVHNBuilderT : public BVHNBuilderV
        {
          BVHNBuilderT (CreateLeafFunc createLeafFunc)
            : createLeafFunc(createLeafFunc) {}

          NodeRef createLeaf (const BVHBuilderBinnedSAH::BuildRecord& current, Allocator* alloc) {
            return createLeafFunc(current,alloc);
          }

        private:
          CreateLeafFunc createLeafFunc;
        };

        template<typename CreateLeafFunc>
        static NodeRef build(FastAllocator* allocator, CreateLeafFunc createLeaf, BuildProgressMonitor& progress, PrimRef* prims, const PrimInfo& pinfo, GeneralBVHBuilder::Settings settings) {
          return BVHNBuilderT<CreateLeafFunc>(createLeaf).build(allocator,progress,prims,pinfo,settings);
        }
      };

    template<int N>
      struct BVHNBuilderMblurVirtual
      {
        typedef BVHN<N> BVH;
        typedef typename BVH::AlignedNodeMB AlignedNodeMB;
        typedef typename BVH::NodeRef NodeRef;
        typedef FastAllocator::ThreadLocal2 Allocator;
      
        struct BVHNBuilderV {
          std::tuple<NodeRef,LBBox3fa> build(FastAllocator* allocator, BuildProgressMonitor& progress, PrimRef* prims, const PrimInfo& pinfo, GeneralBVHBuilder::Settings settings);
          virtual std::pair<NodeRef,LBBox3fa> createLeaf (const BVHBuilderBinnedSAH::BuildRecord& current, Allocator* alloc) = 0;
        };

        template<typename CreateLeafFunc>
        struct BVHNBuilderT : public BVHNBuilderV
        {
          BVHNBuilderT (CreateLeafFunc createLeafFunc)
            : createLeafFunc(createLeafFunc) {}

          std::pair<NodeRef,LBBox3fa> createLeaf (const BVHBuilderBinnedSAH::BuildRecord& current, Allocator* alloc) {
            return createLeafFunc(current,alloc);
          }

        private:
          CreateLeafFunc createLeafFunc;
        };

        template<typename CreateLeafFunc>
        static std::tuple<NodeRef,LBBox3fa> build(FastAllocator* allocator, CreateLeafFunc createLeaf, BuildProgressMonitor& progress, PrimRef* prims, const PrimInfo& pinfo, GeneralBVHBuilder::Settings settings) {
          return BVHNBuilderT<CreateLeafFunc>(createLeaf).build(allocator,progress,prims,pinfo,settings);
        }
      };
  }
}
