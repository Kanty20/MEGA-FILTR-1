#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "stb_image.h"
#include "stb_image_write.h"
#include <time.h>
#include <string.h>

int compare(const void *a, const void *b) {
    return (*(unsigned char *)a - *(unsigned char *)b);
}


// dla szarości
void medianowowowow(unsigned char *input, unsigned char *output, int width, int height, int window_size) {
    int half = window_size / 2;
    int window_size_total = window_size * window_size;  //dynamiczny malloc()
    unsigned char *window = malloc(window_size_total * sizeof(unsigned char));

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int count = 0;

            for (int dy = -half; dy <= half; dy++) {
                for (int dx = -half; dx <= half; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx < 0) nx = 0;
                    if (ny < 0) ny = 0;
                    if (nx >= width) nx = width - 1;
                    if (ny >= height) ny = height - 1;

                    window[count++] = input[ny * width + nx];
                }
            }

            qsort(window, count, sizeof(unsigned char), compare);
            output[y * width + x] = window[count / 2];
        }
    }
    free(window);
}

void maska_gausaaaaaa(double *maska, int size, double sigma) {
    int half = size / 2;
    double sum = 0.0;

    for (int y = -half; y <= half; y++) {
        for (int x = -half; x <= half; x++) {
            double value = exp(-(x*x + y*y) / (2 * sigma * sigma));
            maska[(y + half) * size + (x + half)] = value;
            sum += value;
        }
    }

    // suma wag = 1
    for (int i = 0; i < size * size; i++) {
        maska[i] /= sum;
    }
}

void gaussssssss(unsigned char *input, unsigned char *output, int width, int height, int size, double sigma) {
    int half = size / 2;
    double *maska = malloc(size * size * sizeof(double));
    maska_gausaaaaaa(maska, size, sigma);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double sum = 0.0;

            for (int dy = -half; dy <= half; dy++) {
                for (int dx = -half; dx <= half; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx < 0) nx = 0;
                    if (ny < 0) ny = 0;
                    if (nx >= width) nx = width - 1;
                    if (ny >= height) ny = height - 1;

                    double weight = maska[(dy + half) * size + (dx + half)];
                    sum += input[ny * width + nx] * weight;
                }
            }

            output[y * width + x] = (unsigned char)(sum + 0.5); // zaokrąglenie
        }
    }

    free(maska);
}

void odchylenie_standardowe(unsigned char *input, unsigned char *output, int width, int height, int window_size) {
    int half = window_size / 2;
    int window_len = window_size * window_size;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double sum = 0.0;

            // średnia
            for (int dy = -half; dy <= half; dy++) {
                for (int dx = -half; dx <= half; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;

                    // sprawdzanie granic
                    if (nx < 0) nx = 0;
                    if (ny < 0) ny = 0;
                    if (nx >= width) nx = width - 1;
                    if (ny >= height) ny = height - 1;

                    sum += input[ny * width + nx];
                }
            }

            double mean = sum / window_len;

            // wariancja i potem pierwiastek
            double sq_diff_sum = 0.0;

            for (int dy = -half; dy <= half; dy++) {
                for (int dx = -half; dx <= half; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx < 0) nx = 0;
                    if (ny < 0) ny = 0;
                    if (nx >= width) nx = width - 1;
                    if (ny >= height) ny = height - 1;

                    double val = input[ny * width + nx];
                    sq_diff_sum += (val - mean) * (val - mean);
                }
            }

            double std = sqrt(sq_diff_sum / window_len);

            if (std > 255.0) std = 255.0;
            output[y * width + x] = (unsigned char)(std + 0.5); // zaokrąglenie
        }
    }
}

void filtr_wienera(unsigned char *input, unsigned char *output, int width, int height, int window_size, double noise_variance) {
    int half = window_size / 2;
    int window_len = window_size * window_size;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double sum = 0.0;
            double sum_sq = 0.0;

            for (int dy = -half; dy <= half; dy++) {
                for (int dx = -half; dx <= half; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx < 0) nx = 0;
                    if (ny < 0) ny = 0;
                    if (nx >= width) nx = width - 1;
                    if (ny >= height) ny = height - 1;

                    double val = input[ny * width + nx];
                    sum += val;
                    sum_sq += val * val;
                }
            }

            double mean = sum / window_len;
            double var = (sum_sq / window_len) - (mean * mean);

            // unika dzielenia przez 0
            double pixel = input[y * width + x];
            double coeff = (var > 0) ? (var - noise_variance) / var : 0;

            if (coeff < 0) coeff = 0; // jeżeli szum > lokalna wariancja

            double result = mean + coeff * (pixel - mean);
            if (result < 0) result = 0;
            if (result > 255) result = 255;

            output[y * width + x] = (unsigned char)(result + 0.5);
        }
    }
}

unsigned char* padarray(unsigned char *input, int width, int height, int pad, int *new_width, int *new_height) {
    *new_width = width + 2 * pad;
    *new_height = height + 2 * pad;

    unsigned char *padded = malloc((*new_width) * (*new_height) * sizeof(unsigned char));

    for (int y = 0; y < *new_height; y++) {
        for (int x = 0; x < *new_width; x++) {
            int src_x = x - pad;
            int src_y = y - pad;

            // ctrl c z granic jeśli poza zakresem
            if (src_x < 0) src_x = 0;
            if (src_y < 0) src_y = 0;
            if (src_x >= width) src_x = width - 1;
            if (src_y >= height) src_y = height - 1;

            padded[y * (*new_width) + x] = input[src_y * width + src_x];
        }
    }

    return padded;
}

unsigned char* crop(unsigned char *input, int width, int height,
                    int x0, int y0, int crop_width, int crop_height) {
    unsigned char *cropped = malloc(crop_width * crop_height * sizeof(unsigned char));

    for (int y = 0; y < crop_height; y++) {
        for (int x = 0; x < crop_width; x++) {
            int src_x = x0 + x;
            int src_y = y0 + y;

            // spr czy w granicach
            if (src_x >= 0 && src_x < width && src_y >= 0 && src_y < height) {
                cropped[y * crop_width + x] = input[src_y * width + src_x];
            } else {
                cropped[y * crop_width + x] = 0;  // poza obrazem = 0
            }
        }
    }

    return cropped;
}


typedef void (*filter_fn)(
    unsigned char *in, unsigned char *out, int w, int h);

typedef struct {
    const char *name;
    filter_fn fn;
} Filter;

void apply_median(unsigned char *in, unsigned char *out, int w, int h){
    medianowowowow(in, out, w, h, 3);
}
void apply_gauss(unsigned char *in, unsigned char *out, int w, int h){
    gaussssssss(in, out, w, h, 5, 1.0);
}
void apply_std(unsigned char *in, unsigned char *out, int w, int h){
    odchylenie_standardowe(in, out, w, h, 7);
}
void apply_wiener(unsigned char *in, unsigned char *out, int w, int h){
    filtr_wienera(in, out, w, h, 5, 100.0);
}


//---------------------------------------------------------------------------------------------------------------------


double sredni_blad_kwadratowy(unsigned char *img1, unsigned char *img2, int width, int height) {
    double mse = 0.0;
    int total_pixels = width * height;

    for (int i = 0; i < total_pixels; i++) {
        int diff = img1[i] - img2[i];
        mse += diff * diff;
    }

    return mse / total_pixels;
}

double PSNR(unsigned char *img1, unsigned char *img2, int width, int height) {
    double mse = sredni_blad_kwadratowy(img1, img2, width, height);
    if (mse == 0) return INFINITY; // obrazy identyczne
    double max_val = 255.0;
    return 10.0 * log10((max_val * max_val) / mse);
}

int main() {
    int width, height, channels;
    int new_w, new_h;
    int x0 = 100, y0 = 100;
    int crop_w = 200, crop_h = 200;
    unsigned char *original = stbi_load("oryginal.png",&width,&height,&channels,1);
    unsigned char *input    = stbi_load("1.png",&width,&height,&channels,1);
    unsigned char *current  = malloc(width*height);
    unsigned char *temp     = malloc(width*height);

    // wczytanie obrazu szarości
    unsigned char *input = stbi_load("1.png", &width, &height, &channels, 1);
    if (input == NULL) {
        printf("gówno z dupy, nie udało się wczytac 1.png ;/////////////////\n");
        return 1;
    }

    printf("wzięto obraz: %d x %d\n", width, height);

    unsigned char *output = malloc(width * height); //bufor wyjściowy
    unsigned char *gauss_output = malloc(width * height); 
    unsigned char *odchylenie_output = malloc(width * height);
    unsigned char *wiener_output = malloc(width * height);
    unsigned char *padarray_input = padarray(input, width, height, 2, &new_w, &new_h);
    unsigned char *cropped = crop(input, width, height, x0, y0, crop_w, crop_h);


    clock_t start = clock();

    medianowowowow(input, output, width, height, 3);
    gaussssssss(output, gauss_output, width, height, 5, 1.0);
    odchylenie_standardowe(gauss_output, odchylenie_output, width, height, 7); // np. 7x7 okno
    filtr_wienera(odchylenie_output, wiener_output, width, height, 5, 100.0);


    clock_t end = clock();
    double elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;


    printf("czas: %.6f s.\n", elapsed_time);

    stbi_write_png("odfiltrowany_medianowo.png", width, height, 1, output, width);  
    printf("zapisano medianowo\n");

    stbi_write_png("odfiltrowany_gaussowo.png", width, height, 1, gauss_output, width);  
    printf("zapisano gaussowo\n");

    stbi_write_png("odfiltrowany_standardowo.png", width, height, 1, odchylenie_output, width);
    printf("zapisano z f. odchylenia standardowego\n");

    stbi_write_png("odfiltrowany wienerowo.png", width, height, 1, wiener_output, width);
    printf("zapisano wienerowo\n");

    stbi_write_png("odfiltrowany cropped.png", crop_w, crop_h, 1, cropped, crop_w);
    printf("zapisano cropped\n");


    unsigned char *oryginal = stbi_load("oryginal.png", &width, &height, &channels, 1);   
    if (oryginal == NULL) {
        printf("gówno z dupy nie ma oryginalu.\n");
        return 1;
    }

    double psnr2 = PSNR(oryginal, input, width, height); 
    double psnr3 = PSNR(oryginal, output, width, height);
    double psnr4 = PSNR(oryginal, gauss_output, width, height);
    double psnr5 = PSNR(oryginal, odchylenie_output, width, height);
    double psnr6 = PSNR(oryginal, wiener_output, width, height);
    double psnr7 = PSNR(oryginal, cropped, width, height);


    printf("PSNR2 = %.2f dB\n", psnr2);
    printf("PSNR3 = %.2f dB\n", psnr3);
    printf("PSNR4 = %.2f dB\n", psnr4);
    printf("PSNR5 = %.2f dB\n", psnr5);
    printf("PSNR6 = %.2f dB\n", psnr6);
    printf("PSNR7 = %.2f dB\n", psnr7);


    stbi_image_free(input);
    free(output);
    stbi_image_free(oryginal);
    free(gauss_output);
    free(odchylenie_output);
    free(wiener_output);
    free(padarray_input);
    free(cropped);

    return 0;
}
