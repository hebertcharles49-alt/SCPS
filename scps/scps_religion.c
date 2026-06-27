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
  g_religion_count=0; /* reset banc */
}
