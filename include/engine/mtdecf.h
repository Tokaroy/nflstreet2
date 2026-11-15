#ifndef ENGINE_MTDECF_H
#define ENGINE_MTDECF_H

namespace Snd {

/* The MT (MicroTalk) decode state: the bit reader (+0x0 byte pointer, +0x4 bit buffer, +0x8 bit
   count), the stream header (+0xC reduced-bandwidth flag, +0x10 multi-pulse threshold, +0x14 the 64
   fixed gains), the 12 reflection coefficients +0x114, the 12-sample synthesis history +0x144, the
   324-sample adaptive codebook +0x174 and the 432-sample decoded frame +0x684 that follows it. */
struct UTALKSTATE_CODA {
    unsigned char *m0;
    unsigned int m4;
    int m8;
    int mC;
    int m10;
    float m14[64];
    float m114[12];
    float m144[12];
    float m174[324];
    float m684[432];
};

/* 3432-byte MT block decoder used by src/engine/mtdecf.cpp (the MT unpack filter constructors in
   src/engine/supmutf.c, src/engine/supmutlf.c and src/engine/supmutpf.c allocate it with
   fn_8027C020(3432)): the decode state, then the delivery fields +0xD44-+0xD67. */
class CMTBLKDecf : public UTALKSTATE_CODA {
public:
    void initmut(unsigned char *data, UTALKSTATE_CODA *state, int header);

    int mD44;
    unsigned char *mD48;
    float *mD4C;
    short mD50;
    short mD52;
    int mD54;
    int mD58;
    int mD5C;
    int mD60;
    int mD64;
};

}

typedef Snd::CMTBLKDecf Codec_8027CF9C;

typedef struct Format_8027D284 {
    int m0;
    int m4;
    int m8;
} Format_8027D284;

#ifdef __cplusplus
extern "C" {
#endif

Codec_8027CF9C *fn_8027CF9C(void *memory);
int fn_8027CFCC(Codec_8027CF9C *p, unsigned char *data, int size, int count);
int CodecReadSamples(Codec_8027CF9C *p, float **dst, int count);
Format_8027D284 fn_8027D284(Codec_8027CF9C *p);
void fn_8027D2C4(Codec_8027CF9C *p, Format_8027D284 *format, int flags);
void fn_8027D2E0(Codec_8027CF9C *p, int mode);

#ifdef __cplusplus
}
#endif

#endif
