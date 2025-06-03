#include <stdlib.h>
#include <stdint.h>

static uint64_t mat_rng[2] = {11ULL, 1181783497276652981ULL};

static inline uint64_t xorshift128plus(uint64_t s[2])
{
    uint64_t x, y;
    x = s[0], y = s[1];
    s[0] = y;
    x ^= x << 23;
    s[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
    y += s[1];
    return y;
}

float **mat_init(int n_rows, int n_cols)
{
    float **m;
    int i;
    m = (float **)malloc(n_rows * sizeof(float *));
    m[0] = (float *)calloc(n_rows * n_cols, sizeof(float));
    for (i = 1; i < n_rows; ++i)
        m[i] = m[i - 1] + n_cols;
    return m;
}

double mat_drand(void)
{
    return (xorshift128plus(mat_rng) >> 11) * (1.0 / 9007199254740992.0);
}

float **mat_gen_random(int n_rows, int n_cols)
{
    float **m;
    int i, j;
    m = mat_init(n_rows, n_cols);
    for (i = 0; i < n_rows; ++i)
        for (j = 0; j < n_cols; ++j)
            m[i][j] = mat_drand();
    return m;
}

void mat_mul0(float **m, int n_a_rows, int n_a_cols, float *const *a, int n_b_cols, float *const *b)
{
    int i, j, k;
    for (i = 0; i < n_a_rows; ++i)
    {
        for (j = 0; j < n_b_cols; ++j)
        {
            float t = 0.0;
            for (k = 0; k < n_a_cols; ++k)
                t += a[i][k] * b[k][j];
            m[i][j] = t;
        }
    }
}

void mat_destroy(float **m)
{
    free(m[0]);
    free(m);
}

int main(int argc, char *argv[])
{
    int n = 32;
    float **a, **b, **m = 0;

    m = mat_init(n, n);
    a = mat_gen_random(n, n);
    b = mat_gen_random(n, n);

    mat_mul0(m, n, n, a, n, b);

    mat_destroy(a);
    mat_destroy(b);
    mat_destroy(m);

    return 0;
}