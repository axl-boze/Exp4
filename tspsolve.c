/*
* $gcc -o tspsolve tspsolve.c mt19937.c -lm -I. 
* のような形でコンパイル
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include "mt19937.h"

typedef enum {
    METHOD_RANDOM,
    METHOD_HC
} SearchMethod;

typedef enum {
    NEIGHBOR_SWAP, 
    NEIGHBOR_2OPT
} Neighborhood;

typedef struct {
    SearchMethod method;
    int iterations; //ランダム探索用
    Neighborhood neighborhood; // 山登り法用
} SearhConfig;

typedef struct {
    int cost;
    int iterations;
} HillClimbingResult;

typedef struct {
    int id;
    int x;
    int y;
} City;

typedef int (*ImproveFunction)(int *, int);

int N;
City *city;
char* FileName;

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
    return cost_Array[costIndex(a, b)];
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

void printResultHeader(
    const char *instance,
    const char *method,
    const char *neighborhood,
    unsigned long seedValue
) {
    printf("# Instance: %s\n", instance);
    printf("# Method: %s\n", method);

    if (neighborhood != NULL) {
        printf("# Neighborhood: %s\n", neighborhood);
    }

    printf("# output: step cost\n");
    printf("# seed: %lu\n", seedValue);
}

void printResultRow(int step, int costValue) {
    printf("%d %d\n", step, costValue);
}

void writeTourFile( const char *prefix, int iter, int maxIter, int costValue, int *tour, int n, City *city) {
    char filename[256];
    FILE *fp;

    snprintf(filename, sizeof(filename), "./%s-tour/%s-tour-%d.dat", prefix, prefix, iter);

    fp = fopen(filename, "w");
    if (fp == NULL) {
        fprintf(stderr, "FILE open error: %s\n", filename);
        exit(1);
    }

    fprintf(fp, "# Subject: %s\n", FileName);

    if (maxIter > 0) {
        fprintf(fp, "# Repetition: %d / %d\n", iter, maxIter);
    } else {
        fprintf(fp, "# Repetition: %d\n", iter);
    }

    fprintf(fp, "# Cost: %d\n", costValue);

    for (int k = 0; k < n; k++) {
        fprintf(fp, "%d %d\n", city[tour[k]].x, city[tour[k]].y);
    }

    fprintf(fp, "%d %d\n", city[tour[0]].x, city[tour[0]].y);

    fclose(fp);
}

/*
ランダムな巡回路を探索し、最良解を見つけだす。
*/
int *randomSearch(int iteration, int n, City* city, int interval) {
    if (iteration <= 0) return NULL;

    int *besttour = NULL;
    int bestlength = INT_MAX;

    for (int i = 1; i <= iteration; i++) {
        int *tour = buildRandomTour(n);
        int length = calcTourLength(tour, n);

        if (length < bestlength) {
            free(besttour);
            besttour = tour;
            bestlength = length;
        } else {
            free(tour);
        }

        printResultRow(i, bestlength);

        if (i % interval == 0 || i == iteration) {
            writeTourFile("random", i, iteration, bestlength, besttour, n, city);
        }
    }

    return besttour;
}

//Swap近傍での辺の交換で影響を受ける点を選び、コストの差分を計算する
int calcSwapDelta(int *tour, int n, int i, int j){
    // 位置iまたはjを変更すると影響を受ける辺の始点
    int candidates[4] = {(i + n - 1) % n, i, (j + n - 1) % n, j};
    int affected[4];
    int affectedCount = 0;

    // 重複する辺を取り除く
    for (int k = 0; k < 4; k++) {
        int duplicate = 0;

        for (int l = 0; l < affectedCount; l++) {
            if (affected[l] == candidates[k]) {
                duplicate = 1;
                break;
            }
        }

        if (!duplicate) {
            affected[affectedCount] = candidates[k];
            affectedCount++;
        }
    }

    int oldCost = 0;

    for (int k = 0; k < affectedCount; k++) {
        int p = affected[k];
        int next = (p + 1) % n;

        oldCost += cost(tour[p], tour[next]);
    }

    // 一時的に交換
    swap(&tour[i], &tour[j]);

    int newCost = 0;

    for (int k = 0; k < affectedCount; k++) {
        int p = affected[k];
        int next = (p + 1) % n;

        newCost += cost(tour[p], tour[next]);
    }

    // 元に戻す
    swap(&tour[i], &tour[j]);

    return newCost - oldCost;
}

int improveBySwap(int* tour, int n) {
    int bestDelta = 0;
    int bestI = -1;
    int bestJ = -1;
    for(int i = 0; i < n - 1; i++) {
        for(int j = i + 1; j < n; j++) {
            int delta = calcSwapDelta(tour, n, i, j);

            if (delta < bestDelta) {
                bestDelta = delta;
                bestI = i;
                bestJ = j;
            }
        }
    }
    if (bestDelta < 0) {
        swap(&tour[bestI], &tour[bestJ]);
        return bestDelta;
    }
    return 0;
}

void reverseTourSection(int *tour, int left, int right)
{
    while (left < right) {
        swap(&tour[left], &tour[right]);
        left++;
        right--;
    }
}

int improveBy2Opt(int *tour, int n){
    int bestDelta = 0;
    int bestI = -1;
    int bestJ = -1;

    if (n < 4) {
        return 0;
    }

    /*
     * iとjは、交換する2本の辺の始点。
     *
     * 1本目: tour[i] -> tour[i + 1]
     * 2本目: tour[j] -> tour[(j + 1) % n]
     */
    for (int i = 0; i < n - 1; i++) {
        /*
         * j = i + 1では2本の辺が隣接してしまうため、
         * i + 2から開始する。
         */
        for (int j = i + 2; j < n; j++) {
            /*
             * 辺(n-1 -> 0)と辺(0 -> 1)も隣接しているので除外。
             */
            if (i == 0 && j == n - 1) {
                continue;
            }

            int nextI = i + 1;
            int nextJ = (j + 1) % n;

            int oldCost =
                  cost(tour[i], tour[nextI])
                + cost(tour[j], tour[nextJ]);

            int newCost =
                  cost(tour[i],     tour[j])
                + cost(tour[nextI], tour[nextJ]);

            int delta = newCost - oldCost;

            if (delta < bestDelta) {
                bestDelta = delta;
                bestI = i;
                bestJ = j;
            }
        }
    }

    if (bestDelta < 0) {
        /*
         * 2本の辺をつなぎ替えるため、
         * tour[i + 1]からtour[j]までを逆順にする。
         */
        reverseTourSection(tour, bestI + 1, bestJ);
        return bestDelta;
    }

    return 0;
}   

HillClimbingResult hillClimbing(
    int *tour,
    int n,
    ImproveFunction improve,
    const char *prefix,
    int interval
) {
    HillClimbingResult result;

    result.cost = calcTourLength(tour, n);
    result.iterations = 0;

    printResultRow(result.iterations, result.cost);
    writeTourFile(prefix, result.iterations, -1, result.cost, tour, n, city);

    while (1) {
        int delta = improve(tour, n);

        if (delta == 0) {
            break;
        }

        result.cost += delta;
        result.iterations++;

        printResultRow(result.iterations, result.cost);

        if (result.iterations % interval == 0) {
            writeTourFile(prefix, result.iterations, -1, result.cost, tour, n, city);
        }
    }

    return result;
}

int main(int argc, char *argv[]) {
    unsigned long seedValue = (unsigned long)time(NULL);
    seed((uint_fast32_t)seedValue);
    if (argc != 4) {
        fprintf(stderr,
           "Usage:\n"
           "  %s <file> r  <iterations>\n"
           "  %s <file> hc <neighborhood>\n",
            argv[0], argv[0]);
        exit(1);
    }

    FileName = argv[1];

    SearhConfig config;

    if(strcmp(argv[2], "r") == 0) {
        char *end;
        long value = strtol(argv[3], &end, 10);
        if (*end != '\0' || value <= 0 || value > INT_MAX) {
            fprintf(stderr, "Invalid iteration count: %s\n", argv[3]);
            exit(1);
        }
        config.method = METHOD_RANDOM;
        config.iterations = (int)value;
    }else if (strcmp(argv[2], "hc") == 0) {
        config.method = METHOD_HC;

        if (strcmp(argv[3], "swap") == 0) {
            config.neighborhood = NEIGHBOR_SWAP; 
        }else if (strcmp(argv[3], "2opt") == 0) {
            config.neighborhood = NEIGHBOR_2OPT;
        }else{
            fprintf(stderr, "Invailed Neighborhood option\n");
            exit(1);
        }
    }else {
        fprintf(stderr, "Invailed argument\n");
        exit(1);
    }

    FILE *fp = fopen(FileName, "r");
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
            fscanf(fp, "%s", buf);
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

    int *tour = NULL;

    if (config.method == METHOD_RANDOM) {
        printResultHeader(FileName, "random_search", NULL, seedValue);
        tour = randomSearch(config.iterations, N, city, 100);
    } else if(config.method == METHOD_HC) {
        tour = buildRandomTour(N);
        if(config.neighborhood == NEIGHBOR_SWAP) {
            printResultHeader(FileName, "hill-climbing", "swap", seedValue);
            hillClimbing(tour, N, improveBySwap, "hc-swap", 1);
        }else if(config.neighborhood == NEIGHBOR_2OPT) {
            printResultHeader(FileName, "hill-climbing", "2opt", seedValue);
            hillClimbing(tour, N, improveBy2Opt, "hc-2opt", 1);
        }
    }
    fclose(fp);
    free(city);
    free(cost_Array);
    free(tour);
    return 0;
}
