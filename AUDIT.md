# AUDIT — SCPS (moteur de grande stratégie C99)

Audit de reprise (« on récupère à partir d'ici »). Périmètre : les 121 fichiers
source du moteur (`scps/`), le `Makefile`, l'outillage (chronicle, bancs) et les
bibliothèques vendorées (`third_party/`). Le visualiseur SDL (`viewer.c`) n'a pas
pu être *exécuté* (SDL2 absent de l'environnement d'audit) mais a été relu.

## Verdict global

**Code de très haute qualité, sain.** Discipline défensive remarquable : aucune
fonction de chaîne non bornée, aucun VLA/`alloca`, aucun `system()`, indices
re-validés au point d'usage, RNG correctement borné, gardes anti-division. La
batterie de bancs auto-vérifiants et la télémétrie headless constituent un filet
solide. Les constats ci-dessous sont **mineurs** : un bug d'outillage (cible
`asan` cassée) et des durcissements, **aucune faille mémoire** détectée.

## Tableau de bord (mesuré dans cet environnement — gcc 13.3 / clang 18 / valgrind 3.22)

| Contrôle | Résultat |
|---|---|
| `make core_demo` + tous les bancs (`-Wall -Wextra`, gcc) | **0 warning**, build vert |
| `./core_demo` (cœur §2) | **35 / 35** |
| Bancs auto-vérifiants (diplo, army, labor, revolt, econ, …) | **0 échec** |
| `./ai_demo` | 22 / 23 — l'unique échec = la **dette documentée** (« le Bâtisseur bâtit le PLUS de K »), pas une régression |
| `make determinism` (5 graines × 12 ans, `--hash`) | **hashes stables** ; télémétrie identique entre deux runs |
| ASan + UBSan (chronicle 40 ans, 6 empires, 12 cités) | **0 erreur** ; balayage étendu (60 ans, 16 cités) muet sur la portion exécutée |
| Valgrind memcheck (runs complets 5 et 8 ans) | **0 erreur, 0 fuite** (definitely/indirectly/possibly = 0) |
| GCC `-fanalyzer` sur **tout le moteur** | **0 warning** (les seuls remontés sont dans les *bancs*, cf. C5) |

## Constats

### A — Outillage

**A1 · Cible Makefile `asan` cassée (impossible à construire).** `Makefile:378-380`.
Deux défauts cumulés :
1. La règle ne passe pas `-Ithird_party` → `chronicle.c` n'inclut pas `miniz.h`
   (`fatal error: miniz.h: No such file or directory`).
2. `CHRONICLE_SRCS` (`Makefile:377`) dérive les sources par
   `patsubst $(OBJDIR)/scps_%.o,scps/%.c,...`, motif qui **ne matche pas**
   `$(OBJDIR)/tp_miniz.o` : la variable contient donc `build/tp_miniz.o` (un
   objet, pas une source) et `miniz.c` n'est jamais compilé dans le binaire ASan.

Conséquence : `make asan` échoue, alors que `CLAUDE.md` en fait le filet mémoire
de référence (« ASan+UBSan doivent rester muets »). *Note : pour cet audit, un
binaire ASan corrigé a été reconstruit à la main — il tourne muet.*

Correctif suggéré :
```make
CHRONICLE_SRCS := $(patsubst $(OBJDIR)/scps_%.o,scps/%.c,\
                    $(filter-out $(OBJDIR)/tp_miniz.o,$(CHRONICLE_OBJS)))
asan: $(CHRONICLE_SRCS)
	$(CC) -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer \
	      -Wall -Wextra -std=c99 -Ithird_party \
	      $(CHRONICLE_SRCS) third_party/miniz.c -o chronicle_asan -lm
```

### B — Robustesse / durcissement (sévérité faible)

**B1 · `save_sane()` ne valide que la borne HAUTE de plusieurs index signés.**
`viewer.c:2591-2630`. Les champs `Province.region/country`, `Country.capital_prov`,
`RegionEconomy.owner`, `Cell.{province,region,country,continent}` sont `int16_t`/`int`
signés mais testés uniquement `>= n` (pas `< 0`, hors sentinelle `-1` légitime).
Le commentaire du chargeur affirme pourtant neutraliser les fichiers **forgés**
(l'empreinte FNV n'est pas un MAC). Un index négatif forgé (≠ −1) passe donc
`save_sane`.

**Sévérité faible, et déjà atténué** : (i) modèle de menace assumé « anti
save-scumming, pas sécurité » — un save forgé n'attaque que la machine du joueur ;
(ii) la plupart des consommateurs re-gardent `cp < 0` au point d'usage
(p. ex. `scps_diplo.c:463/674`, `scps_ai.c:889/928`, `scps_demography.c:73`,
`scps_econ.c:602`). Reco : rendre `save_sane` **symétrique** (rejeter `< 0`, ou
`< -1` pour les champs à sentinelle) — sûr, et supprime la dépendance au fait que
*chaque futur* consommateur pense à garder.

**B2 · `strncpy` sans terminaison NUL explicite.** `scps_world.c:1842` :
`strncpy(rg->name, rg->name_hum, sizeof(rg->name)-1);`. Repose sur le dernier
octet de `rg->name` déjà à 0. Inoffensif aujourd'hui (sources courtes, struct
issue de worldgen) mais idiome fragile. Reco : `rg->name[sizeof(rg->name)-1]=0;`.

**B3 · Build release sans flags de durcissement.** `Makefile:8`. `-O2 -Wall -Wextra`
sans `-fstack-protector-strong` ni `-D_FORTIFY_SOURCE=2`. Défense en profondeur
quasi gratuite, surtout sur le chemin qui lit des sauvegardes. Reco facultative.

### C — Hygiène / cosmétique

**C1 · `nmar` calculé puis jamais affiché.** `chronicle.c:867-868`. Le nombre de
routes **maritimes** est compté mais la ligne de télémétrie n'imprime que `nfluv`
(fluvial) → donnée silencieusement perdue (et « variable set but not used » sous
clang). Soit l'afficher, soit le retirer.

**C2 · Propreté clang sous `-Wextra` (le dépôt est *gcc*-propre, pas *clang*-propre).**
- `scps_econ.c` (~15×, table `RECIPE`) : `-Wmissing-field-initializers` sur le
  champ `alt1` omis. **Bénin** — `alt1` non initialisé vaut `RES_NONE` (0) =
  « pas de repli », exactement l'intention. Cosmétique.
- `scps_routes.c:12` : fonction `clampf` définie mais inutilisée (`-Wunused-function`).

**C3 · `-fanalyzer` : aucun défaut dans le moteur.** Les 145 remontées sont
**toutes** dans les *bancs* (`events_demo.c`, `statecraft_demo.c`) : 144 fuites
« à la sortie » (programmes courts qui n'`free` pas avant `return` — l'OS
récupère, inoffensif) + 1 `use-of-uninitialized-value` à `ai_demo.c:126` qui est
un **faux positif** (le garde `if (npol<3) return 1;` précède la lecture de
`polity[0..2]` ; l'analyseur ne corrèle pas `npol` au nombre d'écritures).

## Points forts (à préserver)

- **La membrane / discipline d'index** : `n_regions ≤ SCPS_MAX_REG` etc. garantis
  par `save_sane`, puis les BFS/flood-fill (`scps_campaign.c:37`, `scps_events.c`,
  `scps_world.c`) bornent leurs files par le domaine avec tableau « visité » →
  jamais de débordement.
- **RNG borné proprement** : `rng_f() ∈ [0,1)` *strict* (`scps_world.c:40`,
  `16777215/16777216`) → la macro `PICK` (`:1749`) n'a pas d'off-by-one.
- **Crypto honnête** : ChaCha20 (réf. compacte correcte) + empreinte FNV-1a du
  clair, périmètre explicitement cadré (« obfuscation, pas secret ») ;
  plafond `SAVE_MAX_PAYLOAD` (256 Mo) contre le `malloc` géant sur en-tête forgé ;
  sections taguées + vérif de taille exacte (`sv_r`).
- **Déterminisme** tenu (réductions éco/sim + worldgen), OpenMP `schedule(static)`
  opt-in et isolé en worldgen.

## Recommandations priorisées

1. **(A1)** Réparer la cible `asan` du Makefile — restaure le filet mémoire documenté.
2. **(B1)** Rendre `save_sane` symétrique (bornes basses) — durcit le chargeur
   conformément à son intention affichée.
3. **(C1)** Trancher sur `nmar` (afficher ou retirer).
4. **(B2/B3/C2)** Terminaison NUL explicite, flags de durcissement, propreté clang —
   au fil de l'eau.
