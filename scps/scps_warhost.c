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

/* ───────────────────────────────────────────────────────────────────────────
 * L'ÉTHOS COMPOSE L'ARMÉE (les « intentions ») — affinité faction → unité (0-3).
 * La distribution de factions du pays (enracinée dans sa pop) pondère la RECETTE
 * de levée ; le moteur ne lit ensuite QUE les unités. Conquérir un peuple déplace
 * la distribution → l'armée dérive avec la société. Colonnes = ordre de UnitType :
 *   PIQ LAN EPE ARC ARB CVL CVH MAG HAL AQB ALC GRU
 * Lignes = ordre de EthosFaction. Motivé : l'Ordre/Légiste préfère l'arquebuse
 * (l'arme drillée et standardisée de l'arsenal d'État) ; le Conquérant la
 * cavalerie (le choc et la poursuite) ; le Marchand le tir (ne pas saigner le
 * bourgeois, tuer à distance) ; le Gardien la pique (l'enclume consacrée, haut
 * moral) ; le Transgresseur l'arcane (mage/alchimie/runes — la dette d'entropie) ;
 * le Communautaire la milice (pique + archers de village, défensif). */
static const float AFF[FAC_COUNT][U_COUNT] = {
    /* CONQUERANT    */ { 0,2,2,0,0, 3,3,0, 0,0,0,1 },
    /* MARCHAND      */ { 0,0,0,2,3, 2,0,0, 0,2,1,0 },
    /* LEGISTE       */ { 1,0,2,0,0, 0,0,0, 3,3,0,0 },
    /* GARDIEN       */ { 3,1,2,1,1, 0,0,0, 1,0,0,0 },
    /* TRANSGRESSEUR */ { 0,0,0,0,0, 0,0,3, 0,0,3,3 },
    /* COMMUNAUTAIRE */ { 3,1,1,2,1, 0,0,0, 0,0,0,0 },
};
/* garde-fou C99 : si le roster (U_COUNT) ou les factions (FAC_COUNT) changent, AFF DOIT suivre. */
typedef char aff_dims_check[(FAC_COUNT==6 && U_COUNT==12) ? 1 : -1];

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
    return econ_arms_take(econ, cid, arm, want*POP_PER_UNIT) / POP_PER_UNIT;
}
/* F6 Option B — ARMER un paquet : puise les armes MACRO (RES_ARMS_*, source) → remplit le TAMPON de
 * combat a->weapons[W_*] (que le combat lit, INCHANGÉ) → enrôle. La source du tampon bascule de la
 * fabrication LRes (absorbée) vers le marché macro où la fabrique consomme le fer. */
static void wh_arm_unit(ArmyState *a, LaborEcon *sc, WorldEconomy *econ, int cid, UnitType t, long want){
    const UnitDef *d=unit_def(t); if(!d || want<=0) return;
    long got=wh_arms_take(econ, cid, t, want);
    a->weapons[d->weapon] += got;                   /* le tampon de combat, rempli depuis le macro */
    army_recruit(a, sc, t, got);
}

void warhost_init(WarHost *h){
    memset(h->army, 0, sizeof(h->army));
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){ army_init(&h->army[c]); h->levy[c]=WH_LEVY_GARDE; }
    h->scratch = (LaborEcon*)calloc(1, sizeof(LaborEcon));
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
void warhost_free(WarHost *h){ if (h){ free(h->scratch); h->scratch=NULL; } }

long warhost_units(const WarHost *h, int cid){
    if (!h || cid<0 || cid>=SCPS_MAX_COUNTRY) return 0;
    long n=0; for (int u=0;u<h->army[cid].n_units;u++) n += h->army[cid].units[u].count;
    return n;
}
long warhost_disband(WarHost *h, int cid){
    if (!h || cid<0 || cid>=SCPS_MAX_COUNTRY) return 0;
    long n=warhost_units(h,cid);
    army_init(&h->army[cid]);          /* la réserve levée se dissout */
    h->levy[cid]=WH_LEVY_GARDE;        /* on relâche la jauge (sinon re-levée immédiate) */
    return n;
}

/* Semer le labor transitoire depuis le pays (pop par classe par province), puis
 * DOTER la capacité matérielle de guerre ∝ population (on ne tick pas le labor :
 * la mobilisation puise dans un stock de guerre, pas dans la production courante). */
static long seed_scratch(LaborEcon *e, const World *w, const WorldEconomy *econ, int cid){
    labor_seed_from_world(e, w, econ, cid);
    long elite=0;
    for (int p=0;p<e->n_prov;p++){ elite += e->prov[p].pop_by_class[LAB_ELITE]; }
    e->market.supply=1.f; e->market.price=1.f;   /* P-arc : matériau & or vivent dans le pool éco, plus ici */
    return elite;
}

/* LEVER `batch` paquets dans le scratch déjà semé, en COMPOSANT selon l'ÉTHOS du pays
 * (AFF × distribution de factions). La levée remplit le tampon de combat depuis les armes
 * MACRO (RES_ARMS_*). Gatée par la TECH (unité non débloquée → poids 0) et l'ÉLITE (pas
 * d'élite → pas d'unité d'élite). Plancher conventionnel si l'éthos ne désigne rien de
 * recrutable (Transgresseur sans arcane, p.ex.) : jamais d'armée vide. Chaque type tire SA
 * catégorie d'armes → fabrique spécialisée → FER (la demande diverse, la preuve F8). */
static void wh_levy_batch(ArmyState *a, LaborEcon *sc, WorldEconomy *econ, const World *w,
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
        wh_arm_unit(a, sc, econ, cid, (UnitType)u, n);
        placed+=n;
    }
    if (placed<=0) wh_arm_unit(a, sc, econ, cid, U_PIQUIER, batch);   /* garde-fou ultime */
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
            econ->region[r].stock[arm]+=(float)(take*POP_PER_UNIT); break; }   /* armes rendues au stock */
        if (a->units[i].count<=0){                 /* compacter : retirer l'unité vide */
            for (int j=i;j<a->n_units-1;j++) a->units[j]=a->units[j+1];
            a->n_units--;
        }
    }
}

void warhost_tick(WarHost *h, const World *w, WorldEconomy *econ,
                  const DiploState *dp, const TechState *ts, float dt){
    if (!h || !h->scratch || dt<=0.f) return;
    for (int c=0;c<w->n_countries && c<SCPS_MAX_COUNTRY;c++){
        if (w->country[c].role==POLITY_UNCLAIMED || w->country[c].capital_prov<0) continue;
        int nreg=0; for (int r=0;r<econ->n_regions;r++) if (econ->region[r].owner==c){ nreg++; }
        if (nreg==0) continue;
        bool at_war=false;
        for (int b=0;b<w->n_countries;b++)
            if (b!=c && diplo_status(dp,c,b)==DIPLO_WAR){ at_war=true; break; }
        /* I1 — LA SOLDE suit le régiment : ~10 % du prix moyen/mois × IPM, ×1.5 EN GUERRE.
         * Payée chaque tick pour TOUS les régiments → démobiliser devient une décision
         * économique (une armée de 80 rgt ≈ 100-160/mois). */
        { long u = warhost_units(h,c);
          int cpp = w->country[c].capital_prov;
          int crp = (cpp>=0&&cpp<w->n_provinces)?w->province[cpp].region:-1;
          if (u>0 && crp>=0 && crp<econ->n_regions){
              /* warhost_tick est ANNUEL (dt=1 an) → ×12 : REGIMENT_PAY est MENSUEL.
               * 80 rgt × 1.5 × 12 = 1440/an = 120/mois. */
              /* I1 — la JAUGE renchérit la solde : pied de guerre ×1.25, levée en masse ×1.5
               * (tenir plus d'hommes sous les armes coûte plus que proportionnellement). */
              float lvmult = (h->levy[c]==WH_LEVY_GUERRE)?1.25f : (h->levy[c]==WH_LEVY_MASSE)?1.5f : 1.f;
              float pay = (float)u * tune_f("REGIMENT_PAY",1.5f) * econ_world_ipm(econ)
                        * (at_war?1.5f:1.f) * lvmult * dt * 12.f;
              float paid = fminf(pay, econ->region[crp].treasury);
              econ->region[crp].treasury -= paid;
              econ_flux_add(c, FX_SOLDE, -paid);                /* I0 : la ligne soldes */
              /* IG — LA GARDE DE BUDGET (le garde-fou anti-famine) : si la capitale ne
               * couvre plus ~3 mois de la solde (pay annuel ×0.25), on DÉGRAISSE (jauge −1)
               * — l'armée cesse de croître et fond, plutôt qu'étrangler le trésor en spirale
               * de friche. En paix seulement : on ne désarme pas sous le feu. */
              if (!at_war && pay>0.f && econ->region[crp].treasury < pay*0.25f && h->levy[c]>0)
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
                RegionEconomy *cre=&econ->region[crm];
                cre->coercion = fminf(1.f, cre->coercion + 0.08f*dt);   /* le prix de la masse */
            }
        }
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
                long elite = seed_scratch(h->scratch, w, econ, c);
                wh_levy_batch(&h->army[c], h->scratch, econ, w, ts?&ts[c]:NULL, c, batch, elite);
            }
        } else {
            long garrison = (long)(WH_GARRISON_UNITS*LEVY_MULT[lv] + 0.5f);
            if (cur > garrison){
                wh_shed(&h->army[c], econ, c, (cur - garrison + 1)/2);   /* ~moitié/an vers la garnison */
            } else if (cur < garrison){
                long batch = (long)(WH_BATCH_PEACE*LEVY_MULT[lv]*dt + 0.5f);
                long deficit = garrison - cur; if (batch>deficit) batch=deficit;
                if (batch>0){
                    long elite = seed_scratch(h->scratch, w, econ, c);
                    wh_levy_batch(&h->army[c], h->scratch, econ, w, ts?&ts[c]:NULL, c, batch, elite);
                }
            }
        }
        /* I1 — le PRIX du régiment au RECRUTEMENT (sink d'ENTRÉE : lever des hommes coûte
         * enfin quelque chose) : payé pour les paquets NETS levés ce tick, depuis la capitale. */
        { long grown = warhost_units(h,c) - cur;
          if (grown > 0){
              int cpp2=w->country[c].capital_prov;
              int crp2=(cpp2>=0&&cpp2<w->n_provinces)?w->province[cpp2].region:-1;
              if (crp2>=0 && crp2<econ->n_regions){
                  float price = (float)grown * tune_f("REGIMENT_PRICE",12.f) * econ_world_ipm(econ);
                  float paid = fminf(price, econ->region[crp2].treasury);
                  econ->region[crp2].treasury -= paid;
                  econ_flux_add(c, FX_SOLDE, -paid);
              }
          } }
        /* F6 — la FORCE D'ARMÉE sur la capitale → nourrit diplo_mil_power, par un CANAL DÉDIÉ
         * (re->mil_stock) découplé du RES_ARMS économique : la levée consomme les armes du marché
         * (la demande de fer), mais la puissance militaire reflète l'ARMÉE, pas le stock résiduel. */
        long units = warhost_units(h, c);
        int cp = w->country[c].capital_prov;
        int crr = (cp>=0 && cp<w->n_provinces) ? w->province[cp].region : -1;
        if (crr>=0 && crr<econ->n_regions)
            econ->region[crr].mil_stock = (float)units * WH_ARMS_PER_UNIT;
    }
}
