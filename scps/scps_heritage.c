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
    { .coercition=+0.5f, .rendement=+0.10f },
    "Forte constitution : Coercition +0.5, production +10 %.",
    "Ils apprennent très jeunes que la douleur est une information, non une raison de s'arrêter." },
[T_REGENERANT]   = { "Régénérant",   CAT_PHYSIQUE, +2, T_CONVALESCENT,
    { .demographie=+0.10f, .coercition=+0.5f },
    "Sang de troll, guérit vite : croissance de la population +10 %, Coercition +0.5.",
    "Leurs blessures se ferment vite. Le souvenir de celui qui les a infligées prend généralement davantage de temps." },
[T_PROLIFIQUE]   = { "Prolifique",   CAT_PHYSIQUE, +1, T_LENT_CROITRE,
    { .demographie=+0.10f },
    "Reproduction rapide : croissance de la population +10 %.",
    "Chaque maison manque de lits, chaque table de chaises et chaque génération de terres à partager." },
[T_LONGEVIF]     = { "Longévif",     CAT_PHYSIQUE, +1, T_EPHEMERE,
    { .derive=-0.20f, .capacite=+0.25f },
    "Vie longue : la culture locale se fond 20 % moins vite au contact des voisins (l'héritage se garde), capacité de l'État +0.25 (les minorités mieux servies : −7.5 % de pénalité).",
    "Ils enterrent rarement leurs maîtres, mais vivent assez longtemps pour voir chacune de leurs erreurs devenir une coutume." },
[T_ENDURANT]     = { "Endurant",     CAT_PHYSIQUE, +1, T_FRAGILE_CLIMAT,
    { .demographie=+0.10f },
    "Résiste aux épreuves : croissance de la population +10 %.",
    "L'hiver leur prend du bétail, l'été des récoltes. Aucun des deux n'a encore obtenu leur départ." },
[T_SOBRE]        = { "Sobre",        CAT_PHYSIQUE, +1, T_VORACE,
    { .rendement=+0.10f },
    "Consomme peu : production +10 %.",
    "Ils font durer une miche, un manteau et une rancune bien au-delà de ce que leurs voisins jugent raisonnable." },
/* ---- PHYSIQUE — défauts ---- */
[T_FRELE]        = { "Frêle",        CAT_PHYSIQUE, -1, T_ROBUSTE,
    { .rendement=-0.10f },
    "Faible constitution : production −10 %.",
    "Ils ont appris à survivre en évitant les charges que d'autres appellent courageuses et les travaux que d'autres appellent nécessaires." },
[T_CONVALESCENT] = { "Convalescent", CAT_PHYSIQUE, -1, T_REGENERANT,
    { .demographie=-0.10f },
    "Guérison lente : croissance de la population −10 %.",
    "Chez eux, toute blessure devient une saison et toute épidémie une génération manquante." },
[T_LENT_CROITRE] = { "Lent à croître",CAT_PHYSIQUE, -1, T_PROLIFIQUE,
    { .demographie=-0.10f },
    "Reproduction lente : croissance de la population −10 %.",
    "Chaque enfant est attendu longtemps, protégé jalousement et chargé très tôt de l'avenir de toute une lignée." },
[T_EPHEMERE]     = { "Éphémère",     CAT_PHYSIQUE, -1, T_LONGEVIF,
    { .derive=+0.20f, .capacite=-0.25f },
    "Vie courte : la culture locale se fond 20 % plus vite au contact des voisins (l'héritage se dilue), capacité de l'État −0.25 (+7.5 % de pénalité minoritaire).",
    "Ils vivent assez pour commencer une œuvre, rarement pour la finir. Leurs héritiers appellent cela une tradition familiale." },
[T_FRAGILE_CLIMAT]={ "Fragile au climat",CAT_PHYSIQUE,-1, T_ENDURANT,
    { .demographie=-0.10f },
    "Constitution fragile : croissance de la population −10 %.",
    "Leur corps se souvient d'une terre précise et traite chaque autre saison comme une offense personnelle." },
[T_VORACE]       = { "Vorace",       CAT_PHYSIQUE, -1, T_SOBRE,
    { .rendement=-0.10f },
    "Consomme beaucoup : production −10 %.",
    "Leurs festins commencent avant que les réserves ne soient comptées et se terminent lorsqu'il ne reste plus personne pour protester." },
/* ---- SOCIAL — atouts ---- */
[T_BELLIQUEUX]   = { "Belliqueux",   CAT_SOCIAL, +2, T_DEBONNAIRE,
    { .coercition=+1.0f },
    "Aime la guerre : Coercition +1.",
    "La paix est pour eux l'intervalle pendant lequel on polit les armes et explique aux enfants le nom du prochain ennemi." },
[T_SOUDE]        = { "Soudé",        CAT_SOCIAL, +2, T_FACTIEUX,
    { .fracture=-1.0f },
    "Solidarité large : Fracture −1 (−0.06 sur le grief de révolte).",
    "Ils se querellent entre eux avec une ardeur remarquable, puis se rangent côte à côte dès qu'un étranger prend parti." },
[T_CHARISMATIQUE]= { "Charismatique",CAT_SOCIAL, +1, T_REBUTANT,
    { .influence=+0.5f },
    "Présence : rayonnement diplomatique +5 (portée & prestige).",
    "Leurs ambassadeurs obtiennent rarement tout ce qu'ils demandent. Ils repartent pourtant avec davantage que ce que l'on voulait leur donner." },
[T_OUVERT]       = { "Ouvert",       CAT_SOCIAL, +1, T_INSULAIRE,
    { .permeabilite=+0.5f },
    "Hospitalier : assimilation des minorités ~+8 % plus vite.",
    "Une place reste toujours libre près du foyer. L'étranger qui l'occupe repart parfois parent, parfois propriétaire." },
[T_DISCIPLINE]   = { "Discipliné",   CAT_SOCIAL, +1, T_FRONDEUR,
    { .capacite=+0.5f },
    "Ordre intériorisé : capacité de l'État +0.5 ; les minorités sont mieux servies (−15 % de pénalité de diversité).",
    "Ils obéissent avant que l'ordre ne soit répété, ce qui inquiète parfois davantage que la désobéissance." },
[T_PROSELYTE]    = { "Prosélyte",    CAT_SOCIAL, +1, T_RESERVE,
    { .influence=+0.25f, .permeabilite=+0.25f },
    "Rayonne sa culture : rayonnement diplomatique +2.5, assimilation des minorités ~+4 % plus vite.",
    "Ils ne voyagent jamais seuls. Avec eux partent leurs chansons, leurs coutumes et l'assurance qu'elles amélioreront votre foyer." },
/* ---- SOCIAL — défauts ---- */
[T_DEBONNAIRE]   = { "Débonnaire",   CAT_SOCIAL, -1, T_BELLIQUEUX,
    { .coercition=-0.5f },
    "Rétif à la guerre : Coercition −0.5.",
    "Ils pardonnent facilement, négocient longtemps et découvrent parfois trop tard que l'autre camp ne partageait aucune de ces habitudes." },
[T_FACTIEUX]     = { "Factieux",     CAT_SOCIAL, -1, T_SOUDE,
    { .fracture=+0.5f },
    "Clans rivaux : Fracture +0.5 (+0.03 sur le grief de révolte).",
    "Ils s'unissent volontiers contre l'ennemi commun, après avoir épuisé toutes les possibilités de se désigner mutuellement comme cet ennemi." },
[T_REBUTANT]     = { "Rebutant",     CAT_SOCIAL, -1, T_CHARISMATIQUE,
    { .influence=-0.5f },
    "Déplaisant : rayonnement diplomatique −5.",
    "Leurs émissaires disent toujours la vérité. Le problème est la joie visible avec laquelle ils choisissent le moment de la dire." },
[T_INSULAIRE]    = { "Insulaire",    CAT_SOCIAL, -1, T_OUVERT,
    { .permeabilite=-0.5f },
    "Méfiant : assimilation des minorités ~8 % plus lente.",
    "Ils accueillent les étrangers avec correction, les observent avec patience et attendent avec confiance le jour de leur départ." },
[T_FRONDEUR]     = { "Frondeur",     CAT_SOCIAL, -1, T_DISCIPLINE,
    { .capacite=-0.5f },
    "Indiscipliné : capacité de l'État −0.5 ; les minorités sont moins bien servies (+15 % de pénalité de diversité).",
    "Aucun ordre n'est assez raisonnable pour ne pas mériter une objection, et aucune objection assez faible pour rester privée." },
[T_RESERVE]      = { "Réservé",      CAT_SOCIAL, -1, T_PROSELYTE,
    { .influence=-0.25f, .permeabilite=-0.25f },
    "Sa culture ne s'exporte pas : rayonnement diplomatique −2.5, assimilation des minorités ~4 % plus lente.",
    "Ils gardent leurs chants pour leurs enfants et leurs coutumes pour leurs foyers. Le monde confond souvent discrétion et absence." },
/* ---- INTELLECTUEL — atouts ---- */
[T_INVENTIF]     = { "Inventif",     CAT_INTELLECTUEL, +2, T_BORNE,
    { .rendement=+0.20f },
    "Innovation : production +20 %.",
    "Ils regardent un outil, demandent pourquoi il a cette forme et le rendent inutilisable trois fois avant de le rendre indispensable." },
[T_ARCANIQUE]    = { "Arcanique",    CAT_INTELLECTUEL, +2, T_SOURD_ARCANE,
    { .arcane=+1.0f },
    "Sensible à la magie : la branche faustienne coûte −25 %, mais la pente vers la Brèche s'accentue.",
    "La magie les reconnaît avant même qu'ils apprennent à la nommer. Elle ne leur demande jamais s'ils souhaitent être reconnus." },
[T_STUDIEUX]     = { "Studieux",     CAT_INTELLECTUEL, +1, T_INCULTE,
    { .capacite=+0.5f },
    "Soif de savoir : capacité de l'État +0.5 ; les minorités sont mieux servies (−15 % de pénalité de diversité).",
    "Ils annotent les marges, contestent les maîtres et découvrent parfois que la réponse était cachée dans une note qu'ils avaient eux-mêmes ajoutée." },
[T_BATISSEUR]    = { "Bâtisseur",    CAT_INTELLECTUEL, +1, T_BROUILLON,
    { .capacite=+0.25f, .rendement=+0.05f },
    "Méthodique : capacité de l'État +0.25 (organisation, −7.5 % de pénalité de diversité), production +5 %.",
    "Avant de poser la première pierre, ils savent déjà où tombera l'ombre du dernier mur au solstice d'hiver." },
[T_ADAPTABLE]    = { "Adaptable",    CAT_INTELLECTUEL, +1, T_TRADITIONALISTE,
    { .permeabilite=+0.5f },
    "Curieux : assimilation des minorités ~+8 % plus vite.",
    "Ils arrivent avec leurs usages, empruntent ceux du voisin et jurent ensuite les avoir toujours pratiqués ainsi." },
[T_INDUSTRIEUX]  = { "Industrieux",  CAT_INTELLECTUEL, +1, T_INDOLENT,
    { .rendement=+0.10f },
    "Appliqué : production +10 %.",
    "Ils ne travaillent pas plus vite que les autres. Ils commencent simplement avant que les autres aient fini d'en discuter." },
/* ---- INTELLECTUEL — défauts ---- */
[T_BORNE]        = { "Borné",        CAT_INTELLECTUEL, -1, T_INVENTIF,
    { .rendement=-0.10f },
    "Esprit fermé : production −10 %.",
    "Ils savent déjà ce qui est vrai. Toute preuve contraire démontre donc seulement la perfidie de celui qui la présente." },
[T_SOURD_ARCANE] = { "Sourd à l'arcane",CAT_INTELLECTUEL,-1, T_ARCANIQUE,
    { .arcane=-1.0f },
    "Imperméable à la magie : la branche faustienne coûte +25 % (protection), mais la pente vers la Brèche s'adoucit — au prix des débouchés arcanes, qui se ferment aussi.",
    "Les murmures des ruines ne leur disent rien. Ils considèrent cette ignorance comme une faiblesse, jusqu'à ce que les autres commencent à répondre." },
[T_INCULTE]      = { "Inculte",      CAT_INTELLECTUEL, -1, T_STUDIEUX,
    { .capacite=-0.5f },
    "Indifférent au savoir : capacité de l'État −0.5 ; les minorités sont moins bien servies (+15 % de pénalité de diversité).",
    "Ils se méfient des livres, surtout de ceux qui prétendent conserver la mémoire mieux que les anciens du village." },
[T_BROUILLON]    = { "Brouillon",    CAT_INTELLECTUEL, -1, T_BATISSEUR,
    { .capacite=-0.25f, .rendement=-0.05f },
    "Désordonné : capacité de l'État −0.25 (+7.5 % de pénalité de diversité), production −5 %.",
    "Leurs plans contiennent toutes les bonnes idées, souvent sur des feuilles différentes et rarement dans le bon ordre." },
[T_TRADITIONALISTE]={ "Traditionaliste",CAT_INTELLECTUEL,-1, T_ADAPTABLE,
    { .derive=-0.20f, .permeabilite=-0.5f },
    "Rigide : la culture locale se fond 20 % moins vite au contact des voisins (l'héritage est très stable), assimilation des minorités ~8 % plus lente.",
    "Une coutume n'a pas besoin d'être utile. Il lui suffit d'avoir survécu assez longtemps pour rendre toute alternative suspecte." },
[T_INDOLENT]     = { "Indolent",     CAT_INTELLECTUEL, -1, T_INDUSTRIEUX,
    { .rendement=-0.10f },
    "Nonchalant : production −10 %.",
    "Rien ne presse tant que le soleil reviendra demain. Lorsque demain arrive, cette vérité reste heureusement inchangée." },
};

const TraitDef *trait_def(TraitId t){ return (t>=0&&t<TRAIT_COUNT)?&TRAITS[t]:NULL; }
const char     *trait_name(TraitId t){ return (t>=0&&t<TRAIT_COUNT)?TRAITS[t].name:"?"; }
const char     *trait_hover(TraitId t){ return (t>=0&&t<TRAIT_COUNT)?TRAITS[t].hover:""; }
const char     *trait_flavor(TraitId t){ return (t>=0&&t<TRAIT_COUNT)?TRAITS[t].flavor:""; }

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
