## Parallel Computing for the Mandelbrot with MPI


```bash

g++ mandelbrot_serial.cpp -o mandelbrot_serial.exe
./mandelbrot_serial.exe

mpicxx mandelbrot_mpi.cpp -o mandelbrot_mpi.exe
mpiexec -n 4 ./mandelbrot_mpi.exe
```