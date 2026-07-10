/*
 * scps_revolt.c — la révolte incarnée : qui se soulève, combien, ce qu'il advient
 *
 * Le grief n'est plus un flottant de région : il est ANCRÉ sur un groupe. On
 * lit son déficit, on en mobilise une fraction (qui quitte le travail), on lui
 * donne une force réelle, et on tranche contre la garnison. Une sécession FAIT
 * NAÎTRE un pays ; un coup change la couronne ; une jacquerie arrache une
 * concession ; l'échec se paie en morts et en raideur.
 */
#include "scps_revolt.h"
#include "scps_tune.h"   /* Arc J : calibrage */
#include "scps_heritage.h"   /* heritage_name */
#include "scps_culture.h"   /* ethos_name (via culture nom) */
#include "scps_factions.h"  /* §5 : la tension de coup d'une faction forte aliénée */
#include "scps_labor.h"     /* capitale_* : la capacité de service (logement/services) de la région */
#include "scps_religion.h"  /* dimension FOI : religion_of_region/_of_country/set_region (hérésie) */
#include "scps_campaign.h"  /* Phase 3a : campaign_order — l'armée rebelle sur la carte */
#include "scps_math.h"      /* clampf/absf partagés */
#include "scps_army.h"      /* Phase 3a : ArmyState/army_init/army_doctrine_base + U_MILICE/U_CAV_LOURDE */
#include <stdlib.h>         /* getenv — diagnostic SCPS_REVDIAG */
#include <string.h>
#include <math.h>
#include <stdio.h>

/* ---- Déficit (poids qui somment à ~1 → résultat naturellement [0..1]) -- *
 * Le conquis fraîchement soumis (non-intégré, sous une couronne étrangère) est
 * le moteur classique de la sécession : aliénation + non-intégration pèsent lourd. */
#define W_BASKET    0.32f   /* faim + manque social (la faim domine, ci-dessous) */
#define W_TAX       0.10f   /* sur-taxe ressentie */
#define W_ALIEN     0.24f   /* aliénation : distance culturelle à la couronne */
#define W_REPRESS   0.06f   /* la botte (coercition/H) attise autant qu'elle masque */
#define W_UNINTEG   0.28f   /* le conquis/diaspora mal intégré */
#define ALIEN_NORM  6.0f    /* distance de contenu de saturation (≥6 = étranger total) */
/* ---- Mobilisation : fraction qui prend les armes ---------------------- */
#define MOBIL_K     0.62f
#define MOBIL_FLOOR 0.15f   /* sous ce déficit, on grogne mais on ne se lève pas */
#define MOBIL_CAP   0.40f   /* jamais plus de 40 % d'un groupe au combat */
/* ---- Allumage --------------------------------------------------------- */
#define IGNITE_DEFICIT 0.20f
#define MIN_REBELS     20L
/* PHASE 1 — SEUIL DE POP : une région doit peser ce minimum d'âmes pour envisager une révolte
 * (un hameau/une bourgade ne « fait » pas une insurrection). MIN_REBELS gate les COMBATTANTS
 * (post-mobilisation), pas l'éligibilité — d'où ce plancher distinct sur la pop RÉGIONALE. */
#define REVOLT_MIN_POP 3000L
/* ---- Scan : la misère SOUTENUE finit par lever une région ------------- */
#define SCAN_DEFICIT   0.48f   /* au-delà : CRISE aiguë (pas la pauvreté chronique douce) */
#define SCAN_SUSTAIN   120     /* jours de désespérance avant le soulèvement */

/* DÉDUP RÉVOLTE (Option B) : poids du repli d'agitation legacy (statecraft_agitation
 * /100) dans le `worst` — tunable (registre J, scps_tune_list.h), défaut ici. */
#define W_AGITATION_UNREST 0.20f

/* LOT H — LA RÉVOLTE SERVILE STRUCTURELLE : au-delà de ce seuil de part servile, le
 * déficit de révolte de la région monte structurellement (tunable, registre J). */
#define SLAVE_REVOLT_SHARE 0.20f
#define SLAVE_REVOLT_W     1.20f

/* SUREXTENSION : au-delà d'un seuil de régions, un empire tient mal ses marches —
 * chaque région excédentaire pousse le déficit séparatiste. Les conquêtes mal
 * digérées finissent par se détacher → de NOUVEAUX pays émergent de la démesure.
 * (Remplace l'« invariant 0 absorbé » statique par une carte politique vivante.) */
#define OVEREXT_FREE    6       /* régions « gratuites » : un empire compact tient bien */
#define OVEREXT_PER_REG 0.035f  /* déficit ajouté PAR région au-delà du seuil */
#define OVEREXT_CAP     0.45f   /* plafond du grief de surextension */

/* CAPITALE sous-équipée : poids du grief de mal-logement/mal-service dans le déficit
 * (surface d'équilibrage). Ne mord que les régions surpeuplées vs leur capacité bâtie. */
#define K_CAP_UNREST    0.30f

/* §C3 — la concession CREUSE l'institution, SANS rebond (cumulatif). */
#define C3_K_HOLLOW     0.20f   /* K_inst rongé par concession (l'ossature descend, reste basse) */
#define C3_L_HOLLOW     0.30f   /* légitimité régionale qui ploie d'un cran par concession */
/* ---- Revanchisme : subir la conquête arme le séparatisme --------------- */
#define REVANCHISM_DAYS  (10*365)  /* la blessure de la conquête (≈10 ans) */
#define REVANCHISM_MOBIL  1.45f    /* la rage gonfle les rangs rebelles */
#define REVANCHISM_REBEL  1.45f    /* … et leur ardeur au combat */
#define REVANCHISM_GARR   0.55f    /* une province hostile se garnisonne MAL */
/* ---- Classification --------------------------------------------------- */
#define SECEDE_D      2.6f   /* au-delà : nation étrangère sous la couronne */
#define SECEDE_INTEG  0.55f  /* et mal intégrée → elle veut partir */
/* ---- Grief de FOI (dimension religieuse) ------------------------------- *
 * Une province de foi DISSIDENTE (≠ foi d'État, non calmée par un Moine) gronde
 * pour sa religion : ce grief MONTE le déficit de révolte de la région → l'hérésie
 * (schisme intérieur) et le zèle (culte étranger) deviennent PROMPTS à se lever
 * (Réforme, guerres de religion), là où la seule misère économique ne suffisait pas. */
#define FAITH_UNREST  0.22f
/* Grief de foi au niveau du GROUPE : un groupe porteur d'une foi DISSIDENTE mène la
 * révolte À MESURE qu'il est MAL INTÉGRÉ (×(1−integration)) → le nouvel arrivant aigri
 * (réfugié déraciné) prend la tête, la minorité ÉTABLIE (les Juifs) reste sous le seuil. */
#define FAITH_LEAD    0.20f
/* ---- Résolution ------------------------------------------------------- */
#define REBEL_DECIDE_DAYS 90   /* le soulèvement se décide en ~un trimestre */
#define ZEAL_CLASS    1.0f
#define ZEAL_SECEDE   1.25f
#define ZEAL_COUP     3.0f     /* peu nombreux mais au cœur du pouvoir */
#define ZEAL_HERESIE  1.15f    /* schisme intérieur : la foi donne du cœur au ventre */
#define ZEAL_ZELOTE   1.30f    /* foi ÉTRANGÈRE : la guerre sainte, plus ardente encore */
#define GARR_LOYAL    0.16f    /* part de la pop LOYALE (pondérée intégration) levable */
#define GARR_H        220.f    /* chaque point de coercition bâtie = une garnison */
#define REINFORCE     8.f      /* renforts de la couronne par point de mil_power (l'empire
                                * n'est pas partout : une province lointaine se défend seule) */
#define REINFORCE_CAP 600.f    /* la couronne ne peut projeter qu'une part de son armée ici */
#define REVOLT_COOLDOWN 1095.f /* après TOUT soulèvement (maté ou apaisé), la province se tait ~3 ans */

/* §5 — COUP D'ÉTHOS : grief POLITIQUE d'un groupe ÉTABLI (intégré à la polité, pas
 * une conquête fraîche qui, elle, SÉCÈDE) dont l'éthos appartient à une faction forte
 * et ALIÉNÉE — opposée à la direction effective. Ses membres veulent SAISIR l'État
 * pour imposer leur éthos. L'éthos d'un groupe survit à l'assimilation (signature de
 * heritage + trait d'éthos), donc une minorité enracinée reste porteuse de SA faction. */
#define COUP_ETHOS_W       1.0f   /* une faction fortement aliénée peut soulever seule (motif politique) */
#define COUP_ETHOS_TRIGGER 0.18f  /* §C2 : seuil RELEVÉ 0.12→0.18 — le coup exige un grief plus net
                                   * (le 0.12 faisait tomber le couperet trop tôt → 0-ou-92). */
/* §C2 — COOLDOWN au niveau PAYS (distinct du REVOLT_COOLDOWN de PROVINCE) : après
 * qu'un coup a changé la couronne, le pays ENTIER observe un répit (le nouveau régime
 * se consolide) avant qu'un autre coup ne puisse partir — même si l'affamement persiste.
 * Casse la BOUCLE de fréquence (un empire qui re-coupait via une autre province). */
#define COUP_GRACE_DAYS 1825.f    /* ~5 ans de répit post-coup, par pays */
/* rs->coup_grace[SCPS_MAX_COUNTRY] (scps_revolt.h) — jours de répit restants, par PAYS.
 * SUR LA STRUCT (sérialisée, section RVLT) depuis le fix contrat-de-save défaut #1 : un
 * static hors-struct perdait sa valeur au reload (save/reload ≠ continuation). */
/* PHASE 1 — CD EMPIRE-WIDE DE RÉVOLTE : après TOUT soulèvement (n'importe quel type, n'importe
 * quelle région), l'empire ENTIER se tait ~5 ans. « On ne peut pas encaisser un printemps arabe
 * par jour » : une révolte à la fois par empire → fin du spam par-région ET du runaway de récursion.
 * Généralise le patron coup-only (rs->coup_grace) à TOUS les types de révolte. */
#define REVOLT_GRACE_DAYS 1825.f  /* ~5 ans de répit post-révolte, sur TOUT l'empire */
/* rs->revolt_grace[SCPS_MAX_COUNTRY] (scps_revolt.h) — jours de répit empire-wide restants,
 * par PAYS. SUR LA STRUCT depuis le fix contrat-de-save défaut #1 (idem coup_grace ci-dessus). */
/* TÉLÉMÉTRIE guerre civile (Phase 3a) : compteurs de chronique STATIQUES (hors RevoltState ⇒
 * NON sérialisés, aucun bump save ; RAZ par sim dans revolt_init, lus par chronicle.c). */
static long g_civilwars = 0;         /* soulèvements devenus une VRAIE guerre (armée rebelle déployée) */
static long g_rebel_victories = 0;   /* … remportés par les rebelles (l'armée de la couronne battue) */
long revolt_civilwar_count(void){ return g_civilwars; }
long revolt_rebel_victory_count(void){ return g_rebel_victories; }
/* PHASE 3a suite — SOUTIEN ÉTRANGER : une fois par guerre civile INCARNÉE, on tente UNE fois de
 * trouver un bailleur (rate-limit dur, pas un nag à chaque tick de 30 j). Clé = rebel_country
 * (identité stable de la guerre civile pour toute sa durée) — pure TÉLÉMÉTRIE (compteur de
 * chronique, ne gate AUCUNE décision moteur, ≠ rs->coup_grace/revolt_grace ci-dessus qui EUX
 * gatent et sont désormais sérialisés) : STATIQUE, NON sérialisé, un reload peut ré-offrir
 * une fois, sans conséquence (le vrai rate-limit anti-double-renfort vit sur rb->backing_tried,
 * SUR la struct Rebellion, sérialisé — cf. son commentaire). RAZ par sim dans revolt_init. */
static long g_backing_wars = 0;      /* télémétrie : seconds fronts ouverts par un bailleur étranger */
static long g_backing_materiel = 0;  /* … dont un renfort matériel a été envoyé à l'armée rebelle */
long revolt_backing_war_count(void){ return g_backing_wars; }
long revolt_backing_materiel_count(void){ return g_backing_materiel; }
/* LISIBILITÉ FIL (feed) — guerres civiles INCARNÉES démarrées CE SCAN : un petit
 * tampon STATIQUE (donc NON sérialisé, aucun bump save), RAZ en tête de revolt_scan,
 * APPENDU par revolt_ignite quand un pays rebelle est déployé (rebel_country>=0).
 * sim_day le lit juste après le tick pour pousser feed_push(FEED_REVOLT, rebel,
 * owner, region, 0) — le rebelle est NOMMÉ ("Rebelles de X") au lieu du silence
 * générique. Ordre d'ajout FIXE, taille BORNÉE (débordement ignoré) → déterministe.
 * NEW_CIVILWAR_CAP est déclaré dans scps_revolt.h (sim_day y dimensionne son propre
 * tableau de régions déjà rapportées). */
typedef struct { int rebel_country, owner, region; } NewCivilWar;
static NewCivilWar g_new_civilwar[NEW_CIVILWAR_CAP];
static int         g_new_civilwar_n = 0;
static void new_civilwar_reset(void){ g_new_civilwar_n = 0; }
static void new_civilwar_push(int rebel_country, int owner, int region){
    if (g_new_civilwar_n>=NEW_CIVILWAR_CAP) return;   /* débordement : silencieux, borné */
    NewCivilWar *e=&g_new_civilwar[g_new_civilwar_n++];
    e->rebel_country=rebel_country; e->owner=owner; e->region=region;
}
int revolt_new_civilwar_count(void){ return g_new_civilwar_n; }
int revolt_new_civilwar_at(int i, int *owner, int *region){
    if (i<0 || i>=g_new_civilwar_n) return -1;
    if (owner)  *owner  = g_new_civilwar[i].owner;
    if (region) *region = g_new_civilwar[i].region;
    return g_new_civilwar[i].rebel_country;
}
static float ethos_coup_boost(const PopGroup *g, EthosFaction alien_fac, float coup_tension){
    if (coup_tension<=0.f || g->diaspora || g->integration < SECEDE_INTEG) return 0.f;  /* établi, pas sécessionniste */
    float lean[FAC_COUNT]; group_ethos_lean(&g->culture, lean);
    int gf=0; for (int f=1; f<FAC_COUNT; f++) if (lean[f]>lean[gf]) gf=f;  /* la faction de ce groupe */
    return (gf==(int)alien_fac) ? COUP_ETHOS_W*coup_tension : 0.f;
}
#define CRUSH_KILL    0.55f    /* part des mobilisés tués si écrasés */

/* ---- Phase 3a — LA GUERRE CIVILE : DÉCISIVE, l'armée rebelle est un one-shot ---- *
 * L'armée rebelle n'est PAS un belligérant d'attrition : elle est vaincue UNE SEULE
 * FOIS. La PREMIÈRE bataille décisive tranche (pas d'accumulation de score sur une
 * longue guerre) :
 *   · le rebelle PERD sa bataille (armée BRISÉE/déroutée/détruite — état FieldArmy
 *     direct : broken_days>0 OU !active OU force vidée, une fois battles>0) ⇒ ÉCRASÉ
 *     IMMÉDIATEMENT (la révolte s'effondre au premier revers) ;
 *   · le rebelle GAGNE (score NETTEMENT positif après SA victoire — il a battu l'armée
 *     de la couronne et/ou tient la région : occupation → +50) ⇒ victoire du soulèvement.
 * Un plafond (WAR_MAX_DAYS) est un GARDE-FOU : si AUCUNE bataille ne s'est résolue dans
 * le délai (pas de sortie de la couronne, siège qui traîne), la révolte FIZZLE → ÉCRASÉE. */
#define REBEL_WARSCORE_WIN     8.f      /* un score NETTEMENT positif après la victoire rebelle suffit (pas +20) */
#define REBEL_WARSCORE_LOSE   -1.f      /* dès que le score passe NÉGATIF (une bataille a eu lieu, le rebelle a perdu l'échange) */
#define REBEL_WAR_MAX_DAYS   (5*365)    /* garde-fou : sans bataille résolue au bout de ~5 ans, la révolte fizzle → écrasée */

/* Friction culturelle : econ_content_dist (BASE, sans plancher de branche de
 * foi — ici la foi a son PROPRE canal de révolte, hérésie/zélote : le plancher
 * la double-compterait) + econ_ruling_culture (scps_econ.c). */
static int find_group(const ProvincePop *pp, int drift_id){
    for (int i=0;i<pp->n_groups;i++) if (pp->groups[i].drift_id==drift_id) return i;
    return -1;
}

/* Phase 3a — prototypes (définis plus bas, appelés par revolt_ignite qui précède). */
static int  spawn_rebel_polity(World *w, WorldEconomy *econ, Rebellion *rb);
static void deploy_rebel_army (DiploState *dp, struct Campaign *camp,
                               const WorldEconomy *econ, Rebellion *rb);

/* G0.2 — anti-concession-systématique : un pays ne CÈDE qu'une fois par décennie ;
 * au-delà (ou s'il n'a rien à céder), il RÉPRIME (l'écrasement tranche). */
#define CONCEDE_CD_DAYS    (10*365)  /* déjà concédé → refus pendant 10 ans */
#define CONCEDE_TREAS_FLOOR  200.f   /* … et il faut DE QUOI céder : trésor … */
#define CONCEDE_L_FLOOR        3.f   /* … OU légitimité au-dessus du plancher */
#define CONCEDE_GOLD         150.f   /* prix de l'apaisement (acheter la paix) */
/* rs->concede_cd[SCPS_MAX_COUNTRY] (scps_revolt.h) — jours avant qu'une nouvelle concession
 * soit possible, par pays. SUR LA STRUCT depuis le fix contrat-de-save défaut #1 (idem
 * coup_grace/revolt_grace ci-dessus). */

void revolt_init(RevoltState *rs){ memset(rs,0,sizeof *rs); rs->last_spawned=-1;
    /* rs->coup_grace/concede_cd/revolt_grace sont DÉJÀ à zéro par le memset ci-dessus
     * (ce sont des champs de la struct, plus des statics hors-struct) — un DÉMARRAGE
     * FRAIS les remet à plat comme avant ; un LOAD les restaure désormais À L'IDENTIQUE
     * (section RVLT), ce qui est tout le point du fix. */
    g_civilwars=0; g_rebel_victories=0;              /* télémétrie guerre civile RAZ par sim */
    g_backing_wars=0; g_backing_materiel=0;   /* soutien étranger : télémétrie RAZ par sim (le latch vit sur la Rebellion, sérialisé) */
}

void revolt_on_conquest(RevoltState *rs, int region){
    if (region>=0 && region<SCPS_MAX_REG) rs->revanchism_days[region]=(float)REVANCHISM_DAYS;
}
/* Le séparatisme post-conquête FOND avec sa fenêtre : facteur [0..1] = plein à
 * la conquête fraîche, décroissant à zéro sur ~10 ans (la durée est CÂBLÉE sur
 * l'effet : la rage de l'indépendance s'éteint à mesure que la plaie se referme). */
static inline float revanchism_factor(const RevoltState *rs, int region){
    if (region<0 || region>=SCPS_MAX_REG) return 0.f;
    return clampf(rs->revanchism_days[region] / (float)REVANCHISM_DAYS, 0.f, 1.f);
}

/* ===================================================================== */
/* LES TROIS LECTEURS PURS (déficit, mobilisation, nature)                */
/* ===================================================================== */
float revolt_group_deficit(const PopGroup *g, const ModifierStack *drift,
                           const PopCulture *crown, float food_sat, float society_sat,
                           float tax_pressure, float coercion){
    /* panier : la faim pèse double sur le manque social (un peuple affamé se lève) */
    float basket = clampf(0.70f*(1.f-food_sat) + 0.30f*(1.f-society_sat), 0.f, 1.f);
    PopCulture eff = group_culture_effective(g, drift);
    float alien = clampf(econ_content_dist(&eff, crown)/ALIEN_NORM, 0.f, 1.f);
    float tax   = clampf(tax_pressure, 0.f, 1.f);
    float repr  = clampf(coercion, 0.f, 1.f);
    float unint = clampf(1.f - g->integration, 0.f, 1.f);
    float d = W_BASKET*basket + W_TAX*tax + W_ALIEN*alien
            + W_REPRESS*repr + W_UNINTEG*unint;
    return clampf(d, 0.f, 1.f);
}

long revolt_mobilized(const PopGroup *g, float deficit){
    float frac = clampf(MOBIL_K*deficit - MOBIL_FLOOR, 0.f, MOBIL_CAP);
    long m = (long)((float)g->count * frac);
    return m;
}

RebelKind revolt_classify(const PopGroup *g, const ModifierStack *drift, const PopCulture *crown){
    PopCulture eff = group_culture_effective(g, drift);
    float alien = econ_content_dist(&eff, crown);
    bool unintegrated = (g->integration < SECEDE_INTEG);
    /* SÉCESSION d'abord : une nation conquise sur SA terre (non-diaspora) mal
     * intégrée veut l'INDÉPENDANCE — la conquête EST le grief, quelle que soit la
     * distance culturelle ; une minorité profondément étrangère le veut aussi. */
    if (!g->diaspora && unintegrated)              return REBEL_SECESSION;
    if (alien >= SECEDE_D && unintegrated)         return REBEL_SECESSION;
    /* COUP : seule l'élite ÉTABLIE (native, intégrée au royaume) vise le trône —
     * pas une élite étrangère/diaspora sous occupation (elle, c'est CLASS/SÉCESSION). */
    if (g->klass==CLASS_ELITE && !g->diaspora)     return REBEL_COUP;
    return REBEL_CLASS;                            /* sinon : on réclame, on ne part pas */
}

/* La foi PORTÉE par un groupe : un NATIF suit sa région (dissidence endogène/DÉRIVE) ;
 * une DIASPORA (réfugié/migrant/soumis/déporté) porte la foi de son foyer d'ORIGINE
 * (home_reg) — un réfugié protestant reste protestant en terre catholique. -1 = aucune. */
static int group_carried_faith(const WorldEconomy *econ, const PopGroup *g, int region){
    (void)econ; (void)region;
    int f=g->faith;   /* FOI PAR GROUPE : lecture DIRECTE — un natif de foi schismatique OU une
                       * diaspora de foi étrangère est vue individuellement (plus de proxy home_reg).
                       * Une MINORITÉ de foi dissidente dans une province majoritaire mène l'hérésie. */
    return (f>=0 && f<g_religion_count) ? f : -1;   /* borne REGISTRE : un id hors-borne (faith non
                                                     * posé, banc sans religion) = athée, pas religion 0 */
}
/* ===================================================================== */
/* ALLUMAGE — incarne le soulèvement sur le pire groupe                    */
/* ===================================================================== */
int revolt_ignite(RevoltState *rs, World *w, WorldEconomy *econ,
                  const ModifierStack *drift, DiploState *dp, struct Campaign *camp,
                  int region, float tax_pressure){
    if (region<0||region>=econ->n_regions) return -1;
    RegionEconomy *re=&econ->region[region];
    int owner=re->owner;
    if (owner<0) return -1;
    /* PHASE 1 — CD EMPIRE-WIDE : un empire qui vient de connaître une révolte (n'importe où) se
     * tait ~5 ans → UNE révolte à la fois par empire, fin du spam par-région ET de tout runaway
     * de récursion (une région écrasée ne peut plus rallumer une voisine le mois d'après). */
    if (owner<SCPS_MAX_COUNTRY && rs->revolt_grace[owner]>0.f) return -1;
    /* RE-KEY PROVINCE : re->pop est un MIROIR (copie de la province représentative) — muter
     * un groupe à travers `pp` ne toucherait QUE cette copie, effacée au prochain econ_tick.
     * On route la MUTATION (count/strata) sur la vraie province ; les LECTURES via `re`
     * (food_sat/society_sat/coercion, agrégats pop-pondérés/max) restent au grain région. */
    int pid=econ_region_rep_province(econ, region);
    if (pid<0 || pid>=econ->n_prov) return -1;
    ProvinceEconomy *pe=&econ->prov[pid];
    ProvincePop *pp=&pe->pop;
    if (pp->n_groups<=0) return -1;
    /* PHASE 1 — SEUIL DE POP : une région trop peu peuplée ne fait pas d'insurrection. */
    { long rpop=0; for (int i=0;i<pp->n_groups;i++) rpop+=pp->groups[i].count;
      if (rpop < REVOLT_MIN_POP) return -1; }
    /* un seul soulèvement vif par région (la colère couve déjà ici) */
    for (int i=0;i<rs->count;i++) if (rs->list[i].active && rs->list[i].region==region) return -1;
    const PopCulture *crown = econ_ruling_culture(w,econ,owner);

    /* §5 : la tension de coup du pays — une faction forte aliénée porte son élite. */
    float ct=0.f; EthosFaction cf=FAC_COMMUNAUTAIRE;
    ct = faction_coup_tension_c(w,econ,owner,&cf);   /* tension de coup AVEC le grief des leviers (§4) */

    /* le groupe au plus fort déficit porte le soulèvement (grief politique + de FOI compris) */
    int sfaith0=(owner<SCPS_MAX_COUNTRY)?religion_of_country(owner):-1;
    int worst=-1; float wd=0.f;
    for (int i=0;i<pp->n_groups;i++){
        PopGroup *gg=&pp->groups[i];
        float d=revolt_group_deficit(gg, drift, crown,
                                     re->food_sat, re->society_sat, tax_pressure, re->coercion)
              + ethos_coup_boost(gg, cf, ct);
        /* grief de FOI du GROUPE : un porteur de foi dissidente prend la tête À MESURE qu'il est
         * mal intégré (le réfugié aigri mène ; la minorité établie reste sous le seuil — les Juifs). */
        int gf=group_carried_faith(econ, gg, region);
        if (gf>=0 && gf!=sfaith0 && !religion_region_stabilized(region))
            d += FAITH_LEAD * (1.f - clampf(gg->integration,0.f,1.f));
        /* LOT H — la révolte SERVILE STRUCTURELLE : le MÊME terme de PART (revolt_scan)
         * doit aussi pouvoir ALLUMER, pas seulement accumuler la désespérance — sinon
         * une région servile mais autrement paisible n'ignite JAMAIS (son groupe esclave
         * a un déficit ordinaire quasi nul). Porté par le groupe SERVILE lui-même. */
        if (gg->klass==CLASS_SLAVE){
            float allpop=0.f; for (int k=0;k<pp->n_groups;k++) allpop+=(float)pp->groups[k].count;
            if (allpop>0.f){
                float share=(float)gg->count/allpop;
                float ref=tune_f("SLAVE_REVOLT_SHARE", SLAVE_REVOLT_SHARE);
                if (share>ref) d += tune_f("SLAVE_REVOLT_W", SLAVE_REVOLT_W)*(share-ref);
            }
        }
        if (d>1.f) d=1.f;
        if (d>wd){ wd=d; worst=i; }
    }
    if (worst<0 || wd<IGNITE_DEFICIT) return -1;
    PopGroup *g=&pp->groups[worst];
    /* §C2 : ce soulèvement serait-il un COUP ? Si oui ET le pays est en RÉPIT post-coup,
     * on l'étouffe (le régime fraîchement installé n'est pas renversé l'année d'après). */
    bool would_coup = (ethos_coup_boost(g, cf, ct) >= COUP_ETHOS_TRIGGER);
    if (would_coup && owner>=0 && owner<SCPS_MAX_COUNTRY && rs->coup_grace[owner]>0.f) return -1;
    long mob=revolt_mobilized(g, wd);
    { float rf=revanchism_factor(rs,region);   /* la rage grossit les rangs, ∝ fraîcheur de la conquête */
      if (rf>0.f) mob=(long)((float)mob*(1.f + (REVANCHISM_MOBIL-1.f)*rf)); }
    if (mob<MIN_REBELS) return -1;
    if (mob>g->count) mob=g->count;

    /* slot */
    int slot=-1;
    for (int i=0;i<rs->count;i++) if (!rs->list[i].active){ slot=i; break; }
    if (slot<0){ if (rs->count>=REVOLT_MAX) return -1; slot=rs->count++; }
    Rebellion *rb=&rs->list[slot];
    memset(rb,0,sizeof *rb);
    rb->active=true; rb->region=region; rb->owner=owner;
    /* §5 : si le grief POLITIQUE (faction forte aliénée) domine, c'est un COUP — la
     * faction saisit l'État pour imposer son éthos. Sinon, la nature usuelle (sécession
     * d'une nation conquise, jacquerie de classe). */
    rb->kind = would_coup ? REBEL_COUP : revolt_classify(g, drift, crown);
    /* DIMENSION FOI : une province de foi DISSIDENTE (≠ foi d'État, non calmée par un
     * Moine) se soulève POUR SA FOI — que ce fût, sans la foi, une jacquerie (CLASS) OU
     * une sécession (une marche conquise). Schisme de la MÊME racine → hérésie (Réforme
     * intérieure) ; racine ÉTRANGÈRE → zélote (guerre sainte). On ne touche PAS le COUP
     * (affaire de palais, orthogonale à la foi). La NATURE culturelle est préservée à la
     * RÉSOLUTION : un peuple étranger, vainqueur, SÉCÈDE en État coreligionnaire (révolte
     * de Hollande) ; un peuple intégré obtient la TOLÉRANCE (paix d'Augsbourg). */
    if (rb->kind==REBEL_CLASS || rb->kind==REBEL_SECESSION){
        /* La foi du GROUPE soulevé : un NATIF suit sa région (dissidence endogène, DÉRIVE) ;
         * une DIASPORA (réfugié/migrant/soumis/déporté) PORTE la foi de son foyer d'origine
         * (home_reg) — un réfugié protestant dans une région catholique reste un dissident de
         * foi. Un migrant INTÉGRÉ & paisible (les Juifs, la diaspora établie) n'atteint pas le
         * seuil d'allumage (W_UNINTEG bas ⇒ déficit sous IGNITE) : il est PERSÉCUTÉ, pas
         * rebelle. Seul le nouvel arrivant AIGRI (réfugié déraciné, pauvre) se soulève. */
        int gfaith = group_carried_faith(econ, g, region);
        int sfaith=(owner<SCPS_MAX_COUNTRY)?religion_of_country(owner):-1;
        if (gfaith>=0 && gfaith!=sfaith && !religion_region_stabilized(region))
            rb->kind = (sfaith>=0 && religion_root_of(gfaith)==religion_root_of(sfaith))
                     ? REBEL_HERESIE : REBEL_ZELOTE;
    }
    rb->heritage=g->heritage; rb->klass=g->klass;
    rb->culture=group_culture_effective(g, drift);
    rb->drift_id=g->drift_id; rb->mobilized=mob; rb->deficit=wd;
    rb->outcome=OUT_ONGOING; rb->spawned=-1;
    rb->rebel_country=-1; rb->war_days=0;   /* Phase 3a : le memset a mis 0 = pays 0 VALIDE → forcer -1 */
    rb->backing_tried=false;                /* soutien étranger : neuf → pas encore tenté (latch sérialisé v56) */

    /* CHOC ÉCONOMIQUE : les mobilisés QUITTENT la main-d'œuvre. La province perd
     * ses bras (l'atelier tourne au ralenti) — la révolte a un coût immédiat.
     * ESCLAVAGE — FUITE #4 : un groupe TENU (klass==CLASS_SLAVE) mobilisé doit sortir de
     * SA PROPRE strate (CLASS_SLAVE), jamais de CLASS_LABORER/CLASS_BOURGEOIS — sinon la
     * strate servile ne bouge pas pendant que le groupe rétrécit (groupes < strates) ET
     * qu'on ampute à tort le journalier/bourgeois libre d'une région où ce ne sont pas eux
     * qui se sont soulevés. Miroir exact de demobilize ci-dessous (retour symétrique). */
    g->count -= mob;
    float take=(float)mob;
    if (g->klass==CLASS_SLAVE){
        pe->strata[CLASS_SLAVE].pop = fmaxf(0.f, pe->strata[CLASS_SLAVE].pop - take);
    } else if (pe->strata[CLASS_LABORER].pop>=take) pe->strata[CLASS_LABORER].pop-=take;
    else { take-=pe->strata[CLASS_LABORER].pop; pe->strata[CLASS_LABORER].pop=0.f;
           pe->strata[CLASS_BOURGEOIS].pop=fmaxf(0.f, pe->strata[CLASS_BOURGEOIS].pop-take); }

    /* PHASE 1 — la révolte APPARAÎT ⇒ l'empire entre en CD empire-wide (~5 ans) : plus aucune
     * autre révolte dans ce pays avant l'expiration (une à la fois, fin du spam). */
    if (owner<SCPS_MAX_COUNTRY) rs->revolt_grace[owner] = REVOLT_GRACE_DAYS;
    rs->n_ignited++;
    if (rb->kind==REBEL_HERESIE)      rs->n_heresy++;   /* dimension foi : schisme intérieur */
    else if (rb->kind==REBEL_ZELOTE)  rs->n_zelote++;   /* dimension foi : culte étranger */

    /* Phase 3a — INCARNER LA GUERRE CIVILE : si la couche sim fournit dp/camp, on
     * fait NAÎTRE un pays rebelle + son armée (qui assiège la région et déclare la
     * guerre) → l'issue se jouera au SCORE DE GUERRE dans revolt_tick. Si aucun slot
     * de pays n'est libre (rebel_country reste -1), le tick retombe sur la résolution
     * INSTANTANÉE (repli). Sans dp/camp (bancs), pas de guerre : repli aussi. */
    if (dp && camp){
        rb->rebel_country = spawn_rebel_polity(w, econ, rb);
        if (rb->rebel_country>=0){
            deploy_rebel_army(dp, camp, econ, rb);
            new_civilwar_push(rb->rebel_country, owner, region);   /* lisibilité fil : sim_day nommera "Rebelles de X" */
        }
    }
    return slot;
}

/* ===================================================================== */
/* SCAN — la misère soutenue d'une région finit par la soulever            */
/* ===================================================================== */
void revolt_scan(RevoltState *rs, World *w, WorldEconomy *econ,
                 const ModifierStack *drift, const Statecraft *sc,
                 DiploState *dp, struct Campaign *camp, int days){
    new_civilwar_reset();   /* lisibilité fil : RAZ des guerres civiles incarnées CE scan */
    /* §5 : tension de coup PAR PAYS (faction forte aliénée) — calculée à la demande,
     * mise en cache (un pays a la même tension dans toutes ses régions ce tick). */
    float ctens[SCPS_MAX_COUNTRY]; EthosFaction cfac[SCPS_MAX_COUNTRY];
    char  cdone[SCPS_MAX_COUNTRY]; memset(cdone,0,sizeof cdone);
    /* §C2 : le répit post-coup s'écoule (par pays). */
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){ if (rs->coup_grace[c]>0.f) rs->coup_grace[c]-=(float)days;
                                          if (rs->concede_cd[c]>0.f) rs->concede_cd[c]-=(float)days;      /* G0.2 */
                                          if (rs->revolt_grace[c]>0.f) rs->revolt_grace[c]-=(float)days; } /* PHASE 1 : CD empire-wide */
    /* SUREXTENSION : on compte les régions par pays UNE fois (cache O(n)). */
    int owned[SCPS_MAX_COUNTRY]; memset(owned,0,sizeof owned);
    for (int r=0;r<econ->n_regions;r++){ int o=econ->region[r].owner;
        if (o>=0 && o<SCPS_MAX_COUNTRY) owned[o]++; }
    for (int r=0;r<econ->n_regions && r<SCPS_MAX_REG;r++){
        RegionEconomy *re=&econ->region[r];
        if (rs->revanchism_days[r]>0.f) rs->revanchism_days[r]=fmaxf(0.f, rs->revanchism_days[r]-(float)days);
        /* RE-KEY PROVINCE : .pop est PROVINCE-OWNED — econ->region[r].pop n'est qu'un miroir
         * de la province représentative, réécrit à chaque econ_tick (donc potentiellement VIDE
         * ou périmé selon l'ordre des tick). On route sur la MÊME province que revolt_ignite
         * mobilisera si ce scan déclenche (cohérence : le scan doit juger le groupe qui sera
         * réellement soulevé, pas une diversité que l'ignition ne verra jamais). */
        int rpid=econ_region_rep_province(econ, r);
        ProvinceEconomy *pe=(rpid>=0 && rpid<econ->n_prov) ? &econ->prov[rpid] : NULL;
        if (re->owner<0 || !re->culture.settled || !pe || pe->pop.n_groups<=0){
            rs->desperation_days[r]=0.f; rs->revolt_cooldown[r]=0.f; continue;
        }
        const PopCulture *crown=econ_ruling_culture(w,econ,re->owner);
        int o=re->owner; float ct=0.f; EthosFaction cf=FAC_COMMUNAUTAIRE;
        if (o>=0 && o<SCPS_MAX_COUNTRY){
            if (!cdone[o]){ ctens[o]=faction_coup_tension_c(w,econ,o,&cfac[o]); cdone[o]=1; }
            ct=ctens[o]; cf=cfac[o];
        }
        float worst=0.f, min_integ=1.f;
        for (int i=0;i<pe->pop.n_groups;i++){
            float d=revolt_group_deficit(&pe->pop.groups[i], drift, crown,
                                         re->food_sat, re->society_sat, re->over_tax, re->coercion)
                  + ethos_coup_boost(&pe->pop.groups[i], cf, ct);   /* §5 : grief politique */
            if (d>1.f) d=1.f;
            if (d>worst) worst=d;
            if (pe->pop.groups[i].integration < min_integ) min_integ = pe->pop.groups[i].integration;
        }
        /* SUREXTENSION : un empire trop vaste tient mal ses MARCHES — le grief monte
         * avec la taille, mais surtout là où la province est MAL INTÉGRÉE (conquête
         * étrangère) → SÉCESSION (un pays naît), pas coup du cœur natif. Le cœur ne
         * subit qu'un tiers du grief (un empire homogène fragmente moins). */
        if (o>=0 && o<SCPS_MAX_COUNTRY && owned[o]>OVEREXT_FREE){
            float overext = clampf((float)(owned[o]-OVEREXT_FREE)*OVEREXT_PER_REG, 0.f, OVEREXT_CAP);
            overext *= (0.30f + 0.70f*(1.f - clampf(min_integ,0.f,1.f)));   /* biais marches étrangères */
            worst = clampf(worst + overext, 0.f, 1.f);
        }
        /* CAPITALE SOUS-ÉQUIPÉE : la pop qui dépasse la capacité de SERVICE de la
         * région (capitale tier·1000 + édifices civiques) gronde — mal-logés/mal-servis.
         * Une région qui croît SANS bâtir ses institutions devient agitée. (C3 rétrécira
         * la part capitale + K par (1−rot) : une élite capturée délivre moins.) */
        {
            long rpop = (long)(re->strata[CLASS_LABORER].pop + re->strata[CLASS_BOURGEOIS].pop
                             + re->strata[CLASS_ELITE].pop);
            if (rpop>0){
                /* le tier de capitale que la POP débloque (depuis P-arc la capitale
                 * monte gratuitement dès le déblocage ⇒ « payé » ≡ « débloqué » ;
                 * l'ancien registre E0.4 publié par le viewer est retiré). */
                int  ctier = capitale_max_tier(rpop);
                long nob   = capitale_admin_pop(ctier); if (nob>rpop) nob=rpop;
                float rot  = (o>=0 && o<SCPS_MAX_COUNTRY)? faction_capture_total(o) : 0.f;  /* §C3 */
                float serv = ((float)capitale_housing(ctier, nob)                       /* la capitale */
                            + (re->build.K_inst + re->build.savoir + re->build.faith)*700.f) /* les autres bâtiments */
                           * (1.f - rot);          /* §C3 : une élite CAPTURÉE délivre moins de service → plus d'agitation */
                float unserved = (float)rpop - serv;
                if (unserved>0.f) worst = clampf(worst + (unserved/(float)rpop)*K_CAP_UNREST, 0.f, 1.f);
            }
        }
        /* GRIEF DE FOI : une province de foi DISSIDENTE (≠ foi d'État, non calmée par
         * un Moine) gronde pour SA religion → l'hérésie/le zèle deviennent prompts. */
        {
            int rf=religion_of_region(r);
            int sf=(o>=0&&o<SCPS_MAX_COUNTRY)?religion_of_country(o):-1;
            if (rf>=0 && rf!=sf && !religion_region_stabilized(r))
                worst = clampf(worst + FAITH_UNREST, 0.f, 1.f);
        }
        /* LOT H — LA RÉVOLTE SERVILE STRUCTURELLE : le contrepoids du mécanisme H
         * (sans lui, GARDER ses esclaves est un pur profit). Au-delà de
         * SLAVE_REVOLT_SHARE (0.20 — Rome tient 30 %, pas 60), la part servile de la
         * région pousse structurellement le déficit — même FOLD que W_AGITATION_UNREST. */
        {
            float allpop = re->strata[CLASS_LABORER].pop + re->strata[CLASS_BOURGEOIS].pop
                          + re->strata[CLASS_ELITE].pop + re->strata[CLASS_SLAVE].pop;
            if (allpop>0.f){
                float share = re->strata[CLASS_SLAVE].pop / allpop;
                float ref   = tune_f("SLAVE_REVOLT_SHARE", SLAVE_REVOLT_SHARE);
                if (share>ref)
                    worst = clampf(worst + tune_f("SLAVE_REVOLT_W", SLAVE_REVOLT_W)*(share-ref), 0.f, 1.f);
            }
        }
        /* DÉDUP RÉVOLTE (Option B) : le SIGNAL d'agitation legacy de statecraft
         * (L/coercion/choc de conquête/stabilité/garnison, 0-100 lissé) replié comme
         * un grief POLITIQUE supplémentaire — le canal légitimité/coercition/culture
         * que la misère-de-groupe (faim/taxe/aliénation/répression/non-intégration)
         * ne capte pas directement. statecraft ne fire plus lui-même : c'est ICI que
         * son signal atteint encore une révolte réelle. sc peut être NULL (bancs). */
        if (sc){
            float ag = (float)statecraft_agitation(sc, r) / 100.f;
            worst = clampf(worst + tune_f("W_AGITATION_UNREST", W_AGITATION_UNREST) * ag, 0.f, 1.f);
        }
        /* TRADITIONS — le levier FRACTURE (Soudé/Factieux) : la cohésion sociale du
         * peuple AMORTIT ou AGGRAVE le grief — même FOLD que W_AGITATION_UNREST (le
         * canal sécession/révolte incarné), PAR PAYS (culture_build_for). ±1 levier ×
         * TRAD_FRACT_W=0.06 (échelle : FAITH_UNREST 0.22, OVEREXT_CAP 0.45). Soudé (−1)
         * APAISE ; complète la voie D̄→§2.4 déjà câblée (scps_prosperity.c). */
        if (o>=0){
            HeritageBuild hb = culture_build_for((uint32_t)o);
            float frl = build_leviers(&hb).fracture;
            if (frl!=0.f) worst = clampf(worst + tune_f("TRAD_FRACT_W",0.06f)*frl, 0.f, 1.f);
        }
        /* DEUX compteurs SÉPARÉS (chacun un sens UNIQUE, plus de champ à double sémantique) :
         *  · `desperation_days` = la misère SOUTENUE (≥0) : monte en crise, retombe au calme ;
         *  · `revolt_cooldown`  = le RÉPIT post-révolte (≥0, jours restants) : décrémenté par le
         *    TEMPS, il BLOQUE le ré-allumage tant qu'il court. Une province tout juste écrasée ne se
         *    rallume donc pas avant l'expiration du répit (fin de la boucle écrasement/rallumage). */
        if (rs->revolt_cooldown[r] > 0.f)
            rs->revolt_cooldown[r] = fmaxf(0.f, rs->revolt_cooldown[r] - (float)days);   /* le répit se purge */
        if (worst>=SCAN_DEFICIT || revanchism_factor(rs,r)>0.f)
            rs->desperation_days[r] += (float)days;                                       /* crise : la misère s'accumule */
        else
            rs->desperation_days[r] = fmaxf(0.f, rs->desperation_days[r]-(float)days);    /* calme : la misère retombe */
        if (rs->desperation_days[r] >= (float)SCAN_SUSTAIN && rs->revolt_cooldown[r] <= 0.f){
            if (revolt_ignite(rs, w, econ, drift, dp, camp, r, re->over_tax)>=0)
                rs->desperation_days[r]=0.f;       /* la colère s'est faite acte */
        }
    }
}

/* ===================================================================== */
/* RÉSOLUTION — la garnison contre les rebelles, puis le verdict           */
/* ===================================================================== */
/* Rend les survivants au travail (après échec/concession/coup). ESCLAVAGE — FUITE #4
 * (miroir) : `rb->klass` est figé à l'allumage (revolt_ignite) et NE bouge PAS ici —
 * SAUF si apply_rebel_victory a DÉJÀ manumité (demography_manumit_region bascule le
 * groupe survivant en CLASS_LABORER avant tout appel de demobilize sur la voie
 * victorieuse ; seule la voie CRUSHED garde rb->klass==CLASS_SLAVE). On route donc les
 * survivants vers LA STRATE QUE PORTE ENCORE LE GROUPE (relu via `gi`, pas rb->klass
 * figé) : un crush servile rend les survivants à CLASS_SLAVE, jamais à CLASS_LABORER. */
static void demobilize(WorldEconomy *econ, Rebellion *rb, long survivors){
    if (survivors<=0) return;
    /* RE-KEY PROVINCE : .pop/.strata sont PROVINCE-OWNED — route sur la représentative
     * (comme revolt_ignite qui a mobilisé ces mêmes bras). */
    int pid=econ_region_rep_province(econ, rb->region);
    if (pid<0 || pid>=econ->n_prov) return;
    ProvinceEconomy *pe=&econ->prov[pid];
    int gi=find_group(&pe->pop, rb->drift_id);
    if (gi>=0) pe->pop.groups[gi].count += survivors;
    SocialClass back_to = (gi>=0) ? pe->pop.groups[gi].klass : CLASS_LABORER;
    pe->strata[back_to].pop += (float)survivors;
}

/* Un emplacement de pays RÉUTILISABLE : un placeholder vierge (UNCLAIMED) qui ne
 * tient aucune région — la sécession s'y installe quand la table est pleine
 * (le worldgen la remplit souvent jusqu'au plafond). */
static int free_country_slot(const World *w, const WorldEconomy *econ, int avoid){
    for (int c=0;c<w->n_countries;c++){
        if (c==avoid || w->country[c].role!=POLITY_UNCLAIMED) continue;
        int held=0; for (int r=0;r<econ->n_regions;r++) if (econ->region[r].owner==c){ held=1; break; }
        if (!held) return c;
    }
    return -1;
}
/* ===================================================================== */
/* Phase 3a — INCARNER LE PAYS REBELLE + SON ARMÉE (guerre civile réelle)  */
/* ===================================================================== */
/* Dérivé de spawn_secession, MAIS : ne transfère PAS la région (la couronne la
 * garde — c'est l'ENJEU de la guerre) et ne réinitialise PAS la pop. Le pays
 * rebelle naît SANS terre ; sa capitale POINTE la région du soulèvement (repère,
 * et cible si la sécession l'emporte). Renvoie l'id, ou -1 si aucun slot libre. */
static int spawn_rebel_polity(World *w, WorldEconomy *econ, Rebellion *rb){
    if (rb->region<0 || rb->region>=w->n_regions || rb->region>=econ->n_regions) return -1;
    int nid = free_country_slot(w, econ, rb->owner);      /* un slot vierge (sinon on agrandit) */
    if (nid<0){
        if (w->n_countries>=SCPS_MAX_COUNTRY) return -1;
        nid=w->n_countries++;
    }
    Country *nc=&w->country[nid]; memset(nc,0,sizeof *nc);
    nc->role=POLITY_ANTAGONIST;                           /* un belligérant, pas encore un État terrien */
    nc->continent=w->region[rb->region].continent;
    nc->capital_prov=(w->region[rb->region].n_provinces>0)
                     ? w->region[rb->region].province_ids[0] : -1;
    nc->n_regions=0;                                       /* NE TIENT AUCUNE RÉGION (l'enjeu reste à la couronne) */
    nc->color=0xB03030u;                                   /* rouge insurrection */
    snprintf(nc->name,sizeof nc->name,"Rebelles de %s", heritage_name(rb->heritage));
    return nid;
}
/* Construit l'armée rebelle (milice ∝ mobilisés ; +cavalerie lourde si l'ÉLITE se
 * lève — un coup rallie la noblesse à cheval) et la déploie SUR la région du
 * soulèvement (qu'elle assiège : elle appartient à la couronne). DÉCLARE la guerre
 * civile. Tout est gardé : sans dp/camp/slot valide, no-op (repli instantané). */
static void deploy_rebel_army(DiploState *dp, struct Campaign *camp,
                              const WorldEconomy *econ, Rebellion *rb){
    if (!dp || !camp || rb->rebel_country<0 || rb->rebel_country>=SCPS_MAX_COUNTRY) return;
    if (rb->region<0 || rb->region>=econ->n_regions) return;
    /* la force : paquets de 100. La milice porte le gros ; l'élite soulevée (coup)
     * amène un noyau de cavalerie lourde (peu nombreux mais mordants) ; et un NOYAU DE
     * VÉTÉRANS donne de vraies dents à l'insurrection (cf. plus bas). */
    long packets = rb->mobilized / 100; if (packets<1) packets=1;
    long cav = (rb->klass==CLASS_ELITE) ? packets/4 : 0;   /* le coup : ~1/4 à cheval */
    long mil = packets - cav; if (mil<1){ mil=1; if (cav>0 && packets>1) cav=packets-mil; else cav=0; }
    /* NOYAU DE VÉTÉRANS — ADDITIF (déserteurs de la couronne, anciens soldats, meneurs
     * aguerris qui REJOIGNENT la révolte, EN PLUS de la masse paysanne) : ÉPARS mais RÉEL.
     * Prélevé sur la milice, il resterait dérisoire — l'armée rebelle est minuscule (1-2
     * paquets) et se fait ANÉANTIR d'une bataille (ws −37). Ajouté, un noyau de piquiers
     * disciplinés (piques ≫ armes de fortune) donne de vraies dents au soulèvement — assez
     * pour qu'~1 sur 20 batte la couronne, le reste toujours écrasé. */
    long vet = (long)tune_f("REBEL_VET_ADD", 2.f); if (vet<0) vet=0;
    ArmyState force; army_init(&force);
    force.doctrine = army_doctrine_base();                 /* neutre : moral_mul=1 (side_reserve valide) */
    int nu=0;
    if (mil>0 && nu<ARMY_MAX_UNITS){ force.units[nu].type=U_MILICE;     force.units[nu].count=mil; force.units[nu].moral_courant=0.f; nu++; }
    if (vet>0 && nu<ARMY_MAX_UNITS){ force.units[nu].type=U_PIQUIER;    force.units[nu].count=vet; force.units[nu].moral_courant=0.f; nu++; }
    if (cav>0 && nu<ARMY_MAX_UNITS){ force.units[nu].type=U_CAV_LOURDE; force.units[nu].count=cav; force.units[nu].moral_courant=0.f; nu++; }
    force.n_units=nu;
    if (getenv("SCPS_REVDIAG"))
        fprintf(stderr,"[REVDEPLOY] reb=%d packets=%ld mil=%ld vet=%ld cav=%ld\n", rb->rebel_country, packets, mil, vet, cav);
    /* déploie sur la région du soulèvement (from==target : campaign_order la pose
     * IDLE), puis campaign_redirect la fait ASSIÉGER (la terre est à la couronne,
     * pas au rebelle) → le siège pousse l'occupation (score) et provoque la sortie
     * de la couronne (bataille) via sim_campaign_defense. */
    if (!campaign_order(camp, econ, rb->rebel_country, rb->region, rb->region, &force)) return;
    campaign_redirect(camp, econ, dp, rb->rebel_country, rb->region);   /* → FA_SIEGE (région ennemie) */
    /* LA GUERRE CIVILE est déclarée MAINTENANT (dès l'allumage) : sans DIPLO_WAR, ni
     * le siège (diplo_occupy) ni la sortie de la couronne ne s'enclenchent. Le CB : la
     * foi pour l'hérésie/le zèle, sinon territorial (l'insurrection veut la terre). */
    CasusBelli cb = (rb->kind==REBEL_HERESIE || rb->kind==REBEL_ZELOTE) ? CB_RELIGIOUS : CB_TERRITORIAL;
    diplo_declare_war_cb(dp, rb->rebel_country, rb->owner, cb);
    g_civilwars++;   /* télémétrie : une guerre civile RÉELLE est engagée (armée rebelle sur la carte) */
}

/* ===================================================================== */
/* Phase 3a suite — SOUTIEN ÉTRANGER AUX REBELLES (géopolitique)           */
/* ===================================================================== */
/* Compte les guerres SIMULTANÉES d'un pays (capacité : on ne pioche pas un
 * bailleur déjà surétendu — même filtre que le monde §war-smoothing, à
 * l'échelle du PAYS plutôt que du monde). */
static int country_war_count(const World *w, const DiploState *dp, int c){
    int n=0;
    for (int k=0;k<w->n_countries;k++) if (k!=c && diplo_status(dp,c,k)==DIPLO_WAR) n++;
    return n;
}
/* Le pays est-il un belligérant VIVANT capable de guerre (miroir du gate IA de
 * sim_init : ni UNCLAIMED ni WILD — les Peuples Libres sont PASSIFS, cf. charte). */
static bool country_war_capable(const World *w, int c){
    if (c<0 || c>=w->n_countries) return false;
    PolityRole role = w->country[c].role;
    return role!=POLITY_UNCLAIMED && role!=POLITY_WILD && w->country[c].capital_prov>=0;
}
/* Score d'HOSTILITÉ d'un rival `c` envers la couronne `crown` — les MÊMES signaux
 * que le reste de la diplomatie IA (jamais un système parallèle) : déjà belliqueux
 * ailleurs (ATWAR_W), opinion basse/négative envers la couronne (mémoire d'actes
 * #26 — sc peut être NULL, bancs/repli), menace/rancune envers elle spécifiquement
 * (diplo_relation.threat + diplo_rancor, les DEUX lus par ai_pick_rival ailleurs). */
static float rebel_backer_hostility(const World *w, const WorldEconomy *econ, const WorldProsperity *wp,
                                    const DiploState *dp, const Statecraft *sc, int c, int crown){
    float score = 0.f;
    if (country_war_count(w,dp,c) > 0) score += tune_f("AI_REBEL_BACKING_ATWAR_W", 0.35f);
    if (sc){
        int op = statecraft_opinion(sc, c, crown);            /* −100..100, ce que C pense de la couronne */
        if (op<0) score += (float)(-op) / 100.f;               /* opinion NÉGATIVE seulement (mémoire d'actes) */
    }
    Relation rel = diplo_relation(w, econ, wp, dp, c, crown);  /* rel.threat = menace perçue de crown SUR c */
    score += rel.threat;
    score += diplo_rancor(dp, c, crown);                       /* vieux grief spécifique contre CETTE couronne */
    return score;
}
/* Une fois par guerre civile INCARNÉE (rate-limit dur, cf. g_backing_tried) : le
 * rival hostile le PLUS FORT (argmax déterministe, départage par cid croissant via
 * `>` strict) ouvre un SECOND FRONT contre la couronne assiégée — et, modestement,
 * envoie un renfort matériel à l'armée rebelle (avant sa première bataille SEULEMENT :
 * moral_courant se recalcule de `count` au choc, cf. scps_army.c — jamais en cours de
 * mêlée). Historiquement, c'est ainsi que certaines rébellions l'emportent (le soutien
 * extérieur) — donc, mécaniquement, quelques révoltes de plus survivent à la couronne
 * plutôt que d'être quasi-systématiquement écrasées. AU PLUS UN bailleur par guerre
 * civile (pas de fan-out : la task exige un rate-limit dur). */
static void rebel_foreign_backing(World *w, WorldEconomy *econ, const WorldProsperity *wp,
                                  DiploState *dp, const Statecraft *sc, struct Campaign *camp,
                                  Rebellion *rb){
    if (!dp) return;
    int reb=rb->rebel_country, crown=rb->owner;
    if (reb<0 || reb>=SCPS_MAX_COUNTRY || crown<0 || crown>=SCPS_MAX_COUNTRY) return;
    if (rb->backing_tried) return;                  /* déjà tenté pour CETTE guerre civile (latch SÉRIALISÉ v56) */
    rb->backing_tried=true;                          /* qu'il trouve un bailleur ou non : UNE tentative, jamais un nag */

    int maxwars = (int)tune_f("AI_REBEL_BACKING_MAXWARS", 1.f);
    int best=-1; float best_score=tune_f("AI_REBEL_BACKING_OPINION", 1.60f);
    for (int c=0; c<w->n_countries && c<SCPS_MAX_COUNTRY; c++){
        if (c==crown || c==reb) continue;
        if (!country_war_capable(w,c)) continue;
        if (diplo_status(dp,c,crown)==DIPLO_ALLIED) continue;     /* on ne trahit pas un allié de la couronne */
        if (diplo_status(dp,c,crown)==DIPLO_WAR) continue;        /* déjà en guerre AVEC la couronne : pas un 2e front */
        if (!diplo_can_declare(dp,c,crown)) continue;              /* TRÊVE respectée */
        if (country_war_count(w,dp,c) >= maxwars) continue;        /* capacité : pas de bailleur surétendu */
        if (diplo_casus_belli(w,econ,wp,dp,c,crown,RES_NONE)==CB_NONE) continue;   /* il faut une RAISON */
        float score = rebel_backer_hostility(w,econ,wp,dp,sc,c,crown);
        if (score > best_score){ best_score=score; best=c; }
    }
    if (best<0) return;   /* personne d'assez hostile/éligible : la couronne mate seule */

    /* SECOND FRONT — le bailleur déclare sa PROPRE guerre à la couronne (même CB que
     * diplo_casus_belli a validé), distincte de la guerre civile (rebelle vs couronne). */
    CasusBelli cb = diplo_casus_belli(w,econ,wp,dp,best,crown,RES_NONE);
    diplo_declare_war_cb(dp, best, crown, cb);
    g_backing_wars++;

    /* MATÉRIEL (secondaire, MODESTE) — un renfort UNIQUE de milice à l'armée rebelle,
     * seulement si elle n'a PAS ENCORE combattu (avant la 1re bataille : moral_courant
     * se déduit de `count` au choc, jamais touché en cours de mêlée). PROPORTIONNEL à
     * la force rebelle ACTUELLE (fraction bornée, pas un paquet ABSOLU) — un paquet fixe
     * doublerait un soulèvement au plancher (MIN_REBELS/MOBIL_CAP ⇒ ~12 paquets minimum)
     * tout en restant anecdotique pour un gros soulèvement ; la fraction reste MODESTE
     * aux deux échelles. */
    if (camp && reb<SCPS_MAX_COUNTRY){
        FieldArmy *fa=&camp->army[reb];
        if (fa->active && fa->battles<=0){
            ArmyState *force=&fa->force;
            long cur=campaign_units(camp, reb);              /* la force ACTUELLE (paquets de 100), déjà exportée */
            float frac=tune_f("AI_REBEL_MATERIEL_FRAC", 0.20f);
            long boost=(long)((float)cur*frac); if (boost<1) boost=1;   /* plancher : le geste existe même minuscule */
            int slot=-1;
            for (int i=0;i<force->n_units;i++) if (force->units[i].type==U_MILICE){ slot=i; break; }
            if (slot<0 && force->n_units<ARMY_MAX_UNITS){
                slot=force->n_units++;
                force->units[slot].type=U_MILICE; force->units[slot].count=0; force->units[slot].moral_courant=0.f;
            }
            if (slot>=0){ force->units[slot].count += boost; g_backing_materiel++; }
        }
    }
}

/* SÉCESSION : un pays naît, prend la région, s'installe sur le groupe rebelle. */
/* INSTALLER la sécession dans le pays `nid` (déjà alloué) : la région bascule à lui,
 * le pays devient un État TERRIEN (capitale/région), la terre libérée se relégitime.
 * Partagé par spawn_secession (nouveau slot, voie instantanée) et la voie GUERRE
 * (réutilise rb->rebel_country → PAS de slot en fuite : le belligérant devient l'État). */
static void secede_to_country(World *w, WorldEconomy *econ, WorldLegitimacy *wl, Rebellion *rb, int nid){
    if (nid<0 || nid>=w->n_countries || nid>=SCPS_MAX_COUNTRY) return;
    Country *nc=&w->country[nid];
    nc->role=POLITY_ANTAGONIST;                 /* un nouvel empire libre */
    bool rb_reg_ok = (rb->region>=0 && rb->region<w->n_regions);   /* garde : index région valide (≥0 ET < n) */
    nc->continent=rb_reg_ok ? w->region[rb->region].continent : 0;
    nc->capital_prov=(rb_reg_ok && w->region[rb->region].n_provinces>0)
                     ? w->region[rb->region].province_ids[0] : -1;
    nc->n_regions=1; nc->region_ids[0]=(int16_t)rb->region;
    nc->color=0xC08040u;                                          /* l'or souverain (≠ rouge insurrection) */
    snprintf(nc->name,sizeof nc->name,"%s libre", heritage_name(rb->heritage));

    /* RE-KEY PROVINCE : la sécession est un événement DE RÉGION (une région entière change
     * de maître) — owner sur TOUTES ses provinces (econ_region_set_owner, comme la
     * conquête) ; coercion/.pop sont PROVINCE-OWNED, routés sur la représentative
     * (region[rb->region].<champ> serait écrasé au prochain econ_tick). */
    econ_region_set_owner(econ, w, rb->region, nid);
    if (rb->region<w->n_regions) w->region[rb->region].country=(int16_t)nid;
    /* la nation libérée se relégitime ; les colons de l'ancienne couronne fondent */
    if (rb->region<SCPS_MAX_REG){ wl->L[rb->region]=7.0f; wl->years_held[rb->region]=0.f; }
    int pid=econ_region_rep_province(econ, rb->region);
    if (pid>=0 && pid<econ->n_prov){
        ProvinceEconomy *pe=&econ->prov[pid];
        pe->coercion=0.f;
        int gi=find_group(&pe->pop, rb->drift_id);
        if (gi>=0){ pe->pop.groups[gi].L=7.f; pe->pop.groups[gi].integration=1.f;
                    pe->pop.groups[gi].agit_base=0.f; pe->pop.groups[gi].diaspora=false;
                    /* BRASSAGE : le sécessionniste devient le peuple SOUVERAIN de cette terre —
                     * il est chez lui, plus un déplacé qui « rentre » (arrival/home_reg naturalisés). */
                    pe->pop.groups[gi].arrival=ARR_NATIF; pe->pop.groups[gi].home_reg=-1; }
    }
}
static int spawn_secession(World *w, WorldEconomy *econ, WorldLegitimacy *wl, Rebellion *rb){
    /* GARDE AMONT : sans région valide, le spawn n'a aucun sens — et la suite ÉCRIT
     * dans econ->region[rb->region] (hors-bornes si négatif). On refuse net. */
    if (rb->region<0 || rb->region>=w->n_regions || rb->region>=econ->n_regions) return -1;
    int nid = free_country_slot(w, econ, rb->owner);      /* d'abord un slot vierge */
    if (nid<0){                                            /* sinon on agrandit la table */
        if (w->n_countries>=SCPS_MAX_COUNTRY) return -1;
        nid=w->n_countries++;
    }
    memset(&w->country[nid],0,sizeof(Country));
    secede_to_country(w, econ, wl, rb, nid);
    return nid;
}

/* ── L'ÉCRASEMENT : morts + le pays se raidit (coercition, L brisée) ─────────
 * Corps commun aux DEUX voies (compare instantané ET défaite au score de guerre). */
static void apply_rebel_crush(RevoltState *rs, World *w, WorldEconomy *econ,
                              ProvinceEconomy *pe, WorldLegitimacy *wl, Rebellion *rb){
    (void)w;   /* symétrie de signature avec apply_rebel_victory ; l'écrasement n'a pas besoin du World */
    long killed=(long)((float)rb->mobilized*CRUSH_KILL);
    long survivors=rb->mobilized-killed;
    rs->pop_lost += killed; rs->n_crushed++;
    demobilize(econ, rb, survivors);
    int gi=find_group(&pe->pop, rb->drift_id);
    if (gi>=0){ pe->pop.groups[gi].L=clampf(pe->pop.groups[gi].L-2.f,0.f,10.f);
                pe->pop.groups[gi].agit_base=clampf(pe->pop.groups[gi].agit_base+15.f,0.f,100.f); }
    pe->coercion=1.f;                                   /* loi martiale durable */
    if (rb->region<SCPS_MAX_REG) wl->L[rb->region]*=0.75f;  /* régner par la peur ronge L */
    rb->outcome=OUT_CRUSHED;
    /* CONTRE-RÉFORME : une révolte de FOI écrasée est RECONVERTIE de force à la
     * foi d'État — la couronne impose l'orthodoxie sur les cendres du schisme. */
    if (rb->kind==REBEL_HERESIE || rb->kind==REBEL_ZELOTE){
        int sf=(rb->owner>=0&&rb->owner<SCPS_MAX_COUNTRY)?religion_of_country(rb->owner):-1;
        if (sf>=0) religion_set_region(econ, rb->region, sf);
    }
}

/* ── LA VICTOIRE REBELLE : le verdict suit la nature du soulèvement ──────────
 * Corps commun aux DEUX voies. Pour la SÉCESSION, si un pays rebelle est DÉJÀ
 * incarné (voie guerre : rb->rebel_country≥0), on le RÉUTILISE comme État souverain
 * (pas de slot en fuite) ; sinon on en fait naître un (voie instantanée). */
static void apply_rebel_victory(RevoltState *rs, World *w, WorldEconomy *econ,
                                ProvinceEconomy *pe, WorldLegitimacy *wl, Rebellion *rb){
    /* LOT H — la révolte SERVILE VICTORIEUSE affranchit DE FORCE : le groupe qui a
     * porté le soulèvement était CLASS_SLAVE (revolt_ignite l'a figé dans rb->klass) →
     * TOUTE la strate esclave de la région bascule LIBRE (même chemin que la
     * manœuvre pacifiste, granularité région) AVANT le verdict usuel (concession/
     * sécession/… qui suit toujours la nature — la libération EST le grief comblé). */
    if (rb->klass==CLASS_SLAVE) demography_manumit_region(econ, rb->region);
    switch (rb->kind){
        case REBEL_SECESSION: {
            int nid;
            if (rb->rebel_country>=0 && rb->rebel_country<w->n_countries){
                nid=rb->rebel_country; secede_to_country(w, econ, wl, rb, nid);   /* le belligérant DEVIENT l'État */
            } else nid=spawn_secession(w, econ, wl, rb);
            rb->spawned=nid; rs->last_spawned=nid;
            if (nid>=0){ rs->n_seceded++; rb->outcome=OUT_SECEDED; }
            else { demobilize(econ, rb, rb->mobilized); rb->outcome=OUT_CONCESSION; }
            break; }
        case REBEL_COUP: {
            /* l'élite prend le trône : la couronne adopte SA culture, lune
             * de miel de légitimité ; les hommes rentrent (affaire de palais). */
            pe->culture = rb->culture;
            int cap = (rb->owner<w->n_countries)?w->country[rb->owner].capital_prov:-1;
            int cr  = (cap>=0&&cap<w->n_provinces)?w->province[cap].region:-1;
            if (cr>=0&&cr<econ->n_regions){
                int crp=econ_region_rep_province(econ,cr);
                if (crp>=0&&crp<econ->n_prov) econ->prov[crp].culture=rb->culture;
            }
            for (int r=0;r<econ->n_regions;r++) if (econ->region[r].owner==rb->owner && r<SCPS_MAX_REG)
                wl->L[r]=clampf(wl->L[r]+1.5f,0.f,10.f);
            pe->coercion=fmaxf(0.f, pe->coercion-0.3f);
            demobilize(econ, rb, rb->mobilized);
            faction_levers_on_coup(rb->owner);   /* §4 : le coup purge la rancœur (plus de spirale) */
            if (rb->owner>=0 && rb->owner<SCPS_MAX_COUNTRY)
                rs->coup_grace[rb->owner]=COUP_GRACE_DAYS;   /* §C2 : répit du nouveau régime */
            rs->n_coup++; rb->outcome=OUT_COUP;
            break; }
        case REBEL_HERESIE:
        case REBEL_ZELOTE: {
            /* VICTOIRE d'une révolte de FOI — l'issue suit la NATURE culturelle :
             *  · peuple culturellement ÉTRANGER (nation conquise) → la foi ET la
             *    terre partent : SÉCESSION en État CORELIGIONNAIRE (révolte de
             *    Hollande — la guerre de religion devient guerre d'indépendance) ;
             *  · peuple INTÉGRÉ (même culture, foi dissidente) → paix d'Augsbourg :
             *    la province GARDE sa foi (déjà posée) et RESTE ; la L royale s'érode
             *    d'avoir échoué à imposer l'orthodoxie (un Dieu ÉTRANGER humilie plus). */
            const PopCulture *crown = econ_ruling_culture(w, econ, rb->owner);
            /* une DIASPORA (réfugié/migrant) n'a AUCUNE revendication TERRITORIALE : sa
             * victoire de foi = TOLÉRANCE, jamais sécession. Seul un peuple NATIF étranger
             * (nation conquise sur SA terre) sécède en État coreligionnaire (Hollande). */
            int rgi = find_group(&pe->pop, rb->drift_id);
            bool is_dia = (rgi>=0) && pe->pop.groups[rgi].diaspora;
            bool separatist = crown && !is_dia && (econ_content_dist(&rb->culture, crown) >= SECEDE_D);
            if (separatist){
                int nid;
                if (rb->rebel_country>=0 && rb->rebel_country<w->n_countries){
                    nid=rb->rebel_country; secede_to_country(w, econ, wl, rb, nid);
                } else nid=spawn_secession(w, econ, wl, rb);
                rb->spawned=nid; rs->last_spawned=nid;
                if (nid>=0){ rs->n_seceded++; rb->outcome=OUT_SECEDED; }
                else { demobilize(econ, rb, rb->mobilized); rb->outcome=OUT_CONCESSION; }
            } else {
                float lhit = (rb->kind==REBEL_ZELOTE)?1.5f:1.0f;
                if (rb->region<SCPS_MAX_REG)
                    wl->L[rb->region]=clampf(wl->L[rb->region]-lhit,0.f,10.f);
                pe->satisfaction=clampf(pe->satisfaction+0.15f,0.f,1.f);
                pe->coercion=fmaxf(0.f, pe->coercion-0.3f);
                { int gi=find_group(&pe->pop, rb->drift_id);
                  if (gi>=0) pe->pop.groups[gi].agit_base=clampf(pe->pop.groups[gi].agit_base-20.f,0.f,100.f); }
                demobilize(econ, rb, rb->mobilized);
                rs->n_concession++; rb->outcome=OUT_CONCESSION;   /* la couronne CÈDE sur la foi */
            }
            break; }
        default: {  /* REBEL_CLASS : la couronne CÈDE — SI elle peut (G0.2) */
            int cid=rb->owner;
            int capr=(cid>=0&&cid<w->n_countries)?w->country[cid].capital_prov:-1;
            capr=(capr>=0&&capr<w->n_provinces)?w->province[capr].region:-1;
            int caprp=(capr>=0&&capr<econ->n_regions)?econ_region_rep_province(econ,capr):-1;
            float treas=(caprp>=0&&caprp<econ->n_prov)?econ->prov[caprp].treasury:0.f;
            float capL =(capr>=0&&capr<SCPS_MAX_REG)?wl->L[capr]:0.f;
            bool can_concede = (cid<0||cid>=SCPS_MAX_COUNTRY||rs->concede_cd[cid]<=0.f)  /* pas déjà concédé ce décennie */
                            && (treas>CONCEDE_TREAS_FLOOR || capL>CONCEDE_L_FLOOR);     /* … et de quoi céder */
            if (!can_concede){
                /* REFUS : la couronne RÉPRIME plutôt que de céder encore (l'écrasement tranche). */
                long killed=(long)((float)rb->mobilized*CRUSH_KILL);
                rs->pop_lost += killed; rs->n_crushed++;
                demobilize(econ, rb, rb->mobilized-killed);
                int gi=find_group(&pe->pop, rb->drift_id);
                if (gi>=0){ pe->pop.groups[gi].L=clampf(pe->pop.groups[gi].L-2.f,0.f,10.f);
                            pe->pop.groups[gi].agit_base=clampf(pe->pop.groups[gi].agit_base+15.f,0.f,100.f); }
                pe->coercion=1.f;
                if (rb->region<SCPS_MAX_REG) wl->L[rb->region]*=0.75f;
                rb->outcome=OUT_CRUSHED;
                break;
            }
            if (caprp>=0&&caprp<econ->n_prov && treas>CONCEDE_TREAS_FLOOR)
                econ->prov[caprp].treasury=fmaxf(0.f, econ->prov[caprp].treasury-tune_f("CONCEDE_GOLD",CONCEDE_GOLD));  /* acheter la paix */
            if (cid>=0&&cid<SCPS_MAX_COUNTRY) rs->concede_cd[cid]=CONCEDE_CD_DAYS;                    /* 10 ans avant de re-céder */
            pe->satisfaction=clampf(pe->satisfaction+0.20f,0.f,1.f);
            pe->coercion=fmaxf(0.f, pe->coercion-0.4f);
            int gi=find_group(&pe->pop, rb->drift_id);
            if (gi>=0){ pe->pop.groups[gi].L=clampf(pe->pop.groups[gi].L+2.f,0.f,10.f);
                        pe->pop.groups[gi].agit_base=clampf(pe->pop.groups[gi].agit_base-25.f,0.f,100.f); }
            demobilize(econ, rb, rb->mobilized);
            /* §C3 — la concession a un PRIX : la faction de l'extorqueur CAPTURE
             * l'État (rot↑ → malus noble), et l'OSSATURE ploie sans rebond
             * (K creusé + légitimité d'un cran) → l'empire concédant devient flasque. */
            { float lean[FAC_COUNT]; group_ethos_lean(&rb->culture, lean);
              int wf=0; for (int f=1;f<FAC_COUNT;f++) if (lean[f]>lean[wf]) wf=f;
              faction_concede(rb->owner, (EthosFaction)wf); }
            pe->build.K_inst = fmaxf(0.f, pe->build.K_inst - tune_f("C3_K_HOLLOW",C3_K_HOLLOW));
            if (rb->region<SCPS_MAX_REG)
                wl->L[rb->region] = clampf(wl->L[rb->region]-tune_f("C3_L_HOLLOW",C3_L_HOLLOW), 0.f, 10.f);
            rs->n_concession++; rb->outcome=OUT_CONCESSION;
            break; }
    }
}

/* Phase 3a — CLÔTURER LA GUERRE CIVILE : le pays rebelle a joué son rôle. Pour la
 * SÉCESSION VICTORIEUSE il SURVIT (État souverain, apply_rebel_victory l'a doté d'une
 * région) → on solde juste la guerre par une paix blanche. Dans TOUS les autres cas
 * (défaite OU victoire sans terre : coup/tolérance/concession) il ne tient aucune
 * région → diplo_settle (couronne = vainqueur) le TUE (polity_death interne, aucun
 * slot en fuite) et libère l'occupation. dp requis ; rebel_country validé. */
static void end_civil_war(DiploState *dp, World *w, WorldEconomy *econ, WorldLegitimacy *wl, Rebellion *rb){
    if (!dp || rb->rebel_country<0 || rb->rebel_country>=SCPS_MAX_COUNTRY) return;
    int reb=rb->rebel_country, crown=rb->owner;
    if (rb->outcome==OUT_SECEDED && rb->spawned==reb){
        /* le nouvel État tient sa terre (secede_to_country l'y a installé) : on efface
         * l'occupation résiduelle du siège (la région est SA propriété désormais) puis
         * on signe la paix — il SURVIT en souverain. */
        diplo_liberate(dp, econ, rb->region);
        diplo_make_peace(dp, reb, crown);        /* le nouvel État et son ancienne couronne signent */
    } else {
        diplo_settle(dp, w, econ, wl, crown, reb, false);   /* le rebelle sans terre MEURT (polity_death) */
    }
    /* SOUTIEN ÉTRANGER : `backing_tried` vit sur la Rebellion — un slot `reb` RÉUTILISÉ par
     * une future guerre civile repart à false via l'init d'allumage (revolt_ignite), donc
     * aucune RAZ ici (et l'early-exit qui court-circuite end_civil_war n'introduit plus de
     * staleness). */
    rb->rebel_country=-1;                         /* le lien est soldé (plus d'index vivant à traîner) */
}

void revolt_tick(RevoltState *rs, World *w, WorldEconomy *econ, ModifierStack *drift,
                 WorldLegitimacy *wl, const WorldProsperity *wp,
                 DiploState *dp, struct Campaign *camp, const Statecraft *sc, int days){
    (void)drift; (void)camp;
    rs->last_spawned=-1;
    for (int i=0;i<rs->count;i++){
        Rebellion *rb=&rs->list[i];
        if (!rb->active) continue;
        rb->days += days;
        RegionEconomy *re=&econ->region[rb->region];   /* LECTURES (agrégats) */
        /* RE-KEY PROVINCE : les MUTATIONS ci-dessous (coercion/culture/satisfaction/
         * build.K_inst/.pop.groups[].L,agit_base/revolt_scar/treasury) sont PROVINCE-OWNED
         * (charte règle 1) — region[rb->region].<champ> serait écrasé au prochain econ_tick,
         * on route sur la province représentative. */
        int rpid=econ_region_rep_province(econ, rb->region);
        ProvinceEconomy *pe=(rpid>=0 && rpid<econ->n_prov) ? &econ->prov[rpid] : NULL;

        /* la couronne a-t-elle changé / la région perdue ? le soulèvement tombe. */
        if (re->owner!=rb->owner){ rb->active=false; continue; }
        if (!pe){ rb->active=false; continue; }

        if (rb->days < REBEL_DECIDE_DAYS) continue;   /* la lutte couve */

        /* ══ Phase 3a — LA GUERRE CIVILE : DÉCISIVE (une seule défaite) ═════════
         * Un pays rebelle est incarné (rb->rebel_country≥0) : la campagne/bataille
         * (ticke AVANT nous chaque jour) a déjà appliqué l'issue des combats de ce
         * tick. On LIT l'ÉTAT de l'armée rebelle (FieldArmy) et le score, et on
         * tranche AU PREMIER dénouement — pas d'attrition. (Le compare instantané
         * reste le REPLI pour rebel_country<0 / dp NULL, plus bas.) */
        if (rb->rebel_country>=0 && dp){
            rb->war_days += days;
            int reb=rb->rebel_country;
            bool reb_alive = (reb<w->n_countries && w->country[reb].role!=POLITY_UNCLAIMED);
            bool at_war = reb_alive && diplo_status(dp, reb, rb->owner)==DIPLO_WAR;
            /* SOUTIEN ÉTRANGER (géopolitique) — tant que la guerre civile est VIVE (le
             * rebelle tient encore, at_war), un rival hostile de la couronne peut ouvrir
             * un second front + envoyer un renfort modeste. UNE tentative par guerre
             * civile (rb->backing_tried, latch SÉRIALISÉ dans rebel_foreign_backing). */
            if (at_war) rebel_foreign_backing(w, econ, wp, dp, sc, camp, rb);
            /* ── ÉTAT DE L'ARMÉE REBELLE (détection DIRECTE de la défaite) ──
             * L'armée rebelle a-t-elle DÉJÀ livré bataille (camp->army[reb].battles>0) ?
             * Si oui, une DÉROUTE la laisse BRISÉE (broken_days>0), voire DÉTRUITE
             * (!active OU force vidée) — c'est LA défaite unique. bt_rout pousse alors
             * le score de la couronne (rebelle NÉGATIF) : deux signaux concordants. */
            const FieldArmy *ra = (camp && reb<SCPS_MAX_COUNTRY) ? &camp->army[reb] : NULL;
            bool fought   = ra && ra->battles>0;
            bool army_gone= !ra || !ra->active || campaign_units(camp, reb)<=0;  /* dissoute/anéantie */
            bool army_broken = ra && ra->broken_days>0;                          /* déroutée (brisée) */
            float ws = at_war ? diplo_war_score(dp, reb, rb->owner) : -100.f;

            /* CROWN WINS — DÉFAITE UNIQUE : l'armée rebelle est brisée/détruite (après
             * avoir combattu), OU le pays rebelle est déjà mort/en paix, OU le score a
             * viré négatif suite à une bataille, OU le garde-fou de durée a sauté. */
            bool crown_wins = !at_war
                            || (fought && (army_broken || army_gone))
                            || (fought && ws <= REBEL_WARSCORE_LOSE)
                            || rb->war_days > REBEL_WAR_MAX_DAYS;
            /* REBEL WINS — VICTOIRE DÉCISIVE : le rebelle tient (armée intacte, pas
             * brisée) et son score est NETTEMENT positif (il a battu la couronne et/ou
             * occupe la région). On exige l'armée VIVE pour ne pas confondre avec une
             * victoire à la Pyrrhus où elle vient de se faire détruire. */
            bool rebel_wins = at_war && !army_gone && !army_broken && ws >= REBEL_WARSCORE_WIN;

            if (getenv("SCPS_REVDIAG") && (crown_wins||rebel_wins))
                fprintf(stderr,"[REVDIAG] reb=%d fought=%d broken=%d gone=%d ws=%.1f war_days=%d units=%ld -> %s\n",
                        reb,(int)fought,(int)army_broken,(int)army_gone,ws,rb->war_days,
                        campaign_units(camp,reb),rebel_wins?"REBEL":"CROWN");

            if (crown_wins){
                apply_rebel_crush(rs, w, econ, pe, wl, rb);
            } else if (rebel_wins){
                apply_rebel_victory(rs, w, econ, pe, wl, rb);
                g_rebel_victories++;   /* télémétrie : les rebelles ont VAINCU la couronne (guerre civile) */
            } else {
                continue;   /* rien de décisif ce tick (siège/marche en cours) : on attend */
            }
            end_civil_war(dp, w, econ, wl, rb);         /* solde la guerre (paix blanche ou mort du rebelle) */
            /* épilogue commun (cooldown/cicatrice/désactivation) — partagé avec le repli. */
            if (rb->region<SCPS_MAX_REG && rb->outcome!=OUT_SECEDED){
                rs->revolt_cooldown[rb->region] = (float)REVOLT_COOLDOWN;
                rs->desperation_days[rb->region] = 0.f;
            }
            pe->revolt_scar = 1.0f;
            rb->active=false;
            continue;
        }

        /* ── REPLI (rebel_country<0 / dp NULL) : compare INSTANTANÉ garnison/rebelles ──
         * Forces en présence (en équivalents-combattants). */
        float zeal = (rb->kind==REBEL_COUP)?ZEAL_COUP
                   : (rb->kind==REBEL_SECESSION)?ZEAL_SECEDE
                   : (rb->kind==REBEL_HERESIE)?ZEAL_HERESIE
                   : (rb->kind==REBEL_ZELOTE)?ZEAL_ZELOTE : ZEAL_CLASS;
        float rebel = (float)rb->mobilized * zeal;
        /* Séparatisme à durée CÂBLÉE : l'élan d'indépendance ∝ fraîcheur de la
         * conquête (plein à chaud, nul une fois la plaie refermée). */
        float rf = (rb->kind==REBEL_SECESSION) ? revanchism_factor(rs, rb->region) : 0.f;
        rebel *= (1.f + (REVANCHISM_REBEL-1.f)*rf);   /* l'indépendance galvanise */

        /* Garnison locale : seuls les groupes LOYAUX (pondérés par leur intégration)
         * se lèvent pour l'ordre — le groupe soulevé, lui, ne se garnisonne pas. Une
         * province fraîchement conquise (peuple restif, mince élite coloniale) tient
         * donc mal : c'est là que naissent les sécessions.
         * RE-KEY PROVINCE : .pop est PROVINCE-OWNED — on lit `pe` (déjà résolu ci-dessus
         * sur econ_region_rep_province), la MÊME province d'où le groupe rebelle a été
         * mobilisé (revolt_ignite) : la garnison face À CE soulèvement doit voir SES
         * voisins de province, pas une diversité régionale qui n'a jamais pris les armes. */
        float loyal_pop = 0.f;
        for (int gi=0; gi<pe->pop.n_groups; gi++){
            const PopGroup *g=&pe->pop.groups[gi];
            if (g->drift_id==rb->drift_id) continue;
            loyal_pop += (float)g->count * clampf(g->integration, 0.f, 1.f);
        }
        float milp = diplo_mil_power(w, econ, rb->owner);
        float reinforce = fminf(milp*REINFORCE, REINFORCE_CAP);   /* l'armée ne tient pas TOUT le pays ici */
        float morale = (rb->owner<wp->n_countries) ? clampf(0.6f+wp->country[rb->owner].SI/12.f,0.6f,1.4f) : 1.f;
        float garrison = (loyal_pop*GARR_LOYAL + re->build.H_coerc*GARR_H + reinforce) * morale;
        garrison *= (1.f - (1.f-REVANCHISM_GARR)*rf);  /* une province fraîchement prise tient mal */

        if (garrison >= rebel) apply_rebel_crush  (rs, w, econ, pe, wl, rb);  /* ÉCRASÉS */
        else                   apply_rebel_victory(rs, w, econ, pe, wl, rb);  /* les rebelles l'emportent */
        /* après TOUT soulèvement résolu, la province est épuisée : elle se tait
         * quelques années (le grief doit se reconstruire) — fin des re-flambées. */
        if (rb->region<SCPS_MAX_REG && rb->outcome!=OUT_SECEDED){
            rs->revolt_cooldown[rb->region]  = (float)REVOLT_COOLDOWN;  /* le RÉPIT court (jours restants) */
            rs->desperation_days[rb->region]  = 0.f;                    /* la colère dépensée : la misère repart de 0 */
        }
        /* CICATRICE : la province convulsée se développe mal quelques années
         * (−50 % croissance & production) — la révolte laisse une plaie économique.
         * RE-KEY PROVINCE : `pe` pointe TOUJOURS la même province représentative
         * (spawn_secession n'a fait que changer son owner via econ_region_set_owner). */
        pe->revolt_scar = 1.0f;
        /* usure : le slot se libère (la liste se compacte au prochain allumage) */
        rb->active=false;
    }
    /* compacter la queue inactive */
    while (rs->count>0 && !rs->list[rs->count-1].active) rs->count--;
}

/* ===================================================================== */
/* MEMBRANE — mots/nombres de JEU                                          */
/* ===================================================================== */
int revolt_active_count(const RevoltState *rs){
    int n=0; for (int i=0;i<rs->count;i++) if (rs->list[i].active) n++; return n;
}
const char *revolt_kind_word(RebelKind k){
    switch(k){ case REBEL_SECESSION: return "sécession";
               case REBEL_COUP:      return "coup d'État";
               case REBEL_CLASS:     return "jacquerie";
               case REBEL_HERESIE:   return "hérésie";
               case REBEL_ZELOTE:    return "zèle religieux";
               default:              return "agitation"; }
}
const char *revolt_outcome_word(int outcome){
    switch(outcome){ case OUT_CRUSHED:    return "écrasée";
                     case OUT_SECEDED:    return "indépendance";
                     case OUT_CONCESSION: return "concession arrachée";
                     case OUT_COUP:       return "trône renversé";
                     default:             return "en cours"; }
}
const char *revolt_class_word(SocialClass k){
    switch(k){ case CLASS_ELITE: return "Noblesse"; case CLASS_BOURGEOIS: return "Artisans";
               default: return "Laboureurs"; }
}
