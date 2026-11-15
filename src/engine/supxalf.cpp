/* Sound filter (supxalf.cpp): the unpack filter for looped EA-XA compressed sample blocks. Its init
   either decodes the whole block in memory or, for streamed voices, records the packet loop points. */

#include "engine/sndsys.h"

extern "C" {

int SFILTER_unpackxalf(Node_80279D90 *node, int count, float *a, float *b, int flag)
{
    // FAKEMATCH: unused stack object; reproduces the target frame (0xC0, scratch buffer at 0x10)
    volatile int spare;
    Decoder_8027B7B0 *decoder;
    float scratch[28];
    Pair_8027BFDC saved;
    Pair_8027BFDC state;
    float *discard;
    int total;
    int n;
    int got;
    int pos;

    decoder = (Decoder_8027B7B0 *)node;
    total = 0;
    if (decoder->m30 <= 2) {
        if (decoder->m40 == 0) {
            n = decoder->m44 / 28 * 28;
            fn_8027BDE8(decoder->m1C, (char *)decoder->m34, n * 4, n);
            decoder->m40 = 1;
        }
        while (count != 0) {
            got = fn_8027BE24(decoder->m1C, &b, count);
            b += got;
            decoder->m20 += got;
            total += got;
            if (got < count) {
                count -= got;
                n = decoder->m44 % 28;
                pos = decoder->m44 - n;
                decoder->m20 = pos;
                fn_8027BDE8(decoder->m1C, (char *)decoder->m34 + pos / 28 * 15, (decoder->m48 - pos + 1) * 4,
                            decoder->m48 - pos + 1);
                if (decoder->m3C == 0) {
                    saved = fn_8027BFDC(decoder->m1C);
                    state = saved;
                    decoder->m4C = state.m0;
                    decoder->m50 = state.m4;
                    got = fn_8027BE24(decoder->m1C, &b, count);
                    b += got;
                    count -= got;
                    total += got;
                    decoder->m20 += got;
                    decoder->m3C = 1;
                } else {
                    state.m0 = decoder->m4C;
                    state.m4 = decoder->m50;
                    fn_8027C00C(decoder->m1C, &state);
                    discard = scratch;
                    got = fn_8027BE24(decoder->m1C, &discard, n);
                    decoder->m20 += got;
                }
            } else {
                count -= got;
            }
        }
    } else {
        while (count > 0) {
            got = fn_8027BE24(decoder->m1C, &b, count);
            b += got;
            decoder->m20 += got;
            count -= got;
            total += got;
            if (got < count) {
                decoder->m20 = decoder->m24;
                fn_8027BDE8(decoder->m1C, (char *)decoder->m2C, (decoder->m28 - decoder->m24 + 1) * 4,
                            decoder->m28 - decoder->m24 + 1);
            }
        }
    }
    return total;
}

int SFILTER_unpackgetframexalf(Node_80279D90 *node)
{
    return ((Decoder_8027B7B0 *)node)->m20;
}

void SFILTER_unpackxalfrestore(Node_80279D90 *node)
{
    Decoder_8027B7B0 *decoder;

    decoder = (Decoder_8027B7B0 *)node;
    if (decoder->m1C != 0) {
        fn_8027BD94(decoder->m1C);
    }
}

void SFILTER_unpackxalfinit(Node_80279D90 *node, Params_80275F80 *params)
{
    Decoder_8027B7B0 *decoder;

    decoder = (Decoder_8027B7B0 *)node;
    node->m0 = SFILTER_unpackxalf;
    node->m4 = SFILTER_unpackxalfrestore;
    // FAKEMATCH: store order found by search (the +0x2C store before the +0x20 one)
    decoder->m2C = params->m4;
    decoder->m20 = 0;
    decoder->m30 = params->m24;
    decoder->m24 = params->m10;
    decoder->m28 = params->m14;
    decoder->m1C = fn_8027BDC0(fn_8027BD68(168));
    params->m28 = SFILTER_unpackgetframexalf;
    if (decoder->m30 <= 2) {
        decoder->m34 = params->m0;
        decoder->m38 = params->mC;
        decoder->m3C = 0;
        decoder->m40 = 0;
        decoder->m44 = params->m10;
        decoder->m48 = params->m14;
    } else {
        fn_8027BDE8(decoder->m1C, (char *)params->m0, decoder->m24 * 4, decoder->m24);
    }
}

}
