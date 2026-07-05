# Synthèse de session — 2026-07-05 (bis) : dédoublonnage + code mort

> Handoff. Branche `claude/vibrant-euler-1tgfp3`. **SAVE_VERSION 60 (non bumpé — rien de
> sérialisé ne change).** Mission : « fix, élimine les doublons, raffine ».

---

## État final (tout vérifié)

| Vérif | Résultat |
|---|---|
| `make test` | **37/37 runnable VERTS** (3 KO Windows pré-existants : intertrade `setenv`, campaign/warhost stack 1 Mo) |
| `make golden` | **IDENTIQUE** (consolidation byte-identique par construction) |
| `make determinism` | **STABLE** |
| `--savetest` 9/11/42 | **byte-identique** (2/2 chaque) |
| `make fuzz-save` | **8/8** (216 octets flippés, save_sane rejette tout) |
| `make scps` (viewer) | **0 warning — et SANS SDL désormais** |
| GDExtension `scons` | **0 warning**, DLL liée |

## Ce qui a été livré

### 1. Dédoublonnage moteur (« une définition par concept »)
- **`scps/scps_math.h`** (neuf) : `clampf` (NaN→lo) · `absf` · `iclamp` · `xs32` · `frand` —
  ~32 copies locales supprimées dans ~20 modules (dont les alias `rclampf`/`fabsf_local`/`clampi`).
- **Friction culturelle unifiée** (`scps_econ.{h,c}`) : `econ_content_dist` (D∞ 4 axes, 6 copies
  fusionnées) · `econ_content_dist_faith` (plancher de branche de foi, 2 copies) ·
  `econ_ruling_culture` (5 copies) · `world_capital_region` (`scps_world.{h,c}`, 2 copies).
  Équivalence float PROUVÉE (golden le confirme). `xs01` (campaign) = RNG distinct VOULU, pas touché.
- Commentaire PÉRIMÉ corrigé : revolt n'a jamais eu le plancher de foi (canal hérésie/zélote propre) —
  c'est maintenant documenté au point d'appel au lieu de prétendre « même friction que la démographie ».

### 2. Code mort (orphelins du viewer-strip) — −129 k lignes, −9,1 Mo d'assets
- `scps_audio.{c,h}` + `audio_demo.c` + **miniaudio** (third_party) — plus aucun appelant.
- `dev_overlay.{c,h}` + **Nuklear** (third_party) + cible `make dev` — le devpanel Godot (F10) l'a remplacé.
- `tools/fx_proof.c` + cible `fx-proof` + les **8 .bmp orphelins** (4 planches FX + dressing +
  settlements + port_orientation + route_cover) — les FX SDL sont partis avec l'UI.
- `labor_demo.c` (la cible était déjà RETIRÉE à la dissolution LaborEcon) · `scps_map_dressing.h`.
- `third_party/LICENSES.md` à jour (miniaudio/Nuklear sortis, mention F12 stbiw corrigée).

### 3. Viewer 100 % sans SDL
`viewer.c` perd son dernier `#include <SDL.h>` (compat SDL_main) → binaire console pur, lien `-lm`.
Le Makefile perd SDL_CFLAGS / SDL_LIBS / HAVE_SDL / WINLIBS / AUDIO_LIBS / bloc DEV :
**`make scps` se bâtit partout sans SDL** (plus besoin de `WIN=1` pour les libs, ni de
`SDL_VIDEODRIVER=dummy` pour les harnais).

### 4. Cliquet de langue verrouillé
`scps/lang_baseline.txt` **64 → 0** : la migration STR_* est finie, tout littéral face-joueur neuf
casse le build.

## Pour la prochaine session
- Doublons d'OUTILLAGE assumés (hors moteur, binaires séparés) : `avg_price` (chronicle/econ_scan),
  `write_ppm` (dump/mapshot), helpers de fixtures dans les `*_demo.c`. Sans enjeu de dérive moteur.
- `scps_region_settle_group` (façade) référence encore l'ANCIEN atlas dans son commentaire — la
  fonction est VIVANTE (binding + overlay.gd), seul le commentaire est historique.
- Env : `make scps` marche désormais dans le shell MSYS2 sans WIN=1 ni SDL (cf. mémoire build-windows).
