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

**2 bancs ROUGES** (la dette restante, ci-dessous) :

| banc | score | symptôme |
|------|-------|----------|
| ai_demo          | 22/23 | Bâtisseur pas + de K (ÉGALITÉ à 3 — dette d'ÉQUILIBRAGE documentée) |
| structural_demo  | 15/16 | COERCITIF ne se dissout pas (K6 : snapshot→trajectoire) |

---

## (b) Dette connue (hypothèses de racine, à trancher avant correctif)

- **ai_demo « Bâtisseur bâtit le PLUS de K » (ÉGALITÉ, dette d'équilibrage)** : la chaîne
  K est PROFONDE de 3 (Tribunal→Chancellerie→Académie) et l'IA ne bâtit que dans sa
  région-mère → les TROIS archétypes plafonnent à 3 K (égalité, `strict_max` faux). Pour
  DÉPARTAGER il faut une chaîne K plus longue OU un bâti multi-régions (CONTENU/MÉCANIQUE
  neuve, hors périmètre arc K). Reste la « dette préexistante » de CLAUDE.md.
- **structural_demo « la coercition dissout L » (K6)** : la fragilité σ-forme (A1) lit un
  INSTANTANÉ ; le banc attend une TRAJECTOIRE (la coercition ronge L avec le temps).
  Chaînon snapshot→delta manquant.

---

## (c) Recommandations (ordre de correction)

K4a (events) → K4b (diplo) → K4c (warhost) → **K5** (racine IA = la spirale de friche, FAIT) →
**K6** (érosion endogène `L -= k·coercition·dt`, bornée, réversible) → K7 (hygiène : clang
clean, ASan). La dette « Bâtisseur +K » demande du CONTENU (chaîne K plus longue) — hors
périmètre arc K, laissée en l'état documenté.

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
- **K5 — IA gelée : diagnostic CORRIGÉ (PAS le crédit de largeur)** : l'instrumentation
  montre les archétypes NON aplatis mais GELÉS — les ponctions d'or arc I (admin/cour/
  encadrement/surtaxe IPM) vidaient le trésor d'une capitale-test isolée à 0 → la région
  tombait en FRICHE (prod 0.6×) → satisfaction effondrée → pression fiscale (I) montée →
  SI<5 → frein de consolidation BLOQUÉ à 1 (le Dominateur consolide au lieu de conquérir ;
  le Mercantile, à trésor 0, ne peut bâtir son grenier → verrou de famine, 0 route). Fix
  (scps_econ.c) : (1) RÉSERVE D'EXPLOITATION `SINK_FLOOR` — entretien & redépense d'État ne
  vident jamais sous ce plancher (un État garde de quoi bâtir/amorcer) ; (2) FRICHE = surbâti
  CATASTROPHIQUE seulement (entretien > TOUT le trésor), plus la falaise sur une thune fine ;
  (3) les ponctions anti-thésaurisation (admin/cour/encadrement/surtaxe IPM) ne mordent qu'AU-
  DESSUS du seuil de HOARDING (`COURT_FLOOR`) — elles saignent les gros trésors, pas le bas de
  laine qui finance les chantiers. → ai_demo 22/23, structural_demo 15/16 ; chronique 40 a :
  trésor moy ~15.9k (mieux que les 21-30k d'avant), flux +20/mois (bord de bande), 0 friche.
  Reste l'ÉGALITÉ « Bâtisseur +K » (dette de contenu, supra).
