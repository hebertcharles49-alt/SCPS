# Refonte PROVINCE — le modèle EU4 (charte du chantier `feat/province-economy`)

> Décidé avec l'auteur (2026-07-01). Ce fichier est la **north star** du re-key.
> Si un choix d'implémentation contredit ce tableau, c'est le tableau qui gagne.
>
> ✅ **STATUT : COMPLET & VALIDÉ (2026-07-01)** — bancs 38/38 faisables verts (3 KO
> Windows pré-existants), déterminisme STABLE, golden re-baseliné, sweep 5×250 SAIN
> (satisfaction 73/82/79 · commerce 3834/an · §27 gaté an-180). Sur la branche, pas
> encore mergé sur main. Raffinement futur connu : `region.stock` agrégé comme le
> treasury (écritures stock d'intertrade partiellement perdues ; le monde reste sain).

## Les trois étages

| Tier | Rôle | Ce qui y VIT (source de vérité) |
|------|------|--------------------------------|
| **PROVINCE** | l'unité individualisée (la cellule qu'on clique) | pop · strates · culture locale · **exactement 2 ressources brutes** · bâtiments · production · stock · croissance · colonisé · propriétaire |
| **RÉGION** | groupement politique de provinces ; **reflète stab + prospérité** | agrégats **calculés depuis ses provinces** : prospérité · satisfaction · **marché** (prix/offre/demande, pool des provinces) · légitimité · agitation · **guerre/diplo/commerce/endgame** (grain politique/militaire) |
| **PAYS** | le propriétaire | **aucune incidence propre** — pas de stab/prospérité « pays ». L'UI n'affiche qu'un **roll-up** (or, population, nombre de provinces, savoir) |

## Règles dures

1. **L'économie brute descend à la province.** `strata`, `pop`, `culture`, `build`, `raw_cap[2]`,
   `bld[]`, `stock[]`, trésor local, cicatrices, faustien, geo → **par province**.
2. **La région AGRÈGE.** Prospérité, satisfaction, marché (prix/offre/demande), légitimité,
   agitation = calculés en **sommant/pondérant les provinces de la région**. La région ne
   produit rien elle-même ; elle *reflète*.
3. **Le pays n'a aucune incidence de simulation.** Il possède des provinces, le joueur le
   commande (coloniser, bâtir), mais il n'a pas de stab/prospérité émergente propre. Les
   nombres « pays » de l'UI sont des roll-ups d'affichage.
4. **La guerre, la diplo, le commerce, l'endgame restent au grain RÉGION** (le groupement
   politique). On ne les descend PAS à la province (l'auteur : « groupements politiques »).
5. **Colonisation** : départ = **1 province** · le joueur colonise **n'importe quelle**
   province · la cité-état colonise **sa région** · pas d'exception.
6. **2 ressources par province** (`REGION_RAW_KEEP` → 2, appliqué par province).
7. **Pop de départ (an-0) — CHAQUE entité démarre sur UNE province développée, JAMAIS
   de split** : empire = **4000 hab** sur sa capitale · cité-état = **2000** sur SA
   capitale · hameau sauvage = **750**. Toutes les AUTRES provinces de l'entité (y compris
   la cité-état sur SA région) naissent **VIERGES** (pop 0, colonized=false) et se
   colonisent ensuite. Le pool ne se splitte PAS entre les provinces d'une région au t=0.
   Tunables EXISTANTS (déjà à ces valeurs) : `EMPIRE_SEED` 4000 · `CITY_SEED` 2000 ·
   `WILD_POP` 750. Réparti sur les strates (laboureurs/bourgeois/noblesse) au ratio existant.

## Conséquences assumées

- **Save v46→v47, incompatible** (structs re-dimensionnées).
- **Re-baseline TOTAL** : registre J re-tuné, 40 bancs recalibrés, golden re-baseliné,
  sweep 5×250 revérifié sain. Plusieurs passes de calibrage — ce n'est pas un one-shot.
- La membrane agrège déjà province→région→pays : les readouts suivent.

## Ordre de build (étagé, livré d'un bloc sur la branche)

1. **Struct** — `ProvinceEconomy` (économie brute, `[SCPS_MAX_PROV]`) + `RegionEconomy` slim
   (agrégats + marché). Save v47 + `save_sane`.
2. **Worldgen / colonisation** — semis par province ; 2 ressources/province ; départ 1
   province ; règles de colonisation.
3. **Éco tick** — extraction/manufacture/conso **par province** ; la région somme ses
   provinces pour prospérité/satisfaction/marché.
4. **État régional** — légitimité, agitation, révolte lisent l'agrégat région (leur source
   descend des provinces).
5. **Membrane** — `province_info` lit la province direct ; readouts région = agrégat ;
   readouts pays = roll-up (aucune incidence).
6. **Calibrage** — re-tune registre J → golden + 40 bancs + sweep sain.
7. **UI Godot** — les 14 points (panneau province : bâtiments/hovers/non-colonisé=terrain+
   ressource · top bar roll-up · couleurs d'empire · « 9 » nommé · zoom départ · vue figée
   en pause · créateur culture à déroulants).
