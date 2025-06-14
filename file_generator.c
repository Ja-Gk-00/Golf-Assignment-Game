#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include <errno.h>
#include <sys/stat.h> 
#include <direct.h>  
#include <errno.h>
#include <windows.h> 

#define FILE_COUNT 25
#define MAX_POINTS 50
#define COORD_MAX 1000
#define MAX_PATH_LEN 260

typedef struct { double x, y; } Point;
char *strcat(char *dest, const char *src);

static void get_exe_dir(char *out, size_t len) {
    char buf[MAX_PATH];
    GetModuleFileNameA(NULL, buf, MAX_PATH);
    char *p = strrchr(buf, '\\');
    if (p) *p = '\0';
    strncpy(out, buf, len);
}

// Sprawdzamy wspolliniowosc 3 punktow
bool collinear(Point a, Point b, Point c) {
    double det = (b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x);
    return fabs(det) < 1e-6;
}

int ensure_directory_windows(const char *path) {
    struct _stat st;

    if (_stat(path, &st) == 0) {
        if (st.st_mode & _S_IFDIR) {
            return 0; 
        } else {
            errno = ENOTDIR;
            return -1;
        }
    }

    if (errno != ENOENT) {
        return -1;
    }
    if (_mkdir(path) == 0) {
        return 0;
    }
    return -1;
}

int main(int argc, char *argv[]) {
    srand((unsigned)time(NULL));

    if (argc == 2) {
        int n = atoi(argv[1]);
        if (n <= 0) {
            fprintf(stderr, "Error: liczba punktow musi byc wieksza od 0 (podano %d)\n", n);
            return 1;
        }
        
        const char *dir = "./Input_Data";

        if (ensure_directory_windows(dir) == 0) {
            printf("Folder '%s' zostal utworzony.\n", dir);
        } else {
            perror("ensure_directory");
            return 1;
        }


        unsigned id = (unsigned)time(NULL) ^ (unsigned)rand();
        char filename[128];
        snprintf(filename, sizeof(filename),
                    "Input_Data/input_n_%d_%u.txt", n, id);

        FILE *out = fopen(filename, "w");
        if (!out) { perror(filename); return 1; }

        fprintf(out, "%d\n", n);
        Point *balls = malloc(n * sizeof(Point));
        int count = 0;
        while (count < n) {
            Point p = { rand() % COORD_MAX, rand() % COORD_MAX };
            bool ok = true;
            for (int i = 0; i < count && ok; ++i)
                for (int j = i+1; j < count; ++j)
                    if (collinear(balls[i], balls[j], p)) { ok = false; break; }
            if (ok) balls[count++] = p;
        }
        for (int i = 0; i < n; ++i)
            fprintf(out, "%.2f %.2f\n", balls[i].x, balls[i].y);
        free(balls);

        fprintf(out, "%d\n", n);
        for (int i = 0; i < n; ++i) {
            double hx = rand() % COORD_MAX;
            double hy = rand() % COORD_MAX;
            fprintf(out, "%.2f %.2f\n", hx, hy);
        }
        fclose(out);
        printf("Wygenerowano %s z %d pilkami oraz %d dolkami\n",
                filename, n, n);
        return 0;
    }
    const char *dir = "./Input_Data";

    if (ensure_directory_windows(dir) == 0) {
        printf("Folder '%s' zostal utworzony.\n", dir);
    } else {
        perror("ensure_directory");
        return 1;
    }

    for (int f = 1; f <= FILE_COUNT; f++) {
        char filename[32];
        snprintf(filename, sizeof(filename), "Input_Data/input%d.txt", f);
        FILE *out = fopen(filename, "w");
        if (!out) continue;

        // Generuj ilosc punktow do generacji
        int n = rand() % MAX_POINTS + 1;

        // Generuj pilki
        Point *balls = malloc(sizeof(Point) * n);
        int count = 0;
        while (count < n) {
            Point p = { rand() % COORD_MAX, rand() % COORD_MAX };
            bool ok = true;
            for (int i = 0; i < count && ok; i++) {
                for (int j = i + 1; j < count; j++) {
                    if (collinear(balls[i], balls[j], p)) {
                        ok = false;
                        break;
                    }
                }
            }
            if (ok) balls[count++] = p;
        }

        fprintf(out, "%d\n", n);
        for (int i = 0; i < n; i++) {
            fprintf(out, "%.2f %.2f\n", balls[i].x, balls[i].y);
        }
        free(balls);

        // Generuj dolki
        fprintf(out, "%d\n", n);
        for (int i = 0; i < n; i++) {
            double hx = rand() % COORD_MAX;
            double hy = rand() % COORD_MAX;
            fprintf(out, "%.2f %.2f\n", hx, hy);
        }

        fclose(out);
        printf("Wygenerowano %s z %d pilkami oraz %d dolkami\n", filename, n, n);
    }

    return 0;
}
