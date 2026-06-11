/*
 * scps_warhost.c — la mobilisation par pays (voir scps_warhost.h)
 */
#include "scps_warhost.h"
#include "scps_tune.h"   /* Arc I1 : solde de régiment calibrable */
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define WH_BATCH_WAR    7.0f   /* paquets fabriqués/levés par an en guerre */
#define WH_BATCH_PEACE  1.5f   /* cadence d'entretien de la GARNISON en paix */
#define WH_GARRISON_UNITS 4.0f /* garnison de paix à la jauge GARDE (× LEVY_MULT) */
#define WH_ARMS_PER_UNIT 2.0f  /* armes déposées par paquet levé (→ mil_power) */

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
    long pop=0, elite=0;
    for (int p=0;p<e->n_prov;p++){ pop += e->prov[p].pop; elite += e->prov[p].pop_by_class[LAB_ELITE]; }
    long mat = pop/6 + 100;                      /* matériaux de guerre ∝ pop */
    e->stock[LR_BOIS]=mat; e->stock[LR_METAL]=mat; e->stock[LR_ARGILE]=mat;
    e->stock[LR_PIERRE]=mat; e->stock[LR_CALCAIRE]=mat; e->stock[LR_OUTILS]=mat; 
    e->stock[LR_GOLD] += mat;
    e->market.supply=1.f; e->market.price=1.f;
    return elite;
}

/* LEVER `batch` paquets dans le scratch déjà semé : piquiers (masse) + épéistes,
 * cavalerie si l'élite est là. La fabrication précède l'enrôlement (pas d'arme,
 * pas d'unité). */
static void wh_levy_batch(ArmyState *a, LaborEcon *sc, long batch, long elite){
    if (batch<=0) return;
    army_fabricate_weapon(a, sc, W_PIQUE, batch);
    army_fabricate_weapon(a, sc, W_EPEE,  batch/2 + 1);
    army_recruit(a, sc, U_PIQUIER, batch);
    army_recruit(a, sc, U_EPEISTE, batch/2 + 1);
    if (elite > 200){
        army_fabricate_weapon(a, sc, W_MONTURE_H, batch/3 + 1);
        army_recruit(a, sc, U_CAV_LOURDE, batch/3 + 1);
    }
}

/* DÉMOBILISER `n` paquets : les unités fondent (de la dernière vers la première)
 * et la pop affectée RETOURNE au pool (re-recrutable si la guerre revient). */
static void wh_shed(ArmyState *a, long n){
    for (int i=a->n_units-1; i>=0 && n>0; i--){
        long take = a->units[i].count; if (take>n) take=n;
        a->units[i].count -= take; n -= take;
        LaborClass cl = unit_def(a->units[i].type)->from;
        a->pop_by_class_in_army[cl] -= take*POP_PER_UNIT;
        if (a->pop_by_class_in_army[cl] < 0) a->pop_by_class_in_army[cl] = 0;
        if (a->units[i].count<=0){                 /* compacter : retirer l'unité vide */
            for (int j=i;j<a->n_units-1;j++) a->units[j]=a->units[j+1];
            a->n_units--;
        }
    }
}

void warhost_tick(WarHost *h, const World *w, WorldEconomy *econ,
                  const DiploState *dp, float dt){
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
              float pay = (float)u * tune_f("REGIMENT_PAY",1.5f) * econ_world_ipm(econ)
                        * (at_war?1.5f:1.f) * dt * 12.f;
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
                wh_levy_batch(&h->army[c], h->scratch, batch, elite);
            }
        } else {
            long garrison = (long)(WH_GARRISON_UNITS*LEVY_MULT[lv] + 0.5f);
            if (cur > garrison){
                wh_shed(&h->army[c], (cur - garrison + 1)/2);   /* ~moitié/an vers la garnison */
            } else if (cur < garrison){
                long batch = (long)(WH_BATCH_PEACE*LEVY_MULT[lv]*dt + 0.5f);
                long deficit = garrison - cur; if (batch>deficit) batch=deficit;
                if (batch>0){
                    long elite = seed_scratch(h->scratch, w, econ, c);
                    wh_levy_batch(&h->army[c], h->scratch, batch, elite);
                }
            }
        }
        /* déposer la force en ARMES sur la capitale → nourrit diplo_mil_power. */
        long units = warhost_units(h, c);
        int cp = w->country[c].capital_prov;
        int crr = (cp>=0 && cp<w->n_provinces) ? w->province[cp].region : -1;
        if (crr>=0 && crr<econ->n_regions && units>0)
            econ->region[crr].stock[RES_ARMS] += (float)units * WH_ARMS_PER_UNIT * dt;
    }
}
