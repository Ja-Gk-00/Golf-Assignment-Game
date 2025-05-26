#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

typedef struct { double x, y; bool isBall; } Point;
typedef struct { Point a, b; } Pair;

static Pair  *pairs     = NULL;
static int    pairCount = 0;

// Sprawdz wspolliniowosc punktow
static bool collinear(Point a, Point b, Point c) {
    double det = (b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x);
    return fabs(det) < 1e-8;
}

// Funckja Classify Partition
void classify_partition(Point *P, int n, Point p, Point q,
                        Point **L, int *nl, Point **R, int *nr)
{
    // Krok 1: Oblicz wspolczynniki prostej: ax + by = d
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

// Funkcja Run Algorithm
void run_algorithm(Point *B, int bn, Point *H, int hn) {

    // Krok 0: Jesli ktoras grupa jest pusta, nie ma dopasowan
    if (bn == 0 || hn == 0) return;

    // Krok 1 (warunek bazowy): Dokladnie jedna pilka i jeden dolek
    if (bn == 1 && hn == 1) {
        pairs = realloc(pairs, (pairCount+1) * sizeof(Pair));
        pairs[pairCount++] = (Pair){ B[0], H[0] };
        return;
    }

    // Krok 2: Polocz obie grupy w jedna tablice P
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

    // Krok 8: Usun q z listy S, aby pozostale punkty podzielic na dwie grupy
    Point *T = malloc((m-1) * sizeof(Point));
    for (int i = 0, k = 0; i < m; ++i) {
        if (!(S[i].x == q.x && S[i].y == q.y && S[i].isBall == q.isBall)) {
            T[k++] = S[i];
        }
    }
    free(S);

    // Krok 9: Podziel pozostale punkty względem prostej p <-> q
    Point *Lset, *Rset;
    int nl, nr;
    classify_partition(T, m-1, p, q, &Lset, &nl, &Rset, &nr);
    free(T);

    // Krok 10: Rozdziel Lset na pilki BL i dołki HL
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

// Generuj 2n nie wspoliniowych punktow
void generate_instance(Point *balls, Point *holes, int n) {
    int total = 2*n;
    Point *P = malloc(total * sizeof(Point));
    int cnt = 0;
    while (cnt < total) {
        Point c = { (double)rand()/RAND_MAX, (double)rand()/RAND_MAX, cnt<n };
        bool ok = true;
        for (int i=0; i<cnt && ok; ++i)
            for (int j=i+1; j<cnt; ++j)
                if (collinear(P[i], P[j], c)) { ok=false; break; }
        if (ok) P[cnt++] = c;
    }
    for (int i = 0; i < n; i++) {
        balls[i] = P[i];
        holes[i] = P[n+i];
    }
    free(P);
}

// Funckja do benchmarkingu
int main(int argc, char **argv) {
    if (argc!=4) {
        fprintf(stderr,"Argumenty: %s <reps> <max_n> <out.txt>\n",argv[0]);
        return EXIT_FAILURE;
    }
    int reps = atoi(argv[1]), maxn = atoi(argv[2]);
    const char *outf = argv[3];
    FILE *f = fopen(outf,"w");
    if (!f) { perror(outf); return EXIT_FAILURE; }
    srand((unsigned)time(NULL));

    Point *balls = malloc(maxn*sizeof(Point)),
          *holes = malloc(maxn*sizeof(Point));
    double *times = malloc(reps*sizeof(double));
    for (int n=1; n<=maxn; ++n) {
        double sum=0, sumsq=0;
        for (int r=0; r<reps; ++r) {
            generate_instance(balls, holes, n);
            pairCount = 0;
            pairs = NULL;
            clock_t t0 = clock();
            run_algorithm(balls, n, holes, n);
            clock_t t1 = clock();
            double dt = (double)(t1 - t0)/CLOCKS_PER_SEC;
            times[r] = dt;
            sum += dt; sumsq += dt*dt;
            free(pairs);
        }
        double mean = sum/reps;
        double var  = sumsq/reps - mean*mean;
        double std  = sqrt(var>0?var:0);
        printf("Skonczono eksperyment dla wielkosci n = %d, zapisywanie danych.\n", n);
        fprintf(f,"%d\t%.6f\t%.6f\n", n, mean, std);
        fflush(f);
    }
    free(balls); free(holes); free(times);
    fclose(f);
    return 0;
}
