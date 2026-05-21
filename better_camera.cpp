#include"tgaimage.h"
#include"geometry.h"
#include"model.h"
#include<algorithm>
#include<omp.h>
#include<limits>


//模型坐标系，场景坐标系，屏幕坐标系，摄像机坐标系
constexpr int width = 800;
constexpr int height = 800;

constexpr TGAColor white = {255, 255, 255, 255}; // BGRA order; 0~255
constexpr TGAColor red   = {  0,   0, 255, 255};
constexpr TGAColor green = {  0, 255,   0, 255};
constexpr TGAColor blue  = {255, 128,  64, 255};
constexpr TGAColor yellow= {  0, 200, 255, 255};

mat<4,4>ModelView,Viewport,Perspective;

void lookat(vec3 eye, vec3 center, vec3 up) {
    vec3 n=normalized(eye-center);
    vec3 l=normalized(cross(up, n));
    vec3 v=normalized(cross(n, l));
    ModelView = mat<4,4>{{{l.x, l.y, l.z,0},{v.x, v.y, v.z, 0},{n.x, n.y, n.z, 0},{0,   0,   0,   1}}}*
    mat<4,4>{{{1,0,0,-center.x},{0,1,0,-center.y},{0,0,1,-center.z},{0,0,0,1}}};
}

void perspective(const double f) {
    Perspective = {{{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0, -1/f,1}}};
}

void viewport(const int x, const int y, const int w, const int h) {
    Viewport = {{{w/2., 0, 0, x+w/2.}, {0, h/2., 0, y+h/2.}, {0,0,1,0}, {0,0,0,1}}};
}

double signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy) {
    return .5*((by-ay)*(bx+ax) + (cy-by)*(cx+bx) + (ay-cy)*(ax+cx));
}

// void triangle(int ax, int ay,int az, int bx, int by, int bz ,int cx, int cy, int cz, TGAImage &zbuffer,TGAImage &framebuffer, TGAColor color) {
//     int bbminx = std::min(std::min(ax, bx), cx); // bounding box for the triangle
//     int bbminy = std::min(std::min(ay, by), cy); // defined by its top left and bottom right corners
//     int bbmaxx = std::max(std::max(ax, bx), cx);
//     int bbmaxy = std::max(std::max(ay, by), cy);
//     double total_area =signed_triangle_area(ax, ay, bx, by, cx, cy);
//     if (total_area<1) return; //利用面积判断三角形是否退化，如果面积小于1则认为是退化的三角形，不进行光栅化
// #pragma omp parallel for
//     for (int x=bbminx; x<=bbmaxx; x++) {
//         for (int y=bbminy; y<=bbmaxy; y++) {
//             double alpha = signed_triangle_area(x, y, bx, by, cx, cy) / total_area;//奔驰定理
//             double beta  = signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
//             double gamma = signed_triangle_area(x, y, ax, ay, bx, by) / total_area;
//             if (alpha>=0 && beta>=0 && gamma>=0) {
//                 unsigned char z = static_cast<unsigned char>(alpha * az + beta * bz + gamma * cz);
//                 if(z>=zbuffer.get(x, y)[0]){//深度缓冲算法
//                     zbuffer.set(x, y, {z, z, z, 255});
//                     framebuffer.set(x, y, color);
//                 }
//             }
//         }
//     }
// }
void rasterize(const vec4 clip[3], std::vector<double> &zbuffer, TGAImage &framebuffer, const TGAColor color) {
    vec4 ndc[3]    = { clip[0]/clip[0].w, clip[1]/clip[1].w, clip[2]/clip[2].w };                //NDC
    vec2 screen[3] = { (Viewport*ndc[0]).xy(), (Viewport*ndc[1]).xy(), (Viewport*ndc[2]).xy() }; //创建三角形的屏幕坐标

    mat<3,3> ABC = {{ {screen[0].x, screen[0].y, 1.}, {screen[1].x, screen[1].y, 1.}, {screen[2].x, screen[2].y, 1.} }};
    if (ABC.det()<1) return; //剔除退化三角形

    auto [bbminx,bbmaxx] = std::minmax({screen[0].x, screen[1].x, screen[2].x}); //计算包围盒
    auto [bbminy,bbmaxy] = std::minmax({screen[0].y, screen[1].y, screen[2].y}); 
    mat<3,3> ABCinv = ABC.invert_transpose();
#pragma omp parallel for
    for (int x=std::max<int>(bbminx, 0); x<=std::min<int>(bbmaxx, framebuffer.width()-1); x++) { 
        for (int y=std::max<int>(bbminy, 0); y<=std::min<int>(bbmaxy, framebuffer.height()-1); y++) {
            vec3 bc = ABCinv * vec3{static_cast<double>(x), static_cast<double>(y), 1.}; 
            if (bc.x<0 || bc.y<0 || bc.z<0) continue;                                                    
            double z = bc * vec3{ ndc[0].z, ndc[1].z, ndc[2].z };
            if (z <= zbuffer[x+y*framebuffer.width()]) continue;
            zbuffer[x+y*framebuffer.width()] = z;
            framebuffer.set(x, y, color);
        }
    }
}

int main(int argc,char** argv){//命令行：.\\better_camera.exe a.obj
    std::cout<<"FUCK ALL THE WORLD!"<<std::endl;
    if(argc!=2){//检查model是否正确加载
        std::cerr<<"Usage: "<<argv[0]<<" obj/model.obj"<<std::endl;
        return 1;
    }

    constexpr int width  = 800;   
    constexpr int height = 800;
    constexpr vec3    eye{-1,0,2}; 
    constexpr vec3 center{0,0,0};  
    constexpr vec3     up{0,1,0};  

    lookat(eye, center, up);                             
    perspective(norm(eye-center));                      
    viewport(width/16, height/16, width*7/8, height*7/8);

    TGAImage framebuffer(width, height, TGAImage::RGB);
    std::vector<double> zbuffer(width*height, -std::numeric_limits<double>::max());
    for (int m=1; m<argc; m++) { // iterate through all input objects
        Model model(argv[m]);
        for (int i=0; i<model.nfaces(); i++) {
            vec4 clip[3];
            for (int j=0; j<3; j++) {
                vec3 v = model.vert(i,j);
                clip[j] = Perspective*ModelView*vec4{v.x, v.y, v.z, 1.};
            }
            TGAColor color={(uint8_t)(std::rand()%256), (uint8_t)(std::rand()%256), (uint8_t)(std::rand()%256), 255};//随机颜色
            rasterize(clip, zbuffer, framebuffer, color);
        }
    }
    framebuffer.write_tga_file("better_camera.tga");

    return 0;
}