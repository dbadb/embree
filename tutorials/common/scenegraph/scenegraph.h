// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#pragma once

#include "../default.h"
#include "materials.h"
#include "lights.h"

namespace embree
{  
  struct Texture 
  {
    enum {
      RGBA8        = 1,
      RGB8         = 2,
      FLOAT32      = 3,
      PTEX_RGBA8   = 4,
      PTEX_FLOAT32 = 5
    };
    
    int width;
    int height;    
    int format;
    union {
      int bytesPerTexel;
      int faceTextures;
    };
    int width_mask;
    int height_mask;
    
    void *data;
    
  Texture() 
  : width(-1), height(-1), format(-1), bytesPerTexel(0), data(nullptr), width_mask(0), height_mask(0) {}

  };

  struct SceneGraph
  {
    struct Node : public RefCount
    {
      Node () 
        : name("unnamed") {}
      
      Node (const std::string& name)
        : name(name) {}
      
      std::string name;
    };
    
    struct TransformNode : public Node
    {
      ALIGNED_STRUCT;
      TransformNode (const AffineSpace3fa& xfm, const Ref<Node>& child)
        : xfm0(xfm), xfm1(xfm), child(child) {}
      
    public:
      AffineSpace3fa xfm0;
      AffineSpace3fa xfm1;
      Ref<Node> child;
    };
    
    struct GroupNode : public Node
    { 
      GroupNode (const size_t N = 0) { 
        children.resize(N); 
      }
      
      void add(const Ref<Node>& node) {
        children.push_back(node);
      }
      
      void set(const size_t i, const Ref<Node>& node) {
        children[i] = node;
      }
      
    public:
      std::vector<Ref<Node> > children;
    };
    
    template<typename Light>
      struct LightNode : public Node
    {
      ALIGNED_STRUCT;

    LightNode(const Light& light)
      : light(light) {}
      
      Light light;
    };
    
    struct MaterialNode : public Node
    {
      ALIGNED_STRUCT;

    MaterialNode(const Material& material)
      : material(material) {}
      
      Material material;
    };
    
    /*! Mesh. */
    struct TriangleMeshNode : public Node
    {
      struct Triangle 
      {
      public:
        Triangle() {}
        Triangle (int v0, int v1, int v2) 
        : v0(v0), v1(v1), v2(v2) {}
      public:
        int v0, v1, v2;
      };
      
    public:
      TriangleMeshNode (Ref<MaterialNode> material) 
        : material(material) {}
      
    public:
      avector<Vec3fa> v;
      avector<Vec3fa> v2;
      avector<Vec3fa> vn;
      std::vector<Vec2f> vt;
      std::vector<Triangle> triangles;
      Ref<MaterialNode> material;
    };
    
    /*! Subdivision Mesh. */
    struct SubdivMeshNode : public Node
    {
      SubdivMeshNode (Ref<MaterialNode> material) 
        : material(material) {}
      
      avector<Vec3fa> positions;            //!< vertex positions
      avector<Vec3fa> normals;              //!< face vertex normals
      std::vector<Vec2f> texcoords;             //!< face texture coordinates
      std::vector<int> position_indices;        //!< position indices for all faces
      std::vector<int> normal_indices;          //!< normal indices for all faces
      std::vector<int> texcoord_indices;        //!< texcoord indices for all faces
      std::vector<int> verticesPerFace;         //!< number of indices of each face
      std::vector<int> holes;                   //!< face ID of holes
      std::vector<Vec2i> edge_creases;          //!< index pairs for edge crease 
      std::vector<float> edge_crease_weights;   //!< weight for each edge crease
      std::vector<int> vertex_creases;          //!< indices of vertex creases
      std::vector<float> vertex_crease_weights; //!< weight for each vertex crease
      Ref<MaterialNode> material;
    };
    
    /*! Hair Set. */
    struct HairSetNode : public Node
    {
      struct Hair
      {
      public:
        Hair () {}
        Hair (int vertex, int id)
        : vertex(vertex), id(id) {}
      public:
        int vertex,id;  //!< index of first control point and hair ID
      };
      
    public:
      HairSetNode (Ref<MaterialNode> material)
      : material(material) {}
      
    public:
      avector<Vec3fa> v;        //!< hair control points (x,y,z,r)
      avector<Vec3fa> v2;       //!< hair control points (x,y,z,r)
      std::vector<Hair> hairs;  //!< list of hairs
      Ref<MaterialNode> material;
    };

  public:
    static Ref<Node> load(const FileName& fname);
    static void store(Ref<SceneGraph::Node> root, const FileName& fname);
    static void set_motion_blur(Ref<Node> node0, Ref<Node> node1);
  };
}