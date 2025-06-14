#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <windows.h>
#include <commdlg.h>
#include <ctype.h>
#include <direct.h>
#include <sys/stat.h>


#define WINDOW_WIDTH   800
#define WINDOW_HEIGHT  600
#define BUTTON_W        200
#define BUTTON_H         40
#define MAX_PATH_LEN 260

// 3×5 pomocnicza bitmapa do wypisywania liter
static const unsigned char font3x5[][5] = {
    /* ' ' */ {0b000, 0b000, 0b000, 0b000, 0b000},
    /* 'A' */ {0b010, 0b101, 0b111, 0b101, 0b101},
    /* 'E' */ {0b111, 0b100, 0b110, 0b100, 0b111},
    /* 'I' */ {0b111, 0b010, 0b010, 0b010, 0b111},
    /* 'N' */ {0b101, 0b111, 0b111, 0b111, 0b101},
    /* 'R' */ {0b110, 0b101, 0b110, 0b101, 0b101},
    /* 'S' */ {0b011, 0b100, 0b010, 0b001, 0b110},
    /* 'T' */ {0b111, 0b010, 0b010, 0b010, 0b010},
    /* 'U' */ {0b101, 0b101, 0b101, 0b101, 0b111},
    /* 'V' */ {0b101, 0b101, 0b101, 0b101, 0b010},
    /* 'X' */ {0b101, 0b101, 0b010, 0b101, 0b101},
    /* 'L' */ {0b100, 0b100, 0b100, 0b100, 0b111},
    /* 'O' */ {0b111, 0b101, 0b101, 0b101, 0b111},
    /* 'D' */ {0b110, 0b101, 0b101, 0b101, 0b110}
};

// mapping liter to zdefiniowanej bitmapy
static int charIndex(char c) {
    switch (c) {
        case 'A': return 1;
        case 'E': return 2;
        case 'I': return 3;
        case 'N': return 4;
        case 'R': return 5;
        case 'S': return 6;
        case 'T': return 7;
        case 'U': return 8;
        case 'V': return 9;
        case 'X': return 10;
        case 'L': return 11;
        case 'O': return 12;
        case 'D': return 13;
        default:  return 0;  // spacja
    }
}

// wypisz tekst przy uzyciu bitmapy
void draw_text(SDL_Renderer *ren, float x, float y, const char *msg, int scale) {
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    while (*msg) {
        int idx = charIndex(*msg++);
        for (int row = 0; row < 5; ++row) {
            unsigned char bits = font3x5[idx][row];
            for (int col = 0; col < 3; ++col) {
                if (bits & (1 << (2 - col))) {
                    SDL_FRect px = {
                        x + col * scale,
                        y + row * scale,
                        (float)scale,
                        (float)scale
                    };
                    SDL_RenderFillRect(ren, &px);
                }
            }
        }
        x += 4 * scale;
    }
}

// funkcja do sprawdzania czy folder "Outputs" istnieje
void ensure_outputs_dir(void) {
    struct _stat info;
    if (_stat("Outputs", &info) != 0) {
        if (_mkdir("Outputs") != 0) {
            perror("Nie mozna bylo utworzyc folderu Outputs.");
        }
    }
    else if (!(info.st_mode & _S_IFDIR)) {
        fprintf(stderr, "Error: “Outputs” istnieje, ale nie jest folderem.\n");
    }
}

static int inputIndex = 1;

// pomocnicze objekty
typedef struct {
    double x, y;
    bool   isBall;
} Point;

typedef struct {
    Point a, b;
} Pair;

// globalny poczatkowy stan obiektow
static Point *balls   = NULL;
static Point *holes   = NULL;
static Pair  *pairs   = NULL;
static int    nb = 0, nh = 0;
static int    pairCount = 0;
static bool   matched   = false;

static void get_exe_dir(char *out, size_t len);
char *strcat(char *dest, const char *src);

static char *open_file_dialog(void) {
    static char filename[MAX_PATH_LEN] = {0};
    OPENFILENAMEA ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "Text Files\0*.txt\0All Files\0*.*\0";
    char exeDir[MAX_PATH_LEN] = {0};
    get_exe_dir(exeDir, sizeof(exeDir));
    ofn.lpstrFile   = filename;
    ofn.nMaxFile    = MAX_PATH_LEN;
    ofn.lpstrInitialDir = strcat(exeDir, "/Input_Data");
    ofn.lpstrInitialDir = exeDir;
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameA(&ofn)) {
        return filename;
    }
    return NULL;
}

// wczytaj plik tekstowy
void read_input(void) {
    char *fn = open_file_dialog();
    if (!fn) return; 

    {
        size_t len = strlen(fn);
        int end = (int)len - 1;
        while (end >= 0 && fn[end] != '.') --end;
        int num_end = end;
        int num_start = num_end - 1;
        while (num_start >= 0 && isdigit((unsigned char)fn[num_start])) --num_start;
        ++num_start;
        if (num_start < num_end) {
            char buf[16] = {0};
            int count = num_end - num_start;
            if (count < (int)sizeof(buf)) {
                memcpy(buf, fn + num_start, count);
                inputIndex = atoi(buf);
            }
        }
    }

    // zwolnij alokacje pamieci z uprzedniej konfiguracji
    free(balls);
    free(holes);
    free(pairs);
    balls     = NULL;
    holes     = NULL;
    pairs     = NULL;
    pairCount = 0;
    matched   = false;

    FILE *f = fopen(fn, "r");
    if (!f) { perror(fn); return; }

    fscanf(f, "%d", &nb);
    balls = malloc(nb * sizeof(Point));
    for (int i = 0; i < nb; ++i) {
        fscanf(f, "%lf %lf", &balls[i].x, &balls[i].y);
        balls[i].isBall = true;
    }

    fscanf(f, "%d", &nh);
    holes = malloc(nh * sizeof(Point));
    for (int i = 0; i < nh; ++i) {
        fscanf(f, "%lf %lf", &holes[i].x, &holes[i].y);
        holes[i].isBall = false;
    }
    fclose(f);

    printf("[INFO] Zaladowano %s (index=%d): %d pilek, %d dolkow\n",
           fn, inputIndex, nb, nh);
}

// Funkcja Classify partition
void classify_partition(Point *P, int n, Point p, Point q,
                        Point **L, int *nl, Point **R, int *nr)
{
    // Krok 1: Oblicz wspo;czynniki prostej: ax + by = d
    double dx = q.y - p.y;
    double dy = p.x - q.x;
    double d  = dx * p.x + dy * p.y;

    // Krok 2: Przydzielenie pamieci na podgrupy
    *L = malloc(n * sizeof(Point));
    *R = malloc(n * sizeof(Point));
    *nl = *nr = 0;

    // Krok 3: Dla kazdego punktu oblicz wartosc side = a·x + b·y − d
    for (int i = 0; i < n; ++i) {
        double side = dx * P[i].x + dy * P[i].y - d;
        if (side >= 0) {
            (*L)[(*nl)++] = P[i];
        } else {
            (*R)[(*nr)++] = P[i];
        }
    }
}


// Rekurencyjna funkcja glowna do wyznaczania przydzielania pilek i doklow w pary
void run_algorithm(Point *B, int bn, Point *H, int hn) {
    printf("[INFO] Run algorithm: bn=%d, hn=%d\n", bn, hn);

    // Krok 0: Jesli ktoras grupa jest pusta, nie ma dopasowan
    if (bn == 0 || hn == 0) return;

    // Krok 1 (warunek bazowy): Dokladnie jedna pilka i jeden dolek
    if (bn == 1 && hn == 1) {
        pairs = realloc(pairs, (pairCount+1) * sizeof(Pair));
        pairs[pairCount++] = (Pair){ B[0], H[0] };
        printf("[INFO] Znaleziono dopasowanie: (%.2f,%.2f) -> (%.2f,%.2f)\n",
               B[0].x, B[0].y, H[0].x, H[0].y);
        return;
    }

    // Krok 2: Polocz obie grupy w jedną tablice P
    int total = bn + hn;
    Point *P  = malloc(total * sizeof(Point));
    for (int i = 0; i < bn; ++i)         P[i]       = B[i];
    for (int i = 0; i < hn; ++i)         P[bn + i]  = H[i];

    // Krok 3: Wybierz pivot = p (punkt o najmniejszym y, potem x)
    int pivot = 0;
    for (int i = 1; i < total; ++i) {
        if (P[i].y < P[pivot].y ||
           (P[i].y == P[pivot].y && P[i].x < P[pivot].x)) {
            pivot = i;
        }
    }
    Point p = P[pivot];

    // Krok 4: Zbuduj tablice S (bez pivotu) i oblicz katy każdej pary (p, S[j])
    int m        = total - 1;
    Point *S     = malloc(m * sizeof(Point));
    double *ang  = malloc(m * sizeof(double));
    for (int i = 0, j = 0; i < total; ++i) {
        if (i == pivot) continue;
        S[j]   = P[i];
        ang[j] = atan2(P[i].y - p.y, P[i].x - p.x);
        ++j;
    }
    free(P);

    // Krok 5: Posortuj punkty S według rosnącego kata ang[]
    for (int i = 0; i < m; ++i) {
        for (int j = i+1; j < m; ++j) {
            if (ang[j] < ang[i]) {
                double ta = ang[i]; ang[i] = ang[j]; ang[j] = ta;
                Point tp = S[i];    S[i]   = S[j];   S[j]   = tp;
            }
        }
    }

    // Krok 6: Znajdz q, przy ktorym zrownowazy się #pilek − #dolkow = 0
    int diff = p.isBall ? 1 : -1;
    Point q  = {0};
    for (int i = 0; i < m; ++i) {
        diff += S[i].isBall ? 1 : -1;
        if (diff == 0) {
            q = S[i];
            break;
        }
    }
    free(ang);

    // Krok 7: Zapisz dopasowanie pivot <-> q
    pairs = realloc(pairs, (pairCount+1) * sizeof(Pair));
    if (p.isBall)
        pairs[pairCount++] = (Pair){ p, q };
    else
        pairs[pairCount++] = (Pair){ q, p };
    printf("[INFO] Dokonano parowania: (%.2f,%.2f) z (%.2f,%.2f)\n",
           p.x, p.y, q.x, q.y);

    // Krok 8: Usun q z listy S, aby pozostale punkty podzielic na dwie grupy
    Point *T = malloc((m-1) * sizeof(Point));
    for (int i = 0, k = 0; i < m; ++i) {
        if (!(S[i].x == q.x && S[i].y == q.y && S[i].isBall == q.isBall)) {
            T[k++] = S[i];
        }
    }
    free(S);

    // Krok 9: Podziel pozostałe punkty względem prostej p <-> q
    Point *Lset, *Rset;
    int nl, nr;
    classify_partition(T, m-1, p, q, &Lset, &nl, &Rset, &nr);
    free(T);

    // Krok 10: Rozdziel Lset na pilki BL i dolki HL
    Point *BL = malloc(nl * sizeof(Point)); int bl = 0;
    Point *HL = malloc(nl * sizeof(Point)); int hl = 0;
    for (int i = 0; i < nl; ++i) {
        if (Lset[i].isBall) BL[bl++] = Lset[i];
        else                HL[hl++] = Lset[i];
    }
    free(Lset);

    // Krok 11: Rozdziel Rset na pilki BR i dolki HR
    Point *BR = malloc(nr * sizeof(Point)); int br = 0;
    Point *HR = malloc(nr * sizeof(Point)); int hr = 0;
    for (int i = 0; i < nr; ++i) {
        if (Rset[i].isBall) BR[br++] = Rset[i];
        else                HR[hr++] = Rset[i];
    }
    free(Rset);

    // Krok 12: Rekurencyjne wywolanie na obu podzbiorach
    run_algorithm(BL, bl, HL, hl);
    run_algorithm(BR, br, HR, hr);

    // Krok 13: Zwolnienie pamieci pomocniczej
    free(BL); free(HL);
    free(BR); free(HR);
}

static void get_exe_dir(char *out, size_t len) {
     char buf[MAX_PATH];
     GetModuleFileNameA(NULL, buf, MAX_PATH);
     char *p = strrchr(buf, '\\');
     if (p) *p = '\0';
     strncpy(out, buf, len);
 }

// Zapisz plik do output
void save_output(void) {
    char exeDir[MAX_PATH];
    get_exe_dir(exeDir, sizeof(exeDir));
    char dirPath[MAX_PATH];
    snprintf(dirPath, sizeof(dirPath), "%s\\Outputs", exeDir);
    if (_mkdir(dirPath) != 0 && errno != EEXIST) {
        perror("mkdir Outputs");
    }
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\output_%d.txt", dirPath, inputIndex);
    FILE *f = fopen(path, "w");
    if (!f) { perror(path); return; }
    for (int i = 0; i < pairCount; ++i) {
        fprintf(f, "%.2f %.2f %.2f %.2f\n",
                pairs[i].a.x, pairs[i].a.y,
                pairs[i].b.x, pairs[i].b.y);
    }
    fclose(f);
    printf("[INFO] Zapisano plik wyjsciowy do %s\n", path);
}

int main(void) {
    {
        char exeDir[MAX_PATH_LEN] = {0};
        get_exe_dir(exeDir, sizeof(exeDir));
        SetCurrentDirectoryA(exeDir);
    }
    read_input();

    if (SDL_Init(SDL_INIT_VIDEO) != true) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    SDL_Window *win;
    SDL_Renderer *ren;
    if (SDL_CreateWindowAndRenderer(
            "Golf GUI",
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            0,
            &win,
            &ren
        ) != true)
    {
        fprintf(stderr, "Window Error: %s\n", SDL_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_FRect btnRun  = { 10.0f, WINDOW_HEIGHT - 50.0f, BUTTON_W, BUTTON_H };
    SDL_FRect btnLoad = {230.0f, WINDOW_HEIGHT - 50.0f, BUTTON_W, BUTTON_H};
    SDL_FRect btnExit = {450.0f, WINDOW_HEIGHT - 50.0f, BUTTON_W, BUTTON_H };
    SDL_Event e;

    while (true) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) goto cleanup;
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                float x = (float)e.button.x;
                float y = (float)e.button.y;
                if (!matched &&
                    x >= btnRun.x && x <= btnRun.x + btnRun.w &&
                    y >= btnRun.y && y <= btnRun.y + btnRun.h)
                {
                    printf("[INFO] Uruchomiono algorytm\n");
                    run_algorithm(balls, nb, holes, nh);
                    save_output();
                    matched = true;
                }
                else if (
                    x >= btnLoad.x && x <= btnLoad.x + btnLoad.w &&
                    y >= btnLoad.y && y <= btnLoad.y + btnLoad.h
                ) {
                    printf("[INFO] Klinkieto w przycisk Load\n");
                    read_input();
                    matched = false; pairCount = 0;
                }
                else if (
                    x >= btnExit.x && x <= btnExit.x + btnExit.w &&
                    y >= btnExit.y && y <= btnExit.y + btnExit.h)
                {
                    printf("[INFO] Kliknieto w przycisk Exit\n");
                    goto cleanup;
                }
            }
        }

        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        SDL_RenderClear(ren);

        // Rysuj pilki
        SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
        for (int i = 0; i < nb; ++i) {
            SDL_FRect r = { 0.5*balls[i].x - 4.0f, 0.5*balls[i].y - 4.0f, 8.0f, 8.0f };
            SDL_RenderFillRect(ren, &r);
        }

        // Rysuj dolki
        SDL_SetRenderDrawColor(ren, 0, 0, 255, 255);
        for (int i = 0; i < nh; ++i) {
            SDL_FRect r = { 0.5*holes[i].x - 4.0f, 0.5*holes[i].y - 4.0f, 8.0f, 8.0f };
            SDL_RenderFillRect(ren, &r);
        }

        // Rysuj przyciski
        SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);
        SDL_RenderFillRect(ren, &btnRun);
        SDL_RenderFillRect(ren, &btnLoad);
        SDL_RenderFillRect(ren, &btnExit);
        draw_text(ren, btnRun.x + 5, btnRun.y + 5, "RUN", 2);
        draw_text(ren, btnLoad.x + 5, btnLoad.y + 5, "LOAD", 2);
        draw_text(ren, btnExit.x + 5, btnExit.y + 5, "EXIT", 2);

        // Rysuj linie laczace
        if (matched) {
            SDL_SetRenderDrawColor(ren, 0, 255, 0, 255);
            for (int i = 0; i < pairCount; ++i) {
                SDL_RenderLine(
                    ren,
                    0.5*pairs[i].a.x, 0.5*pairs[i].a.y,
                    0.5*pairs[i].b.x, 0.5*pairs[i].b.y
                );
            }
        }

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

cleanup:
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    free(balls);
    free(holes);
    free(pairs);
    return EXIT_SUCCESS;
}