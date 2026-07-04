/*
 * scps_save.c — implémentation de la sauvegarde partagée (voir scps_save.h).
 * EXTRAIT de viewer.c (verbatim, à 3 ajustements près : l'identité de culture passe
 * en paramètre au lieu des globals du viewer ; le nonce ne dépend plus de SDL ; une
 * section CULT persiste les slots de culture). Le format reste IDENTIQUE par ailleurs.
 */
#include "scps_save.h"
#include "scps_crypt.h"     /* scrypt_stream, scrypt_fnv1a */
#include "scps_save_io.h"   /* save_write_atomic */
#include "scps_tune.h"      /* tune_active_string */
#include "scps_heritage.h"   /* culture_slots_save/load (section CULT) */
#include "scps_religion.h"  /* religion_save/load (section RELG, v37) */
#include "scps_demography.h"/* demography_dyn_id_rebase */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#  include <direct.h>
#  define scps_mkdir(p) _mkdir(p)
#else
#  include <sys/stat.h>
#  include <sys/types.h>
#  define scps_mkdir(p) mkdir((p), 0755)
#endif

typedef struct { int32_t day, year, player, prev_dawned; uint32_t camp_rng;
                 int32_t heritage, ethos; int16_t prev_owner[SCPS_MAX_REG];
                 int32_t player_age_engaged;   /* v48 : engagement d'âge JOUEUR (§7) */
                 int32_t diplo_ready_day; } SaveMisc;   /* v50 : le DIPLOMATE (1 acte / 2 mois) */

const char *save_slot_path(int slot){
    static char p[64]; snprintf(p,sizeof p,"saves/slot_%d.scps",slot); return p;
}
bool scps_save_slot_info(int slot, SaveHeader *out){
    FILE *f=fopen(save_slot_path(slot),"rb");
    if (!f) return false;
    bool ok = fread(out,sizeof *out,1,f)==1 && out->magic==SAVE_MAGIC;
    fclose(f); return ok;
}
#define SV_TAG(a,b,c,d) ((uint32_t)(a)|((uint32_t)(b)<<8)|((uint32_t)(c)<<16)|((uint32_t)(d)<<24))
/* REGISTRE DES SECTIONS (X-macro) — l'ORDRE et les tags vivent ICI. */
#define SV_SECTIONS(X) \
    X(WRLD,'W','R','L','D')  /* World                       */ \
    X(ECON,'E','C','O','N')  /* WorldEconomy                */ \
    X(PROS,'P','R','O','S')  /* WorldProsperity             */ \
    X(LEGI,'L','E','G','I')  /* WorldLegitimacy             */ \
    X(NETW,'N','E','T','W')  /* InfluenceNet                */ \
    X(TECH,'T','E','C','H')  /* TechState[pays]             */ \
    X(STAT,'S','T','A','T')  /* Statecraft                  */ \
    X(AGCY,'A','G','C','Y')  /* AgencyState                 */ \
    X(EVNT,'E','V','N','T')  /* EventsState                 */ \
    X(DRFT,'D','R','F','T')  /* DriftState                  */ \
    X(LABO,'L','A','B','O')  /* LaborEcon                   */ \
    X(DIPL,'D','I','P','L')  /* DiploState                  */ \
    X(RTES,'R','T','E','S')  /* RouteNetwork                */ \
    X(RVLT,'R','V','L','T')  /* RevoltState                 */ \
    X(MISS,'M','I','S','S')  /* MissionState                */ \
    X(CAMP,'C','A','M','P')  /* Campaign                    */ \
    X(NAVY,'N','A','V','Y')  /* NavyState                   */ \
    X(HARM,'H','A','R','M')  /* WarHost.army (sans scratch) */ \
    X(HLVY,'H','L','V','Y')  /* WarHost.levy                */ \
    X(AIAC,'A','I','A','C')  /* AiActor[pays]               */ \
    X(AION,'A','I','O','N')  /* ai_on[pays]                 */ \
    X(MISC,'M','I','S','C')  /* SaveMisc                    */ \
    X(ITRD,'I','T','R','D')  /* intertrade (états statiques) */ \
    X(AGYS,'A','G','Y','S')  /* agency (états statiques)    */ \
    X(DPLS,'D','P','L','S')  /* diplo (statiques)           */ \
    X(FACT,'F','A','C','T')  /* factions (statiques)        */ \
    X(CRDT,'C','R','D','T')  /* dette : g_creditor[]        */ \
    X(PCAP,'P','C','A','P')  /* limiteur de production (v24) */ \
    X(CULT,'C','U','L','T')  /* v36 : slots de culture + map cid→slot */ \
    X(RELG,'R','E','L','G')  /* v37 : registre religion + liens pays */ \
    X(WILD,'W','I','L','D')  /* v48 : compteurs de contact des hameaux libres (ralliement) */
#define SV_DECL_TAG(name,a,b,c,d) enum { SVT_##name = SV_TAG(a,b,c,d) };
SV_SECTIONS(SV_DECL_TAG)
#undef SV_DECL_TAG
static bool sv_w(FILE *f, uint32_t tag, const void *p, size_t sz){
    uint32_t z=(uint32_t)sz;
    return fwrite(&tag,4,1,f)==1 && fwrite(&z,4,1,f)==1 && (sz==0 || fwrite(p,sz,1,f)==1);
}
static bool sv_r(FILE *f, uint32_t tag, void *p, size_t sz){
    uint32_t t,z;
    if (fread(&t,4,1,f)!=1 || fread(&z,4,1,f)!=1) return false;
    if (t!=tag || z!=(uint32_t)sz) return false;
    return sz==0 || fread(p,sz,1,f)==1;
}

bool scps_save_game(int slot, World *w, Sim *s, const WorldParams *params, int setup_heritage, int setup_ethos){
    if (slot<1 || slot>3) return false;
    scps_mkdir("saves");
    FILE *f=tmpfile();
    if (!f) return false;
    SaveHeader h; memset(&h,0,sizeof h);
    h.magic=SAVE_MAGIC; h.version=SAVE_VERSION; h.seed=params->seed;
    h.day=s->day; h.year=s->year; h.player=s->player; h.params=*params;
    h.stamp=(int64_t)time(NULL);
    { const char *tstr=tune_active_string();
      h.tune_ck=(uint32_t)scrypt_fnv1a(tstr, strlen(tstr)); }
    { int nreg=0; for (int r=0;r<s->econ->n_regions;r++) if (s->econ->region[r].owner==s->player) nreg++;
      snprintf(h.line,sizeof h.line,"An %d — %s, %d région(s)",
               s->year, (s->player>=0&&s->player<w->n_countries)?w->country[s->player].name:"?", nreg); }
    bool ok = true;
    ok&=sv_w(f,SVT_WRLD, w,        sizeof *w);
    /* ECON : WorldEconomy embarque désormais prov_adj, un POINTEUR TAS (adjacence de
     * provinces, ~2.7 Mo, possédé par le module econ.c). On écrit le struct tel quel —
     * les octets du pointeur finissent dans le fichier mais ne sont JAMAIS relus comme
     * une adresse valide : au chargement on l'écrase à NULL puis on REBÂTIT l'adjacence
     * (régions ET provinces) + le cache region_rep_prov via econ_build_adjacency, qui
     * alloue/possède sa propre copie module-static (cf. scps_econ.c g_prov_adj). Ne PAS
     * faire confiance à la valeur désérialisée du pointeur, jamais. */
    ok&=sv_w(f,SVT_ECON, s->econ,  sizeof *s->econ);
    ok&=sv_w(f,SVT_PROS, s->wp,    sizeof *s->wp);
    ok&=sv_w(f,SVT_LEGI, s->wl,    sizeof *s->wl);
    ok&=sv_w(f,SVT_NETW, s->net,   sizeof *s->net);
    ok&=sv_w(f,SVT_TECH, s->ts,    sizeof(TechState)*SCPS_MAX_COUNTRY);
    ok&=sv_w(f,SVT_STAT, s->sc,    sizeof *s->sc);
    ok&=sv_w(f,SVT_AGCY, s->ag,    sizeof *s->ag);
    ok&=sv_w(f,SVT_EVNT, s->ev,    sizeof *s->ev);
    ok&=sv_w(f,SVT_DRFT, s->drift, sizeof *s->drift);
    ok&=sv_w(f,SVT_LABO, s->labor, sizeof *s->labor);
    ok&=sv_w(f,SVT_DIPL, s->dp,    sizeof *s->dp);
    ok&=sv_w(f,SVT_RTES, s->rn,    sizeof *s->rn);
    ok&=sv_w(f,SVT_RVLT, s->rs,    sizeof *s->rs);
    ok&=sv_w(f,SVT_MISS, s->missions, sizeof *s->missions);
    ok&=sv_w(f,SVT_CAMP, s->camp,  sizeof *s->camp);
    ok&=sv_w(f,SVT_NAVY, s->navy,  sizeof *s->navy);
    ok&=sv_w(f,SVT_HARM, s->host->army, sizeof s->host->army);
    ok&=sv_w(f,SVT_HLVY, s->host->levy, sizeof s->host->levy);
    ok&=sv_w(f,SVT_AIAC, s->ai,    sizeof(AiActor)*SCPS_MAX_COUNTRY);
    ok&=sv_w(f,SVT_AION, s->ai_on, sizeof(bool)*SCPS_MAX_COUNTRY);
    { SaveMisc m; memset(&m,0,sizeof m);
      m.day=s->day; m.year=s->year; m.player=s->player; m.prev_dawned=s->prev_dawned;
      m.camp_rng=s->camp_rng; m.heritage=(int32_t)setup_heritage; m.ethos=(int32_t)setup_ethos;
      memcpy(m.prev_owner,s->prev_owner_mo,sizeof m.prev_owner);
      m.player_age_engaged=(int32_t)s->player_age_engaged;
      m.diplo_ready_day=(int32_t)s->diplo_ready_day;
      ok&=sv_w(f,SVT_MISC, &m, sizeof m); }
    ok&=sv_w(f,SVT_ITRD, NULL,0); intertrade_save(f);
    ok&=sv_w(f,SVT_AGYS, NULL,0); agency_save(f);
    ok&=sv_w(f,SVT_DPLS, NULL,0); diplo_save_statics(f);
    ok&=sv_w(f,SVT_FACT, NULL,0); faction_save(f);
    ok&=sv_w(f,SVT_CRDT, NULL,0); credit_save(f);
    ok&=sv_w(f,SVT_PCAP, NULL,0); econ_prodcap_save(f);
    if (s->eg) ok&=sv_w(f,SV_TAG('E','G','A','M'), s->eg, sizeof *s->eg);
    ok&=sv_w(f,SVT_CULT, NULL,0); culture_slots_save(f);   /* v36 : cultures composées (joueur + IA) */
    ok&=sv_w(f,SVT_RELG, NULL,0); religion_save(f);        /* v37 : registre religion + liens pays */
    ok&=sv_w(f,SVT_WILD, NULL,0); sim_wild_save(f);        /* v48 : compteurs de ralliement des hameaux */
    if (ok && fflush(f)!=0) ok=false;
    long psz = ok ? ftell(f) : -1;
    if (!ok || psz<0){ fclose(f); return false; }
    h.payload=(uint32_t)psz;
    uint8_t *img=(uint8_t*)malloc(sizeof h + (size_t)h.payload);
    if (!img){ fclose(f); return false; }
    uint8_t *pay = img + sizeof h;
    rewind(f);
    if (fread(pay,1,h.payload,f)!=h.payload){ free(img); fclose(f); return false; }
    fclose(f);
    h.plain_ck = scrypt_fnv1a(pay,h.payload);
    /* Nonce : « obfuscation, pas secret » — unicité suffisante (time^seed^ptr^seq^clock). */
    { static uint64_t seq=0; ++seq;
      h.nonce = ((uint64_t)time(NULL)<<32)
              ^ ((uint64_t)params->seed<<13) ^ (uint64_t)(uintptr_t)pay
              ^ (seq<<48) ^ (uint64_t)clock(); }
    h.flags = SAVE_F_CRYPT;
    scrypt_stream(h.nonce, pay, h.payload);
    memcpy(img,&h,sizeof h);
    ok = save_write_atomic(save_slot_path(slot), img, sizeof h + (size_t)h.payload);
    free(img);
    return ok;
}

bool scps_save_sane(const World *w, const Sim *s, int player){
    if (w->n_provinces <0 || w->n_provinces >SCPS_MAX_PROV)      return false;
    if (w->n_regions   <0 || w->n_regions   >SCPS_MAX_REG)       return false;
    /* le cache région→province représentative est TRUSTÉ au load (état sérialisé,
     * continuation déterministe) ⇒ chaque index désérialisé se REVALIDE ici. */
    for (int r=0;r<w->n_regions && r<SCPS_MAX_REG;r++){
        int rp=s->econ->region_rep_prov[r];
        if (rp < -1 || rp >= s->econ->n_prov) return false;
    }
    /* v49 — conseil : slot ∈ [-1, CANDS) et génération ∈ [-1, 120] (désérialisés, indexent
     * les fonctions de pool — une forge hors-borne est refusée net). */
    for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++)
        for (int st=0;st<SC_COUNCIL_SEATS;st++){
            if (s->sc->council[c][st] < -1 || s->sc->council[c][st] >= SC_COUNCIL_CANDS) return false;
            if (s->sc->council_gen[c][st] < -1 || s->sc->council_gen[c][st] > 120) return false;
        }
    if (w->n_countries <0 || w->n_countries >SCPS_MAX_COUNTRY)   return false;
    if (w->n_continents<0 || w->n_continents>SCPS_MAX_CONTINENT) return false;
    for (int c=0;c<w->n_countries;c++)
        if (credit_of(c) < -1 || credit_of(c) >= w->n_countries) return false;
    if (w->n_rivers    <0 || w->n_rivers    >SCPS_MAX_RIVERS)    return false;
    for (int i=0;i<w->n_rivers;i++)
        if (w->river[i].len<0 || w->river[i].len>SCPS_RIVER_MAXLEN) return false;
    if (player<0 || player>=w->n_countries) return false;
    for (int i=0;i<SCPS_N;i++){ const Cell *c=&w->cell[i];
        if (c->province < -1 || c->region    < -1 ||
            c->country  < -1 || c->continent < -1) return false;
        if (c->province >= w->n_provinces || c->region    >= w->n_regions ||
            c->country  >= w->n_countries || c->continent >= w->n_continents) return false; }
    for (int p=0;p<w->n_provinces;p++){ const Province *pr=&w->province[p];
        if (pr->region < 0 || pr->country < 0) return false;
        if (pr->region >= w->n_regions || pr->country >= w->n_countries) return false; }
    for (int r=0;r<w->n_regions;r++){ const Region *rg=&w->region[r];
        if (rg->n_provinces<0 || rg->n_provinces>12 || rg->country< -1 || rg->country>=w->n_countries) return false;
        for (int k=0;k<rg->n_provinces;k++)
            if (rg->province_ids[k]<0 || rg->province_ids[k]>=w->n_provinces) return false;
        if (!(rg->harbor>=0.f && rg->harbor<=1.f)) return false; }
    for (int c=0;c<w->n_countries;c++){ const Country *ct=&w->country[c];
        if (ct->n_regions<0 || ct->n_regions>32 || ct->capital_prov< -1 || ct->capital_prov>=w->n_provinces) return false;
        for (int k=0;k<ct->n_regions;k++)
            if (ct->region_ids[k]<0 || ct->region_ids[k]>=w->n_regions) return false; }
    if (s->econ->n_regions<0 || s->econ->n_regions>SCPS_MAX_REG) return false;
    for (int r=0;r<s->econ->n_regions;r++){ const RegionEconomy *re=&s->econ->region[r];
        if (re->owner < -1 || re->owner >= w->n_countries) return false;
        if (re->pop.n_groups<0 || re->pop.n_groups>SCPS_MAX_GROUPS) return false;
        if (!(re->annex_scar>=0.f && re->annex_scar<=1.f)) return false; }
    /* ÉCONOMIE PAR-PROVINCE (v47) : prov[] est LA VÉRITÉ désormais — mêmes bornes que
     * region[] ci-dessus (owner) + region (mirroir de World.province[].region, lu par
     * econ_tick pour agréger SANS World*) doit indexer une région du monde chargé. */
    if (s->econ->n_prov<0 || s->econ->n_prov>SCPS_MAX_PROV) return false;
    for (int p=0;p<s->econ->n_prov;p++){ const ProvinceEconomy *pe=&s->econ->prov[p];
        if (pe->owner < -1 || pe->owner >= w->n_countries) return false;
        if (pe->region < -1 || pe->region >= w->n_regions) return false;
        if (pe->pop.n_groups<0 || pe->pop.n_groups>SCPS_MAX_GROUPS) return false;
        for (int g=0;g<pe->pop.n_groups;g++){
            if (pe->pop.groups[g].arrival>=ARR_COUNT) return false;   /* mode d'arrivée borné (coeff de diffusion) */
            if (pe->pop.groups[g].home_reg < -1 || pe->pop.groups[g].home_reg >= w->n_regions) return false;  /* v52 : foyer du déplacé (indexe une région) */
            if (pe->pop.groups[g].faith    < -1 || pe->pop.groups[g].faith    >= RELIG_MAX)     return false;  /* v54 : foi PORTÉE par le groupe (id de religion, borne registre) */
        }
        if (!(pe->annex_scar>=0.f && pe->annex_scar<=1.f)) return false; }
    /* v50 — chantiers de colonisation : src/dst indexent prov[] (ou -1), délais/cadence
     * bornés (une forge hors-borne indexerait la fondation ou gèlerait la cadence). */
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){
        const struct ColonyWork *cw=&s->econ->colony[c];
        if (cw->src < -1 || cw->src >= s->econ->n_prov) return false;
        if (cw->dst < -1 || cw->dst >= s->econ->n_prov) return false;
        if (cw->days_left<0 || cw->days_left>2000 || cw->total_days<0 || cw->total_days>2000) return false;
        if (cw->cd_days<0 || cw->cd_days>2000) return false;
        if (!(cw->yield>=0.f && cw->yield<=1.f) || !(cw->seed_base>=0.f && cw->seed_base<=1e6f)) return false; }
    if (s->rn->n<0 || s->rn->n>SCPS_MAX_ROUTES) return false;
    for (int i=0;i<s->rn->n;i++){ const TradeRoute *rt=&s->rn->route[i];
        if (rt->ra<0 || rt->ra>=s->econ->n_regions || rt->rb<0 || rt->rb>=s->econ->n_regions) return false;
        if (rt->choke_region< -1 || rt->choke_region>=s->econ->n_regions) return false; }
    for (int i=0;i<SCPS_MAX_COUNTRY;i++){ const FieldArmy *a=&s->camp->army[i];
        if (!a->active) continue;
        if (a->owner<0 || a->owner>=w->n_countries) return false;
        if (a->loc <0 || a->loc >=s->econ->n_regions) return false;
        if (a->dest< -1 || a->dest>=s->econ->n_regions || a->next< -1 || a->next>=s->econ->n_regions) return false; }
    for (int i=0;i<SCPS_MAX_COUNTRY;i++){ const Navy *nv=&s->navy->n[i];
        for (int t=0;t<HULL_COUNT;t++) if (nv->hull[t]<0 || nv->hull[t]>100000) return false;
        if (nv->at_sea<0 || nv->build_hull<-1 || nv->build_hull>=HULL_COUNT) return false;
        if (nv->home_port< -1 || nv->home_port>=s->econ->n_regions) return false; }
    for (int r=0;r<s->econ->n_regions && r<SCPS_MAX_REG;r++)
        if (s->dp->occupier[r] < -1 || s->dp->occupier[r] >= w->n_countries) return false;
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){
        if (s->dp->suzerain[c] < -1 || s->dp->suzerain[c] >= w->n_countries) return false;
        if (!(s->dp->v_integration[c]>=0.f && s->dp->v_integration[c]<=1.f)) return false;
        if (!(s->dp->v_annex[c]      >=0.f && s->dp->v_annex[c]      <=1.f)) return false; }
    for (int i=0;i<SCPS_MAX_COUNTRY;i++)
        if (s->camp->army[i].taken_region < -1 || s->camp->army[i].taken_region >= s->econ->n_regions) return false;
    if (s->ag){
        if (s->ag->n < 0 || s->ag->n > SCPS_MAX_BUILDS) return false;
        for (int i=0;i<s->ag->n;i++){ const BuildOrder *o=&s->ag->order[i];
            if (o->region < -1 || o->region >= s->econ->n_regions) return false;
            if ((o->kind==AGY_RELOCATE || o->kind==AGY_COLONIZE) &&
                (o->param < -1 || o->param >= s->econ->n_regions)) return false; }
    }
    if (s->rs){
        if (s->rs->count < 0 || s->rs->count > REVOLT_MAX) return false;
        for (int i=0;i<s->rs->count;i++){
            if (s->rs->list[i].region < -1 || s->rs->list[i].region >= s->econ->n_regions) return false;
            /* Phase 3a — le pays rebelle incarné (-1 = aucun) indexe w->country[] : borne. */
            if (s->rs->list[i].rebel_country < -1 || s->rs->list[i].rebel_country >= w->n_countries) return false;
        }
    }
    for (int i=0;i<SCPS_MAX_COUNTRY;i++){
        if (s->camp->army[i].force.n_units < 0 || s->camp->army[i].force.n_units > ARMY_MAX_UNITS) return false;
        if (s->host && (s->host->army[i].n_units < 0 || s->host->army[i].n_units > ARMY_MAX_UNITS)) return false;
    }
    if (s->eg) {
        const EndgameState *eg = s->eg;
        if ((int)eg->fin < 0 || (int)eg->fin > (int)FIN_ASCENSION) return false;
        if ((int)eg->merv < 0 || (int)eg->merv > (int)MERV_ASCENDED) return false;
        if (eg->epicenter_reg < -1 || eg->epicenter_reg >= s->econ->n_regions) return false;
        if (eg->fauteur_country < -1 || eg->fauteur_country >= w->n_countries) return false;
        if (eg->merv_country    < -1 || eg->merv_country    >= w->n_countries) return false;
        if (eg->merv_site_reg   < -1 || eg->merv_site_reg   >= s->econ->n_regions) return false;
        if (eg->n_sunken < 0 || eg->n_sunken > SCPS_MAX_REG) return false;
        if (eg->sink_pending < 0) return false;
        if (eg->thorn_front_n < 0 || eg->thorn_front_n > SCPS_THORN_FRONT_MAX) return false;
        for (int i = 0; i < eg->thorn_front_n; i++)
            if (eg->thorn_front[i] < 0 || eg->thorn_front[i] >= SCPS_N) return false;
        if (eg->cold_offset < 0.0f || eg->cold_offset > 1.0f) return false;
        if (eg->merv_progress < 0.0f || eg->merv_progress > 1.0f) return false;
    }
    if (!director_save_sane(s->ev, SCPS_MAX_COUNTRY*SCPS_MAX_COUNTRY)) return false;
    return true;
}

#define SAVE_MAX_PAYLOAD (256u<<20)   /* plafond de vraisemblance : pas de malloc(4 Go) sur en-tête forgé */
int scps_load_game(int slot, World *w, Sim *s, WorldParams *params, int *out_heritage, int *out_ethos){
    if (slot<1 || slot>3) return 1;
    FILE *f=fopen(save_slot_path(slot),"rb");
    if (!f) return 1;
    SaveHeader h;
    if (fread(&h,sizeof h,1,f)!=1 || h.magic!=SAVE_MAGIC){ fclose(f); return 1; }
    if (h.version!=SAVE_VERSION){ fclose(f); return 2; }
    if (h.payload==0 || h.payload>SAVE_MAX_PAYLOAD){ fclose(f); return 1; }
    { uint8_t *buf=(uint8_t*)malloc(h.payload?h.payload:1);
      if (!buf){ fclose(f); return 1; }
      if (fread(buf,1,h.payload,f)!=h.payload){ free(buf); fclose(f); return 1; }
      fclose(f);
      if (h.flags & SAVE_F_CRYPT) scrypt_stream(h.nonce, buf, h.payload);
      if (scrypt_fnv1a(buf,h.payload)!=h.plain_ck){ free(buf); return 1; }
      f=tmpfile();
      if (!f){ free(buf); return 1; }
      if (fwrite(buf,1,h.payload,f)!=h.payload){ free(buf); fclose(f); return 1; }
      free(buf); rewind(f); }
    long p0=ftell(f);
    bool ok=true;
    ok&=sv_r(f,SVT_WRLD, w,        sizeof *w);
    ok&=sv_r(f,SVT_ECON, s->econ,  sizeof *s->econ);
    /* prov_adj est un POINTEUR TAS — la valeur désérialisée est une adresse d'un AUTRE
     * process/run, jamais valide ici : écrasée puis REBÂTIE (pure géographie). En revanche
     * adj[] et region_rep_prov[] sont des ÉTATS SÉRIALISÉS et on les GARDE : les recalculer
     * à l'état courant divergeait du fil continu (le rep « province la plus peuplée » bouge
     * avec la pop — sauve-recharge ≠ continuation, pris par scps_viewer --savetest). Le
     * cache rep désérialisé est BORNÉ par scps_save_sane (P0-1). */
    if (ok){ s->econ->prov_adj = NULL; econ_rebuild_prov_adj(s->econ, w); }
    ok&=sv_r(f,SVT_PROS, s->wp,    sizeof *s->wp);
    ok&=sv_r(f,SVT_LEGI, s->wl,    sizeof *s->wl);
    ok&=sv_r(f,SVT_NETW, s->net,   sizeof *s->net);
    ok&=sv_r(f,SVT_TECH, s->ts,    sizeof(TechState)*SCPS_MAX_COUNTRY);
    ok&=sv_r(f,SVT_STAT, s->sc,    sizeof *s->sc);
    ok&=sv_r(f,SVT_AGCY, s->ag,    sizeof *s->ag);
    ok&=sv_r(f,SVT_EVNT, s->ev,    sizeof *s->ev);
    ok&=sv_r(f,SVT_DRFT, s->drift, sizeof *s->drift);
    ok&=sv_r(f,SVT_LABO, s->labor, sizeof *s->labor);
    ok&=sv_r(f,SVT_DIPL, s->dp,    sizeof *s->dp);
    ok&=sv_r(f,SVT_RTES, s->rn,    sizeof *s->rn);
    ok&=sv_r(f,SVT_RVLT, s->rs,    sizeof *s->rs);
    ok&=sv_r(f,SVT_MISS, s->missions, sizeof *s->missions);
    ok&=sv_r(f,SVT_CAMP, s->camp,  sizeof *s->camp);
    ok&=sv_r(f,SVT_NAVY, s->navy,  sizeof *s->navy);
    ok&=sv_r(f,SVT_HARM, s->host->army, sizeof s->host->army);
    ok&=sv_r(f,SVT_HLVY, s->host->levy, sizeof s->host->levy);
    ok&=sv_r(f,SVT_AIAC, s->ai,    sizeof(AiActor)*SCPS_MAX_COUNTRY);
    ok&=sv_r(f,SVT_AION, s->ai_on, sizeof(bool)*SCPS_MAX_COUNTRY);
    { SaveMisc m;
      ok&=sv_r(f,SVT_MISC, &m, sizeof m);
      if (ok){ s->day=m.day; s->year=m.year; s->player=m.player; s->prev_dawned=m.prev_dawned;
               s->camp_rng=m.camp_rng;
               if (m.heritage <0 || m.heritage >=(int32_t)HERITAGE_COUNT) m.heritage =(int32_t)HERITAGE_ADAPTATIF;
               if (m.ethos<0 || m.ethos>=(int32_t)ETHOS_COUNT)    m.ethos=0;
               if (out_heritage)  *out_heritage  = (int)m.heritage;
               if (out_ethos) *out_ethos = (int)m.ethos;
               memcpy(s->prev_owner_mo,m.prev_owner,sizeof m.prev_owner);
               s->player_age_engaged = (m.player_age_engaged>=-1 && m.player_age_engaged<1024)
                                       ? (int)m.player_age_engaged : -1;   /* v48 : borné (forge → -1) */
               s->diplo_ready_day = (m.diplo_ready_day>=0 && m.diplo_ready_day<=m.day+120)
                                    ? (int)m.diplo_ready_day : 0; } }   /* v50 : borné (forge → dispo) */
    ok&=sv_r(f,SVT_ITRD, NULL,0); ok&=intertrade_load(f);
    ok&=sv_r(f,SVT_AGYS, NULL,0); ok&=agency_load(f);
    ok&=sv_r(f,SVT_DPLS, NULL,0); ok&=diplo_load_statics(f);
    ok&=sv_r(f,SVT_FACT, NULL,0); ok&=faction_load(f);
    ok&=sv_r(f,SVT_CRDT, NULL,0); ok&=credit_load(f);
    ok&=sv_r(f,SVT_PCAP, NULL,0); ok&=econ_prodcap_load(f);
    if (s->eg) ok&=sv_r(f,SV_TAG('E','G','A','M'), s->eg, sizeof *s->eg);
    ok&=sv_r(f,SVT_CULT, NULL,0); ok&=culture_slots_load(f);   /* v36 : cultures composées */
    ok&=sv_r(f,SVT_RELG, NULL,0); ok&=(religion_load(f)==0);   /* v37 : religion + liens pays */
    ok&=sv_r(f,SVT_WILD, NULL,0); ok&=sim_wild_load(f);        /* v48 : compteurs de ralliement des hameaux */
    long p1=ftell(f); fclose(f);
    if (!ok || (uint32_t)(p1-p0)!=h.payload) return 1;
    if (!scps_save_sane(w, s, s->player)) return 1;
    demography_dyn_id_rebase(s->econ);
    *params=h.params;
    warhost_set_human(s->player);
    { const char *tstr=tune_active_string();
      if ((uint32_t)scrypt_fnv1a(tstr, strlen(tstr)) != h.tune_ck)
          fprintf(stderr, "[save] AVERTISSEMENT : SCPS_TUNE actif ≠ celui de la sauvegarde — la partie évoluera "
                          "sous d'AUTRES règles (replays / graines partagées invalides).\n"); }
    /* (l'état « prêt » du front est tenu par l'appelant : la façade pose ScpsSim.ready ;
     * le Sim moteur n'a pas de champ ready.) */
    return 0;
}
