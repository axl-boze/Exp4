#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include "mt19937.h"

typedef struct {
    int id;
    int x;
    int y;
} City;

int N;
City *city;
int IMAX;

/*
 * コスト行列は対称行列なので、下三角部分だけを1次元配列に格納する。
 * 対角成分 cost(i, i) も0として配列に保存する。
 *
 * 0始まりの添字で i >= j のとき、元の行列 [i][j] の要素は
 * i * (i + 1) / 2 + j で指定できる。
 */
int *cost_Array;

int *setCostArray(int n) {
    int size = n * (n + 1) / 2;
    int *array = malloc(sizeof(int) * size);

    if (array == NULL) {
        fprintf(stderr, "cost array allocation fault.\n");
        exit(1);
    }

    return array;
}

int costIndex(int i, int j) {
    if (i < j) {
        int tmp = i;
        i = j;
        j = tmp;
    }

    return i * (i + 1) / 2 + j;
}

int calcNorm(City city1, City city2) {
    int delta_x = city1.x - city2.x;
    int delta_y = city1.y - city2.y;
    double norm = sqrt(delta_x * delta_x + delta_y * delta_y);

    return (int)round(norm);
}

int *buildCostArray(City *city, int n) {
    int *array = setCostArray(n);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j <= i; j++) {
            array[costIndex(i, j)] = calcNorm(city[i], city[j]);
        }
    }

    return array;
}

int cost(int a, int b) {
    return cost_Array[costIndex(a - 1, b - 1)];
}

/*
昇順にたどる巡回路を設定し、総移動コストを計算する。
終点から始点に戻ってくる際のコストを加え忘れないように注意。
6/19 1~nから0~n-1に変更。[都市番号] - 1に対応
*/
int *buildAscendingTour(int n) {
    int *tour = malloc(sizeof(int) * n);

    if (tour == NULL) {
        fprintf(stderr, "tour allocation fault.\n");
        exit(1);
    }

    for (int i = 0; i < n; i++) {
        tour[i] = i;
    }

    return tour;
}

void swap(int* a, int* b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}
/*
ランダムな巡回路を作成。
n番目の要素を、乱数の割り算によって決めていく。
*/
int *buildRandomTour(int n) {
    int *tour = buildAscendingTour(n);
    for( int i = n - 1; i > 0; i--) {
        int j = genrand() % (i + 1);
        swap(&tour[i], &tour[j]);
    }
    return tour;
}

int calcTourLength(int *tour, int n) {
    int length = 0;

    for (int i = 0; i < n - 1; i++) {
        length += cost(tour[i], tour[i + 1]);
    }

    length += cost(tour[n - 1], tour[0]);

    return length;
}

void writeTourFile(int i,int n, int iteration, int cost, int* tour, City* city) {
    char filename[30];
    FILE *fp;
    sprintf(filename, "./tour/random-tour-%d.dat", i);
    if((fp = fopen(filename, "w")) == NULL) {
        fprintf(stderr, "FILE open error: %s", filename);
        exit(1);
    }
    fprintf(fp, "# Repetition: %d / %d\n", i, iteration);
    fprintf(fp, "# Cost: %d\n", cost);
    for(int i = 0; i < n; i++) {
        fprintf(fp, "%d %d\n", city[tour[i]].x, city[tour[i]].y);
    }
    fprintf(fp, "%d %d\n", city[tour[0]].x, city[tour[0]].y);
    fclose(fp);
}

/*
ランダムな巡回路を探索し、最良解を見つけだす。
繰り返し回数と暫定解を出力できる。
*/
int *randomSearch(int iteration, int n, City* city) {
    int interval = 100;
    if (iteration <= 0) return NULL;
    //printf("Iteration,Tentative Solution\n");
    int *besttour = NULL;
    int bestlength = INT_MAX;
    for(int i = 1; i <= iteration; i++) {
        int *tour = buildRandomTour(n);
        int length = calcTourLength(tour, n);
        if(length < bestlength) {
            free(besttour);
            besttour = tour;
            bestlength = length;
        }else {
            free(tour);
        }
        //printf("%d,%d\n", i, bestlength);
        if( i % interval == 0) {
            writeTourFile(i, n, iteration, bestlength, besttour, city);
        }
    }
    return besttour;
}


int main(int argc, char *argv[]) {
    seed((uint_fast32_t)time(NULL));
    int IMAX = atoi(argv[2]);
    if (argc < 2) {
        fprintf(stderr, "Usage: tspsolve <file_path>\n");
        exit(1);
    }

    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        fprintf(stderr, "File open error: %s\n", argv[1]);
        exit(1);
    }

    do {
        char buf[100];

        if (fscanf(fp, "%99s", buf) != 1) {
            fprintf(stderr, "DIMENSION not found.\n");
            fclose(fp);
            exit(1);
        }

        if (strcmp("DIMENSION", buf) == 0) {
            fscanf(fp, "%99s", buf);
            break;
        }

        if (strcmp("DIMENSION:", buf) == 0) {
            break;
        }
    } while (1);

    fscanf(fp, "%d", &N);

    city = malloc(sizeof(City) * N);
    if (city == NULL) {
        fprintf(stderr, "memory allocation fault.\n");
        fclose(fp);
        exit(1);
    }

    do {
        char buf[100];

        if (fscanf(fp, "%99s", buf) != 1) {
            fprintf(stderr, "NODE_COORD_SECTION not found.\n");
            free(city);
            fclose(fp);
            exit(1);
        }

        if (strcmp("NODE_COORD_SECTION", buf) == 0) {
            for (int i = 0; i < N; i++) {
                fscanf(fp, "%d %d %d", &city[i].id, &city[i].x, &city[i].y);
            }
            break;
        }
    } while (1);

    cost_Array = buildCostArray(city, N);

    int *tour = randomSearch(IMAX, N, city);

    fclose(fp);
    free(city);
    free(cost_Array);
    free(tour);

    return 0;
}
