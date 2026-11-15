/* MT block decoder (mtdecf.cpp): decodes MT (MicroTalk) compressed sample blocks into a 432-sample
   float buffer for the MT unpack filters: allocation, the bit reader, the stream header, the
   excitation, reflection-coefficient and LPC synthesis stages of each frame, sample delivery and the
   decoder's state save/restore. */

#include "engine/sndsys.h"

extern "C" {


typedef struct MtCommand_t {
    int m0;
    int m4;
    float m8;
} MtCommand_t;

extern float lbl_8030144C[64];
extern unsigned char lbl_8030154C[2][256];
extern MtCommand_t lbl_8030174C[];

void *fn_8027C020(int size)
{
    return lbl_803B53E4(size);
}

void fn_8027C04C(void *p)
{
    lbl_803B53E8(p);
}

}

namespace Snd {

unsigned int getbits(UTALKSTATE_CODA *bits, int count)
{
    unsigned int value;

    value = bits->m4 & lbl_80301428[count];
    bits->m4 >>= count;
    bits->m8 -= count;
    if (bits->m8 <= 7) {
        bits->m4 |= *bits->m0++ << bits->m8;
        bits->m8 += 8;
    }
    return value;
}

void discardbits(UTALKSTATE_CODA *bits, int count)
{
    bits->m4 >>= count;
    bits->m8 -= count;
    if (bits->m8 <= 7) {
        bits->m4 |= *bits->m0++ << bits->m8;
        bits->m8 += 8;
    }
}

void readsamples(UTALKSTATE_CODA *s, int multipulse, float *out, int stride)
{
    // FAKEMATCH: unused stack object; reproduces the target frame (0x58, conversion slot at 0x18)
    volatile int spare;
    int i;
    int model;
    int cnt;
    int x;
    int n;

    if (multipulse) {
        model = 0;
        i = 0;
        while (i < 108) {
            cnt = lbl_8030154C[model][s->m4 & 0xFF];
            model = lbl_8030174C[cnt].m0;
            discardbits(s, lbl_8030174C[cnt].m4);
            if (cnt > 3) {
                out[i] = lbl_8030174C[cnt].m8;
                i += stride;
            } else if (cnt > 1) {
                n = getbits(s, 6) + 7;
                if (i + stride * n > 108) {
                    n = (108 - i) / stride;
                }
                for (x = 0; x < n; x++) {
                    out[i] = 0.0f;
                    i += stride;
                }
            } else {
                x = 7;
                while (getbits(s, 1) == 1) {
                    x++;
                }
                if (getbits(s, 1) == 1) {
                    out[i] = x;
                } else {
                    out[i] = -x;
                }
                i += stride;
            }
        }
    } else {
        x = 0;
        do {
            i = s->m4 & 3;
            if (i == 1) {
                out[x] = -2.0f;
                discardbits(s, 2);
            } else if (i == 3) {
                out[x] = 2.0f;
                discardbits(s, 2);
            } else {
                out[x] = 0.0f;
                discardbits(s, 1);
            }
        } while ((x += stride) < 108);
    }
}

void interpolate(float *p)
{
    int i;

    for (i = 0; i < 108; i += 2) {
        p[i] = (p[i - 1] + p[i + 1]) * 0.59738594f + (p[i - 3] + p[i + 3]) * -0.11459156f
            + (p[i - 5] + p[i + 5]) * 0.018032679f;
    }
}

void reftolpc(float *rc, float *lpc)
{
    int i;
    int j;
    float x;
    float tmp2[12];
    float tmp1[12];

    for (j = 10; j >= 0; j--) {
        tmp2[j + 1] = rc[j];
    }
    tmp2[0] = 1.0f;
    for (i = 0; i < 12; i++) {
        x = -rc[11] * tmp2[11];
        for (j = 10; j >= 0; j--) {
            x -= rc[j] * tmp2[j];
            tmp2[j + 1] = rc[j] * x + tmp2[j];
        }
        tmp1[i] = tmp2[0] = x;
        for (j = 0; j < i; j++) {
            x -= lpc[j] * tmp1[i - j - 1];
        }
        lpc[i] = x;
    }
}

void filter(UTALKSTATE_CODA *p, int offset, int count)
{
    float lpc[12];
    float *out;
    int i;

    out = &p->m684[offset];
    reftolpc(p->m114, lpc);
    for (i = 0; i < count; i++) {
        out[0] = p->m144[11] = out[0] + lpc[0] * p->m144[0] + lpc[1] * p->m144[1] + lpc[2] * p->m144[2] + lpc[3] * p->m144[3]
            + lpc[4] * p->m144[4] + lpc[5] * p->m144[5] + lpc[6] * p->m144[6] + lpc[7] * p->m144[7]
            + lpc[8] * p->m144[8] + lpc[9] * p->m144[9] + lpc[10] * p->m144[10] + lpc[11] * p->m144[11];
        out[1] = p->m144[10] = out[1] + lpc[0] * p->m144[11] + lpc[1] * p->m144[0] + lpc[2] * p->m144[1] + lpc[3] * p->m144[2]
            + lpc[4] * p->m144[3] + lpc[5] * p->m144[4] + lpc[6] * p->m144[5] + lpc[7] * p->m144[6]
            + lpc[8] * p->m144[7] + lpc[9] * p->m144[8] + lpc[10] * p->m144[9] + lpc[11] * p->m144[10];
        out[2] = p->m144[9] = out[2] + lpc[0] * p->m144[10] + lpc[1] * p->m144[11] + lpc[2] * p->m144[0] + lpc[3] * p->m144[1]
            + lpc[4] * p->m144[2] + lpc[5] * p->m144[3] + lpc[6] * p->m144[4] + lpc[7] * p->m144[5]
            + lpc[8] * p->m144[6] + lpc[9] * p->m144[7] + lpc[10] * p->m144[8] + lpc[11] * p->m144[9];
        out[3] = p->m144[8] = out[3] + lpc[0] * p->m144[9] + lpc[1] * p->m144[10] + lpc[2] * p->m144[11] + lpc[3] * p->m144[0]
            + lpc[4] * p->m144[1] + lpc[5] * p->m144[2] + lpc[6] * p->m144[3] + lpc[7] * p->m144[4]
            + lpc[8] * p->m144[5] + lpc[9] * p->m144[6] + lpc[10] * p->m144[7] + lpc[11] * p->m144[8];
        out[4] = p->m144[7] = out[4] + lpc[0] * p->m144[8] + lpc[1] * p->m144[9] + lpc[2] * p->m144[10] + lpc[3] * p->m144[11]
            + lpc[4] * p->m144[0] + lpc[5] * p->m144[1] + lpc[6] * p->m144[2] + lpc[7] * p->m144[3]
            + lpc[8] * p->m144[4] + lpc[9] * p->m144[5] + lpc[10] * p->m144[6] + lpc[11] * p->m144[7];
        out[5] = p->m144[6] = out[5] + lpc[0] * p->m144[7] + lpc[1] * p->m144[8] + lpc[2] * p->m144[9] + lpc[3] * p->m144[10]
            + lpc[4] * p->m144[11] + lpc[5] * p->m144[0] + lpc[6] * p->m144[1] + lpc[7] * p->m144[2]
            + lpc[8] * p->m144[3] + lpc[9] * p->m144[4] + lpc[10] * p->m144[5] + lpc[11] * p->m144[6];
        out[6] = p->m144[5] = out[6] + lpc[0] * p->m144[6] + lpc[1] * p->m144[7] + lpc[2] * p->m144[8] + lpc[3] * p->m144[9]
            + lpc[4] * p->m144[10] + lpc[5] * p->m144[11] + lpc[6] * p->m144[0] + lpc[7] * p->m144[1]
            + lpc[8] * p->m144[2] + lpc[9] * p->m144[3] + lpc[10] * p->m144[4] + lpc[11] * p->m144[5];
        out[7] = p->m144[4] = out[7] + lpc[0] * p->m144[5] + lpc[1] * p->m144[6] + lpc[2] * p->m144[7] + lpc[3] * p->m144[8]
            + lpc[4] * p->m144[9] + lpc[5] * p->m144[10] + lpc[6] * p->m144[11] + lpc[7] * p->m144[0]
            + lpc[8] * p->m144[1] + lpc[9] * p->m144[2] + lpc[10] * p->m144[3] + lpc[11] * p->m144[4];
        out[8] = p->m144[3] = out[8] + lpc[0] * p->m144[4] + lpc[1] * p->m144[5] + lpc[2] * p->m144[6] + lpc[3] * p->m144[7]
            + lpc[4] * p->m144[8] + lpc[5] * p->m144[9] + lpc[6] * p->m144[10] + lpc[7] * p->m144[11]
            + lpc[8] * p->m144[0] + lpc[9] * p->m144[1] + lpc[10] * p->m144[2] + lpc[11] * p->m144[3];
        out[9] = p->m144[2] = out[9] + lpc[0] * p->m144[3] + lpc[1] * p->m144[4] + lpc[2] * p->m144[5] + lpc[3] * p->m144[6]
            + lpc[4] * p->m144[7] + lpc[5] * p->m144[8] + lpc[6] * p->m144[9] + lpc[7] * p->m144[10]
            + lpc[8] * p->m144[11] + lpc[9] * p->m144[0] + lpc[10] * p->m144[1] + lpc[11] * p->m144[2];
        out[10] = p->m144[1] = out[10] + lpc[0] * p->m144[2] + lpc[1] * p->m144[3] + lpc[2] * p->m144[4] + lpc[3] * p->m144[5]
            + lpc[4] * p->m144[6] + lpc[5] * p->m144[7] + lpc[6] * p->m144[8] + lpc[7] * p->m144[9]
            + lpc[8] * p->m144[10] + lpc[9] * p->m144[11] + lpc[10] * p->m144[0] + lpc[11] * p->m144[1];
        out[11] = p->m144[0] = out[11] + lpc[0] * p->m144[1] + lpc[1] * p->m144[2] + lpc[2] * p->m144[3] + lpc[3] * p->m144[4]
            + lpc[4] * p->m144[5] + lpc[5] * p->m144[6] + lpc[6] * p->m144[7] + lpc[7] * p->m144[8]
            + lpc[8] * p->m144[9] + lpc[9] * p->m144[10] + lpc[10] * p->m144[11] + lpc[11] * p->m144[0];
        out += 12;
    }
}

void CMTBLKDecf::initmut(unsigned char *data, UTALKSTATE_CODA *p, int flag)
{
    // FAKEMATCH: unused stack object; reproduces the target frame (0x28, conversion slot at 0x10)
    volatile int spare;
    int i;
    float mult;

    p->m0 = data + 1;
    p->m8 = 8;
    p->m4 = p->m0[-1];
    if (flag != 0) {
        p->mC = getbits(p, 1);
        p->m10 = 32 - getbits(p, 4);
        p->m14[0] = (float)(getbits(p, 4) + 1) * 8.0f;
        mult = (float)getbits(p, 6) * 0.001f + 1.04f;
        for (i = 1; i < 64; i++) {
            p->m14[i] = p->m14[i - 1] * mult;
        }
        for (i = 0; i < 12; i++) {
            p->m114[i] = 0.0f;
            p->m144[i] = 0.0f;
        }
        for (i = 0; i < 324; i++) {
            p->m174[i] = 0.0f;
        }
    }
}

void decodemut(UTALKSTATE_CODA *p)
{
    float excitation[118];
    float delta[12];
    int multipulse;
    int idx;
    int i;
    int k;
    int j;
    int lag;
    int align;
    int zero;
    float fixedGain;
    float pitchGain;

    idx = getbits(p, 6);
    multipulse = idx < p->m10;
    delta[0] = (lbl_8030144C[idx] - p->m114[0]) * 0.25f;
    for (i = 1; i < 4; i++) {
        delta[i] = (lbl_8030144C[getbits(p, 6)] - p->m114[i]) * 0.25f;
    }
    for (i = 4; i < 12; i++) {
        delta[i] = (lbl_8030144C[getbits(p, 5) + 16] - p->m114[i]) * 0.25f;
    }
    for (k = 0; k < 4; k++) {
        lag = (k + 2) * 108 - getbits(p, 8);
        pitchGain = (float)getbits(p, 4) * 0.06666667f;
        fixedGain = p->m14[getbits(p, 6)];
        if (p->mC == 0) {
            readsamples(p, multipulse, &excitation[5], 1);
        } else {
            align = getbits(p, 1);
            zero = getbits(p, 1);
            readsamples(p, multipulse, &excitation[5 + align], 2);
            if (zero != 0) {
                for (j = 0; j < 108; j += 2) {
                    excitation[j - (align - 6)] = 0.0f;
                }
            } else {
                for (j = 0; j < 5; j++) {
                    excitation[j] = 0.0f;
                }
                for (j = 113; j < 118; j++) {
                    excitation[j] = 0.0f;
                }
                interpolate(excitation - (align - 6));
                fixedGain *= 0.5f;
            }
        }
        for (j = 0; j < 108; j++) {
            p->m684[k * 108 + j] = fixedGain * excitation[5 + j] + pitchGain * p->m174[lag + j];
        }
    }
    lbl_80301000(p->m174, &p->m684[108], 1296);
    for (i = 0; i < 12; i++) {
        p->m114[i] += delta[i];
    }
    filter(p, 0, 1);
    for (i = 0; i < 12; i++) {
        p->m114[i] += delta[i];
    }
    filter(p, 12, 1);
    for (i = 0; i < 12; i++) {
        p->m114[i] += delta[i];
    }
    filter(p, 24, 1);
    for (i = 0; i < 12; i++) {
        p->m114[i] += delta[i];
    }
    filter(p, 36, 33);
}

}

extern "C" {

Codec_8027CF9C *fn_8027CF9C(void *memory)
{
    Codec_8027CF9C *p = (Codec_8027CF9C *)memory;

    p->mD44 = 0;
    p->mD54 = 0;
    p->mD58 = 0;
    p->mD50 = 0;
    p->mD52 = 0;
    p->mD5C = 0;
    p->mD60 = 0;
    p->mD64 = 1;
    return p;
}

int fn_8027CFCC(Codec_8027CF9C *p, unsigned char *data, int size, int count)
{
    if (data == 0 || p->mD54 != 0) {
        return -1;
    }
    p->mD54 = count;
    p->mD58 = size;
    p->mD44 = 0;
    p->mD48 = data;
    if (p->mD64 == 1) {
        if (*data == 0xEE) {
            p->mD60 = 1;
        } else {
            p->mD60 = 0;
        }
        p->initmut(data + 1, p, p->mD5C);
    } else {
        p->initmut(data, p, p->mD5C);
    }
    return 0;
}

int CodecReadSamples(Codec_8027CF9C *p, float **dst, int count)
{
    int total;
    int left;
    int n;
    unsigned char *src;
    float *out;
    short i;
    short value;
    unsigned char *d;
    unsigned char *e;
    unsigned char *f;

    n = p->mD44;
    if (p->mD54 == 0) {
        return 0;
    }
    p->mD4C = *dst;
    total = p->mD54;
    if (total > count) {
        total = count;
    }
    left = total;
    while (left > 0) {
        if (left < n) {
            n = left;
        }
        lbl_80301000(p->mD4C, &p->m684[432 - p->mD44], n * 4);
        left -= n;
        p->mD4C += n;
        p->mD44 -= n;
        if (left <= 0) {
            break;
        }
        Snd::decodemut(p);
        p->mD44 = 432;
        n = 432;
        if (p->mD64 == 1) {
            p->mD48 = p->m0 - 1;
            src = p->mD48;
            if (p->mD60 != 0) {
                d = (unsigned char *)&p->mD52;
                d[0] = src[0];
                d[1] = src[1];
                src += 2;
                e = (unsigned char *)&p->mD50;
                e[0] = src[0];
                e[1] = src[1];
                src += 2;
                p->mD48 = p->mD48 + p->mD50 * 2 + 4;
                p->m0 = p->mD48 + 1;
                out = &p->m684[p->mD52];
                for (i = 0; i < p->mD50; i++) {
                    f = (unsigned char *)&value;
                    f[0] = src[0];
                    f[1] = src[1];
                    src += 2;
                    *out++ = value;
                }
                p->mD60 = 0;
            }
            if (*p->mD48 == 0xEE) {
                p->mD60 = 1;
            }
            p->m8 = 8;
            p->m4 = *p->m0;
            p->m0++;
        }
    }
    p->mD54 -= total;
    return total;
}

Format_8027D284 fn_8027D284(Codec_8027CF9C *p)
{
    Format_8027D284 format;

    format.m0 = p->m4;
    format.m8 = p->mD5C;
    format.m4 = p->mD44;
    return format;
}

void fn_8027D2C4(Codec_8027CF9C *p, Format_8027D284 *format, int flags)
{
    p->m4 = format->m0;
    p->mD5C = format->m8;
    p->mD44 = format->m4;
}

void fn_8027D2E0(Codec_8027CF9C *p, int mode)
{
    p->mD64 = mode;
}

}
