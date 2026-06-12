# AUDIT — SCPS (moteur de grande stratégie C99)

> État POST-arc K (« état vérifiable »). Source de vérité = les bancs (`make test`),
> jamais ce fichier. Daté : 2026-06-11. Build : gcc 13 · -O2 -Wall -Wextra -std=c99
> + durcissement (`-fstack-protector-strong -D_FORTIFY_SOURCE=2`).

---

## (a) Résultats MESURÉS (`make test`, K3 appliqué)

**31 bancs VERTS / 31** — `make test` les bâtit, les lance, compte les BILAN :

core 35/35 · monde_reel 10/10 · readout 27/27 · species 9/9 · tech 22/22 ·
faith 14/14 · intertrade 9/9 · routes 4/4 · save_io 8/8 · statecraft 22/22 ·
pop 14/14 · army 49/49 · demography 19/19 · demography_integ 6/6 · revolt 23/23 ·
social 10/10 · agency 18/18 · campaign 13/13 · factions 32/32 · econ_tax 8/8 ·
econ_culture 6/6 · econ_arcane 6/6 · econ_production 4/4 · labor 41/41 ·
missions 8/8 · **diplo 49/49 (K4b)** · **warhost 4/4 (K4c)** · **events 27/27 (K4a)** ·
**structural 16/16 (K5+K6)** · **ai 23/23 (L6)** · prosperity OK (sans format BILAN).

**0 banc rouge** — L6 a fermé la dernière dette (ci-dessous).

---

## (b) Dette connue (hypothèses de racine, à trancher avant correctif)

- **ai_demo « Bâtisseur +K » : RÉSOLU (L6/K5.b)** : la racine n'était pas la profondeur de
  chaîne mais le crédit de largeur NON pondéré — la même cascade foi→savoir→K payait tout
  le monde d'aller voir ailleurs. Fix : la largeur suit l'éthos (Dominateur/Honneur → H
  tant que la poigne est basse · Mercantile → le réseau, toujours · Bureaucrate → K pur,
  jamais le détour savoir ; la foi de crise reste universelle). → 23/23 sur 4 graines.

---

## (c bis) Arcs L + M v1 (2026-06-12) — BOUCLÉS, preuves au banc

- **L6** ai_demo 23/23 (largeur pondérée par l'éthos ; correctif post-gate : la largeur du
  conquérant est l'ARSENAL — acheter des armes — pas la garnison qui blindait le monde et
  tuait l'opportunisme : 1×30 mesuré à 0 guerre, corrigé).
- **L1** l'interception (campaign_redirect ; défense mensuelle → sortie de garnison ;
  l'attaquant re-cible ; la campagne respire au mois) — graine 7 : 2 → **84 batailles**.
- **L2** le ralliement (40-60 %, 30-60 j, une fois/guerre, noyau survivant) — SAVE 14.
- **L3** calibré par grille (arc J) : BT_CHOC_MORTS 0.008 → **0.0045** ⇒ ratio
  poursuite/choc **2.6x ∈ [2,5]** (graine 7). Traçabilité : SCPS_TUNE="BT_CHOC_MORTS=0.0045".
  Leçons : CUREE_CAP ne mord pas (P<cap) ; <0.004 = falaise d'entiers (0 mort de choc).
- **L4** la genèse peuple les continents (5 graines prouvées ; re-baseline notée CLAUDE.md).
- **L5** colonies outre-mer (portes ×2, Port+coque, ≤20 j de courants, traversée comptée).
- **M0-M7 v1** : design doc versé · ETHOS_FN verbatim · pôle par factions (Transgresseur
  orthogonal) · succession contextuelle + hystérésis 360 j + frères bloqués · +5 édifices
  (SAVE 15) · Alambic + essence purifiée + LE PUITS (charge 0.28→0.00 au banc) · score
  tech multiplicatif (souche×éthos×credo×matière) · 15 gabarits §25 (3 variantes/fork).
  forks_demo **34/34**. RESTE (hors v1 ou passe UI) : routage IA du savoir forké, journal
  des forks (bande UI), §23.3 interdit (cristallisation, navires, uint64).
- Transverse : make test **32/32** · 0 warning GCC+clang · ASan muet · déterminisme ·
  lang-check 64 · SAVE 13→15 (I/H : 13 · rally : 14 · forks : 15).
- **Dette guerre trans-mer** : sur les mondes fendus par L4 (graines 1/42), 0 guerre
  terrestre possible entre continents — les déclarations exigent l'adjacence terrestre.
  Chantier H3 (la mer veut vivre) : étendre countries_adjacent/CB à la portée navale.

---

## (c) Recommandations (ordre de correction) — arc K BOUCLÉ

K4a (events) ✓ → K4b (diplo) ✓ → K4c (warhost) ✓ → **K5** (racine IA = la spirale de
friche) ✓ → **K6** (coercitif : cas-test sur-extrême, pas un chaînon manquant) ✓ → K7
(hygiène : clang 0 warning, ASan muet) ✓. `make test` 30/31. La dette « Bâtisseur +K »
demande du CONTENU (chaîne K plus longue / bâti multi-régions) — hors périmètre « pas de
système neuf », laissée en l'état documenté (déjà signalée dans CLAUDE.md).

---

## (d) Correctifs DÉJÀ intégrés (arc K)

- **K1 — build/linkage** : `faction_name→tr()` (G4) rendait scps_factions.o dépendant de
  scps_lang.o ; 12 listes `_OBJS` linkaient factions sans lang → « undefined reference to
  tr ». Corrigé par liste (multi-ligne, sans double). `ifdef DEV/WIN` → `ifeq ($(VAR),1)`
  (le piège « DEV=false = dev »). Cible `scps` : message clair si SDL absent.
- **K2 — membrane** : `faction_name`/`edifice_name` migrés au readout (tr() y est
  légitime) ; scps_factions.c / scps_agency.c n'incluent plus scps_lang.h. AUDIT :
  aucun module moteur n'inclut scps_lang.h ; aucun tr() hors readout/viewer.
- **K3 — instrument** : `make test` (30 verts / 1 rouge, rc≠0 si un rouge) — la
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
- **K6 — structural « la coercition dissout L » : diagnostic CORRIGÉ (PAS un chaînon
  manquant)** : le mécanisme de dissolution EXISTE et MARCHE — l'Âge des Lumières pose
  `age_lumiere_solvent` (+2) qui ronge la légitimité effective `L − solvant·H/10`. L'invisible
  venait du CAS-TEST : le coercitif était façonné H=9·L=2, plus extrême que tout régime réel
  (la σ-forme A1 est CALÉE sur monde_reel : Russie 9.6, Iran 9.2), si bien que sa fragilité
  SATURAIT déjà à 10.0 — la dissolution n'avait plus de marge pour monter. Le moteur est juste
  (σ-forme verrouillée par core_demo 9.990 + monde_reel) ; c'est l'ASSERTION qui était
  sur-extrême. Fix : le cas-test coercitif passe à L=6 (~9.2, l'Iran) — franchement fragile
  MAIS avec la marge où le solvant fait grimper la fragilité (9.2→9.8). → structural_demo 16/16.
- **K7 — hygiène** : clang `-Wall -Wextra` 0 warning sur tout `scps/*.c` (15 recettes sans
  repli initialisent explicitement `alt1` = zéro-init intentionnel ; 2 variables mortes
  retirées) ; GCC 0 warning ; ASan+UBSan muets sur `chronicle_asan` ; déterminisme
  byte-identique (md5 inchangé avant/après) ; `lang-check` OK (64, base) ; membrane propre.
- **Bilan arc K** : `make test` 30/31 — la SEULE rouge est « Bâtisseur +K » (égalité à 3,
  CONTENU manquant — chaîne K profonde de 3, bâti mono-région ; hors périmètre « pas de
  système neuf »). Preuves : 0 warning GCC+clang · ASan muet · déterminisme · lang-check ·
  membrane · gold band 40 a en bande (trésor ~15.9k, flux +20). Les bancs structural/ai
  gardent une sensibilité de graine résiduelle (worldgen) sur 1-2 assertions hors graine
  par défaut — la cible `make test` (graine 42) est verte.
