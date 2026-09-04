/*
 * scps_warhost.c — la mobilisation par pays (voir scps_warhost.h)
 */
#include "scps_warhost.h"
#include "scps_tune.h"   /* Arc I1 : solde de régiment calibrable */
#include "scps_factions.h"   /* country_faction_weights : l'éthos enraciné qui compose l'armée */
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define WH_BATCH_WAR    7.0f   /* paquets fabriqués/levés par an en guerre */
#define WH_BATCH_PEACE  3.0f   /* cadence d'entretien de la GARNISON en paix (1.5→3.0 : la garnison
                                * size-proportionnelle est plus grande, il faut la rejoindre à mesure
                                * que l'empire croît — bornée au déficit, jamais de sur-levée) */
#define WH_GARRISON_UNITS 4.0f /* garnison de paix à la jauge GARDE (× LEVY_MULT) */
#define WH_ARMS_PER_UNIT 8.0f  /* F6 : force d'armée/paquet → mil_stock (calé pour retrouver l'ordre de
                                * grandeur de l'ancien stock RES_ARMS plafonné, après découplage) */
/* ─── SOLDE : L'ANCRE EU4 + LA LIMITE DE FORCE (mission 2026-07-06, raffinée) ────────
 * L'entretien mensuel d'un régiment ≈ SON PRIX DE RECRUTEMENT / 13 (EU4 : 12-14).
 * Le prix de recrutement RÉEL = l'or (REGIMENT_PRICE × unit_pay_mult — le drill/
 * l'équipement personnel suivent la complexité) + LES ARMES consommées à la levée
 * (100 armes macro/paquet, valorisées au prix NATIONAL du pays — P1 : toutes ses
 * régions partagent le prix). L'élite coûte cher à LEVER (armes chères) → cher à
 * ENTRETENIR, naturellement.
 *   solde = Σ(effectif × pay_month(type)) × sizemult × dial × guerre × jauge
 * LA LIMITE DE FORCE (lecture EU4 de la surcharge de taille) : un pays entretient
 * SOLDE_FL_FLOOR + SOLDE_FL_PER_REG×n_régions régiments À PRIX PLEIN ; au-delà,
 * l'INTENDANCE (magasins, convois, fourrage) renchérit CHAQUE régiment
 * (sizemult = 1 + dépassement_relatif × SOLDE_OVER_K) — le frein naturel au
 * doomstack devient « dépasser sa limite de force ».
 * dial = REGIMENT_PAY/90 : le tunable (registre J, calibré 90) reste le levier
 * GLOBAL de la solde — neutre à 90, balayable en env sans recompiler.
 * P2 (CALIB_ARMEE 2026-09-03) — les 5 constantes de solde ont QUITTÉ le #define local
 * pour le REGISTRE J (scps_tune_list.h) : SOLDE_EU4_DIV · SOLDE_ARMS_DIV ·
 * SOLDE_FL_FLOOR · SOLDE_FL_PER_REG · SOLDE_OVER_K. Mêmes valeurs, surchargeables. */
#define SOLDE_PAY_ANCHOR 90.0f  /* valeur calibrée de REGIMENT_PAY à laquelle le dial est neutre */
/* SOLDE_FORTUNE_ARMS (registre J) — L'UNITÉ SANS ARME PAIE QUAND MÊME SON ÉQUIPEMENT.
 * `unit_res_arm(MILICE) == RES_NONE` faisait tomber le terme « armes » de la solde à
 * ZÉRO — or ce terme porte 97-99 % du prix d'un régiment (CALIB_ARMEE §1.3-a) : la
 * milice coûtait 0.6 or/mois contre 35 pour un piquier, 59× moins cher pour une force
 * seulement 2.2× plus faible (efficacité 112 contre 4.2 — 27× la meilleure suivante).
 * Le rabais VOULU est celui de SOLDE_FORTUNE_DISC (−35 %), pas −99 %. La levée d'une
 * unité de fortune reste GRATUITE en armes (elle n'en consomme aucune) ; son ENTRETIEN
 * paie une part forfaitaire d'armement léger — piques taillées, cuir, ravitaillement.
 * 0 = ancien comportement (kill-switch). */

/* ─── AUDIT DU GOULOT D'ARMES (SCPS_ARMSDIAG, 2026-07-06) ──────────────────────────
 * Compteurs de module (jamais lus par le moteur, RAZ par warhost_init → déterminisme
 * intact) : ce que la levée VOULAIT d'armes vs ce que l'arsenal a DONNÉ vs ce que la
 * pop a permis de lever — pour trancher « goulot d'armes réel ou sain ». */
static long g_ad_want[RES_COUNT];     /* armes demandées à la levée (unités d'arme) */
static long g_ad_got [RES_COUNT];     /* armes réellement prises (arsenal) */
static long g_ad_levied[RES_COUNT];   /* paquets ×100 réellement levés (gate pop après armes) */
static long g_ad_returned[RES_COUNT]; /* armes RENDUES à la démob (wh_shed) */
void warhost_armsdiag(const long **want, const long **got, const long **levied, const long **returned){
    if (want)     *want=g_ad_want;
    if (got)      *got=g_ad_got;
    if (levied)   *levied=g_ad_levied;
    if (returned) *returned=g_ad_returned;
}

/* ─── LE FREIN ÉCONOMIQUE DE LA LEVÉE, COMPTÉ (2026-09-03, W2-4 · PRINT-ONLY) ────────
 * Deux compteurs de module, écrits par warhost_tick, JAMAIS relus par le moteur (aucune
 * décision n'en dépend, aucun flottant n'en sort côté joueur) : le sweep de validation
 * doit pouvoir dire combien l'armée a FONDU faute de solde (WH_DESERT_RATE) et combien de
 * mois-pays ont tourné au-dessus du plafond de solde (WH_PAY_REVENUE_FRAC) — sans quoi les
 * deux tunables du frein sont invérifiables. RAZ par warhost_init (par sim), comme ARMSDIAG. */
static long g_wh_deserted=0;      /* paquets ×100 partis faute de solde (Σ monde, Σ sim) */
static long g_wh_overbudget=0;    /* mois-pays où la solde dépassait WH_PAY_REVENUE_FRAC × revenu */
static long g_wh_paycheck=0;      /* mois-pays observés (le dénominateur honnête de ci-dessus) */
void warhost_braking_stats(long *deserted, long *overbudget_months, long *checked_months){
    if (deserted)          *deserted          = g_wh_deserted;
    if (overbudget_months) *overbudget_months = g_wh_overbudget;
    if (checked_months)    *checked_months    = g_wh_paycheck;
}
/* A4 — la PART DES CORPS dans la solde du dernier tick, par pays (PRINT-ONLY : la
 * chronique la lit pour dire « dont corps X or/an », le moteur ne la relit jamais).
 * RAZ par warhost_init comme les compteurs ci-dessus. */
static float g_wh_corps_share[SCPS_MAX_COUNTRY];
float warhost_corps_pay_share(int cid){
    return (cid>=0 && cid<SCPS_MAX_COUNTRY) ? g_wh_corps_share[cid] : 0.f;
}

/* ─── LA RAISON DU REFUS DE LEVÉE (2026-09-04, P3 · PRINT-ONLY) ──────────────────────
 * Trois missions se sont succédé sur « l'empire riche à 0 régiment » en devinant la cause
 * (armes ? pop ? or ?) ; on la MESURE désormais. Compteurs de module, jamais relus par le
 * moteur, RAZ par warhost_init — même contrat que ARMSDIAG et le frein. */
static signed char g_wh_reason[SCPS_MAX_COUNTRY];   /* dernier code par pays (-1 = jamais vu) */
static long g_wh_reason_cnt[WHR_COUNT];             /* pays-an par code (Σ monde, Σ sim) */
static long g_wh_elite_gated=0;   /* pays-an où le gate d'élite a rayé une unité voulue */
static long g_wh_norev=0;         /* pays-mois où le revenu fiscal est nul (plafond DÉSARMÉ) */
static long g_wh_grow_over=0;     /* pays-an où la levée a grossi une armée déjà hors limite */
static void wh_reason(int cid, int code){
    if (cid<0||cid>=SCPS_MAX_COUNTRY||code<0||code>=WHR_COUNT) return;
    g_wh_reason[cid]=(signed char)code; g_wh_reason_cnt[code]++;
}
int warhost_levy_reason(int cid){
    return (cid>=0&&cid<SCPS_MAX_COUNTRY)? g_wh_reason[cid] : -1;
}
const char *warhost_levy_reason_name(int code){
    static const char *N[WHR_COUNT]={ "levée","garnison au complet","budget","arsenal vide",
                                      "plus d'hommes","sans capitale","sans région","main du joueur" };
    return (code>=0&&code<WHR_COUNT)?N[code]:"jamais levé";
}
void warhost_levy_reason_stats(const long **par_code, long *elite_gated,
                               long *sans_revenu, long *croissance_hors_limite){
    if (par_code)               *par_code               = g_wh_reason_cnt;
    if (elite_gated)            *elite_gated            = g_wh_elite_gated;
    if (sans_revenu)            *sans_revenu            = g_wh_norev;
    if (croissance_hors_limite) *croissance_hors_limite = g_wh_grow_over;
}
/* Ce que le DERNIER appel à wh_levy_batch a obtenu, pour trancher armes vs hommes. */
static long g_lb_got=0, g_lb_levied=0, g_lb_poolcut=0;

/* ───────────────────────────────────────────────────────────────────────────
 * L'ÉTHOS COMPOSE L'ARMÉE (les « intentions ») — affinité faction → unité (0-3).
 * La distribution de factions du pays (enracinée dans sa pop) pondère la RECETTE
 * de levée ; le moteur ne lit ensuite QUE les unités. Conquérir un peuple déplace
 * la distribution → l'armée dérive avec la société. Colonnes = ordre de UnitType :
 *   PIQ LAN EPE ARC ARB CVL CVH MAG HAL AQB ALC GRU | ABL BSK LCH MIL HRC TRQ LMF GES CVC CVR
 * (les 10 nouvelles : Arbalète lourde · Berserker · Lancier de choc · Milice · Harceleur ·
 *  Traqueur · Lame franche · Garde d'escorte · Cav cuirassée · Cav de raid)
 * Lignes = ordre de EthosFaction. Motivé : l'Ordre/Légiste préfère l'arquebuse
 * (l'arme drillée et standardisée de l'arsenal d'État) ; le Conquérant la
 * cavalerie (le choc et la poursuite) ; le Marchand le tir (ne pas saigner le
 * bourgeois, tuer à distance) ; le Gardien la pique (l'enclume consacrée, haut
 * moral) ; le Transgresseur l'arcane (mage/alchimie/runes — la dette d'entropie) ;
 * le Communautaire la milice (pique + archers de village, défensif). */
static const float AFF[FAC_COUNT][U_COUNT] = {
    /*                     PIQ LAN EPE ARC ARB  CVL CVH MAG  HAL AQB ALC GRU   ABL BSK LCH MIL HRC TRQ LMF GES CVC CVR */
    /* CONQUERANT    */ { 0,2,2,0,0, 3,3,0, 0,0,0,1,   0,3,0,0,0,0,0,0,3,3 },  /* cavalerie, choc, berserker, cuirassée/raid */
    /* MARCHAND      */ { 0,0,0,2,3, 2,0,0, 0,2,1,0,   3,0,0,0,3,1,3,0,0,0 },  /* tir ; le HARCELEUR (mercenaire mobile) > traqueur */
    /* LEGISTE       */ { 1,0,2,0,0, 0,0,0, 3,3,0,0,   2,0,2,0,0,0,0,3,0,0 },  /* arme drillée : arbalète lourde, lancier de choc, garde d'escorte */
    /* GARDIEN       */ { 3,1,2,1,1, 0,0,0, 1,0,0,0,   0,0,3,1,0,0,0,2,0,0 },  /* la hampe consacrée : lancier de choc, garde d'escorte, milice */
    /* TRANSGRESSEUR */ { 0,0,0,0,0, 0,0,3, 0,0,3,3,   0,3,0,0,0,0,0,0,0,2 },  /* l'arcane + le berserker/raid transgressifs */
    /* COMMUNAUTAIRE */ { 3,1,1,2,1, 0,0,0, 0,0,0,0,   0,0,0,3,1,3,0,0,0,0 },  /* la milice ; le TRAQUEUR (chasseur de village) > harceleur */
};
/* garde-fou C99 : si le roster (U_COUNT) ou les factions (FAC_COUNT) changent, AFF DOIT suivre. */
typedef char aff_dims_check[(FAC_COUNT==6 && U_COUNT==22) ? 1 : -1];

/* lecture seule de la table AFF (UI de construction) — n'influe sur aucun calcul. */
float warhost_unit_affinity(int f, int u){
    return (f>=0 && f<FAC_COUNT && u>=0 && u<U_COUNT) ? AFF[f][u] : 0.f;
}

/* PAYS JOUEUR (main humaine) : pour lui, warhost_tick ne MOBILISE/DÉMOBILISE pas
 * tout seul — l'humain compose son armée au panneau. -1 = aucun (l'IA gère tout,
 * comme en chronique). Statique de module, remis à -1 par warhost_init (donc la
 * chronique, qui n'appelle jamais le setter, garde le comportement IA → déterminisme
 * inchangé). Il PAIE toujours la solde (l'armée coûte). */
static int g_human_player = -1;
void warhost_set_human(int cid){ g_human_player = cid; }

static void wh_shed(ArmyState *a, WorldEconomy *econ, int cid, long n);   /* déclarée plus bas, utilisée par warhost_disband (LOT 2) */

/* unit_res_arm (la catégorie d'arme macro d'une unité) vit dans scps_army.c — un seul point de
 * vérité, partagé entre le warhost (levée/démob) et le campaign (renfort). */
/* F6 (Option B) — CONSOMME les armes MACRO (RES_ARMS_*, le marché économique où vit le prix du fer)
 * du stock de l'empire (100/paquet, région par région) et RENVOIE le nombre de paquets QU'ON PEUT
 * lever (plafonné par le stock — macro nul → 0, pas de levée). La levée tire la demande d'armes →
 * de fer via les fabriques (F2) : LA preuve F8. */
static long wh_arms_take(WorldEconomy *econ, int cid, UnitType t, long want){
    if (want<=0) return 0;
    Resource arm=unit_res_arm(t);
    if (arm==RES_NONE) return want;                 /* pas de catégorie → pas de gate (sécurité) */
    long got_arms = econ_arms_take(econ, cid, arm, want*POP_PER_UNIT);
    g_ad_want[arm] += want*POP_PER_UNIT; g_ad_got[arm] += got_arms;   /* ARMSDIAG (jamais lu par le moteur) */
    return got_arms / POP_PER_UNIT;
}
/* F6 Option B — ARMER un paquet : puise les armes MACRO (RES_ARMS_*, source) → remplit le TAMPON de
 * combat a->weapons[W_*] (que le combat lit, INCHANGÉ) → enrôle. La source du tampon bascule de la
 * fabrication LRes (absorbée) vers le marché macro où la fabrique consomme le fer. */
static void wh_arm_unit(ArmyState *a, WorldEconomy *econ, int cid, UnitType t, long want,
                        const long deployed[LAB_CLASS_COUNT]){
    const UnitDef *d=unit_def(t); if(!d || want<=0) return;
    /* NE DEMANDER À L'ARSENAL QUE CE QUE LA CLASSE PEUT ARMER (2026-09-04, sweep W1/W2 P3).
     * `army_recruit_ex` est TOUT-OU-RIEN : demander 5 paquets quand la classe n'en porte
     * que 4 rend 0 — et les armes, DÉJÀ prélevées au stock national, restent dans le
     * tampon de combat sans un homme dessus. Année après année l'arsenal se vidait dans un
     * tampon mort pendant que le pays restait à 0 régiment (« plus d'hommes » : 459 pays-an
     * sur 60 ans, graine 512). On borne la DEMANDE au pool : la levée devient PARTIELLE.
     * C'est de la comptabilité, pas un plafond — la part mobilisable est déjà ARMY_POOL_FRAC.
     * 0 = ancien comportement (kill-switch). */
    if (tune_f("WH_POOL_CLAMP",1.f)>0.f){
        long free_pk = army_class_free_ex(a, econ, cid, d->from,
                                          deployed?deployed[d->from]:0) / POP_PER_UNIT;
        if (free_pk<0) free_pk=0;
        if (want>free_pk) want=free_pk;
        if (want<=0){ g_lb_poolcut++; return; }   /* P3 : print-only */
    }
    long got=wh_arms_take(econ, cid, t, want);
    a->weapons[d->weapon] += got;                   /* le tampon de combat, rempli depuis le macro */
    /* pool par classe = strates PROVINCE du pays × part mobilisable, MOINS ce que le host
     * porte ET ce que les corps portent au front (`deployed`) — cf. army_class_free_ex. */
    long levied=army_recruit_ex(a, econ, cid, t, got, deployed?deployed[d->from]:0);
    g_lb_got += got; g_lb_levied += levied;   /* P3 (print-only) : armes obtenues vs hommes levés */
    { Resource arm=unit_res_arm(t);
      if (arm!=RES_NONE) g_ad_levied[arm] += levied*POP_PER_UNIT; }   /* ARMSDIAG : le gate POP après le gate armes */
}

void warhost_init(WarHost *h){
    memset(h->army, 0, sizeof(h->army));
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){ army_init(&h->army[c]); h->levy[c]=WH_LEVY_GARDE; }
    g_human_player = -1;            /* RAZ : par défaut l'IA gère toutes les armées */
    memset(g_ad_want,0,sizeof g_ad_want); memset(g_ad_got,0,sizeof g_ad_got);   /* ARMSDIAG : RAZ par sim */
    memset(g_ad_levied,0,sizeof g_ad_levied); memset(g_ad_returned,0,sizeof g_ad_returned);
    g_wh_deserted=0; g_wh_overbudget=0; g_wh_paycheck=0;   /* FREIN (print-only) : RAZ par sim */
    memset(g_wh_reason_cnt,0,sizeof g_wh_reason_cnt);      /* RAISON DE REFUS (print-only) : RAZ par sim */
    for (int c=0;c<SCPS_MAX_COUNTRY;c++) g_wh_reason[c]=-1;
    g_wh_elite_gated=0; g_wh_norev=0; g_wh_grow_over=0;
    g_lb_got=0; g_lb_levied=0; g_lb_poolcut=0;
    memset(g_wh_corps_share,0,sizeof g_wh_corps_share);    /* A4 part des corps (print-only) */
}
/* Jauge de levée (sidebar §5) : un palier, pas un float. */
void warhost_set_levy(WarHost *h, int cid, int levy){
    if (!h || cid<0 || cid>=SCPS_MAX_COUNTRY) return;
    if (levy<WH_LEVY_BASSE) levy=WH_LEVY_BASSE;
    if (levy>WH_LEVY_MASSE) levy=WH_LEVY_MASSE;
    h->levy[cid]=levy;
}
int warhost_levy(const WarHost *h, int cid){
    return (h && cid>=0 && cid<SCPS_MAX_COUNTRY) ? h->levy[cid] : WH_LEVY_GARDE;
}
const char *warhost_levy_name(int levy){
    static const char *N[4]={ "levée basse","garde","pied de guerre","levée en masse" };
    return (levy>=0&&levy<4)?N[levy]:"?";
}
void warhost_free(WarHost *h){ (void)h; /* plus de scratch à libérer : l'adaptateur LaborEcon a disparu */ }

long warhost_units(const WarHost *h, int cid){
    if (!h || cid<0 || cid>=SCPS_MAX_COUNTRY) return 0;
    long n=0; for (int u=0;u<h->army[cid].n_units;u++) n += h->army[cid].units[u].count;
    return n;
}

/* L'ANCRE EU4 — l'entretien mensuel d'UN régiment = son prix de recrutement / 13 :
 * or (REGIMENT_PRICE × unit_pay_mult × IPM) + armes consommées à la levée (100 armes
 * macro au prix de `price_region` — passer la région-capitale : prix NATIONAL P1).
 * Public : chronicle (EARLYDIAG) et l'UI lisent LE MÊME prix que le moteur paie. */
float warhost_unit_pay_month(const WorldEconomy *econ, int price_region, UnitType t){
    if (!econ) return 0.f;
    float gold = tune_f("REGIMENT_PRICE",12.f) * unit_pay_mult(t) * econ_world_ipm(econ);
    float arms = 0.f;
    Resource arm = unit_res_arm(t);
    if (price_region>=0 && price_region<econ->n_regions){
        if (arm!=RES_NONE) arms = (float)POP_PER_UNIT * econ->region[price_region].price[arm];
        else               arms = (float)POP_PER_UNIT * econ->region[price_region].price[RES_ARMS_LIGHT]
                                  * tune_f("SOLDE_FORTUNE_ARMS",0.25f);   /* l'unité de fortune paie SA part */
    }
    /* or/13 (la solde du drill, EU4) + armes/26 (amortissement : elles sont RENDUES à la démob). */
    return gold/tune_f("SOLDE_EU4_DIV",13.f) + arms/tune_f("SOLDE_ARMS_DIV",26.f);
}
/* LA LIMITE DE FORCE d'un pays (lecture EU4) : combien de régiments il entretient à
 * prix plein — plancher + 2.5/région. Public (UI/diag : « n/limite »). */
float warhost_force_limit(int n_regions){
    return tune_f("SOLDE_FL_FLOOR",6.f) + tune_f("SOLDE_FL_PER_REG",0.7f)*(float)(n_regions>0?n_regions:0);
}
long warhost_disband(WarHost *h, WorldEconomy *econ, int cid){
    if (!h || cid<0 || cid>=SCPS_MAX_COUNTRY) return 0;
    long n=warhost_units(h,cid);
    /* LOT 2 — ALIGNÉ SUR wh_shed (le downsizing naturel de paix) : on fond TOUTE la
     * réserve par le MÊME mécanisme (armes rendues au stock macro, pop libérée du
     * pool_by_class_in_army) plutôt qu'un memset qui perdait le fer déjà dépensé à
     * la levée. `wh_shed(...,n)` fond exactement `n` paquets = toute l'armée. */
    if (n>0) wh_shed(&h->army[cid], econ, cid, n);
    army_init(&h->army[cid]);          /* purge le reliquat (arrondis/unités vides) */
    h->levy[cid]=WH_LEVY_GARDE;        /* on relâche la jauge (sinon re-levée immédiate) */
    return n;
}

/* L'ÉLITE du pays = Σ des strates econ de ses régions — le gate des unités d'élite
 * (cavalerie lourde, mages…). La levée LIT désormais la pop UNIQUE (strates econ) ;
 * plus de labor transitoire à semer (l'adaptateur LaborEcon a disparu). */
static long wh_country_elite(const WorldEconomy *econ, int cid){
    long elite=0;
    /* RE-KEY PROVINCE (CALIB_ARMEE 2026-09-03) — même correction de grain que
     * `army_class_free_ex` qu'il garde : `region[].strata[]` SOMME toutes les provinces
     * membres quel que soit LEUR propriétaire, et `region.owner` n'est que le
     * propriétaire MAJORITAIRE — le gate comptait donc l'aristocratie adverse des
     * régions mixtes. Aucune fraction ici : ce n'est pas le pool mais un SEUIL
     * (« ce pays a-t-il une aristocratie ? »), lu tel quel contre les 200. */
    if (econ) for (int p=0;p<econ->n_prov;p++)
        if (econ->prov[p].owner==cid) elite += (long)econ->prov[p].strata[CLASS_ELITE].pop;
    return elite;
}

/* CE QUE LES CORPS DE CAMPAGNE DU PAYS PORTENT DÉJÀ, par classe (§4.2 CALIB_ARMEE) :
 * le pool de levée doit voir TOUT ce que le pays a sous les armes, pas seulement le
 * host — un corps parti emporte son affectation et le host repart de zéro. `cmp` NULL
 * (bancs sans campagne) ⇒ zéros : comptabilité du host seul, l'ancien comportement. */
static void wh_deployed(const Campaign *cmp, int cid, long out[LAB_CLASS_COUNT]){
    for (int i=0;i<LAB_CLASS_COUNT;i++)
        out[i] = cmp ? campaign_deployed_class(cmp, cid, (LaborClass)i) : 0;
}

/* LEVER `batch` paquets dans le scratch déjà semé, en COMPOSANT selon l'ÉTHOS du pays
 * (AFF × distribution de factions). La levée remplit le tampon de combat depuis les armes
 * MACRO (RES_ARMS_*). Gatée par la TECH (unité non débloquée → poids 0) et l'ÉLITE (pas
 * d'élite → pas d'unité d'élite). Plancher conventionnel si l'éthos ne désigne rien de
 * recrutable (Transgresseur sans arcane, p.ex.) : jamais d'armée vide. Chaque type tire SA
 * catégorie d'armes → fabrique spécialisée → FER (la demande diverse, la preuve F8). */
static void wh_levy_batch(ArmyState *a, WorldEconomy *econ, const World *w,
                          const TechState *t, int cid, long batch, long elite,
                          const long deployed[LAB_CLASS_COUNT]){
    if (batch<=0) return;
    g_lb_got=0; g_lb_levied=0; g_lb_poolcut=0;   /* P3 (print-only) : ce que CETTE levée obtient */
    bool elite_gated=false;      /* P3 : le gate d'aristocratie a-t-il rayé une unité voulue ? */
    float arsenal_gate = tune_f("WH_ARSENAL_GATE",1.f);
    float milice_floor = tune_f("WH_MILICE_FLOOR",1.f);
    float fw[FAC_COUNT];
    country_faction_weights(w, econ, cid, fw);
    float target[U_COUNT], sum=0.f;
    for (int u=0; u<U_COUNT; u++){
        float v=0.f;
        for (int f=0; f<FAC_COUNT; f++) v += fw[f]*AFF[f][u];
        if (!unit_recruitable(t,(UnitType)u))                      v=0.f;   /* tech absente → 0 */
        if (unit_def((UnitType)u)->from==LAB_ELITE && elite<=200){
            if (v>0.f) elite_gated=true;                                    /* P3 : print-only */
            v=0.f;   /* pas d'élite → 0 */
        }
        /* GATE ARSENAL (2026-09-04) — L'IA LÈVE CE QUE SON STOCK PERMET. Une recette que
         * l'arsenal ne peut pas armer pèse 0, exactement comme une tech absente. Sans ce
         * gate, l'éthos réclamait une catégorie d'armes que le pays n'avait pas et la levée
         * rendait ZÉRO : `essai_s11:698` — 1er empire du monde, 184 577 or, 127 329 armes
         * LOURDES et 108 922 de trait en stock, **0 régiment** (« arsenal vide » : 594
         * pays-an sur 60 ans, graine 512 — la première cause de refus, devant le pool).
         * 0 = ancien comportement (kill-switch). */
        if (arsenal_gate>0.f && v>0.f){
            Resource arm=unit_res_arm((UnitType)u);
            if (arm!=RES_NONE && econ_country_stock_sum(econ,cid,arm) < (float)POP_PER_UNIT) v=0.f;
        }
        target[u]=v; sum+=v;
    }
    /* LE PLANCHER — LE « BAN » DU PAYS SANS ARSENAL (CALIB_ARMEE §5-P4, posé 2026-09-04).
     * Le plancher conventionnel piquier/épéiste/archer est LUI AUSSI gaté sur l'arsenal :
     * un pays sans armes de ces catégories ne levait RIEN, à vie. La MILICE (armes de
     * fortune, `RES_NONE` → aucun gate d'arme) est le ban : elle perd, mais elle EXISTE.
     * Ouverte UNIQUEMENT ici, jamais dans la composition normale — §5-P4 : sinon l'IA
     * n'aurait plus de raison de lever autre chose ; son prix a été rendu honnête par P5
     * (SOLDE_FORTUNE_ARMS, efficacité 112 → 7,2). 0 = ancien plancher (kill-switch). */
    if (sum<=0.f){
        if (milice_floor>0.f){ target[U_MILICE]=1.f; sum=1.f; }
        else { target[U_PIQUIER]=2.f; target[U_EPEISTE]=1.f; target[U_ARCHER]=1.f; sum=4.f; }
    }
    long placed=0;
    for (int u=0; u<U_COUNT; u++){
        if (target[u]<=0.f) continue;
        long n=(long)((target[u]/sum)*(float)batch + 0.5f);
        if (n<=0) continue;
        wh_arm_unit(a, econ, cid, (UnitType)u, n, deployed);
        placed+=n;
    }
    if (placed<=0)   /* garde-fou ultime : le ban de fortune plutôt que rien */
        wh_arm_unit(a, econ, cid, (milice_floor>0.f)?U_MILICE:U_PIQUIER, batch, deployed);
    /* P3 (print-only) : NOMMER le refus. `got` = paquets que l'arsenal a armés (armes DU TYPE
     * voulu — un stock plein d'armes lourdes ne sert à rien à une recette d'armes légères) ;
     * `poolcut` = demandes rabotées faute d'hommes ; `levied` = paquets réellement levés. */
    if (elite_gated) g_wh_elite_gated++;
    wh_reason(cid, (g_lb_levied>0) ? WHR_LEVE
                 : (g_lb_poolcut>0 || g_lb_got>0) ? WHR_POOL : WHR_ARMES);
}

/* DÉMOBILISER `n` paquets : les unités fondent (de la dernière vers la première), la pop affectée
 * RETOURNE au pool (re-recrutable), ET les ARMES RETOURNENT au stock macro (F6, Option B) — le fer
 * dépensé à la mobilisation est RÉCUPÉRÉ au licenciement (réutilisable, pas de gaspillage) : la
 * demande de fer suit la CROISSANCE de l'armée, pas son maintien. */
static void wh_shed(ArmyState *a, WorldEconomy *econ, int cid, long n){
    for (int i=a->n_units-1; i>=0 && n>0; i--){
        long take = a->units[i].count; if (take>n) take=n;
        UnitType t = a->units[i].type;
        a->units[i].count -= take; n -= take;
        LaborClass cl = unit_def(t)->from;
        a->pop_by_class_in_army[cl] -= take*POP_PER_UNIT;
        if (a->pop_by_class_in_army[cl] < 0) a->pop_by_class_in_army[cl] = 0;
        Resource arm=unit_res_arm(t);
        if (arm!=RES_NONE && econ){
            /* STOCK NATIONAL (2026-09-03) : les armes retournent à L'entrepôt du pays —
             * plus besoin de dénicher une région propriétaire où les déposer. */
            econ_nation_stock_add(econ, cid, arm, (float)(take*POP_PER_UNIT));
            g_ad_returned[arm] += take*POP_PER_UNIT;   /* ARMSDIAG */
        }
        if (a->units[i].count<=0){                 /* compacter : retirer l'unité vide */
            for (int j=i;j<a->n_units-1;j++) a->units[j]=a->units[j+1];
            a->n_units--;
        }
    }
}

/* ── A4 : LA DÉSERTION MORD AUSSI AU FRONT (2026-09-04) ─────────────────────────────
 * Fondre `n` paquets sur les CORPS DE CAMPAGNE de `cid`, au PRORATA de leur effectif.
 * Mêmes gestes que wh_shed (c'est wh_shed qui opère, il est générique sur l'ArmyState) :
 * les hommes rentrent au pays — l'AFFECTATION se rend au registre DU CORPS
 * (`force.pop_by_class_in_army`, celui que lit campaign_deployed_class : le pool de
 * levée les revoit aussitôt, cf. 38523b6) et les armes retournent au stock national.
 * Ce n'est PAS une mort (kill_packets/dead_class_pending) : un déserteur rentre chez lui.
 * Un corps vidé quitte la carte (campaign_prune_empty). Renvoie les paquets fondus.
 * Aucun symbole de scps_campaign.c n'est référencé ici — que des inlines d'en-tête et
 * des champs publics : les dix bancs qui lient warhost.o sans campaign.o tiennent. */
static long wh_shed_corps(Campaign *cmp, WorldEconomy *econ, int cid, long n){
    if (!cmp || n<=0 || cid<0 || cid>=SCPS_MAX_COUNTRY) return 0;
    long tot = campaign_deployed_units(cmp, cid);
    if (tot<=0) return 0;
    if (n>tot) n=tot;
    long done=0;
    /* passe 1 : la part de chacun (troncature — jamais plus que ce qu'il a) */
    for (int s=0;s<CAMPAIGN_MAX_CORPS && done<n;s++){
        FieldArmy *a=&cmp->army[CAMPAIGN_CORPS_ID(cid,s)];
        if (!a->active) continue;
        long have=0;
        for (int u=0;u<a->force.n_units;u++) if (a->force.units[u].count>0) have+=a->force.units[u].count;
        if (have<=0) continue;
        long take=(n*have)/tot;
        if (take>have)   take=have;
        if (take>n-done) take=n-done;
        if (take>0){ wh_shed(&a->force, econ, cid, take); done+=take; }
    }
    /* passe 2 : le RELIQUAT d'arrondi, du premier corps encore garni au dernier */
    for (int s=0;s<CAMPAIGN_MAX_CORPS && done<n;s++){
        FieldArmy *a=&cmp->army[CAMPAIGN_CORPS_ID(cid,s)];
        if (!a->active) continue;
        long have=0;
        for (int u=0;u<a->force.n_units;u++) if (a->force.units[u].count>0) have+=a->force.units[u].count;
        if (have<=0) continue;
        long take=n-done; if (take>have) take=have;
        wh_shed(&a->force, econ, cid, take); done+=take;
    }
    campaign_prune_empty(cmp, cid);
    return done;
}

/* ACTION JOUEUR — lever `packs` paquets d'un TYPE choisi (le verbe que l'IA n'a pas :
 * elle compose par AFF). Mêmes gates que la levée : tech (unit_recruitable), classe
 * (élite ⇒ pop d'élite requise), et ARMES en stock macro (consommées). La pop est
 * affectée (pas retirée du pool). Renvoie les paquets RÉELLEMENT levés (0 si gate). */
long warhost_player_recruit(WarHost *h, const World *w, WorldEconomy *econ,
                            const TechState *ts, const Campaign *cmp,
                            int cid, UnitType t, long packs){
    if (!h || !econ || cid<0 || cid>=SCPS_MAX_COUNTRY || packs<=0) return 0;
    if (!unit_recruitable(ts, t)) return 0;
    long elite = wh_country_elite(econ, cid); (void)w;   /* w : réservé (plus de semis labor transitoire) */
    const UnitDef *d = unit_def(t);
    if (d && d->from==LAB_ELITE && elite<=200) return 0;
    long deployed[LAB_CLASS_COUNT]; wh_deployed(cmp, cid, deployed);
    long before = warhost_units(h, cid);
    wh_arm_unit(&h->army[cid], econ, cid, t, packs, deployed);
    return warhost_units(h, cid) - before;
}

void warhost_tick(WarHost *h, const World *w, WorldEconomy *econ,
                  const DiploState *dp, const TechState *ts, Campaign *cmp, float dt){
    if (!h || !econ || dt<=0.f) return;
    for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
        if (w->country[c].role==POLITY_UNCLAIMED) continue;
        if (w->country[c].capital_prov<0){ wh_reason(c, WHR_SANS_CAPITALE); continue; }   /* P3 : print-only */
        int nreg=0; for (int r=0;r<econ->n_regions;r++) if (econ->region[r].owner==c){ nreg++; }
        if (nreg==0){ wh_reason(c, WHR_SANS_REGION); continue; }   /* P3 : print-only */
        bool at_war=false;
        for (int b=0;b<w->n_countries;b++)
            if (b!=c && diplo_status(dp,c,b)==DIPLO_WAR){ at_war=true; break; }
        /* I1 — LA SOLDE : ANCRE EU4 + LIMITE DE FORCE (mission 2026-07-06) : chaque
         * régiment coûte SON prix de recrutement/13 par mois (or typé + armes au prix
         * NATIONAL — cf. warhost_unit_pay_month) ; au-delà de la LIMITE DE FORCE du
         * pays (∝ régions), l'intendance renchérit CHAQUE régiment (frein doomstack).
         * Payée chaque tick pour TOUS les régiments → démobiliser reste une décision
         * économique. */
        /* AUDIT 2026-09-02 — la GARDE DE BUDGET ne valait qu'en PAIX : à la guerre, la levée
         * ne connaissait AUCUN plafond (ni limite de force, ni trésor — `paid` est clampé au
         * trésor, donc une armée que l'État ne paie plus est GRATUITE). Un pays en guerre
         * permanente empilait 165 à 342 régiments pour une limite de force de ~11 (sweep
         * doctrines A3, les DEUX bras). On ne DÉMOBILISE pas sous le feu — on cesse de
         * GROSSIR quand le trésor ne couvre plus les ~3 mois de solde que la garde de
         * budget exige déjà en paix (même seuil, aucun nombre neuf). */
        bool pay_starved = false;
        { long u = warhost_units(h,c);
          /* A4 (2026-09-04) — UN RÉGIMENT AU FRONT N'EST PLUS GRATUIT : `campaign_order`
           * TRANSFÈRE la force au corps de campagne (LOT 1 : le host est VIDÉ), si bien
           * que la solde, qui n'itérait que `h->army[c]`, ne facturait plus rien dès que
           * l'armée partait en guerre. Un pays en guerre permanente entretenait donc une
           * armée de campagne GRATUITE que rien ne faisait fondre (sweep A2 : host 21 rgt
           * pour une limite de 7, corps 27 rgt, solde à 77 % du revenu — le frein de levée
           * avait stoppé la CROISSANCE, pas nourri l'existant). Les corps entrent dans la
           * MÊME assiette, au MÊME barème typé et sous les MÊMES multiplicateurs.
           * WH_PAY_CORPS=0 = ancien comportement (kill-switch byte-identique). */
          bool pay_corps = (cmp!=NULL) && (tune_f("WH_PAY_CORPS",1.f) > 0.f);
          long u_corps = pay_corps ? campaign_deployed_units(cmp,c) : 0;
          g_wh_corps_share[c] = 0.f;   /* PRINT-ONLY : jamais une valeur périmée d'un tick d'avant */
          int cpp = w->country[c].capital_prov;
          int crp = (cpp>=0&&cpp<w->n_provinces)?w->province[cpp].region:-1;
          /* TRÉSOR NATIONAL (2026-09-03) : la solde sort du trésor DU PAYS — plus de la
           * caisse de la seule capitale (un hégémon riche pouvait s'y trouver « ruiné »).
           * La région capitale ne sert plus qu'au PRIX (warhost_unit_pay_month) ; sa
           * province représentative ne reçoit que la RICHESSE des pops soldées. */
          int crpp = (crp>=0&&crp<econ->n_regions)?econ_region_rep_province(econ,crp):-1;
          if ((u>0 || u_corps>0) && crp>=0 && crp<econ->n_regions){
              /* warhost_tick est ANNUEL (dt=1 an) → ×12 : la solde est MENSUELLE. */
              /* I1 — la JAUGE renchérit la solde : pied de guerre ×1.25, levée en masse ×1.5
               * (tenir plus d'hommes sous les armes coûte plus que proportionnellement). */
              float lvmult = (h->levy[c]==WH_LEVY_GUERRE)?1.25f : (h->levy[c]==WH_LEVY_MASSE)?1.5f : 1.f;
              float typed_pay = 0.f;   /* Σ(count × pay_month(type)) — l'ancre EU4, prix au NATIONAL */
              for (int i=0;i<h->army[c].n_units;i++)
                  typed_pay += (float)h->army[c].units[i].count
                             * warhost_unit_pay_month(econ, crp, h->army[c].units[i].type);
              /* A4 — LES CORPS AU FRONT DANS LA MÊME ASSIETTE (même barème typé). */
              float corps_pay = 0.f;
              if (u_corps>0){
                  long ct[U_COUNT]; campaign_deployed_by_type(cmp, c, ct);
                  for (int t2=0;t2<U_COUNT;t2++)
                      if (ct[t2]>0) corps_pay += (float)ct[t2]
                                               * warhost_unit_pay_month(econ, crp, (UnitType)t2);
                  typed_pay += corps_pay;
              }
              g_wh_corps_share[c] = (typed_pay>1e-6f)? corps_pay/typed_pay : 0.f;   /* PRINT-ONLY */
              /* LIMITE DE FORCE : en-deçà ×1 ; au-delà, l'intendance mord. */
              float fl   = warhost_force_limit(nreg);
              float over = (fl>0.f)? ((float)u/fl - 1.f) : 0.f;
              float sizemult = 1.f + (over>0.f ? over*tune_f("SOLDE_OVER_K",3.f) : 0.f);
              /* dial global : REGIMENT_PAY/90 (registre J — neutre à 90, balayable en env). */
              float dial = tune_f("REGIMENT_PAY",1.5f)/SOLDE_PAY_ANCHOR;
              float base_pay = typed_pay * sizemult * dial
                             * (at_war?1.5f:1.f) * lvmult * dt * 12.f;
              float army_mult = econ_country_budget_mult(econ,c,BUDGET_ARMY);
              float pay = base_pay * army_mult;
              /* MONNAIE M14 — B1 : un trésor NÉGATIF inversait le paiement (la solde devenait
               * un revenu, la richesse des Laborers passait sous zéro). econ_nation_gold_add
               * borne le débit au disponible et rend l'appliqué : à sec, on ne paie rien et
               * on ne reçoit jamais. */
              float paid = -econ_nation_gold_add(econ, c, -pay);
              econ_flux_add(c, FX_SOLDE, -paid);                /* I0 : la ligne soldes */
              /* MONNAIE M3b-v2 — item 5 : la solde atterrit chez les LABORERS de la capitale
               * (la richesse des POPS reste PROVINCIALE, elle). */
              if (crpp>=0 && crpp<econ->n_prov)
                  econ->prov[crpp].strata[CLASS_LABORER].wealth += paid;
              /* Sous-financer la solde ne fait plus DÉSERTER (l'armée reste entière) :
               * elle perd le MORAL — la pénalité est lue au combat (scps_campaign.c,
               * army_pay_morale ← BUDGET_ARMY). Surpayer reste un choix de trésorerie,
               * sans bonus militaire artificiel. */
              /* IG — LA GARDE DE BUDGET (le garde-fou anti-famine) : si le trésor NATIONAL ne
               * couvre plus ~3 mois de la solde (pay annuel ×0.25), on DÉGRAISSE (jauge −1)
               * — l'armée cesse de croître et fond, plutôt qu'étrangler le trésor en spirale
               * de friche. En paix seulement : on ne désarme pas sous le feu. */
              /* DÉSERTION (2026-09-03 — interaction trésor NATIONAL × pool province) : la
               * solde sort désormais de l'or de TOUT l'empire, et le pool de levée compte
               * juste (corps au front, grain province) — la garde de budget « 3 mois de
               * solde » ne freinait plus rien à l'échelle d'un empire : 62 rgt pour une
               * limite de 25, trésor à sec, taxes effondrées (graine 7, an 120). Le frein
               * n'est PAS un plafond (décision joueur) : une armée que l'État ne PAIE PAS
               * FOND — la part impayée déserte au rythme WH_DESERT_RATE par an (0 = ancien
               * comportement : moral seul, l'armée impayée restait entière et gratuite). */
              /* A4 — LA DÉSERTION MORD OÙ EST L'ARMÉE : l'assiette qui déserte est celle
               * qu'on vient de FACTURER (host + corps), et les paquets fondent AU PRORATA
               * des deux — sans quoi une armée entièrement partie au front serait facturée
               * sans jamais fondre (u=0 ⇒ nd=0), et le frein resterait un mot. */
              { float unpaid = (pay>1e-3f)? 1.f - paid/pay : 0.f;
                float drate  = tune_f("WH_DESERT_RATE", 0.5f);
                long  u_tot  = u + u_corps;
                if (unpaid>0.01f && drate>0.f && u_tot>0){
                    long nd = (long)((double)u_tot * (double)unpaid * (double)drate * (double)dt + 0.5);
                    if (nd>u_tot) nd=u_tot;
                    long nd_corps = (u_corps>0)? (long)((double)nd*(double)u_corps/(double)u_tot) : 0;
                    long nd_host  = nd - nd_corps;
                    if (nd_host>u){ nd_corps += nd_host-u; nd_host=u; }   /* l'arrondi retombe au front */
                    if (nd_corps>u_corps) nd_corps=u_corps;
                    if (nd_host>0){ wh_shed(&h->army[c], econ, c, nd_host); g_wh_deserted += nd_host; }
                    if (nd_corps>0) g_wh_deserted += wh_shed_corps(cmp, econ, c, nd_corps);   /* PRINT-ONLY */
                } }
              /* BUDGET MILITAIRE SUR LE FLUX (2026-09-03) : la solde annuelle ne doit pas
               * dépasser WH_PAY_REVENUE_FRAC du revenu fiscal de l'an écoulé (cible du
               * rapport armée : 10-15 % en paix, plus sous le feu). Au-delà, on cesse de
               * GROSSIR (même mécanique que la famine de trésor) — le stock d'or, national,
               * n'est plus un juge : c'est le REVENU qui porte une armée. 0 = désactivé.
               * econ_country_tax_year rend 0 durant le bootstrap (<90 j) — REPLI depuis
               * 2026-09-04 (WH_REV_FALLBACK, ci-dessous) : un revenu nul désarmait le frein. */
              bool over_budget = false;
              { float rev  = econ_country_tax_year(c);
                /* L'ASSIETTE DU FREIN DOIT ÊTRE UN REVENU VRAI (2026-09-04, sweep W1/W2 A2/A5).
                 * `rev>0` est la condition d'ARMEMENT du plafond : quand le registre FX_TAX
                 * rend 0 — année de bootstrap, pays neuf, pays dont l'impôt n'a rien inscrit —
                 * le frein était ENTIÈREMENT DÉSARMÉ et la levée de guerre grossissait sans
                 * aucune borne (armée/limite jusqu'à 432 %, `temoin_s3:1017` ; mesuré ici :
                 * 97 pays-mois à revenu nul sur 60 ans, graine 512). On replie sur le rendement
                 * fiscal RECALCULÉ de l'état courant (`econ_country_tax_class_month` : lecteur
                 * pur, aucun accumulateur, aucun champ sérialisé) — un revenu, pas un plafond.
                 * ⚠ registre et recalcul divergent fortement dans les DEUX sens (mesuré
                 * `revenu 1194.5 / assiette 11.9` et `revenu 3287.7 / assiette 13133.6`) : le
                 * recalcul est un REPLI, jamais un plancher — c'est le registre de flux (A1,
                 * scps_econ.c) qui doit être réparé. 0 = ancien comportement (kill-switch). */
                if (rev<=0.f && tune_f("WH_REV_FALLBACK",1.f)>0.f)
                    rev = 12.f*( econ_country_tax_class_month(econ,c,CLASS_LABORER)
                               + econ_country_tax_class_month(econ,c,CLASS_BOURGEOIS)
                               + econ_country_tax_class_month(econ,c,CLASS_ELITE) );
                float frac = tune_f("WH_PAY_REVENUE_FRAC", 0.35f);
                over_budget = (rev>0.f && frac>0.f && base_pay > rev*frac);
                if (rev>0.f && frac>0.f){ g_wh_paycheck++; if (over_budget) g_wh_overbudget++; }   /* compteurs PRINT-ONLY */
                else if (frac>0.f) g_wh_norev++;   /* P3 : le plafond de revenu est DÉSARMÉ ici */ }
              pay_starved = (base_pay>0.f && ((float)econ_country_gold(econ,c) < base_pay*0.25f || over_budget));
              if (!at_war && pay_starved && h->levy[c]>0)
                  h->levy[c] -= 1;
              /* SYMÉTRIE (2026-07-06) : la jauge REMONTE quand le trésor est confortable —
               * l'ancien code ne la faisait que DESCENDRE (garde de budget), si bien que TOUT
               * empire ayant connu un mois serré dérivait vers BASSE et n'en remontait jamais
               * (→ garnison plancher, jamais la « vingtaine »). Bande d'hystérésis (0.25×
               * descend · 1.5× remonte) ; plafonnée à GARDE = le plein plafond de PAIX (les
               * pieds de guerre GUERRE/MASSE restent des choix, pas une escalade auto). */
              else if (!at_war && h->levy[c] < WH_LEVY_GARDE && !over_budget
                       && (float)econ_country_gold(econ,c) > base_pay*1.5f)
                  h->levy[c] += 1;
          } }
        /* la JAUGE DE LEVÉE module la cadence : basse 0.4× · garde 1× · guerre 1.6× ·
         * masse 2.6× — et la levée en masse FORCE LA MAIN (coercition à la capitale). */
        static const float LEVY_MULT[4]={0.4f,1.0f,1.6f,2.6f};
        int lv=h->levy[c]; if(lv<0)lv=0; if(lv>3)lv=3;
        if (lv==WH_LEVY_MASSE){
            int cpm=w->country[c].capital_prov;
            int crm=(cpm>=0&&cpm<w->n_provinces)?w->province[cpm].region:-1;
            if (crm>=0 && crm<econ->n_regions){
                int crmp=econ_region_rep_province(econ,crm);
                if (crmp>=0&&crmp<econ->n_prov)
                    econ->prov[crmp].coercion = fminf(1.f, econ->prov[crmp].coercion + 0.08f*dt);   /* le prix de la masse */
            }
        }
        /* PAYS JOUEUR : l'humain compose son armée à la main (panneau Construction) →
         * on saute la MOBILISATION/DÉMOBILISATION auto (la solde ci-dessus est déjà
         * payée : son armée coûte ; mais elle ne croît/fond plus toute seule). */
        if (c == g_human_player){ wh_reason(c, WHR_JOUEUR); continue; }   /* P3 : print-only */
        /* GUERRE = MOBILISER · PAIX = DÉMOBILISER. La guerre lève au pied de guerre
         * vers le plafond de pop (la cadence rate-limite la montée) ; la paix tend
         * vers une GARNISON ∝ jauge (le « plancher de levée ») — au-dessus on
         * dégraisse, en-dessous on complète à l'entretien. C'est ce qui fait que la
         * paix tient MOINS d'hommes sous les armes que la guerre (et la solde I1 en
         * abaisse la jauge quand le trésor ne suit plus : démobiliser PAR LE COÛT). */
        long cur = warhost_units(h,c);
        long deployed[LAB_CLASS_COUNT]; wh_deployed(cmp, c, deployed);   /* §4.2 : les corps au front comptent */
        if (at_war){
            long batch = (long)(WH_BATCH_WAR*LEVY_MULT[lv]*dt + 0.5f);
            if (batch>0 && !pay_starved){   /* AUDIT 2026-09-02 : on ne lève plus ce qu'on ne solde plus */
                long elite = wh_country_elite(econ, c);
                if (cur >= (long)warhost_force_limit(nreg)) g_wh_grow_over++;   /* P3 : print-only */
                wh_levy_batch(&h->army[c], econ, w, ts?&ts[c]:NULL, c, batch, elite, deployed);
            } else wh_reason(c, WHR_BUDGET);   /* P3 : print-only */
        } else {
            /* GARNISON DE PAIX ∝ TAILLE (2026-07-06 : « une vingtaine de rgt pour un empire
             * moyen ») — l'ancien plancher PLAT (WH_GARRISON_UNITS×LEVY_MULT = 2-4 rgt) était
             * DÉCOUPLÉ de la taille : un empire de 20 régions et 76k hab tenait les mêmes 2 rgt
             * qu'un hameau. Désormais l'armée permanente est une part de la LIMITE DE FORCE
             * (∝ régions, déjà size-proportionnelle) modulée par la JAUGE de levée : GARDE
             * (défaut de paix) = plein plafond ; la garde de budget (plus bas) fait redescendre
             * la jauge → la garnison SUIT ce que le trésor porte (démobiliser par le coût). */
            static const float PEACE_GAR_FRAC[4] = { 0.55f, 1.00f, 1.20f, 1.40f };  /* BASSE·GARDE·GUERRE·MASSE */
            long garrison = (long)(warhost_force_limit(nreg) * PEACE_GAR_FRAC[lv] + 0.5f);
            if (cur > garrison){
                wh_shed(&h->army[c], econ, c, (cur - garrison + 1)/2);   /* ~moitié/an vers la garnison */
                wh_reason(c, WHR_COMPLET);   /* P3 : print-only */
            } else if (cur < garrison){
                long batch = (long)(WH_BATCH_PEACE*LEVY_MULT[lv]*dt + 0.5f);
                long deficit = garrison - cur; if (batch>deficit) batch=deficit;
                if (batch>0){
                    long elite = wh_country_elite(econ, c);
                    wh_levy_batch(&h->army[c], econ, w, ts?&ts[c]:NULL, c, batch, elite, deployed);
                } else wh_reason(c, WHR_COMPLET);   /* P3 : print-only */
            } else wh_reason(c, WHR_COMPLET);   /* P3 : print-only */
        }
        /* I1 — le PRIX du régiment au RECRUTEMENT (sink d'ENTRÉE : lever des hommes coûte
         * enfin quelque chose) : payé pour les paquets NETS levés ce tick, depuis la capitale. */
        { long grown = warhost_units(h,c) - cur;
          if (grown > 0){
              int cpp2=w->country[c].capital_prov;
              int crp2=(cpp2>=0&&cpp2<w->n_provinces)?w->province[cpp2].region:-1;
              int crp2p=(crp2>=0&&crp2<econ->n_regions)?econ_region_rep_province(econ,crp2):-1;
              float price = (float)grown * tune_f("REGIMENT_PRICE",12.f) * econ_world_ipm(econ);
              /* TRÉSOR NATIONAL (2026-09-03) : le prix de recrutement sort du trésor DU PAYS
               * (débit borné : un trésor à sec ne paie rien et ne reçoit jamais). */
              float paid = -econ_nation_gold_add(econ, c, -price);
              econ_flux_add(c, FX_SOLDE, -paid);
              /* item 5 : le prix de recrutement → LABORERS (recruteurs/intendance) de la
               * capitale — la richesse des POPS reste PROVINCIALE. */
              if (crp2p>=0 && crp2p<econ->n_prov)
                  econ->prov[crp2p].strata[CLASS_LABORER].wealth += paid;
          } }
        /* F6 — la FORCE D'ARMÉE sur la capitale → nourrit diplo_mil_power, par un CANAL DÉDIÉ
         * (re->mil_stock) découplé du RES_ARMS économique : la levée consomme les armes du marché
         * (la demande de fer), mais la puissance militaire reflète l'ARMÉE, pas le stock résiduel. */
        long units = warhost_units(h, c);
        int cp = w->country[c].capital_prov;
        int crr = (cp>=0 && cp<w->n_provinces) ? w->province[cp].region : -1;
        if (crr>=0 && crr<econ->n_regions){
            int crrp=econ_region_rep_province(econ,crr);
            if (crrp>=0 && crrp<econ->n_prov)
                econ->prov[crrp].mil_stock = (float)units * WH_ARMS_PER_UNIT;
        }
    }
}
