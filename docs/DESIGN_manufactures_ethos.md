# DESIGN — Manufactures signature d'éthos + désir croisé + expéditions (à venir)

Statut : **conçu, non implémenté.** À faire APRÈS le brouillard de guerre (le verbe « financer les
expéditions » en dépend). Décisions tranchées avec le joueur le 2026-07-09.

## Vision — le moteur d'interdépendance culturelle

« Il manque un truc qui drive le monde. » Chaque culture fabrique un bien de luxe **unique** que sa
culture **opposée** désire — donc tout le monde a besoin de tout le monde. Ça pousse le commerce, la
métabolisation (digérer l'autre pour apprendre son artisanat) et l'échange. Un moteur d'échange
culturel, pas juste des ressources fongibles.

## 1. Les 6 manufactures signature — indexées par ÉTHOS

Un bien par éthos. L'axe ordre↔chaos donne les **opposés naturels** (`scps_culture.h:36`, « §8 éthos
opposés ») → le désir croisé est gratuit :

| Éthos (producteur) | Manufacture (nom à valider) | Désirée par (opposé) |
|---|---|---|
| **Dominateur** (9) | Heaumes de guerre | **Pacifiste** |
| **Honneur** (7.5) | Parures de gloire | **Mercantile** |
| **Ordre** (6) | Horloges réglées | **Bureaucrate** |
| **Bureaucrate** (4.5) | Registres scellés | **Ordre** |
| **Mercantile** (3) | Colifichets exotiques | **Honneur** |
| **Pacifiste** (1.5) | Ouvrages d'agrément | **Dominateur** |

(Les noms reprennent l'esprit des exemples — colifichets/bottes/horloges — remappés sur les éthos ;
à ajuster librement, c'est du flavor.)

## 2. Le désir croisé — chaque manufacture nourrit 2 besoins de l'opposé

La manufacture de l'éthos X est un **besoin de confort** pour les pops de l'éthos **opposé**, à DEUX
niveaux : un besoin **Laborer** et un besoin **Élite**. Elle s'ajoute au panier (`NEED[CLASS][RES]`,
`scps_econ.c`) des pops dont la culture porte l'éthos opposé — jamais un bonus plat, un vrai besoin
qui pèse sur la satisfaction (comme poterie/statuaire). Bonus hors-panier façon `COMFORT_JOY` possible.

## 3. La métabolisation débloque la PRODUCTION — palier 1 / palier 3

« La métabolisation rapporte ces manufactures, palier 1/3 (laborer/élites). » En digérant une diaspora
qui porte l'éthos X (mesure par éthos, analogue à `econ_country_heritage_digested` mais sur l'axe
éthos de la culture des groupes) :
- **Palier 1** (`METAB_TIER1` ≈ 0.10 digéré) → tu peux produire la version **Laborer** de la manufacture X.
- **Palier 3** (`METAB_TIER3` ≈ 0.35 digéré) → tu débloques aussi la version **Élite**.

Tu apprends l'artisanat du peuple que tu assimiles, et tu le revends à ceux qui le désirent (l'opposé).

⚠ **Point à câbler** : un helper `econ_country_ethos_digested(cid, ethos)` (le pont éthos, les PopGroup
portent une culture avec `ethos`). Le natif produit-il d'emblée SA propre manufacture (éthos de sa
capitale) ? — à confirmer (probablement oui : palier natif = plein).

## 4. Verbe statecraft « Financer les expéditions » (lève le brouillard)

Une décision `scps_decrees`/statecraft (player + IA), **dépend du fog** :
- **Coût** : `0.05 × revenu mensuel × IPM` (par expédition, mensuel tant qu'active).
- **Effet** : une expédition **révèle les tuiles progressivement, 1 par mois, en cercles concentriques**
  (spirale croissante depuis un point d'ancrage — la capitale ou une frontière). Quand elle atteint une
  région d'un empire inconnu → **découverte** (le pont vers le système de connaissance du fog).
- **Cap** : **3 expéditions recrutables max** simultanément.
- Flavor : « lever le brouillard ».

## 5. Traductions & prix

- Toutes les nouvelles manufactures + le verbe expéditions naissent en `STR_*` (FR + EN, les deux tables).
- L'ajout de `RES_*` (6 manufactures) + `BLD_*` (6 bâtiments) fait grandir `RegionEconomy` ⇒ **SAVE bump**.

## Séquençage & risques

1. **Le fog d'abord** (en cours) — l'infra de connaissance + le câblage décision IA.
2. **Puis ce système** : les 6 manufactures (éco/recettes) → le désir croisé (panier) → la métabolisation
   (pont éthos) → le verbe expéditions (adossé au fog).
3. ⚠ **Re-baseline** attendue (nouveaux besoins + production mordent dès l'an-0) + **calibrage** (le désir
   croisé ne doit pas affamer les pops au début, quand personne n'a métabolisé personne — prévoir que la
   manufacture soit un besoin de CONFORT tardif, pas vital, débloqué par le niveau de capitale comme le
   reste du panier progressif `NEED_ORDER`).
