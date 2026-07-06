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
#define WH_BATCH_PEACE  1.5f   /* cadence d'entretien de la GARNISON en paix */
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
 * ⚠ #define LOCAUX (scps_tune_list.h verrouillé par une mission parallèle) —
 * À MIGRER au registre J (SOLDE_EU4_DIV · SOLDE_FL_* · SOLDE_OVER_K), cf. carnet. */
#define SOLDE_EU4_DIV    13.0f  /* la SOLDE (l'or du drill) : entretien = prix/13 (EU4 : 12-14) */
#define SOLDE_ARMS_DIV   26.0f  /* les ARMES : rendues à la démob (pas consommées par l'entretien)
                                 * → leur part est un AMORTISSEMENT/maintenance, moitié moins lourde */
#define SOLDE_FL_FLOOR    6.0f  /* limite de force plancher (le plus petit pays entretient 6 rgt à prix plein) */
#define SOLDE_FL_PER_REG  0.5f  /* +0.5 régiment de limite par région tenue */
#define SOLDE_OVER_K      3.0f  /* au double de la limite : chaque régiment coûte ×4 (1+1×3) */
#define SOLDE_PAY_ANCHOR 90.0f  /* valeur calibrée de REGIMENT_PAY à laquelle le dial est neutre */

/* ─── AUDIT DU GOULOT D'ARMES (SCPS_ARMSDIAG, 2026-07-06) ──────────────────────────
 * Compteurs de module (jamais lus par le moteur, RAZ par warhost_init → déterminisme
 * intact) : ce que la levée VOULAIT d'armes vs ce que l'arsenal a DONNÉ vs ce que la
 * pop a permis de lever — pour trancher « goulot d'armes réel ou sain ». */
static long g_ad_want[RES_COUNT];     /* armes demandées à la levée (unités d'arme) */
static long g_ad_got [RES_COUNT];     /* armes réellement prises (arsenal) */
static long g_ad_levied[RES_COUNT];   /* paquets ×100 réellement levés (gate pop après armes) */
static long g_ad_returned[RES_COUNT]; /* armes RENDUES à la démob (wh_shed) */
void warhost_armsdiag(const long **want, const long **got, const long **levied, const long **returned){
    if (want) *want=g_ad_want; if (got) *got=g_ad_got;
    if (levied) *levied=g_ad_levied; if (returned) *returned=g_ad_returned;
}

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
static void wh_arm_unit(ArmyState *a, WorldEconomy *econ, int cid, UnitType t, long want){
    const UnitDef *d=unit_def(t); if(!d || want<=0) return;
    long got=wh_arms_take(econ, cid, t, want);
    a->weapons[d->weapon] += got;                   /* le tampon de combat, rempli depuis le macro */
    long levied=army_recruit(a, econ, cid, t, got); /* pool par classe = strates econ du pays (pool UNIFIÉ) */
    { Resource arm=unit_res_arm(t);
      if (arm!=RES_NONE) g_ad_levied[arm] += levied*POP_PER_UNIT; }   /* ARMSDIAG : le gate POP après le gate armes */
}

void warhost_init(WarHost *h){
    memset(h->army, 0, sizeof(h->army));
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){ army_init(&h->army[c]); h->levy[c]=WH_LEVY_GARDE; }
    g_human_player = -1;            /* RAZ : par défaut l'IA gère toutes les armées */
    memset(g_ad_want,0,sizeof g_ad_want); memset(g_ad_got,0,sizeof g_ad_got);   /* ARMSDIAG : RAZ par sim */
    memset(g_ad_levied,0,sizeof g_ad_levied); memset(g_ad_returned,0,sizeof g_ad_returned);
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
    if (arm!=RES_NONE && price_region>=0 && price_region<econ->n_regions)
        arms = (float)POP_PER_UNIT * econ->region[price_region].price[arm];
    /* or/13 (la solde du drill, EU4) + armes/26 (amortissement : elles sont RENDUES à la démob). */
    return gold/SOLDE_EU4_DIV + arms/SOLDE_ARMS_DIV;
}
/* LA LIMITE DE FORCE d'un pays (lecture EU4) : combien de régiments il entretient à
 * prix plein — plancher + 2.5/région. Public (UI/diag : « n/limite »). */
float warhost_force_limit(int n_regions){
    return SOLDE_FL_FLOOR + SOLDE_FL_PER_REG*(float)(n_regions>0?n_regions:0);
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
    if (econ) for (int r=0;r<econ->n_regions;r++)
        if (econ->region[r].owner==cid) elite += (long)econ->region[r].strata[CLASS_ELITE].pop;
    return elite;
}

/* LEVER `batch` paquets dans le scratch déjà semé, en COMPOSANT selon l'ÉTHOS du pays
 * (AFF × distribution de factions). La levée remplit le tampon de combat depuis les armes
 * MACRO (RES_ARMS_*). Gatée par la TECH (unité non débloquée → poids 0) et l'ÉLITE (pas
 * d'élite → pas d'unité d'élite). Plancher conventionnel si l'éthos ne désigne rien de
 * recrutable (Transgresseur sans arcane, p.ex.) : jamais d'armée vide. Chaque type tire SA
 * catégorie d'armes → fabrique spécialisée → FER (la demande diverse, la preuve F8). */
static void wh_levy_batch(ArmyState *a, WorldEconomy *econ, const World *w,
                          const TechState *t, int cid, long batch, long elite){
    if (batch<=0) return;
    float fw[FAC_COUNT];
    country_faction_weights(w, econ, cid, fw);
    float target[U_COUNT], sum=0.f;
    for (int u=0; u<U_COUNT; u++){
        float v=0.f;
        for (int f=0; f<FAC_COUNT; f++) v += fw[f]*AFF[f][u];
        if (!unit_recruitable(t,(UnitType)u))                      v=0.f;   /* tech absente → 0 */
        if (unit_def((UnitType)u)->from==LAB_ELITE && elite<=200)  v=0.f;   /* pas d'élite → 0 */
        target[u]=v; sum+=v;
    }
    if (sum<=0.f){ target[U_PIQUIER]=2.f; target[U_EPEISTE]=1.f; target[U_ARCHER]=1.f; sum=4.f; } /* plancher */
    long placed=0;
    for (int u=0; u<U_COUNT; u++){
        if (target[u]<=0.f) continue;
        long n=(long)((target[u]/sum)*(float)batch + 0.5f);
        if (n<=0) continue;
        wh_arm_unit(a, econ, cid, (UnitType)u, n);
        placed+=n;
    }
    if (placed<=0) wh_arm_unit(a, econ, cid, U_PIQUIER, batch);   /* garde-fou ultime */
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
        if (arm!=RES_NONE && econ) for (int r=0;r<econ->n_regions;r++) if (econ->region[r].owner==cid){
            /* RE-KEY : rendues aux PROVINCES (la vue seule s'évaporait à la clôture). */
            econ_region_stock_add(econ, r, arm, (float)(take*POP_PER_UNIT));
            g_ad_returned[arm] += take*POP_PER_UNIT;   /* ARMSDIAG */
            break; }   /* armes rendues au stock */
        if (a->units[i].count<=0){                 /* compacter : retirer l'unité vide */
            for (int j=i;j<a->n_units-1;j++) a->units[j]=a->units[j+1];
            a->n_units--;
        }
    }
}

/* ACTION JOUEUR — lever `packs` paquets d'un TYPE choisi (le verbe que l'IA n'a pas :
 * elle compose par AFF). Mêmes gates que la levée : tech (unit_recruitable), classe
 * (élite ⇒ pop d'élite requise), et ARMES en stock macro (consommées). La pop est
 * affectée (pas retirée du pool). Renvoie les paquets RÉELLEMENT levés (0 si gate). */
long warhost_player_recruit(WarHost *h, const World *w, WorldEconomy *econ,
                            const TechState *ts, int cid, UnitType t, long packs){
    if (!h || !econ || cid<0 || cid>=SCPS_MAX_COUNTRY || packs<=0) return 0;
    if (!unit_recruitable(ts, t)) return 0;
    long elite = wh_country_elite(econ, cid); (void)w;   /* w : réservé (plus de semis labor transitoire) */
    const UnitDef *d = unit_def(t);
    if (d && d->from==LAB_ELITE && elite<=200) return 0;
    long before = warhost_units(h, cid);
    wh_arm_unit(&h->army[cid], econ, cid, t, packs);
    return warhost_units(h, cid) - before;
}

void warhost_tick(WarHost *h, const World *w, WorldEconomy *econ,
                  const DiploState *dp, const TechState *ts, float dt){
    if (!h || !econ || dt<=0.f) return;
    for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
        if (w->country[c].role==POLITY_UNCLAIMED || w->country[c].capital_prov<0) continue;
        int nreg=0; for (int r=0;r<econ->n_regions;r++) if (econ->region[r].owner==c){ nreg++; }
        if (nreg==0) continue;
        bool at_war=false;
        for (int b=0;b<w->n_countries;b++)
            if (b!=c && diplo_status(dp,c,b)==DIPLO_WAR){ at_war=true; break; }
        /* I1 — LA SOLDE : ANCRE EU4 + LIMITE DE FORCE (mission 2026-07-06) : chaque
         * régiment coûte SON prix de recrutement/13 par mois (or typé + armes au prix
         * NATIONAL — cf. warhost_unit_pay_month) ; au-delà de la LIMITE DE FORCE du
         * pays (∝ régions), l'intendance renchérit CHAQUE régiment (frein doomstack).
         * Payée chaque tick pour TOUS les régiments → démobiliser reste une décision
         * économique. */
        { long u = warhost_units(h,c);
          int cpp = w->country[c].capital_prov;
          int crp = (cpp>=0&&cpp<w->n_provinces)?w->province[cpp].region:-1;
          /* RE-KEY PROVINCE : treasury/coercion/mil_stock sont PROVINCE-OWNED (charte
           * règle 1) — route sur la province représentative de la capitale (sinon
           * écrasé au prochain econ_tick, econ_aggregate_regions). */
          int crpp = (crp>=0&&crp<econ->n_regions)?econ_region_rep_province(econ,crp):-1;
          if (u>0 && crpp>=0 && crpp<econ->n_prov){
              /* warhost_tick est ANNUEL (dt=1 an) → ×12 : la solde est MENSUELLE. */
              /* I1 — la JAUGE renchérit la solde : pied de guerre ×1.25, levée en masse ×1.5
               * (tenir plus d'hommes sous les armes coûte plus que proportionnellement). */
              float lvmult = (h->levy[c]==WH_LEVY_GUERRE)?1.25f : (h->levy[c]==WH_LEVY_MASSE)?1.5f : 1.f;
              float typed_pay = 0.f;   /* Σ(count × pay_month(type)) — l'ancre EU4, prix au NATIONAL */
              for (int i=0;i<h->army[c].n_units;i++)
                  typed_pay += (float)h->army[c].units[i].count
                             * warhost_unit_pay_month(econ, crp, h->army[c].units[i].type);
              /* LIMITE DE FORCE : en-deçà ×1 ; au-delà, l'intendance mord. */
              float fl   = warhost_force_limit(nreg);
              float over = (fl>0.f)? ((float)u/fl - 1.f) : 0.f;
              float sizemult = 1.f + (over>0.f ? over*SOLDE_OVER_K : 0.f);
              /* dial global : REGIMENT_PAY/90 (registre J — neutre à 90, balayable en env). */
              float dial = tune_f("REGIMENT_PAY",1.5f)/SOLDE_PAY_ANCHOR;
              float pay = typed_pay * sizemult * dial
                        * (at_war?1.5f:1.f) * lvmult * dt * 12.f;
              float paid = fminf(pay, econ->prov[crpp].treasury);
              econ->prov[crpp].treasury -= paid;
              econ_flux_add(c, FX_SOLDE, -paid);                /* I0 : la ligne soldes */
              /* IG — LA GARDE DE BUDGET (le garde-fou anti-famine) : si la capitale ne
               * couvre plus ~3 mois de la solde (pay annuel ×0.25), on DÉGRAISSE (jauge −1)
               * — l'armée cesse de croître et fond, plutôt qu'étrangler le trésor en spirale
               * de friche. En paix seulement : on ne désarme pas sous le feu. */
              if (!at_war && pay>0.f && econ->prov[crpp].treasury < pay*0.25f && h->levy[c]>0)
                  h->levy[c] -= 1;
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
        if (c == g_human_player) continue;
        /* GUERRE = MOBILISER · PAIX = DÉMOBILISER. La guerre lève au pied de guerre
         * vers le plafond de pop (la cadence rate-limite la montée) ; la paix tend
         * vers une GARNISON ∝ jauge (le « plancher de levée ») — au-dessus on
         * dégraisse, en-dessous on complète à l'entretien. C'est ce qui fait que la
         * paix tient MOINS d'hommes sous les armes que la guerre (et la solde I1 en
         * abaisse la jauge quand le trésor ne suit plus : démobiliser PAR LE COÛT). */
        long cur = warhost_units(h,c);
        if (at_war){
            long batch = (long)(WH_BATCH_WAR*LEVY_MULT[lv]*dt + 0.5f);
            if (batch>0){
                long elite = wh_country_elite(econ, c);
                wh_levy_batch(&h->army[c], econ, w, ts?&ts[c]:NULL, c, batch, elite);
            }
        } else {
            long garrison = (long)(WH_GARRISON_UNITS*LEVY_MULT[lv] + 0.5f);
            if (cur > garrison){
                wh_shed(&h->army[c], econ, c, (cur - garrison + 1)/2);   /* ~moitié/an vers la garnison */
            } else if (cur < garrison){
                long batch = (long)(WH_BATCH_PEACE*LEVY_MULT[lv]*dt + 0.5f);
                long deficit = garrison - cur; if (batch>deficit) batch=deficit;
                if (batch>0){
                    long elite = wh_country_elite(econ, c);
                    wh_levy_batch(&h->army[c], econ, w, ts?&ts[c]:NULL, c, batch, elite);
                }
            }
        }
        /* I1 — le PRIX du régiment au RECRUTEMENT (sink d'ENTRÉE : lever des hommes coûte
         * enfin quelque chose) : payé pour les paquets NETS levés ce tick, depuis la capitale. */
        { long grown = warhost_units(h,c) - cur;
          if (grown > 0){
              int cpp2=w->country[c].capital_prov;
              int crp2=(cpp2>=0&&cpp2<w->n_provinces)?w->province[cpp2].region:-1;
              int crp2p=(crp2>=0&&crp2<econ->n_regions)?econ_region_rep_province(econ,crp2):-1;
              if (crp2p>=0 && crp2p<econ->n_prov){
                  float price = (float)grown * tune_f("REGIMENT_PRICE",12.f) * econ_world_ipm(econ);
                  float paid = fminf(price, econ->prov[crp2p].treasury);
                  econ->prov[crp2p].treasury -= paid;
                  econ_flux_add(c, FX_SOLDE, -paid);
              }
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
