/*
 * scps_toponym.c — TOPONYMIE DES VILLES (docs/DESIGN_TOPONYMIE_VILLES.md).
 *
 * Pipeline (doc §4, brief de mission) :
 *   1. localisation (§10, ordre STRICT — géographie de la province-ANCRE) ;
 *   2. clé d'éthos (§7, ~1/4 de base, renforcée SEULEMENT si un état réel le
 *      prouve — jamais une IF isolée sur l'éthos seul) ;
 *   3. marqueur historique (§8 SEULEMENT — un seul, priorité = ordre du
 *      tableau du doc, première condition vraie retenue) ;
 *   4. morphèmes via jitter de novlang (§5 : hash(culture_id,clé) 80 % /
 *      hash(province_id,clé) 20 %, DÉTERMINISTE — hash pur, aucun rng_f()) ;
 *   5. assemblage (§11 : racine de lieu obligatoire + 0-1 éthos + 0-1
 *      marqueur, gabarits [modificateur]+[racine de lieu]) ;
 *   6. lissage (§12 : fusion de voyelles identiques, retrait d'une consonne
 *      devant un groupe imprononçable, assimilation gm→mm/gs→ss/dt→t).
 *
 * SIMPLIFICATIONS ASSUMÉES (documentées, aucune n'invente un état — §9) :
 *   - le gabarit « [mot autonome] de [racine de lieu] » (§11) n'est PAS
 *     implémenté : tous les « mots autonomes » du §6/§7/§8 (Havre, Castel,
 *     Nouvelle, Marché, Siège…) sont donc absents du lexique ci-dessous — ce
 *     qui rend 5 des 6 combos interdits du §11 STRUCTURELLEMENT inatteignables
 *     (ils ne mêlent qu'un mot autonome jamais émis avec un préfixe) ; seul
 *     « Mont + Berg » (préfixe ET suffixe de la MÊME ligne §6 Montagne) reste
 *     atteignable — gardé explicitement (cf. toponym_words_collide) ;
 *   - le gabarit « [racine] sur [nom de fleuve] » (désambiguïsation, §11) et
 *     la désambiguïsation géographique des doublons PROCHES (§14) ne sont pas
 *     implémentés (aucun nom de fleuve stocké ; les doublons MONDIAUX restent
 *     explicitement autorisés par le doc — seul le cas proche resterait à
 *     traiter, laissé en Restes) ;
 *   - la ligne « Vallée » du §6 n'est jamais sélectionnée : le §10 (source de
 *     vérité pour la SÉLECTION) ne l'atteint par aucune branche — dégradation
 *     propre vers le suffixe géré par la branche relief la plus proche.
 *   - le suffixe accentué de 2 lignes (Île « -île », Plaine « -pré ») est
 *     translittéré en ASCII (« ile », « pre ») : le lissage §12 fait de la
 *     chirurgie de chaîne OCTET PAR OCTET à la jonction (fusion de voyelles,
 *     retrait de consonne) — un accent UTF-8 (2 octets) coupé au mauvais
 *     endroit corromprait le nom. Simplification délibérée, ASCII partout.
 *
 * STOCKAGE : tableau STATIC de MODULE, grain RÉGION (doc §1), motif WILD/
 * EMOB/COLC/TXYR (scps_econ.c) — PAS sur World.region[] (aurait fallu
 * toucher scps_types.h, hors périmètre de cette mission) ni sur
 * WorldEconomy.region[] (RegionEconomy est une VUE recomposée à chaque
 * econ_aggregate_regions — un champ posé là serait effacé au tick suivant).
 */
#include "scps_toponym.h"
#include "scps_agency.h"   /* EDI_* (édifices réellement construits, §7-§8) */
#include "scps_tune.h"     /* tune_f : seuils registre J (SCPS_TUNE=X=Y) */
#include <string.h>
#include <ctype.h>

/* ════════════════════════════════════════════════════════════════════════
 * LEXIQUE — §5 jitter de novlang, §6 géographie, §7 éthos, §8 marqueurs.
 * Tout en MINUSCULES, SANS trait d'union (concaténation directe) : la
 * capitale du nom final est posée une seule fois, à l'export.
 * ══════════════════════════════════════════════════════════════════════ */

typedef enum {
    LOC_ESTUAIRE=0, LOC_PORT, LOC_FLEUVE, LOC_GUE, LOC_LAC, LOC_RADE, LOC_ILE,
    LOC_MONTAGNE, LOC_COL, LOC_COLLINE, LOC_FORET, LOC_CLAIRIERE, LOC_MARAIS,
    LOC_PLAINE, LOC_LANDE, LOC_DESERT, LOC_FROID, LOC_VOLCAN,
    LOC_COUNT
} LocKey;

typedef struct { const char *pfx[5]; int npfx; const char *sfx[3]; int nsfx; } LocLex;

/* §6 — la forme EXACTE est choisie par le jitter (culture dominante 80 % /
 * variante locale 20 %) ; l'ordre des colonnes suit le tableau du doc. */
static const LocLex LOC_TABLE[LOC_COUNT] = {
    [LOC_ESTUAIRE]  = { {"aber","abar","avr","aver","embr"}, 5, {"avre","aber","mund"}, 3 },
    [LOC_PORT]      = { {"port","por","haf"},                3, {"port","haven","hafen"}, 3 },
    [LOC_FLEUVE]    = { {"av","ava","avar","evr","nant"},    5, {"rive","avon","nant"}, 3 },
    [LOC_GUE]       = { {"briv","brev","rit","brod"},        4, {"brive","furt","brod"}, 3 },
    [LOC_LAC]       = { {"lim","limn","lann","len","mer"},   5, {"lac","mere","lann"}, 3 },
    [LOC_RADE]      = { {"mor","mar","cal","kal","vik"},     5, {"mer","cale","vik"}, 3 },
    [LOC_ILE]       = { {"ins","inis","yn","nis","holm"},    5, {"ile","nis","holm"}, 3 },          /* -île → ASCII, cf. en-tête */
    [LOC_MONTAGNE]  = { {"mont","mon","mund","tor","dorn"},  5, {"mont","tor","berg"}, 3 },
    [LOC_COL]       = { {"col","pas","porth"},               3, {"col","passe","porte"}, 3 },
    [LOC_COLLINE]   = { {"dun","don","doun","bel","ben"},    5, {"bel","dun","col"}, 3 },
    [LOC_FORET]     = { {"silv","selv","syl","vern","wald"}, 5, {"sylve","bois","wald"}, 3 },
    [LOC_CLAIRIERE] = { {"sart","sarth","ess","ros","rod"},  5, {"sart","clair","rode"}, 3 },
    [LOC_MARAIS]    = { {"pal","pall","fagn","vagn","marn"}, 5, {"pal","fagne","marais"}, 3 },
    [LOC_PLAINE]    = { {"camp","champ","kam","prat","feld"},5, {"champ","pre","feld"}, 3 },        /* -pré → ASCII, cf. en-tête */
    [LOC_LANDE]     = { {"land","lann","causs","daur","step"},5,{"lande","causse","step"}, 3 },
    [LOC_DESERT]    = { {"sab","sabr","aren","aran","erg"},  5, {"sable","dune","erg"}, 3 },
    [LOC_FROID]     = { {"nev","nevr","gel","ghel","giv"},   5, {"neige","gel","givre"}, 3 },
    [LOC_VOLCAN]    = { {"cendr","sendr","pyr","pir","sulf"},5, {"cendre","feu","puy"}, 3 },
};

typedef struct { const char *pfx[4]; int npfx; } EthosLex;

/* §7 — même ORDRE que l'enum Ethos (DOMINATEUR..PACIFISTE, scps_culture.h). */
static const EthosLex ETHOS_TABLE[ETHOS_COUNT] = {
    [ETHOS_DOMINATEUR]  = { {"cast","bast","gard","haut"}, 4 },
    [ETHOS_HONNEUR]     = { {"fran","bren","bran","ser"},  4 },
    [ETHOS_ORDRE]       = { {"reg","cad","clos","met"},    4 },
    [ETHOS_BUREAUCRATE] = { {"cour","cur","sig","chan"},   4 },
    [ETHOS_MERCANTILE]  = { {"marc","merc","bors","carr"}, 4 },
    [ETHOS_PACIFISTE]   = { {"hav","hal","eir","len"},     4 },
};

typedef enum {
    MARK_NONE = -1,
    MARK_FERVEUR = 0, MARK_CAPITALE, MARK_CITYSTATE, MARK_WILD, MARK_FRONTIER,
    MARK_GARRISON, MARK_MARKET, MARK_PORT, MARK_SANCT, MARK_SAVOIR, MARK_RECONSTRUCTION,
    MARK_COUNT
} MarkKey;

typedef struct { const char *pfx[7]; int npfx; } MarkLex;

/* §8 — SEUL le préfixe est retenu (les formes « -neuve »/« -cour »/« -franc »… du
 * doc sont des SUFFIXES, hors gabarit : la localisation fournit toujours le
 * suffixe, §1 « la localisation fournit le sens principal »). Ordre du
 * tableau = ordre de PRIORITÉ (select_marker, première condition vraie). */
static const MarkLex MARK_TABLE[MARK_COUNT] = {
    [MARK_FERVEUR]        = { {"novi","nova","novo","nouv","neuv","neu","nav"}, 7 },
    [MARK_CAPITALE]       = { {"grand","gran","haut","alt","cour"}, 5 },
    [MARK_CITYSTATE]      = { {"fran","franc","frei","libre"}, 4 },
    [MARK_WILD]           = { {"libre","fran","foyer"}, 3 },
    [MARK_FRONTIER]       = { {"marc","march","gard","ward"}, 4 },
    [MARK_GARRISON]       = { {"fort","cast","castel","garde"}, 4 },
    [MARK_MARKET]         = { {"marc","merc","bors"}, 3 },
    [MARK_PORT]           = { {"port","por","hav"}, 3 },
    [MARK_SANCT]          = { {"sanct","sant","sacr"}, 3 },
    [MARK_SAVOIR]         = { {"sav","sap","vig","clair"}, 4 },
    [MARK_RECONSTRUCTION] = { {"re","neuve"}, 2 },
};

/* ════════════════════════════════════════════════════════════════════════
 * HASH DÉTERMINISTE (finisseur murmur3, style déjà utilisé par place_make_
 * name/country_make_name/worldgen_seed_peoples, scps_world.c) — AUCUN
 * rng_f() : le jitter de novlang §5 est un hash PUR, jamais un flux d'état.
 * ══════════════════════════════════════════════════════════════════════ */
static uint32_t toponym_hash2(uint32_t a, uint32_t b){
    uint32_t h = a*2654435761u ^ (b*40503u + 0x9E3779B9u);
    h ^= h>>16; h *= 0x7feb352du; h ^= h>>15; h *= 0x846ca68bu; h ^= h>>16;
    return h;
}
/* §5 : 80 % forme DOMINANTE de la culture (hash(culture_id,clé)), 20 % variante
 * LOCALE de la même famille (hash(province_id,clé)) — jamais un tirage indépendant
 * par ville (la culture garde ses choix dominants, doc §5/§14). */
static int toponym_jitter_w(uint16_t culture_id, int province_id, uint32_t salt, int n, uint32_t dom){
    if (n<=1) return 0;
    uint32_t hc = toponym_hash2((uint32_t)culture_id, salt);
    uint32_t roll = toponym_hash2(hc, 0x5A5A5A5Au ^ salt);
    if ((roll % 100u) < dom) return (int)(hc % (uint32_t)n);
    uint32_t hp = toponym_hash2((uint32_t)province_id, salt ^ 0xC0FFEEu);
    return (int)(hp % (uint32_t)n);
}
/* 80→62 (joueur 2026-08-19) : la dominante culturelle reste MAJORITAIRE (§5)
 * mais respire — 38 %% de variantes locales au lieu de 20. Les SUFFIXES de
 * localisation, eux, parlent pour la GÉOGRAPHIE : dominance INVERSÉE (38 %%),
 * sinon un empire entier finit en -avre/-furt (mesuré graine 205). */
static int toponym_jitter(uint16_t culture_id, int province_id, uint32_t salt, int n){
    return toponym_jitter_w(culture_id, province_id, salt, n, 62u);
}

/* ════════════════════════════════════════════════════════════════════════
 * §11 — COMBOS INTERDITS : 6 paires nommées (aucun empilement de synonymes).
 * ══════════════════════════════════════════════════════════════════════ */
static bool toponym_words_collide(const char *a, const char *b){
    static const char *PAIR[6][2] = {
        {"port","havre"}, {"mont","berg"}, {"fort","castel"},
        {"neuve","nouvelle"}, {"marc","marche"}, {"cour","siege"},
    };
    if (strcmp(a,b)==0) return true;   /* le MÊME morphème deux fois = empilement (§11), pas seulement les 6 nommées */
    for (int i=0;i<6;i++){
        if ((strcmp(a,PAIR[i][0])==0 && strcmp(b,PAIR[i][1])==0) ||
            (strcmp(a,PAIR[i][1])==0 && strcmp(b,PAIR[i][0])==0))
            return true;
    }
    return false;
}

/* ════════════════════════════════════════════════════════════════════════
 * §12 — LISSAGE PHONÉTIQUE (déterministe, 3 règles exactes du doc).
 * ══════════════════════════════════════════════════════════════════════ */
static bool toponym_is_vowel(char c){ return c=='a'||c=='e'||c=='i'||c=='o'||c=='u'||c=='y'; }

static void toponym_smooth(char *out, size_t outsz, const char *a, const char *b){
    size_t la=strlen(a), lb=strlen(b);
    if (la==0){ snprintf(out,outsz,"%s",b); return; }
    if (lb==0){ snprintf(out,outsz,"%s",a); return; }
    char last=a[la-1], first=b[0];
    /* règle 1 : fusionner deux voyelles identiques à la jonction (Nova+Avre→Novavre) */
    if (toponym_is_vowel(last) && toponym_is_vowel(first) && last==first){
        snprintf(out,outsz,"%.*s%s",(int)la,a,b+1); return;
    }
    /* règle « assimilation locale » : gm→mm, gs→ss, dt→t (Reg+Sart→Ressart) */
    if (last=='g' && first=='m'){ snprintf(out,outsz,"%.*smm%s",(int)(la-1),a,b+1); return; }
    if (last=='g' && first=='s'){ snprintf(out,outsz,"%.*sss%s",(int)(la-1),a,b+1); return; }
    if (last=='d' && first=='t'){ snprintf(out,outsz,"%.*st%s",(int)(la-1),a,b+1); return; }
    /* règle 2 : retirer la dernière consonne d'un préfixe devant un groupe
     * imprononçable (≥3 consonnes consécutives à la jonction) : Cast+Brive→Casbrive */
    if (!toponym_is_vowel(last) && !toponym_is_vowel(first)){
        int ca=0; for (int i=(int)la-1; i>=0 && !toponym_is_vowel(a[i]); i--) ca++;
        int cb=0; for (size_t i=0; i<lb && !toponym_is_vowel(b[i]); i++) cb++;
        if (ca+cb>=3){ snprintf(out,outsz,"%.*s%s",(int)(la-1),a,b); return; }
    }
    snprintf(out,outsz,"%s%s",a,b);
}

static int toponym_syllables(const char *s){
    int n=0; bool prev=false;
    for (const char *p=s; *p; p++){ bool v=toponym_is_vowel(*p); if (v && !prev) n++; prev=v; }
    return n>0 ? n : 1;
}

/* ════════════════════════════════════════════════════════════════════════
 * §10 — SÉLECTION DE LA LOCALISATION (ordre STRICT, dégradation propre :
 * la ligne « Vallée » du §6 n'est jamais atteinte par ce chaînage — cf.
 * en-tête). Ancre = province colonisée qui PORTE la ville (jamais un
 * pointeur dynamique — figé par l'appelant, cf. toponym_world_tick).
 * ══════════════════════════════════════════════════════════════════════ */
static LocKey toponym_select_location(const World *w, const Province *pv, const ProvinceEconomy *pe,
                                       const Region *rg, bool has_river, bool has_lake, uint8_t river_max){
    if (pe->estuary) return LOC_ESTUAIRE;
    if (pe->edi_built & (1u<<EDI_PORT)) return LOC_PORT;
    if (has_river) return (river_max >= (uint8_t)tune_f("TOPONYM_RIVER_MAJOR",160.f)) ? LOC_FLEUVE : LOC_GUE;
    if (has_lake) return LOC_LAC;
    if (pv->coastal && rg->harbor >= tune_f("TOPONYM_HARBOR_HIGH",0.5f)) return LOC_RADE;
    if (pv->continent>=0 && pv->continent<w->n_continents &&
        w->continent[pv->continent].area <= (int)tune_f("TOPONYM_ISLAND_MAX_AREA",700.f)) return LOC_ILE;
    switch (pv->biome_dominant){
        case BIO_MOUNTAINS: case BIO_PEAK:     return LOC_MONTAGNE;
        case BIO_HIGHLANDS:                    return LOC_COL;
        case BIO_HILLS:                        return LOC_COLLINE;
        case BIO_FOREST: case BIO_WOODS: case BIO_JUNGLE:
            return (pe->ferveur>0.5f) ? LOC_CLAIRIERE : LOC_FORET;   /* fondation fraîche = défrichement */
        case BIO_MARSH: case BIO_BOG: case BIO_MANGROVE: return LOC_MARAIS;
        case BIO_PLAINS: case BIO_FARMLAND: case BIO_GRASSLAND:      return LOC_PLAINE;
        case BIO_STEPPE: case BIO_SAVANNA: case BIO_DRYLANDS:        return LOC_LANDE;
        case BIO_DESERT: case BIO_COASTAL_DESERT:                    return LOC_DESERT;
        case BIO_GLACIER:                                            return LOC_FROID;
        case BIO_VOLCANO:                                            return LOC_VOLCAN;
        default: return LOC_PLAINE;   /* dégradation propre — §9, on n'invente pas de biome */
    }
}

/* frontière avec un AUTRE propriétaire (grain RÉGION, adjacence LIVE — jamais
 * les flags border_* figés à la genèse, cf. TROUVAILLES « TOPONYMIE DES
 * VILLES »). */
static bool toponym_is_frontier(const WorldEconomy *econ, int region){
    int owner = econ->region[region].owner;
    if (owner<0) return false;
    int nr = econ->n_regions; if (nr>SCPS_MAX_REG) nr=SCPS_MAX_REG;
    for (int s=0; s<nr; s++){
        if (s==region || !econ->adj[region][s]) continue;
        int so = econ->region[s].owner;
        if (so>=0 && so!=owner) return true;
    }
    return false;
}

/* §8 — un seul marqueur, priorité = ordre du tableau (première condition VRAIE). */
static MarkKey toponym_select_marker(const ProvinceEconomy *pe, int role, bool frontier){
    /* RÉORDONNÉ (joueur 2026-08-19 « trop grosses récurrences ») : la ferveur en
     * tête de priorité capturait PRESQUE TOUTES les villes — des cartes entières
     * en « Neuv-/Nov- ». La FONCTION prime désormais (une ville à port s'appelle
     * Port-, fervente ou pas) ; la ferveur — relevée à 0.8, l'ARDENTE seule —
     * devient un repli avant reconstruction. */
    if (pe->is_capital) return MARK_CAPITALE;
    if (role==POLITY_CITY_STATE) return MARK_CITYSTATE;
    if (role==POLITY_WILD) return MARK_WILD;
    if (frontier) return MARK_FRONTIER;
    if (pe->edi_built & ((1u<<EDI_GARNISON)|(1u<<EDI_FORTERESSE)|(1u<<EDI_CITADELLE))) return MARK_GARRISON;
    if (pe->edi_built & ((1u<<EDI_MARCHE)|(1u<<EDI_COMPTOIR)|(1u<<EDI_TRADE_CENTER))) return MARK_MARKET;
    if (pe->edi_built & ((1u<<EDI_PORT)|(1u<<EDI_PORT_MARCHAND))) return MARK_PORT;
    if (pe->edi_built & ((1u<<EDI_SANCTUAIRE)|(1u<<EDI_TEMPLE)|(1u<<EDI_CATHEDRALE))) return MARK_SANCT;
    if (pe->edi_built & ((1u<<EDI_ACADEMIE)|(1u<<EDI_BIBLIOTHEQUE)|(1u<<EDI_MONASTERE)|(1u<<EDI_OBSERVATOIRE))) return MARK_SAVOIR;
    if (pe->ferveur > 0.8f) return MARK_FERVEUR;
    /* attribution INITIALE seulement (§8 dernière ligne) — toujours vrai ici : ce
     * balayage n'assigne JAMAIS un nom deux fois (cf. toponym_world_tick). */
    if (pe->reconstruction > 0.5f) return MARK_RECONSTRUCTION;
    return MARK_NONE;
}

/* §7 — clé d'éthos : ~1/4 de base, renforcée SEULEMENT si un état réel le
 * prouve (les 11 IF du §7 « renforcements conditionnels »). */
static bool toponym_select_ethos(const ProvinceEconomy *pe, int role, const Region *rg, bool frontier){
    Ethos e = pe->culture.ethos;
    if (e<0 || e>=ETHOS_COUNT) return false;
    bool reinforced=false;
    switch (e){
        case ETHOS_DOMINATEUR:
            reinforced = frontier || (pe->edi_built & ((1u<<EDI_GARNISON)|(1u<<EDI_FORTERESSE)|(1u<<EDI_CITADELLE)));
            break;
        case ETHOS_HONNEUR:
            reinforced = (role==POLITY_WILD);
            break;
        case ETHOS_ORDRE:
            reinforced = (pe->edi_built & ((1u<<EDI_GARNISON)|(1u<<EDI_TRIBUNAL))) != 0;
            break;
        case ETHOS_BUREAUCRATE:
            reinforced = pe->is_capital || (pe->edi_built & ((1u<<EDI_CHANCELLERIE)|(1u<<EDI_TRIBUNAL)));
            break;
        case ETHOS_MERCANTILE:
            reinforced = pe->estuary || (pe->edi_built & ((1u<<EDI_MARCHE)|(1u<<EDI_COMPTOIR)|(1u<<EDI_TRADE_CENTER)));
            break;
        case ETHOS_PACIFISTE:
            reinforced = (pe->edi_built & ((1u<<EDI_SANCTUAIRE)|(1u<<EDI_TEMPLE)|(1u<<EDI_CATHEDRALE)))
                       || (rg->harbor >= tune_f("TOPONYM_HARBOR_HIGH",0.5f));
            break;
        default: break;
    }
    float base = reinforced ? tune_f("TOPONYM_ETHOS_REINFORCED",0.80f) : tune_f("TOPONYM_ETHOS_BASE",0.25f);
    uint32_t roll = toponym_hash2((uint32_t)pe->culture_id, 0x3000u+(uint32_t)e*4u+1u) % 1000u;
    return (roll/1000.f) < base;
}

/* ════════════════════════════════════════════════════════════════════════
 * §11 — ASSEMBLAGE : 1 racine de lieu (obligatoire, fournit le SENS
 * principal) + 0-1 éthos + 0-1 marqueur historique. Ordre marqueur→éthos→
 * lieu (précédent du doc §4 : NOVA+MERC+AVRE) ; au-delà de 4 syllabes on
 * abandonne l'éthos (précédent « forme courte » du doc §4).
 * ══════════════════════════════════════════════════════════════════════ */
static void toponym_compose(uint16_t culture_id, int pid, LocKey lk, MarkKey mk, bool ethos_on, Ethos eth,
                             char *out, size_t outsz){
    char loc_pfx[16], loc_sfx[16]="", mark_pfx[16]="", eth_pfx[16]="";
    { const LocLex *L=&LOC_TABLE[lk];
      int idx=toponym_jitter(culture_id,pid,(uint32_t)(lk*4u+0u),L->npfx);
      snprintf(loc_pfx,sizeof loc_pfx,"%s",L->pfx[idx]); }
    if (mk!=MARK_NONE){ const MarkLex *L=&MARK_TABLE[mk];
      int idx=toponym_jitter(culture_id,pid,0x2000u+(uint32_t)mk*4u,L->npfx);
      snprintf(mark_pfx,sizeof mark_pfx,"%s",L->pfx[idx]); }
    if (ethos_on){ const EthosLex *L=&ETHOS_TABLE[eth];
      int idx=toponym_jitter(culture_id,pid,0x1000u+(uint32_t)eth*4u,L->npfx);
      snprintf(eth_pfx,sizeof eth_pfx,"%s",L->pfx[idx]); }

    bool no_modifier = (!mark_pfx[0] && !eth_pfx[0]);
    const char *head1 = mark_pfx[0] ? mark_pfx : (eth_pfx[0] ? eth_pfx : loc_pfx);
    const char *head2 = (mark_pfx[0] && eth_pfx[0]) ? eth_pfx : NULL;

    /* suffixe de LIEU : toujours présent (doc §1, « la localisation fournit le
     * sens principal ») — jitté, avec retentative bornée si collision §11. */
    const LocLex *L=&LOC_TABLE[lk];
    int attempt=0;
    for (; attempt<L->nsfx; attempt++){
        int idx=toponym_jitter_w(culture_id,pid,(uint32_t)(lk*4u+1u) ^ ((uint32_t)attempt*0x1000193u),L->nsfx,38u);
        snprintf(loc_sfx,sizeof loc_sfx,"%s",L->sfx[idx]);
        bool bad = no_modifier ? toponym_words_collide(loc_pfx,loc_sfx)
                                : (toponym_words_collide(head1,loc_sfx) || (head2 && toponym_words_collide(head2,loc_sfx)));
        if (!bad) break;
    }

    char tmp[48];
    if (head2){
        char headmerge[32];
        toponym_smooth(headmerge,sizeof headmerge,head1,head2);
        toponym_smooth(tmp,sizeof tmp,headmerge,loc_sfx);
        if (toponym_syllables(tmp) > 4)   /* précédent §4 : Novamercavre → forme courte Novavre (abandonne l'éthos) */
            toponym_smooth(tmp,sizeof tmp,head1,loc_sfx);
    } else {
        toponym_smooth(tmp,sizeof tmp,head1,loc_sfx);
    }
    if (tmp[0]) tmp[0]=(char)toupper((unsigned char)tmp[0]);
    snprintf(out,outsz,"%s",tmp);
}

/* ════════════════════════════════════════════════════════════════════════
 * STOCKAGE — motif WILD/EMOB/COLC/TXYR (scps_econ.c) : static de MODULE,
 * grain RÉGION, RAZ à world_generate, sérialisé section TOPO (scps_save.c).
 * ══════════════════════════════════════════════════════════════════════ */
static char g_ville_name[SCPS_MAX_REG][TOPONYM_NAME_MAX];

void toponym_reset(void){ memset(g_ville_name,0,sizeof g_ville_name); }

const char *toponym_region_name(int region){
    if (region<0 || region>=SCPS_MAX_REG) return "";
    return g_ville_name[region];
}

void toponym_save(FILE *f){ fwrite(g_ville_name,sizeof g_ville_name,1,f); }
bool toponym_load(FILE *f){
    if (fread(g_ville_name,sizeof g_ville_name,1,f)!=1) return false;
    for (int r=0;r<SCPS_MAX_REG;r++) g_ville_name[r][TOPONYM_NAME_MAX-1]='\0';   /* save_sane : NUL défensif */
    return true;
}

/* génère et FIGE le nom d'UNE région, depuis SA province-ancre `pid`. */
static void toponym_generate_one(const World *w, const WorldEconomy *econ, int region, int pid,
                                  const bool *has_river, const bool *has_lake, const uint8_t *river_max){
    const Province        *pv=&w->province[pid];
    const ProvinceEconomy *pe=&econ->prov[pid];
    const Region           *rg=&w->region[region];
    int owner=pe->owner;
    int role=(owner>=0 && owner<w->n_countries) ? (int)w->country[owner].role : (int)POLITY_UNCLAIMED;
    bool frontier=toponym_is_frontier(econ, region);

    LocKey  lk=toponym_select_location(w,pv,pe,rg,has_river[pid],has_lake[pid],river_max[pid]);
    MarkKey mk=toponym_select_marker(pe,role,frontier);
    bool    eth_on=toponym_select_ethos(pe,role,rg,frontier);

    toponym_compose(pe->culture_id, pid, lk, mk, eth_on, pe->culture.ethos,
                     g_ville_name[region], sizeof g_ville_name[region]);
}

/* balayage annuel (appelé par world_tick, scps_world.c) — idempotent : ne
 * comble QUE les régions encore sans nom, jamais de ré-tirage (doc §1/§14). */
void toponym_world_tick(World *w, WorldEconomy *econ){
    if (!w || !econ) return;
    int nr=w->n_regions; if (nr>SCPS_MAX_REG) nr=SCPS_MAX_REG;
    bool any_missing=false;
    for (int r=0;r<nr;r++) if (!g_ville_name[r][0]){ any_missing=true; break; }
    if (!any_missing) return;

    /* eau par province : UN SEUL passage de cellules (motif refine_capitals,
     * scps_world.c) — river/lake déjà calculés au worldgen. */
    static bool has_river[SCPS_MAX_PROV], has_lake[SCPS_MAX_PROV];
    static uint8_t river_max[SCPS_MAX_PROV];
    int np=w->n_provinces; if (np>SCPS_MAX_PROV) np=SCPS_MAX_PROV;
    for (int p=0;p<np;p++){ has_river[p]=false; has_lake[p]=false; river_max[p]=0; }
    for (int i=0;i<SCPS_N;i++){
        int p=w->cell[i].province;
        if (p<0||p>=np) continue;
        uint8_t rv=w->cell[i].river;
        if (rv>76){ has_river[p]=true; if (rv>river_max[p]) river_max[p]=rv; }   /* >0.30·255, motif refine_capitals */
        if (w->cell[i].lake) has_lake[p]=true;
    }

    for (int r=0;r<nr;r++){
        if (g_ville_name[r][0]) continue;   /* déjà nommée — jamais retiré */
        const Region *rg=&w->region[r];
        if (rg->n_provinces<=0) continue;
        /* ancre : la province-CAPITALE de la région si elle en a une, sinon la
         * PREMIÈRE colonisée dans l'ordre FIXE province_ids[] — jamais un
         * pointeur dynamique (region_rep_prov[]/rep_pid[]) qui pourrait faire
         * « migrer » la ville si la démographie régionale bascule (§14). */
        int anchor=-1;
        for (int k=0;k<rg->n_provinces;k++){
            int pid=rg->province_ids[k];
            if (pid<0||pid>=np||pid>=econ->n_prov) continue;
            if (econ->prov[pid].is_capital){ anchor=pid; break; }
        }
        if (anchor<0) for (int k=0;k<rg->n_provinces;k++){
            int pid=rg->province_ids[k];
            if (pid<0||pid>=np||pid>=econ->n_prov) continue;
            if (econ->prov[pid].colonized){ anchor=pid; break; }
        }
        if (anchor<0) continue;   /* §10 : province non colonisée => aucun nom */
        toponym_generate_one(w, econ, r, anchor, has_river, has_lake, river_max);
    }
}
