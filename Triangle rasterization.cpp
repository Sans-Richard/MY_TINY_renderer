#include"tgaimage.h"
#include<cmath>
#include"geometry.h"


constexpr int width = 150;
constexpr int height = 150;

constexpr TGAColor white = {255, 255, 255, 255}; // BGRA order; 0~255
constexpr TGAColor red   = {  0,   0, 255, 255};
constexpr TGAColor green = {  0, 255,   0, 255};
constexpr TGAColor blue  = {255, 128,  64, 255};
constexpr TGAColor yellow= {  0, 200, 255, 255};

void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color){
    bool steep=std::abs(y1-y0)>std::abs(x1-x0);//斜率：大于1bool值为true
    if(steep){
        std::swap(x0, y0);
        std::swap(x1, y1);
    }
    if(x0>x1){
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    int y=y0;//bresenham算法
    int ierror=0;
    for(int x=x0; x<=x1; x++){
        if(steep){
            image.set(y, x, color);
        } else {
            image.set(x, y, color);
        }
        ierror+=2*std::abs(y1-y0);
        y+=(y1>y0?1:-1)*(ierror>std::abs(x1-x0));
        ierror-=2*(x1-x0)*(ierror>std::abs(x1-x0));
    }
}

double signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy) {
    return .5*((by-ay)*(bx+ax) + (cy-by)*(cx+bx) + (ay-cy)*(ax+cx));
}

void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color) {
    int bbminx = std::min(std::min(ax, bx), cx); // bounding box for the triangle
    int bbminy = std::min(std::min(ay, by), cy); // defined by its top left and bottom right corners
    int bbmaxx = std::max(std::max(ax, bx), cx);
    int bbmaxy = std::max(std::max(ay, by), cy);
    double total_area =signed_triangle_area(ax, ay, bx, by, cx, cy);
#pragma omp parallel for
    for (int x=bbminx; x<=bbmaxx; x++) {
        for (int y=bbminy; y<=bbmaxy; y++) {
            double alpha = signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            double beta  = signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            double gamma = signed_triangle_area(x, y, ax, ay, bx, by) / total_area;
            if (alpha>=0 && beta>=0 && gamma>=0) {
                framebuffer.set(x, y, color);
            }
        }
    }
}
int main() {
    TGAImage framebuffer(width, height, TGAImage::RGB);
    triangle(  7, 45, 35, 100, 45,  60, framebuffer, red);
    triangle(120, 35, 90,   5, 45, 110, framebuffer, white);
    triangle(115, 83, 80,  90, 85, 120, framebuffer, green);
    framebuffer.write_tga_file("triangle_rasterization.tga");
    return 0;
}