# AUDIT — SCPS (moteur de grande stratégie C99)

> État POST-arc K (« état vérifiable »). Source de vérité = les bancs (`make test`),
> jamais ce fichier. Daté : 2026-06-11. Build : gcc 13 · -O2 -Wall -Wextra -std=c99
> + durcissement (`-fstack-protector-strong -D_FORTIFY_SOURCE=2`).

---

## (a) Résultats MESURÉS (`make test`, K3 appliqué)

**29 bancs VERTS / 31** — `make test` les bâtit, les lance, compte les BILAN :

core 35/35 · monde_reel 10/10 · readout 27/27 · species 9/9 · tech 22/22 ·
faith 14/14 · intertrade 9/9 · routes 4/4 · save_io 8/8 · statecraft 22/22 ·
pop 14/14 · army 49/49 · demography 19/19 · demography_integ 6/6 · revolt 23/23 ·
social 10/10 · agency 18/18 · campaign 13/13 · factions 32/32 · econ_tax 8/8 ·
econ_culture 6/6 · econ_arcane 6/6 · econ_production 4/4 · labor 41/41 ·
missions 8/8 · **diplo 49/49 (K4b)** · **warhost 4/4 (K4c)** · **events 27/27 (K4a)** ·
prosperity OK (sans format BILAN).

**2 bancs ROUGES** (la dette IA, ci-dessous — K5/K6) :

| banc | score | symptôme |
|------|-------|----------|
| ai_demo          | 20/23 | Mercantile pas + de routes · Bâtisseur pas + de K · Dominateur pas + agressif |
| structural_demo  | 13/16 | Dominateur ne SERRE pas (H) · Bureaucrate ne RÉFORME pas (K) · COERCITIF ne se dissout pas |

---

## (b) Dette connue (hypothèses de racine, à trancher avant correctif)

- **ai_demo + structural_demo (6 assertions, UNE racine présumée — K5)** : le crédit de
  largeur (arc B3) + la garde de budget (arc IG) ont APLATI les archétypes — tout le
  monde diversifie, plus personne ne maximise son thème/éthos. À INSTRUMENTER avant de
  toucher 6 endroits (part du score : largeur vs biais d'éthos).
- **structural_demo « la coercition dissout L » (K6)** : la fragilité σ-forme (A1) lit un
  INSTANTANÉ ; le banc attend une TRAJECTOIRE (la coercition ronge L avec le temps).
  Chaînon snapshot→delta manquant.

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
- **K3 — instrument** : `make test` (29 verts / 2 rouges, rc≠0 si un rouge) — la
  non-régression de tout l'arc.
- **K4a — events** : `events_fire_risk` retourne 0 dès `forest < 0.01` (le feu de forêt
  ne naît plus sans forêt). → 27/27.
- **K4b — diplo (cooldown)** : diagnostic CORRIGÉ — la racine n'était PAS la migration
  tune mais une garde `colonized` : le décrément de `pillage_cd` vivait DANS la boucle
  des régions colonisées, donc une région `colonized=0` ne le purgeait jamais. Décrément
  déplacé en tête de `econ_tick`, pour TOUTES les régions. → 49/49.
- **K4c — warhost (levée)** : diagnostic CORRIGÉ — la solde I1 draine bien le trésor,
  mais le recrutement puise dans un scratch ∝ pop, PAS dans le trésor : la paix
  n'était donc pas freinée par le coût, elle ACCUMULAIT (batch +2/an vers le plafond de
  pop) pendant que la guerre SATURAIT son pool en an 0 (recrutement tout-ou-rien). Un
  pays paisible à grand pool dépassait un pays en guerre à petit pool. Fix : la guerre
  MOBILISE vers le plafond de pop ; la paix DÉMOBILISE vers une GARNISON ∝ jauge
  (`WH_GARRISON_UNITS`) — au-dessus on dégraisse (`wh_shed`, la pop retourne au pool),
  en-dessous on complète à l'entretien. La solde I1 reste le COÛT qui, via la garde IG,
  abaisse la jauge → la garnison → démobilise PAR LE COÛT. → 4/4 (8 graines vérifiées).
