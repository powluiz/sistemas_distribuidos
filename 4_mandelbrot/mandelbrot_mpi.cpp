#include <mpi.h>

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

unsigned char *calcMandelbrot(int startY, int endY) {
    // Cada processo vai calcular um bloco de linhas da imagem.
    unsigned char *canvas = new unsigned char[IMG_WIDTH * (endY - startY)];

    for (int yIterator = startY; yIterator < endY; yIterator++) {
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
            canvas[(yIterator - startY) * IMG_WIDTH + xIterator] = pixelColor;
        }
    }

    return canvas;
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // Divisão das linhas da imagem entre os processos
    int linesPerProcess = IMG_HEIGHT / world_size;
    int startY = world_rank * linesPerProcess;
    int endY =
        (world_rank == world_size - 1) ? IMG_HEIGHT : startY + linesPerProcess;

    // Cada processo calcula sua parte do Mandelbrot
    unsigned char *local_canvas = calcMandelbrot(startY, endY);

    // Processo 0 recebe os resultados de todos os outros processos
    unsigned char *global_canvas = nullptr;
    if (world_rank == 0) {
        global_canvas = new unsigned char[IMG_WIDTH * IMG_HEIGHT];
    }

    // Recolher os resultados de todos os processos
    MPI_Gather(local_canvas, IMG_WIDTH * (endY - startY), MPI_UNSIGNED_CHAR,
               global_canvas, IMG_WIDTH * (endY - startY), MPI_UNSIGNED_CHAR, 0,
               MPI_COMM_WORLD);

    // Processo 0 salva a imagem
    if (world_rank == 0) {
        string filename = "mandelbrot-canvas.pgm";
        ofstream output_file(filename.c_str(), ios::binary);
        if (output_file.is_open()) {
            output_file << "P5\n"
                        << IMG_WIDTH << " " << IMG_HEIGHT << "\n255\n";
            output_file.write(reinterpret_cast<char *>(global_canvas),
                              IMG_WIDTH * IMG_HEIGHT * sizeof(unsigned char));
            output_file.close();

            cout << "The file "
                 << "\033[1;32m" << filename << "\033[0m"
                 << " was created successfully." << endl;

            delete[] global_canvas;
        } else {
            cout << "Unable to open the file." << endl;
        }
    }

    delete[] local_canvas;
    MPI_Finalize();

    return 0;
}
