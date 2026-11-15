/* EA-XA block decoder (eaxadecf.cpp): decodes EA-XA compressed sample blocks to floats for the unpack
   filters. Allocation goes through the sound library's allocator callbacks. */

#include "engine/sndsys.h"

extern "C" {

extern float lbl_80301008[2][4];
extern float lbl_80301028[16][16];

void fn_8027BAFC(Block_8027BC18 *p)
{
    short value;
    unsigned int i;

    if (*p->mC == 0xEE) {
        p->mC++;
        {
            unsigned char *b = (unsigned char *)&value;
            b[0] = p->mC[0];
            b[1] = p->mC[1];
            p->mC += 2;
        }
        p->m4 = value;
        {
            unsigned char *b = (unsigned char *)&value;
            b[0] = p->mC[0];
            b[1] = p->mC[1];
            p->mC += 2;
        }
        p->m8 = value;
        for (i = 0; i < 28; i++) {
            {
                unsigned char *b = (unsigned char *)&value;
                b[0] = p->mC[0];
                b[1] = p->mC[1];
            }
            *p->m10 = value;
            p->m10++;
            p->mC += 2;
        }
    }
}

void fn_8027BC18(Block_8027BC18 *p)
{
    unsigned char *src;
    float *out;
    float c1;
    float c2;
    int shift;
    int n;
    int i;
    unsigned char d;

    src = p->mC;
    p->m10[-2] = p->m8;
    p->m10[-1] = p->m4;
    while (p->m0 > 0) {
        if (*src == 0xEE) {
            fn_8027BAFC(p);
            p->m0 -= 28;
        } else {
            n = *p->mC >> 4;
            p->m0 -= 28;
            c1 = lbl_80301008[0][n];
            c2 = lbl_80301008[1][n];
            shift = *p->mC & 0xF;
            p->mC++;
            for (i = 0; i < 14; i++) {
                d = p->mC[i];
                p->m10[0] = lbl_80301028[shift][d >> 4] + p->m10[-1] * c1 + p->m10[-2] * c2;
                p->m10[1] = lbl_80301028[shift][d & 0xF] + p->m10[0] * c1 + p->m10[-1] * c2;
                p->m10 += 2;
            }
            p->mC += 14;
            p->m8 = p->m10[-2];
            p->m4 = p->m10[-1];
        }
        src = p->mC;
    }
}

void *fn_8027BD68(int size)
{
    return lbl_803B53E4(size);
}

void fn_8027BD94(void *p)
{
    lbl_803B53E8(p);
}

Struct_8027BE24 *fn_8027BDC0(void *memory)
{
    Struct_8027BE24 *p = (Struct_8027BE24 *)memory;

    p->mC = 0;
    p->m8 = 0;
    p->m4 = 0;
    p->m94.m4 = 0.0f;
    p->m94.m8 = 0.0f;
    return p;
}

int fn_8027BDE8(Struct_8027BE24 *p, char *data, int size, int count)
{
    if (data == 0) {
        return -1;
    }
    if (p->m8 != 0) {
        return -1;
    }
    p->m8 = count;
    p->mC = size;
    p->m94.mC = (unsigned char *)data;
    return 0;
}

int fn_8027BE24(Struct_8027BE24 *p, float **dst, int count)
{
    int done;
    int n;
    int k;
    int i;
    int blocks;
    int tail;
    float *save;

    done = 0;
    p->m94.m10 = *dst;
    if (p->m8 == 0) {
        return 0;
    }
    n = p->m8;
    if (n > count) {
        n = count;
    }
    if (p->m4 != 0) {
        if (p->m4 > n) {
            k = n;
        } else {
            k = p->m4;
        }
        for (i = 0; i < k; i++) {
            *p->m94.m10 = *p->m10;
            p->m94.m10++;
            p->m10++;
        }
        n -= k;
        p->m8 -= k;
        p->m4 -= k;
        done = k;
    }
    blocks = n / 28 * 28;
    tail = n - blocks;
    p->m94.m0 = blocks;
    if (blocks > 0) {
        done += blocks;
        fn_8027BC18(&p->m94);
        p->m8 -= blocks;
    }
    if (tail > 0) {
        save = p->m94.m10;
        p->m94.m0 = tail;
        p->m94.m10 = p->m24;
        i = 0;
        fn_8027BC18(&p->m94);
        p->m8 -= tail;
        p->m10 = p->m94.m10 + p->m94.m0;
        p->m4 = -p->m94.m0;
        p->m94.m10 -= 28;
        for (; i < tail; i++) {
            save[i] = p->m94.m10[i];
        }
        done += tail;
    }
    if (p->m8 <= 0) {
        p->m4 = 0;
    }
    return done;
}

Pair_8027BFDC fn_8027BFDC(Struct_8027BE24 *p)
{
    Pair_8027BFDC pair;

    pair.m0 = p->m94.m4;
    pair.m4 = p->m94.m8;
    return pair;
}

void fn_8027C00C(Struct_8027BE24 *p, Pair_8027BFDC *pair)
{
    p->m94.m4 = pair->m0;
    p->m94.m8 = pair->m4;
}

}
