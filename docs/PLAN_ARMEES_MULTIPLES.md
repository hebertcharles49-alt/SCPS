# PLAN — Armées multiples par nation (corps manœuvrables + boîte de sélection)

**Statut : DÉCISIONS PRISES (2026-07-12) — implémentation phasée autorisée (P5 complet).**
Déclencheur : « je veux scinder mon armée en plusieurs corps que je manœuvre séparément,
et les sélectionner à la boîte (draw-box) ».

### DÉCISIONS VERROUILLÉES
- **Combat : B2 empilement** — corps amis d'une région = front commun, pertes au prorata.
- **Portée : P5 complet équilibré** (jusqu'à l'IA multi-corps + le sweep de re-validation).
- **Plafond : un stack par RÉGIMENT, fusionnable.** `MAX_CORPS = 32`/pays (plafond dur ;
  au-delà, les régiments restent groupés). Descendre à 1 régiment/corps est FAISABLE mais
  jamais l'intérêt — la fusion est la norme, le design ne doit pas récompenser l'éparpillement.
- **IA : D2** — un corps par front actif (pas une IA RTS ; une doctrine de concentration).

---

## 0. État actuel (ce qu'on part démolir)

- `Campaign.army[SCPS_MAX_COUNTRY]` (320) — **UNE** force expéditionnaire par pays, indexée
  par `owner`. `WarHost.army[SCPS_MAX_COUNTRY]` — **UNE** réserve levée par pays.
- `FieldArmy` porte DÉJÀ : `owner`, `loc`, `dest`, `next`, `phase` (marche/siège/bataille/
  embarque/mer), `force` (compo), `posture`, ralliement (`rally_*`), traversée (`sail_*`),
  journal (`taken`/`taken_region`/`legs`/`battles`).
- **Toute** l'API est keyée par `owner` : `campaign_order/redirect/active/location/phase/
  units/taken/set_posture/posture/can_refill/refill_cost/refill/order_sea`, + `campaign_tick`
  (itère les pays), + la sortie défensive `sim_campaign_defense` + les ordres `sim_campaign_orders`.
- Combat = **1 attaquant vs 1 défenseur par région** (le siège d'un corps unique, la sortie
  du défenseur unique).

**Conséquence** : « un pays = une armée » est câblé dans ~30 sites moteur + la façade + l'IA +
la sérialisation. Le passage à N corps les touche tous.

---

## 1. LES DÉCISIONS DE DESIGN (à trancher AVANT le code — j'ai besoin de tes choix)

### A. Combien de corps par pays ?
- **A1 — plafond dur par pays** (ex. 6). Pool = 320×6 = 1920 `FieldArmy`. Mémoire : ~1920 ×
  sizeof(FieldArmy) ≈ 1920 × ~160 o ≈ 300 Ko. Acceptable.
- **A2 — pool global** (ex. 768 corps partagés, `n_armies` vivant). Moins de mémoire, mais
  enumération pays→corps à chaque accès.
- **Reco : A1** (plafond 6/pays) — indexation simple `army[pays][k]`, save borné trivialement,
  l'IA a une limite nette. Le joueur comme l'IA plafonnent pareil (équité).

### B. LE COMBAT multi-armées — **la décision structurante**
- **B1 — séquentiel indépendant** : chaque corps assiège/combat seul. Deux corps à toi sur la
  même province ennemie se battent chacun leur tour. Facile, mais absurde tactiquement.
- **B2 — empilement (stack)** : à la résolution, les corps AMIS d'une région fusionnent leur
  force pour la bataille (front commun), les ENNEMIS aussi → **une** bataille/région entre le
  stack attaquant total et le stack défenseur total ; les pertes se répartissent au prorata
  sur les corps engagés. Réaliste, mais refonte de la résolution siège/choc (le module le plus
  testé et le plus délicat).
- **Reco : B2.** Sans lui, les corps multiples n'ont aucun sens tactique (concentrer la force
  EST le but des manœuvres). C'est le poste le plus risqué du chantier.

### C. D'où viennent les corps ?
- Lever depuis la **réserve** (le vivier warhost, 1/pays) : « déployer un corps de N unités ».
- **Scinder** un corps déployé en deux (choisir combien d'unités partent).
- **Fusionner** deux corps du même owner sur la même région.
- **Reco : les trois.**

### D. L'IA multi-corps — **le second poste dur**
- **D1 — l'IA reste à 1 corps** : à REJETER (tu manœuvres 5 corps, l'IA 1 → tu écrases tout,
  balance ruinée).
- **D2 — l'IA lève N corps ∝ ses fronts** : un corps par guerre/menace active, ciblage
  indépendant, fusion défensive quand débordée. Étend `sim_campaign_orders` (pas une IA RTS
  complète — une doctrine « un corps par front »).
- **Reco : D2 minimal.** C'est là que vit la qualité : une IA multi-corps bête déséquilibre
  autant que D1.

---

## 2. Modèle de données (si A1 + B2 + D2)

- `Campaign.army[SCPS_MAX_COUNTRY][MAX_CORPS]` + `int n_corps[SCPS_MAX_COUNTRY]`. `FieldArmy`
  garde `owner` ; on ajoute un `id` stable (pays×MAX_CORPS+k) pour la sélection Godot.
- `WarHost.army[pays]` reste le VIVIER (réserve non déployée) ; les corps s'en détachent.
- Helper `corps_of(camp, owner, out_ids[])` → énumère les corps vivants d'un pays.

## 3. Fonctions campagne à re-clés (~30 sites)
- `campaign_*(owner)` → `campaign_*(owner, k)` (ou un `army_id` global). `campaign_tick` itère
  les corps. La sortie défensive choisit QUEL corps sort (le plus proche / le plus fort).
  Ralliement, mer, posture : déjà par-FieldArmy → suivent le corps.

## 4. Verbes neufs (moteur + façade + binding)
- `campaign_split(camp, econ, owner, k, n_units)` · `campaign_merge(camp, owner, k1, k2)` ·
  `campaign_raise(camp, host, owner, n_units, from_region)`.
- Façade : `scps_player_raise_corps` · `_split_corps` · `_merge_corps` · `_move_corps(id,tgt)` ·
  `_corps_posture(id,p)` · `_corps_refill(id)` · lecteurs `scps_country_corps_count` +
  `scps_corps_info(id)`.

## 5. Combat (détail de B2)
- Avant la résolution d'une région contestée : sommer `force` des corps amis (attaquant) et
  ennemis (défenseur) présents/arrivés ; UNE bataille (le modèle choc/poursuite/ralliement
  existant sur les forces sommées) ; répartir tués/brisure au prorata de l'effectif de chaque
  corps engagé ; un corps vidé → détruit (slot libéré). Le siège agrège pareil.

## 6. Sérialisation
- Campaign/WarHost changent de forme → **bump SAVE**. `save_sane` : `n_corps[pays] ∈ [0,MAX]`,
  chaque corps `owner∈[0,n)`, `loc/dest/next ∈ [-1,n_regions)`, `force` bornée, ids cohérents.
  `--fuzztest` forge n_corps + chaque champ hors-borne → rejet net. `--savetest` byte-identique.

## 7. Façade + Godot (la boîte de sélection vit ICI)
- `scps_corps_info(id)` + `country_corps_count` + liste des ids d'un pays.
- Overlay : **un pion par corps** (déjà le motif du pion actuel, ×N) ; **sélection multiple**
  (`Set` d'ids) ; **DRAW-BOX** : drag gauche → rectangle marquee → sélectionne tous les corps
  du JOUEUR dont le pion tombe dedans ; clic simple = un corps ; clic-destination = ordre à
  TOUS les corps sélectionnés.
- Panneau d'armée : multi-sélection (« 3 corps · 4 200 hommes »), ordres groupés, **Scinder** /
  **Fusionner** / poser un corps.

## 8. Vérification (discipline du dépôt)
- **golden** : re-baseline au P4 (l'IA change) ; P1-P3 visent golden-IDENTIQUE (cf. phasage).
- **giga-sweep de re-validation** 20 graines × sims × 250 ans : mortalité hégémon, nb guerres,
  satisfaction, §27 gaté — la balance a bougé (projection de force multipliée).
- **savetest** byte-identique (nouveau format) · **fuzz-save** · **determinism/-deep** ·
  `campaign_demo`/`warhost_demo` réécrits (multi-corps).

## 9. PHASAGE (landing progressif, chaque phase vérifiée et commitée)

| Phase | Contenu | golden |
|---|---|---|
| **P0** | Ce doc validé + design fin du combat B2 & de l'IA D2 (sur papier) | — |
| **P1** | Refactor pur : `army[pays]` → `army[pays][MAX]` mais **1 corps/pays** utilisé. Re-key les ~30 sites. | **IDENTIQUE** (refactor sans changement de comportement — le gate de non-régression) |
| **P2** | Verbes JOUEUR : lever/scinder/fusionner/déplacer par corps. Player-only → drain no-op en chronique. | **IDENTIQUE** (golden-safe) |
| **P3** | Combat B2 (empilement) — mais tant que seul le joueur a >1 corps, l'IA à 1 corps ⇒ résolution = l'ancienne. | **IDENTIQUE** attendu (à prouver) |
| **P4** | IA D2 (l'IA lève plusieurs corps). | **RE-BASELINE** + giga-sweep |
| **P5** | Godot : pions multiples, **draw-box**, UI scinder/fusionner/ordres groupés. Display-only. | intacte |

**L'intérêt du phasage** : P1-P3 restent golden-IDENTIQUES (refactor + joueur-only), donc on
lande le socle SANS re-baseline ni risque de balance ; la seule re-baseline (P4) est isolée et
suivie de son sweep. Si on s'arrête après P3, tu as déjà des corps multiples JOUABLES (l'IA
reste à 1 corps — déséquilibré mais fonctionnel pour tester la sensation).

## 10. Estimation grossière (effort relatif, pas d'horloge)
- P1 (refactor) : **gros** — mécanique mais ~30 sites, chaque erreur = un bug de guerre.
- P2 (verbes joueur) : moyen.
- P3 (combat B2) : **gros + délicat** — le module le plus testé.
- P4 (IA D2) : **gros** — la qualité de l'équilibre en dépend + le sweep.
- P5 (Godot) : moyen (la draw-box elle-même est petite).
- **Total : le plus gros chantier depuis longtemps.** Plusieurs sessions, plusieurs re-vérifs.

## Risques majeurs
1. **P3 combat B2** : refonte de la résolution siège/choc, le code le plus fragile.
2. **P4 IA D2** : une IA multi-corps médiocre ruine la balance autant que pas d'IA.
3. **Save churn** + fuzz + le sweep de re-validation (coût de vérif réel).

---

## Ce qu'il me faut de toi pour finaliser (avant P0→P1)
1. **A** : plafond 6 corps/pays, ou un autre nombre ?
2. **B** : combat B2 (empilement) confirmé, ou B1 (séquentiel, plus simple mais absurde) ?
3. **D** : IA D2 (un corps/front) confirmée — ou tu acceptes de t'arrêter à P3 (IA à 1 corps,
   déséquilibré) pour tester la sensation d'abord ?
4. **Portée d'arrêt** : on vise P5 complet, ou un premier jalon à P3 (jouable, non équilibré) ?
