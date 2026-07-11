#ifndef SCPS_FOG_H
#define SCPS_FOG_H
/*
 * scps_fog.h — LE BROUILLARD DE GUERRE : la CONNAISSANCE que chaque empire a du
 * monde. ÉTAPE 1/2 : ce module CALCULE la connaissance (radius 2, cumulative) et
 * expose le lecteur public + le masque VISUEL du joueur — mais AUCUNE décision de
 * simulation ne la lit ici (le câblage IA/diplo sur cette connaissance est une
 * mission séparée, plus tard). `country_knows` est fourni prêt à être câblé mais
 * n'est appelé nulle part dans scps_ai.c/scps_diplo.c pour l'instant : golden et
 * déterminisme restent intacts PAR CONSTRUCTION (fog_update ÉCRIT un état que rien
 * ne LIT en aval dans la sim).
 *
 * MODÈLE (décidé par le joueur) :
 *  - Vision d'un empire = ses régions + les régions à ≤2 sauts d'adjacence (grain
 *    RÉGION, WorldEconomy.adj[][], booléenne — la même adjacence que la colonisation
 *    et le réseau de hubs commerciaux).
 *  - Découverte = SUR LE TERRAIN, CUMULATIVE : dès qu'une région d'un empire B entre
 *    dans le radius 2 d'un empire A, A connaît B POUR TOUJOURS (jamais d'oubli —
 *    known[][] ne décroît jamais, seul fog_reset() le remet à plat).
 *  - Calculée au tick ANNUEL (comme demography_contact_tick/migration_pact_tick/
 *    refugee_tick) + UNE fois à la genèse (sim_init, même fonction, pas de cas
 *    spécial « an 0 ») : le joueur voit déjà ses voisins au jour 0 plutôt que de
 *    rester aveugle une année entière.
 */
#include "scps_world.h"
#include "scps_econ.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define FOG_RADIUS 2

/* RAZ (nouvelle partie/sim) — motif religion_reset/decrees_reset : appelé par
 * sim_init, AVANT le premier fog_update. */
void fog_reset(void);

/* BFS radius-2 PAR EMPIRE VIVANT (regions_of(econ,a)>0), depuis SES régions sur
 * econ->adj[][] : toute région ATTEINTE (quel qu'en soit l'owner) marque
 * known[a][owner]=1 — CUMULATIF (jamais effacé hors fog_reset). Un empire se
 * connaît toujours lui-même. Appelée au tick annuel + une fois à sim_init. */
/* `radius_bonus` (raccord Découvertes, docs/AGES_FINS_2026-07-11.md : « rayon du
 * brouillard +1 saut ») s'ajoute AU RADIUS 2 de base — 0 = comportement d'origine.
 * L'appelant le calcule (`ages_dawned(ev,AGE_DISCOVERY)?1:0`) : fog n'a jamais
 * besoin de connaître les Âges. */
void fog_update(const World *w, const WorldEconomy *econ, int radius_bonus);

/* LECTURE publique — a connaît-il b (b existe-t-il aux yeux de a) ? Le helper que
 * l'orchestrateur câblera plus tard dans l'IA/diplo (guerre/diplomatie filtrées
 * par la connaissance) — INUTILISÉ ailleurs dans la sim pour l'instant. */
bool country_knows(int a, int b);
/* raccord 6 — le ratio de paires de pays VIVANTS qui se connaissent (ordonné,
 * a≠b) : Σcountry_knows(a,b) / Σpaires. Alimente le déclencheur des Découvertes
 * (35 %) ET un readout (scps_api.c). */
float country_known_pair_share(const World *w);

/* Régions VISIBLES pour `viewer_cid` MAINTENANT (dérivé à la volée, rien n'est
 * mémorisé) : {ses régions} ∪ {BFS radius-2 à la volée} ∪ {toute région dont
 * l'owner est CONNU de viewer_cid, cf. country_knows — cumulatif}. `out_region`
 * doit pointer ≥ SCPS_MAX_REG octets (0/1). viewer_cid<0 (ou w/econ NULL) ⇒ tout
 * à 1 (aucun voile — chronique/viewer sans joueur humain). Pur lecteur (aucun
 * état modifié) : LE masque VISUEL, jamais lu par une décision de sim — motif
 * map_state_tint (scps_api.c) : calculé UNE fois, consommé en boucle. */
void fog_visible_regions(const World *w, const WorldEconomy *econ, int viewer_cid,
                         int radius_bonus, uint8_t *out_region);

/* section FOGV du save (motif WILD/DCRE : sim_wild_save/decrees_save — un tag
 * NULL,0 dans scps_save.c puis cet appel dédié). fog_load() RAZ avant de lire. */
void fog_save(FILE *f);
bool fog_load(FILE *f);

/* BANC/FUZZ SEULEMENT (motif intertrade_debug_set_hub_of) : force `a` à connaître
 * TOUT le monde. Les bancs de verbes diplo testent la PLOMBERIE (journal → drain →
 * relations), pas l'exploration — sans ça, un joueur passif d'un monde clairsemé
 * ne rencontre légitimement personne (radius 2) et tout verbe est « cible
 * inconnue ». JAMAIS appelé par la sim. */
void fog_debug_meet_all(int a);

#endif /* SCPS_FOG_H */
