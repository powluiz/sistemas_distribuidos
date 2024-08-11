#include <dirent.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cmath>
#include <iostream>
#include <vector>

// jpeg lib
#include <jpeglib.h>

using namespace std;

static inline int read_file(const char *filepath, int *width, int *height,
                            int *channels, unsigned char **image) {
    FILE *input_file;
    if ((input_file = fopen(filepath, "rb")) == NULL) {
        fprintf(stderr, "can't open %s\n", filepath);
        return 0;
    }

    struct jpeg_error_mgr img_error;
    struct jpeg_decompress_struct compression_info;
    compression_info.err = jpeg_std_error(&img_error);
    jpeg_create_decompress(&compression_info);
    jpeg_stdio_src(&compression_info, input_file);
    jpeg_read_header(&compression_info, TRUE);
    jpeg_start_decompress(&compression_info);

    *width = compression_info.output_width;
    *height = compression_info.output_height;
    *channels = compression_info.num_components;
    *image = (unsigned char *)malloc(*width * *height * *channels *
                                     sizeof(unsigned char));

    JSAMPROW rowptr[1];
    int row_stride = *width * *channels;

    while (compression_info.output_scanline < compression_info.output_height) {
        rowptr[0] = *image + row_stride * compression_info.output_scanline;
        jpeg_read_scanlines(&compression_info, rowptr, 1);
    }

    jpeg_finish_decompress(&compression_info);
    jpeg_destroy_decompress(&compression_info);
    fclose(input_file);
    return 1;
}

static inline void write_file(const char *filepath, int width, int height,
                              int channels, unsigned char *image) {
    int quality = 100;
    FILE *output_file;

    struct stat st = {0};
    if (stat("./output", &st) == -1) {
        mkdir("./output", 0700);
    }

    if ((output_file = fopen(filepath, "wb")) == NULL) {
        fprintf(stderr, "can't open %s\n", filepath);
        exit(1);
    }

    struct jpeg_error_mgr jerr;
    struct jpeg_compress_struct cinfo;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, output_file);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = channels;
    cinfo.in_color_space = (channels == 1) ? JCS_GRAYSCALE : JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);
    JSAMPROW rowptr[1];
    int row_stride = width * channels;
    while (cinfo.next_scanline < cinfo.image_height) {
        rowptr[0] = &image[cinfo.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo, rowptr, 1);
    }
    jpeg_finish_compress(&cinfo);

    fclose(output_file);
    jpeg_destroy_compress(&cinfo);
}

void process_image(int width, int height, int &channels,
                   unsigned char *&image) {
    if (channels == 3) {
        unsigned char *grayscale_image =
            (unsigned char *)malloc(width * height * sizeof(unsigned char));
        channels = 1;

// Paralelizando a conversão para escala de cinza
#pragma omp parallel for collapse(2)
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                grayscale_image[i * width + j] =
                    (image[i * width * 3 + j * 3] +
                     image[i * width * 3 + j * 3 + 1] +
                     image[i * width * 3 + j * 3 + 2]) /
                    3;
            }
        }
        free(image);
        image = grayscale_image;
    }

    vector<vector<int>> img2d(height, vector<int>(width));
    vector<vector<int>> img2dhororg(height, vector<int>(width));
    vector<vector<int>> img2dverorg(height, vector<int>(width));
    vector<vector<int>> img2dmag(height, vector<int>(width));

#pragma omp parallel for collapse(2)
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            img2d[i][j] = image[i * width + j];
        }
    }

    int max = -200, min = 2000;
#pragma omp parallel for reduction(max : max) reduction(min : min)
    for (int i = 1; i < height - 1; i++) {
        for (int j = 1; j < width - 1; j++) {
            int curr = img2d[i - 1][j - 1] + 2 * img2d[i - 1][j] +
                       img2d[i - 1][j + 1] - img2d[i + 1][j - 1] -
                       2 * img2d[i + 1][j] - img2d[i + 1][j + 1];
            img2dhororg[i][j] = curr;
            if (curr > max) max = curr;
            if (curr < min) min = curr;
        }
    }

    max = -200;
    min = 2000;
#pragma omp parallel for reduction(max : max) reduction(min : min)
    for (int i = 1; i < height - 1; i++) {
        for (int j = 1; j < width - 1; j++) {
            int curr = img2d[i - 1][j - 1] + 2 * img2d[i][j - 1] +
                       img2d[i + 1][j - 1] - img2d[i - 1][j + 1] -
                       2 * img2d[i][j + 1] - img2d[i + 1][j + 1];
            img2dverorg[i][j] = curr;
            if (curr > max) max = curr;
            if (curr < min) min = curr;
        }
    }

    max = -200;
    min = 2000;
#pragma omp parallel for reduction(max : max) reduction(min : min)
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            img2dmag[i][j] =
                sqrt(pow(img2dhororg[i][j], 2) + pow(img2dverorg[i][j], 2));
            if (img2dmag[i][j] > max) max = img2dmag[i][j];
            if (img2dmag[i][j] < min) min = img2dmag[i][j];
        }
    }

    int diff = max - min;
#pragma omp parallel for collapse(2)
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            float normalized_value = (img2dmag[i][j] - min) / (diff * 1.0);
            image[i * width + j] =
                static_cast<unsigned char>(normalized_value * 255);
        }
    }
}

int main(int argc, char *argv[]) {
    double start, end;
    unsigned char *image;
    int width, height, channels;
    int num_of_threads, available_threads;

    char input_path[256], output_path[256];
    const char *base_input_path = "./input/";
    const char *base_output_path = "./output/";

    if (argc != 2) {
        cout << "Usage: ./sobel_parallel <num_of_threads>" << endl;
        return -1;
    }

    available_threads = omp_get_max_threads();
    num_of_threads = atoi(argv[1]);

    if (num_of_threads > available_threads) {
        cout << "Requested number of threads is more than available" << endl;
        return -1;
    }

    omp_set_num_threads(num_of_threads);
    start = omp_get_wtime();

    DIR *dir = opendir(base_input_path);
    if (!dir) {
        perror("Error while opening input directory");
        return -1;
    }

    vector<string> files;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            files.push_back(entry->d_name);
        }
    }
    closedir(dir);

// Paralelização do processamento de arquivos
#pragma omp parallel for private(image, width, height, channels, input_path, \
                                     output_path)
    for (int i = 0; i < files.size(); i++) {
        snprintf(input_path, sizeof(input_path), "%s%s", base_input_path,
                 files[i].c_str());
        snprintf(output_path, sizeof(output_path), "%s%s", base_output_path,
                 files[i].c_str());

        if (!read_file(input_path, &width, &height, &channels, &image)) {
            fprintf(stderr, "Failed to read %s\n", input_path);
            continue;
        }

        process_image(width, height, channels, image);
        write_file(output_path, width, height, channels, image);
        free(image);
    }

    end = omp_get_wtime();
    cout << "Time: " << end - start << endl;
    return 0;
}
