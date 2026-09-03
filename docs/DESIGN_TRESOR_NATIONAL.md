# TRÉSOR ET STOCK NATIONAUX — design (décision joueur 2026-09-03)

## La décision

**UN trésor par empire. UN stock par empire et par ressource.** La banque et l'entrepôt
par province étaient un choix d'implémentation jamais tranché par le joueur ; il les refuse.

Motif : un hégémon à 90 000 or *dispersés* était « ruiné » à sa capitale — la solde
(`scps_warhost.c`), les décrets (`scps_decrees.c`), les chantiers (`scps_agency.c`)
débitaient UNE province. Le pool national des stocks était déjà un prélèvement *simulé*
(`econ_country_stock_take` drainait les provinces une à une). Le crédit lisait une Σ
région périmable. Trois vérités concurrentes pour un seul or : le plat de spaghettis.

## Ce qui disparaît

- `ProvinceEconomy.treasury`, `ProvinceEconomy.stock[]`
- `RegionEconomy.treasury`, `RegionEconomy.stock[]`
- `econ_region_treasury_add`, `econ_prov_treasury_credit`, `econ_region_stock_add`
  (fonctions de miroir / dual-write « B6 · motif M11-A2 »)

Pas de caisse locale résiduelle, pas de dual-write, pas de projection.

## Ce que la province GARDE

pop/strates/groupes · bâtiments · allocation de main-d'œuvre · production &
consommation (`supply[]`/`demand[]`) · prix projeté (déjà national) · culture ·
raws · richesse des classes (`strata[].wealth` — la bourse des POPS, pas celle de
l'État). La région reste une vue reconstruite chaque clôture.

## Le stockage

Dans `WorldEconomy`, au même patron que `reserve_gold[]` / `va_country_prev[]` :

```c
float nat_treasury[SCPS_MAX_COUNTRY];              /* or NET (négatif = dette) */
float nat_stock   [SCPS_MAX_COUNTRY][RES_COUNT];   /* l'entrepôt national */
```

**AUCUN plafond.** Ni sur l'or, ni sur les stocks. Le frein reste économique — prix,
solde, entretien, panier — jamais une borne dure. L'ancien cap de stock provincial
(200 + 500/Entrepôt) disparaît avec le champ ; l'Entrepôt garde ses autres effets.

## Les accès (une seule porte chacun)

| Fonction | Rôle |
|---|---|
| `econ_country_gold(e,c)` | lecture directe de `nat_treasury[c]` |
| `econ_nation_gold_add(e,c,d)` | crédit/débit **borné** au disponible ; rend l'appliqué (jamais de dette fantôme) |
| `econ_nation_gold_force(e,c,d)` | débit **non borné** (dette RÉELLE, philosophie `credit_spend`) |
| `econ_country_stock_sum(e,c,r)` | lecture directe de `nat_stock[c][r]` |
| `econ_country_stock_take(e,c,r,n)` | prélèvement borné ; rend le pris |
| `econ_nation_stock_add(e,c,r,d)` | dépôt (production) / débit borné |

`econ_empire_stock` devient un simple cast de `econ_country_stock_sum`.

## La table des FLUX re-keyés

**Vers le trésor national** : taxes et captation des élites · tribut vassal ·
péages (détroit, marge d'import) · pillage de campagne · saisie à la conquête ·
frappe (royale et libre) · emprunt et dépôt de crédit · gains d'événements.

**Depuis le trésor national** : solde des armées · décrets · chantiers (`scps_agency`) ·
rénovation · colonisation · intérêts / amortissement / saisie de banqueroute ·
achat de la production par l'État · conseil · dons diplomatiques · indemnités de paix.

**Vers le stock national** : toute production de province (brutes + manufactures).

**Depuis le stock national** : intrants d'atelier · panier de consommation des pops ·
matériaux de construction · ravitaillement naval · armes de la levée.

La province calcule toujours `supply[]`/`demand[]` **localement** (c'est sa réalité
productive) ; seul le solde net atterrit au grain nation.

## Les cas limites — TRANCHÉS

**Province sans propriétaire (`owner < 0`).** Ne stocke rien : sa production est
consommée au tick, sa consommation ne prélève rien. Aucun transfert à la colonisation
ni à la conquête. Justification : les hameaux libres (POLITY_WILD) ont un *vrai* `cid`
— ils gardent donc trésor et stock nationaux comme tout le monde ; seules les
provinces **vierges** (`owner == -1`) tombent dans ce cas, et elles ne produisent pas.
Le cas est donc vide en pratique et le no-op est sûr.

**Sécession / naissance d'un pays.** Le nouveau pays naît **à sec** (trésor 0,
stock 0) ; l'or reste chez le parent. Choix retenu pour la simplicité et parce qu'il
**conserve** la masse monétaire (aucune création). La révolte garde son flux explicite
« acheter la paix » — c'est lui, pas un partage automatique, qui déplace l'or.
Le slot national est **remis à zéro à la naissance** pour qu'un pays ne puisse jamais
hériter du magot d'un mort dont le slot serait recyclé.

**Mort d'un pays.** Rien n'est effacé implicitement. La saisie à la conquête (flux
explicite, `scps_diplo.c` « on vide les coffres ») a déjà vidé le trésor avant la mort ;
le résidu éventuel reste dans son slot, inerte et **compté dans la masse monétaire**.
Zéroter en silence casserait l'invariant M3c pour un gain nul.

**Fog / lecteurs façade.** Le trésor et le stock d'un pays sont une donnée
**nationale** : la fiche province ne les montre plus du tout (champs API retirés, pas
mis à 0 « pour compat »). La topbar et l'écran Pays les lisent au grain nation, sous
le même fog qu'aujourd'hui (on ne voit le trésor d'autrui que par les canaux diplo
existants).

## Les seuils : nationaux, mais à l'échelle de l'empire

Trois seuils étaient **par province** et le sont restés — sommés sur l'empire (`× n_prov`)
pour que le calibrage pré-national tienne à l'identique :

- `SINK_FLOOR` (réserve d'exploitation, 500) → `500 × n_prov`. Un État large a besoin
  d'une réserve large ; sinon un empire de 30 provinces n'aurait plus que 500 de fonds
  de roulement et vivrait en friche perpétuelle.
- `COURT_FLOOR` (seuil de thésaurisation, 10 000) → `10 000 × n_prov`. « Thésauriser »
  est relatif à la taille du royaume.

Ce ne sont **pas des plafonds** : rien ne borne le trésor par le haut. Ce sont les seuils
au-dessus desquels les ponctions (faste de cour, admin, encadrement) commencent à mordre.

Deux ponctions sont **nationales par nature** et auraient mordu N fois si on les avait
laissées dans la boucle par-province : le **faste de cour** et la **redépense §B**. Elles
sont désormais appliquées au trésor national et réparties sur les provinces au prorata
de leur population (`Σ pshare = 1`) — le total mensuel est donc inchangé, seule la clé de
répartition des gages change (population au lieu de « trésor local »).

**Effet de bord assumé** : l'entretien des édifices se sert sur une bourse commune dans
l'ordre des `pid`. Un empire insolvable met donc en friche ses provinces de plus haut
`pid` plutôt que ses plus pauvres. C'est déterministe, et c'est le comportement normal
d'une caisse unique — mais c'est une sélection *arbitraire* : à revoir si le sweep montre
un motif spatial de friche.

## Une conséquence collatérale

`ECON_EFFCAP_BODY` (le logement) gatait son bonus de confort sur `supply + stock` de
poterie et de statuaire. Cet inline est **pur** (pas de `WorldEconomy`, pas de dépendance
de lien) : il ne peut plus lire le stock national. Le gate lit désormais `society_sat` —
la satisfaction sociale, déjà calculée depuis les fractions **servies** du panier
non-vivrier. Même intention (« les biens de confort sont là »), au grain province, sans
nouvel état.

## La preuve

`chronicle_money_mass` devient `Σ nat_treasury[c] + Σ prov[].strata[].wealth` — les deux
termes ne se recouvrent pas (or d'État vs bourse des pops), aucun double-compte.
L'invariant M3c (delta annuel documenté vs « autres ») reste le sceau : chaque flux
re-keyé est un **transfert**, jamais une création ni une fuite.
