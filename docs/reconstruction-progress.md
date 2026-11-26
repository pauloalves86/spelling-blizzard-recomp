# Reconstruction progress

Generated from Ghidra 12.2 function inventory for `original/BLIZZARD.EXE` (SHA-256 `ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2`).
All functions in the five initialized code segments are listed in [reconstruction-status.csv](reconstruction-status.csv) and classified by subsystem in [subsystem-map.md](subsystem-map.md).

## Function inventory

- Code-segment functions: **1422** (180,530 bytes)
- Decompiled & tested functions (Path 2 C + Win16 ABI): **415 / 1422 (29.18%)**
- Decompiled & tested code bytes: **41,235 / 180,530 (22.84%)**
  - Clean C decompiled against Win16 shim (`decompiled`): **412** (41,136 bytes)
  - Win16 OMF ABI-checked entrypoints (`win16-abi-checked`): **3** (99 bytes)
- Partial original functions: **1** (340 bytes)
- Remaining functions (mapped in `docs/subsystem-map.md`): **1006**

## Decompiled Functions (Path 2 Clean C + Thin Win16 Shim)

| Original entry | C source | Recovered behavior |
|---|---|---|
| `1008:018c` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:018c`). |
| `1008:0190` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:0190`). |
| `1008:01c0` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:01c0`). |
| `1008:01c4` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:01c4`). |
| `1008:01c8` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:01c8`). |
| `1008:020e` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:020e`). |
| `1008:0212` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:0212`). |
| `1008:0216` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:0216`). |
| `1008:0226` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:0226`). |
| `1008:02ba` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:02ba`). |
| `1008:02be` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:02be`). |
| `1008:02c2` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:02c2`). |
| `1008:02c6` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:02c6`). |
| `1008:0330` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:0330`). |
| `1008:0398` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:0398`). |
| `1008:039c` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:039c`). |
| `1008:0596` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:0596`). |
| `1008:0602` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:0602`). |
| `1008:0606` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:0606`). |
| `1008:0684` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:0684`). |
| `1008:0694` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:0694`). |
| `1008:06c6` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:06c6`). |
| `1008:06ca` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:06ca`). |
| `1008:06ce` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:06ce`). |
| `1008:06ec` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:06ec`). |
| `1008:06fc` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:06fc`). |
| `1008:0700` | `src/recovered/framework/ui_framework.c` | Application base lifecycle & main loop (`CApplication_1008_2e32`) (`1008:0700`). |
| `1008:225a` | `src/recovered/framework/audio_waveout.c` | `CSoundManager_1008_7edc` virtual destructor (`CSoundManager_Dtor_1008_225a`). |
| `1008:22ac` | `src/recovered/framework/audio_waveout.c` | Checks if `waveOut` playback is still active (`CSoundManager_IsHardwareBusy_1008_22ac`). |
| `1008:22ba` | `src/recovered/framework/audio_waveout.c` | Returns sound manager active flag `+0x16` (`CSoundManager_IsActive_1008_22ba`). |
| `1008:22c8` | `src/recovered/framework/audio_waveout.c` | Returns sound interrupted flag `+0x0a` (`CSoundManager_WasInterrupted_1008_22c8`). |
| `1008:22d6` | `src/recovered/framework/audio_waveout.c` | Loads `'snd '` (`0x03ee`) BSW resource and plays via `waveOut` (`CSoundManager_PlaySoundResource_1008_22d6`). |
| `1008:23b2` | `src/recovered/framework/audio_waveout.c` | Waits for sound completion or user key/mouse interrupt (`CSoundManager_WaitOrInterrupt_1008_23b2`). |
| `1008:2436` | `src/recovered/framework/audio_waveout.c` | Waits for sound playback to complete (`CSoundManager_WaitUntilDone_1008_2436`). |
| `1008:2462` | `src/recovered/framework/audio_waveout.c` | Stops active sound and restores resource handle flags (`CSoundManager_Stop_1008_2462`). |
| `1008:24ec` | `src/recovered/framework/audio_waveout.c` | Updates 10-minute (36000 tick) idle sound timer (`CSoundManager_IdleTimerCheck_1008_24ec`). |
| `1008:2530` | `src/recovered/framework/audio_waveout.c` | Returns active `'snd '` resource ID (`CSoundManager_GetCurrentSoundId_1008_2530`). |
| `1008:2bbc` | `src/recovered/framework/resource_manager.c` | Loads `SB_CDRES.DLL` into `DAT_1060_001c` (`ResMgr_LoadCdResLibrary_1008_2bbc`). |
| `1008:2bcc` | `src/recovered/framework/resource_manager.c` | Frees `SB_CDRES.DLL` module handle (`ResMgr_FreeCdResLibrary_1008_2bcc`). |
| `1008:2bdc` | `src/recovered/framework/resource_manager.c` | Routes `'WIND'` to `BLIZZARD.EXE` and all other FourCCs to `SB_CDRES.DLL` (`ResMgr_GetResourceModule_1008_2bdc`). |
| `1008:2bf4` | `src/recovered/framework/ui_framework.c` | WinMain application startup & main loop entry (`Application_WinMainEntry_1008_2bf4`) (`1008:2bf4`). |
| `1008:31b8` | `src/recovered/framework/ui_framework.c` | Desktop root view (`CDesktop_1008_7d10`) (`1008:31b8`). |
| `1008:31da` | `src/recovered/framework/ui_framework.c` | Desktop root view (`CDesktop_1008_7d10`) (`1008:31da`). |
| `1008:31fc` | `src/recovered/framework/ui_framework.c` | Desktop root view (`CDesktop_1008_7d10`) (`1008:31fc`). |
| `1008:3218` | `src/recovered/framework/ui_framework.c` | Desktop root view (`CDesktop_1008_7d10`) (`1008:3218`). |
| `1008:326e` | `src/recovered/framework/graphics_gdi.c` | Initializes 236-entry `CPaletteManager_1008_8278` at `DAT_1060_0198` (`CPaletteManager_Init_1008_326e`). |
| `1008:3464` | `src/recovered/framework/graphics_gdi.c` | `CPaletteManager_1008_8278` virtual destructor (`CPaletteManager_Dtor_1008_3464`). |
| `1008:34bc` | `src/recovered/framework/graphics_gdi.c` | Crossfades current palette to target palette (`CPaletteManager_FadeToCurrent_1008_34bc`). |
| `1008:34e6` | `src/recovered/framework/graphics_gdi.c` | Crossfades active palette range to black (`CPaletteManager_FadeOut_1008_34e6`). |
| `1008:3596` | `src/recovered/framework/graphics_gdi.c` | Applies target CLUT entries across range (`CPaletteManager_ApplyTargetRange_1008_3596`). |
| `1008:3624` | `src/recovered/framework/graphics_gdi.c` | Applies black entries across range (`CPaletteManager_ApplyBlackRange_1008_3624`). |
| `1008:36b2` | `src/recovered/framework/graphics_gdi.c` | Loads `'clut'` (`0x03ef`) primary and companion palette resources (`CPaletteManager_LoadClutResource_1008_36b2`). |
| `1008:382e` | `src/recovered/framework/graphics_gdi.c` | Saves current 236-entry palette and realizes (`CPaletteManager_SaveAndRealize_1008_382e`). |
| `1008:3950` | `src/recovered/framework/graphics_gdi.c` | Restores saved 236-entry palette and realizes (`CPaletteManager_RestoreAndRealize_1008_3950`). |
| `1008:3a74` | `src/recovered/framework/graphics_gdi.c` | Resets palette to initial system entries (`CPaletteManager_ResetToInitial_1008_3a74`). |
| `1008:3b44` | `src/recovered/framework/graphics_gdi.c` | Rotates/cycles palette range by 1 slot and calls `AnimatePalette` (`CPaletteManager_CycleRange_1008_3b44`). |
| `1008:3c5c` | `src/recovered/framework/graphics_gdi.c` | Finds palette index matching 16-bit Mac RGB triplet (`CPaletteManager_FindColorIndex_1008_3c5c`). |
| `1008:3cd2` | `src/recovered/framework/graphics_gdi.c` | 32-step interpolated palette crossfade (`CPaletteManager_CrossfadeRange_1008_3cd2`). |
| `1008:3e34` | `src/recovered/framework/graphics_gdi.c` | Animates all 236 palette entries (`CPaletteManager_AnimateAll_1008_3e34`). |
| `1008:5646` | `src/recovered/framework/graphics_gdi.c` | Initializes blank offscreen `CPictSurface_1008_6448` (`CPictSurface_InitBlank_1008_5646`). |
| `1008:56a8` | `src/recovered/framework/graphics_gdi.c` | `CPictSurface_1008_6448` virtual destructor (`CPictSurface_Dtor_1008_56a8`). |
| `1008:56f8` | `src/recovered/framework/graphics_gdi.c` | Copies screen rectangle into surface for mode `0`/`0x24` (`CPictSurface_CopyToScreenIfMode_1008_56f8`). |
| `1008:5724` | `src/recovered/framework/graphics_gdi.c` | `CPictSurface_1008_6448` slot `+0x14` no-op (`CPictSurface_Noop14_1008_5724`). |
| `1008:5728` | `src/recovered/framework/graphics_gdi.c` | Dispatches opaque (`0`) or transparent (`0x24`) surface-to-surface blit (`CPictSurface_BlitToSurfaceByMode_1008_5728`). |
| `1008:5786` | `src/recovered/framework/graphics_gdi.c` | Dispatches opaque (`0`) or transparent (`0x24`) surface-to-screen blit (`CPictSurface_BlitToScreenByMode_1008_5786`). |
| `1008:57ce` | `src/recovered/framework/graphics_gdi.c` | Stores surface bounds and creates memory DC (`CPictSurface_CreateDc_1008_57ce`). |
| `1008:5804` | `src/recovered/framework/graphics_gdi.c` | Loads and decodes `'PICT'` (`0x03f3`) resource into surface (`CPictSurface_LoadPictResource_1008_5804`). |
| `1008:5a0a` | `src/recovered/framework/graphics_gdi.c` | Fills surface rectangle with black (`CPictSurface_FillBlack_1008_5a0a`). |
| `1008:5a2a` | `src/recovered/framework/graphics_gdi.c` | Clips source and destination blit rectangles against bounds (`CPictSurface_ClipRects_1008_5a2a`). |
| `1008:5b78` | `src/recovered/framework/graphics_gdi.c` | Fills surface rectangle with palette color index (`CPictSurface_FillColorRect_1008_5b78`). |
| `1008:5c68` | `src/recovered/framework/graphics_gdi.c` | Copies rectangle from screen DC into surface (`CPictSurface_CopyFromScreen_1008_5c68`). |
| `1008:5c88` | `src/recovered/framework/graphics_gdi.c` | Opaque blit from surface to target surface (`CPictSurface_CopyToSurface_1008_5c88`). |
| `1008:5caa` | `src/recovered/framework/graphics_gdi.c` | Opaque blit from surface to screen DC (`CPictSurface_CopyToScreen_1008_5caa`). |
| `1008:5cca` | `src/recovered/framework/graphics_gdi.c` | Fills screen DC rectangle with black (`CPictSurface_ClearScreenRect_1008_5cca`). |
| `1008:5cea` | `src/recovered/framework/graphics_gdi.c` | Transparent blit from surface to target surface (`CPictSurface_TransparentBlitToSurface_1008_5cea`). |
| `1008:5d36` | `src/recovered/framework/graphics_gdi.c` | Transparent blit from surface to screen DC (`CPictSurface_TransparentBlitToScreen_1008_5d36`). |
| `1008:5d7c` | `src/recovered/framework/graphics_gdi.c` | Returns surface `HBITMAP` (`CPictSurface_GetHBitmap_1008_5d7c`). |
| `1008:5d8a` | `src/recovered/framework/graphics_gdi.c` | Returns surface `HDC` (`CPictSurface_GetHDc_1008_5d8a`). |
| `1008:5d98` | `src/recovered/framework/graphics_gdi.c` | Reads 8-bit palette index at `(x, y)` in surface (`CPictSurface_GetPixel_1008_5d98`). |
| `1008:5e14` | `src/recovered/framework/graphics_gdi.c` | Initializes `CLayeredOffscreen_1008_b310` from `'PICT'` resource (`CLayeredOffscreen_InitFromPict_1008_5e14`). |
| `1008:5ed4` | `src/recovered/framework/graphics_gdi.c` | `CLayeredOffscreen_1008_b310` slot `+0x64` forwarder (`CLayeredOffscreen_Forward18_1008_5ed4`). |
| `1008:5f02` | `src/recovered/framework/graphics_gdi.c` | `CLayeredOffscreen_1008_b310` slot `+0x68` forwarder (`CLayeredOffscreen_Forward1c_1008_5f02`). |
| `1008:5f2c` | `src/recovered/framework/graphics_gdi.c` | `CLayeredOffscreen_1008_b310` slot `+0x6c` forwarder (`CLayeredOffscreen_Forward38_1008_5f2c`). |
| `1008:5f52` | `src/recovered/framework/graphics_gdi.c` | `CLayeredOffscreen_1008_b310` slot `+0x70` forwarder (`CLayeredOffscreen_Forward3c_1008_5f52`). |
| `1008:5f6e` | `src/recovered/framework/graphics_gdi.c` | Initializes `CMaskedPictSurface_1008_b384` and precomputes 1-bit mask (`CMaskedPictSurface_InitFromPict_1008_5f6e`). |
| `1008:604a` | `src/recovered/framework/graphics_gdi.c` | `CMaskedPictSurface_1008_b384` virtual destructor (`CMaskedPictSurface_Dtor_1008_604a`). |
| `1008:60b2` | `src/recovered/framework/graphics_gdi.c` | `CMaskedPictSurface_1008_b384` slot `+0x64` forwarder (`CMaskedPictSurface_Forward18_1008_60b2`). |
| `1008:60e0` | `src/recovered/framework/graphics_gdi.c` | `CMaskedPictSurface_1008_b384` slot `+0x68` forwarder (`CMaskedPictSurface_Forward1c_1008_60e0`). |
| `1008:610a` | `src/recovered/framework/graphics_gdi.c` | Blits masked surface to target surface (`CMaskedPictSurface_BlitToSurface_1008_610a`). |
| `1008:612e` | `src/recovered/framework/graphics_gdi.c` | Blits masked surface to screen DC (`CMaskedPictSurface_BlitToScreen_1008_612e`). |
| `1008:614e` | `src/recovered/framework/graphics_gdi.c` | Initializes tiled surface from `'PICT'` resource (`CTiledPictSurface_InitFromPict_1008_614e`). |
| `1008:6246` | `src/recovered/framework/graphics_gdi.c` | Repeats tile pattern across full surface bounds (`CTiledPictSurface_CreateTiled_1008_6246`). |
| `1008:6436` | `src/recovered/framework/graphics_gdi.c` | `CPictSurface_1008_6448` slot `+0x0c` no-op (`CPictSurface_Noop0c_1008_6436`). |
| `1008:643a` | `src/recovered/framework/graphics_gdi.c` | `CPictSurface_1008_6448` slot `+0x58` returns 1 (`CPictSurface_True58_1008_643a`). |
| `1008:6440` | `src/recovered/framework/graphics_gdi.c` | `CPictSurface_1008_6448` slot `+0x5c` no-op (`CPictSurface_Noop5c_1008_6440`). |
| `1008:6444` | `src/recovered/framework/graphics_gdi.c` | `CPictSurface_1008_6448` slot `+0x60` no-op (`CPictSurface_Noop60_1008_6444`). |
| `1008:64ac` | `src/recovered/framework/graphics_gdi.c` | Procedural 32x32 water/ice shimmer animation across palette indices `0xa8..0xaf` (`WaterShimmer_AnimateBitmap_1008_64ac`). |
| `1008:65f2` | `src/recovered/framework/resource_manager.c` | Initializes `CResourceManager_1008_2da6` (`CResourceManager_Init_1008_65f2`). |
| `1008:6602` | `src/recovered/framework/resource_manager.c` | Purges `PICT` (`0x03f3`) immediately on unlock when purgeable (`CResourceManager_UpdateEntryFlags_1008_6602`). |
| `1008:6a28` | `src/recovered/framework/audio_waveout.c` | Verifies Win 3.10+, 11.025 kHz 8-bit mono async `waveOut`, and display capabilities (`System_CheckRequirements_1008_6a28`). |
| `1008:72b6` | `src/recovered/framework/ui_framework.c` | Application subsystem initialization, idle pump & shutdown (`CApplication_1008_2e32`) (`1008:72b6`). |
| `1008:7592` | `src/recovered/framework/ui_framework.c` | Application subsystem initialization, idle pump & shutdown (`CApplication_1008_2e32`) (`1008:7592`). |
| `1008:75b2` | `src/recovered/framework/ui_framework.c` | Application subsystem initialization, idle pump & shutdown (`CApplication_1008_2e32`) (`1008:75b2`). |
| `1008:769a` | `src/recovered/framework/ui_framework.c` | Application subsystem initialization, idle pump & shutdown (`CApplication_1008_2e32`) (`1008:769a`). |
| `1008:76cc` | `src/recovered/framework/ui_framework.c` | Application subsystem initialization, idle pump & shutdown (`CApplication_1008_2e32`) (`1008:76cc`). |
| `1008:7716` | `src/recovered/framework/ui_framework.c` | Application subsystem initialization, idle pump & shutdown (`CApplication_1008_2e32`) (`1008:7716`). |
| `1008:77cc` | `src/recovered/framework/ui_framework.c` | Application subsystem initialization, idle pump & shutdown (`CApplication_1008_2e32`) (`1008:77cc`). |
| `1008:77d0` | `src/recovered/framework/ui_framework.c` | Application subsystem initialization, idle pump & shutdown (`CApplication_1008_2e32`) (`1008:77d0`). |
| `1008:783a` | `src/recovered/framework/ui_framework.c` | Application subsystem initialization, idle pump & shutdown (`CApplication_1008_2e32`) (`1008:783a`). |
| `1008:7b9a` | `src/recovered/framework/ui_framework.c` | Application subsystem initialization, idle pump & shutdown (`CApplication_1008_2e32`) (`1008:7b9a`). |
| `1008:7bbe` | `src/recovered/framework/ui_framework.c` | Application subsystem initialization, idle pump & shutdown (`CApplication_1008_2e32`) (`1008:7bbe`). |
| `1008:7bce` | `src/recovered/framework/ui_framework.c` | Application subsystem initialization, idle pump & shutdown (`CApplication_1008_2e32`) (`1008:7bce`). |
| `1008:7bf2` | `src/recovered/framework/ui_framework.c` | Application subsystem initialization, idle pump & shutdown (`CApplication_1008_2e32`) (`1008:7bf2`). |
| `1008:7c02` | `src/recovered/framework/ui_framework.c` | Event Dispatcher & Cursor Manager (`CEventDispatcher_1008_7f84`) (`1008:7c02`). |
| `1008:7c5c` | `src/recovered/framework/ui_framework.c` | Event Dispatcher & Cursor Manager (`CEventDispatcher_1008_7f84`) (`1008:7c5c`). |
| `1008:7cb4` | `src/recovered/framework/graphics_gdi.c` | Returns active `HPALETTE` handle (`CPaletteManager_GetHPalette_1008_7cb4`). |
| `1008:b6f8` | `src/recovered/framework/ui_framework.c` | Base Event/Command Target (`CEventTarget_1008_07f4`) (`1008:b6f8`). |
| `1008:b6fa` | `src/recovered/framework/ui_framework.c` | Base Event/Command Target (`CEventTarget_1008_07f4`) (`1008:b6fa`). |
| `1008:b70e` | `src/recovered/framework/ui_framework.c` | Base Event/Command Target (`CEventTarget_1008_07f4`) (`1008:b70e`). |
| `1008:b746` | `src/recovered/framework/ui_framework.c` | Base Event/Command Target (`CEventTarget_1008_07f4`) (`1008:b746`). |
| `1008:b7aa` | `src/recovered/framework/ui_framework.c` | Dynamic Collection (`CCollection_1008_0824`) (`1008:b7aa`). |
| `1008:b7ae` | `src/recovered/framework/ui_framework.c` | Dynamic Collection (`CCollection_1008_0824`) (`1008:b7ae`). |
| `1008:b7b2` | `src/recovered/framework/ui_framework.c` | Dynamic Collection (`CCollection_1008_0824`) (`1008:b7b2`). |
| `1008:b7de` | `src/recovered/framework/ui_framework.c` | Dynamic Collection (`CCollection_1008_0824`) (`1008:b7de`). |
| `1008:b802` | `src/recovered/framework/ui_framework.c` | Dynamic Collection (`CCollection_1008_0824`) (`1008:b802`). |
| `1008:b82e` | `src/recovered/framework/ui_framework.c` | Dynamic Collection (`CCollection_1008_0824`) (`1008:b82e`). |
| `1008:b85a` | `src/recovered/framework/ui_framework.c` | Dynamic Collection (`CCollection_1008_0824`) (`1008:b85a`). |
| `1008:b86c` | `src/recovered/framework/ui_framework.c` | Dynamic Collection (`CCollection_1008_0824`) (`1008:b86c`). |
| `1008:b88c` | `src/recovered/framework/ui_framework.c` | Dynamic Collection (`CCollection_1008_0824`) (`1008:b88c`). |
| `1008:b8fc` | `src/recovered/framework/ui_framework.c` | Dynamic Collection (`CCollection_1008_0824`) (`1008:b8fc`). |
| `1008:b928` | `src/recovered/framework/ui_framework.c` | Dynamic Collection (`CCollection_1008_0824`) (`1008:b928`). |
| `1008:b93a` | `src/recovered/framework/ui_framework.c` | Dynamic Collection (`CCollection_1008_0824`) (`1008:b93a`). |
| `1008:b95c` | `src/recovered/framework/ui_framework.c` | Dynamic Collection (`CCollection_1008_0824`) (`1008:b95c`). |
| `1008:b994` | `src/recovered/framework/ui_framework.c` | Dynamic Collection (`CCollection_1008_0824`) (`1008:b994`). |
| `1008:b9b6` | `src/recovered/framework/ui_framework.c` | Dynamic Collection (`CCollection_1008_0824`) (`1008:b9b6`). |
| `1008:b9ee` | `src/recovered/framework/ui_framework.c` | Dynamic Collection (`CCollection_1008_0824`) (`1008:b9ee`). |
| `1008:baa0` | `src/recovered/framework/ui_framework.c` | Dynamic Collection (`CCollection_1008_0824`) (`1008:baa0`). |
| `1008:bb56` | `src/recovered/framework/ui_framework.c` | Dynamic Collection (`CCollection_1008_0824`) (`1008:bb56`). |
| `1008:bb96` | `src/recovered/framework/ui_framework.c` | Dynamic Collection (`CCollection_1008_0824`) (`1008:bb96`). |
| `1008:bbbc` | `src/recovered/framework/ui_framework.c` | Dynamic Collection (`CCollection_1008_0824`) (`1008:bbbc`). |
| `1008:bca0` | `src/recovered/framework/ui_framework.c` | Indexed List (`CList_1008_0904`) (`1008:bca0`). |
| `1008:bcbe` | `src/recovered/framework/ui_framework.c` | Indexed List (`CList_1008_0904`) (`1008:bcbe`). |
| `1008:bd78` | `src/recovered/framework/ui_framework.c` | Indexed List (`CList_1008_0904`) (`1008:bd78`). |
| `1008:bdfe` | `src/recovered/framework/ui_framework.c` | Indexed List (`CList_1008_0904`) (`1008:bdfe`). |
| `1008:be48` | `src/recovered/framework/ui_framework.c` | Indexed List (`CList_1008_0904`) (`1008:be48`). |
| `1008:be9a` | `src/recovered/framework/ui_framework.c` | Indexed List (`CList_1008_0904`) (`1008:be9a`). |
| `1008:beba` | `src/recovered/framework/ui_framework.c` | Indexed List (`CList_1008_0904`) (`1008:beba`). |
| `1008:bf06` | `src/recovered/framework/ui_framework.c` | Indexed List (`CList_1008_0904`) (`1008:bf06`). |
| `1008:bf30` | `src/recovered/framework/ui_framework.c` | Indexed List (`CList_1008_0904`) (`1008:bf30`). |
| `1008:bf5e` | `src/recovered/framework/ui_framework.c` | Indexed List (`CList_1008_0904`) (`1008:bf5e`). |
| `1008:bff4` | `src/recovered/framework/ui_framework.c` | Indexed List (`CList_1008_0904`) (`1008:bff4`). |
| `1008:c01e` | `src/recovered/framework/ui_framework.c` | Indexed List (`CList_1008_0904`) (`1008:c01e`). |
| `1008:c05c` | `src/recovered/framework/ui_framework.c` | Ordered Collection (`COrderedList_1008_0b08`) (`1008:c05c`). |
| `1008:c08e` | `src/recovered/framework/ui_framework.c` | Ordered Collection (`COrderedList_1008_0b08`) (`1008:c08e`). |
| `1008:c104` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c104`). |
| `1008:c17e` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c17e`). |
| `1008:c224` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c224`). |
| `1008:c2ce` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c2ce`). |
| `1008:c33c` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c33c`). |
| `1008:c362` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c362`). |
| `1008:c376` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c376`). |
| `1008:c388` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c388`). |
| `1008:c3a4` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c3a4`). |
| `1008:c3a8` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c3a8`). |
| `1008:c3b8` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c3b8`). |
| `1008:c3cc` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c3cc`). |
| `1008:c3ee` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c3ee`). |
| `1008:c444` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c444`). |
| `1008:c468` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c468`). |
| `1008:c4aa` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c4aa`). |
| `1008:c502` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c502`). |
| `1008:c54c` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c54c`). |
| `1008:c5aa` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c5aa`). |
| `1008:c5ec` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c5ec`). |
| `1008:c60a` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c60a`). |
| `1008:c628` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c628`). |
| `1008:c666` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c666`). |
| `1008:c68a` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c68a`). |
| `1008:c69e` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c69e`). |
| `1008:c6b6` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c6b6`). |
| `1008:c762` | `src/recovered/framework/ui_framework.c` | Object Pointer List (`CObList_1008_0a48`) (`1008:c762`). |
| `1008:d1d8` | `src/recovered/framework/ui_framework.c` | Desktop allocator (`CDesktop_New_1008_d1d8`) (`1008:d1d8`). |
| `1008:e412` | `src/recovered/framework/message_pump.c` | Message ranges/order decompiled (`framework_drain_message_ranges`). |
| `1008:eed8` | `src/recovered/framework/resource_manager.c` | Zero-initialized memory allocator (`AllocZeroed_1008_eed8`). |
| `1008:ef1e` | `src/recovered/framework/resource_manager.c` | Memory release helper (`FreeMem_1008_ef1e`). |
| `1008:ef2c` | `src/recovered/framework/resource_manager.c` | Zero-initialized C++ object allocator (`NewObject_1008_ef2c`). |
| `1008:ef80` | `src/recovered/framework/resource_manager.c` | Object memory deallocator (`DeleteObject_1008_ef80`). |
| `1008:ef8e` | `src/recovered/framework/resource_manager.c` | Root `CObject_1008_0bc8` virtual destructor (`CObject_Dtor_1008_ef8e`). |
| `1008:ef9e` | `src/recovered/framework/resource_manager.c` | Root `CObject_1008_0bc8` virtual no-op (`CObject_Noop_1008_ef9e`). |
| `1010:0076` | `src/recovered/framework/resource_manager.c` | Returns 32-bit byte size at handle `+0x08` (`Handle_GetSize_1010_0076`). |
| `1010:0096` | `src/recovered/framework/resource_manager.c` | Returns flag byte at handle `+0x0c` (`Handle_GetFlags_1010_0096`). |
| `1010:00b0` | `src/recovered/framework/resource_manager.c` | Updates handle flags via Resource Manager slot `+0x7c` (`Handle_SetFlags_1010_00b0`). |
| `1010:00e8` | `src/recovered/framework/resource_manager.c` | Sets locked bit `0x40` on handle (`Handle_Lock_1010_00e8`). |
| `1010:0122` | `src/recovered/framework/resource_manager.c` | Clears locked bit `0x40` on handle (`Handle_Unlock_1010_0122`). |
| `1010:015c` | `src/recovered/framework/resource_manager.c` | Clears purgeable bit `0x20` on handle (`Handle_NoPurge_1010_015c`). |
| `1010:0196` | `src/recovered/framework/resource_manager.c` | Sets purgeable bit `0x20` on handle (`Handle_AllowPurge_1010_0196`). |
| `1010:01d0` | `src/recovered/framework/resource_manager.c` | Disposes heap/global buffer and frees handle descriptor (`Handle_Dispose_1010_01d0`). |
| `1010:023e` | `src/recovered/framework/resource_manager.c` | Allocates heap/global buffer and double-indirected handle (`Handle_Alloc_1010_023e`). |
| `1010:035c` | `src/recovered/framework/resource_manager.c` | Allocates zero-initialized handle (`Handle_AllocZeroed_1010_035c`). |
| `1010:036c` | `src/recovered/framework/ui_framework.c` | Allocates and constructs `CObList_1008_0a48` (`CObList_New_1010_036c`) (`1010:036c`). |
| `1010:03ee` | `src/recovered/framework/resource_manager.c` | Resizes handle payload buffer (`Handle_Resize_1010_03ee`). |
| `1010:057c` | `src/recovered/framework/resource_manager.c` | Disposes handle via pointer and clears pointer (`Handle_FreeIndirect_1010_057c`). |
| `1010:0608` | `src/recovered/framework/resource_manager.c` | No-op handle compaction stub (`Handle_MoveNeck_1010_0608`). |
| `1010:0614` | `src/recovered/framework/graphics_gdi.c` | Unlocks and frees global memory block (`Global_FreePtr_1010_0614`). |
| `1010:07ae` | `src/recovered/framework/audio_waveout.c` | Global 60 Hz tick query (`Timer_GetTicks60Hz_1010_07ae`). |
| `1010:07ba` | `src/recovered/framework/graphics_gdi.c` | Random byte helper (`Random_Byte_1010_07ba`). |
| `1010:08e0` | `src/recovered/framework/graphics_gdi.c` | Decodes PackBits RLE or raw `'PICT'` (`0x03f3`) resource into 8-bit `HBITMAP` (`Pict_DecodeToBitmap_1010_08e0`). |
| `1010:0c94` | `src/recovered/framework/graphics_gdi.c` | Low-memory banded PackBits RLE `'PICT'` decoder (`Pict_DecodeRleBanded_1010_0c94`). |
| `1010:0f1c` | `src/recovered/framework/graphics_gdi.c` | `IsRectEmpty` wrapper (`Rect_IsEmpty_1010_0f1c`). |
| `1010:0f2a` | `src/recovered/framework/graphics_gdi.c` | `EqualRect` wrapper (`Rect_Equal_1010_0f2a`). |
| `1010:0f3c` | `src/recovered/framework/graphics_gdi.c` | Insets rectangle by `(dx, dy)` (`Rect_Inset_1010_0f3c`). |
| `1010:0f74` | `src/recovered/framework/graphics_gdi.c` | `OffsetRect` wrapper (`Rect_Offset_1010_0f74`). |
| `1010:0f88` | `src/recovered/framework/graphics_gdi.c` | `IntersectRect` wrapper (`Rect_Intersect_1010_0f88`). |
| `1010:0f9e` | `src/recovered/framework/graphics_gdi.c` | Sets 16-bit `(x, y)` point (`Point_Set_1010_0f9e`). |
| `1010:0fb6` | `src/recovered/framework/graphics_gdi.c` | `SetRect` wrapper (`Rect_Set_1010_0fb6`). |
| `1010:0fd0` | `src/recovered/framework/graphics_gdi.c` | `UnionRect` wrapper (`Rect_Union_1010_0fd0`). |
| `1010:0fe6` | `src/recovered/framework/resource_manager.c` | Finds, loads, sizes, and locks NE resource into handle (`ResHandle_LoadAndLock_1010_0fe6`). |
| `1010:10f2` | `src/recovered/framework/resource_manager.c` | Fetches or loads resource by `(win16_type_id, mac_fourcc, res_id)` (`ResMgr_GetResource_1010_10f2`). |
| `1010:11d8` | `src/recovered/framework/resource_manager.c` | Reloads purged resource handle on demand (`ResHandle_EnsureLoaded_1010_11d8`). |
| `1010:1206` | `src/recovered/framework/resource_manager.c` | Purges unlocked resources and dereferences handle (`ResHandle_PurgeAndDeref_1010_1206`). |
| `1010:1246` | `src/recovered/framework/ui_framework.c` | Dispatches virtual slot across `CObList` elements (`CObList_CallVirtual_1010_1246`) (`1010:1246`). |
| `1010:15f0` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:15f0`). |
| `1010:16da` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:16da`). |
| `1010:16ea` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:16ea`). |
| `1010:1794` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:1794`). |
| `1010:1804` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:1804`). |
| `1010:183c` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:183c`). |
| `1010:1840` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:1840`). |
| `1010:18ec` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:18ec`). |
| `1010:193c` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:193c`). |
| `1010:197a` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:197a`). |
| `1010:19b6` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:19b6`). |
| `1010:19d8` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:19d8`). |
| `1010:19fa` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:19fa`). |
| `1010:1a22` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:1a22`). |
| `1010:1a4a` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:1a4a`). |
| `1010:1a72` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:1a72`). |
| `1010:1a8a` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:1a8a`). |
| `1010:1b42` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:1b42`). |
| `1010:1bae` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:1bae`). |
| `1010:1c20` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:1c20`). |
| `1010:1cbe` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:1cbe`). |
| `1010:1cde` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:1cde`). |
| `1010:1d32` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:1d32`). |
| `1010:1d84` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:1d84`). |
| `1010:1db6` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:1db6`). |
| `1010:1de8` | `src/recovered/framework/ui_framework.c` | View hierarchy & `Pane` (`0x03f1`) resource loader (`CView_1008_1aac`) (`1010:1de8`). |
| `1010:24ae` | `src/recovered/framework/graphics_gdi.c` | Loads `'PICT'` (`0x03f3`) resource handle by ID (`Pict_LoadResourceHandle_1010_24ae`). |
| `1010:24f8` | `src/recovered/framework/audio_waveout.c` | Inclusive bounded random integer helper (`Random_Range_1010_24f8`). |
| `1010:25be` | `src/recovered/framework/resource_manager.c` | `CResourceTypeList_1010_2b12` virtual destructor (`CResourceTypeList_Dtor_1010_25be`). |
| `1010:25e4` | `src/recovered/framework/resource_manager.c` | Finds or creates resource handle entry by ID (`CResourceTypeList_FindOrCreateById_1010_25e4`). |
| `1010:264c` | `src/recovered/framework/resource_manager.c` | Copies 6-byte type descriptor (`CResourceTypeList_GetTypeDescriptor_1010_264c`). |
| `1010:266c` | `src/recovered/framework/resource_manager.c` | Allocates 0x1a resource handle and inserts ID mapping (`CResourceTypeList_CreateEntry_1010_266c`). |
| `1010:26ba` | `src/recovered/framework/resource_manager.c` | Purges unlocked purgeable resource handle (`CResourceTypeList_PurgeEntry_1010_26ba`). |
| `1010:2712` | `src/recovered/framework/resource_manager.c` | Purges all purgeable entries in type list (`CResourceTypeList_PurgeAll_1010_2712`). |
| `1010:2784` | `src/recovered/framework/resource_manager.c` | Frees resource handle and removes from type list (`CResourceTypeList_ReleaseEntry_1010_2784`). |
| `1010:2810` | `src/recovered/framework/resource_manager.c` | Releases all entries in type list (`CResourceTypeList_ReleaseAll_1010_2810`). |
| `1010:286c` | `src/recovered/framework/resource_manager.c` | Reads `0x03e8` `#1001`/`#1002` from `BLIZZARD.EXE` and builds 42 type lists (`CResourceMap_InitFromDirectory_1010_286c`). |
| `1010:29ea` | `src/recovered/framework/resource_manager.c` | `CResourceMap_1008_2ece` virtual destructor (`CResourceMap_Dtor_1010_29ea`). |
| `1010:2a14` | `src/recovered/framework/resource_manager.c` | Updates `0x60` lock/purge bits on resource handle (`CResourceMap_UpdateEntryFlags_1010_2a14`). |
| `1010:2a2e` | `src/recovered/framework/resource_manager.c` | Finds `CResourceTypeList_1010_2b12` matching `(win16_type_id, mac_fourcc)` (`CResourceMap_FindTypeList_1010_2a2e`). |
| `1010:2a54` | `src/recovered/framework/resource_manager.c` | Purges all purgeable handles across all 42 type lists (`CResourceMap_PurgeAllTypes_1010_2a54`). |
| `1010:2a74` | `src/recovered/framework/resource_manager.c` | Releases all handles across all 42 type lists (`CResourceMap_ReleaseAllTypes_1010_2a74`). |
| `1010:2a94` | `src/recovered/framework/resource_manager.c` | Resource ID comparator callback (`ResEntry_CompareId_1010_2a94`). |
| `1010:2ae2` | `src/recovered/framework/resource_manager.c` | Callback invoking type list slot `+0x64` (`PurgeAll`). |
| `1010:2afa` | `src/recovered/framework/resource_manager.c` | Callback invoking type list slot `+0x6c` (`ReleaseAll`). |
| `1010:2ba8` | `src/recovered/framework/graphics_gdi.c` | Stretches `HBITMAP` to `HBITMAP` via temporary DCs (`Gdi_StretchBitmapToBitmap_1010_2ba8`). |
| `1010:2c40` | `src/recovered/framework/graphics_gdi.c` | Blits `HBITMAP` into destination `HDC` (`Gdi_BlitBitmapToDc_1010_2c40`). |
| `1010:2cc0` | `src/recovered/framework/graphics_gdi.c` | Stretches source `HDC` to destination `HDC` (`Gdi_StretchDcToDc_1010_2cc0`). |
| `1010:2d16` | `src/recovered/framework/ui_framework.c` | Win16 Window class registration & screen DC (`Window_RegisterClasses_1010_2d16`) (`1010:2d16`). |
| `1010:2d22` | `src/recovered/framework/ui_framework.c` | Win16 Window class registration & screen DC (`Window_RegisterClasses_1010_2d16`) (`1010:2d22`). |
| `1010:2d2e` | `src/recovered/framework/graphics_gdi.c` | Allocates and builds 14-byte 1-bit transparent sprite mask (`SpriteMask_Create_1010_2d2e`). |
| `1010:2d58` | `src/recovered/framework/graphics_gdi.c` | Generates 1-bit monochrome mask where index 0 is transparent (`SpriteMask_InitFromBitmap_1010_2d58`). |
| `1010:2e12` | `src/recovered/framework/graphics_gdi.c` | Destroys 14-byte transparent sprite mask (`SpriteMask_Destroy_1010_2e12`). |
| `1010:2e60` | `src/recovered/framework/graphics_gdi.c` | Transparent `SRCAND`+`SRCPAINT` sprite blitter (`Gdi_TransparentBlit_1010_2e60`). |
| `1010:2f6a` | `src/recovered/framework/graphics_gdi.c` | Transparent blitter using precomputed `SBSpriteMask` (`Gdi_TransparentBlitPrecomputed_1010_2f6a`). |
| `1010:312a` | `src/recovered/framework/audio_waveout.c` | Zero-initializes 10-byte `SBBswSample` (`BswSample_InitEmpty_1010_312a`). |
| `1010:3152` | `src/recovered/framework/audio_waveout.c` | Frees PCM buffer in `SBBswSample` (`BswSample_FreeBuffer_1010_3152`). |
| `1010:3172` | `src/recovered/framework/audio_waveout.c` | `SBBswSample` constructor (`BswSample_Ctor_1010_3172`). |
| `1010:3190` | `src/recovered/framework/audio_waveout.c` | Loads BSW audio sample directly from module resource (`BswSample_LoadFromExeResource_1010_3190`). |
| `1010:31d8` | `src/recovered/framework/audio_waveout.c` | Parses 16-byte `BSW\0` header and copies PCM payload (`BswSample_ParseHeaderAndCopyPcm_1010_31d8`). |
| `1010:32bc` | `src/recovered/framework/audio_waveout.c` | Unprepares and frees chunked `WAVEHDR` stream buffers (`BswStream_UnprepareAndFree_1010_32bc`). |
| `1010:3386` | `src/recovered/framework/audio_waveout.c` | Pumps next chunked `WAVEHDR` buffer to `waveOutWrite` (`BswStream_PumpNextChunk_1010_3386`). |
| `1010:34ea` | `src/recovered/framework/audio_waveout.c` | Allocates and starts multi-chunk `waveOut` stream (`BswStream_StartChunked_1010_34ea`). |
| `1010:36a6` | `src/recovered/framework/audio_waveout.c` | Constructs 50-byte `SBWavePlayer` at `DAT_1060_07cc` (`WavePlayer_Ctor_1010_36a6`). |
| `1010:36d0` | `src/recovered/framework/audio_waveout.c` | Resets, unprepares, and closes `HWAVEOUT` (`WavePlayer_StopAndClose_1010_36d0`). |
| `1010:37e6` | `src/recovered/framework/audio_waveout.c` | Opens `waveOut` at 11.025/22.05 kHz and writes `WAVEHDR` (`WavePlayer_OpenAndWrite_1010_37e6`). |
| `1010:39d4` | `src/recovered/framework/audio_waveout.c` | Stops previous sound and starts new `SBBswSample` (`WavePlayer_PlaySample_1010_39d4`). |
| `1010:3a10` | `src/recovered/framework/audio_waveout.c` | Sets 60 Hz playback completion tick (`WavePlayer_SetEndTick_1010_3a10`). |
| `1010:3a24` | `src/recovered/framework/audio_waveout.c` | No-op waveOut completion callback (`WavePlayer_NoopCallback_1010_3a24`). |
| `1010:3a28` | `src/recovered/framework/ui_framework.c` | Event Dispatcher (`CEventDispatcherBase_1008_0b84`) (`1010:3a28`). |
| `1010:3b76` | `src/recovered/framework/ui_framework.c` | Event Dispatcher (`CEventDispatcherBase_1008_0b84`) (`1010:3b76`). |
| `1010:3bac` | `src/recovered/framework/ui_framework.c` | Event Dispatcher (`CEventDispatcherBase_1008_0b84`) (`1010:3bac`). |
| `1010:3be2` | `src/recovered/framework/ui_framework.c` | Event Dispatcher (`CEventDispatcherBase_1008_0b84`) (`1010:3be2`). |
| `1010:3bfa` | `src/recovered/framework/ui_framework.c` | Event Dispatcher (`CEventDispatcherBase_1008_0b84`) (`1010:3bfa`). |
| `1010:3bfe` | `src/recovered/framework/ui_framework.c` | Event Dispatcher (`CEventDispatcherBase_1008_0b84`) (`1010:3bfe`). |
| `1010:3c82` | `src/recovered/framework/ui_framework.c` | Event Dispatcher (`CEventDispatcherBase_1008_0b84`) (`1010:3c82`). |
| `1010:3d2e` | `src/recovered/framework/ui_framework.c` | Event Dispatcher (`CEventDispatcherBase_1008_0b84`) (`1010:3d2e`). |
| `1010:3d32` | `src/recovered/framework/ui_framework.c` | Event Dispatcher (`CEventDispatcherBase_1008_0b84`) (`1010:3d32`). |
| `1010:3d42` | `src/recovered/framework/ui_framework.c` | Event Dispatcher (`CEventDispatcherBase_1008_0b84`) (`1010:3d42`). |
| `1010:3d52` | `src/recovered/framework/ui_framework.c` | Event Dispatcher (`CEventDispatcherBase_1008_0b84`) (`1010:3d52`). |
| `1010:3d56` | `src/recovered/framework/ui_framework.c` | Event Dispatcher (`CEventDispatcherBase_1008_0b84`) (`1010:3d56`). |
| `1010:4270` | `src/recovered/framework/ui_framework.c` | Event Dispatcher (`CEventDispatcherBase_1008_0b84`) (`1010:4270`). |
| `1010:42ce` | `src/recovered/framework/ui_framework.c` | Event Dispatcher (`CEventDispatcherBase_1008_0b84`) (`1010:42ce`). |
| `1010:446a` | `src/recovered/framework/audio_waveout.c` | Constructs 60 Hz `TIMEGETTIME` timer at `DAT_1060_0a1e` (`CTimer60Hz_Ctor_1010_446a`). |
| `1010:44d6` | `src/recovered/framework/audio_waveout.c` | `CTimer60Hz_1010_456a` destructor (`CTimer60Hz_Dtor_1010_44d6`). |
| `1010:44ec` | `src/recovered/framework/audio_waveout.c` | Advances 60 Hz Bresenham tick counter from `TIMEGETTIME` (`CTimer60Hz_GetTicks_1010_44ec`). |
| `1010:45ea` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:45ea`). |
| `1010:4640` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4640`). |
| `1010:46e0` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:46e0`). |
| `1010:4762` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4762`). |
| `1010:47c0` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:47c0`). |
| `1010:4868` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4868`). |
| `1010:487c` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:487c`). |
| `1010:48aa` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:48aa`). |
| `1010:48b0` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:48b0`). |
| `1010:491e` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:491e`). |
| `1010:4a52` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4a52`). |
| `1010:4a56` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4a56`). |
| `1010:4a5a` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4a5a`). |
| `1010:4a5e` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4a5e`). |
| `1010:4a62` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4a62`). |
| `1010:4a66` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4a66`). |
| `1010:4a6a` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4a6a`). |
| `1010:4a6e` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4a6e`). |
| `1010:4ac6` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4ac6`). |
| `1010:4aec` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4aec`). |
| `1010:4b04` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4b04`). |
| `1010:4b08` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4b08`). |
| `1010:4b0c` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4b0c`). |
| `1010:4b1e` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4b1e`). |
| `1010:4b3c` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4b3c`). |
| `1010:4b52` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4b52`). |
| `1010:4b60` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4b60`). |
| `1010:4bb8` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4bb8`). |
| `1010:4bbe` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4bbe`). |
| `1010:4bfc` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4bfc`). |
| `1010:4c0a` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4c0a`). |
| `1010:4c8c` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4c8c`). |
| `1010:4cf8` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4cf8`). |
| `1010:4d28` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4d28`). |
| `1010:4d4c` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4d4c`). |
| `1010:4d60` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4d60`). |
| `1010:4d72` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4d72`). |
| `1010:4d84` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4d84`). |
| `1010:4ddc` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4ddc`). |
| `1010:4e1a` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4e1a`). |
| `1010:4e28` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4e28`). |
| `1010:4e4a` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4e4a`). |
| `1010:4ebe` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4ebe`). |
| `1010:4f9a` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4f9a`). |
| `1010:4fc0` | `src/recovered/framework/ui_framework.c` | View coordinate transform, hit-test & GDI drawing (`CView_1008_1aac`) (`1010:4fc0`). |
| `1010:50ce` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:50ce`). |
| `1010:512a` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:512a`). |
| `1010:519a` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:519a`). |
| `1010:527c` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:527c`). |
| `1010:52c4` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:52c4`). |
| `1010:530a` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:530a`). |
| `1010:5348` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:5348`). |
| `1010:5384` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:5384`). |
| `1010:539a` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:539a`). |
| `1010:53c8` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:53c8`). |
| `1010:544e` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:544e`). |
| `1010:54cc` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:54cc`). |
| `1010:553e` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:553e`). |
| `1010:559e` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:559e`). |
| `1010:55c0` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:55c0`). |
| `1010:55fe` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:55fe`). |
| `1010:56c8` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:56c8`). |
| `1010:5722` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:5722`). |
| `1010:57a6` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:57a6`). |
| `1010:5824` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:5824`). |
| `1010:596e` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:596e`). |
| `1010:5a64` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:5a64`). |
| `1010:5b18` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:5b18`). |
| `1010:5bd8` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:5bd8`). |
| `1010:5c9e` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:5c9e`). |
| `1010:5d5e` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:5d5e`). |
| `1010:5d62` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:5d62`). |
| `1010:5d66` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:5d66`). |
| `1010:5d7c` | `src/recovered/framework/ui_framework.c` | Win16 Window wrapper & `WndProc` bridge (`CWindow_1010_5942`) (`1010:5d7c`). |
| `1018:63b2` | `src/recovered/framework/audio_waveout.c` | `CTimedCallback_1008_823c` flag `+0x0a` getter (`CTimedCallback_GetByte0a_1018_63b2`). |
| `1018:63c0` | `src/recovered/framework/audio_waveout.c` | `CTimedCallback_1008_823c` flag `+0x09` getter (`CTimedCallback_GetByte09_1018_63c0`). |
| `1018:63ce` | `src/recovered/framework/audio_waveout.c` | `CTimedCallback_1008_823c` paused flag `+0x08` getter (`CTimedCallback_IsPaused_1018_63ce`). |
| `1018:63dc` | `src/recovered/framework/audio_waveout.c` | Resumes timed callback (`CTimedCallback_Resume_1018_63dc`). |
| `1018:63ec` | `src/recovered/framework/audio_waveout.c` | Pauses timed callback (`CTimedCallback_Pause_1018_63ec`). |
| `1018:63fc` | `src/recovered/framework/audio_waveout.c` | Timed callback virtual no-op (`CTimedCallback_OnStartNoop_1018_63fc`). |
| `1018:6400` | `src/recovered/framework/audio_waveout.c` | Timed callback virtual no-op (`CTimedCallback_OnTickNoop_1018_6400`). |
| `1018:6404` | `src/recovered/framework/audio_waveout.c` | Executes timed callback and schedules next interval (`CTimedCallback_Execute_1018_6404`). |
| `1018:642c` | `src/recovered/framework/audio_waveout.c` | Checks if unpaused timed callback reached target 60 Hz tick (`CTimedCallback_IsReady_1018_642c`). |
| `1018:646a` | `src/recovered/framework/audio_waveout.c` | Schedules next 60 Hz callback tick (`CTimedCallback_ScheduleNext_1018_646a`). |
| `1018:648e` | `src/recovered/framework/audio_waveout.c` | Returns 0 (`CTimedCallback_False0_1018_648e`). |
| `1018:6494` | `src/recovered/framework/audio_waveout.c` | Returns 0 (`CTimedCallback_False1_1018_6494`). |
| `1018:9140` | `src/recovered/framework/resource_manager.c` | Loads `RsLt` (`0x03e9`) resource list by ID (`CResourceList_Init_1018_9140`). |
| `1018:9176` | `src/recovered/framework/resource_manager.c` | `CResourceList_1008_821c` virtual destructor (`CResourceList_Dtor_1018_9176`). |
| `1018:919e` | `src/recovered/framework/resource_manager.c` | Marks all resources in `RsLt` non-purgeable (`CResourceList_MarkAllNoPurge_1018_919e`). |
| `1018:9242` | `src/recovered/framework/resource_manager.c` | Marks all resources in `RsLt` purgeable (`CResourceList_MarkAllPurgeable_1018_9242`). |
| `1018:92e6` | `src/recovered/framework/resource_manager.c` | Loads and locks all resources in `RsLt` (`CResourceList_LoadAndLockAll_1018_92e6`). |
| `1018:93dc` | `src/recovered/framework/resource_manager.c` | Unlocks all resources in `RsLt` (`CResourceList_UnlockAll_1018_93dc`). |
| `1018:9480` | `src/recovered/framework/resource_manager.c` | Preloads all resources in `RsLt` (`CResourceList_PreloadAll_1018_9480`). |
| `1018:a704` | `src/recovered/application/object_model.c` | Top-7 descending ordered record insertion (`RecordTable_InsertTop7_1018_a704`). |
| `1018:adb8` | `src/recovered/application/object_model.c` | Indexed word adjustment and accumulated total update (`1018:adb8`). |
| `1018:ae3a` | `src/recovered/application/object_model.c` | High-water mark update and top-7 table sync (`1018:ae3a`). |
| `1018:aeaa` | `src/recovered/win16/application_primitives.c` | Direct far entry follows parent +0x250 to holder +0 and reads nested +0x06; RETF 4 verified. |
| `1018:aede` | `src/recovered/win16/application_primitives.c` | Direct far entry writes nested +0x1a8 then parent +0x254; RETF 6 verified. |
| `1018:af08` | `src/recovered/win16/application_primitives.c` | Direct far entry returns AL=1 for signed argument <= nested +0x1a8; RETF 6 verified. |
| `1020:2534` | `src/recovered/framework/ui_framework.c` | Event Dispatcher mouse state query (`CEventDispatcher_1008_7f84`) (`1020:2534`). |
| `1020:2552` | `src/recovered/framework/ui_framework.c` | Event Dispatcher mouse state query (`CEventDispatcher_1008_7f84`) (`1020:2552`). |
| `1020:27d8` | `src/recovered/framework/audio_waveout.c` | `CSoundChannel_1010_bdd4` virtual destructor (`CSoundChannel_Dtor_1020_27d8`). |
| `1020:2810` | `src/recovered/framework/audio_waveout.c` | Selects next sequential/random `'snd '` in channel and triggers playback (`CSoundChannel_ExecuteNext_1020_2810`). |
| `1020:28e0` | `src/recovered/framework/audio_waveout.c` | Checks if sound channel is ready and sound manager is idle (`CSoundChannel_IsReady_1020_28e0`). |
| `1020:2922` | `src/recovered/framework/audio_waveout.c` | Returns 1 (`CSoundChannel_True_1020_2922`). |
| `1020:2928` | `src/recovered/framework/audio_waveout.c` | Appends `'snd '` resource ID to channel and locks handle (`CSoundChannel_AddSound_1020_2928`). |
| `1020:29b6` | `src/recovered/framework/audio_waveout.c` | Sets primary `'snd '` resource ID on channel (`CSoundChannel_SetPrimarySound_1020_29b6`). |
| `1020:2a96` | `src/recovered/framework/audio_waveout.c` | Stops and unlocks all `'snd '` handles in channel (`CSoundChannel_ClearSounds_1020_2a96`). |

## Partial work

`1008:ac80` has a C fragment for the `0x2af9` state writes. It is not a reconstruction of the full original function.

Regenerate with `python3 tools/reconstruction_progress.py build/functions.json && python3 tools/map_subsystems.py`.
