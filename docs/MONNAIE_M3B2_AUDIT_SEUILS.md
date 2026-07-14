# MONNAIE M3b-v2 — L'AUDIT DES SEUILS NOMINAUX (avant le circuit)

> Contrat : brief M3b-v2, « Le piège à auditer avant tout ». La satisfaction doit lire
> la consommation RÉELLE, jamais la richesse NOMINALE — chaque seuil qui compare une
> valeur monétaire à une CONSTANTE fixe est un piège de pauvreté potentiel une fois que
> le niveau des prix devient endogène (v1 est mort d'un seul de ces seuils : le forfait
> fiscal per-capita rasant une richesse déjà nulle, cf. TROUVAILLES « CHANTIER MONNAIE —
> M3b », §Découvertes). Registre : chaque seuil classé DÉJÀ RELATIF (s'échelonne avec le
> panier/prix, rien à faire) / CONVERTI (rendu relatif par cette mission, voir commit
> dédié) / ASSUMÉ NOMINAL (raison donnée pour laquelle ce n'est PAS un piège).

## 1. Le forfait fiscal per-capita — CONVERTI (le seul qui a tué v1)

- `scps/scps_econ.c` §6-7 (`tax_base[c]`, appliqué ligne ~3284) : `collected = tax_base[c]
  (or/hab/mois, CONSTANTE) × pop × mult × (1−évasion) × (dt·12)`, clampé à `st->wealth`
  (jamais négatif) mais **rien n'empêche de raser une richesse déjà proche de 0** — c'est
  EXACTEMENT le mécanisme qui a tué M3b v1 (« la taxe per-capita FORFAITAIRE... rase ce
  qu'il en reste », TROUVAILLES M3b).
- **CONVERTI** (commit séparé « exonération sous le panier vital ») : sous un plancher de
  richesse-par-tête ∝ panier vital (lu via `g_basket_pc[pid][c]`, le panier/tête CAPTÉ au
  tick précédent — même lag d'1 tick que les autres lectures croisées de ce fichier), la
  classe est EXONÉRÉE (collected=0) au lieu d'être rasée. Le forfait NOMINAL lui-même
  N'EST PAS touché (interdit par le brief) — seule la MORSURE sur les indigents l'est.

## 2. Le plancher de prix (`clampf(p, BASE_PRICE*0.15, BASE_PRICE*8)`) — CONVERTI

- `scps/scps_econ.c` (bloc PRIX NATIONAL, clôture d'`econ_tick`) : le prix ne peut jamais
  descendre sous 15 % ni monter au-dessus de 800 % d'une CONSTANTE `BASE_PRICE[r]` figée à
  la genèse. Le postmortem M3b v1 nomme EXPLICITEMENT ce plancher comme co-cause de la
  mort du monde (« conso bornée par tête + prix RIGIDES (ancres BASE_PRICE, IPM clampé) »)
  : si le NIVEAU des prix doit pouvoir flotter fortement à la baisse (déflation
  d'équilibre, le mécanisme anti-6:1 voulu par ce brief), un plancher ANCRÉ à la valeur de
  genèse l'en empêche mécaniquement dès que le facteur monétaire voudrait pousser plus bas
  que 15 %.
- **CONVERTI** (commit « le circuit d'État+prix ») : les deux bornes SUIVENT désormais le
  facteur monétaire par pays (`BASE_PRICE[r]*0.15f*price_level[c]` /
  `BASE_PRICE[r]*8.f*price_level[c]`) au lieu d'ancrer sur `BASE_PRICE[r]` nu — la largeur
  RELATIVE de la bande (0.15×..8×) reste, mais son ancrage bouge avec la monnaie
  disponible. Exception : les MÉTAUX monétaires (or/cuivre) restent ancrés SANS le facteur
  (`price_level=1` pour eux dans cette formule) — c'est l'étalon (v5), qui doit rester un
  numéraire stable pendant que tout le reste flotte contre lui.

## 3. Les seuils de promotion/démotion de classe — DÉJÀ RELATIFS (rien à faire)

- `scps/scps_econ.c:2724` (`mobility_tick_region`) : `thr = mult × g_basket_pc[rid][from]`
  — le seuil de promotion (journalier→bourgeois 1.4×, bourgeois→élite 2.5×) est DÉJÀ une
  MULTIPLE du panier/tête RÉEL du tick précédent (`g_basket_pc`, recalculé chaque tick
  depuis `re->price[]` courant) — il s'échelonne automatiquement avec le niveau des prix,
  aucune conversion nécessaire.
- `scps/scps_econ.c:2734-2737` (démotion, 2 mois consécutifs sous 30 % de satisfaction) :
  le seuil est une SATISFACTION (déjà normalisée 0..1, dérivée du panier réel servi), pas
  une valeur monétaire — rien à convertir.

## 4. Les planchers de trésorerie D'ÉTAT (COURT_FLOOR=4000, SINK_FLOOR=500) — ASSUMÉS NOMINAUX

- `scps/scps_econ.c` (`SINK_FLOOR`, réserve d'exploitation sous laquelle l'État ne dépense
  plus — entretien/redépense) et `COURT_FLOOR` (seuil de hoarding au-delà duquel
  court/encadrement/admin mordent) : ce sont des seuils sur le TRÉSOR D'ÉTAT (province),
  PAS sur la richesse d'une CLASSE/ménage — le piège du brief (« la satisfaction doit lire
  la consommation RÉELLE ») vise spécifiquement les ménages, pas la trésorerie publique.
  Un État qui garde un plancher NOMINAL de fonctionnement n'affame personne directement ;
  au pire, sous déflation forte, ces planchers deviennent RELATIVEMENT plus faciles à
  atteindre (l'État semble « plus riche » en unités dévaluées) — un biais dans le sens
  opposé au piège de pauvreté (il ferait plutôt dépenser l'État un peu PLUS tôt sous
  déflation, jamais l'inverse). Assumé nominal, non converti — hors du risque nommé par le
  brief.

## 5. Le plafond de crédit (`CREDIT_LINE_BASE × population`) — ASSUMÉ NOMINAL, HORS SCOPE M3c

- `scps/scps_credit.c:30-32` (`credit_line`) : le plafond de dette ÉMERGE de la taille de
  la population, pas d'un chiffre fixe — DÉJÀ partiellement relatif (grossit avec
  l'empire), mais ne suit PAS le niveau des prix (une population inchangée garde le même
  plafond même si tout le reste est devenu 10× moins cher). **Interdiction explicite du
  brief** : « INTERDITS : toucher au crédit (M3c) ». Noté pour mémoire — si un futur M3c
  veut un plafond sensible au panier, c'est ici — non touché par cette mission.

## 6. L'IPM borné (IPM_LO=0.85 / IPM_HI=1.35) — ASSUMÉ NOMINAL, DEVIENT UN AUTRE MÉCANISME

- `scps/scps_econ.c:2742-2748` : ce n'est pas un seuil de richesse mais un COEFFICIENT
  borné (±15/35 %). Il reste actif pour (a) les provinces ISOLÉES (owner<0, hors périmètre
  M3b — fixtures/bancs) et (b) la surcharge d'entretien (`ipmf`, un monde cher coûte plus
  cher à tenir — mécanisme distinct de la consommation des ménages). Pour les provinces
  d'EMPIRE, ce coefficient est NEUTRALISÉ au site du prix consommateur (double emploi avec
  le nouveau `price_level[c]` par pays — commit séparé « IPM neutralisé ») ; il continue
  d'exister ailleurs, non retiré du fichier.

## 7. Le plafond joueur `g_prod_cap` (limiteur par ressource) — HORS SUJET

- Un plafond de STOCK absolu (unités physiques, pas or) posé par le curseur joueur — aucun
  rapport avec un seuil de richesse monétaire. Non concerné par cet audit.

## Conclusion

Un seul site NOMINAL constituait un vrai piège de pauvreté pour les MÉNAGES (le forfait
fiscal, §1 — la cause avérée de la mort de v1) et un seul autre bridait mécaniquement la
déflation d'équilibre voulue par ce brief (le plancher de prix ancré à `BASE_PRICE`, §2).
Les deux sont convertis par cette mission (voir commits « exonération sous le panier
vital » et « le circuit d'État+prix »). Tous les autres seuils examinés sont déjà relatifs
au panier/à la satisfaction, ou nominaux sur un compte qui n'est pas le pouvoir d'achat
d'un ménage (trésor d'État, crédit — ce dernier explicitement hors scope).
