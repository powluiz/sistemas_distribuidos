#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

#define MAX_ITERATIONS 1000
#define IMG_WIDTH 1920
#define IMG_HEIGHT 1080

typedef struct ComplexNumber {
    double real, img;
} ComplexNumber;

unsigned char *calcMandelbrot() {
    unsigned char *canvas =
        new unsigned char[IMG_WIDTH * IMG_HEIGHT * sizeof(unsigned char)];

    for (int yIterator = 0; yIterator < IMG_HEIGHT; yIterator++) {
        for (int xIterator = 0; xIterator < IMG_WIDTH; xIterator++) {
            ComplexNumber z = {0.0, 0.0};
            double iterationCount = MAX_ITERATIONS;

            double xComplexPos =
                ((xIterator - (IMG_WIDTH / 2.0)) * 4.0 / IMG_WIDTH);
            double yComplexPos =
                (yIterator - (IMG_HEIGHT / 2.0)) * 4.0 / IMG_WIDTH;

            for (int iterations = 0; iterations < MAX_ITERATIONS;
                 iterations++) {
                double real2 = z.real * z.real;
                double img2 = z.img * z.img;
                bool isInsideSet = (real2 + img2) <= 4.0;

                if (!isInsideSet) {
                    iterationCount = iterations;
                    break;
                }

                z.img = 2 * z.real * z.img + yComplexPos;
                z.real = real2 - img2 + xComplexPos;
            }

            unsigned char pixelColor =
                (unsigned char)(255 *
                                (iterationCount / (double)MAX_ITERATIONS));
            canvas[yIterator * IMG_WIDTH + xIterator] = pixelColor;
        }
    }

    return canvas;
}

int main(int argc, char **argv) {
    unsigned char *canvas = calcMandelbrot();
    string filename = "mandelbrot-canvas.pgm";

    ofstream output_file(filename.c_str(), ios::binary);
    if (output_file.is_open()) {
        output_file << "P5\n" << IMG_WIDTH << " " << IMG_HEIGHT << "\n255\n";
        output_file.write(reinterpret_cast<char *>(canvas),
                          IMG_WIDTH * IMG_HEIGHT * sizeof(unsigned char));
        output_file.close();

        cout << "The file "
             << "\033[1;32m" << filename << "\033[0m"
             << " was created successfully." << endl;
    } else {
        cout << "Unable to open the file." << endl;
    }

    delete (canvas);

    return 0;
}