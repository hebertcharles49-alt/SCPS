/*
 * scps_world.h — API de génération de monde
 *
 * Point d'extension : pour ajouter une couche de génération, implémenter
 * une fonction `step_XXX(World*)` dans scps_world.c et l'appeler dans
 * world_generate() entre les étapes existantes.
 */
#ifndef SCPS_WORLD_H
#define SCPS_WORLD_H

#include "scps_types.h"
#include "scps_econ.h"   /* gen_population/world_tick écrivent RegionEconomy.culture */

/* Réglages par défaut (monde « standard ») pour une graine donnée. */
WorldParams worldparams_default(uint32_t seed);

/* Génère un monde complet (géographie seule) selon les paramètres. ~200ms. */
void world_generate(World *w, const WorldParams *params);

/* Intensité agricole [0..10] d'un biome — source unique de vérité partagée
 * avec l'axe de subsistance culturel (cf. lifeway_subs). Sert de proxy de
 * capacité d'accueil dans econ_init. */
float subsistance_for_biome(Biome b);

/* Peuple la culture de chaque région (PopCulture) à partir du biome dominant,
 * de l'éthos tiré, de la latitude (branche religieuse) et des proto-familles
 * linguistiques ancrées sur les continents. À appeler APRÈS econ_init (les
 * régions doivent exister). */
void gen_population(World *w, WorldEconomy *econ);

/* Avance les processus lents liés au monde+population d'un pas dt : pour
 * l'instant la dérive de l'horloge linguistique des régions peuplées. */
void world_tick(World *w, WorldEconomy *econ, float dt);

/* Assigne une race à chaque pays en GRADIENT autour du joueur (distance de
 * sphère ~ distance géographique) ; cités-états = isolats exotiques. Pose la
 * race sur RegionEconomy.culture.race. À appeler après gen_population. */
void worldgen_seed_peoples(World *w, WorldEconomy *econ, SpeciesArchetype player_race);

/* Utilitaires biome */
uint32_t biome_base_color(Biome b);
const char *biome_name(Biome b);

/* Utilitaires ressource */
const char *resource_name(Resource r);
uint32_t    resource_color(Resource r);

/* Palette de provinces — couleur ARGB stable par id */
uint32_t province_palette(int id);
/* P1.5 — couleur d'empire par FAMILLE DE RACE (teinte) + variante par pays. */
uint32_t country_race_color(SpeciesArchetype race, int cid);
/* P1.9 — nom d'empire procédural = f(race, ethos), déterministe par pays. */
void country_make_name(char *out, int n, SpeciesArchetype race, Ethos ethos, int cid);
/* P3.20 — TOPONYME procédural (lieu) : syllabaire de la RACE, sans épithète d'éthos. */
void place_make_name(char *out, int n, SpeciesArchetype race, uint32_t seed);

/* ── LA MER (brief mer §4) : le mouvement directionnel sur le champ de courants ──
 * coût(tuile, direction) = base / (1 + k·max(0, v̂·d̂)) × (1 + m·max(0, −v̂·d̂)) ;
 * eaux mortes ×P ; cabotage = constante (sûr, lent, indifférent aux courants).
 * Conséquence à NE PAS rater : l'aller ≠ le retour (la volta émerge du champ). */

/* Jours de mer entre deux cellules MARINES (Dijkstra directionnel, 8 voisins).
 * < 0 si injoignable (bassins séparés / cellule terrestre). */
float world_sea_days(const World *w, int ax, int ay, int bx, int by);
/* Variante BORNÉE : explore au plus cap_days jours (cap_days<0 = sans borne, exact).
 * Au-delà du rayon → -1. Identique à world_sea_days pour toute cible à ≤ cap_days. */
float world_sea_days_capped(const World *w, int ax, int ay, int bx, int by, float cap_days);

/* Ancre marine d'une RÉGION (l'avant-port) : la cellule de mer adjacente à la
 * côte de la région, la plus proche du germe de sa meilleure province côtière.
 * DÉRIVÉE du monde (cache interne par seed — rien à sérialiser).
 * false si la région n'a aucune côte. */
bool world_region_sea_anchor(const World *w, int region, int *sx, int *sy);

/* ── WG (worldgen-graphe) — LES DÉTROITS ÉMERGENTS (chokepoints) ──────────────
 * Là où un sea_days COURT pince DEUX masses terrestres, la mer se RÉTRÉCIT en un
 * goulet : qui tient la terre qui le flanque tient le passage. Détectés à la
 * GENÈSE par la seule FORME (un bras d'eau étroit entre deux continents/côtes),
 * DÉRIVÉS du monde (cache par seed — rien à sérialiser, comme les ancres). Chaque
 * détroit porte (a) un PÉAGE-au-tenant — le pays qui possède la région-flanc
 * encaisse un droit sur le trafic maritime qui le franchit — et (b) une VALEUR DE
 * BLOCUS (l'enjeu d'y mouiller : plus le chenal est étroit, plus le verrou tient). */
typedef struct {
    int16_t sx, sy;         /* la cellule de mer du goulet (le point le plus étroit) */
    int16_t region;         /* la région-flanc qui le contrôle (-1 si vierge) — le TENANT */
    int16_t width;          /* largeur du chenal en cellules (petit = stratégique)        */
    float   blockade;       /* valeur de BLOCUS [0..1] : croît avec l'ÉTROITESSE du chenal */
} Chokepoint;

/* La table des détroits du monde (construite paresseusement par seed). Renvoie le
 * nombre ; *out (option) pointe la table interne (lecture seule, valide jusqu'au
 * prochain seed). */
int  world_chokepoints(const World *w, const Chokepoint **out);
/* Le détroit (index) que FRANCHIT une route maritime entre les ancres (ax,ay)→(bx,by) :
 * son goulet est proche du segment ET « entre » les deux bouts. -1 si la route n'en
 * croise aucun. (Le plus étroit gagne si plusieurs.) */
int  world_route_chokepoint(const World *w, int ax, int ay, int bx, int by);
/* Le TENANT actuel d'un détroit = le pays propriétaire de sa région-flanc (-1 si
 * vierge / index hors borne). `owner_of_region` mappe région→pays (econ->region.owner). */
int  world_chokepoint_holder(const World *w, int choke_idx,
                             const int16_t *owner_of_region, int n_regions);

#endif /* SCPS_WORLD_H */
