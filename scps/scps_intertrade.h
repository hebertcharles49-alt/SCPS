#ifndef SCPS_INTERTRADE_H
#define SCPS_INTERTRADE_H
/*
 * scps_intertrade.h — LE COMMERCE INTER-PAYS (les grandes routes marchandes)
 *
 * Hiérarchie : marché régional (autarcie) → commerce inter-régional (scps_trade,
 * intra-pays + frontières adjacentes) → COMMERCE INTER-PAYS (ici).
 *
 * Les grandes routes (scps_routes) ne portaient que du PE (prospérité). Ici elles
 * portent des GOODS sur de longues distances ENTRE PAYS : un royaume importe le
 * bien stratégique qui lui manque d'un partenaire lointain (arbitrage cheap→cher,
 * plafonné par la route) ; l'exportateur encaisse l'OR. → des empires marchands.
 *
 * Couplage géopolitique (la pièce qui manquait) :
 *   - EMBARGO : aucune route ne porte de goods entre deux pays EN GUERRE
 *     (guerre commerciale : on prive l'ennemi de son accès aux ressources).
 *   - Cela donne des DENTS au casus belli économique : un bien monopolisé qu'on
 *     ne peut plus importer → il faut CONQUÉRIR la source.
 *
 * Membrane : ce module est SIM. Les lecteurs renvoient des nombres tangibles (or,
 * volume) ; la traduction en mots reste à la membrane.
 */
#include "scps_econ.h"
#include <stdio.h>
#include "scps_routes.h"
#include "scps_diplo.h"

/* Un pas de commerce inter-pays : pour chaque route OUVERTE entre régions de PAYS
 * DIFFÉRENTS et NON en guerre, arbitre les biens (le cher s'approvisionne au bon
 * marché, plafonné par la route) ; l'exportateur encaisse l'or, les prix
 * convergent. À appeler après routes_advance(). `dp` peut être NULL (pas d'embargo). */
void intertrade_tick(WorldEconomy *e, const RouteNetwork *rn, const DiploState *dp);

/* Lecteurs (IA / UI) — sur le dernier tick. */
float intertrade_imports_value(const WorldEconomy *e);   /* valeur totale échangée au dernier tick */
/* COMMERCE ASYMÉTRIQUE : volumes du dernier tick par SENS (aval = le sens
 * facile), et leur composition vrac (grain/bois/charbon/bétail) vs précieux
 * (étoffe précieuse/orfèvrerie/remèdes/armes). */
void intertrade_asym_stats(float *vdown, float *vup,
                           float *bulk_down, float *bulk_up,
                           float *prec_down, float *prec_up);
int  intertrade_precious_upstream_events(void);   /* la niche du luxe : il REMONTE */
int   intertrade_active_routes(const WorldEconomy *e, const RouteNetwork *rn,
                               const DiploState *dp, int cid);  /* routes marchandes vivantes d'un pays */

/* ---- DÉTAIL par pays × bien (sidebar Import/Export) — nombres de jeu --------
 * Accumulé par intertrade_tick (dernier tick = l'année écoulée) : volumes
 * importés/exportés par bien, partenaire DOMINANT du flux, or encaissé à
 * l'export, et la valeur échangée par PAIRE (le coût d'un embargo, lisible
 * AVANT de le décréter). Tout est tangible : volume, or, identifiants. */
float intertrade_import_vol (int cid, int good);
float intertrade_export_vol (int cid, int good);
/* P3.20/E3 — valeur du commerce PASSÉE PAR CE CENTRE au dernier tick (chaque
 * échange crédite le Centre de chacun des deux pays pour moitié). Sert à lire
 * la part des cités-états dans le commerce mondial (les premiers hubs vivants). */
float intertrade_centre_value(int region);
int   intertrade_import_from(int cid, int good);   /* pays-source dominant (-1 si aucun) */
int   intertrade_export_to  (int cid, int good);   /* pays-client dominant (-1 si aucun) */
float intertrade_export_gold(int cid);             /* or encaissé à l'export (dernier tick) */
float intertrade_pair_value (int cid, int other);  /* valeur échangée avec ce partenaire */

/* ---- EMBARGO DÉCRÉTÉ (décision joueur/IA) -----------------------------------
 * En sus de l'embargo de guerre (automatique) : un pays peut DÉCRÉTER l'embargo
 * contre un autre — aucune route ne porte de goods entre eux tant qu'il tient.
 * intertrade_reset() remet embargos & flux à zéro (init de chaque partie/sim). */
void  intertrade_order_embargo(int cid, int target, bool on);
bool  intertrade_embargoed    (int cid, int target);   /* l'un OU l'autre a décrété */
void  intertrade_reset(void);

/* ---- CENTRES COMMERCIAUX (P3.20) — hubs du réseau inter-régional ------------
 * Un par batch de ~4-5 régions, planté là où le FLUX est le plus fort (carrefour
 * + côte). Un pays sans Centre commercial dans son territoire est COUPÉ du réseau
 * inter-pays — il faut en conquérir un. À semer après econ_init (géographique). */
void  intertrade_seed_centres   (const World *w, WorldEconomy *e);   /* M2 : pose le bâti EDI_TRADE_CENTER sur la meilleure région de chaque cité-état */
bool  intertrade_has_centre     (int region);                       /* cette région est-elle un hub ? */
bool  intertrade_country_has_centre(const WorldEconomy *e, int cid);/* ce pays tient-il un hub ? */
bool  intertrade_has_global_access(int cid);                        /* M3 : Centre propre OU pacte commercial avec un porteur (cache 1×/tick) */
int   intertrade_country_centre (const WorldEconomy *e, int cid);   /* 1re région-hub du pays (-1) */
bool  intertrade_relocate_centre(WorldEconomy *e, int from, int to);/* déplace le BÂTIMENT-hub (coût: appelant) */
/* #5 — LE PUMP À 2 ÉTAGES (marché LOCAL cité-état la plus proche → marché MONDIAL via
 * Centre, double taxe). Les achats touchent de VRAIS stocks (ils ne pompent pas dans
 * le vide) : `intertrade_buy_cost` DEVISE le sourcing d'un bien (stock propre ×1 →
 * Centre local ×marge → autres Centres ×marge×2), `intertrade_market_consume` le
 * DÉPLÉTÉ dans le même ordre. `intertrade_region_hub` : le Centre de rattachement
 * (-1 = autarcie). `intertrade_global_stock` : profondeur du marché mondial. */
float intertrade_buy_cost      (const WorldEconomy *e, int region, int good, float qty, float unit_price, float *import_base_out);
void  intertrade_market_consume(WorldEconomy *e, int region, int good, float qty, float unit_price);
/* F-arc — POMPE D'ARMES : propre GRATUIT → Centres ÉTRANGERS = ACHAT borné par le trésor (nu→source,
 * marge→hôte) ; RENVOIE le total prélevé. La levée (econ_arms_take) s'arme au marché quand le stock manque. */
float intertrade_market_pull   (WorldEconomy *e, int region, int good, float want, float unit_price);
/* F-arc — chaque CITÉ-ÉTAT naît ARMURIER : une manufacture d'armes ALÉATOIRE (graine×index) sur son
 * Centre, intrant brut garanti. Les empires y POMPENT leurs armes spécialisées. À semer APRÈS _centres. */
void  intertrade_seed_citystate_arms(const World *w, WorldEconomy *e);
int   intertrade_region_hub    (int region);
float intertrade_global_stock  (const WorldEconomy *e, int good);
float intertrade_market_avail  (const WorldEconomy *e, int region, int good);   /* dispo au marché atteignable (gate de matière) */
/* ACTIONNEUR joueur (UI) — achat/vente direct au marché à 2 étages (tier 0 = régional /
 * Centre le plus proche ; tier 1 = mondial / réseau, exige un Centre du pays). L'achat
 * débite le trésor au prix courant×marge (×2 au mondial), crédite le stock, DÉPLÉTÉ le
 * marché — borné par le disponible ET le trésor. La vente est l'inverse (au prix, sans
 * marge). Renvoient les unités échangées ; *or = l'or débité (achat) / encaissé (vente). */
long  intertrade_market_buy (WorldEconomy *e, int region, int good, long want, int tier, long *spent);
long  intertrade_market_sell(WorldEconomy *e, int region, int good, long want, int tier, long *gained);
/* sauvegarde (shell §6) : le module possède sa sérialisation — embargos décrétés
 * (les flux du dernier tick se recalculent, eux). */
void  intertrade_save(FILE *f);
bool  intertrade_load(FILE *f);

#endif /* SCPS_INTERTRADE_H */
