#include"tgaimage.h"
#include<cmath>
#include"geometry.h"
#include"model.h"
#include<tuple>
#include<omp.h>
#include<windows.h>


constexpr int width = 800;
constexpr int height = 800;

constexpr TGAColor white = {255, 255, 255, 255}; // BGRA order; 0~255
constexpr TGAColor red   = {  0,   0, 255, 255};
constexpr TGAColor green = {  0, 255,   0, 255};
constexpr TGAColor blue  = {255, 128,  64, 255};
constexpr TGAColor yellow= {  0, 200, 255, 255};

// void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color){
//     bool steep=std::abs(y1-y0)>std::abs(x1-x0);//斜率：大于1bool值为true
//     if(steep){
//         std::swap(x0, y0);
//         std::swap(x1, y1);
//     }
//     if(x0>x1){
//         std::swap(x0, x1);
//         std::swap(y0, y1);
//     }
//     int y=y0;//bresenham算法
//     int ierror=0;
//     for(int x=x0; x<=x1; x++){
//         if(steep){
//             image.set(y, x, color);
//         } else {
//             image.set(x, y, color);
//         }
//         ierror+=2*std::abs(y1-y0);
//         y+=(y1>y0?1:-1)*(ierror>std::abs(x1-x0));
//         ierror-=2*(x1-x0)*(ierror>std::abs(x1-x0));
//     }
// }

double signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy) {
    return .5*((by-ay)*(bx+ax) + (cy-by)*(cx+bx) + (ay-cy)*(ax+cx));
}


void triangle(int ax, int ay,int az, int bx, int by, int bz ,int cx, int cy, int cz, TGAImage &zbuffer,TGAImage &framebuffer, TGAColor color) {
    int bbminx = std::min(std::min(ax, bx), cx); // bounding box for the triangle
    int bbminy = std::min(std::min(ay, by), cy); // defined by its top left and bottom right corners
    int bbmaxx = std::max(std::max(ax, bx), cx);
    int bbmaxy = std::max(std::max(ay, by), cy);
    double total_area =signed_triangle_area(ax, ay, bx, by, cx, cy);
    if (total_area<1) return; //利用面积判断三角形是否退化，如果面积小于1则认为是退化的三角形，不进行光栅化
#pragma omp parallel for
    for (int x=bbminx; x<=bbmaxx; x++) {
        for (int y=bbminy; y<=bbmaxy; y++) {
            double alpha = signed_triangle_area(x, y, bx, by, cx, cy) / total_area;//奔驰定理
            double beta  = signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            double gamma = signed_triangle_area(x, y, ax, ay, bx, by) / total_area;
            if (alpha>=0 && beta>=0 && gamma>=0) {
                unsigned char z = static_cast<unsigned char>(alpha * az + beta * bz + gamma * cz);
                if(z>=zbuffer.get(x, y)[0]){//深度缓冲算法
                    zbuffer.set(x, y, {z, z, z, 255});
                    framebuffer.set(x, y, color);
                }
            }
        }
    }
}

std::tuple<int,int,int> project(vec3 v) { //投影函数，摄像机位于正中心，obj文件中的坐标范围为[-1,1],定义域：向量v，值域：屏幕坐标(x,y)
    return { (v.x + 1.) *  width/2,  //add1 is to shift the range from [-1,1] to 0[0,2], then scale it to fit the screen width
             (v.y + 1.) * height/2, //same for height
             (v.z + 1.) * 255/2 }; //depth buffer
}
int main(int argc,char** argv){//命令行：.\\rasterization_of_model.exe a.obj
    if(argc!=2){//检查model是否正确加载
        std::cerr<<"Usage: "<<argv[0]<<" obj/model.obj"<<std::endl;
        return 1;
    }
    Model model(argv[1]);
    TGAImage framebuffer(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

    for(int i=0; i<model.nfaces(); i++){//遍历所有三角形
        auto [ax, ay, az] = project(model.vert(i, 0));//获取第i个三角形的第0个顶点，并投影到屏幕坐标
        auto [bx, by, bz] = project(model.vert(i, 1));//同上，获取第1个顶点
        auto [cx, cy, cz] = project(model.vert(i, 2));//同上，获取第2个顶点
        TGAColor color={(uint8_t)(std::rand()%256), (uint8_t)(std::rand()%256), (uint8_t)(std::rand()%256), 255};//随机颜色
        triangle(ax, ay, az, bx, by, bz, cx, cy, cz, zbuffer, framebuffer, color);
    //     if(i%30==0){
    //         framebuffer.write_tga_file("rasterization_of_model.tga");
    //         Sleep(100);//观察
    //     }
    }

    // for(int i=0; i<model.nverts(); i++){//遍历所有顶点
    //     vec3 v=model.vert(i);//获取第i个顶点
    //     auto [x, y] = project(v);//将顶点投影到屏幕坐标
    //     framebuffer.set(x, y, white);//在屏幕上设置该坐标为白色
    // }
    
    framebuffer.write_tga_file("rasterization_of_model.tga");
    zbuffer.write_tga_file("zbuffer.tga");

    return 0;
}