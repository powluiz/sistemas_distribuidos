#include <stdio.h>
#include <stdlib.h>

#include <cmath>
#include <iostream>

// jpeg lib
#include <jpeglib.h>

using namespace std;

static inline int read_file(char *filepath, int *width, int *height,
                            int *channels, unsigned char *(image[])) {
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
    (void)jpeg_read_header(&compression_info, TRUE);
    (void)jpeg_start_decompress(&compression_info);

    *width = compression_info.output_width;
    *height = compression_info.output_height;
    *channels = compression_info.num_components;
    *image =
        (unsigned char *)malloc(*width * *height * *channels * sizeof(*image));

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

static inline void write_file(char *filepath, int width, int height,
                              int channels, unsigned char image[]) {
    int quality = 100;
    FILE *output_file;
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
    cinfo.in_color_space = channels == 1 ? JCS_GRAYSCALE : JCS_RGB;
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

void process_image(int width, int height, int &channels, unsigned char *&image,
                   char *output_path) {
    if (channels == 3) {
        unsigned char *grayscale_image;
        grayscale_image =
            (unsigned char *)malloc(width * height * channels * sizeof(image));
        channels = 1;
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
        write_file("./img/output_grayscale.jpg", width, height, channels,
                   image);
    }

    int img2d[height][width];

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            img2d[i][j] = image[i * width + j];
        }
    }
    int img2dhororg[height][width];
    int img2dverorg[height][width];
    int img2dmag[height][width];

    /// horizontal
    int max = -200, min = 2000;

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

    /// vertical:
    max = -200;
    min = 2000;

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

    /// magnitude
    max = -200;
    min = 2000;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            img2dmag[i][j] =
                sqrt(pow(img2dhororg[i][j], 2) + pow(img2dverorg[i][j], 2));
            if (img2dmag[i][j] > max) max = img2dmag[i][j];
            if (img2dmag[i][j] < min) min = img2dmag[i][j];
        }
    }

    int diff = max - min;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            float normalized_value = (img2dmag[i][j] - min) / (diff * 1.0);
            img2dmag[i][j] = normalized_value * 255;
            image[i * width + j] = img2dmag[i][j];
        }
    }

    write_file(output_path, width, height, channels, image);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf(
            "Correct usage: ./sobel ./img/sample.jpg "
            "./img/output.jpg\n");
        return 1;
    }

    unsigned char *image;
    int width, height, channels;
    char *input_path = argv[1];
    char *output_path = argv[2];

    read_file(input_path, &width, &height, &channels, &image);
    process_image(width, height, channels, image, output_path);
    free(image);

    return 0;
}
