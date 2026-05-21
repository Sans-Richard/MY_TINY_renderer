#include"tgaimage.h"
#include<cmath>
#include"geometry.h"
#include"model.h"
#include<tuple>


constexpr int width = 800;
constexpr int height = 800;

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

std::tuple<int,int> project(vec3 v) { //投影函数，摄像机位于正中心，obj文件中的坐标范围为[-1,1],定义域：向量v，值域：屏幕坐标(x,y)
    return { (v.x + 1.) *  width/2,  //add1 is to shift the range from [-1,1] to [0,2], then scale it to fit the screen width
             (v.y + 1.) * height/2 }; //same for height
}
int main(int argc,char** argv){//命令行：.\\wireframe_render.exe a.obj
    if(argc!=2){//检查model是否正确加载
        std::cerr<<"Usage: "<<argv[0]<<" obj/model.obj"<<std::endl;
        return 1;
    }
    Model model(argv[1]);
    TGAImage framebuffer(width, height, TGAImage::RGB);

    for(int i=0; i<model.nfaces(); i++){//遍历所有三角形
        auto [ax, ay] = project(model.vert(i, 0));//获取第i个三角形的第0个顶点，并投影到屏幕坐标
        auto [bx, by] = project(model.vert(i, 1));//同上，获取第1个顶点
        auto [cx, cy] = project(model.vert(i, 2));//同上，获取第2个顶点
        TGAColor color={(uint8_t)(std::rand()%256), (uint8_t)(std::rand()%256), (uint8_t)(std::rand()%256), 255};//随机颜色
        line(ax, ay, bx, by, framebuffer, color);//画线连接第0和第1个顶点
        line(bx, by, cx, cy, framebuffer, color);//画线连接第1和第2个顶点
        line(cx, cy, ax, ay, framebuffer, color);//画线连接第2和第0个顶点
    }

    for(int i=0; i<model.nverts(); i++){//遍历所有顶点
        vec3 v=model.vert(i);//获取第i个顶点
        auto [x, y] = project(v);//将顶点投影到屏幕坐标
        framebuffer.set(x, y, white);//在屏幕上设置该坐标为白色
    }
    
    framebuffer.write_tga_file("wireframe_render.tga");
    return 0;
}