#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <omp.h>

static const int DI[8] = {-1, +1, 0, 0, -1, -1, +1, +1};
static const int DJ[8] = {0, 0, -1, +1, -1, +1, -1, +1};
#define AT(A, i, j, N) (A[(i) * (N) + (j)])

int dentro(int i, int j, int N) { return (i >= 0 && i < N && j >= 0 && j < N); }

void imprimir(char *T, int N)
{
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
}

int leer_entrada(FILE *f, int *N, int *M, char **prev)
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
            continue;
        }
        else
        {
            return 0;
        }
    }
    return count == Nloc * Nloc;
}

void paso_minuto_omp(char *prev, char *next, unsigned char *marcado, int N)
{
// 1) limpiar marcado en paralelo
#pragma omp parallel for schedule(static)
    for (int idx = 0; idx < N * N; ++idx)
        marcado[idx] = 0;

// 2) marcar infecciones (lectura de prev; escritura atómica a marcado)
#pragma omp parallel for schedule(static)
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
// set a 1 de forma atómica para evitar data race
#pragma omp atomic write
                    AT(marcado, ni, nj, N) = 1;
                    break; // este zombie ya cumplió su “uno por minuto”
                }
            }
        }
    }

// 3) construir next (cada celda se escribe exactamente una vez)
#pragma omp parallel for schedule(static)
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
        fprintf(stderr, "Uso: %s archivo.txt [hilos]\n", argv[0]);
        return 1;
    }
    int threads = (argc >= 3) ? atoi(argv[2]) : 0;
    if (threads > 0)
        omp_set_num_threads(threads);

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

    printf("== Minuto 0 ==\n");
    imprimir(prev, N);

    double start = omp_get_wtime();

    for (int t = 1; t <= M; t++)
    {
        paso_minuto_omp(prev, next, marcado, N);
        char *tmp = prev;
        prev = next;
        next = tmp;

        printf("== Minuto %d ==\n", t);
        imprimir(prev, N);
    }

    double end = omp_get_wtime();
    printf("Tiempo total: %.6f segundos con %d hilos\n", end - start, (threads > 0 ? threads : omp_get_max_threads()));

    free(prev);
    free(next);
    free(marcado);
    return 0;
}