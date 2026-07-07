#ifndef SCPS_DIPLO_H
#define SCPS_DIPLO_H
/*
 * scps_diplo.h — DIPLOMATIE (§6) & GUERRE (§5)
 *
 * Les relations se LISENT (lecteurs), jamais ne se posent à la main :
 *   - threat     = (eco + mil) / (distance + projection)   [ta formule]
 *   - complement = complémentarité de ressources (durable)
 *   - kinship    = distance de sphère (du même sang … étranger d'une autre sphère)
 *   - schism     = branche religieuse proche + prosélytisme = ennemi naturel
 *   - alliance   = menace partagée (transitoire) + complément (durable) − valeurs − schisme
 *
 * Guerre = acquisition de territoire ET montée de la diversité interne :
 * conquérir une culture lointaine monte le D̄ du conquérant → fracture tant que
 * K ne métabolise pas. La récompense est gatée par le K.
 */
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_prosperity.h"
#include "scps_legitimacy.h"
#include <stdio.h>

typedef struct {
    float threat;       /* menace de b sur a */
    float complement;   /* complémentarité de ressources [0..1] */
    float kinship;      /* distance de sphère [0..7] (haut = étranger) */
    float schism;       /* ennemi naturel religieux [0..1] */
    float alliance;     /* score d'alliance (haut = allié naturel) */
    float shared_rel;   /* §D1 : menace COMMUNE relative au monde — la RAISON de l'alliance.
                         * La dissolution la relit pour LÂCHER quand la menace fond (sans
                         * quoi complément + parenté maintiennent le lien à jamais). */
} Relation;

typedef enum { DIPLO_NEUTRAL = 0, DIPLO_ALLIED, DIPLO_WAR } DiploStatus;

/* CASUS BELLI — la guerre a une RAISON (lue de la relation) ; son type fixe le
 * BUT de guerre et ce qui est exigible à la paix (un frein de plus). */
typedef enum {
    CB_NONE = 0,
    CB_TERRITORIAL,   /* adjacence / revendication / province perdue → prend des provinces */
    CB_RELIGIOUS,     /* schisme + prosélytisme → humiliation (peu/pas de terre) */
    CB_ECONOMIC,      /* un bien aigu MONOPOLISÉ par la cible → la province-source */
    CB_SUBJUGATION,   /* menace + projection → vassalité (pas d'annexion massive) */
    CB_ANTIPIRATERIE  /* la course subie trop longtemps (coques §5) → but : DÉSARMER le commanditaire */
} CasusBelli;

typedef struct DiploState {
    DiploStatus status[SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];
    float       war_years[SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];
    float       truce[SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];  /* jours d'interdiction de guerre (fond) */
    float       momentum[SCPS_MAX_COUNTRY];                 /* conquêtes RÉCENTES (décroît) → fulgurance perçue */
    float       ambient_threat;                             /* §D2 : menace MOYENNE du monde (réf. relative des alliances) */
    int8_t      cb[SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];     /* casus belli ACTIF de a contre b (but de guerre) */
    /* SCORE DE GUERRE — le bras-de-fer (a = ATTAQUANT, celui qui a le CB) :
     * batailles (∝ avantage militaire, PLAFONNÉ +50) + occupation (provinces prises,
     * l'autre +50→+100) ; le défenseur pousse vers −100 par l'attrition. */
    float       battle_score[SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];  /* [-100 .. +50] */
    int16_t     conquered  [SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];   /* régions OCCUPÉES ce conflit (= sièges réels tenus) → pousse le score */
    float       conq_value [SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];   /* §5 combat : PRIX cumulé des provinces TRANSFÉRÉES à la paix (budget de score dépensé) */
    /* OCCUPATION RÉELLE (brief « la guerre se gagne sur le terrain ») : occupier[r]
     * = le pays qui TIENT militairement la région r (siège mené à terme), -1 = libre.
     * La propriété (region.owner) ne change PAS à l'occupation — seulement à la paix
     * (diplo_settle). conquered[occ][owner] = Σ régions de owner tenues par occ. */
    int16_t     occupier   [SCPS_MAX_REG];
    /* RANCUNE NATIONALE (§6) — rancor[a][b] = grief de a contre b qui lui a PRIS des
     * terres. ASYMÉTRIQUE, SURVIT à la paix (le grief reste), décroît sur une
     * génération. Donne à a un casus belli territorial (irrédentisme, sans adjacence)
     * et galvanise sa guerre de reconquête (ralliement). */
    float       rancor     [SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];
    /* LA COURSE (coques §5) : rancune de la VICTIME envers le COMMANDITAIRE
     * identifié — même patron d'accumulation/déclin que la fronde. pirate_disarm :
     * verdict d'une guerre anti-piraterie perdue — la flotte pirate se désarme
     * (lu/exécuté par scps_navy, qui efface le drapeau). */
    float       pirate_rancor[SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];
    int8_t      pirate_disarm[SCPS_MAX_COUNTRY];
    int         n_war_antipirate;     /* télémétrie : guerres anti-piraterie déclarées */
    /* SOUILLURE FAUSTIENNE — faustian[c] = à quel point c développe l'interdit
     * (synchronisé sur sa charge de tech). Une foi ORTHODOXE a une CHANCE de
     * croiser contre un empire qui développe le faustien (Gardiens vs Transgresseurs). */
    float       faustian   [SCPS_MAX_COUNTRY];
    /* ── SUZERAINETÉ (brief leviers §3) — un statut ASYMÉTRIQUE, distinct d'ALLIED :
     * le vassal garde son trône, sa culture, son IA (qui complote sa défection). Le
     * lien vit par la FORCE ou l'INTÉRÊT — le ratio s'effondre, le vassal dénonce. */
    int16_t     suzerain   [SCPS_MAX_COUNTRY];   /* -1 = libre */
    int8_t      contrat    [SCPS_MAX_COUNTRY];   /* SuzContrat du lien (porté par le vassal) */
    int n_servage, n_protectorat, n_concordat, n_cite, n_defections;   /* chronique (cumul sim) */
    /* M3 — LE PACTE COMMERCIAL : un accord RÉCIPROQUE (trade_pact[a][b]==trade_pact[b][a])
     * signé dans l'UI diplo. Il GARANTIT la route (comme la cité-état) ET, surtout, ouvre
     * l'ACCÈS AU MARCHÉ GLOBAL du partenaire : si l'un tient un Centre, l'autre y accède. */
    uint8_t  trade_pact[SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];
    /* BRASSAGE — LE PACTE MIGRATOIRE : un accord RÉCIPROQUE (migration_pact[a][b]==[b][a]),
     * un CRAN de confiance AU-DESSUS du pacte commercial (ouvrir ses frontières à la
     * population de l'autre engage plus que commercer). Il autorise l'ÉCHANGE PASSIF de
     * population entre les deux — à l'avantage du plus ATTRACTIF (la prospérité attire) — et
     * SURTOUT la diaspora reçue diffuse son savoir (métabolisation). L'isolationniste stagne
     * (Song), l'ouvert absorbe. Le tick vit dans demography_migration_pact_tick. */
    uint8_t  migration_pact[SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];
    /* ── LA FRONDE VASSALE (brief fronde) — grief (combustible) × ratio (oxygène) ── */
    float    v_grief [SCPS_MAX_COUNTRY];  /* grief du vassal envers son maître [0..1] */
    float    v_loyal [SCPS_MAX_COUNTRY];  /* jours de LOYAUTÉ ACHETÉE (bloque l'entrée en ligue) */
    uint8_t  v_ligue [SCPS_MAX_COUNTRY];  /* membre de la ligue contre son suzerain ? */
    uint8_t  v_dons  [SCPS_MAX_COUNTRY];  /* usure du don (chaque don pèse moins) */
    int16_t  fronde_suz, fronde_lead;     /* fronde ACTIVE contre ce suzerain (-1 = aucune) */
    float    fronde_score;                /* bras-de-fer de la fronde, capturé AVANT la paix */
    uint32_t fronde_rng;                  /* probabilité par tick (jamais un couperet) */
    /* chronique fronde */
    int n_ligues, n_frondes, n_indep, n_renvers, n_ecrase, n_defect_paix, n_defect_guerre;
    int n_lev_don, n_lev_allege, n_lev_divise, n_lev_intim;
    /* ── VASSALITÉ SUR LA DURÉE (pipeline diplo étage 3) — la VALEUR cible, l'ÉTHOS décide la
     * MÉTHODE (tenir-et-traire vs digérer). v_integration : un vassal TENU à la paix se rapproche
     * de son maître [0..1] (∝ proximité culturelle réelle × appréciation) ; passé un seuil il VERSE
     * une contribution TYPÉE (commerce/agraire/martial). v_annex : un maître ANNEXEUR (Dom/Honneur)
     * DIGÈRE un vassal intégré — PROCESSUS de durée ∝ prix × (1−intégration), payé en or ; à 1.0,
     * transfert + cicatrice DOUCE. (Tout mord APRÈS l'an-12 ⇒ déterminisme golden intact.) */
    float    v_integration[SCPS_MAX_COUNTRY];   /* [0..1] intégration du vassal vers son maître */
    float    v_annex       [SCPS_MAX_COUNTRY];  /* [0..1] avancement de l'annexion-processus */
    int      n_annex;                            /* chronique : annexions PAR DIGESTION abouties */
    /* ── W-GUERRE-3 — LE CASUS BELLI SE FABRIQUE ET SE PAIE ────────────────────────────
     * Un CB « offensif » (territorial/économique/religieux, hors défensif/anti-piraterie/
     * subjugation-de-hameau) n'est plus GRATUIT : le fabricant PAIE (2 ans de revenu de la
     * CIBLE, corrompre ses élites), attend 1 an que l'intrigue MÛRISSE, puis dispose de 5 ans
     * de fenêtre pour déclarer contre CETTE cible avant que le grief acheté ne s'évente.
     * fab_state[a][b] : 0=aucune intrigue · 1=en cours de maturation (fab_days décompte) ·
     * 2=mûre/valide (fab_days = jours restants avant expiration). AUCUN cooldown entre cibles
     * DIFFÉRENTES — a peut financer plusieurs intrigues en parallèle (chacune paie son prix). */
    int8_t   fab_state[SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];
    float    fab_days [SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];  /* jours restants (maturation OU validité) */
    int8_t   fab_cb   [SCPS_MAX_COUNTRY][SCPS_MAX_COUNTRY];  /* le CB acheté (CasusBelli), figé à la fabrication */
} DiploState;

/* Une intrigue fabriquée, par cible : NONE (rien), MATURING (payée, patiente), READY (valide,
 * expire si non utilisée). Miroir enum de fab_state[][] (int8_t sérialisé — la sémantique
 * vit ici, pas un magic number nu). */
typedef enum { FAB_NONE=0, FAB_MATURING, FAB_READY } FabState;

/* Les QUATRE contrats de suzeraineté — quatre logiques (cf. brief §3). */
typedef enum { CONTRAT_NONE=0, CONTRAT_SERVAGE, CONTRAT_PROTECTORAT,
               CONTRAT_CONCORDAT, CONTRAT_CITE } SuzContrat;
/* Pose un lien suzerain→vassal (met fin à leur guerre, ouvre une trêve) ; remplace l'ancien. */
void        diplo_set_vassal  (DiploState *d, int suzerain, int vassal, SuzContrat c);
/* Le vassal dénonce : libre — le SERF part en guerre d'indépendance (to_war). */
void        diplo_break_vassal(DiploState *d, int vassal, bool to_war);
int         diplo_suzerain    (const DiploState *d, int cid);   /* -1 = libre */
SuzContrat  diplo_contrat     (const DiploState *d, int cid);
int         diplo_vassal_count(const DiploState *d, int cid);
const char *diplo_contrat_name(SuzContrat c);
/* DÉCRET « Politique de tribut » (scps_decrees.c) : drapeau LU par diplo_suzerainty_tick
 * (×1.5 tribut/contribution + grief continu). PAS un #include de scps_decrees.h — un
 * simple drapeau par pays pour ne rien devoir à decrees.o dans les ~20 bancs qui lient
 * scps_diplo.o. RAZ implicite (static, memset 0) — jamais sérialisé ICI (l'état persistant
 * du décret vit dans g_decree_mask, section DCRE ; ce drapeau est reposé par decrees.c à
 * chaque decree_toggle ET après un chargement, cf. scps_decrees.c). */
void        diplo_set_tribute_decree(int cid, bool on);
bool        diplo_tribute_decree(int cid);
/* Route GARANTIE (pacte commercial OU cité-état) : ni guerre ni embargo ne coupent ce
 * lien ; le pacte ouvre AUSSI l'accès au marché global du partenaire (M3). */
bool        diplo_trade_pact  (const DiploState *d, int a, int b);
/* M3 — signer/rompre un pacte commercial RÉCIPROQUE (les deux sens). */
void        diplo_set_trade_pact(DiploState *d, int a, int b, bool on);
/* BRASSAGE — le pacte MIGRATOIRE (réciproque) : autorise l'échange passif de population. */
bool        diplo_migration_pact(const DiploState *d, int a, int b);
void        diplo_set_migration_pact(DiploState *d, int a, int b, bool on);
/* Tick ANNUEL : entre pays sous pacte migratoire et à la PAIX, un flux passif de population
 * s'échange — le plus ATTRACTIF (prospère) reçoit net ; les migrants portent leur héritage
 * → diaspora chez l'hôte → métabolisation (diffusion du savoir). Renvoie le nb de flux. */
int         demography_migration_pact_tick(WorldEconomy *e, const DiploState *dp);
/* Tick ANNUEL : TRIBUTS (servage lourd 8 %/an + coercition chez le serf ; protectorat
 * léger 2 %), APPEL du protecteur (les guerres du protégé l'appellent), DÉFECTION
 * (ratio de force < ~1.15 → dénonciation ; le serf part en guerre), ACCEPTATION par
 * la MENACE (un petit SANS allié sous un voisin écrasant ≥ 1.8 accepte le protectorat
 * — la voie « menace » ; la voie « mission » D3 viendra par statecraft).
 * étage 3 : peut MUTER le World (l'annexion-processus DIGÈRE un vassal → polity_death). */
void diplo_suzerainty_tick(DiploState *d, World *w, WorldEconomy *econ,
                           const WorldProsperity *wp);
/* Fidélité d'un vassal — pour la membrane (le ratio 1,2 ne s'affiche JAMAIS ;
 * la fronde se PRESSENT en bandes et en signes, elle ne se calcule pas à l'écran). */
float diplo_vassal_grief(const DiploState *d, int vassal);   /* [0..1] nu — à BANDER côté readout */

void diplo_init(DiploState *d);
/* graine de fronde ∝ monde (à appeler à la CRÉATION d'une partie, après init —
 * la sauvegarde préserve fronde_rng, on ne re-sème pas au chargement). */
void diplo_seed_rng(DiploState *d, uint32_t seed);
/* sauvegarde : statiques du module (cooldown d'intimidation de la fronde). */
void diplo_save_statics(FILE *f);
bool diplo_load_statics(FILE *f);

/* ---- Lecteurs ---------------------------------------------------------- */
float    diplo_eco_power(const WorldProsperity *wp, const WorldEconomy *econ, int cid);  /* §5 : P_realise + poids de la puissance commerciale */
float    diplo_mil_power(const World *w, const WorldEconomy *econ, int cid);
Relation diplo_relation (const World *w, const WorldEconomy *econ,
                         const WorldProsperity *wp, const DiploState *d, int a, int b);

/* ---- Actions ----------------------------------------------------------- */
void        diplo_declare_war (DiploState *d, int a, int b);
/* Le CASUS BELLI inhérent le plus pertinent de a contre b (lu de la relation +
 * du besoin `want` : un bien aigu monopolisé par b → CB économique). CB_NONE si
 * aucune raison ne tient → l'IA ne peut pas déclarer (elle renonce ou attend). */
CasusBelli  diplo_casus_belli (const World *w, const WorldEconomy *econ,
                               const WorldProsperity *wp, const DiploState *d,
                               int a, int b, Resource want);
/* Déclare la guerre AVEC un but (le CB est mémorisé → il gate la paix). */
void        diplo_declare_war_cb(DiploState *d, int a, int b, CasusBelli cb);
CasusBelli  diplo_war_goal     (const DiploState *d, int a, int b);
const char *diplo_cb_name      (CasusBelli cb);
/* TÉLÉMÉTRIE (chronicle) — déclarations de guerre PAR MOTIF (casus belli) sur la sim courante.
 * out[CB_*] ; statique remis à plat par diplo_init, jamais sérialisé (hors déterminisme/SAVE). */
void        diplo_war_cb_counts(int out[CB_ANTIPIRATERIE+1]);

/* ---- W-GUERRE-3 — LE CASUS BELLI FABRIQUÉ (payant) --------------------------------
 * Certains CB restent GRATUITS (aucune fabrication requise) : la guerre DÉFENSIVE (se
 * défendre n'exige jamais de CB — rien ne gate le défenseur qui riposte, diplo_declare_war_cb
 * accepte n'importe quel appelant), CB_ANTIPIRATERIE (subie, pas choisie) et CB_SUBJUGATION
 * (projection de puissance sur un hameau/faible — cf. tableau au carnet TROUVAILLES.md).
 * CB_TERRITORIAL/CB_ECONOMIC/CB_RELIGIOUS « offensifs » (choisis pour PRENDRE, pas pour se
 * défendre) exigent désormais une intrigue FABRIQUÉE et MÛRE contre CETTE cible précise.
 *
 * diplo_can_fabricate  : `a` a-t-il assez d'or pour fabriquer contre `b` maintenant (et
 *                        aucune intrigue déjà EN COURS contre b — une seule à la fois PAR
 *                        CIBLE, mais aucune limite inter-cibles) ?
 * diplo_fabricate_cost : le PRIX (2× le revenu annuel de la CIBLE, econ_country_tax_year).
 * diplo_fabricate_cb   : FABRIQUE l'intrigue (débite `a`, fige le CB choisi, POSE le minuteur
 *                        de maturation). `a` doit avoir l'or (l'appelant vérifie can_fabricate
 *                        — sinon no-op, false). Le CB est choisi par l'appelant (IA : le même
 *                        calcul que diplo_casus_belli déciderait naturellement ; joueur :
 *                        déduit par diplo_casus_belli au moment de fabriquer).
 * diplo_fab_state/_days_left : lecteurs (façade/UI) — état + jours restants (maturation OU
 *                        validité selon l'état).
 * diplo_fab_ready_cb   : le CB PRÊT (mûr, non expiré) contre `b`, CB_NONE si aucun. C'est CE
 *                        CB qu'une déclaration de guerre offensive doit consommer. */
bool        diplo_can_fabricate  (const World *w, const WorldEconomy *econ, const DiploState *d, int a, int b);
float       diplo_fabricate_cost (const WorldEconomy *econ, int target);
bool        diplo_fabricate_cb   (World *w, WorldEconomy *econ, DiploState *d, int a, int b, CasusBelli cb);
FabState    diplo_fab_state      (const DiploState *d, int a, int b);
float       diplo_fab_days_left  (const DiploState *d, int a, int b);
CasusBelli  diplo_fab_ready_cb   (const DiploState *d, int a, int b);
/* CB « gratuit » (pas de fabrication requise) : défensif (implicite, jamais gaté),
 * anti-piraterie, subjugation. Un CB EXIGE la fabrication ssi ce prédicat est faux. */
bool        diplo_cb_needs_fabrication(CasusBelli cb);
/* Tick ANNUEL des intrigues (appelé depuis diplo_tick) : la maturation descend vers READY,
 * la validité descend vers l'expiration (→ FAB_NONE, le grief acheté s'est éventé). */
void        diplo_fab_tick(DiploState *d, float dt_days);
void        diplo_form_alliance(DiploState *d, int a, int b);
void        diplo_make_peace  (DiploState *d, int a, int b);
DiploStatus diplo_status      (const DiploState *d, int a, int b);
/* §D-sat : plafond d'alliances par polité (l'alliance est une ressource RARE) +
 * compteur — partagés par l'IA (alliances naturelles) ET le statecraft (missions),
 * pour que « 2 slots max » soit un invariant GLOBAL, sans fuite par une autre voie. */
#define     DIPLO_ALLY_SLOTS  2
int         diplo_ally_count  (const DiploState *d, int a);
/* Peut-on déclarer la guerre ? false pendant la TRÊVE (espace l'enchaînement). */
bool        diplo_can_declare (const DiploState *d, int a, int b);
float       diplo_truce_days  (const DiploState *d, int a, int b);   /* lecture (UI/IA) */

/* ---- Diplomatie d'ÉQUILIBRE (rétroaction négative, pas d'interdiction) ----- *
 * Coût d'élargissement : frapper un protégé d'alliés puissants risque d'étendre
 * la guerre → renchérit la cible (somme des forces alliées susceptibles d'entrer). */
float diplo_war_widening_cost(const World *w, const WorldEconomy *econ,
                              const DiploState *d, int attacker, int target);
/* La menace dominante perçue par `self` ; renvoie -1 si aucune ne franchit le
 * seuil de coalition. Une coalition ÉMERGE quand un même hégémon dépasse ce seuil
 * pour plusieurs royaumes (aucun script, juste des lectures de menace sommées). */
int   diplo_perceived_hegemon(const World *w, const WorldEconomy *econ,
                              const WorldProsperity *wp, const DiploState *d, int self);

/* (diplo_conquer_region RETIRÉ — brief terrain : plus de transfert EN GUERRE. La
 * propriété change à la PAIX via diplo_settle ; l'occupation tient le terrain.) */

/* OCCUPATION (brief terrain) — `occ` prend militairement `region` (siège réel mené
 * à terme par la couche sim) : exige DIPLO_WAR avec son propriétaire, déloge un
 * tiers occupant (son occupation tombe), pose occupier[region]=occ et pousse le
 * score (conquered[occ][owner]++). NE CHANGE PAS la propriété. false si non éligible. */
bool diplo_occupy   (DiploState *d, const WorldEconomy *econ, int occ, int region);
/* LIBÉRATION — l'occupation de `region` tombe (le propriétaire l'a reprise, ou la
 * paix l'efface) : occupier[region]=-1 et conquered de l'ex-occupant décrémenté. */
void diplo_liberate (DiploState *d, const WorldEconomy *econ, int region);

/* RÈGLEMENT À LA PAIX (brief terrain) — c'est ICI, et nulle part en guerre, que la
 * PROPRIÉTÉ change : le vainqueur annexe les régions du vaincu qu'il OCCUPE, triées
 * (adjacentes d'abord, puis prix croissant) et BORNÉES par diplo_war_budget (§5) ;
 * le reste des occupations (deux sens) est relâché, la paix solde le bras-de-fer.
 * Un vaincu réduit à 0 région MEURT (role→UNCLAIMED, relations & vassalité dénouées).
 * `winner_enslaves` = le vainqueur a-t-il l'Économie servile (résolu par l'appelant).
 * Renvoie le nombre de régions transférées (0 = paix blanche). */
int  diplo_settle   (DiploState *d, World *w, WorldEconomy *econ, WorldLegitimacy *wl,
                     int winner, int loser, bool winner_enslaves);

/* SACCAGE (§4 → LOT P 2026-07-07) — une province PRISE est DÉPOUILLÉE une fois : la
 * VALEUR pillée = 20 % du revenu ANNUEL de la VICTIME (`victim_cid`, econ_country_
 * tax_year — règle joueur unifiée : « piraterie, raids, tout type d'occupation =
 * pillage »), transférée RÉELLEMENT (trésor d'abord, puis stock valorisé pour
 * combler), BORNÉE par ce qui existe vraiment ; fondue dans le trésor de l'occupant
 * (région `dst_region`, sa capitale) ; 1×/5 ans/province (plus rien à prendre avant).
 * Le sac convulse la province (cicatrice au plancher → gel du développement).
 * Renvoie la valeur pillée (or-équivalent) ; 0 si encore à vif/victime sans revenu.
 * Appelé par settle_transfer ET l'occupation-capture (scps_sim.c) ; exposé pour le
 * banc d'essai. */
float diplo_pillage_region(WorldEconomy *econ, int region, int dst_region, int victim_cid);

/* diplo_pillage_value — le MOVER NU derrière diplo_pillage_region : transfère la
 * valeur (20 % du revenu annuel de `victim_cid`) SANS poser de cooldown — pour les
 * appelants qui gèrent LEUR PROPRE marqueur anti-répétition (la course pirate :
 * raid_cd_days/balafre). Exposé pour scps_navy.c et le banc d'essai. */
float diplo_pillage_value(WorldEconomy *econ, int region, int dst_region, int victim_cid);

/* diplo_pillage_fresh — la province (grain agrégat région→province représentative)
 * a-t-elle échappé à un sac RÉCENT (pillage_cd épuisé) ? Lecture PARTAGÉE par
 * l'appelant pour gater ENSEMBLE l'esclavage et le pillage-valeur d'un MÊME
 * évènement (cf. diplo_enslave_capture, plus bas). */
bool diplo_pillage_fresh(const WorldEconomy *econ, int region);

/* TÉLÉMÉTRIE « pillage réel » (LOT P, chronicle) — motif diplo_war_cb_counts :
 * statiques RAZ par diplo_init, jamais sérialisés/lus par le moteur. events = nb de
 * pillages-valeur exécutés · value = or-équivalent RÉELLEMENT transféré · target =
 * Σ des cibles (20% du revenu annuel) — le ratio value/target dit si la borne
 * « ce qui existe vraiment » mord · souls = âmes DÉPORTÉES (enslave_capture). */
void diplo_pillage_stats(long *events, double *value, double *target, long *souls);

/* LOT 4 — LE PILLAGE DE SIÈGE (mensuel, PENDANT le siège/l'occupation, distinct du
 * butin final ci-dessus) : détourne une FRACTION (SIEGE_LOOT_FRAC, registre J) de la
 * PRODUCTION du mois (supply[], pas le stock accumulé) de `region` vers le trésor de
 * `dst_region` (la capitale du besiégeur). Matière RÉELLEMENT prise (le stock
 * équivalent est décrémenté). Gaté par le MÊME cooldown anti-re-saccage
 * (pillage_cd) qu'un butin final récent. Renvoie la valeur détournée ce mois (0 si
 * rien à prendre / cooldown actif). À appeler CHAQUE MOIS pendant qu'une force
 * assiège/occupe (cf. scps_sim.c, sim_campaign_year). Mécanique DISTINCTE du pillage
 * unifié (LOT P) — hors scope, non touchée. */
float diplo_siege_loot(WorldEconomy *econ, int region, int dst_region);

/* ESCLAVAGE (§4c → LOT P 2026-07-07) — une société ASSERVISSANTE (TECH_ESCLAVAGE OU
 * éthos conquérant Dominateur/Honneur, `can_enslave` résolu par l'appelant) déporte
 * SLAVE_FRACTION (registre J, 5 %) de la population prise vers son CŒUR (capitale) :
 * un groupe DIASPORA non-intégré (restif) de culture étrangère → le D̄ du maître
 * monte, la fracture s'installe au centre. Renvoie le nombre de captifs ; 0 si
 * `enslaves` est faux. Ne pose PLUS son propre pillage_cd (LOT P : toujours appelée
 * aux côtés de diplo_pillage_region, gatée UNE FOIS par diplo_pillage_fresh).
 * Appelée par settle_transfer, l'occupation-capture (scps_sim.c) et la course
 * pirate (scps_navy.c) ; exposée pour le banc d'essai. */
long diplo_enslave_capture(const World *w, WorldEconomy *econ, int conqueror, int region, bool enslaves);

void diplo_tick(DiploState *d, float dt);   /* usure de guerre (war_years++) + trêve/momentum */

/* ---- SCORE DE GUERRE (§2) — le bras-de-fer, à ticker chaque an ---------- *
 * Met à jour le battle_score (∝ avantage militaire, plafonné +50) et applique
 * l'ATTRITION (la guerre SAIGNE les armes des deux camps, le perdant plus →
 * mil_power baisse → la guerre s'épuise). L'occupation se lit à part (conquered). */
void  diplo_war_tick (DiploState *d, World *w, WorldEconomy *econ,
                      const WorldProsperity *wp, float dt);
/* Le score courant du point de vue de l'ATTAQUANT a contre b [-100..+100] :
 * batailles (≤+50) + occupation (+50→+100) − attrition (vers −100). */
float diplo_war_score(const DiploState *d, int a, int b);

/* ---- PAIX PROPORTIONNELLE (§5) — la victoire ACHÈTE des termes -------- *
 * REVENDICATION légitime de a contre b : combien de provinces la domination
 * MILITAIRE justifie d'annexer. Territorial → 1 + ∝ dominance ; les autres CB →
 * 1 prise (la source / l'humiliation) ; sans CB → 1 province tampon si l'on
 * domine, 0 sinon. PRENDRE AU-DELÀ est de la SUREXPANSION : diplo_conquer_region
 * la punit en fulgurance (→ coalition) — biaisé, jamais interdit. */
int   diplo_war_claim (const DiploState *d, const World *w,
                       const WorldEconomy *econ, int a, int b);

/* §5 COMBAT — LE PRIX D'UNE PROVINCE en score de guerre, ∝ sa VALEUR DÉVELOPPÉE
 * (bâti + prospérité + population). Le score de guerre [0..100] est un BUDGET dépensé
 * à la paix : on ne prend que les provinces dont le prix cumulé ≤ score. Un cœur
 * développé coûte cher (victoire DÉCISIVE requise) ; un arrière-pays est bon marché ;
 * une province SACCAGÉE (valeur effondrée) devient moins chère à annexer. */
float diplo_province_price(const WorldEconomy *econ, int region);
/* VALEUR SUBJECTIVE (pipeline diplo) : prix objectif + BESOIN (Σ raw_cap × stress(runway de
 * cid) × prix) + stratégique. `fc` = forecast de cid (econ_country_forecast), NULL = objectif seul. */
float ai_province_value(const WorldEconomy *econ, int cid, int region, const EconForecast *fc);
float diplo_war_budget(const DiploState *d, const World *w, const WorldEconomy *econ,
                       int attacker, int defender);  /* domination militaire + prime de score */
float diplo_country_value(const WorldEconomy *econ, int cid);             /* Σ prix des provinces */
/* §5 BUTIN : le budget de score NON dépensé en terres VIDE les coffres du vaincu. */
float diplo_loot(World *w, WorldEconomy *econ, int attacker, int defender, float leftover_value);
/* RÉPARATIONS : à la paix, le VAINCU (score adverse net) indemnise le vainqueur
 * ∝ |score de guerre| — ponction des trésors provinciaux du perdant → capitale du
 * vainqueur. Renvoie l'or transféré ; 0 si match nul (pas de vainqueur net). */
float diplo_reparations(DiploState *d, World *w, WorldEconomy *econ, int a, int b);

/* RANCUNE (§6) — le grief de a contre b (terres perdues). Lecture pour l'IA (biais
 * de reconquête) et l'UI. Posée par diplo_conquer_region sur le DÉPOSSÉDÉ, plus
 * profonde si la prise fut ILLÉGITIME ; survit à la paix, décroît dans diplo_tick. */
float diplo_rancor(const DiploState *d, int a, int b);
/* COURSE : grief de piraterie (victime → commanditaire IDENTIFIÉ) ; clampé [0..10]. */
void  diplo_pirate_grief (DiploState *d, int victim, int sponsor, float amount);
float diplo_pirate_rancor(const DiploState *d, int victim, int sponsor);

/* SOUILLURE FAUSTIENNE — synchronisée chaque an depuis la charge de tech d'un pays.
 * diplo_faustian_cb : un attaquant ORTHODOXE (foi régnante austère) contre un
 * empire qui développe nettement le faustien a une RAISON religieuse (croisade). */
void  diplo_set_faustian(DiploState *d, int cid, float level);
float diplo_faustian    (const DiploState *d, int cid);
bool  diplo_faustian_cb (const World *w, const WorldEconomy *econ, const DiploState *d,
                         int attacker, int target);

#endif /* SCPS_DIPLO_H */
