#include "../src/recovered/framework/audio_waveout.h"
#include "../src/recovered/framework/resource_manager.h"
#include "../src/shim/win16_resource.h"
#include "../src/shim/win16_waveout.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    SBResourceManager *res_mgr;
    SBTimer60Hz *timer;
    SBSoundManager *snd_mgr;
    SBSoundChannel *chan;
    uint32_t t0;
    uint32_t t1;
    uint32_t interval = 0;

    /* 1. Initialize Win16 Resource and WaveOut Shims */
    assert(sb_win16_resource_shim_init("original/BLIZZARD.EXE", "original/SB_CDRES.DLL") == 0);
    sb_win16_setTimeMs(1000);
    sb_win16_waveOut_resetStats();

    /* 2. Check system requirements (`1008:6a28`) */
    assert(System_CheckRequirements_1008_6a28() == 1);

    /* 3. Initialize 60 Hz Timer (`1010:446a` at `DAT_1060_0a1e`) */
    timer = (SBTimer60Hz *)NewObject_1008_ef2c(sizeof(SBTimer60Hz));
    assert(timer != NULL);
    g_DAT_1060_0a1e_timer60Hz = CTimer60Hz_Ctor_1010_446a(timer);
    t0 = Timer_GetTicks60Hz_1010_07ae();
    assert(t0 == 60); /* 1000 ms * 3 / 50 = 60 ticks */

    /* Advancing 500 ms (50 x 10ms steps) with `<` condition advances 49 steps = 29 ticks (60 + 29 = 89),
     * and 1 more ms crosses the 50th step to reach 90 ticks.
     */
    sb_win16_advanceTimeMs(500);
    t1 = Timer_GetTicks60Hz_1010_07ae();
    assert(t1 == 89);
    sb_win16_advanceTimeMs(1);
    assert(Timer_GetTicks60Hz_1010_07ae() == 90);

    /* 4. Initialize Resource Manager (`1008:2bf4` / `1010:286c`) and Sound Manager (`1008:21ec`) */
    res_mgr = CResourceManager_CreateGlobal_1008_2bf4_part();
    assert(res_mgr != NULL);

    snd_mgr = (SBSoundManager *)NewObject_1008_ef2c(sizeof(SBSoundManager));
    assert(snd_mgr != NULL);
    CSoundManager_Init_1008_21ec(snd_mgr);
    assert(g_DAT_1060_07c8_soundMgr == snd_mgr);
    assert(g_DAT_1060_07cc_wavePlayer != NULL);
    assert(CSoundManager_IsActive_1008_22ba(snd_mgr) == 0);

    /* 5. Play real BSW audio resource `'snd '` #131 from `SB_CDRES.DLL` (`1008:22d6`) */
    CSoundManager_PlaySoundResource_1008_22d6(snd_mgr, 131);
    assert(CSoundManager_IsActive_1008_22ba(snd_mgr) == 1);
    assert(CSoundManager_GetCurrentSoundId_1008_2530(snd_mgr) == 131);
    assert(CSoundManager_IsHardwareBusy_1008_22ac(snd_mgr) == 1);
    assert(sb_win16_waveOut_getLastSampleRate() == 11025);
    assert(sb_win16_waveOut_getTotalBytesWritten() > 20000);

    /* Wait until playback finishes (`1008:2436`) */
    CSoundManager_WaitUntilDone_1008_2436(snd_mgr);
    assert(CSoundManager_IsHardwareBusy_1008_22ac(snd_mgr) == 0);
    assert(CSoundManager_IsActive_1008_22ba(snd_mgr) == 0);

    /* 6. Test `CSoundChannel_1010_bdd4` sequence playback (`1020:27d8..2a96`) */
    chan = (SBSoundChannel *)NewObject_1008_ef2c(sizeof(SBSoundChannel));
    assert(chan != NULL);
    CSoundChannel_Init(chan);
    chan->interval_ticks_04 = 10;
    CSoundChannel_AddSound_1020_2928(chan, 130);
    CSoundChannel_AddSound_1020_2928(chan, 131);
    CSoundChannel_SetPrimarySound_1020_29b6(chan, 129);
    assert(chan->sound_count_22 == 2);
    assert(chan->primary_snd_id_20 == 129);

    assert(CSoundChannel_IsReady_1020_28e0(chan) == 1);
    assert(CSoundChannel_ExecuteNext_1020_2810(chan, &interval) == 130);
    assert(interval == 10);
    assert(CSoundManager_GetCurrentSoundId_1008_2530(snd_mgr) == 130);
    CSoundManager_Stop_1008_2462(snd_mgr);

    /* Cleanup */
    CSoundChannel_Dtor_1020_27d8(chan);
    CSoundManager_Dtor_1008_225a(snd_mgr);
    CTimer60Hz_Dtor_1010_44d6(timer);
    DeleteObject_1008_ef80(timer);
    g_DAT_1060_0a1e_timer60Hz = NULL;
    CResourceMap_Dtor_1010_29ea(res_mgr);
    sb_win16_resource_shim_shutdown();

    printf("test_audio_waveout: 60Hz timer, BSW 11.025 kHz PCM decoding, CSoundManager, and CSoundChannel verified!\n");
    return 0;
}
