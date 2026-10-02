#include "dev_agent.h"
#ifdef WII64_HBC_AGENT
#include <network.h>
#include <hbc_agent.h>
#include "version.h"
#include "../gc_memory/MEM2.h"
#include "perf_prof.h"

_Static_assert(HBC_CRASH_ADDR >= (unsigned long)TLBLUT_HI &&
               HBC_CRASH_ADDR + sizeof(hbc_crash_block) <= (unsigned long)TEXCACHE_LO,
               "HBC crash record must fit the reserved MEM2 gap");
#ifdef HBC_LASTLOG_ADDR
_Static_assert(HBC_LASTLOG_ADDR + sizeof(hbc_lastlog_block) <= (unsigned long)TEXCACHE_LO,
               "HBC last-output block must fit the reserved MEM2 gap");
#endif

/* --- The HOME menu: the agent's overlay --------------------------------------------
   HOME on a Wii Remote or Classic Controller (or `hbc.py key h`) opens it, in a game
   and in Wii64's menus. In a game it stops emulation first, like the exit combo, so the
   overlay runs from the menu thread with emulation, audio and GX idle, over the game's
   last frame; closing it resumes the game. It draws into whichever of Wii64's two MEM2
   framebuffers is not on screen and reads the frame in place, so it allocates nothing
   (HBC-Reborn >= 29e19e0; Wii64 has ~0.4 MB of MEM1 free, the overlay's own path needs
   1.8 MB). The bar's left button (slot 0) is a Wii64 menu: live info and Wii64 Menu.
   Its right button keeps the agent's Shot (sd:/screenshots). */
#include <stdio.h>
#include <wiiuse/wpad.h>
#include <ogc/gx.h>
#include <ogc/video.h>
#include "rom.h"
#include "timers.h"
#include "wii64config.h"
#include "../r4300/r4300.h"

extern GXRModeObj *vmode;
extern char shutdown;
extern unsigned int hasLoadedROM;
extern timers Timers;

static volatile int homeWanted; // HOME stopped the game: open the overlay after go()
static int toWii64Menu;         // the overlay's Wii64 Menu button was pressed
static int inGame;              // the overlay was opened from a stopped game

static char infoGame[48], infoSpeed[40];
static void pressWii64Menu(void *user) { (void)user; toWii64Menu = 1; }
static hbc_agent_item wii64Items[] = {
    { "Version", WII64_VERSION, NULL, NULL, 0 },
    { "Game", infoGame, NULL, NULL, 0 },
    { "Speed", infoSpeed, NULL, NULL, 0 },
#ifdef GLN64_GX
    { "Video", "glN64", NULL, NULL, 0 },
#else
    { "Video", "Rice", NULL, NULL, 0 },
#endif
    { "CPU core", "", NULL, NULL, 0 },
    { "Wii64 Menu", NULL, pressWii64Menu, NULL, HBC_AGENT_ITEM_CLOSE },
};
#define WII64_ITEM_CORE 4
#define WII64_ITEM_MENU 5

static void refreshInfo(void)
{
    const char *name = ROM_SETTINGS.goodname[0] ? ROM_SETTINGS.goodname : (const char *)ROM_HEADER.Name;
    snprintf(infoGame, sizeof(infoGame), "%.*s", hasLoadedROM ? 40 : 4, hasLoadedROM ? name : "none");
    snprintf(infoSpeed, sizeof(infoSpeed), "%.1f VI/s, %.1f fps", Timers.vis, Timers.fps);
    wii64Items[WII64_ITEM_CORE].value = dynacore == DYNACORE_DYNAREC ? "Dynarec" :
        dynacore == DYNACORE_PURE_INTERP ? "Pure interpreter" : "Interpreter";
    wii64Items[WII64_ITEM_MENU].flags = inGame ? HBC_AGENT_ITEM_CLOSE : HBC_AGENT_ITEM_DISABLED;
}

/* Exit's choices: Wii64's own shutdown (Gui::draw fades out, restores a Wii U's aspect
   ratio, then exits or powers off) for HBC and Power off; the agent does the rest. */
static bool exitChoice(int choice, void *user)
{
    (void)user;
    if (choice == HBC_AGENT_EXIT_HBC) shutdown = 2;
    else if (choice == HBC_AGENT_EXIT_POWER_OFF) shutdown = 1;
    else return false;
    return true;
}

static int homePressed(void)
{
    static u32 previous;
    u32 held = 0;
    for (int chan = 0; chan < 4; chan++) {
        WPADData *d = WPAD_Data(chan);
        if (d && d->err == WPAD_ERR_NONE) held |= d->btns_h;
    }
    u32 down = held & ~previous;
    previous = held;
    return (down & (WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME)) || hbc_agent_home_pending();
}

static void openOverlay(void)
{
    GX_DrawDone(); // no pending draw-sync callback may flip the display under it
    refreshInfo();
    u32 shown = (u32)VIDEO_GetCurrentFramebuffer() & 0x1fffffff;
    void *lend = shown == ((u32)XFB0_LO & 0x1fffffff) ? XFB1_LO : XFB0_LO;
    if (hbc_agent_home_fb(vmode, lend, NULL) < 0)
        hbc_agent_home(vmode); // an older SDK: its own buffers, or over ours
}

void devAgent_pollHome(void)
{
    if (homePressed()) { homeWanted = 1; r4300.stop = 1; }
}

int devAgent_homeAfterStop(void)
{
    if (!homeWanted) return 0;
    homeWanted = 0;
    toWii64Menu = 0;
    inGame = 1;
    openOverlay();
    inGame = 0;
    return !toWii64Menu && !shutdown && !devAgent_exitRequested();
}

void devAgent_menuHome(void)
{
    if (homePressed()) openOverlay();
}

void devAgent_init(void)
{
    /* SDK starts networking asynchronously; uploads wait before net_init(). */
    hbc_agent_config cfg = {0};
    cfg.name = "Wii64";
    cfg.version = WII64_VERSION;
    cfg.priority = 40;
    cfg.app_polls_exit = true;
    cfg.exit_grace_ms = 10000;
    cfg.crash_reload_s = 3;
    cfg.gc_pads = true;
    cfg.on_exit_choice = exitChoice;
    int result = hbc_agent_init(&cfg);
    hbc_agent_set_slot_menu(0, "Wii64", "Wii64 " WII64_VERSION, wii64Items,
                            sizeof(wii64Items) / sizeof(wii64Items[0]));
    perfProf_mark(result == 0 ? "HBC agent ready" : "HBC agent init failed");
    (void)result; // release builds compile perfProf_mark away
}

int devAgent_exitRequested(void)
{
    PERF_SUBSYSTEM_C_SCOPE(PERF_SUB_AGENT_POLL);
    return hbc_agent_exit_requested();
}
int devAgent_netReady(void) { return hbc_agent_net_wait(5000) >= 0; }

/* The agent's hang watchdog (SDK 1.9): armed by the first call, then 60 s without
   one is reported as a hang (`hbc.py crash`) and the Wii returns to HBC. Called per
   guest VI and per menu frame, from thread context: a call from the VI interrupt
   would keep it happy while the main thread is stuck. */
void devAgent_alive(void) { hbc_agent_alive(); }
void devAgent_hold(int hold) { hbc_agent_hold(hold != 0); }

/* libogc's vector code sends an exception taken with MSR[RI] clear straight to
   default_exceptionhandler, past the agent's table hook, so the Wii returned to HBC
   with no report (WiiStation, 2026-10-01). Makefile.wii wraps the function every
   crash screen goes through and records the exception here unless the agent already
   recorded this one. From WiiStation's Gamecube/ws_crash.c. */
#include <string.h>
#include <ogc/cache.h>
#include <ogc/context.h>
#include <ogc/machine/processor.h>

static int ram_word(u32 a)
{
    return !(a & 3) && ((a >= 0x80000000 && a < 0x81800000) || (a >= 0x90000000 && a < 0x94000000));
}

void __real_c_default_exceptionhandler(frame_context *ctx);

void __wrap_c_default_exceptionhandler(frame_context *ctx)
{
    static const u8 vector[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0x0c, 0x0d, 0x0f, 0x13, 0x14, 0x17 };
    hbc_crash_block *b = (hbc_crash_block *)HBC_CRASH_ADDR;
    u32 n = ctx->nExcept, sp = ctx->gpr[1], i;

    if (!(b->magic == HBC_CRASH_MAGIC && b->check == hbc_crash_check(b) && b->pc == ctx->srr0)) {
        memset(b, 0, sizeof *b);
        b->magic = HBC_CRASH_MAGIC;
        b->version = HBC_CRASH_VERSION;
        b->kind = HBC_CRASH_EXCEPTION;
        b->exception = n < sizeof vector ? vector[n] : n;
        b->pc = ctx->srr0;
        b->msr = ctx->srr1;
        b->lr = ctx->lr;
        b->cr = ctx->cr;
        b->ctr = ctx->ctr;
        b->dar = mfspr(19);
        b->dsisr = mfspr(18);
        b->sp = sp;
        for (i = 0; i < HBC_CRASH_FRAMES && ram_word(sp); i++) {
            u32 next = *(u32 *)sp;
            if (!next)
                break;
            next |= 0x80000000; /* an exception frame's back chain is a physical address */
            if (!ram_word(next) || next <= sp)
                break;
            b->frames[i] = *(u32 *)(next + 4);
            sp = next;
        }
        strcpy(b->app, "Wii64");
        strcpy(b->reason, "past the agent's hook (MSR[RI] clear?)");
        b->check = hbc_crash_check(b);
        DCFlushRange(b, sizeof *b);
    }
    __real_c_default_exceptionhandler(ctx);
}

#ifdef PERF_PROF
unsigned int devAgent_crashVi;
__attribute__((noinline)) void devAgent_testCrash(void)
{
    *(volatile u32*)0x10 = 0x57494936; // Deliberate fatal DSI, outside the ROM VM range.
}
#endif
#endif
