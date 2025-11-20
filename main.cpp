#include "headers/rtweekend.h"

#include "headers/camera.h"
#include "headers/hittable_list.h"
#include "headers/material.h"
#include "headers/quad.h"
#include "headers/sphere.h"

void cornell_box(int image_width, int samples_per_pixel, int max_depth, const std::string& output_file = "", const std::string& denoise_mode = "") {
    hittable_list world;

    auto red   = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light = make_shared<diffuse_light>(color(15, 15, 15));

    // Cornell box sides
    world.add(make_shared<quad>(point3(555,0,0), vec3(0,0,555), vec3(0,555,0), green));
    world.add(make_shared<quad>(point3(0,0,555), vec3(0,0,-555), vec3(0,555,0), red));
    world.add(make_shared<quad>(point3(0,555,0), vec3(555,0,0), vec3(0,0,555), white));
    world.add(make_shared<quad>(point3(0,0,555), vec3(555,0,0), vec3(0,0,-555), white));
    world.add(make_shared<quad>(point3(555,0,555), vec3(-555,0,0), vec3(0,555,0), white));

    // Light
    world.add(make_shared<quad>(point3(213,554,227), vec3(130,0,0), vec3(0,0,105), light));

    // Box
    shared_ptr<hittable> box1 = box(point3(0,0,0), point3(165,330,165), white);
    box1 = make_shared<rotate_y>(box1, 15);
    box1 = make_shared<translate>(box1, vec3(265,0,295));
    world.add(box1);

    // Glass Sphere
    auto glass = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(190,90,190), 90, glass));

    // Light Sources for importance sampling
    auto empty_material = shared_ptr<material>();
    hittable_list lights;
    lights.add(
        make_shared<quad>(point3(343,554,332), vec3(-130,0,0), vec3(0,0,-105), empty_material));
    // Add sphere for better ray distribution in importance sampling
    lights.add(make_shared<sphere>(point3(190, 90, 190), 90, empty_material));

    camera cam;

    cam.aspect_ratio      = 1.0;
    cam.image_width       = image_width;
    cam.samples_per_pixel = samples_per_pixel;
    cam.max_depth         = max_depth;
    cam.background        = color(0,0,0);

    cam.vfov     = 40;
    cam.lookfrom = point3(278, 278, -800);
    cam.lookat   = point3(278, 278, 0);
    cam.vup      = vec3(0, 1, 0);

    cam.defocus_angle = 0;
    
    if (!denoise_mode.empty()) {
        cam.denoise = true;
        cam.denoise_mode = denoise_mode;
    }

    cam.render_to_file(output_file, world, lights);
}

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " [quality] [output_file.png] [--denoise MODE]\n"
              << "Quality presets: draft, low, medium, high, ultra (default=medium)\n"
              << "  draft:  400x400, 50 samples, depth 8\n"
              << "  low:    800x800, 150 samples, depth 20\n"
              << "  medium: 1200x1200, 500 samples, depth 50\n"
              << "  high:   1920x1920, 1000 samples, depth 80\n"
              << "  ultra:  2560x2560, 4000 samples, depth 200\n"
              << "Output file: PNG filename (optional, outputs PPM to stdout if omitted)\n"
              << "Denoising: --denoise bilateral|median|fast (optional, post-processes final image)\n"
              << "Examples:\n"
              << "  " << program_name << " high cornell.png\n"
              << "  " << program_name << " medium cornell.png --denoise bilateral\n"
              << "  " << program_name << " draft --denoise fast\n";
}

void get_quality_settings(const std::string& quality, int& width, int& samples, int& depth) {
    if (quality == "draft") {
        width = 400; samples = 50; depth = 8;
    } else if (quality == "low") {
        width = 800; samples = 150; depth = 20;
    } else if (quality == "high") {
        width = 1920; samples = 1000; depth = 80;
    } else if (quality == "ultra") {
        width = 2560; samples = 4000; depth = 200;
    } else { // default: medium
        width = 1200; samples = 500; depth = 50;
    }
}

int main(int argc, char* argv[]) {
    int width = 1200;
    int samples = 250;
    int depth = 30;
    std::string quality = "medium";
    std::string output_file;
    std::string denoise_mode;

    if (argc > 1) {
        if (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
            print_usage(argv[0]);
            return 0;
        }
        quality = argv[1];
        get_quality_settings(quality, width, samples, depth);
    }
    if (argc > 2) output_file = argv[2];
    
    // Parse denoising option
    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--denoise" && i + 1 < argc) {
            denoise_mode = argv[++i];
        }
    }

    std::cerr << "Cornell Box [" << quality << "] (" << width << "x" << width 
              << ", " << samples << " samples, depth " << depth << ")\n";
    if (!output_file.empty()) {
        std::cerr << "Output: " << output_file << "\n";
    }
    if (!denoise_mode.empty()) {
        std::cerr << "Denoising: " << denoise_mode << "\n";
    }

    cornell_box(width, samples, depth, output_file, denoise_mode);
    return 0;
}