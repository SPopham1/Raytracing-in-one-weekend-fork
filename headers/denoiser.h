#ifndef DENOISER_H
#define DENOISER_H

#include "rtweekend.h"
#include <vector>
#include <algorithm>
#include <cmath>

class denoiser {
public:
    // Bilateral filter for denoising
    // Parameters:
    //   - sigma_spatial: controls spatial extent (larger = more smoothing)
    //   - sigma_intensity: controls intensity threshold (larger = blurs across edges)
    static std::vector<color> bilateral_denoise(
        const std::vector<color>& image,
        int width,
        int height,
        double sigma_spatial = 2.0,
        double sigma_intensity = 0.1
    ) {
        std::vector<color> result(width * height);
        
        // Kernel radius
        int radius = static_cast<int>(std::ceil(sigma_spatial * 2.5));
        double spatial_factor = 1.0 / (2.0 * sigma_spatial * sigma_spatial);
        double intensity_factor = 1.0 / (2.0 * sigma_intensity * sigma_intensity);

        #pragma omp parallel for collapse(2)
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = y * width + x;
                color center = image[idx];
                
                color weighted_sum(0, 0, 0);
                double weight_sum = 0.0;

                // Apply bilateral filter kernel
                for (int ky = -radius; ky <= radius; ++ky) {
                    for (int kx = -radius; kx <= radius; ++kx) {
                        int nx = x + kx;
                        int ny = y + ky;
                        
                        // Boundary handling (clamp)
                        nx = std::clamp(nx, 0, width - 1);
                        ny = std::clamp(ny, 0, height - 1);
                        
                        int neighbor_idx = ny * width + nx;
                        color neighbor = image[neighbor_idx];
                        
                        // Spatial weight (Gaussian)
                        double spatial_dist = kx * kx + ky * ky;
                        double spatial_weight = std::exp(-spatial_dist * spatial_factor);
                        
                        // Intensity weight (Gaussian)
                        color diff = center - neighbor;
                        double intensity_dist = diff.x() * diff.x() + 
                                               diff.y() * diff.y() + 
                                               diff.z() * diff.z();
                        double intensity_weight = std::exp(-intensity_dist * intensity_factor);
                        
                        double combined_weight = spatial_weight * intensity_weight;
                        weighted_sum += neighbor * combined_weight;
                        weight_sum += combined_weight;
                    }
                }

                result[idx] = weighted_sum * (1.0 / weight_sum);
            }
        }
        
        return result;
    }

    // Faster edge-preserving box filter (for quick previews)
    static std::vector<color> fast_denoise(
        const std::vector<color>& image,
        int width,
        int height,
        int kernel_size = 3,
        double edge_threshold = 0.05
    ) {
        std::vector<color> result(width * height);
        int radius = kernel_size / 2;

        #pragma omp parallel for collapse(2)
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = y * width + x;
                color center = image[idx];
                
                color sum(0, 0, 0);
                int count = 0;

                for (int ky = -radius; ky <= radius; ++ky) {
                    for (int kx = -radius; kx <= radius; ++kx) {
                        int nx = x + kx;
                        int ny = y + ky;
                        
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            int neighbor_idx = ny * width + nx;
                            color neighbor = image[neighbor_idx];
                            
                            // Only average if color difference is below threshold
                            color diff = center - neighbor;
                            double color_diff = std::sqrt(
                                diff.x() * diff.x() + 
                                diff.y() * diff.y() + 
                                diff.z() * diff.z()
                            );
                            
                            if (color_diff < edge_threshold) {
                                sum += neighbor;
                                count++;
                            }
                        }
                    }
                }

                result[idx] = sum * (1.0 / count);
            }
        }
        
        return result;
    }

    // Median filter (very effective at removing noise while preserving edges)
    static std::vector<color> median_denoise(
        const std::vector<color>& image,
        int width,
        int height,
        int kernel_size = 3
    ) {
        std::vector<color> result(width * height);
        int radius = kernel_size / 2;

        #pragma omp parallel for collapse(2)
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = y * width + x;
                
                std::vector<double> r_values, g_values, b_values;

                for (int ky = -radius; ky <= radius; ++ky) {
                    for (int kx = -radius; kx <= radius; ++kx) {
                        int nx = x + kx;
                        int ny = y + ky;
                        
                        nx = std::clamp(nx, 0, width - 1);
                        ny = std::clamp(ny, 0, height - 1);
                        
                        int neighbor_idx = ny * width + nx;
                        color pixel = image[neighbor_idx];
                        r_values.push_back(pixel.x());
                        g_values.push_back(pixel.y());
                        b_values.push_back(pixel.z());
                    }
                }

                // Sort and find median for each channel
                std::sort(r_values.begin(), r_values.end());
                std::sort(g_values.begin(), g_values.end());
                std::sort(b_values.begin(), b_values.end());
                
                size_t mid = r_values.size() / 2;
                result[idx] = color(r_values[mid], g_values[mid], b_values[mid]);
            }
        }
        
        return result;
    }
};

#endif
