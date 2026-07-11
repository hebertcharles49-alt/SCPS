#ifndef SCPS_MISSIONS_H
#define SCPS_MISSIONS_H
/*
 * scps_missions.h — LES MISSIONS DÉCENNALES (factions §8)
 *
 * Tous les 10 ans, une mission est émise par pays — CONTEXTUELLE (tirée de l'état /
 * de l'éthos dominant / des besoins) : bâtir une institution, renforcer une chaîne
 * de production, percer des technologies. L'accomplir verse OR + MATIÈRES (∝ ampleur)
 * — un rythme, une direction, une injection de ressources, décennie après décennie.
 * Maintenant que bâtir COÛTE (l'or et les matériaux comptent), la récompense pèse.
 *
 * Passif : la mission s'émet et se récompense seule à l'accomplissement (l'IA, qui
 * bâtit/recherche naturellement, en complète ; le joueur y lit un but). Diégétique :
 * la mission porte un texte (membrane), jamais une coordonnée SCPS.
 */
#include "scps_world.h"   /* World, WorldEconomy */
#include "scps_econ.h"    /* RegionEconomy, Resource, ProvBuild */
#include "scps_tech.h"    /* TechState */
#include "scps_statecraft.h" /* P3 — Statecraft/WorldProsperity : le siège responsable */

typedef enum { MIS_NONE = 0, MIS_BUILD, MIS_CHAIN, MIS_TECH } MissionKind;

/* Les six coordonnées bâties adressables par MIS_BUILD (cf. ProvBuild) — exposées
 * (P3) pour que le siège responsable (mission_responsible_seat) et les bancs
 * puissent les nommer, plus une énumération privée du .c. */
enum { MIS_COORD_K=0, MIS_COORD_PE, MIS_COORD_FAITH, MIS_COORD_SAVOIR, MIS_COORD_H, MIS_COORD_FOOD };

typedef struct {
    MissionKind kind;
    int      coord;        /* MIS_BUILD : quelle coordonnée bâtie (MIS_COORD_*) */
    Resource good;         /* MIS_CHAIN : le bien à produire */
    float    threshold;    /* la cible (coordonnée bâtie / stock / nb de techs) */
    int      issued_year;
    bool     active, done;
    float    reward_gold;
    Resource reward_mat;   /* un lot de matières en récompense */
    float    reward_qty;
    char     text[80];     /* la mission, en mots (membrane) */
    /* raccord 7 (Âge des Héros) — vrai PENDANT le tour de missions_tick où cette
     * mission vient d'être accomplie (RAZ en tête de chaque missions_tick) : le
     * signal que scps_sim.c lit pour tester l'éligibilité au héros (rang III +
     * efficacité + loyauté + encore assis) SANS coupler ce module aux évènements. */
    bool     just_completed;
} Mission;

/* raccord 7 — le bonus d'un héros consacré (« Lui confier »/« Lui donner les
 * clefs ») s'attache au SIÈGE, identifié par (slot,gen) : si le titulaire change
 * avant la PROCHAINE mission routée sur ce siège, le successeur ne le reçoit PAS
 * (« sans le bonus »). mult<=0 = aucun bonus en attente. */
typedef struct { float mult; int8_t slot, gen; } HeroMissionBonus;

#define SCPS_MISSIONS_MAX SCPS_MAX_COUNTRY
typedef struct {
    Mission m[SCPS_MISSIONS_MAX];
    HeroMissionBonus hero_bonus[SCPS_MISSIONS_MAX][SC_COUNCIL_SEATS];
} MissionsState;

void missions_init(MissionsState *ms);

/* P3 — le SIÈGE du Conseil RESPONSABLE de la mission courante (0 Savoir/1 Royaume/
 * 2 Ouvrages), DÉDUIT du type — aucun état neuf (le successeur reprend : on relit
 * le siège pourvu à chaque grant/pénalité, jamais un id de ministre figé) :
 * MIS_TECH→Savoir · MIS_CHAIN→Ouvrages · MIS_BUILD savoir/foi→Savoir ·
 * institutions/garde/vivres→Royaume · commerce→Ouvrages. -1 si aucune mission. */
int mission_responsible_seat(const Mission *m);

/* Appelé une fois par AN : à chaque décennie émet une mission contextuelle par pays
 * (une mission ACTIVE et INACHEVÉE à ce moment est un ÉCHEC — RÉSOLU ici, AVANT
 * l'émission de la suivante : pénalise la loyauté du siège responsable) ; chaque
 * an, vérifie l'accomplissement et verse la récompense (or au trésor de la capitale
 * + matières au marché, bonus ∝ rang×efficacité du siège responsable — P3) et la
 * loyauté (+succès/−échec). `sc`/`wp` peuvent être NULL (aucun bonus/loyauté —
 * mission neutre, comme avant le Conseil vivant). */
void missions_tick(MissionsState *ms, const World *w, WorldEconomy *econ,
                   const TechState *ts, Statecraft *sc, const WorldProsperity *wp,
                   uint32_t seed, int year);

/* Lecture membrane (UI) : la mission courante d'un pays (NULL si aucune). */
const Mission *mission_of(const MissionsState *ms, int cid);

#endif /* SCPS_MISSIONS_H */
