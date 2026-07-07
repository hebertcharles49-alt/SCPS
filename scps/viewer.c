/*
 * viewer.c — HARNAIS HEADLESS du moteur SCPS
 *
 * Le front interactif est désormais GODOT (godot/). Ce fichier ne garde QUE les
 * outils en ligne de commande, sans rendu : la vérif de sauvegarde (--savetest),
 * le durcissement du save (--fuzztest) et les dumps de tables (lang/readout/fnv).
 *
 * Toute l'UI SDL — carte, panneaux, sidebar, shell, FX, ~5000 lignes — a été
 * RETIRÉE : elle vivait ici quand le viewer ÉTAIT le front ; Godot l'a remplacée,
 * et le save est PARTAGÉ (scps_save) → ces outils restent la preuve headless du
 * moteur (byte-identité save/reload, rejet des saves forgées, tables éditables).
 *
 * SANS SDL : plus aucun appel ni include — binaire console pur (le dernier
 * vestige, l'entrée SDL_main MinGW, est tombé avec l'UI).
 *
 * Outils : --savetest · --fuzztest · --dump-lang · --dump-readout · --lang-audit · --dump-fnv
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include "scps_world.h"
#include "scps_econ.h"      /* WorldEconomy + culture/heritage (via econ.h) : sim_rebuild compose */
#include "scps_tech.h"
#include "scps_legitimacy.h"
#include "scps_prosperity.h"
#include "scps_readout.h"   /* readout_dump_file (--dump-readout) */
#include "scps_trade.h"
#include "scps_statecraft.h"
#include "scps_agency.h"
#include "scps_routes.h"
#include "scps_diplo.h"
#include "scps_events.h"
#include "scps_modifier.h"
#include "scps_demography.h"
#include "scps_labor.h"
#include "scps_ai.h"
#include "scps_revolt.h"
#include "scps_intertrade.h"
#include "scps_credit.h"
#include "scps_warhost.h"
#include "scps_campaign.h"
#include "scps_missions.h"
#include "scps_navy.h"
#include "scps_endgame.h"
#include "scps_tune.h"
#include "scps_crypt.h"     /* sauvegardes chiffrées (ChaCha20 + empreinte du clair) */
#include "scps_save_io.h"
#include "scps_lang.h"      /* tables de chaînes : lang_dump_file / lang_audit_file / lang_dump_fingerprints */
#include "scps_sim.h"       /* LE TICK PARTAGÉ (chronicle · Godot · viewer) : Sim + sim_init + sim_day */
#include "scps_save.h"      /* LA SAUVEGARDE PARTAGÉE : scps_save_game/load/sane/slot_info/slot_path */
#include "scps_provlog.h"   /* feed_set_focus : le fil suit le joueur */

/* ── Identité de setup (shell) : le créateur de culture a disparu avec l'UI ;
 *    ces défauts suffisent au harnais (sim_rebuild / game_load les lisent). ── */
static Heritage g_player_heritage = HERITAGE_ADAPTATIF;
static int  g_setup_ethos    = 5;
static int  g_gen_day        = 0;    /* progression d'amorce (écrit par sim_rebuild, sans lecteur ici) */
static int  g_age_alert_seen = -1;
static bool g_sim_ready      = false;
#define GEN_BOOT_DAYS (3*365)   /* amorce : 3 ans de tick → une carte déjà vivante avant le 1er ordre */

/* ── Amorce d'une simulation (miroir de la façade Godot) : composition du joueur
 *    → sim_init (RAZ + seed du monde) → débrayage IA du pays joueur → GEN_BOOT_DAYS
 *    d'amorce pour une carte déjà vivante. ── */
static void sim_rebuild(Sim *s, World *w) {
    if (!s->econ || !s->wp || !s->wl || !s->net || !s->ts || !s->sc
        || !s->ag || !s->ev || !s->drift || !s->rs || !s->host || !s->camp || !s->navy || !s->eg) return;
    culture_player_compose(g_player_heritage, g_setup_ethos, heritage_default_build(g_player_heritage));
    culture_reset_cid_map();
    { int ord=1;
      for (int c=0;c<w->n_countries;c++){
          int role = w->country[c].role;
          if (role==POLITY_PLAYER)          culture_bind_cid(c, 0);
          else if (role==POLITY_ANTAGONIST) culture_bind_cid(c, ord++);
      } }
    sim_init(s, w);   /* LE TICK PARTAGÉ : RAZ pleine + seed du monde (religion, hameaux libres, garanties joueur…) */
    /* DÉBRAYAGE DE L'IA (miroir scps_api) : le pays « joueur » passe sous la MAIN
     * HUMAINE — ai_on=false le retire des décisions ; human_player le retire des
     * fuites non gardées par ai_on. Les systèmes PASSIFS tournent pour lui. */
    s->human_player = s->player;
    s->ai_on[s->player] = false;
    warhost_set_human(s->player);   /* l'armée du joueur ne s'auto-mobilise pas */
    econ_set_human(s->player);      /* §NF : la construction autonome le skippe */
    feed_set_focus(s->player);      /* le fil d'évènements suit le joueur */
    for (int t=0; t<GEN_BOOT_DAYS; t++){ sim_day(s, w); g_gen_day=t+1; }   /* amorce : une carte déjà vivante */
    g_age_alert_seen = s->ev->ages.last_dawned;   /* les âges de l'amorce ne sonnent pas */
    g_sim_ready = true;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * SAUVEGARDE PARTAGÉE (scps_save) : wrappers minces gardant les noms historiques
 * du shell — savetest/fuzztest les appellent tels quels. Le format est le MÊME
 * que la façade Godot (un seul format, une seule vérité).
 * ═══════════════════════════════════════════════════════════════════════════ */
static bool game_save(int slot, World *w, Sim *s, const WorldParams *params){
    return scps_save_game(slot, w, s, params, (int)g_player_heritage, g_setup_ethos);
}
static bool save_sane(const World *w, const Sim *s, int player){ return scps_save_sane(w, s, player); }
static int game_load(int slot, World *w, Sim *s, WorldParams *params){
    int her=(int)g_player_heritage, eth=g_setup_ethos;
    int rc = scps_load_game(slot, w, s, params, &her, &eth);
    if (rc==0){
        g_player_heritage=(Heritage)her; g_setup_ethos=eth;   /* l'identité de setup revient au shell */
        /* miroir scps_api (chemin load) : la MAIN HUMAINE — warhost_set_human est
         * déjà rétabli DANS scps_load_game (P0-2) ; le reste se rétablit ici. */
        s->human_player = s->player;
        s->ai_on[s->player] = false;
        econ_set_human(s->player);
        feed_set_focus(s->player);
        g_age_alert_seen = s->ev->ages.last_dawned;   /* pas d'alerte fantôme au chargement */
        g_sim_ready = true;
    }
    return rc;
}

int main(int argc, char **argv) {
    bool savetest = false;
    bool fuzztest = false;      /* P0-1 : forge des compteurs (save_sane les rejette) + fuzz d'octets (jamais de crash) */
    uint32_t the_seed = 0; bool have_seed = false;
    for (int i=1;i<argc;i++) {
        if      (!strcmp(argv[i], "--savetest")) savetest=true;   /* sauver-recharger = continuation identique */
        else if (!strcmp(argv[i], "--fuzztest")) fuzztest=true;   /* forge de compteurs + fuzz d'octets du save */
        else if (!strcmp(argv[i], "--dump-lang")) {   /* écrit scps_lang.txt (tout le texte joueur, éditable) puis sort */
            int nw = lang_dump_file("scps_lang.txt");
            printf("[scps] scps_lang.txt écrit (%d entrées) — édite le texte, relance le jeu.\n", nw);
            return nw>0 ? 0 : 1;
        }
        else if (!strcmp(argv[i], "--dump-readout")) {   /* manifeste de la membrane (bandes/labels/hovers) puis sort */
            int nb = readout_dump_file("scps_readout.txt");
            printf("[scps] scps_readout.txt écrit (%d bandes) — manifeste de la membrane (outillage).\n", nb);
            return nb>0 ? 0 : 1;
        }
        else if (!strcmp(argv[i], "--lang-audit")) {  /* confronte un scps_lang.txt au set COMPILÉ (IDs périmés/manquants) */
            const char *path = (i+1<argc && argv[i+1][0]!='-') ? argv[++i] : "scps_lang.txt";
            int an = lang_audit_file(path, stdout);
            return an==0 ? 0 : 1;                      /* 0 = sain, sinon (anomalies ou absent) ≠ 0 */
        }
        else if (!strcmp(argv[i], "--dump-fnv")) {    /* manifeste d'empreintes (ID<TAB>hash<TAB>texte) puis sort */
            int nw = lang_dump_fingerprints("scps_lang.fnv");
            printf("[scps] scps_lang.fnv écrit (%d entrées).\n", nw);
            return nw>0 ? 0 : 1;
        }
        else { the_seed = (uint32_t)strtoul(argv[i], NULL, 10); have_seed = true; }
    }

    if (!savetest && !fuzztest) {
        printf("[scps] viewer = HARNAIS HEADLESS (le front interactif est GODOT).\n"
               "  Outils : --savetest --fuzztest --dump-lang --dump-readout --lang-audit --dump-fnv\n");
        return 0;
    }

    /* §SURCHARGE TEXTE : si scps_lang.txt est présent, il REMPLACE le texte joueur
     * (par ID). Absent → défauts compilés. Display-only. */
    { int nov = lang_load_file("scps_lang.txt");
      if (nov>0) printf("[scps] scps_lang.txt chargé : %d libellé(s) surchargé(s).\n", nov); }
    /* MODTOOLS : surcharge des valeurs (prix/recettes/tech/unités) si SCPS_MODS pointe un fichier. */
    { const char *m=getenv("SCPS_MODS");
      if (m && *m){ econ_moddata_load(m); tech_moddata_load(m); army_moddata_load(m); } }

    /* La simulation partagée (mêmes membres que le shell — sans aucun état d'UI). */
    Sim sim = {0};
    sim.econ = (WorldEconomy*)    malloc(sizeof(WorldEconomy));
    sim.wp   = (WorldProsperity*) malloc(sizeof(WorldProsperity));
    sim.wl   = (WorldLegitimacy*) malloc(sizeof(WorldLegitimacy));
    sim.net  = (TradeNetwork*)    malloc(sizeof(TradeNetwork));
    sim.ts   = (TechState*)       calloc(SCPS_MAX_COUNTRY, sizeof(TechState));
    sim.sc   = (Statecraft*)      malloc(sizeof(Statecraft));
    sim.ag   = (AgencyState*)     malloc(sizeof(AgencyState));
    sim.ev   = (EventsState*)     malloc(sizeof(EventsState));
    sim.drift= (ModifierStack*)   malloc(sizeof(ModifierStack));
    sim.dp   = (DiploState*)      malloc(sizeof(DiploState));
    sim.rn   = (RouteNetwork*)    malloc(sizeof(RouteNetwork));
    sim.rs   = (RevoltState*)     malloc(sizeof(RevoltState));
    sim.host = (WarHost*)         malloc(sizeof(WarHost));
    sim.camp = (Campaign*)        malloc(sizeof(Campaign));
    sim.missions = (MissionsState*) malloc(sizeof(MissionsState));
    sim.navy = (NavyState*)       malloc(sizeof(NavyState));
    sim.ai   = (AiActor*)         calloc(SCPS_MAX_COUNTRY, sizeof(AiActor));
    sim.ai_on= (bool*)            calloc(SCPS_MAX_COUNTRY, sizeof(bool));
    sim.eg   = (EndgameState*)    calloc(1, sizeof(EndgameState));

    World *world = (World*)malloc(sizeof(World));
    if (!world) { fprintf(stderr,"OOM\n"); return 1; }

    uint32_t seed = have_seed ? the_seed : (uint32_t)time(NULL);
    WorldParams params = worldparams_default(seed);
    printf("[scps] Génération (graine %u)…\n", seed);
    world_generate(world, &params);
    sim_rebuild(&sim, world);

    /* ── --savetest : LA VÉRIF du brief (4) — sauver puis recharger restitue la
     * partie AU JOUR PRÈS : on avance N jours, on sauve, on avance M jours (digest A) ;
     * on recharge, on ré-avance M jours (digest B) ; A doit ÉGALER B. ── */
    if (savetest){
        #define DIGEST(tag) do{ double dpop=0,dgld=0; long dtech=0; unsigned long downer=5381; \
            for (int r=0;r<sim.econ->n_regions;r++){ const RegionEconomy *re=&sim.econ->region[r]; \
                for (int c2=0;c2<CLASS_COUNT;c2++) dpop+=re->strata[c2].pop; \
                dgld+=re->treasury; downer=downer*33+(unsigned long)(re->owner+2); } \
            for (int c2=0;c2<SCPS_MAX_COUNTRY;c2++) dtech+=sim.ts[c2].n_unlocked; \
            snprintf(tag,sizeof tag,"day=%d pop=%.1f or=%.1f tech=%ld own=%lu pays=%d frondes=%d", \
                     sim.day,dpop,dgld,dtech,downer,world->n_countries,sim.dp->n_frondes); }while(0)
        char dA[200], dB[200];
        for (int d2=0;d2<600;d2++) sim_day(&sim, world);
        if (!game_save(3, world, &sim, &params)){ printf("savetest: ÉCHEC d'écriture\n"); return 1; }
        for (int d2=0;d2<400;d2++) sim_day(&sim, world);
        DIGEST(dA);
        int rc=game_load(3, world, &sim, &params);
        if (rc!=0){ printf("savetest: ÉCHEC de lecture (%d)\n", rc); return 1; }
        for (int d2=0;d2<400;d2++) sim_day(&sim, world);
        DIGEST(dB);
        bool same = (strcmp(dA,dB)==0);
        /* 2e contrôle : un octet ALTÉRÉ au milieu du payload chiffré → REFUS net. */
        bool tamper_ok=false;
        { FILE *tf=fopen(save_slot_path(3),"r+b");
          if (tf){ SaveHeader th;
            if (fread(&th,sizeof th,1,tf)==1){
                long mid=(long)sizeof th + (long)th.payload/2;
                fseek(tf,mid,SEEK_SET); int c2=fgetc(tf);
                fseek(tf,mid,SEEK_SET); fputc(c2^0x5A,tf);
            }
            fclose(tf);
            tamper_ok = (game_load(3, world, &sim, &params)!=0);   /* doit ÉCHOUER */
          } }
        printf("A: %s\nB: %s\n  altération d'un octet → %s\n"
               "══════════════════════════════════════\n BILAN : %d réussis, %d échoués\n",
               dA, dB, tamper_ok?"REFUSÉE (empreinte)":"ACCEPTÉE (BUG)",
               (same?1:0)+(tamper_ok?1:0), (same?0:1)+(tamper_ok?0:1));
        return (same&&tamper_ok)?0:1;
    }

    /* ── --fuzztest : LE DURCISSEMENT « contrat public » du save (audit P0-1). (1) chaque COMPTEUR/
     * INDEX désérialisé, forgé HORS-BORNE, doit être REJETÉ par save_sane (le vecteur d'écriture hors-bornes) ;
     * (2) un FUZZ d'octets du fichier (en-tête + payload) : game_load doit TOUJOURS rendre proprement — jamais
     * planter (un OOB serait attrapé sous ASan). Headless : SDL_VIDEODRIVER=dummy ./scps_viewer --fuzztest 9. ── */
    {
        for (int d2=0; d2<365*5; d2++) sim_day(&sim, world);   /* de l'état RÉEL : ordres, armées, révoltes */
        int ok=0, ko=0;
        #define FZ(cond,msg) do{ if (cond) ok++; else { ko++; printf("  ✗ %s\n",(msg)); } }while(0)
        FZ(save_sane(world,&sim,sim.player), "sim valide accepté par save_sane");
        { int v=sim.ag->n; sim.ag->n=SCPS_MAX_BUILDS+1; FZ(!save_sane(world,&sim,sim.player), "agency.n hors-borne REJETÉ"); sim.ag->n=v; }
        { int v=sim.ag->n; sim.ag->n=-1;                FZ(!save_sane(world,&sim,sim.player), "agency.n négatif REJETÉ");   sim.ag->n=v; }
        if (sim.ag->n>0){ int v=sim.ag->order[0].region; sim.ag->order[0].region=sim.econ->n_regions+9;
            FZ(!save_sane(world,&sim,sim.player), "ordre.region OOB REJETÉ (le vecteur purge_slice)"); sim.ag->order[0].region=v; }
        { int v=sim.rs->count; sim.rs->count=REVOLT_MAX+5; FZ(!save_sane(world,&sim,sim.player), "revolt.count hors-borne REJETÉ"); sim.rs->count=v; }
        { int v=sim.camp->army[0].force.n_units; sim.camp->army[0].force.n_units=ARMY_MAX_UNITS+1;
            FZ(!save_sane(world,&sim,sim.player), "camp army.n_units hors-borne REJETÉ"); sim.camp->army[0].force.n_units=v; }
        { int v=sim.host->army[0].n_units; sim.host->army[0].n_units=ARMY_MAX_UNITS+13;
            FZ(!save_sane(world,&sim,sim.player), "host army.n_units hors-borne REJETÉ"); sim.host->army[0].n_units=v; }
        { int v=intertrade_region_hub(0); intertrade_debug_set_hub_of(0, 20000);   /* défaut #5 : au-delà de n_regions ET de SCPS_MAX_REG */
            FZ(!save_sane(world,&sim,sim.player), "intertrade hub_of hors-borne REJETÉ"); intertrade_debug_set_hub_of(0, v); }
        long flips=0;
        if (game_save(3, world, &sim, &params)){
            const char *fp=save_slot_path(3);
            long fsz=0; { FILE *g=fopen(fp,"rb"); if(g){ fseek(g,0,SEEK_END); fsz=ftell(g); fclose(g); } }
            long hdr=(long)sizeof(SaveHeader);
            /* tout l'EN-TÊTE octet par octet (magic/version/payload/nonce/ck) + un échantillon du payload
             * (protégé de toute façon par l'empreinte FNV). */
            long limit = (fsz < hdr+2048) ? fsz : hdr+2048;
            for (long b=0; b<limit; b += (b<hdr?1:128)){
                FILE *g=fopen(fp,"r+b"); if(!g) break;
                fseek(g,b,SEEK_SET); int c=fgetc(g);
                fseek(g,b,SEEK_SET); fputc(c^0xFF,g); fclose(g);
                (void)game_load(3, world, &sim, &params);   /* doit RENDRE (0/1/2) — jamais planter */
                flips++;
                FILE *g2=fopen(fp,"r+b"); if(g2){ fseek(g2,b,SEEK_SET); fputc(c,g2); fclose(g2); }   /* restaure l'octet */
            }
            FZ(flips>0, "fuzz d'octets exécuté (game_load a toujours rendu — aucun crash)");
        } else FZ(0, "game_save a écrit le fichier de fuzz");
        #undef FZ
        printf("  (%ld octets flippés ; save_sane a rejeté chaque forge ; aucun crash)\n", flips);
        printf("══════════════════════════════════════\n BILAN : %d réussis, %d échoués\n", ok, ko);
        return ko?1:0;
    }
}
