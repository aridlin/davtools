#include "common.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct GrayImage {
    int width, height;
    std::vector<std::uint8_t> pixels;
};
GrayImage read_gray(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    auto token = [&]() {
        std::string value;
        while (stream >> value) {
            if (value[0] != '#') return value;
            std::getline(stream, value);
        }
        throw std::runtime_error("Invalid grayscale image");
    };
    const auto magic = token();
    const int width = std::stoi(token()), height = std::stoi(token());
    const int maxval = std::stoi(token());
    if ((magic != "P5" && magic != "P2") || width < 1 || height < 1 || maxval != 255 ||
        static_cast<std::uint64_t>(width) * height > 100000000)
        throw std::runtime_error("Unsupported grayscale image dimensions or depth");
    GrayImage image{width, height, std::vector<std::uint8_t>(static_cast<std::size_t>(width)*height)};
    if (magic == "P5") {
        stream.get();
        stream.read(reinterpret_cast<char*>(image.pixels.data()), image.pixels.size());
        if (!stream) throw std::runtime_error("Truncated grayscale image");
    } else for (auto& p : image.pixels) p = static_cast<std::uint8_t>(std::stoi(token()));
    return image;
}

void halftone(GrayImage& image, int size, int density) {
    const auto source = image.pixels;
    std::fill(image.pixels.begin(), image.pixels.end(), 255);
    // Each cell is measured once, then rendered as ONE circular ink dot.
    // Coverage at the circle boundary is antialiased in output-pixel coordinates.
    // Dots can overlap in shadows, just as on a printed halftone screen.
    for (int top=0; top<image.height; top+=size) for (int left=0; left<image.width; left+=size) {
        const int right=std::min(left+size,image.width), bottom=std::min(top+size,image.height);
        double sum=0;
        for (int y=top; y<bottom; ++y) for (int x=left; x<right; ++x)
            sum += source[static_cast<std::size_t>(y)*image.width+x];
        const double gray=sum/((right-left)*(bottom-top)*255.0);
        const double ink=1.0-std::pow(gray,density/50.0);
        // Invert the area of a disk clipped to its square cell, including the
        // overlapping-dot range. This preserves tone instead of over-darkening it.
        double lo=0, hi=std::sqrt(0.5);
        constexpr double pi=3.14159265358979323846;
        for (int i=0; i<24; ++i) {
            const double r=(lo+hi)*0.5;
            double area=pi*r*r;
            if (r>0.5) area-=4*(r*r*std::acos(0.5/r)-0.5*std::sqrt(r*r-0.25));
            if (area<ink) lo=r; else hi=r;
        }
        const double radius=(lo+hi)*0.5*size;
        if (ink<1e-9) continue;
        const double cx=left+size/2.0, cy=top+size/2.0;
        const int x0=std::max(0,static_cast<int>(std::floor(cx-radius-1)));
        const int x1=std::min(image.width,static_cast<int>(std::ceil(cx+radius+1)));
        const int y0=std::max(0,static_cast<int>(std::floor(cy-radius-1)));
        const int y1=std::min(image.height,static_cast<int>(std::ceil(cy+radius+1)));
        for (int y=y0; y<y1; ++y) for (int x=x0; x<x1; ++x) {
            const double dx=x+0.5-cx, dy=y+0.5-cy;
            const double coverage=std::clamp(radius+0.5-std::sqrt(dx*dx+dy*dy),0.0,1.0);
            auto& p=image.pixels[static_cast<std::size_t>(y)*image.width+x];
            p=std::min(p,static_cast<std::uint8_t>(std::lround(255*(1-coverage))));
        }
    }
}

void diffuse(GrayImage& image, int method, int tone, int grain) {
    const int w=(image.width+grain-1)/grain, h=(image.height+grain-1)/grain;
    std::vector<float> values(static_cast<std::size_t>(w)*h);
    for (int y=0; y<h; ++y) for (int x=0; x<w; ++x) {
        double sum=0; int count=0;
        for (int yy=y*grain; yy<std::min((y+1)*grain,image.height); ++yy)
            for (int xx=x*grain; xx<std::min((x+1)*grain,image.width); ++xx) {
                sum+=image.pixels[static_cast<std::size_t>(yy)*image.width+xx]; ++count;
            }
        values[static_cast<std::size_t>(y)*w+x]=255*std::pow(sum/(count*255),tone/50.0);
    }
    auto add=[&](int x,int y,float e) {
        if (x>=0 && x<w && y>=0 && y<h) values[static_cast<std::size_t>(y)*w+x]+=e;
    };
    for (int y=0; y<h; ++y) {
        const int dir=y%2 ? -1 : 1;
        for (int step=0; step<w; ++step) {
            const int x=dir==1 ? step : w-1-step;
            auto& v=values[static_cast<std::size_t>(y)*w+x];
            const float output=v>=127.5f ? 255 : 0, error=v-output;
            v=output;
            if (method==2) {
                for (const auto& offset : {std::pair{dir,0}, {2*dir,0}, {-dir,1}, {0,1}, {dir,1}, {0,2}})
                    add(x+offset.first,y+offset.second,error/8);
            } else {
                add(x+dir,y,error*7/16); add(x-dir,y+1,error*3/16);
                add(x,y+1,error*5/16); add(x+dir,y+1,error/16);
            }
        }
    }
    for (int y=0; y<image.height; ++y) for (int x=0; x<image.width; ++x)
        image.pixels[static_cast<std::size_t>(y)*image.width+x]=static_cast<std::uint8_t>(values[static_cast<std::size_t>(y/grain)*w+x/grain]);
}

std::vector<OutputArtifact> monochrome(const std::string& name,
    const std::vector<std::uint8_t>& input, const std::string& mode, int size, int density, int method=1)
{
    conv::TempDir tmp("conv-monochrome-");
    const auto in=tmp.path()/"input", gray=tmp.path()/"gray.pgm";
    const auto stem=conv::basename_no_ext(std::filesystem::path(name.empty()?"input.png":name).filename().string());
    const auto out=tmp.path()/(stem+"_"+mode+".png");
    const std::string cli=conv::program_exists("magick")?"magick":"convert";
    conv::write_file_bytes(in,input);
    conv::require_success(conv::run_process({cli,in.string()+"[0]","-background","white","-alpha","remove",
        "-alpha","off","-colorspace","Gray","-depth","8",gray.string()}),cli);
    auto image=read_gray(gray);
    if (mode=="halftone") halftone(image,size,density);
    else if (mode=="dither") diffuse(image,method,density,size);
    else {
        for (int y=0; y<image.height; ++y) for (int x=0; x<image.width; ++x) {
            int rank=0;
            for (int bit=0; (1<<bit)<size; ++bit)
                rank=4*rank+2*(((x>>bit)^(y>>bit))&1)+((y>>bit)&1);
            auto& p=image.pixels[static_cast<std::size_t>(y)*image.width+x];
            p=(1-p/255.0)>(rank+0.5)/(size*size)?0:255;
        }
    }
    std::ofstream result(gray,std::ios::binary|std::ios::trunc);
    result << "P5\n" << image.width << ' ' << image.height << "\n255\n";
    result.write(reinterpret_cast<const char*>(image.pixels.data()),image.pixels.size());
    result.close();
    conv::require_success(conv::run_process({cli,gray.string(),"-strip",out.string()}),cli);
    return {conv::make_artifact_from_file(out)};
}
}
std::vector<OutputArtifact> convert_dither(const std::string& n,const std::vector<std::uint8_t>& i,int method,int tone,int grain) {
    return monochrome(n,i,"dither",std::clamp(grain,1,8),std::clamp(tone,1,100),method);
}
std::vector<OutputArtifact> convert_halftone(const std::string& n,const std::vector<std::uint8_t>& i,int density,int size) {
    return monochrome(n,i,"halftone",std::clamp(size,2,64),std::clamp(density,1,100));
}
std::vector<OutputArtifact> convert_bayer(const std::string& n,const std::vector<std::uint8_t>& i,int size) {
    if (size!=2 && size!=4 && size!=8 && size!=16) throw std::runtime_error("Bayer grid must be 2, 4, 8 or 16");
    return monochrome(n,i,"bayer",size,50);
}
