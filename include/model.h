#include "geometry.h"
#include "tgaimage.h"

class Model {
    std::vector<vec4> verts = {};    // array of vertices        ┐ generally speaking, these arrays
    std::vector<vec4> norms = {};    // array of normal vectors  │ do not have the same size
    std::vector<vec2> tex = {};      // array of tex coords      ┘ check the logs of the Model() constructor
    std::vector<int> facet_vrt = {}; //  ┐ per-triangle indices in the above arrays,
    std::vector<int> facet_nrm = {}; //  │ the size is supposed to be
    std::vector<int> facet_tex = {}; //  ┘ nfaces()*3
    TGAImage diffusemap  = {};// 漫反射纹理
    TGAImage specularmap={};       // 镜面反射
    TGAImage normalmap   = {};       // 法线贴图
    TGAImage tangentmap  = {};       // 切线空间法线贴图
public:
    Model(const std::string filename);
    int nverts() const; // number of vertices
    int nfaces() const; // number of triangles
    vec4 vert(const int i) const;                          // 0 <= i < nverts()
    vec4 vert(const int iface, const int nthvert) const;   // 0 <= iface <= nfaces(), 0 <= nthvert < 3
    vec4 normal(const int iface, const int nthvert) const; // normal coming from the "vn x y z" entries in the .obj file
    vec4 normal(const vec2 &uv) const;       
    vec4 tangentnormal(const vec2 &uv) const; //切线空间法线
    vec2 uv(const int iface, const int nthvert) const;     // uv coordinates of triangle corners
    TGAColor diffuse(const vec2 &uv) const;               // diffuse color from the diffuse map texture
    double specular(const vec2 &uv) const;               // mirror reflection intensity from the
};