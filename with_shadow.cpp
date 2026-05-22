#include <cstdlib>
#include <algorithm>
#include "our_gl.h"
#include "model.h"

extern mat<4,4> ModelView, Perspective, Viewport;
extern std::vector<double> zbuffer;

constexpr int width  = 800;      
constexpr int height = 800;
constexpr int shadoww=800;
constexpr int shadowh=800;
constexpr vec3    eye{-1, 0, 2};
constexpr vec3 center{ 0, 0, 0}; 
constexpr vec3     up{ 0, 1, 0};
constexpr vec3 light{1,1,1};


struct ShadowShader : IShader {
    const Model &model;
    ShadowShader(const Model &m) : model(m) {}

    virtual vec4 vertex(const int face, const int vert) {
        vec4 gl_Position = ModelView * model.vert(face, vert);  
        return Perspective * gl_Position;                         // 投影变换
    }

    virtual std::pair<bool,TGAColor> fragment(const vec3 bar) const {
        return {false, {255,255,255,255}}; //根据片段的深度值计算阴影强度
    }
};

struct PhongShader : IShader {
    const Model &model;
    vec4 l;
    vec2 varying_uv[3]; 
    vec4 varying_nrm[3];
    vec4 tri[3];
    float one_over_w[3];
    PhongShader(const vec3 light,const Model &m) : model(m) {
        l=normalized((ModelView*vec4{light.x, light.y, light.z, .0})); //光源在眼坐标系下的方向
    }

    virtual vec4 vertex(const int face, const int vert) {
        varying_uv[vert] = model.uv(face, vert);
        varying_nrm[vert] = ModelView.invert_transpose() * model.normal(face, vert);
        vec4 gl_Position = ModelView * model.vert(face, vert);  
        tri[vert] = gl_Position;
        return Perspective * gl_Position;                         // 投影变换
    }

    virtual std::pair<bool,TGAColor> fragment(const vec3 bar) const {
        mat<2,4> E = { tri[1]-tri[0], tri[2]-tri[0] };
        mat<2,2> U = { varying_uv[1]-varying_uv[0], varying_uv[2]-varying_uv[0] };
        mat<2,4> T = U.invert() * E;
        mat<4,4> D = {normalized(T[0]),  // 切线
                      normalized(T[1]),  // 副切线
                      normalized(varying_nrm[0]*bar[0] + varying_nrm[1]*bar[1] + varying_nrm[2]*bar[2]), // 
                      {0,0,0,1}}; // Darboux frame
        TGAColor base = model.diffuse(varying_uv[0]*bar.x + varying_uv[1]*bar.y + varying_uv[2]*bar.z);  // 漫反射纹理颜色
        vec2 uv = (varying_uv[0]*bar.x + varying_uv[1]*bar.y + varying_uv[2]*bar.z); //插值计算片段的纹理坐标
        vec4 n = normalized(D.transpose() * model.normal(uv)); //法线向量
   

        vec4 r=normalized(n*(n*l)*2-l); //反射光线方向
        double ambinent = .3; //环境光强度 
        double diff = std::max(0., n*l); //漫反射强度
        double spec = std::pow(std::max(0., r.z), 35); //镜面反射强度
        double intensity = std::min(1., ambinent + diff + spec);
        for (int i : {0,1,2}) base[i] *= intensity;

        return {false, base};
    }
};


//绘制zbuffer
void drop_zbuffer(std::string filename,std::vector<double> &zbuffer,int width,int height) {
    TGAImage zimg(width, height, TGAImage::RGBA, {0, 0, 0, 0});
    double minz=1000,maxz=-1000;
    //选定最大最小深度，确定后续范围
    for (int x=0;x<width;x++){
        for (int y=0;y<height;y++){
            double z=zbuffer[x+y*width];
            if(z>-100){
                minz=std::min(minz,z);
                maxz=std::max(maxz,z);
            }
        }
    }    
    //绘制zbuffer
    for (int x=0; x<width; x++) {
        for (int y=0; y<height; y++) {
            double z = zbuffer[x+y*width];
            if (z<-100) continue;
            z = (z - minz)/(maxz-minz) * 255;
            unsigned char c=std::clamp(int(z), 0, 255);//不知为何强制转换后全是乱值，只好用clamp限制0—255范围
            zimg.set(x, y, {c, c, c,255});
        }
    }
    zimg.write_tga_file(filename);

}


int main(int argc, char** argv) {//命令行：         .\\with_shadow.exe a.obj f.obj
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1;
    }
    
    //重要数据初始化
    lookat(eye, center, up);                                
    init_perspective(norm(eye-center));                       
    init_viewport(width/16, height/16, width*7/8, height*7/8); 
    init_zbuffer(width, height);
    TGAImage framebuffer(width, height, TGAImage::RGB, {177, 195, 209, 255});
    mat<4,4> M =  (Viewport*Perspective * ModelView).invert();//相机变化逆矩阵
    for (int m=1; m<argc; m++) {                    
        Model model(argv[m]);                      
        PhongShader shader(light, model);     
        for (int f=0; f<model.nfaces(); f++) {    
            Triangle clip = { shader.vertex(f, 0), 
                              shader.vertex(f, 1),
                              shader.vertex(f, 2) };
            rasterize(clip, shader, framebuffer);   
        }
    }
    std::vector<double> zbuffer_copy = zbuffer;



    {
        lookat(light, center, up);                                
        init_perspective(norm(eye-center));                       
        init_viewport(shadoww/16, shadowh/16, shadoww*7/8, shadowh*7/8); 
        init_zbuffer(shadoww, shadowh);
        TGAImage shadowmap(shadoww, shadowh, TGAImage::RGB,{177, 195, 209, 255});
        for (int m=1; m<argc; m++) {                   
            Model model(argv[m]);                      
            ShadowShader shader(model);    
            for (int f=0; f<model.nfaces(); f++) {      
                Triangle clip = { shader.vertex(f, 0),  
                              shader.vertex(f, 1),
                              shader.vertex(f, 2) };
            rasterize(clip, shader, shadowmap);   
            }
        }
        shadowmap.write_tga_file("shadowmap.tga");
    }
    drop_zbuffer("zbuffer.tga", zbuffer, shadoww, shadowh);

    std::vector<bool> mask(width*height, false);//1表示被光照到，0表示被遮挡
    mat<4,4> N = Viewport * Perspective * ModelView;//灯光坐标系变化矩阵

    for (int x=0; x<width; x++) {
        for (int y=0; y<height; y++) {
            vec4 fragment = M * vec4{double(x), double(y), zbuffer_copy[x+y*width], 1.};
            vec4 q = N * fragment;//将片段从屏幕坐标系变换到灯光坐标系
            vec3 p = q.xyz()/q.w;
            bool lit =  (fragment.z<-100 ||                                   // it's the background or
                        (p.x<0 || p.x>=shadoww || p.y<0 || p.y>=shadowh) ||   // it is out of bounds of the shadow buffer
                        (p.z > zbuffer[int(p.x) + int(p.y)*shadoww] - .03));  // it is visible
            mask[x+y*width] = lit;
        }
    }

    TGAImage maskimg(width, height, TGAImage::GRAYSCALE);
    for (int x=0; x<width; x++) {
        for (int y=0; y<height; y++) {
            if (mask[x+y*width]) continue;
            maskimg.set(x, y, {255, 255, 255, 255});
        }
    }
    maskimg.write_tga_file("mask.tga");

    for (int x=0; x<width; x++) {
        for (int y=0; y<height; y++) {
            if (mask[x+y*width]) continue;
            TGAColor c = framebuffer.get(x, y);
            vec3 a = {double(c[0]), double(c[1]), double(c[2])};
            if (norm(a)<80) continue;
            a = normalized(a)*80;
            framebuffer.set(x, y, {
                (unsigned char)std::clamp(a[0], 0.0, 255.0),
                (unsigned char)std::clamp(a[1], 0.0, 255.0),
                (unsigned char)std::clamp(a[2], 0.0, 255.0),
                255
            });
        }
    }
    framebuffer.write_tga_file("with_shadow.tga");


    return 0;
}

