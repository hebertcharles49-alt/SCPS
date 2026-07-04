/*
 * scps_heritage.c — roster de héritages & traits (voir scps_heritage.h)
 *
 * Data-driven et autonome. Les leviers (chiffres) sont calibrables ; la
 * structure (12 traits/pool, atouts/défauts appariés, équilibre à 0) est fixe.
 */
#include "scps_heritage.h"
#include <stddef.h>

/* ===================================================================== */
/* SPHÈRES                                                                */
/* ===================================================================== */
/* Continuum (pas un mur) : 5 = demi-ésotériques courants, 7 = demi-claniques rares. */
static const float SPHERE_DIST[SPHERE_COUNT][SPHERE_COUNT] = {
    /*            Anciens Hommes Étrangers */
    /* Anciens */ { 0.f,   5.f,   7.f },
    /* Hommes  */ { 5.f,   0.f,   7.f },
    /* Étrangers*/ { 7.f,   7.f,   0.f },
};
float sphere_distance(Sphere a, Sphere b) {
    if (a<0||a>=SPHERE_COUNT||b<0||b>=SPHERE_COUNT) return 0.f;
    return SPHERE_DIST[a][b];
}
const char *sphere_name(Sphere s) {
    static const char *N[SPHERE_COUNT] = { "Anciens", "Hommes", "Étrangers" };
    return (s>=0&&s<SPHERE_COUNT)?N[s]:"?";
}
const char *category_name(TraitCategory c) {
    static const char *N[CAT_COUNT] = { "Physique", "Social", "Intellectuel" };
    return (c>=0&&c<CAT_COUNT)?N[c]:"?";
}

/* ===================================================================== */
/* TABLE DES 36 TRAITS                                                    */
/* ===================================================================== */
static const TraitDef TRAITS[TRAIT_COUNT] = {
/* ---- PHYSIQUE — atouts ---- */
[T_ROBUSTE]      = { "Robuste",      CAT_PHYSIQUE, +2, T_FRELE,
    { .coercition=+1.0f, .rendement=+0.20f },
    "Forte constitution : meilleur au combat et au travail de force." },
[T_REGENERANT]   = { "Régénérant",   CAT_PHYSIQUE, +2, T_CONVALESCENT,
    { .demographie=+0.20f, .coercition=+1.0f },
    "Guérit vite (sang de troll) : récupère ses pertes, tient au front." },
[T_PROLIFIQUE]   = { "Prolifique",   CAT_PHYSIQUE, +1, T_LENT_CROITRE,
    { .demographie=+0.15f },
    "Reproduction rapide : croissance de population élevée." },
[T_LONGEVIF]     = { "Longévif",     CAT_PHYSIQUE, +1, T_EPHEMERE,
    { .derive=-0.20f },
    "Vie longue : dérive lente, dirigeants durables, héritage stable." },
[T_ENDURANT]     = { "Endurant",     CAT_PHYSIQUE, +1, T_FRAGILE_CLIMAT,
    { .demographie=+0.10f },
    "Résiste au climat et à la disette : tient hors de sa niche." },
[T_SOBRE]        = { "Sobre",        CAT_PHYSIQUE, +1, T_VORACE,
    { .rendement=+0.10f },
    "Consomme peu : nourriture nette supérieure, entretien faible." },
/* ---- PHYSIQUE — défauts ---- */
[T_FRELE]        = { "Frêle",        CAT_PHYSIQUE, -1, T_ROBUSTE,
    { .coercition=-1.0f, .rendement=-0.20f },
    "Faible constitution : médiocre au combat et au travail." },
[T_CONVALESCENT] = { "Convalescent", CAT_PHYSIQUE, -1, T_REGENERANT,
    { .demographie=-0.20f, .coercition=-1.0f },
    "Guérison lente : encaisse mal les pertes." },
[T_LENT_CROITRE] = { "Lent à croître",CAT_PHYSIQUE, -1, T_PROLIFIQUE,
    { .demographie=-0.15f },
    "Reproduction lente : croissance de population faible." },
[T_EPHEMERE]     = { "Éphémère",     CAT_PHYSIQUE, -1, T_LONGEVIF,
    { .derive=+0.20f },
    "Vie courte : dérive rapide, dirigeants instables." },
[T_FRAGILE_CLIMAT]={ "Fragile au climat",CAT_PHYSIQUE,-1, T_ENDURANT,
    { .demographie=-0.10f },
    "Souffre hors de sa niche : croissance pénalisée en biome dur." },
[T_VORACE]       = { "Vorace",       CAT_PHYSIQUE, -1, T_SOBRE,
    { .rendement=-0.10f },
    "Consomme beaucoup : entretien lourd, pression alimentaire." },
/* ---- SOCIAL — atouts ---- */
[T_BELLIQUEUX]   = { "Belliqueux",   CAT_SOCIAL, +2, T_DEBONNAIRE,
    { .coercition=+1.0f },
    "Aime la guerre : bonus militaire, casus belli faciles." },
[T_SOUDE]        = { "Soudé",        CAT_SOCIAL, +2, T_FACTIEUX,
    { .fracture=-1.0f },
    "Solidarité large : tient la diversité, fracture amortie." },
[T_CHARISMATIQUE]= { "Charismatique",CAT_SOCIAL, +1, T_REBUTANT,
    { .influence=+0.5f },
    "Présence : meilleure diplomatie et rayonnement." },
[T_OUVERT]       = { "Ouvert",       CAT_SOCIAL, +1, T_INSULAIRE,
    { .permeabilite=+0.5f },
    "Hospitalier : perméabilité élevée, porte d'assimilation plus ouverte." },
[T_DISCIPLINE]   = { "Discipliné",   CAT_SOCIAL, +1, T_FRONDEUR,
    { .capacite=+0.5f },
    "Ordre intériorisé : soutient la capacité, agitation réduite." },
[T_PROSELYTE]    = { "Prosélyte",    CAT_SOCIAL, +1, T_RESERVE,
    { .influence=+0.25f, .derive=+0.20f },
    "Rayonne sa culture : assimile les autres plus vite." },
/* ---- SOCIAL — défauts ---- */
[T_DEBONNAIRE]   = { "Débonnaire",   CAT_SOCIAL, -1, T_BELLIQUEUX,
    { .coercition=-1.0f },
    "Rétif à la guerre : malus militaire, casus belli rares." },
[T_FACTIEUX]     = { "Factieux",     CAT_SOCIAL, -1, T_SOUDE,
    { .fracture=+1.0f },
    "Clans rivaux : fracture aggravée, diversité dure à tenir." },
[T_REBUTANT]     = { "Rebutant",     CAT_SOCIAL, -1, T_CHARISMATIQUE,
    { .influence=-0.5f },
    "Déplaisant : diplomatie et rayonnement pénalisés." },
[T_INSULAIRE]    = { "Insulaire",    CAT_SOCIAL, -1, T_OUVERT,
    { .permeabilite=-0.5f },
    "Méfiant : perméabilité basse, friction élevée, assimile mal." },
[T_FRONDEUR]     = { "Frondeur",     CAT_SOCIAL, -1, T_DISCIPLINE,
    { .capacite=-0.5f },
    "Indiscipliné : agitation, capacité plus difficile à tenir." },
[T_RESERVE]      = { "Réservé",      CAT_SOCIAL, -1, T_PROSELYTE,
    { .influence=-0.25f, .derive=-0.20f },
    "Sa culture ne s'exporte pas : assimile lentement." },
/* ---- INTELLECTUEL — atouts ---- */
[T_INVENTIF]     = { "Inventif",     CAT_INTELLECTUEL, +2, T_BORNE,
    { .rendement=+0.20f },
    "Innovation : recherche accélérée, rendement supérieur." },
[T_ARCANIQUE]    = { "Arcanique",    CAT_INTELLECTUEL, +2, T_SOURD_ARCANE,
    { .arcane=+1.0f },
    "Sensible à la magie : branche Magie facilitée — pente faustienne." },
[T_STUDIEUX]     = { "Studieux",     CAT_INTELLECTUEL, +1, T_INCULTE,
    { .rendement=+0.10f },
    "Soif de savoir : recherche améliorée." },
[T_BATISSEUR]    = { "Bâtisseur",    CAT_INTELLECTUEL, +1, T_BROUILLON,
    { .capacite=+0.5f },
    "Méthodique : capacité supérieure, tient plus de diversité." },
[T_ADAPTABLE]    = { "Adaptable",    CAT_INTELLECTUEL, +1, T_TRADITIONALISTE,
    { .derive=+0.20f },
    "Curieux : dérive rapide, prend la forme du biome, assimile vite." },
[T_INDUSTRIEUX]  = { "Industrieux",  CAT_INTELLECTUEL, +1, T_INDOLENT,
    { .rendement=+0.10f },
    "Appliqué : rendement économique régulier." },
/* ---- INTELLECTUEL — défauts ---- */
[T_BORNE]        = { "Borné",        CAT_INTELLECTUEL, -1, T_INVENTIF,
    { .rendement=-0.20f },
    "Esprit fermé : recherche et rendement pénalisés." },
[T_SOURD_ARCANE] = { "Sourd à l'arcane",CAT_INTELLECTUEL,-1, T_ARCANIQUE,
    { .arcane=-1.0f },
    "Imperméable à la magie : branche Magie très coûteuse." },
[T_INCULTE]      = { "Inculte",      CAT_INTELLECTUEL, -1, T_STUDIEUX,
    { .rendement=-0.10f },
    "Indifférent au savoir : recherche affaiblie." },
[T_BROUILLON]    = { "Brouillon",    CAT_INTELLECTUEL, -1, T_BATISSEUR,
    { .capacite=-0.5f },
    "Désordonné : capacité plus basse." },
[T_TRADITIONALISTE]={ "Traditionaliste",CAT_INTELLECTUEL,-1, T_ADAPTABLE,
    { .derive=-0.20f },
    "Rigide : dérive lente, change mal — mais héritage très stable." },
[T_INDOLENT]     = { "Indolent",     CAT_INTELLECTUEL, -1, T_INDUSTRIEUX,
    { .rendement=-0.10f },
    "Nonchalant : rendement économique faible." },
};

const TraitDef *trait_def(TraitId t){ return (t>=0&&t<TRAIT_COUNT)?&TRAITS[t]:NULL; }
const char     *trait_name(TraitId t){ return (t>=0&&t<TRAIT_COUNT)?TRAITS[t].name:"?"; }
const char     *trait_hover(TraitId t){ return (t>=0&&t<TRAIT_COUNT)?TRAITS[t].hover:""; }

/* ===================================================================== */
/* ROSTER — builds par défaut (tous équilibrés à 0)                       */
/* ===================================================================== */
static const Sphere RACE_SPHERE[HERITAGE_COUNT] = {
    [HERITAGE_ESOTERIQUE]=SPHERE_ANCIENS, [HERITAGE_METALLURGISTE]=SPHERE_ANCIENS, [HERITAGE_MECANISTE]=SPHERE_ANCIENS,
    [HERITAGE_ADAPTATIF]=SPHERE_HOMMES, [HERITAGE_AGRAIRE]=SPHERE_HOMMES, [HERITAGE_CLANIQUE]=SPHERE_ETRANGERS,
};
Sphere heritage_sphere(Heritage r){
    return (r>=0&&r<HERITAGE_COUNT)?RACE_SPHERE[r]:SPHERE_HOMMES;
}
const char *heritage_name(Heritage r){
    /* GR1 — l'HÉRITAGE par FONCTION (mêmes valeurs/ordre que l'ex-heritage). */
    static const char *N[HERITAGE_COUNT]={"Ésotérique","Métallurgiste",
                                          "Mécaniste","Adaptatif","Agraire","Clanique"};
    return (r>=0&&r<HERITAGE_COUNT)?N[r]:"?";
}

/* Builds par défaut — une tradition par axe {Physique, Social, Intellectuel}, chacun
 * 1 majeur (+2) + 1 mineur (+1) + 1 défaut (−1). (« Plus de héritages » : ces presets ne sont
 * plus que des points de DÉPART de tradition — la culture personnalisable les remplace.) */
static const HeritageBuild ROSTER[HERITAGE_COUNT] = {
    [HERITAGE_ESOTERIQUE]    = {{ T_LONGEVIF,   T_DEBONNAIRE, T_ARCANIQUE }},      /* +1 / −1 / +2 */
    [HERITAGE_METALLURGISTE] = {{ T_ROBUSTE,    T_FACTIEUX,   T_BATISSEUR }},      /* +2 / −1 / +1 */
    [HERITAGE_MECANISTE]     = {{ T_FRELE,      T_CHARISMATIQUE, T_INVENTIF }},    /* −1 / +1 / +2 */
    [HERITAGE_ADAPTATIF]     = {{ T_PROLIFIQUE, T_FRONDEUR,   T_INVENTIF }},       /* +1 / −1 / +2 (ex-Adaptable : il fallait 1 majeur) */
    [HERITAGE_AGRAIRE]       = {{ T_SOBRE,      T_SOUDE,      T_TRADITIONALISTE }},/* +1 / +2 / −1 (ex-Ouvert : il fallait 1 majeur) */
    [HERITAGE_CLANIQUE]      = {{ T_ENDURANT,   T_BELLIQUEUX, T_BORNE }},          /* +1 / +2 / −1 */
};
HeritageBuild heritage_default_build(Heritage r){
    if (r>=0&&r<HERITAGE_COUNT) return ROSTER[r];
    HeritageBuild empty = {{ T_PROLIFIQUE, T_FRONDEUR, T_INVENTIF }};
    return empty;
}

/* ===================================================================== */
/* BUDGET, VALIDATION, LEVIERS                                            */
/* ===================================================================== */
/* RÈGLE DE TRADITIONS (ex-budget, supprimé) — un build = 3 TRADITIONS, une par catégorie (les
 * 3 AXES Physique/Social/Intellectuel — jamais deux fois le même axe), FORCÉ à EXACTEMENT
 * 1 atout MAJEUR (+2) + 1 atout MINEUR (+1) + 1 défaut (−1, plus de défaut majeur). Le joueur
 * choisit QUEL axe porte le revers et lequel est majeur. Antonymes interdits (opposables). */
bool build_is_valid(const HeritageBuild *b){
    int major=0, minor=0, def=0;
    for (int c=0;c<CAT_COUNT;c++){
        TraitId t=b->trait[c];
        if (t<0||t>=TRAIT_COUNT) return false;
        if (TRAITS[t].cat != (TraitCategory)c) return false;   /* une tradition par axe */
        int p=TRAITS[t].pts;
        if (p>=2) major++; else if (p==1) minor++; else if (p<0) def++;
    }
    if (major!=1 || minor!=1 || def!=1) return false;          /* FORCÉ : 1 majeur, 1 mineur, 1 défaut */
    for (int i=0;i<CAT_COUNT;i++)
        for (int j=i+1;j<CAT_COUNT;j++)
            if (TRAITS[b->trait[i]].antonym == b->trait[j]) return false;   /* antonymes (opposables) */
    return true;
}

HeritageLeviers build_leviers(const HeritageBuild *b){
    HeritageLeviers L = {0};
    for (int c=0;c<CAT_COUNT;c++){
        TraitId t=b->trait[c];
        if (t<0||t>=TRAIT_COUNT) continue;
        const HeritageLeviers *d=&TRAITS[t].lev;
        L.demographie  += d->demographie;  L.rendement += d->rendement;
        L.influence    += d->influence;    L.coercition   += d->coercition;
        L.capacite     += d->capacite;     L.permeabilite += d->permeabilite;
        L.arcane       += d->arcane;       L.derive       += d->derive;
        L.fracture     += d->fracture;
    }
    return L;
}

/* TRADITIONS ALÉATOIRES — un build VALIDE (1 majeur + 1 mineur + 1 défaut, une tradition par
 * axe) tiré DÉTERMINISTEMENT d'un seed (= l'identité de l'empire). INDÉPENDANT de l'héritage
 * (l'héritage ne touche QUE les noms) : c'est ce que l'IA reçoit (« traditions choisies au
 * hasard ») et le défaut tant que le joueur n'a pas composé la sienne. Antonymes impossibles
 * (ils partagent un axe, or on prend une tradition par axe). */
HeritageBuild culture_random_build(uint32_t seed){
    uint32_t h=(seed*2654435761u)^0x9E3779B9u; h^=h>>13; h*=0x85ebca6bu; h^=h>>16;
    static const int PERM[6][3]={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
    const int *role=PERM[h%6]; h/=6;          /* role[axe] : 0=majeur 1=mineur 2=défaut */
    HeritageBuild b={{0,0,0}};
    for (int c=0;c<CAT_COUNT;c++){
        TraitId pool[TRAIT_COUNT]; int np=0;
        for (int t=0;t<TRAIT_COUNT;t++){
            if (TRAITS[t].cat!=(TraitCategory)c) continue;
            int p=TRAITS[t].pts, want=role[c];
            if ((want==0&&p>=2)||(want==1&&p==1)||(want==2&&p<0)) pool[np++]=(TraitId)t;
        }
        if (np>0){ b.trait[c]=pool[h%(uint32_t)np]; h/=(uint32_t)np; }
    }
    return b;
}

/* ===================================================================== */
/* CRÉATEUR DE CULTURE — SLOTS par empire (voir .h)                        */
/* ===================================================================== */
/* Slot 0 = joueur ; 1..N-1 = empires IA. Le cid→slot est établi à la genèse
 * (culture_bind_cid). AUCUN slot posé ⇒ culture_build_for ≡ culture_random_build,
 * culture_*_heritage ≡ ADAPTATIF : le moteur (chronique, bancs, golden, déterminisme)
 * ne voit STRICTEMENT aucun changement. */
#define CULTURE_MAX_CID 512   /* ≥ SCPS_MAX_COUNTRY ; découplé de scps_types.h */
typedef struct {
    bool             active;
    Heritage heritage;
    int              ethos;      /* Ethos (scps_culture.h) — int pour éviter le cycle d'include */
    HeritageBuild     build;
} CultureSlot;
static CultureSlot g_slot[CULTURE_SLOTS];           /* défaut : tout {false,…} (BSS) */
static int  g_cid_slot[CULTURE_MAX_CID];            /* cid→slot, -1 = aucun (init au 1er reset) */
static bool g_cid_map_init = false;

static void cid_map_ensure(void){
    if (!g_cid_map_init){ for (int i=0;i<CULTURE_MAX_CID;i++) g_cid_slot[i]=-1; g_cid_map_init=true; }
}

void culture_slot_set(int slot, Heritage heritage, int ethos, HeritageBuild build){
    if (slot<0 || slot>=CULTURE_SLOTS) return;
    g_slot[slot].active   = true;
    g_slot[slot].heritage = (heritage>=0&&heritage<HERITAGE_COUNT) ? heritage : HERITAGE_ADAPTATIF;
    g_slot[slot].ethos    = ethos;
    g_slot[slot].build    = build;
}
void culture_slot_clear_all(void){
    for (int s=0;s<CULTURE_SLOTS;s++){ g_slot[s].active=false; g_slot[s].heritage=HERITAGE_ADAPTATIF; g_slot[s].ethos=2; }
    cid_map_ensure();
    for (int i=0;i<CULTURE_MAX_CID;i++) g_cid_slot[i]=-1;
}
bool             culture_slot_active(int slot){ return (slot>=0&&slot<CULTURE_SLOTS) && g_slot[slot].active; }
Heritage culture_slot_heritage(int slot){ return culture_slot_active(slot)?g_slot[slot].heritage:HERITAGE_ADAPTATIF; }
int              culture_slot_ethos(int slot){ return culture_slot_active(slot)?g_slot[slot].ethos:2; }

void culture_reset_cid_map(void){ cid_map_ensure(); for (int i=0;i<CULTURE_MAX_CID;i++) g_cid_slot[i]=-1; }
void culture_bind_cid(int cid, int slot){ cid_map_ensure(); if (cid>=0&&cid<CULTURE_MAX_CID) g_cid_slot[cid]=slot; }
int  culture_slot_of_cid(int cid){ cid_map_ensure(); return (cid>=0&&cid<CULTURE_MAX_CID)?g_cid_slot[cid]:-1; }

bool culture_any_active(void){ for (int s=0;s<CULTURE_SLOTS;s++) if (g_slot[s].active) return true; return false; }

HeritageBuild culture_build_for(uint32_t cid){
    int slot = culture_slot_of_cid((int)cid);
    if (slot>=0 && culture_slot_active(slot)) return g_slot[slot].build;
    return culture_random_build(cid);
}

/* ---- SAUVEGARDE des slots (section CULT) ---------------------------------- */
void culture_slots_save(FILE *f){
    uint32_t n = (uint32_t)CULTURE_SLOTS;
    fwrite(&n, 4, 1, f);
    for (int s=0;s<CULTURE_SLOTS;s++){
        uint8_t act = g_slot[s].active ? 1u : 0u;
        int32_t her = (int32_t)g_slot[s].heritage;
        int32_t eth = (int32_t)g_slot[s].ethos;
        int32_t tr[CAT_COUNT];
        for (int c=0;c<CAT_COUNT;c++) tr[c] = (int32_t)g_slot[s].build.trait[c];
        fwrite(&act,1,1,f); fwrite(&her,4,1,f); fwrite(&eth,4,1,f);
        fwrite(tr, 4, CAT_COUNT, f);
    }
    cid_map_ensure();
    uint32_t m = (uint32_t)CULTURE_MAX_CID;
    fwrite(&m, 4, 1, f);
    for (int i=0;i<CULTURE_MAX_CID;i++){ int32_t v=(int32_t)g_cid_slot[i]; fwrite(&v,4,1,f); }
}
bool culture_slots_load(FILE *f){
    uint32_t n=0;
    if (fread(&n,4,1,f)!=1 || n!=(uint32_t)CULTURE_SLOTS) return false;
    for (int s=0;s<CULTURE_SLOTS;s++){
        uint8_t act=0; int32_t her=0, eth=0, tr[CAT_COUNT];
        if (fread(&act,1,1,f)!=1 || fread(&her,4,1,f)!=1 || fread(&eth,4,1,f)!=1) return false;
        if (fread(tr,4,CAT_COUNT,f)!=(size_t)CAT_COUNT) return false;
        g_slot[s].active   = act!=0;
        g_slot[s].heritage = (her>=0&&her<HERITAGE_COUNT) ? (Heritage)her : HERITAGE_ADAPTATIF;
        g_slot[s].ethos    = eth;
        for (int c=0;c<CAT_COUNT;c++)
            g_slot[s].build.trait[c] = (tr[c]>=0&&tr[c]<TRAIT_COUNT) ? (TraitId)tr[c] : T_PROLIFIQUE;
    }
    uint32_t m=0;
    if (fread(&m,4,1,f)!=1 || m!=(uint32_t)CULTURE_MAX_CID) return false;
    cid_map_ensure();
    for (int i=0;i<CULTURE_MAX_CID;i++){
        int32_t v=0; if (fread(&v,4,1,f)!=1) return false;
        g_cid_slot[i] = (v>=-1 && v<CULTURE_SLOTS) ? (int)v : -1;   /* borne : slot valide ou aucun */
    }
    g_cid_map_init = true;
    return true;
}

/* ---- Compat « joueur » = slot 0 ------------------------------------------- */
void culture_player_compose(Heritage heritage, int ethos, HeritageBuild build){
    culture_slot_set(0, heritage, ethos, build);
}
void culture_player_bind(int cid){ culture_bind_cid(cid, 0); }
void culture_player_clear(void){ culture_slot_clear_all(); }
bool culture_player_active(void){ return culture_any_active(); }
int  culture_player_cid(void){
    cid_map_ensure();
    for (int i=0;i<CULTURE_MAX_CID;i++) if (g_cid_slot[i]==0) return i;
    return -1;
}
Heritage culture_player_heritage(void){ return culture_slot_heritage(0); }
int              culture_player_ethos(void){ return culture_slot_ethos(0); }
