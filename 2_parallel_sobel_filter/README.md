# Parallel Sobel Edge Detection with C++ and OpenMP

The goal of this project is to compare the performance between the serial version and parallel versions (by varying the number of threads) when running Sobel Edge Detection on multiple images. For a more noticeable difference, you can duplicate images inside the ./input folder.


### To run this code, it's required to install **libjpeg**:

```bash
sudo apt-get install libjpeg-dev
```


## How to Run

### Serial:

```bash
# Serial Version
g++ sobel_filter_serial.cpp -o sobel_serial -ljpeg -fpermissive -fopenmp
./sobel_serial
```

### Parallel:

```bash
# Parallel Version
g++ sobel_filter_parallel.cpp -o sobel_parallel -ljpeg -fpermissive -fopenmp
./sobel_parallel <num_of_threads>
```
