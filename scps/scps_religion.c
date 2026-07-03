/*
 * scps_religion.c — module fondation religion (voir scps_religion.h).
 * PUR : tables statiques + primitives. Aucun câblage moteur, aucune sérialisation.
 */
#include "scps_religion.h"
#include <string.h>
#include <assert.h>

/* magnitudes Civ-tier, calibrables ICI */
#define M_COORD   1.0f   /* coord 0-10  */
#define M_HALF    0.5f
#define M_SOFT    0.6f   /* morale/cohésion/influence/agitation/assim/coercion/revenue */
#define M_HARD    0.8f
#define R_POP_UP  0.12f
#define R_POP_DN (-0.10f)
#define R_RES_UP  0.12f
#define R_RES_DN (-0.10f)
#define COLOR_SHIFT 40   /* variante "proche" */
#define COLOR_NEAR_MAX 64

const ReligPoleDef RELIG_POLES[RP_COUNT] = {
 [RP_FECONDITE] ={RA_SANG,   "relig.pole.fecondite", {{RC_POPGROWTH,R_POP_UP},{RC_I,M_HARD}}},
 [RP_OFFRANDE]  ={RA_SANG,   "relig.pole.offrande",  {{RC_STAB,M_COORD},{RC_POPGROWTH,R_POP_DN}}},
 [RP_TRANSE]    ={RA_FEU,    "relig.pole.transe",    {{RC_COHESION,M_HARD},{RC_K,-M_HARD}}},
 [RP_SILENCE]   ={RA_FEU,    "relig.pole.silence",   {{RC_K,M_COORD},{RC_COHESION,-M_SOFT}}},
 [RP_ACCUEIL]   ={RA_SEUIL,  "relig.pole.accueil",   {{RC_P,M_COORD},{RC_H,-M_COORD}}},
 [RP_MUR]       ={RA_SEUIL,  "relig.pole.mur",       {{RC_H,M_COORD},{RC_PE,-M_SOFT}}},
 [RP_COURONNE]  ={RA_SERMENT,"relig.pole.couronne",  {{RC_L,M_COORD},{RC_F,-M_COORD}}},
 [RP_ASSEMBLEE] ={RA_SERMENT,"relig.pole.assemblee", {{RC_F,M_COORD},{RC_K,-M_HALF}}},
 [RP_ANCETRES]  ={RA_VEILLE, "relig.pole.ancetres",  {{RC_STAB,M_HARD},{RC_ASSIM,-M_SOFT}}},
 [RP_CENDRE]    ={RA_VEILLE, "relig.pole.cendre",    {{RC_ASSIM,M_HARD},{RC_STAB,-M_SOFT}}},
 [RP_GNOSE]     ={RA_CANON,  "relig.pole.gnose",     {{RC_RESEARCH,R_RES_UP},{RC_ENTROPY,M_HARD}}},
 [RP_ORTHODOXIE]={RA_CANON,  "relig.pole.orthodoxie",{{RC_STAB,M_HARD},{RC_RESEARCH,R_RES_DN}}},
 [RP_FRUGALITE] ={RA_DON,    "relig.pole.frugalite", {{RC_COHESION,M_HARD},{RC_REVENUE,-M_SOFT}}},
 [RP_FASTE]     ={RA_DON,    "relig.pole.faste",     {{RC_INFLUENCE,M_HARD},{RC_COHESION,-M_SOFT}}},
 [RP_COURAGE]   ={RA_GLAIVE, "relig.pole.courage",   {{RC_MORALE,M_HARD},{RC_PE,-M_SOFT}}},
 [RP_TREVE]     ={RA_GLAIVE, "relig.pole.treve",     {{RC_INFLUENCE,M_HARD},{RC_MORALE,-M_SOFT}}},
};

/* deltas-self du crédo (sentinelle RC_COUNT). INDEXÉS PAR L'ENUM Credo RÉEL de
 * scps_culture.h : 0=CREDO_PLURALISTE · 1=CREDO_EVANGELISTE (« prosélyte ») ·
 * 2=CREDO_PURIFICATEUR (« loyaliste »). Noms d'ARRAY distincts des constantes d'enum
 * (sinon collision). « prosélyte » volontairement léger : son poids est RELATIONNEL
 * (conversion/CB/tension cross-entités), câblé en phases diplo/scholar, pas ici. */
static const ReligDelta g_credo_pluraliste[] = {
  {RC_P,M_COORD},{RC_PE,M_HALF},{RC_AGITATION,-M_SOFT},{RC_H,-M_COORD},{RC_I,M_HALF},{RC_COUNT,0}};
static const ReligDelta g_credo_evangeliste[] = {   /* « prosélyte » */
  {RC_ASSIM,M_SOFT},{RC_PE,-0.4f},{RC_COUNT,0}};
static const ReligDelta g_credo_purificateur[] = {  /* « loyaliste » */
  {RC_H,M_COORD},{RC_COHESION,M_SOFT},{RC_COERCION,M_HALF},{RC_P,-M_COORD},{RC_PE,-0.4f},{RC_COUNT,0}};

/* table indexée par valeur de l'enum Credo — ALIGNÉE sur l'ordre de scps_culture.h. */
static const ReligDelta* const RELIG_CREDO[CREDO_COUNT] = {
  g_credo_pluraliste,    /* CREDO_PLURALISTE   = 0 */
  g_credo_evangeliste,   /* CREDO_EVANGELISTE  = 1 (« prosélyte ») */
  g_credo_purificateur,  /* CREDO_PURIFICATEUR = 2 (« loyaliste »)  */
};

Religion g_religions[RELIG_MAX];
int      g_religion_count = 0;
static int g_relig_n_emp_ref = 0;   /* compte d'empires de GENÈSE → ancre le plafond ⌈N/3⌉ (stable) */

/* lien pays→religion (P3) + cache d'accumulateur (P4) + religion par région (P8). */
static int        g_country_religion[RELIG_MAX_COUNTRY];
static ReligAccum g_country_relig_acc[RELIG_MAX_COUNTRY];   /* P4 : acc caché par pays */
static int        g_region_religion[RELIG_MAX_REGION];      /* P8 : religion par région (-1=aucune) */
static int g_cr_init = 0;
static const ReligAccum g_zero_acc = {{0}};
static void cr_ensure(void){
  if(!g_cr_init){
    for(int i=0;i<RELIG_MAX_COUNTRY;i++) g_country_religion[i]=-1;
    for(int i=0;i<RELIG_MAX_REGION;i++)  g_region_religion[i]=-1;
    g_cr_init=1;
  }
}
int  religion_of_region(int rg){ cr_ensure(); return (rg>=0&&rg<RELIG_MAX_REGION)?g_region_religion[rg]:-1; }
/* pose la foi `rid` sur les NATIFS de souche de la région (rep-province) — fondation /
 * missionnaire / Contre-Réforme convertissent la SOUCHE ; la diaspora garde sa foi portée
 * (un migrant/réfugié ne se fait pas convertir d'office). */
static void region_set_native_faith(WorldEconomy *econ, int r, int rid){
  if(!econ || r<0 || r>=econ->n_regions) return;
  int rpid=econ_region_rep_province(econ, r);
  if(rpid<0 || rpid>=econ->n_prov) return;
  ProvincePop *pp=&econ->prov[rpid].pop;
  for(int i=0;i<pp->n_groups;i++)
    if(!pp->groups[i].diaspora) pp->groups[i].faith=rid;
}
void religion_set_region(WorldEconomy *econ, int rg, int rid){
  cr_ensure();
  if(rg<0||rg>=RELIG_MAX_REGION) return;
  region_set_native_faith(econ, rg, rid);
  g_region_religion[rg]=rid;   /* cache immédiat (le refresh dérivera ensuite) */
}
void religion_inherit_regions(const World *w, WorldEconomy *econ, int cid){
  if(!w) return;
  cr_ensure();
  int rid=religion_of_country(cid);
  for(int r=0;r<w->n_regions && r<RELIG_MAX_REGION;r++){
    if(w->region[r].country!=cid) continue;
    region_set_native_faith(econ, r, rid);   /* les natifs adoptent la foi d'État */
    g_region_religion[r]=rid;
  }
}

/* CACHE DÉRIVÉ (foi par groupe) : le culte DOMINANT d'une région = la foi majoritaire (en
 * pop) parmi ses groupes. La foi VIT sur le groupe (PopGroup.faith) ; g_region_religion n'en
 * est que le REFLET, recalculé au tick → religion_of_region reste l'API des 17 lecteurs
 * « dominant ». -1 = athée / aucun culte majoritaire. */
void religion_refresh_region(const WorldEconomy *econ, int r){
  cr_ensure();
  if(!econ || r<0 || r>=RELIG_MAX_REGION || r>=econ->n_regions) return;
  int rpid=econ_region_rep_province(econ, r);
  if(rpid<0 || rpid>=econ->n_prov){ g_region_religion[r]=-1; return; }
  const ProvincePop *pp=&econ->prov[rpid].pop;
  long tally[RELIG_MAX]={0};
  for(int i=0;i<pp->n_groups;i++){
    int f=pp->groups[i].faith;
    if(f>=0 && f<RELIG_MAX && f<g_religion_count) tally[f]+=pp->groups[i].count;
  }
  long best=0; int bestf=-1;
  for(int f=0; f<g_religion_count && f<RELIG_MAX; f++) if(tally[f]>best){ best=tally[f]; bestf=f; }
  g_region_religion[r]=bestf;
}
void religion_refresh_all(const WorldEconomy *econ){
  if(!econ) return;
  cr_ensure();
  for(int r=0;r<econ->n_regions && r<RELIG_MAX_REGION;r++) religion_refresh_region(econ, r);
}

/* FRACTURE (P8) — au schisme INTERNE : les régions du pays distantes (culture vs centre)
 * ET peu légitimes (L bas) basculent vers l'enfant ; le noyau (région-centre) garde le
 * parent. D∞ inline sur les axes PopCulture (valeurs/subsistance/parenté/religion). */
#define SCHISM_FLIP_D 5.0f   /* D∞ vs culture du centre (calibrable) */
#define SCHISM_FLIP_L 4.0f   /* L locale sous laquelle le flip devient probable */
/* la province r DÉRIVE-t-elle de la foi du centre ? — culturellement DISTANTE du
 * centre orthodoxe (D∞ > SCHISM_FLIP_D sur un axe PopCulture) ET peu légitime
 * (L < SCHISM_FLIP_L). MÊME porte pour l'ÉLIGIBILITÉ (lecture) et la FRACTURE
 * (mutation) : « la foi ne colle plus à la culture d'une marche » (Rome catholique
 * → Germanie protestante ; → Grèce orthodoxe — le schisme SUIT la culture). */
static bool region_faith_drifts(const WorldEconomy *econ, const WorldLegitimacy *wl,
                                int r, const PopCulture *fc){
  const PopCulture *rc=&econ->region[r].culture;
  float dv=rc->valeurs-fc->valeurs;          if(dv<0)dv=-dv;
  float ds=rc->subsistance-fc->subsistance;  if(ds<0)ds=-ds;
  float dp=rc->parente-fc->parente;          if(dp<0)dp=-dp;
  float dr=rc->religion-fc->religion;         if(dr<0)dr=-dr;
  float dinf=dv; if(ds>dinf)dinf=ds; if(dp>dinf)dinf=dp; if(dr>dinf)dinf=dr;
  float L=(wl && r<SCPS_MAX_REG)?wl->L[r]:5.f;
  return (dinf>SCHISM_FLIP_D && L<SCHISM_FLIP_L);
}
int religion_fracture(const World *w, WorldEconomy *econ,
                      const WorldLegitimacy *wl, int cid, int child_rid){
  if(!w||!econ||child_rid<0||child_rid>=g_religion_count) return 0;
  cr_ensure();
  int parent_rid=religion_of_country(cid);
  int centre=(parent_rid>=0&&parent_rid<g_religion_count)?g_religions[parent_rid].centre_cell:-1;
  int centre_rg=(centre>=0&&centre<SCPS_N)?w->cell[centre].region:-1;
  if(centre_rg<0||centre_rg>=econ->n_regions) return 0;
  const PopCulture *fc=&econ->region[centre_rg].culture;   /* culture « orthodoxe » du centre */
  int flipped=0;
  for(int r=0;r<w->n_regions && r<RELIG_MAX_REGION && r<econ->n_regions;r++){
    if(w->region[r].country!=cid || r==centre_rg) continue;   /* le centre garde le parent */
    if(religion_region_resisted(r)) continue;   /* un Gourou y bloque la bascule (P6) */
    if(region_faith_drifts(econ, wl, r, fc)){ region_set_native_faith(econ, r, child_rid);   /* la SOUCHE schisme */
                                              g_region_religion[r]=child_rid; flipped++; }
  }
  return flipped;
}

/* ── P6 : LETTRÉ (scholar) ─────────────────────────────────────────────── */
#define SCHOLAR_DURATION 1825   /* ~5 ans d'action (jours) */
typedef struct { int active; int role; int region; int timer; } ReligScholarSt;
static ReligScholarSt g_scholar[RELIG_MAX_COUNTRY];

int scholar_role_from_credo(int credo){
  switch(credo){
    case CREDO_PLURALISTE:   return SCHOLAR_RESIST;     /* Gourou : protège */
    case CREDO_EVANGELISTE:  return SCHOLAR_CONVERT;    /* Missionnaire : répand */
    case CREDO_PURIFICATEUR: return SCHOLAR_STABILIZE;  /* Moine : calme */
    default: return -1;
  }
}
int religion_scholar_recruit(int cid,int region){
  cr_ensure();
  if(cid<0||cid>=RELIG_MAX_COUNTRY) return -1;
  int rid=religion_of_country(cid); if(rid<0||rid>=g_religion_count) return -1;
  int role=scholar_role_from_credo(g_religions[rid].credo); if(role<0) return -1;
  g_scholar[cid].active=1; g_scholar[cid].role=role;
  g_scholar[cid].region=region; g_scholar[cid].timer=SCHOLAR_DURATION;
  return role;
}
int religion_scholar_active(int cid){ cr_ensure(); return (cid>=0&&cid<RELIG_MAX_COUNTRY)?g_scholar[cid].active:0; }
int religion_scholar_role(int cid){ cr_ensure(); return (cid>=0&&cid<RELIG_MAX_COUNTRY&&g_scholar[cid].active)?g_scholar[cid].role:-1; }
int religion_scholar_region(int cid){ cr_ensure(); return (cid>=0&&cid<RELIG_MAX_COUNTRY&&g_scholar[cid].active)?g_scholar[cid].region:-1; }
int religion_region_stabilized(int rg){
  cr_ensure();
  for(int c=0;c<RELIG_MAX_COUNTRY;c++)
    if(g_scholar[c].active && g_scholar[c].role==SCHOLAR_STABILIZE && g_scholar[c].region==rg) return 1;
  return 0;
}
int religion_region_resisted(int rg){
  cr_ensure();
  for(int c=0;c<RELIG_MAX_COUNTRY;c++)
    if(g_scholar[c].active && g_scholar[c].role==SCHOLAR_RESIST && g_scholar[c].region==rg) return 1;
  return 0;
}
/* ── PLAFOND mondial : ⌈n_empires/3⌉ religions FONDÉES (racines) ────────── */
int religion_root_count(void){
  int n=0; for(int i=0;i<g_religion_count;i++) if(g_religions[i].parent<0) n++; return n;
}
int religion_cap(int n_empires){ return (n_empires<=0)?1:((n_empires+2)/3); }   /* ceil(n/3) */
/* FONDER une RACINE : le plafond ⌈N/3⌉ borne les foi FONDATRICES (racines, parent<0). Les schismes
 * (sectes) ne comptent PAS ici — ils ont leur propre plafond PAR RACINE (religion_can_schism). */
int religion_can_found(int n_empires){ return religion_root_count() < religion_cap(n_empires); }
/* la RACINE-ANCÊTRE d'une foi (remonte la chaîne parent ; borne anti-cycle = RELIG_MAX). */
int religion_root_of(int rid){
  int guard=0;
  while(rid>=0 && rid<g_religion_count && g_religions[rid].parent>=0 && guard++<RELIG_MAX)
    rid=g_religions[rid].parent;
  return rid;
}
/* SCHISMER : au plus RELIG_SCHISM_MAX schismes par RACINE fondatrice (toute sa lignée comptée). */
int religion_can_schism(int parent_rid){
  if(parent_rid<0 || parent_rid>=g_religion_count) return 0;
  if(g_religion_count>=RELIG_MAX) return 0;                 /* borne DURE du registre */
  int root=religion_root_of(parent_rid), n=0;
  for(int i=0;i<g_religion_count;i++)
    if(g_religions[i].parent>=0 && religion_root_of(i)==root) n++;   /* descendants de CETTE racine */
  return n < RELIG_SCHISM_MAX;
}
/* le plafond est ANCRÉ au compte d'empires de GENÈSE (stable) — pas au compte courant, qui
 * gonfle avec les sécessions : « 6 empires ⇒ 2 religions », pas 8 quand le monde se fragmente. */
void religion_set_empire_ref(int n){ g_relig_n_emp_ref = (n>0)?n:0; }
int  religion_empire_ref(void){ return g_relig_n_emp_ref; }

int religion_found_random(int cid, int centre_cell, uint32_t seed){
  uint32_t h = seed ? seed : 1u;
  for(int tries=0; tries<24; tries++){
    h ^= h>>13; h *= 0x5bd1e995u; h ^= h>>15;
    int p0=(int)(h%(uint32_t)RP_COUNT); h/=(uint32_t)RP_COUNT;
    h ^= h>>11; h *= 0x9e3779b1u;
    int p1=(int)(h%(uint32_t)RP_COUNT); h/=(uint32_t)RP_COUNT;
    h ^= h>>7;  h *= 0x85ebca6bu;
    int p2=(int)(h%(uint32_t)RP_COUNT); h/=(uint32_t)RP_COUNT;
    int nc=(int)(h%(uint32_t)CREDO_COUNT);
    if(!religion_picks_valid(p0,p1,p2)) continue;
    int trad[3]={p0,p1,p2};
    int rid=religion_spawn(nc, trad, centre_cell, cid, NULL);
    if(rid>=0){ religion_set_country(cid, rid); return rid; }
    break;
  }
  return -1;
}
int religion_adopt_existing(int cid, uint32_t seed){
  int roots[RELIG_MAX], nr=0;
  for(int i=0;i<g_religion_count && nr<RELIG_MAX;i++) if(g_religions[i].parent<0) roots[nr++]=i;
  if(nr<=0) return -1;
  int pick=roots[seed % (uint32_t)nr];
  religion_set_country(cid, pick);
  return pick;
}

void religion_scholar_tick(const World *w, WorldEconomy *econ){
  cr_ensure();
  for(int c=0;c<RELIG_MAX_COUNTRY;c++){
    if(!g_scholar[c].active) continue;
    int rg=g_scholar[c].region;
    if(g_scholar[c].role==SCHOLAR_CONVERT && rg>=0 && rg<RELIG_MAX_REGION && (!w||rg<w->n_regions)
       && !religion_region_resisted(rg)){
      int rid=religion_of_country(c);
      if(rid>=0){ region_set_native_faith(econ, rg, rid); g_region_religion[rg]=rid; }   /* Missionnaire répand la foi d'État sur la souche */
    }
    if(--g_scholar[c].timer<=0) g_scholar[c].active=0;
  }
}
/* recalcule le cache d'un pays depuis sa religion (P4). */
static void cr_recompute(int cid){
  if(cid<0||cid>=RELIG_MAX_COUNTRY) return;
  int rid=g_country_religion[cid];
  if(rid>=0&&rid<g_religion_count) religion_apply(&g_religions[rid], &g_country_relig_acc[cid]);
  else memset(&g_country_relig_acc[cid], 0, sizeof g_country_relig_acc[cid]);
}
int  religion_of_country(int cid){ cr_ensure(); return (cid>=0&&cid<RELIG_MAX_COUNTRY)?g_country_religion[cid]:-1; }
void religion_set_country(int cid,int rid){
  cr_ensure();
  if(cid>=0&&cid<RELIG_MAX_COUNTRY){ g_country_religion[cid]=rid; cr_recompute(cid); }
}
const ReligAccum* religion_country_acc(int cid){
  cr_ensure();
  return (cid>=0&&cid<RELIG_MAX_COUNTRY)?&g_country_relig_acc[cid]:&g_zero_acc;
}
void religion_reset(void){
  g_religion_count=0; g_relig_n_emp_ref=0; cr_ensure();
  for(int i=0;i<RELIG_MAX_COUNTRY;i++) g_country_religion[i]=-1;
  for(int i=0;i<RELIG_MAX_REGION;i++)  g_region_religion[i]=-1;
  memset(g_country_relig_acc, 0, sizeof g_country_relig_acc);
  memset(g_scholar, 0, sizeof g_scholar);
}

/* éligibilité au schisme :
 *  · RUPTURE — la cellule-centre de la foi du pays n'est PAS sous son contrôle
 *    (conquise/étrangère) : l'empire ROMPT et adopte une foi autonome (il s'unit) ;
 *  · DÉRIVE (la Réforme) — le pays TIENT son centre, mais une marche SUR la foi
 *    d'État est culturellement DISTANTE du centre orthodoxe & peu légitime : la foi
 *    ne colle plus à sa culture, un schisme adapté À SA CULTURE va y prendre (Rome
 *    → Germanie protestante). Le centre garde le parent → minorité de MÊME RACINE
 *    (hérésie). Lecture pure (même porte `region_faith_drifts` que la fracture). */
ReligSchismMode religion_schism_eligible(const World *w, const WorldEconomy *econ,
                                        const WorldLegitimacy *wl, int cid){
  if(!w) return RSE_NONE;
  int rid=religion_of_country(cid);
  if(rid<0||rid>=g_religion_count) return RSE_NONE;
  int centre=g_religions[rid].centre_cell;
  if(centre<0||centre>=SCPS_N) return RSE_NONE;
  if(w->cell[centre].country != cid) return RSE_RUPTURE;
  if(econ){
    int centre_rg=w->cell[centre].region;
    if(centre_rg>=0 && centre_rg<econ->n_regions){
      const PopCulture *fc=&econ->region[centre_rg].culture;
      for(int r=0;r<w->n_regions && r<RELIG_MAX_REGION && r<econ->n_regions;r++){
        if(w->region[r].country!=cid || r==centre_rg) continue;
        if(religion_of_region(r)!=rid) continue;    /* déjà schismée/étrangère : autre grief */
        if(religion_region_resisted(r)) continue;   /* un Gourou y bloque la dérive */
        if(region_faith_drifts(econ, wl, r, fc)) return RSE_DERIVE;
      }
    }
  }
  return RSE_NONE;
}

void religion_save(FILE *f){
  cr_ensure();
  uint32_t n=(uint32_t)g_religion_count; fwrite(&n,4,1,f);
  for(int i=0;i<g_religion_count;i++){ const Religion *r=&g_religions[i];
    int32_t v[7]={r->id,r->parent,r->centre_cell,r->credo,
                  r->traditions[0],r->traditions[1],r->traditions[2]};
    fwrite(v,4,7,f); fwrite(r->color,1,3,f);
    int32_t fc=r->founder_cid; fwrite(&fc,4,1,f); }
  uint32_t m=(uint32_t)RELIG_MAX_COUNTRY; fwrite(&m,4,1,f);
  for(int i=0;i<RELIG_MAX_COUNTRY;i++){ int32_t v=g_country_religion[i]; fwrite(&v,4,1,f); }
  uint32_t q=(uint32_t)RELIG_MAX_REGION; fwrite(&q,4,1,f);    /* P8 : religion par région */
  for(int i=0;i<RELIG_MAX_REGION;i++){ int32_t v=g_region_religion[i]; fwrite(&v,4,1,f); }
  uint32_t sc=(uint32_t)RELIG_MAX_COUNTRY; fwrite(&sc,4,1,f); /* P6 : lettrés (scholars) */
  for(int i=0;i<RELIG_MAX_COUNTRY;i++){
    int32_t v[4]={g_scholar[i].active,g_scholar[i].role,g_scholar[i].region,g_scholar[i].timer};
    fwrite(v,4,4,f);
  }
}
int religion_load(FILE *f){
  cr_ensure();
  uint32_t n=0; if(fread(&n,4,1,f)!=1 || n>(uint32_t)RELIG_MAX) return 1;
  g_religion_count=(int)n;
  for(int i=0;i<(int)n;i++){ Religion *r=&g_religions[i];
    int32_t v[7]; if(fread(v,4,7,f)!=7) return 1;
    r->id=v[0]; r->parent=v[1]; r->centre_cell=v[2]; r->credo=v[3];
    r->traditions[0]=v[4]; r->traditions[1]=v[5]; r->traditions[2]=v[6];
    if(fread(r->color,1,3,f)!=3) return 1;
    int32_t fc=0; if(fread(&fc,4,1,f)!=1) return 1; r->founder_cid=fc;
    if(r->credo<0||r->credo>=CREDO_COUNT) r->credo=0;                 /* borne désérialisée */
    for(int k=0;k<3;k++) if(r->traditions[k]<0||r->traditions[k]>=RP_COUNT) r->traditions[k]=0; }
  uint32_t m=0; if(fread(&m,4,1,f)!=1 || m!=(uint32_t)RELIG_MAX_COUNTRY) return 1;
  for(int i=0;i<RELIG_MAX_COUNTRY;i++){ int32_t v=0; if(fread(&v,4,1,f)!=1) return 1;
    g_country_religion[i]=(v>=-1 && v<g_religion_count)?(int)v:-1; }  /* borne : religion valide ou aucune */
  uint32_t q=0; if(fread(&q,4,1,f)!=1 || q!=(uint32_t)RELIG_MAX_REGION) return 1;   /* P8 : région */
  for(int i=0;i<RELIG_MAX_REGION;i++){ int32_t v=0; if(fread(&v,4,1,f)!=1) return 1;
    g_region_religion[i]=(v>=-1 && v<g_religion_count)?(int)v:-1; }
  uint32_t sc=0; if(fread(&sc,4,1,f)!=1 || sc!=(uint32_t)RELIG_MAX_COUNTRY) return 1;   /* P6 : scholars */
  for(int i=0;i<RELIG_MAX_COUNTRY;i++){ int32_t v[4]; if(fread(v,4,4,f)!=4) return 1;
    g_scholar[i].active=(v[0]!=0);
    g_scholar[i].role=(v[1]>=0&&v[1]<SCHOLAR_ROLE_COUNT)?v[1]:0;            /* borné */
    g_scholar[i].region=(v[2]>=-1&&v[2]<RELIG_MAX_REGION)?v[2]:-1;
    g_scholar[i].timer=(v[3]>=0&&v[3]<=SCHOLAR_DURATION)?v[3]:0;
    if(g_scholar[i].timer<=0) g_scholar[i].active=0; }
  for(int i=0;i<RELIG_MAX_COUNTRY;i++) cr_recompute(i);              /* P4 : reconstruit le cache d'acc */
  return 0;
}

int religion_picks_valid(int p0,int p1,int p2){
  int v[3]={p0,p1,p2};
  for(int i=0;i<3;i++){ if(v[i]<0||v[i]>=RP_COUNT) return 0;
    for(int j=i+1;j<3;j++) if(RELIG_POLES[v[i]].axis==RELIG_POLES[v[j]].axis) return 0; }
  return 1;
}

int religion_spawn(int credo,const int trad[3],int centre_cell,int founder_cid,const uint8_t color[3]){
  if(g_religion_count>=RELIG_MAX) return -1;
  if(credo<0||credo>=CREDO_COUNT) return -1;
  if(!religion_picks_valid(trad[0],trad[1],trad[2])) return -1;
  Religion* r=&g_religions[g_religion_count];
  r->id=g_religion_count; r->parent=-1; r->centre_cell=centre_cell;
  r->credo=credo; r->founder_cid=founder_cid;
  for(int i=0;i<3;i++) r->traditions[i]=trad[i];
  if(color){ r->color[0]=color[0]; r->color[1]=color[1]; r->color[2]=color[2]; }
  else { r->color[0]=128; r->color[1]=128; r->color[2]=128; }
  return g_religion_count++;
}

int religion_schism(int parent_id,int slot_a,int pole_a,int slot_b,int pole_b,
                    int new_credo,int declare_cell,int founder_cid,
                    int randomize_color,uint32_t seed){
  if(parent_id<0||parent_id>=g_religion_count) return -1;
  if(slot_a<0||slot_a>2||slot_b<0||slot_b>2||slot_a==slot_b) return -1;
  if(new_credo<0||new_credo>=CREDO_COUNT) return -1;
  if(g_religion_count>=RELIG_MAX) return -1;
  int trad[3]; for(int i=0;i<3;i++) trad[i]=g_religions[parent_id].traditions[i];
  trad[slot_a]=pole_a; trad[slot_b]=pole_b;
  if(!religion_picks_valid(trad[0],trad[1],trad[2])) return -1;
  Religion* c=&g_religions[g_religion_count];
  c->id=g_religion_count; c->parent=parent_id; c->centre_cell=declare_cell;
  c->credo=new_credo; c->founder_cid=founder_cid;
  for(int i=0;i<3;i++) c->traditions[i]=trad[i];
  religion_color_variant(g_religions[parent_id].color,c->color,randomize_color,seed);
  return g_religion_count++;
}

void religion_apply(const Religion* r,ReligAccum* out){
  memset(out,0,sizeof *out);
  for(int i=0;i<3;i++){ const ReligPoleDef* p=&RELIG_POLES[r->traditions[i]];
    out->ch[p->d[0].ch]+=p->d[0].mag; out->ch[p->d[1].ch]+=p->d[1].mag; }
  const ReligDelta* cd=RELIG_CREDO[(r->credo>=0&&r->credo<CREDO_COUNT)?r->credo:0];
  for(int j=0; cd[j].ch!=RC_COUNT; j++) out->ch[cd[j].ch]+=cd[j].mag;
}

static uint8_t clamp8(int v){ return v<0?0:(v>255?255:(uint8_t)v); }

void religion_color_variant(const uint8_t parent[3],uint8_t out[3],int randomize,uint32_t seed){
  int ch = randomize ? (int)(seed%3u) : 1;          /* défaut: canal vert (bleu->cyan) */
  int dir= (randomize && (seed&8u)) ? -1 : 1;
  for(int i=0;i<3;i++) out[i]=parent[i];
  out[ch]=clamp8((int)parent[ch]+dir*COLOR_SHIFT);
  if(out[0]==parent[0]&&out[1]==parent[1]&&out[2]==parent[2])
    out[ch]=clamp8((int)parent[ch]-dir*COLOR_SHIFT); /* garantir perceptible */
}

int religion_color_near(const uint8_t parent[3],const uint8_t chosen[3]){
  int same=1;
  for(int i=0;i<3;i++){
    int d=(int)chosen[i]-(int)parent[i];
    if(d<0) d=-d;
    if(d>COLOR_NEAR_MAX) return 0;
    if(chosen[i]!=parent[i]) same=0;
  }
  return !same;
}

/* ===================================================================== */
/* i18n — mots résolus (littéraux, comme credo_name/heritage_name)          */
/* ===================================================================== */
const char *relig_axis_name(ReligAxis a){
  static const char *N[RA_AXIS_COUNT]={
    "Sang","Feu","Seuil","Serment","Veille","Canon","Don","Glaive"};
  return (a>=0&&a<RA_AXIS_COUNT)?N[a]:"?";
}
const char *relig_pole_name(ReligPole p){
  static const char *N[RP_COUNT]={
    "F\xc3\xa9""condit\xc3\xa9","Offrande","Transe","Silence","Accueil","Mur",
    "Couronne","Assembl\xc3\xa9""e","Anc\xc3\xaatres","Cendre","Gnose","Orthodoxie",
    "Frugalit\xc3\xa9","Faste","Courage","Tr\xc3\xaave"};
  return (p>=0&&p<RP_COUNT)?N[p]:"?";
}
const char *relig_pole_tip(ReligPole p){
  static const char *T[RP_COUNT]={
    "Croissance accrue, flux plus lourd",          /* Fécondité  */
    "Stabilit\xc3\xa9 par le sang vers\xc3\xa9",    /* Offrande   */
    "Ferveur chaude, ordre dilu\xc3\xa9",           /* Transe     */
    "Discipline, peu d'effervescence",             /* Silence    */
    "Membrane ouverte, peu d'expulsion",           /* Accueil    */
    "Membrane ferm\xc3\xa9""e, contact appauvri",   /* Mur        */
    "Pouvoir centralis\xc3\xa9",                     /* Couronne   */
    "D\xc3\xa9""cision distribu\xc3\xa9""e",         /* Assemblée  */
    "Continuit\xc3\xa9, assimilation lente",        /* Ancêtres   */
    "Table rase, assimilation rapide",             /* Cendre     */
    "Savoir pouss\xc3\xa9, app\xc3\xa9tit faustien",/* Gnose      */
    "Doctrine fig\xc3\xa9""e, d\xc3\xa9rive verrouill\xc3\xa9""e", /* Orthodoxie */
    "Coh\xc3\xa9sion sobre, revenu moindre",        /* Frugalité  */
    "Prestige, grief d'in\xc3\xa9galit\xc3\xa9",     /* Faste      */
    "Ardeur guerri\xc3\xa8re, contact m\xc3\xa9""fiant", /* Courage */
    "Concorde marchande, ardeur moindre"};         /* Trêve      */
  return (p>=0&&p<RP_COUNT)?T[p]:"";
}
const char *scholar_role_name(int role){
  static const char *N[SCHOLAR_ROLE_COUNT]={"Missionnaire","Gourou","Moine"};
  return (role>=0&&role<SCHOLAR_ROLE_COUNT)?N[role]:"?";
}
const char *scholar_role_ability(int role){
  static const char *N[SCHOLAR_ROLE_COUNT]={"Conversion","R\xc3\xa9sistance","Stabilisation"};
  return (role>=0&&role<SCHOLAR_ROLE_COUNT)?N[role]:"?";
}

void religion_selftest(void){
  for(int p=0;p<RP_COUNT;p++) assert(RELIG_POLES[p].axis==(ReligAxis)(p>>1)); /* ordre */
  assert(!religion_picks_valid(RP_FECONDITE,RP_OFFRANDE,RP_ACCUEIL)); /* même axe SANG */
  assert( religion_picks_valid(RP_FECONDITE,RP_ACCUEIL,RP_GNOSE));
  int trad[3]={RP_FECONDITE,RP_MUR,RP_GNOSE};
  uint8_t col[3]={20,40,200};
  /* crédo 1 (évangéliste) : sans terme RC_H, donc le +H du pôle MUR reste lisible
     (le crédo 0 pluraliste a RC_H −1.0 qui ANNULERAIT le +1.0 de MUR → net 0). */
  int root=religion_spawn(1,trad,123,1,col); assert(root>=0);
  ReligAccum a; religion_apply(&g_religions[root],&a);
  assert(a.ch[RC_POPGROWTH]>0 && a.ch[RC_H]>0 && a.ch[RC_RESEARCH]>0 && a.ch[RC_ENTROPY]>0);
  int kid=religion_schism(root,1,RP_ACCUEIL,2,RP_ORTHODOXIE,2,124,2,1,0xC0FFEE);
  assert(kid>=0 && g_religions[kid].parent==root);
  assert(g_religions[kid].traditions[0]==RP_FECONDITE); /* slot 0 conservé */
  assert(religion_color_near(col,g_religions[kid].color)); /* variante proche */
  /* i18n : chaque axe/pôle a un nom + tip non vides (membrane résolue). */
  for(int x=0;x<RA_AXIS_COUNT;x++) assert(relig_axis_name((ReligAxis)x)[0]);
  for(int p=0;p<RP_COUNT;p++){ assert(relig_pole_name((ReligPole)p)[0]); assert(relig_pole_tip((ReligPole)p)[0]); }
  for(int s=0;s<SCHOLAR_ROLE_COUNT;s++){ assert(scholar_role_name(s)[0]); assert(scholar_role_ability(s)[0]); }
  g_religion_count=0; /* reset banc */
}
