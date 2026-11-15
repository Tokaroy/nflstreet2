/* Sound packet players (spktplay.c): the streamed players that queue decoded audio packets per channel
   for the voices that play them: create, submit, submit space, frames outstanding, purge, stop, destroy,
   the packet fetch of the decoders, frame release and the callback flush. SNDPKTPLAY_start is not
   reconstructed yet. Names from the GoldenEye: Rogue Agent, Medal of Honor: European Assault and
   NASCAR 2005 symbol sets (spktplay.c). */

#include "engine/sndsys.h"

extern "C" {

int fn_802743A0(int index, void *stream);
int SNDstop(int handle);
void SNDPKTPLAYI_flushcallbackdata(void);
int SNDPKTPLAY_purge(int index, int a, int b);

int SNDPKTPLAYI_gethighchannel(int index, int count)
{
    int i;
    int value = -1;
    int result = -1;
    for (i = 0; i < count; i++) {
        if (lbl_803A6C24.m1CC[index].m4[i] > value) {
            value = lbl_803A6C24.m1CC[index].m4[i];
            result = i;
        }
    }
    return result;
}

int SNDPKTPLAYI_overhead(int count)
{
    return count * 32 + 156;
}

int SNDPKTPLAY_overhead(int count)
{
    return SNDPKTPLAYI_overhead(count) + fn_80274398();
}

int SNDPKTPLAY_create(void (*release)(int value, void *context), void (*callback)(int index, int value, void *context),
                      void *context, void *memory,
                      unsigned int size)
{
    SndPacketPlayer_t *stream;
    void *voice;
    int i = 0;

    SNDSYS_entercritical();
    for (i = 0; i < lbl_803A6C24.m1C.m22; i++) {
        if (sndpps.m304[i] == 0) {
            goto found;
        }
    }
    SNDSYS_leavecritical();
    return -9;
found:
    if (fn_802743A0(i, memory) < 0) {
        SNDSYS_leavecritical();
        return -6;
    }
    voice = memory;
    memory = (char *)memory + fn_80274398();
    size -= fn_80274398();
    stream = (SndPacketPlayer_t *)memory;
    stream->m40 = (size - 156) / 32;
    stream->m50 = voice;
    stream->m54 = release;
    stream->m58 = callback;
    stream->m5C = context;
    stream->m0 = -1;
    sndpps.m304[i] = stream;
    SNDSYS_leavecritical();
    return i;
}

int SNDPKTPLAY_submit(int index, SndPacket_t *packet)
{
    SndPacketPlayer_t *stream = sndpps.m304[index];
    SndPacketEntry_t *entry;
    int id;
    int i;

    for (i = 0; i < stream->m62; i++) {
        if (stream->m34[i] >= stream->m40 - 1) {
            return -13;
        }
    }
    entry = &stream->m7C[stream->m46];
    entry->m4 = packet->m4;
    entry->m4b = packet->m4b;
    entry->m0 = stream->m4;
    for (i = 0; i < stream->m62; i++) {
        entry->m8[i] = packet->mC[i];
        stream->m34[i]++;
    }
    stream->m48 += packet->m4;
    id = stream->m4;
    stream->m4++;
    if (++stream->m46 >= stream->m40) {
        stream->m46 = 0;
    }
    return id;
}

int SNDPKTPLAY_submitspace(int index)
{
    SndPacketPlayer_t *stream = sndpps.m304[index];
    int value = 0;
    int i;
    for (i = 0; i < stream->m62; i++) {
        if (stream->m34[i] >= value) {
            value = stream->m34[i];
        }
    }
    return stream->m40 - value - 1;
}

int SNDPKTPLAY_framesoutstanding(int index)
{
    SndPacketPlayer_t *stream;

    stream = sndpps.m304[index];
    return stream->m48 + stream->m4C;
}

int SNDPKTPLAY_purge(int index, int a, int b)
{
    return -15;
}

int SNDPKTPLAY_stop(int index)
{
    SndPacketPlayer_t *stream = sndpps.m304[index];
    int i = 0;

    SNDstop(stream->m0);
    SNDPKTPLAYI_flushcallbackdata();
    SNDPKTPLAY_purge(index, 0, 0x7FFFFFFF);
    stream->m0 = -1;
    for (i = 0; i < stream->m62; i++) {
        if (stream->m64[i] != 0) {
            SNDMEMI_free(stream->m64[i]);
        }
    }
    return 0;
}

int SNDPKTPLAY_destroy(int index)
{
    SNDSYS_entercritical();
    fn_8027474C(index);
    sndpps.m304[index] = 0;
    SNDSYS_leavecritical();
    return 0;
}

unsigned char *SNDPKTPLAYI_get(int index, int channel, int *count, int *last)
{
    SndPacketPlayer_t *stream = sndpps.m304[index];
    SndPacketEntry_t *oldest;
    SndPacketEntry_t *entry;
    SndPacketEvent_t *event;
    unsigned char *data;
    unsigned char *result;
    int i;
    int all = 1;
    int done = 1;

    for (i = 0; i < stream->m62; i++) {
        if (stream->m10[i] <= stream->m8) {
            all = 0;
        }
    }
    if (all) {
        oldest = &stream->m7C[stream->m44];
        if (stream->m54 != 0) {
            event = &sndpps.m4[sndpps.m0];
            event->m0 = 1;
            event->m2 = index;
            event->m4 = (int)oldest->m8[0];
            sndpps.m0++;
        }
        stream->m44++;
        stream->m8++;
        if (stream->m44 >= stream->m40) {
            stream->m44 = 0;
        }
    }
    if (stream->m34[channel] == 0) {
        return 0;
    }
    entry = &stream->m7C[stream->m28[channel]];
    *count = entry->m4;
    *last = entry->m4b;
    stream->m28[channel]++;
    stream->m10[channel]++;
    i = 0;
    do {
        if (stream->m10[i] <= stream->mC) {
            done = 0;
        }
        i++;
    } while (done == 1 && i < stream->m62);
    if (done) {
        stream->mC++;
        stream->m48 -= entry->m4;
        stream->m4C += entry->m4;
    }
    if (stream->m28[channel] >= stream->m40) {
        stream->m28[channel] = 0;
    }
    stream->m34[channel]--;
    result = (unsigned char *)-1;
    data = (unsigned char *)entry->m8[channel];
    if (data != 0) {
        result = data;
    }
    return result;
}

void SNDPKTPLAYI_freeframes(int index, int channel, int value)
{
    SndPacketPlayer_t *stream;
    SndPacketEvent_t *event;

    stream = sndpps.m304[index];
    if (channel == stream->m42) {
        stream->m4C -= value;
        if (stream->m58 != 0) {
            event = &sndpps.m4[sndpps.m0];
            event->m0 = 0;
            event->m2 = index;
            event->m4 = value;
            sndpps.m0++;
        }
    }
}

void SNDPKTPLAYI_flushcallbackdata(void)
{
    SndPacketEvent_t *event;
    SndPacketPlayer_t *stream;
    int i;

    for (i = 0; i < sndpps.m0; i++) {
        event = &sndpps.m4[i];
        stream = sndpps.m304[event->m2];
        if (event->m0 == 0) {
            stream->m58(event->m2, event->m4, stream->m5C);
        } else {
            stream->m54(event->m4, stream->m5C);
        }
    }
    sndpps.m0 = 0;
}

}
