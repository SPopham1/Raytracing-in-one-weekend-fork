#include "headers/rtweekend.h"

#include "headers/bvh.h"
#include "headers/camera.h"
#include "headers/constant_medium.h"
#include "headers/hittable.h"
#include "headers/hittable_list.h"
#include "headers/material.h"
#include "headers/quad.h"
#include "headers/sphere.h"
#include "headers/texture.h"

// Forward declarations
void simple_scene(int image_width, int samples_per_pixel, int max_depth, const std::string& output_file = "", const std::string& denoise_mode = "");
void final_scene(int image_width, int samples_per_pixel, int max_depth, const std::string& output_file = "", const std::string& denoise_mode = "");

void simple_scene(int image_width, int samples_per_pixel, int max_depth, const std::string& output_file, const std::string& denoise_mode) {
    hittable_list world;
    auto ground = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, ground));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_double();
            point3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());
            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> sphere_material;
                if (choose_mat < 0.8) {
                    auto albedo = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                } else if (choose_mat < 0.95) {
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                } else {
                    sphere_material = make_shared<dielectric>(1.5);
                }
                world.add(make_shared<sphere>(center, 0.2, sphere_material));
            }
        }
    }

    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    hittable_list lights;
    lights.add(make_shared<sphere>(point3(0, 1, 0), 1.0, shared_ptr<material>()));

    camera cam;
    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = image_width;
    cam.samples_per_pixel = samples_per_pixel;
    cam.max_depth         = max_depth;
    cam.background        = color(0.7, 0.8, 1.0);

    cam.vfov     = 20;
    cam.lookfrom = point3(13, 2, 3);
    cam.lookat   = point3(0, 0, 0);
    cam.vup      = vec3(0, 1, 0);

    cam.defocus_angle = 0.6;
    cam.focus_dist    = 10.0;
    
    if (!denoise_mode.empty()) {
        cam.denoise = true;
        cam.denoise_mode = denoise_mode;
    }

    cam.render_to_file(output_file, world, lights);
}

void final_scene(int image_width, int samples_per_pixel, int max_depth, const std::string& output_file, const std::string& denoise_mode) {
    hittable_list boxes1;
    auto ground = make_shared<lambertian>(color(0.48, 0.83, 0.53));

    int boxes_per_side = 20;
    for (int i = 0; i < boxes_per_side; i++) {
        for (int j = 0; j < boxes_per_side; j++) {
            auto w = 100.0;
            auto x0 = -1000.0 + i*w;
            auto z0 = -1000.0 + j*w;
            auto y0 = 0.0;
            auto x1 = x0 + w;
            auto y1 = random_double(1,101);
            auto z1 = z0 + w;

            boxes1.add(box(point3(x0,y0,z0), point3(x1,y1,z1), ground));
        }
    }

    hittable_list world;
    world.add(make_shared<bvh_node>(boxes1));

    // Main light
    auto light = make_shared<diffuse_light>(color(7, 7, 7));
    world.add(make_shared<quad>(point3(123,554,147), vec3(300,0,0), vec3(0,0,265), light));

    // Moving sphere
    auto center1 = point3(400, 400, 200);
    auto center2 = center1 + vec3(30,0,0);
    auto sphere_material = make_shared<lambertian>(color(0.7, 0.3, 0.1));
    world.add(make_shared<sphere>(center1, center2, 50, sphere_material));

    // Glass & metal spheres
    world.add(make_shared<sphere>(point3(260, 150, 45), 50, make_shared<dielectric>(1.5)));
    world.add(make_shared<sphere>(
        point3(0, 150, 145), 50, make_shared<metal>(color(0.8, 0.8, 0.9), 1.0)
    ));

    // Constant medium around a glass sphere
    auto boundary = make_shared<sphere>(point3(360,150,145), 70, make_shared<dielectric>(1.5));
    world.add(boundary);
    world.add(make_shared<constant_medium>(boundary, 0.2, color(0.2, 0.4, 0.9)));

    // Large boundary for atmospheric effect
    boundary = make_shared<sphere>(point3(0,0,0), 5000, make_shared<dielectric>(1.5));
    world.add(make_shared<constant_medium>(boundary, .0001, color(1,1,1)));

    // Textured earth sphere
    auto emat = make_shared<lambertian>(make_shared<image_texture>("earthmap.jpg"));
    world.add(make_shared<sphere>(point3(400,200,400), 100, emat));

    // Perlin noise sphere
    auto pertext = make_shared<noise_texture>(0.2);
    world.add(make_shared<sphere>(point3(220,280,300), 80, make_shared<lambertian>(pertext)));

    // Cluster of small spheres
    hittable_list boxes2;
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    int ns = 1000;
    for (int j = 0; j < ns; j++) {
        boxes2.add(make_shared<sphere>(point3::random(0,165), 10, white));
    }
    world.add(make_shared<translate>(
        make_shared<rotate_y>(
            make_shared<bvh_node>(boxes2), 15),
            vec3(-100,270,395)
        )
    );

    // Lights list (for importance sampling)
    auto empty_material = shared_ptr<material>();
    hittable_list lights;
    lights.add(make_shared<quad>(point3(123,554,147), vec3(300,0,0), vec3(0,0,265), empty_material));

    camera cam;
    cam.aspect_ratio      = 1.0;
    cam.image_width       = image_width;
    cam.samples_per_pixel = samples_per_pixel;
    cam.max_depth         = max_depth;
    cam.background        = color(0,0,0);

    cam.vfov     = 40;
    cam.lookfrom = point3(478, 278, -600);
    cam.lookat   = point3(278, 278, 0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;
    
    if (!denoise_mode.empty()) {
        cam.denoise = true;
        cam.denoise_mode = denoise_mode;
    }

    cam.render_to_file(output_file, world, lights);
}

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " [scene] [quality] [output_file.png] [--denoise MODE]\n"
              << "Scenes: 1=simple, 2=final (default=2)\n"
              << "Quality presets: draft, low, medium, high, ultra (default=medium)\n"
              << "  draft:  400x400, 10 samples, depth 2 (instant preview)\n"
              << "  low:    800x800, 50 samples, depth 10 (30 sec)\n"
              << "  medium: 1200x1200, 250 samples, depth 30 (2-3 min)\n"
              << "  high:   1920x1920, 500 samples, depth 50 (5-10 min)\n"
              << "  ultra:  2560x2560, 2000 samples, depth 100 (30+ min)\n"
              << "Output file: PNG filename (optional, outputs PPM to stdout if omitted)\n"
              << "Denoising: --denoise bilateral|median|fast (optional, post-processes final image)\n"
              << "Examples:\n"
              << "  " << program_name << " 1 high output.png\n"
              << "  " << program_name << " 2 medium final.png --denoise bilateral\n"
              << "  " << program_name << " 1 draft --denoise fast\n";
}

void get_quality_settings(const std::string& quality, int& width, int& samples, int& depth) {
    if (quality == "draft") {
        width = 400; samples = 10; depth = 3;
    } else if (quality == "low") {
        width = 800; samples = 50; depth = 15;
    } else if (quality == "high") {
        width = 1920; samples = 500; depth = 60;
    } else if (quality == "ultra") {
        width = 2560; samples = 1000; depth = 150;
    } else { // default: medium
        width = 1200; samples = 250; depth = 40;
    }
}

int main(int argc, char* argv[]) {
    int scene = 2;
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
        scene = atoi(argv[1]);
    }
    if (argc > 2) {
        quality = argv[2];
        get_quality_settings(quality, width, samples, depth);
    }
    if (argc > 3) output_file = argv[3];
    
    // Parse denoising option
    for (int i = 4; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--denoise" && i + 1 < argc) {
            denoise_mode = argv[++i];
        }
    }

    std::cerr << "Scene " << scene << " [" << quality << "] (" << width << "x" << width 
              << ", " << samples << " samples, depth " << depth << ")\n";
    if (!output_file.empty()) {
        std::cerr << "Output: " << output_file << "\n";
    }
    if (!denoise_mode.empty()) {
        std::cerr << "Denoising: " << denoise_mode << "\n";
    }

    switch (scene) {
        case 1:  simple_scene(width, samples, depth, output_file, denoise_mode); break;
        case 2:  final_scene(width, samples, depth, output_file, denoise_mode); break;
        default: 
            std::cerr << "Unknown scene: " << scene << "\n";
            print_usage(argv[0]);
            return 1;
    }

    return 0;
}