#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "mt19937.h"

typedef struct {
    int id;
    int x;
    int y;
} City;

int N;
City *city;

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
昇順にたどる順回路を設定し、総移動コストを計算する。
終点から始点に戻ってくる際のコストを加え忘れないように注意。
*/
int *buildAscendingTour(int n) {
    int *tour = malloc(sizeof(int) * n);

    if (tour == NULL) {
        fprintf(stderr, "tour allocation fault.\n");
        exit(1);
    }

    for (int i = 0; i < n; i++) {
        tour[i] = i + 1;
    }

    return tour;
}

void swap(int* a, int* b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

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

int main(int argc, char *argv[]) {
    seed((uint_fast32_t)time(NULL));
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

    int *tour = buildRandomTour(N);
    for(int i = 0; i < N; i++) {
        printf("%d ", tour[i]);
    }
    printf("\n");
    printf("総移動コスト: %d\n", calcTourLength(tour, N));

    fclose(fp);
    free(city);
    free(cost_Array);

    return 0;
}
