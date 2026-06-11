# AUDIT — SCPS (moteur de grande stratégie C99)

> État POST-arc K (« état vérifiable »). Source de vérité = les bancs (`make test`),
> jamais ce fichier. Daté : 2026-06-11. Build : gcc 13 · -O2 -Wall -Wextra -std=c99
> + durcissement (`-fstack-protector-strong -D_FORTIFY_SOURCE=2`).

---

## (a) Résultats MESURÉS (`make test`, K3 appliqué)

**26 bancs VERTS / 31** — `make test` les bâtit, les lance, compte les BILAN :

core 35/35 · monde_reel 10/10 · readout 27/27 · species 9/9 · tech 22/22 ·
faith 14/14 · intertrade 9/9 · routes 4/4 · save_io 8/8 · statecraft 22/22 ·
pop 14/14 · army 49/49 · demography 19/19 · demography_integ 6/6 · revolt 23/23 ·
social 10/10 · agency 18/18 · campaign 13/13 · factions 32/32 · econ_tax 8/8 ·
econ_culture 6/6 · econ_arcane 6/6 · econ_production 4/4 · labor 41/41 ·
missions 8/8 · prosperity OK (sans format BILAN).

**5 bancs ROUGES** (la dette, ci-dessous) :

| banc | score | symptôme |
|------|-------|----------|
| ai_demo          | 20/23 | Mercantile pas + de routes · Bâtisseur pas + de K · Dominateur pas + agressif |
| structural_demo  | 13/16 | Dominateur ne SERRE pas (H) · Bureaucrate ne RÉFORME pas (K) · COERCITIF ne se dissout pas |
| diplo_demo       | 48/49 | après ~5 ans la province redevient saccageable (cooldown) |
| events_demo      | 26/27 | « feu de forêt » présent là où forest==0 |
| warhost_demo     | 3/4   | la paix lève MOINS que la guerre — non vérifié |

---

## (b) Dette connue (hypothèses de racine, à trancher avant correctif)

- **ai_demo + structural_demo (6 assertions, UNE racine présumée — K5)** : le crédit de
  largeur (arc B3) + la garde de budget (arc IG) ont APLATI les archétypes — tout le
  monde diversifie, plus personne ne maximise son thème/éthos. À INSTRUMENTER avant de
  toucher 6 endroits (part du score : largeur vs biais d'éthos).
- **structural_demo « la coercition dissout L » (K6)** : la fragilité σ-forme (A1) lit un
  INSTANTANÉ ; le banc attend une TRAJECTOIRE (la coercition ronge L avec le temps).
  Chaînon snapshot→delta manquant.
- **diplo_demo (K4b)** : suspect n°1 — la migration tune (arc J) a pu changer le défaut
  ou l'UNITÉ d'une constante de cooldown (jours/mois/ticks). Comparer `--tunables` au
  défaut attendu par le test.
- **events_demo (K4a)** : la formule de risque d'incendie garde une composante non nulle
  sans forêt. Décision : gater sur forest>0 (« strictement nul ») sauf wildfire de
  steppe explicitement voulu.
- **warhost_demo (K4c)** : la solde de régiment (arc I1) devait rendre la paix
  démobilisante PAR LE COÛT ; le couplage levée↔guerre/solde n'est pas aligné sur
  l'intention (paix = moins d'hommes sous les armes).

---

## (c) Recommandations (ordre de correction)

K4a (events, localisé) → K4b (diplo, cooldown) → K4c (warhost, levée) → **K5** (racine IA :
TESTER avant de fixer ; fix unique = pondérer le crédit de largeur par l'éthos ; re-tester
les 6 ensemble) → **K6** (érosion endogène `L -= k·coercition·dt`, bornée, réversible ;
dépend de K5 pour le fork Dominateur/Bureaucrate) → K7 (hygiène : clang clean, ASan).
Ne PAS entamer un nouveau système tant que `make test` n'est pas 31/31.

---

## (d) Correctifs DÉJÀ intégrés (arc K)

- **K1 — build/linkage** : `faction_name→tr()` (G4) rendait scps_factions.o dépendant de
  scps_lang.o ; 12 listes `_OBJS` linkaient factions sans lang → « undefined reference to
  tr ». Corrigé par liste (multi-ligne, sans double). `ifdef DEV/WIN` → `ifeq ($(VAR),1)`
  (le piège « DEV=false = dev »). Cible `scps` : message clair si SDL absent.
- **K2 — membrane** : `faction_name`/`edifice_name` migrés au readout (tr() y est
  légitime) ; scps_factions.c / scps_agency.c n'incluent plus scps_lang.h. AUDIT :
  aucun module moteur n'inclut scps_lang.h ; aucun tr() hors readout/viewer.
- **K3 — instrument** : `make test` (26 verts / 5 rouges, rc≠0 si un rouge) — la
  non-régression de tout l'arc.
