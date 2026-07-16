#ifndef SCPS_CREDIT_H
#define SCPS_CREDIT_H
/*
 * scps_credit.h — DETTE & PRÊTS (M3c : LE CRÉDIT RÉEL).
 *
 * v88 (incrément 1) laissait le trésor net passer négatif ("dette" implicite, non
 * sérialisée) puis assignait un créancier a posteriori sur l'intérêt annuel — la dette
 * elle-même n'était jamais un passif RÉEL (aucune pièce ne quittait un prêteur au
 * moment de l'emprunt). M3c ferme ce canal : la dette est un PASSIF SÉPARÉ (CountryDebt),
 * ventilé par créancier (les PROPRES classes du pays, PUIS une cité-état), alimenté par
 * une chaîne d'emprunt RÉELLE (péréquation nationale → classes → cité-état → épuisement,
 * cf. credit_borrow*) : chaque étape déplace des pièces qui existent déjà quelque part.
 * `credit_spend` (dépense ad-hoc : chantiers, soldes, manufactures…) ET le circuit
 * d'ACHAT D'ÉTAT (scps_econ.c, la VA que l'État achète à ses provinces) empruntent au
 * MÊME livre. `credit_of(c)` garde son sens externe (créancier CITÉ-ÉTAT, -1 = aucun) —
 * seule la mécanique interne change.
 */
#include <stdbool.h>
#include <stdio.h>
#include "scps_world.h"
#include "scps_econ.h"
#include "scps_legitimacy.h"

void  credit_init(void);                                       /* RAZ dette + créancier */
int   credit_of(int c);                                        /* créancier CITÉ-ÉTAT de c (-1 = aucun) — readout/diplo/save_sane */
float credit_line(const World *w, const WorldEconomy *e, int c); /* plafond ÉMERGENT (taille éco) — gate de credit_can_spend, inchangé */
bool  credit_can_spend(const WorldEconomy *e, const World *w, int c, float cost);
/* Dépense ad-hoc (agency/warhost/navy/ai/decrees…) : débite le trésor local ; si le
 * découvert dépasse ce que la province peut porter, DÉCLENCHE la chaîne d'emprunt
 * (credit_borrow) au lieu de laisser le trésor "monnaie négative". Un résidu peut
 * subsister SEULEMENT à épuisement total (péréquation+classes+cité-état insuffisants) —
 * cas rare, PAS de gameplay de banqueroute (hors scope M3c). */
void  credit_spend(WorldEconomy *e, const World *w, int c, float cost);
/* Intérêt annuel (rentier) + amortissement + rachat de crédit (cités-états/mercantiles
 * rachètent la dette-classes à sa valeur faciale). Refonte M3c : opère sur les DEUX
 * compartiments de dette (to_class/to_cs) au lieu d'un g_creditor implicite. */
void  credit_year_tick(WorldEconomy *e, const WorldLegitimacy *wl, const World *w);
bool  credit_save(FILE *f);
bool  credit_load(FILE *f);

/* ---- M3c : LA CHAÎNE D'EMPRUNT (le cœur du chantier) --------------------------------
 * Chaque credit_borrow* tente de couvrir `need` (>0) pour le pays c, dans l'ORDRE du
 * brief (péréquation → classes → cité-état), et retourne le montant RÉELLEMENT couvert
 * (<=need — jamais plus). Le reliquat non couvert reste à la charge de l'appelant
 * (résidu MESURÉ, jamais une dette silencieuse). Toutes mutent treasury/wealth/dette. */

/* Péréquation (autres provinces du MÊME pays, surplus>SINK_FLOOR) PUIS emprunt aux
 * PROPRES classes (élites+bourgeois, ∝ richesse, capacité/tick plafonnée, registre J).
 * AUCUN World* requis (province-grain pur) — appelable depuis scps_econ.c (econ_tick
 * n'a pas de World*, cf. ProvinceEconomy::region). */
float credit_borrow_local(WorldEconomy *e, int c, float need);
/* Emprunt à une cité-état/mercantile-pacifiste solvable (créancier existant prolongé en
 * priorité, sinon le plus riche éligible) : trésor RÉEL du prêteur, capacité/tick
 * plafonnée. Requiert World* (rôle/éthos du prêteur — hors grain WorldEconomy seul). */
float credit_borrow_citystate(WorldEconomy *e, const World *w, int c, float need);
/* La chaîne COMPLÈTE (local PUIS cité-état) — utilisée par credit_spend, qui dispose
 * déjà d'un World*. */
float credit_borrow(WorldEconomy *e, const World *w, int c, float need);
/* Appelée UNE FOIS/mois par scps_sim.c juste après econ_tick (qui a accumulé le besoin
 * résiduel NATIONAL — péréquation+classes déjà tentés en interne, cf.
 * econ_va_shortfall_pending) : tente l'étage CITÉ-ÉTAT pour chaque pays, puis retranche
 * ce qui a pu être financé de l'instrument (econ_va_shortfall_resolve). */
void  credit_settle_monthly(WorldEconomy *e, const World *w);

/* ---- Lecture (UI/diplo/télémétrie chronicle) ---------------------------------------- */
float credit_debt_class(int c);       /* dette due aux PROPRES classes du pays c (AGRÉGAT elite+bourgeois) */
/* MONNAIE M14 — B5 : LA VENTILATION PAR ORDRE (v96) — ce que CHAQUE ordre a RÉELLEMENT
 * prêté à l'État (et se voit rembourser en proportion, plus les poids fixes ELITE/
 * BOURGEOIS_LEND_WEIGHT qui ne bornent que la CAPACITÉ de prêt). */
float credit_debt_elite(int c);
float credit_debt_bourgeois(int c);
float credit_debt_citystate(int c);   /* dette due à SA cité-état créancière */
float credit_debt_total(int c);       /* to_class+to_cs */
/* M3d — années consécutives au plafond (save_sane la revalide, motif credit_debt_class). */
int   credit_insolvent_streak(int c);
/* Compteurs MONDE, cumulés depuis credit_init (RAZ par partie/sim, non sérialisés —
 * même esprit que econ_money_instrument_get) : rachats de crédit et épisodes
 * d'ÉPUISEMENT (need>0 non couvert par la chaîne complète) observés CE run. */
void  credit_stats_get(long *buybacks, long *defaults);

/* ---- M3d : LA SOUTENABILITÉ + LA BANQUEROUTE (décision joueur 2026-07-15) -----------
 * LE PLAFOND (brief §1) : dette max = DEBT_CEILING_YEARS × revenu annuel nominal (lecture
 * UI/télémétrie — credit_borrow_local/citystate l'appliquent déjà en interne). */
float credit_debt_ceiling(int c);
/* LA BANQUEROUTE (brief §5) : répudiation TOTALE + cicatrice −75 % (bankruptcy_scar,
 * scps_econ.h) sur toutes les provinces du pays. `forced` pilote SEULEMENT la télémétrie
 * (forcée/chronique vs volontaire CMD_BANKRUPTCY) ; retourne l'ex-créancier cité-état
 * (-1 = aucun) — l'appelant applique l'effet diplomatique (DiploState hors scope credit.c). */
int   credit_bankruptcy(WorldEconomy *e, int c, bool forced);
/* Un pays au plafond depuis BANKRUPTCY_GRACE_YEARS années consécutives (posé par
 * credit_year_tick) : scps_sim.c doit appeler credit_bankruptcy(e,c,true) CE tick. */
bool  credit_bankrupt_pending(int c);
void  credit_bankruptcy_stats(long *forced, long *voluntary);

/* ---- M3g : LA BANQUEROUTE-SAISIE (décision joueur 2026-07-15) ------------------------
 * Remplace le malus PLAT −75 % (production/croissance, M3d) par une SAISIE : la
 * production CONTINUE PLEINE (scps_econ.c retire le malus de `out`), mais une part
 * BANKRUPTCY_GARNISH(0.75)×bankruptcy_scar de sa VALEUR — décroissante avec la
 * cicatrice EXISTANTE, aucun nouvel état pour la FRACTION elle-même — est CONFISQUÉE
 * au profit des créanciers D'AVANT-répudiation plutôt que simplement non-produite :
 *   - créanciers DOMESTIQUES (les classes rentières du débiteur lui-même) → crédités
 *     en WEALTH directement par scps_econ.c (province-grain, aucun aller-retour ici) ;
 *   - la CITÉ-ÉTAT créancière (cs_id figé au moment de la répudiation, credit_bankruptcy)
 *     → repli TRÉSOR (valeur — le transport PHYSIQUE des biens saisis vers le stock
 *     étranger est le repli documenté, pas fait ici : complexité hors budget M3g).
 * L'identité du créancier CS et sa PART de la dette totale (le reste = domestique) sont
 * figées au moment de la banqueroute (credit_bankruptcy) — la dette elle-même est déjà
 * répudiée à cet instant (motif M3d, inchangé), donc CETTE mémoire est un état SÉPARÉ
 * (SAVE_VERSION 92 : g_garnish_cs_id/g_garnish_cs_share/g_garnish_cs_pending). */
/* Part (0..1) de la saisie qui va à la cité-état créancière figée pour ce débiteur —
 * 0 si aucun créancier CS n'a été figé à la dernière banqueroute (tout va domestique).
 * Lecture pure (scps_econ.c, dans la boucle de production — aucun World* requis). */
float credit_garnish_cs_share(int debtor_c);
/* Créancier CS figé (-1 = aucun) / cumul en attente (≥0) — lecture pure, réservée à la
 * revalidation save_sane (scps_save.c, motif credit_of/credit_debt_class) et à la télémétrie. */
int   credit_garnish_cs_id(int debtor_c);
float credit_garnish_cs_pending(int debtor_c);
/* Enregistre UNE saisie (scps_econ.c, par ressource/bâtiment/province/mois) : la part
 * domestique est déjà créditée par l'appelant (télémétrie seule ici) ; la part cité-état
 * s'accumule (inter-tick, réglée UNE FOIS/an par credit_year_tick — même cadence que le
 * paiement d'intérêt cs, même point d'entrée : home_reg + province représentative). */
void  credit_garnish_note(int debtor_c, float domestic_value, float cs_value);
/* Télémétrie MONDE cumulée depuis credit_init (RAZ par partie/sim, non sérialisée —
 * motif g_buybacks/g_defaults) : valeur totale saisie, et sa ventilation domestique vs
 * cité-état (brief : « télémétrie : valeur saisie/banqueroute, part domestique/CS »). */
void  credit_garnish_stats(double *total, double *domestic, double *citystate);

/* ---- V1 (MONNAIE M9) : LE VERBE « EMPRUNTER À UN ORDRE » (panneau éco, décision joueur
 * 2026-07-16) — l'État emprunte EXPLICITEMENT à UNE SEULE classe (Laborer/Bourgeois/Élite)
 * de son propre empire ; la classe NE REFUSE JAMAIS (contrairement à V2, un État étranger).
 * Réutilise le socle M3c (credit_borrow_local §2 : capacité CLASS_LEND_SHARE, plafond+
 * tranche M3d) — grain NATIONAL (la dette est dans le pool P1, g_debt ci-dessus), créancier
 * = la strate identifiée (CLASS_ELITE/CLASS_BOURGEOIS ; CLASS_LABORER/CLASS_SLAVE renvoient
 * 0, aucune épargne, motif M3c). */
/* Lecture PURE (aucune mutation) — la capacité DISPONIBLE : montant max empruntable MAINTENANT
 * pour la classe `cls` du pays `c`. Sert le reader façade scps_country_loan_capacity. */
float credit_class_borrow_capacity(const WorldEconomy *e, int c, SocialClass cls);
/* LE VERBE : emprunte `need` (<=0 ⇒ le MAXIMUM disponible) à la classe `cls` du pays `c` —
 * transfert COMPLET (débite la classe, crédite le trésor national via la capitale). Retourne
 * le montant RÉELLEMENT prêté. AUCUN World* requis (province-grain pur, motif credit_borrow_
 * local — econ_country_capital_prov scanne prov[] directement). */
float credit_borrow_class(WorldEconomy *e, int c, SocialClass cls, float need);
/* Lecture PURE — le TAUX courant du pays c (factorisé hors de credit_year_tick, formule M3d
 * §3 INCHANGÉE) : sert le reader façade (taux proposé) SANS dupliquer le calcul. */
float credit_current_rate(int c);

/* ---- V2 (MONNAIE M9) : LA DEMANDE D'EMPRUNT DIPLOMATIQUE (diplomatie, décision joueur
 * 2026-07-16) — le joueur demande un prêt à un ÉTAT ÉTRANGER de son choix (pas l'auto-
 * sélection pick_lender de M3c) ; celui-ci PEUT REFUSER — value SUBJECTIVE évaluée par
 * l'APPELANT (scps_sim.c, ai_consider_offer/OFFER_LOAN, scps_ai.c — relation+confiance+
 * liquidité+éthos, réutilise diplo_relation/statecraft_opinion EXISTANTS). */
/* Assume le consentement DÉJÀ acquis (l'appelant l'a obtenu via ai_consider_offer) — fait
 * SEULEMENT le transfert réel (motif credit_borrow_citystate, mais créancier CHOISI, pas élu ;
 * transfert COMPLET comme credit_borrow_class ci-dessus, pas un débit-seul). Refuse si le
 * débiteur a déjà un AUTRE créancier étranger engagé (motif « un seul créancier-cité-état par
 * pays », M3c). amount<=0 ⇒ le MAXIMUM disponible (motif credit_borrow_class, V1). Retourne
 * le montant RÉELLEMENT prêté (0 = impossible). */
float credit_borrow_state(WorldEconomy *e, const World *w, int debtor_c, int lender_c, float amount);
/* État TRANSIENT de la DERNIÈRE demande (façade/UI — « [État] accorde/refuse », jamais un
 * flottant). Non sérialisé (motif g_buybacks/g_defaults — un fanion d'UI, pas un état de
 * simulation ; le PRÊT lui-même, s'il a été accordé, EST réel/sérialisé via g_debt). */
void  credit_loan_request_note(int debtor_c, int lender_c, bool granted);
int   credit_loan_request_target(int debtor_c);    /* -1 = aucune demande depuis credit_init */
bool  credit_loan_request_granted(int debtor_c);   /* valide seulement si target>=0 */

/* ---- V3 (MONNAIE M9) : LES RACHATS À MÉTABOLISATION DISTINCTE (décision joueur 2026-07-16,
 * « système de rachat de crédit par les cités états et les mercantiles [et pacifistes], leur
 * métabolisation distincte ») — le RACHAT lui-même (M3c, credit_year_tick, « les Fugger »)
 * est INCHANGÉ ; ce qu'il RAPPORTE au racheteur diffère désormais par ARCHÉTYPE : cité-état
 * → influence/vassalité douce (rancor, scps_sim.c a DiploState), mercantile → RIEN de plus
 * que l'intérêt annuel déjà uniforme (son profit PUR, brief), pacifiste → stabilité/relation
 * (faction_lever_apply/FAC_COMMUNAUTAIRE, scps_sim.c a Statecraft). */
enum { LOAN_ARCHETYPE_NONE=0, LOAN_ARCHETYPE_CITYSTATE, LOAN_ARCHETYPE_MERCANTILE, LOAN_ARCHETYPE_PACIFIST };
/* Flag TRANSIENT posé par credit_year_tick (RAZ en tête de CHAQUE appel) : l'archétype du
 * racheteur qui vient d'acquérir la dette-classes de `debtor_c` CETTE année, LOAN_ARCHETYPE_
 * NONE si aucun rachat. Lu par scps_sim.c juste après credit_year_tick (credit.c n'a pas
 * DiploState/Statecraft) pour appliquer l'effet DISTINCT. RRACHAT_META<=0 : kill-switch —
 * reste NONE pour tout le monde (golden pré-M9 byte-identique ; le rachat M3c continue de
 * fonctionner à l'identique, seule la distinction est coupée). */
int   credit_buyback_archetype(int debtor_c);
/* Télémétrie MONDE cumulée depuis credit_init (motif g_buybacks/g_defaults) : rachats PAR
 * ARCHÉTYPE — la preuve chiffrée que les 3 métabolisations sont bien distinctes. */
void  credit_buyback_stats(long *cs, long *mercantile, long *pacifist);

#endif /* SCPS_CREDIT_H */
