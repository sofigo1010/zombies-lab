#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>

static const int DI[8] = {-1, +1, 0, 0, -1, -1, +1, +1}; // Up, Down, Left, Right, UL, UR, DL, DR
static const int DJ[8] = {0, 0, -1, +1, -1, +1, -1, +1};

#define AT(A, i, j, N) (A[(i) * (N) + (j)])

#define PRINT_STEPS 1

static int dentro(int i, int j, int N) { return (i >= 0 && i < N && j >= 0 && j < N); }

static void imprimir(char *T, int N)
{
#if PRINT_STEPS
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            putchar(AT(T, i, j, N));
            if (j + 1 < N)
                putchar(' ');
        }
        putchar('\n');
    }
#else
    (void)T;
    (void)N;
#endif
}

static double walltime(void)
{
#ifdef CLOCK_MONOTONIC
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
#endif
}

static int leer_entrada(FILE *f, int *N, int *M, char **prev)
{
    if (fscanf(f, "%d %d", N, M) != 2)
        return 0;
    int Nloc = *N;
    *prev = (char *)malloc((size_t)Nloc * Nloc);
    if (!*prev)
        return 0;

    int count = 0;
    while (count < Nloc * Nloc)
    {
        int c = fgetc(f);
        if (c == EOF)
            break;
        if (c == 'H' || c == 'Z' || c == '.' || c == 'h' || c == 'z')
        {
            (*prev)[count++] = (char)toupper(c);
        }
        else if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
        {
            continue; // ignora espacios
        }
        else
        {
            return 0; // símbolo inválido
        }
    }
    return count == Nloc * Nloc;
}

static void paso_minuto_serial(char *prev, char *next, unsigned char *marcado, int N)
{
    // 1) limpiar marcado
    for (int i = 0; i < N * N; i++)
        marcado[i] = 0;

    // 2) marcar infecciones mirando SOLO prev
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            if (AT(prev, i, j, N) != 'Z')
                continue;
            for (int k = 0; k < 8; k++)
            {
                int ni = i + DI[k], nj = j + DJ[k];
                if (!dentro(ni, nj, N))
                    continue;
                if (AT(prev, ni, nj, N) == 'H')
                {
                    AT(marcado, ni, nj, N) = 1; // uno por minuto
                    break;
                }
            }
        }
    }

    // 3) construir next desde prev + marcado
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            char c = AT(prev, i, j, N);
            if (c == 'Z')
                AT(next, i, j, N) = 'Z';
            else if (c == 'H')
                AT(next, i, j, N) = AT(marcado, i, j, N) ? 'Z' : 'H';
            else
                AT(next, i, j, N) = '.';
        }
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Uso: %s archivo.txt\n", argv[0]);
        return 1;
    }
    FILE *f = fopen(argv[1], "r");
    if (!f)
    {
        perror("fopen");
        return 1;
    }

    int N, M;
    char *prev;
    if (!leer_entrada(f, &N, &M, &prev))
    {
        fprintf(stderr, "Error de entrada.\n");
        return 1;
    }
    fclose(f);

    char *next = (char *)malloc((size_t)N * N);
    unsigned char *marcado = (unsigned char *)malloc((size_t)N * N);
    if (!next || !marcado)
    {
        fprintf(stderr, "Memoria insuficiente.\n");
        return 1;
    }

#if PRINT_STEPS
    printf("== Minuto 0 ==\n");
    imprimir(prev, N);
#endif

    double t0 = walltime();

    for (int t = 1; t <= M; t++)
    {
        paso_minuto_serial(prev, next, marcado, N);
        char *tmp = prev;
        prev = next;
        next = tmp;

#if PRINT_STEPS
        printf("== Minuto %d ==\n", t);
        imprimir(prev, N);
#endif
    }

    double t1 = walltime();
    printf("Tiempo total: %.6f segundos\n", t1 - t0);

    free(prev);
    free(next);
    free(marcado);
    return 0;
}