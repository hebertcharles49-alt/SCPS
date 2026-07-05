# Synthèse de session — 2026-07-05

> Handoff pour la prochaine session. Branche `claude/vibrant-euler-1tgfp3`, **HEAD `1ef0726`**,
> synchronisé avec origin, arbre de travail propre. **SAVE_VERSION 60**.

---

## État final (tout vérifié, committé, poussé)

| Vérif | Résultat |
|---|---|
| `make golden` | **RE-BASELINÉ** (dissolution golden-safe ; puissance commerciale re-baseline) — re-confirmé identique |
| `make determinism` | **STABLE** (A==A byte-identique) |
| `--savetest` (viewer) | **byte-identique v60** sur graines 9/11/42 |
| `make test` | **37/37 runnable VERTS** (voir les 3 KO Windows ci-dessous) |
| `scps_api_demo` | **107/107** |
| `make WIN=1 scps` · GDExtension `scons` | **0 warning** |
| sweep chronicle 9×3×(150-200) | **SAIN** (satisfaction 74-89 %, hégémon mortel, IPM 1.12-1.17) |

**3 KO PRÉ-EXISTANTS Windows/MinGW** (confirmés sur `main` vierge, RIEN à voir avec cette session) :
`intertrade_demo` (build : `setenv` est POSIX), `campaign_demo` + `warhost_demo` (crash `0xC00000FD`
STACK_OVERFLOW : pile Windows 1 Mo vs 8 Mo Linux — rc=127, **0 assertion échouée**). Sur Linux/cloud : **40/40**.

---

## Ce qui a été livré (4 chantiers, 7 commits)

### 1. `nettoie viewer.c` — commit `d558a71`
`viewer.c` **5768 → 278 lignes**. Le front interactif est Godot ; viewer.c ne garde que les 6 outils
CLI headless (`--savetest` `--fuzztest` `--dump-lang` `--dump-readout` `--lang-audit` `--dump-fnv`) +
`sim_rebuild` + les wrappers de save partagée. Toute l'UI SDL retirée. `#include <SDL.h>` gardé pour
la SEULE compat d'entrée MinGW (`-Dmain=SDL_main`) — aucun appel SDL, lien Makefile inchangé.

### 2. `nettoie laborecon` / DISSOLUTION LaborEcon — commit `08e745d`
Le module de population PARALLÈLE (`LaborEcon` : `LProvince`, mobilité de classe, `LMarket`, tick,
`pop_in_army`) est **DISSOUS** — une pop, une définition. La levée militaire lit désormais les strates
econ DIRECTEMENT : `army_recruit(a, const WorldEconomy*, int cid, …)`, `class_free` = Σ `region.strata[cl].pop`.
Les deux pipelines (campaign_refill JOUEUR + warhost `seed_scratch` IA) n'étaient que des ADAPTATEURS
semant un LaborEcon transitoire depuis econ. `scps_labor.{c,h}` SLIMMÉ aux fonctions PURES survivantes
(`capitale_*` + `labor_upkeep_per100`), qui restent dans `labor.o` (aucune migration, aucun ripple).
**GOLDEN-SAFE** (surprise) : dans la fenêtre 12 ans le gate de recrutement passe identiquement (pools
larges) + `pop_in_army` était mort. Section LABO retirée du save → **SAVE 58→59**. `labor_demo` retiré,
`army_demo` réécrit (fixture WorldEconomy), `audit_eco` perd la borne 2 (BOUCHE).

### 3. `finis le eco fable` — commit `7a06912` (notebook [019])
Les VRAIS bugs de l'audit étaient tous corrigés (savoir, trou trade×guerre C5, dissolution). Les 4
« reste » tranchés, plusieurs par RÉFUTATION (Invariant 2) :
- **C1** (retrait `PopGroup.L`) **REFUTÉ** — c'est un PRIMITIF vivant (gate de coercition `demography.c:176`,
  `agit_base`→révolte, `loyaute` readout `:348`, agrégats, issues revolt.c). Pas un silo. Les « trois L »
  sont 3 concepts distincts, tous vivants.
- **C4** (`governance_P` stub 5.f) **DÉLIBÉRÉ** — `prosperity.c:131` documente le socle neutre et prévient
  contre le re-routage (double-compte). Non-action.
- **§5** (puissance commerciale) — décoratif à l'époque → **rouvert + implémenté ensuite** (chantier 4).
- **C3** (~12 canaux RC orphelins) — chantier de DESIGN, pas un bug (floats non lus, inertes).

### 4. PUISSANCE COMMERCIALE — commits `8ee31de` (moteur) + `56a3b00` (membrane+UI) + `1ef0726` (notebook [020])
Le §5 rouvert par le joueur. Chaque empire a un **pool MENSUEL de volume échangeable** au marché ; les
achats le DRAINENT jusqu'au plafond (fin du « rafler tout le stock sans contrepartie »).
- `econ_country_commerce(econ,cid) = (0.04·bourgeois + 0.01·élite) × (1 + Σ build.PE_infra × BLD_PER)` —
  patron du savoir ; `build.PE_infra` = les 6 édifices commerciaux (marché/entrepôt/comptoir/banque/
  port marchand/centre), commerce-exclusif ⇒ aucun champ neuf.
- Pool FIXE par mois, reset par `sim_day` (`intertrade_commerce_reset`, day%30==0). GATE l'IMPORT de
  `intertrade_market_consume` (chantiers) + `intertrade_market_buy` (manuel), joueur ET IA ; stages
  propre+empire GRATUITS. Alimente `diplo_eco_power` (+`econ` à la signature, 6 sites).
- **Flag `g_commerce_active`** : inactif hors sim ⇒ BANCS INCHANGÉS par construction. Save : ITRD
  sérialise budget+spent (état intra-mois) → **SAVE 59→60**.
- **Membrane** : `scps_commerce_power` → binding `commerce_power` → menu marché `sidebar_drawer.gd`
  (readout « Puissance comm. : restant / pool ce mois (+X%) » + hover explicatif).
- **Mesuré** (seed 9) : le cap MORD (38-73 achats bornés/sim, ex-décoratif à 0.10), pool ~19/mois
  (petit empire, l'échelle « 18/20 » demandée) à ~546/mois (gros). Tunables registre J :
  `COMMERCE_W_BOURGEOIS` 0.04 · `_W_ELITE` 0.01 · `_BLD_PER` 0.10 · `_BLD_MAX` 0.50 · `_ECO_W` 0.05.

---

## Pour la prochaine session

- **Bumps de save de la session** : 58 → 59 (dissolution LABO) → 60 (puissance commerciale ITRD). Toute
  save < v60 est refusée.
- **RESTE de l'éco-mission** (design à arbitrer, PAS des bugs — voir `eco_fable.md` [019]/[020]) :
  - **C3** : ~12 canaux RC (religion) computés mais non consommés. Décider consommer-ou-retirer par canal
    exige l'intention du concepteur de la foi. Ni bug, ni urgent.
  - **C4** : le vrai producteur de `governance_P` serait une POLITIQUE nationale (loi d'ouverture/
    centralisation) — une FEATURE, pas un fix. Le socle neutre est délibéré.
- **Dialables d'une ligne** si l'équilibrage de la puissance commerciale doit bouger : `SCPS_TUNE=COMMERCE_W_BOURGEOIS=…`
  (0.04 mord ; 0.10 était décoratif ; ~0.007 serait la limite basse mesurée en [013]). `_ECO_W` (0.05) règle
  son poids diplo.
- **Env de build** (cf. [[scps-build-windows]]) : MSYS2 sous `D:\MSYS2`, `bash.exe -l <script>` avec `cd`
  en 1re ligne (le `-lc` perd le cwd). `make WIN=1 scps` pour le viewer SDL. GDExtension : `scons` a besoin de
  `export PROCESSOR_ARCHITECTURE=AMD64` dans le shell MSYS2 (sinon `KeyError`). `godot/godot-cpp` est un
  symlink vers `E:\JEUX\SCPS\godot\godot-cpp`.
- **Discipline** maintenue partout : membrane (viewer/façade ne lisent que des mots + nombres), « on lit des
  coordonnées, on n'assigne pas de modificateur » (le savoir & la puissance commerciale passent par les
  ENTRÉES du moteur), golden/determinism/savetest à chaque changement, télémétrie chronicle = preuve d'équilibre.

---
*Commits de la session, du plus récent au plus ancien :*
`1ef0726` eco_fable [020] · `56a3b00` puissance commerciale UI · `8ee31de` puissance commerciale moteur ·
`7a06912` eco_fable [019] clôture · `08e745d` dissolution LaborEcon · `d558a71` viewer strip · `a32c53a` nettoyage.
