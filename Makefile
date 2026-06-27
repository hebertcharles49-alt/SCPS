# Makefile — moteur SCPS (simulation de civilisations, headless + visualiseur)
# Linux/Mac : make            → construit core_demo (banc d'essai vérifié, sans SDL)
# MinGW Win : make            (autodétection OS=Windows_NT) ou make WIN=1
#
# stb_perlin.h est vendorisé dans scps/ : le moteur est entièrement autonome.

CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra -std=c99
# Génération automatique des dépendances d'en-têtes (.d) : un .o est recompilé
# quand un .h qu'il inclut change.
CFLAGS  += -MMD -MP
# Vendoring single-header (third_party/) : un chemin d'inclusion, des objets
# committés. Aucun gestionnaire de paquets, aucun lien dynamique nouveau.
CFLAGS  += -Ithird_party
# Durcissement (défense en profondeur, surtout le chemin qui lit des sauvegardes) :
# canari de pile + contrôles _FORTIFY des fonctions mémoire/chaîne. Gratuit en
# perf à cette échelle, et reste muet sous -Wall -Wextra (code sans appel non borné).
HARDEN  := -fstack-protector-strong -D_FORTIFY_SOURCE=2
CFLAGS  += $(HARDEN)
OBJDIR  := build

# miniz (MIT, vendoré) : on n'emploie que crc32 + deflate (zlib mz_compress/
# mz_uncompress). On COUPE l'archive ZIP, le stdio interne et le temps —
# surface minimale, et plus de #pragma message parasite.
MINIZ_FLAGS := -DMINIZ_NO_STDIO -DMINIZ_NO_TIME -DMINIZ_NO_ARCHIVE_APIS

# OpenMP (brief build §4) : OPT-IN — `make OMP=1 …`. Les pragmas vivent sous
# #ifdef _OPENMP, donc le build SANS OpenMP reste valide à l'identique. À ne
# retenir qu'après `make OMP=1 chronicle && make determinism` VERT.
OMPFLAG := $(if $(OMP),-fopenmp,)
CFLAGS  += $(OMPFLAG)

# Overlay de dev (brief build §6) : DEV=1 active -DSCPS_DEV partout (les blocs
# #ifdef SCPS_DEV du viewer s'allument) + débogage. Construit dans un OBJDIR
# SÉPARÉ (build_dev) → ne contamine jamais les objets release. La cible `dev`
# fait le sous-make ; le RELEASE n'embarque pas une once de Nuklear.
ifeq ($(DEV),1)
  CFLAGS += -DSCPS_DEV -O0 -g
endif

# Détection automatique : MSYS2/MinGW expose OS=Windows_NT.
ifeq ($(OS),Windows_NT)
  WIN := 1
endif

# SDL n'est requis QUE par le visualiseur (scps_viewer). Les bancs d'essai
# headless (core_demo, scps_dump, …) se construisent sans SDL. La détection
# est silencieuse : en l'absence de sdl2-config, SDL_* reste vide.
# `-Dmain=SDL_main` est FILTRÉ : le sdl2-config MinGW l'émet, mais ce renommage
# casse le `main` des outils headless (chronicle, bancs → undefined WinMain). Le
# viewer, lui, n'en a pas besoin — SDL_main.h le fait déjà via #include <SDL.h>.
SDL_CFLAGS := $(filter-out -Dmain=SDL_main,$(shell sdl2-config --cflags 2>/dev/null))
SDL_LIBS   := $(shell sdl2-config --libs   2>/dev/null)

ifeq ($(WIN),1)
  EXE     := .exe
  WINLIBS := -lopengl32 -mwindows -static-libgcc -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic
else
  EXE     :=
  WINLIBS := -lGL
endif

# Cible par défaut : le moteur vérifié §2, autonome (aucune dépendance SDL).
all: core_demo

$(OBJDIR):
	@mkdir -p $(OBJDIR)

# Compilation générique des sources scps/. SDL_CFLAGS n'ajoute que des chemins
# d'inclusion (inoffensif pour les fichiers headless).
$(OBJDIR)/scps_%.o: scps/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

# ---- third_party : objets vendorés (single-file, MIT/domaine public) ------
$(OBJDIR)/tp_miniz.o: third_party/miniz.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(MINIZ_FLAGS) -c $< -o $@
$(OBJDIR)/tp_stbiw.o: third_party/stb_image_write_impl.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@
# miniaudio : gros single-header — compilé À PART (sans -Wextra ni -std strict),
# surface réduite à la lecture de device. Sur Linux il dlopen ses backends →
# -ldl/-lpthread au lien (déjà tirés par SDL pour le viewer).
$(OBJDIR)/tp_miniaudio.o: third_party/miniaudio_impl.c | $(OBJDIR)
	$(CC) -O2 -Ithird_party -c $< -o $@
AUDIO_LIBS := $(if $(WIN),-lole32 -lwinmm,-lpthread -lm -ldl)

# ---- Moteur SCPS headless (§2 + annexe) — colonne vertébrale VÉRIFIÉE -----
# Banc d'essai auto-vérifiant (35 contrôles, sortie ≠ 0 si échec).
CORE_DEMO_OBJS := $(OBJDIR)/scps_scps_core.o $(OBJDIR)/scps_core_demo.o
core_demo: $(CORE_DEMO_OBJS)
	$(CC) $(CORE_DEMO_OBJS) -o $@ -lm

# ---- Banc d'étalonnage de la fragilité (A1) sur des régimes réels ---------
# La forme 10·σ(0.9(H−L)+0.3(H−P)−1.5) doit TRIER : autocraties → coercitif-
# fragile, démocraties → consenti, submergés → révolution/sécession.
MONDE_REEL_OBJS := $(OBJDIR)/scps_scps_core.o $(OBJDIR)/scps_monde_reel.o
monde_reel: $(MONDE_REEL_OBJS)
	$(CC) $(MONDE_REEL_OBJS) -o $@ -lm

# ---- Membrane diégétique (flottants SCPS → mots) — banc d'essai headless --
# Prouve le test décisif « Tenue · Contrainte » et la couverture du lexique.
READOUT_DEMO_OBJS := $(OBJDIR)/scps_scps_core.o $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_lang.o \
                     $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_culture.o \
                     $(OBJDIR)/scps_scps_species.o $(OBJDIR)/scps_scps_tech.o $(OBJDIR)/scps_scps_tune.o \
                     $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_readout_demo.o
readout_demo: $(READOUT_DEMO_OBJS)
	$(CC) $(READOUT_DEMO_OBJS) -o $@ -lm

# ---- Localisation (tr_fmt {n|spec}, FNV, audit, glossaire) — banc headless -
# Autonome : scps_lang.c n'a aucune dépendance moteur (que des tables + format).
LANG_DEMO_OBJS := $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_lang_demo.o
lang_demo: $(LANG_DEMO_OBJS)
	$(CC) $(LANG_DEMO_OBJS) -o $@

# ---- Roster de races & système de traits (autonome) ----------------------
SPECIES_DEMO_OBJS := $(OBJDIR)/scps_scps_species.o $(OBJDIR)/scps_species_demo.o
species_demo: $(SPECIES_DEMO_OBJS)
	$(CC) $(SPECIES_DEMO_OBJS) -o $@

# ---- religion_demo : module fondation religion (P1, PUR) ---------------------
RELIGION_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_religion_demo.o
religion_demo: $(RELIGION_DEMO_OBJS)
	$(CC) $(RELIGION_DEMO_OBJS) -o $@

# ---- Visualiseur de carte + UI diégétique (SDL2 + SDL_ttf) ---------------
# Le viewer lie toute la chaîne sim (la membrane scps_readout traduit en mots),
# mais N'inclut PAS scps_core.h (cloison vérifiée par grep).
SCPS_OBJS := $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_render.o \
             $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o \
             $(OBJDIR)/scps_scps_trade.o $(OBJDIR)/scps_scps_tech.o \
             $(OBJDIR)/scps_scps_core.o $(OBJDIR)/scps_scps_legitimacy.o \
             $(OBJDIR)/scps_scps_prosperity.o $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_lang.o \
             $(OBJDIR)/scps_scps_species.o $(OBJDIR)/scps_scps_diplo.o \
             $(OBJDIR)/scps_scps_routes.o $(OBJDIR)/scps_scps_statecraft.o \
             $(OBJDIR)/scps_scps_agency.o $(OBJDIR)/scps_scps_events.o $(OBJDIR)/scps_scps_provlog.o \
             $(OBJDIR)/scps_scps_demography.o $(OBJDIR)/scps_scps_labor.o \
             $(OBJDIR)/scps_scps_modifier.o $(OBJDIR)/scps_scps_revolt.o $(OBJDIR)/scps_scps_missions.o $(OBJDIR)/scps_scps_intertrade.o \
             $(OBJDIR)/scps_scps_army.o $(OBJDIR)/scps_scps_warhost.o $(OBJDIR)/scps_scps_campaign.o \
             $(OBJDIR)/scps_scps_navy.o $(OBJDIR)/scps_scps_endgame.o \
             $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_ai.o $(OBJDIR)/scps_scps_credit.o $(OBJDIR)/scps_scps_crypt.o \
             $(OBJDIR)/scps_scps_save_io.o $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/tp_miniz.o \
             $(OBJDIR)/scps_scps_audio.o $(OBJDIR)/tp_stbiw.o $(OBJDIR)/tp_miniaudio.o $(OBJDIR)/scps_viewer.o
SCPS_TARGET := scps_viewer$(EXE)
# Sous DEV : l'overlay Nuklear rejoint le lien, le binaire change de NOM (le
# release garde scps_viewer, intact).
ifeq ($(DEV),1)
  SCPS_OBJS   += $(OBJDIR)/scps_dev_overlay.o
  SCPS_TARGET := scps_viewer_dev$(EXE)
endif

HAVE_SDL := $(shell sdl2-config --version 2>/dev/null)
scps:
	@test -n "$(HAVE_SDL)" || { echo "SDL2 dev headers introuvables : installer libsdl2-dev, ou bâtir les cibles non-viewer (make core_demo, chronicle, test, …)"; exit 1; }
	@$(MAKE) --no-print-directory $(SCPS_TARGET)
$(SCPS_TARGET): $(SCPS_OBJS)
	$(CC) $(SCPS_OBJS) -o $@ $(SDL_LIBS) -lSDL2_ttf -lm $(WINLIBS) $(OMPFLAG) $(AUDIO_LIBS)

# dev_overlay porte l'implémentation Nuklear (single-header) : compilé À PART,
# sans -Wextra (la lib est vendorée), toujours -DSCPS_DEV.
$(OBJDIR)/scps_dev_overlay.o: scps/dev_overlay.c | $(OBJDIR)
	$(CC) -O0 -g -DSCPS_DEV -Ithird_party $(SDL_CFLAGS) -c $< -o $@

# ---- make dev : le viewer + overlay F3 (-DSCPS_DEV), OBJDIR isolé ---------
dev:
	$(MAKE) DEV=1 OBJDIR=build_dev scps
.PHONY: dev
run_scps: scps
	./$(SCPS_TARGET)

# ---- Générateur d'images headless (sans SDL) -----------------------------
SCPS_DUMP_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_tech.o $(OBJDIR)/scps_scps_render.o \
                  $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_labor.o \
                  $(OBJDIR)/scps_scps_species.o $(OBJDIR)/scps_dump.o
scps_dump: $(SCPS_DUMP_OBJS)
	$(CC) $(SCPS_DUMP_OBJS) -o $@ -lm

# ---- Captures de carte headless : N graines → PPM (vue terrain/politique) -
MAPSHOT_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_tech.o $(OBJDIR)/scps_scps_render.o \
                $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_labor.o \
                $(OBJDIR)/scps_scps_species.o $(OBJDIR)/scps_mapshot.o
mapshot: $(MAPSHOT_OBJS)
	$(CC) $(MAPSHOT_OBJS) -o $@ -lm

# ---- Diagnostic éco headless : satisfaction par strate & prix (réglage TAX_RATE) -
ECON_SCAN_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_tech.o $(OBJDIR)/scps_scps_render.o \
                  $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_labor.o $(OBJDIR)/scps_scps_trade.o \
                  $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_species.o $(OBJDIR)/scps_econ_scan.o
econ_scan: $(ECON_SCAN_OBJS)
	$(CC) $(ECON_SCAN_OBJS) -o $@ -lm

# ---- Planche-contact de 5 mondes (montage.bmp) ---------------------------
SCPS_BATCH_OBJS := $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_render.o \
                   $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_species.o \
                   $(OBJDIR)/tp_stbiw.o $(OBJDIR)/scps_batch.o
scps_batch: $(SCPS_BATCH_OBJS)
	$(CC) $(SCPS_BATCH_OBJS) -o $@ -lm

# ---- Banc d'essai économie + commerce ------------------------------------
ECON_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_tech.o $(OBJDIR)/scps_scps_render.o \
                  $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_labor.o $(OBJDIR)/scps_scps_trade.o \
                  $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_species.o $(OBJDIR)/scps_econ_demo.o
econ_demo: $(ECON_DEMO_OBJS)
	$(CC) $(ECON_DEMO_OBJS) -o $@ -lm

ECON_TAX_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_tech.o $(OBJDIR)/scps_scps_render.o \
                  $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_labor.o $(OBJDIR)/scps_scps_trade.o \
                  $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_species.o $(OBJDIR)/scps_econ_tax_demo.o
econ_tax_demo: $(ECON_TAX_DEMO_OBJS)
	$(CC) $(ECON_TAX_DEMO_OBJS) -o $@ -lm

ECON_CULTURE_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_tech.o $(OBJDIR)/scps_scps_render.o \
                  $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_labor.o $(OBJDIR)/scps_scps_trade.o \
                  $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_species.o $(OBJDIR)/scps_econ_culture_demo.o
econ_culture_demo: $(ECON_CULTURE_DEMO_OBJS)
	$(CC) $(ECON_CULTURE_DEMO_OBJS) -o $@ -lm

ECON_ARCANE_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_render.o \
                  $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_labor.o $(OBJDIR)/scps_scps_trade.o \
                  $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_tech.o \
                  $(OBJDIR)/scps_scps_core.o $(OBJDIR)/scps_scps_legitimacy.o \
                  $(OBJDIR)/scps_scps_prosperity.o $(OBJDIR)/scps_scps_species.o \
                  $(OBJDIR)/scps_scps_diplo.o $(OBJDIR)/scps_econ_arcane_demo.o
econ_arcane_demo: $(ECON_ARCANE_DEMO_OBJS)
	$(CC) $(ECON_ARCANE_DEMO_OBJS) -o $@ -lm

ECON_PRODUCTION_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_tech.o $(OBJDIR)/scps_scps_render.o \
                  $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_labor.o $(OBJDIR)/scps_scps_trade.o \
                  $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_species.o $(OBJDIR)/scps_econ_production_demo.o
econ_production_demo: $(ECON_PRODUCTION_DEMO_OBJS)
	$(CC) $(ECON_PRODUCTION_DEMO_OBJS) -o $@ -lm

# ---- Banc d'essai de l'arbre de technologies -----------------------------
TECH_DEMO_OBJS := $(OBJDIR)/scps_scps_tech.o $(OBJDIR)/scps_tech_demo.o
tech_demo: $(TECH_DEMO_OBJS)
	$(CC) $(TECH_DEMO_OBJS) -o $@ -lm

# ---- Banc d'essai des pools culturels ------------------------------------
CULTURE_DEMO_OBJS := $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_culture_demo.o
culture_demo: $(CULTURE_DEMO_OBJS)
	$(CC) $(CULTURE_DEMO_OBJS) -o $@ -lm

# ---- Banc d'essai du générateur de prospérité PE/SI ----------------------
PROSPERITY_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_render.o \
                        $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_labor.o $(OBJDIR)/scps_scps_trade.o \
                        $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_tech.o \
                        $(OBJDIR)/scps_scps_core.o $(OBJDIR)/scps_scps_legitimacy.o \
                        $(OBJDIR)/scps_scps_prosperity.o $(OBJDIR)/scps_scps_species.o $(OBJDIR)/scps_prosperity_demo.o
prosperity_demo: $(PROSPERITY_DEMO_OBJS)
	$(CC) $(PROSPERITY_DEMO_OBJS) -o $@ -lm

# ---- Couche d'agency : actions, temps, bâtiments-leviers (§1-§2) ----------
AGENCY_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_demography.o $(OBJDIR)/scps_scps_modifier.o $(OBJDIR)/scps_scps_render.o \
                    $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_labor.o $(OBJDIR)/scps_scps_trade.o \
                    $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_tech.o \
                    $(OBJDIR)/scps_scps_core.o $(OBJDIR)/scps_scps_legitimacy.o \
                    $(OBJDIR)/scps_scps_prosperity.o $(OBJDIR)/scps_scps_species.o \
                    $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_agency.o \
                    $(OBJDIR)/scps_scps_diplo.o $(OBJDIR)/scps_scps_routes.o $(OBJDIR)/scps_scps_intertrade.o $(OBJDIR)/scps_scps_credit.o \
                    $(OBJDIR)/scps_agency_demo.o
agency_demo: $(AGENCY_DEMO_OBJS)
	$(CC) $(AGENCY_DEMO_OBJS) -o $@ -lm

# ---- Diplomatie & guerre (§5-§6) -----------------------------------------
DIPLO_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_render.o \
                   $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_labor.o $(OBJDIR)/scps_scps_trade.o \
                   $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_tech.o \
                   $(OBJDIR)/scps_scps_core.o $(OBJDIR)/scps_scps_legitimacy.o \
                   $(OBJDIR)/scps_scps_prosperity.o $(OBJDIR)/scps_scps_species.o \
                   $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_diplo.o \
                   $(OBJDIR)/scps_diplo_demo.o
diplo_demo: $(DIPLO_DEMO_OBJS)
	$(CC) $(DIPLO_DEMO_OBJS) -o $@ -lm

FAITH_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_render.o \
                   $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_labor.o $(OBJDIR)/scps_scps_trade.o \
                   $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_tech.o \
                   $(OBJDIR)/scps_scps_core.o $(OBJDIR)/scps_scps_legitimacy.o \
                   $(OBJDIR)/scps_scps_prosperity.o $(OBJDIR)/scps_scps_species.o \
                   $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_faith.o \
                   $(OBJDIR)/scps_faith_demo.o
faith_demo: $(FAITH_DEMO_OBJS)
	$(CC) $(FAITH_DEMO_OBJS) -o $@ -lm

FACTIONS_DEMO_OBJS := $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_lang.o \
                      $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_core.o \
                      $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_culture.o \
                      $(OBJDIR)/scps_scps_species.o $(OBJDIR)/scps_scps_tech.o $(OBJDIR)/scps_scps_tune.o \
                      $(OBJDIR)/scps_factions_demo.o
factions_demo: $(FACTIONS_DEMO_OBJS)
	$(CC) $(FACTIONS_DEMO_OBJS) -o $@ -lm

MISSIONS_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_render.o \
                   $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_labor.o $(OBJDIR)/scps_scps_trade.o \
                   $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_tech.o \
                   $(OBJDIR)/scps_scps_core.o $(OBJDIR)/scps_scps_legitimacy.o \
                   $(OBJDIR)/scps_scps_prosperity.o $(OBJDIR)/scps_scps_species.o \
                   $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_missions.o \
                   $(OBJDIR)/scps_missions_demo.o
missions_demo: $(MISSIONS_DEMO_OBJS)
	$(CC) $(MISSIONS_DEMO_OBJS) -o $@ -lm

INTERTRADE_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_render.o \
                   $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_labor.o $(OBJDIR)/scps_scps_trade.o \
                   $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_tech.o \
                   $(OBJDIR)/scps_scps_core.o $(OBJDIR)/scps_scps_legitimacy.o \
                   $(OBJDIR)/scps_scps_prosperity.o $(OBJDIR)/scps_scps_species.o \
                   $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_diplo.o \
                   $(OBJDIR)/scps_scps_routes.o $(OBJDIR)/scps_scps_intertrade.o \
                   $(OBJDIR)/scps_intertrade_demo.o
intertrade_demo: $(INTERTRADE_DEMO_OBJS)
	$(CC) $(INTERTRADE_DEMO_OBJS) -o $@ -lm

WARHOST_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_render.o \
                   $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_trade.o \
                   $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_tech.o \
                   $(OBJDIR)/scps_scps_core.o $(OBJDIR)/scps_scps_legitimacy.o \
                   $(OBJDIR)/scps_scps_prosperity.o $(OBJDIR)/scps_scps_species.o \
                   $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_diplo.o \
                   $(OBJDIR)/scps_scps_army.o $(OBJDIR)/scps_scps_labor.o \
                   $(OBJDIR)/scps_scps_warhost.o $(OBJDIR)/scps_warhost_demo.o
warhost_demo: $(WARHOST_DEMO_OBJS)
	$(CC) $(WARHOST_DEMO_OBJS) -o $@ -lm

# ---- navy_demo : LA FLOTTE (rade · chantier · emport · conversion · invariants save_sane) ----
NAVY_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_render.o \
                  $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_trade.o \
                  $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_tech.o \
                  $(OBJDIR)/scps_scps_core.o $(OBJDIR)/scps_scps_legitimacy.o \
                  $(OBJDIR)/scps_scps_prosperity.o $(OBJDIR)/scps_scps_species.o \
                  $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_lang.o \
                  $(OBJDIR)/scps_scps_diplo.o $(OBJDIR)/scps_scps_labor.o \
                  $(OBJDIR)/scps_scps_routes.o $(OBJDIR)/scps_scps_intertrade.o \
                  $(OBJDIR)/scps_scps_army.o $(OBJDIR)/scps_scps_campaign.o \
                  $(OBJDIR)/scps_scps_navy.o $(OBJDIR)/scps_navy_demo.o
navy_demo: $(NAVY_DEMO_OBJS)
	$(CC) $(NAVY_DEMO_OBJS) -o $@ -lm

# ---- La campagne : les armées sur la carte (marche, siège, bataille) -------
# Pose les primitives combat-dans-le-temps (déplacement/siège/bataille/doctrine)
# sur une vraie carte ; non-invasif (lecture seule sur econ).
CAMPAIGN_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_render.o \
                   $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_trade.o \
                   $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_tech.o \
                   $(OBJDIR)/scps_scps_core.o $(OBJDIR)/scps_scps_legitimacy.o \
                   $(OBJDIR)/scps_scps_prosperity.o $(OBJDIR)/scps_scps_species.o \
                   $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_diplo.o \
                   $(OBJDIR)/scps_scps_army.o $(OBJDIR)/scps_scps_labor.o \
                   $(OBJDIR)/scps_scps_campaign.o $(OBJDIR)/scps_campaign_demo.o
campaign_demo: $(CAMPAIGN_DEMO_OBJS)
	$(CC) $(CAMPAIGN_DEMO_OBJS) -o $@ -lm

# ---- Routes commerciales : la cloche f(D̄) faite action (§7) --------------
ROUTES_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_render.o \
                    $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_labor.o $(OBJDIR)/scps_scps_trade.o \
                    $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_tech.o \
                    $(OBJDIR)/scps_scps_core.o $(OBJDIR)/scps_scps_legitimacy.o \
                    $(OBJDIR)/scps_scps_prosperity.o $(OBJDIR)/scps_scps_species.o \
                    $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_routes.o \
                    $(OBJDIR)/scps_routes_demo.o
routes_demo: $(ROUTES_DEMO_OBJS)
	$(CC) $(ROUTES_DEMO_OBJS) -o $@ -lm

# ---- Boucle de décision IA : un lecteur de coordonnées qui choisit des leviers (§13.1)
# Aucune dépendance membrane (l'IA lit les coordonnées du moteur, pas les mots).
AI_DEMO_OBJS := $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_demography.o $(OBJDIR)/scps_scps_modifier.o $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_labor.o \
                $(OBJDIR)/scps_scps_trade.o $(OBJDIR)/scps_scps_culture.o \
                $(OBJDIR)/scps_scps_tech.o $(OBJDIR)/scps_scps_core.o \
                $(OBJDIR)/scps_scps_legitimacy.o $(OBJDIR)/scps_scps_prosperity.o \
                $(OBJDIR)/scps_scps_species.o $(OBJDIR)/scps_scps_agency.o \
                $(OBJDIR)/scps_scps_routes.o $(OBJDIR)/scps_scps_diplo.o $(OBJDIR)/scps_scps_intertrade.o \
                $(OBJDIR)/scps_scps_statecraft.o \
                $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_ai.o $(OBJDIR)/scps_scps_credit.o \
                $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_ai_demo.o
ai_demo: $(AI_DEMO_OBJS)
	$(CC) $(AI_DEMO_OBJS) -o $@ -lm

# ---- Les fourches (arc M) : éthos→fonction, factions→pôle -----------------
FORKS_DEMO_OBJS := $(filter-out $(OBJDIR)/scps_ai_demo.o,$(AI_DEMO_OBJS)) $(OBJDIR)/scps_forks_demo.o
forks_demo: $(FORKS_DEMO_OBJS)
	$(CC) $(FORKS_DEMO_OBJS) -o $@ -lm

CREDIT_DEMO_OBJS := $(filter-out $(OBJDIR)/scps_ai_demo.o,$(AI_DEMO_OBJS)) $(OBJDIR)/scps_credit_demo.o
credit_demo: $(CREDIT_DEMO_OBJS)
	$(CC) $(CREDIT_DEMO_OBJS) -o $@ -lm

CAP_DEMO_OBJS := $(filter-out $(OBJDIR)/scps_ai_demo.o,$(AI_DEMO_OBJS)) $(OBJDIR)/scps_cap_demo.o
cap_demo: $(CAP_DEMO_OBJS)
	$(CC) $(CAP_DEMO_OBJS) -o $@ -lm

CHRONICLE_OBJS := $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o \
                  $(OBJDIR)/scps_scps_trade.o $(OBJDIR)/scps_scps_culture.o \
                  $(OBJDIR)/scps_scps_tech.o $(OBJDIR)/scps_scps_core.o \
                  $(OBJDIR)/scps_scps_legitimacy.o $(OBJDIR)/scps_scps_prosperity.o \
                  $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_species.o \
                  $(OBJDIR)/scps_scps_diplo.o $(OBJDIR)/scps_scps_routes.o $(OBJDIR)/scps_scps_intertrade.o \
                  $(OBJDIR)/scps_scps_statecraft.o $(OBJDIR)/scps_scps_agency.o \
                  $(OBJDIR)/scps_scps_events.o $(OBJDIR)/scps_scps_provlog.o $(OBJDIR)/scps_scps_demography.o \
                  $(OBJDIR)/scps_scps_labor.o $(OBJDIR)/scps_scps_modifier.o \
                  $(OBJDIR)/scps_scps_revolt.o $(OBJDIR)/scps_scps_army.o \
                  $(OBJDIR)/scps_scps_warhost.o $(OBJDIR)/scps_scps_campaign.o $(OBJDIR)/scps_scps_missions.o \
                  $(OBJDIR)/scps_scps_navy.o $(OBJDIR)/scps_scps_endgame.o $(OBJDIR)/tp_miniz.o \
                  $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_ai.o $(OBJDIR)/scps_scps_credit.o \
                  $(OBJDIR)/scps_scps_religion.o \
                  $(OBJDIR)/scps_scps_sim.o $(OBJDIR)/scps_chronicle.o
chronicle: $(CHRONICLE_OBJS)
	$(CC) $(CHRONICLE_OBJS) -o $@ -lm $(OMPFLAG)

# ---- Banc PERMANENT de l'arc « une économie » : 4 bornes auto-vérifiées ----
#   1. pop d'un hameau ×[1.1..2.5] en 10 ans   2. conso == pop/100
#   3. flux d'or ≠ constante (variance > 0)    4. premier 360 j payé ≤ an 4
AUDIT_OBJS := $(filter-out $(OBJDIR)/scps_chronicle.o,$(CHRONICLE_OBJS)) $(OBJDIR)/scps_audit_eco.o
audit_eco: $(AUDIT_OBJS)
	$(CC) $(AUDIT_OBJS) -o $@ -lm $(OMPFLAG)
audit: audit_eco
	./audit_eco 7 10
.PHONY: audit

# ---- scps_api_demo : le banc de la FAÇADE C (scps_api) -----------------------
# scps_api est la surface de binding pour un hôte natif (Godot/GDExtension) : le
# moteur reste 100 % C, l'hôte affiche/saisit. Ce banc prouve génération + rendu
# (render_map) + couches + avancement + REPRODUCTIBILITÉ, sans Godot. Lie le
# moteur complet + scps_render. (Le binding C++ Godot vit dans godot/, à part.)
API_DEMO_OBJS := $(filter-out $(OBJDIR)/scps_chronicle.o,$(CHRONICLE_OBJS)) \
                 $(OBJDIR)/scps_scps_crypt.o $(OBJDIR)/scps_scps_save_io.o $(OBJDIR)/scps_scps_save.o \
                 $(OBJDIR)/scps_scps_render.o $(OBJDIR)/scps_scps_api.o $(OBJDIR)/scps_scps_api_demo.o
scps_api_demo: $(API_DEMO_OBJS)
	$(CC) $(API_DEMO_OBJS) -o $@ -lm $(OMPFLAG)

# ---- fx-proof : PREUVE VISUELLE headless des animations FX (hors test) -------
# Composite les 4 planches FX (mer/côte/armée/vortex) sur un terrain render_map
# RÉEL via un renderer LOGICIEL → fx_proof.png. Aucun affichage requis (driver
# vidéo « dummy »). Requiert SDL2 (comme le viewer). Outil de vérif, pas un banc.
FX_PROOF_SRCS := $(patsubst $(OBJDIR)/scps_%.o,scps/%.c,\
                   $(filter-out $(OBJDIR)/tp_miniz.o $(OBJDIR)/scps_chronicle.o,$(CHRONICLE_OBJS))) \
                 scps/scps_render.c
fx-proof:
	@test -n "$(HAVE_SDL)" || { echo "SDL2 introuvable : installer libsdl2-dev"; exit 1; }
	$(CC) -O2 -std=c99 -Iscps -Ithird_party $(SDL_CFLAGS) tools/fx_proof.c $(FX_PROOF_SRCS) -o fx_proof $(SDL_LIBS) -lm
.PHONY: fx-proof

# ---- Banc audio : le mixeur procédural sort du son (build §9.6) -----------
AUDIO_DEMO_OBJS := $(OBJDIR)/scps_scps_audio.o $(OBJDIR)/tp_miniaudio.o $(OBJDIR)/scps_audio_demo.o
audio_demo: $(AUDIO_DEMO_OBJS)
	$(CC) $(AUDIO_DEMO_OBJS) -o $@ $(AUDIO_LIBS)

# ---- Banc save_io : compression de bloc + CRC32 round-trip (build §9.5) ---
SAVE_IO_DEMO_OBJS := $(OBJDIR)/scps_scps_save_io.o $(OBJDIR)/tp_miniz.o $(OBJDIR)/scps_save_io_demo.o
save_io_demo: $(SAVE_IO_DEMO_OBJS)
	$(CC) $(SAVE_IO_DEMO_OBJS) -o $@ -lm

# ---- LE HARNAIS DE DÉTERMINISME (brief build §2) — le juge de paix --------
# Deux balayages IDENTIQUES (5 graines × 12 ans), comparaison des HASH par sim :
# vert si reproductible, rouge à la moindre divergence. À RELANCER après chaque
# pragma OpenMP retenu (§4) — un speedup qui casse le hash est une dette.
# Horizon court À DESSEIN : worldgen (la cible §4) est figé dès l'an 0 et toute
# divergence cascade aussitôt (courants → routes → colonisation) ; 12 ans
# couvrent aussi les réductions éco/sim — assez pour juger, assez bref pour
# rejuger après chaque boucle. (DET_YEARS surchargeable : `make determinism DET_YEARS=40`.)
DET_YEARS ?= 12
determinism: chronicle
	@A=$$(./chronicle --hash 7 5 $(DET_YEARS) 2>/dev/null | grep '^HASH'); \
	 B=$$(./chronicle --hash 7 5 $(DET_YEARS) 2>/dev/null | grep '^HASH'); \
	 if [ "$$A" = "$$B" ] && [ -n "$$A" ]; then \
	   echo "determinism OK : $$(printf '%s\n' "$$A" | wc -l) sims, hashes STABLES (5 graines × $(DET_YEARS) ans)"; \
	   printf '%s\n' "$$A"; \
	 else \
	   echo "determinism ÉCHEC : deux runs IDENTIQUES divergent —"; \
	   echo "  run A :"; printf '%s\n' "$$A"; \
	   echo "  run B :"; printf '%s\n' "$$B"; exit 1; \
	 fi
.PHONY: determinism

# ---- make determinism-deep : le déterminisme sur des SIÈCLES (nightly, pas pre-commit) ----
# Le gate à 12 ans (ci-dessus) n'exerce JAMAIS l'endgame §27 (ENDGAME_YEAR_OPEN=180), le clamp
# anti-emballement crédit (tardif), ni les mutations de carte (cataclysm_sink_region / rebiome) —
# exactement les siècles où vit une partie joueur. On rejoue 2 graines × DEEP_YEARS ans DEUX FOIS
# et on exige A==B (self-cohérence longue ; complète le golden 12 ans).
DEEP_YEARS ?= 200
determinism-deep: chronicle
	@ok=1; for s in 7 9; do \
	   A=$$(./chronicle --hash $$s 2 $(DEEP_YEARS) 2>/dev/null | grep '^HASH'); \
	   B=$$(./chronicle --hash $$s 2 $(DEEP_YEARS) 2>/dev/null | grep '^HASH'); \
	   if [ -n "$$A" ] && [ "$$A" = "$$B" ]; then echo "deep graine $$s : STABLE ($(DEEP_YEARS) ans)"; \
	   else echo "deep graine $$s : DIVERGE —"; printf 'A:\n%s\nB:\n%s\n' "$$A" "$$B"; ok=0; fi; \
	 done; [ $$ok -eq 1 ] || { echo "determinism-deep ÉCHEC"; exit 1; }
.PHONY: determinism-deep

# ---- make golden : NON-RÉGRESSION du monde (le hash comme CONTRAT public) --
# `determinism` ci-dessus prouve la SELF-COHÉRENCE (A==B sur le MÊME binaire) — il reste VERT
# si un changement DÉCALE le monde, tant que les deux runs du nouveau binaire concordent. Or
# « monde reproductible » (graines partageables, replays, compat-save) repose sur le hash comme
# CONTRAT. `golden` diffe la sortie courante CONTRE un golden COMMITÉ (scps/golden_hashes.txt) :
# une re-baseline DÉLIBÉRÉE devient un diff d'UNE LIGNE, revu (puis `make golden-update`), au
# lieu d'un glissement silencieux. (Le golden suit DET_YEARS=12 ; le long-run est un gate à part.)
golden: chronicle
	@CUR=$$(./chronicle --hash 7 5 $(DET_YEARS) 2>/dev/null | grep '^HASH'); \
	 GOLD=$$(cat scps/golden_hashes.txt 2>/dev/null); \
	 if [ -n "$$CUR" ] && [ "$$CUR" = "$$GOLD" ]; then \
	   echo "golden OK : hash monde IDENTIQUE au golden commité (5 graines × $(DET_YEARS) ans)"; \
	 else \
	   echo "golden ÉCHEC : le hash monde a CHANGÉ vs scps/golden_hashes.txt —"; \
	   echo "  re-baseline DÉLIBÉRÉE ? → revoir le diff ci-dessous, puis : make golden-update"; \
	   printf '  golden :\n%s\n  actuel :\n%s\n' "$$GOLD" "$$CUR"; exit 1; \
	 fi
.PHONY: golden

# Re-baseline ASSUMÉE : régénère le golden (à committer, le diff étant revu — le pendant outillé
# des notes « ⚠ RE-BASELINE » faites à la main).
golden-update: chronicle
	@./chronicle --hash 7 5 $(DET_YEARS) 2>/dev/null | grep '^HASH' > scps/golden_hashes.txt
	@echo "golden RE-BASELINÉ : scps/golden_hashes.txt mis à jour (à committer, diff revu)."
.PHONY: golden-update

# ---- make fuzz-save : DURCISSEMENT du save (audit P0-1, bonus) ------------
# (1) forge chaque COMPTEUR désérialisé hors-borne → save_sane DOIT rejeter (le vecteur d'écriture
# hors-bornes) ; (2) fuzz d'octets du fichier → game_load ne plante JAMAIS. Headless via SDL dummy.
# HORS `make test` : le viewer a besoin de SDL ; les bancs, non. (~50 s ; idéal sous un build ASan.)
fuzz-save: scps_viewer
	@out=$$(SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./scps_viewer --fuzztest 9 2>&1); rc=$$?; \
	 printf '%s\n' "$$out" | grep -E "BILAN|✗|flippés"; \
	 if [ $$rc -eq 0 ]; then echo "fuzz-save OK"; else echo "fuzz-save ÉCHEC (rc=$$rc)"; exit 1; fi
.PHONY: fuzz-save

# ---- Diagnostic mémoire : chronicle sous AddressSanitizer + UBSan ---------
# Compile les sources d'un bloc AVEC les sanitizers (compile + link ensemble),
# pour traquer double-free, use-after-free, hors-bornes et comportement indéfini :
#   make asan && ./chronicle_asan 7 1 40 6 12
# tp_miniz.o ne matche pas le motif scps_%.o : on le FILTRE et on ajoute miniz.c
# (avec ses flags) à la main — sinon CHRONICLE_SRCS garde « build/tp_miniz.o »
# (un objet, pas une source) et miniz.h reste introuvable faute de -Ithird_party.
CHRONICLE_SRCS := $(patsubst $(OBJDIR)/scps_%.o,scps/%.c,\
                    $(filter-out $(OBJDIR)/tp_miniz.o,$(CHRONICLE_OBJS)))
asan: $(CHRONICLE_SRCS)
	$(CC) -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer \
	      -Wall -Wextra -std=c99 -Ithird_party $(MINIZ_FLAGS) \
	      $(CHRONICLE_SRCS) third_party/miniz.c -o chronicle_asan -lm

# ---- Métriques de jeu (0-100), Influence, Diplomates & Révolte -----------
# La membrane projette les coordonnées en nombres+mots ; le statecraft est SIM
# (il lit des flottants), son API ne rend que des entiers de jeu.
STATECRAFT_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_labor.o \
                        $(OBJDIR)/scps_scps_trade.o $(OBJDIR)/scps_scps_culture.o \
                        $(OBJDIR)/scps_scps_tech.o $(OBJDIR)/scps_scps_core.o \
                        $(OBJDIR)/scps_scps_legitimacy.o $(OBJDIR)/scps_scps_prosperity.o \
                        $(OBJDIR)/scps_scps_species.o $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_lang.o \
                        $(OBJDIR)/scps_scps_diplo.o $(OBJDIR)/scps_scps_routes.o $(OBJDIR)/scps_scps_intertrade.o \
                        $(OBJDIR)/scps_scps_statecraft.o $(OBJDIR)/scps_statecraft_demo.o
statecraft_demo: $(STATECRAFT_DEMO_OBJS)
	$(CC) $(STATECRAFT_DEMO_OBJS) -o $@ -lm

# ---- Évènements, chocs géo & âges : la dynamique du monde -----------------
# Chocs ancrés dans la géo (failles/rivières/pluie/routes), évènements par la
# fiche, âges déclenchés par l'état du monde. Effets = coordonnées/métriques.
EVENTS_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_labor.o \
                    $(OBJDIR)/scps_scps_trade.o $(OBJDIR)/scps_scps_culture.o \
                    $(OBJDIR)/scps_scps_tech.o $(OBJDIR)/scps_scps_core.o \
                    $(OBJDIR)/scps_scps_legitimacy.o $(OBJDIR)/scps_scps_prosperity.o \
                    $(OBJDIR)/scps_scps_species.o $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_lang.o \
                    $(OBJDIR)/scps_scps_diplo.o $(OBJDIR)/scps_scps_routes.o $(OBJDIR)/scps_scps_intertrade.o \
                    $(OBJDIR)/scps_scps_statecraft.o $(OBJDIR)/scps_scps_events.o $(OBJDIR)/scps_scps_provlog.o \
                    $(OBJDIR)/scps_events_demo.o
events_demo: $(EVENTS_DEMO_OBJS)
	$(CC) $(EVENTS_DEMO_OBJS) -o $@ -lm

# ---- Âges structurels : Lumières, Soulèvements, l'Ordre de Fer ------------
# Ils poussent les ENTRÉES du moteur d'ordre ; le verdict §2.4 fait le reste.
STRUCTURAL_DEMO_OBJS := $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_demography.o $(OBJDIR)/scps_scps_modifier.o $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_labor.o \
                    $(OBJDIR)/scps_scps_trade.o $(OBJDIR)/scps_scps_culture.o \
                    $(OBJDIR)/scps_scps_tech.o $(OBJDIR)/scps_scps_core.o \
                    $(OBJDIR)/scps_scps_legitimacy.o $(OBJDIR)/scps_scps_prosperity.o \
                    $(OBJDIR)/scps_scps_species.o $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_lang.o \
                    $(OBJDIR)/scps_scps_diplo.o $(OBJDIR)/scps_scps_routes.o $(OBJDIR)/scps_scps_intertrade.o \
                    $(OBJDIR)/scps_scps_statecraft.o $(OBJDIR)/scps_scps_agency.o \
                    $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_ai.o $(OBJDIR)/scps_scps_credit.o $(OBJDIR)/scps_scps_events.o $(OBJDIR)/scps_scps_provlog.o \
                    $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_structural_demo.o
structural_demo: $(STRUCTURAL_DEMO_OBJS)
	$(CC) $(STRUCTURAL_DEMO_OBJS) -o $@ -lm

# ---- L'économie des populations : main-d'œuvre, jobs, matériaux, marché ---
# La prod scale sur les JOBS REMPLIS ; les sorties LISENT la géo du worldgen.
LABOR_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_tech.o $(OBJDIR)/scps_scps_culture.o \
                   $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_species.o \
                   $(OBJDIR)/scps_scps_labor.o $(OBJDIR)/scps_labor_demo.o
labor_demo: $(LABOR_DEMO_OBJS)
	$(CC) $(LABOR_DEMO_OBJS) -o $@ -lm

# ---- Les armées : recrutement, armes, contres, combat au dé ---------------
# Bâti sur l'économie (pop par classe + armes fabriquées). Autonome (pas de SDL).
# ---- La population PRÉCISE : race × culture × foi × classe émergente -------
POP_DEMO_OBJS := $(OBJDIR)/scps_scps_popsim.o $(OBJDIR)/scps_scps_culture.o \
                 $(OBJDIR)/scps_scps_species.o $(OBJDIR)/scps_pop_demo.o
pop_demo: $(POP_DEMO_OBJS)
	$(CC) $(POP_DEMO_OBJS) -o $@ -lm

ARMY_DEMO_OBJS := $(OBJDIR)/scps_scps_labor.o $(OBJDIR)/scps_scps_army.o \
                  $(OBJDIR)/scps_scps_tech.o $(OBJDIR)/scps_army_demo.o
army_demo: $(ARMY_DEMO_OBJS)
	$(CC) $(ARMY_DEMO_OBJS) -o $@ -lm

# ---- Le refactor démographique : la province contient des GROUPES (clé de voûte)
# Branche scps_modifier (pile de dérive). Alimente scps_order (inchangé).
DEMOGRAPHY_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o \
                    $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_species.o \
                    $(OBJDIR)/scps_scps_tech.o $(OBJDIR)/scps_scps_core.o \
                    $(OBJDIR)/scps_scps_legitimacy.o $(OBJDIR)/scps_scps_prosperity.o \
                    $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_lang.o $(OBJDIR)/scps_scps_modifier.o \
                    $(OBJDIR)/scps_scps_demography.o $(OBJDIR)/scps_scps_labor.o \
                    $(OBJDIR)/scps_scps_diplo.o $(OBJDIR)/scps_scps_routes.o $(OBJDIR)/scps_demography_demo.o
demography_demo: $(DEMOGRAPHY_DEMO_OBJS)
	$(CC) $(DEMOGRAPHY_DEMO_OBJS) -o $@ -lm

# ---- L'intégration au moteur vivant (la province réelle porte des groupes) -
DEMOGRAPHY_INTEG_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o \
                    $(OBJDIR)/scps_scps_trade.o $(OBJDIR)/scps_scps_culture.o \
                    $(OBJDIR)/scps_scps_species.o $(OBJDIR)/scps_scps_tech.o \
                    $(OBJDIR)/scps_scps_core.o $(OBJDIR)/scps_scps_legitimacy.o \
                    $(OBJDIR)/scps_scps_prosperity.o $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_lang.o \
                    $(OBJDIR)/scps_scps_modifier.o $(OBJDIR)/scps_scps_demography.o $(OBJDIR)/scps_scps_labor.o \
                    $(OBJDIR)/scps_scps_diplo.o $(OBJDIR)/scps_scps_routes.o \
                    $(OBJDIR)/scps_demography_integ_demo.o
demography_integ_demo: $(DEMOGRAPHY_INTEG_OBJS)
	$(CC) $(DEMOGRAPHY_INTEG_OBJS) -o $@ -lm

# ---- La révolte INCARNÉE : un soulèvement est un acteur ancré sur un groupe -
# QUI se lève (pire déficit), COMBIEN (fraction mobilisée qui quitte le travail),
# ce qu'il VEUT (jacquerie/sécession/coup), ce qu'il ADVIENT (écrasé/né/concession).
REVOLT_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o \
                    $(OBJDIR)/scps_scps_trade.o $(OBJDIR)/scps_scps_culture.o \
                    $(OBJDIR)/scps_scps_species.o $(OBJDIR)/scps_scps_tech.o \
                    $(OBJDIR)/scps_scps_core.o $(OBJDIR)/scps_scps_legitimacy.o \
                    $(OBJDIR)/scps_scps_prosperity.o $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_lang.o \
                    $(OBJDIR)/scps_scps_diplo.o $(OBJDIR)/scps_scps_modifier.o \
                    $(OBJDIR)/scps_scps_demography.o $(OBJDIR)/scps_scps_labor.o $(OBJDIR)/scps_scps_factions.o \
                    $(OBJDIR)/scps_scps_revolt.o $(OBJDIR)/scps_revolt_demo.o
revolt_demo: $(REVOLT_DEMO_OBJS)
	$(CC) $(REVOLT_DEMO_OBJS) -o $@ -lm

# ---- Le tissu social : brasserie + boisson culturelle + foi (Temple→L) -----
# Première passe du catalogue SOCIAL : une chaîne (grain→bière), la variante
# culturelle du palier moral (bière/vin), et la foi qui soutient la légitimité.
SOCIAL_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_demography.o $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_labor.o \
                    $(OBJDIR)/scps_scps_trade.o $(OBJDIR)/scps_scps_culture.o \
                    $(OBJDIR)/scps_scps_species.o $(OBJDIR)/scps_scps_tech.o \
                    $(OBJDIR)/scps_scps_core.o $(OBJDIR)/scps_scps_legitimacy.o \
                    $(OBJDIR)/scps_scps_prosperity.o $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_lang.o \
                    $(OBJDIR)/scps_scps_diplo.o $(OBJDIR)/scps_scps_modifier.o \
                    $(OBJDIR)/scps_scps_routes.o $(OBJDIR)/scps_scps_intertrade.o \
                    $(OBJDIR)/scps_scps_agency.o $(OBJDIR)/scps_scps_credit.o $(OBJDIR)/scps_social_demo.o
social_demo: $(SOCIAL_DEMO_OBJS)
	$(CC) $(SOCIAL_DEMO_OBJS) -o $@ -lm

# ---- Capstone §27 : Entropie mondiale + 4 fins + Merveille ----------------
# Banc auto-vérifiant du cataclysme (contrôles C0-C6). N'a besoin que des
# briques moteur traversées par scps_endgame (econ/prosperity/tech/routes/navy/diplo).
ENDGAME_DEMO_OBJS := $(OBJDIR)/scps_scps_religion.o $(OBJDIR)/scps_scps_world.o $(OBJDIR)/scps_scps_render.o \
                     $(OBJDIR)/scps_scps_econ.o $(OBJDIR)/scps_scps_tune.o $(OBJDIR)/scps_scps_labor.o $(OBJDIR)/scps_scps_trade.o \
                     $(OBJDIR)/scps_scps_culture.o $(OBJDIR)/scps_scps_tech.o \
                     $(OBJDIR)/scps_scps_core.o $(OBJDIR)/scps_scps_legitimacy.o \
                     $(OBJDIR)/scps_scps_prosperity.o $(OBJDIR)/scps_scps_species.o \
                     $(OBJDIR)/scps_scps_factions.o $(OBJDIR)/scps_scps_readout.o $(OBJDIR)/scps_scps_lang.o \
                     $(OBJDIR)/scps_scps_diplo.o $(OBJDIR)/scps_scps_routes.o $(OBJDIR)/scps_scps_navy.o \
                     $(OBJDIR)/scps_scps_army.o $(OBJDIR)/scps_scps_campaign.o \
                     $(OBJDIR)/scps_scps_endgame.o $(OBJDIR)/scps_endgame_demo.o
endgame_demo: $(ENDGAME_DEMO_OBJS)
	$(CC) $(ENDGAME_DEMO_OBJS) -o $@ -lm

# Tous les binaires de bancs (MIROIR de run_tests.sh — à garder synchrone) +
# les legacy + les outils + les variantes .exe (MSYS2/Windows) et _asan.
BENCH_BINS := core_demo monde_reel readout_demo species_demo tech_demo faith_demo \
  intertrade_demo routes_demo save_io_demo statecraft_demo pop_demo army_demo \
  demography_demo demography_integ_demo revolt_demo social_demo agency_demo \
  campaign_demo factions_demo econ_tax_demo econ_culture_demo econ_arcane_demo \
  econ_production_demo labor_demo missions_demo ai_demo diplo_demo warhost_demo \
  events_demo structural_demo forks_demo prosperity_demo credit_demo cap_demo \
  endgame_demo audit_eco lang_demo scps_api_demo audio_demo econ_demo culture_demo navy_demo
TOOL_BINS := scps_viewer scps_dump scps_batch chronicle chronicle_asan econ_scan fx_proof

clean:
	rm -rf $(OBJDIR) $(BENCH_BINS) $(TOOL_BINS) \
	       $(addsuffix .exe,$(BENCH_BINS) $(TOOL_BINS)) \
	       out_*.ppm montage.bmp fx_proof.png

.PHONY: all scps run_scps clean core_demo monde_reel readout_demo lang_demo species_demo scps_dump scps_batch asan \
        econ_demo tech_demo culture_demo prosperity_demo agency_demo diplo_demo routes_demo ai_demo statecraft_demo events_demo

# Inclusion des fichiers de dépendances générés (-MMD). Le tiret ignore leur
# absence au premier build.
-include $(wildcard $(OBJDIR)/*.d)

# ---- lang-check : le CLIQUET de localisation (CLAUDE.md §langue) ----------
# Compte les littéraux face-joueur passés aux primitives d'affichage ; échoue
# si le compte MONTE au-dessus de la base (toute chaîne nouvelle naît en
# STR_*). Le reflux est le bienvenu : abaisser scps/lang_baseline.txt.
LANG_FACE := scps/viewer.c scps/scps_readout.c
lang-check:
	@n=$$(grep -hoE '(draw_text|sh_button|sh_slider|zone_add)\([^;]*"[A-Za-z]' $(LANG_FACE) | wc -l); \
	b=$$(cat scps/lang_baseline.txt); \
	if [ $$n -gt $$b ]; then \
	  echo "lang-check ÉCHEC : $$n littéraux face-joueur (base $$b) — toute chaîne NOUVELLE doit naître en STR_* (cf. CLAUDE.md)"; exit 1; \
	else \
	  echo "lang-check OK : $$n littéraux face-joueur (base $$b)"; \
	  if [ $$n -lt $$b ]; then echo "  (reflux : abaisser scps/lang_baseline.txt à $$n)"; fi; \
	fi
.PHONY: lang-check

# ---- membrane-check : LA CLOISON readout → renderer (CLAUDE.md §Disciplines) --
# Le RENDERER (viewer.c + scps_render.{c,h}, le côté display) ne doit JAMAIS
# inclure scps_core.h NI nommer un symbole exclusif du cœur (scps_order /
# scps_metabolisation / scps_bell, ScpsState / ScpsOrder — la physique §2.4,
# jamais lue côté affichage). rc≠0 à la moindre fuite.
#
# Robustesse aux COMMENTAIRES : on STRIPPE d'abord les commentaires C
# (gcc -fpreprocessed -dD -E ré-émet le fichier sans commentaires, sans
# expanser les #include — SDL.h absent n'a pas d'importance), PUIS on grep.
# Ainsi un commentaire de prose qui mentionne « scps_core.h » ou « scps_order »
# (viewer.c en a) n'est PAS un faux positif ; seul un VRAI include ou un VRAI
# usage de symbole déclenche. L'include est repéré par la directive nue
# (#include "scps_core.h"), jamais par une mention en clair.
MEMBRANE_FILES := scps/viewer.c scps/scps_render.c scps/scps_render.h
MEMBRANE_SYMS  := scps_order|scps_metabolisation|scps_bell|ScpsState|ScpsOrder
membrane-check:
	@viol=0; \
	for f in $(MEMBRANE_FILES); do \
	  [ -f "$$f" ] || continue; \
	  stripped=$$($(CC) -fpreprocessed -dD -E "$$f" 2>/dev/null); \
	  if printf '%s\n' "$$stripped" | grep -qE '^[[:space:]]*#[[:space:]]*include[[:space:]]*["<]scps_core\.h[">]'; then \
	    echo "membrane-check ÉCHEC : $$f INCLUT scps_core.h (la cloison readout→renderer est franchie)"; viol=1; \
	  fi; \
	  if printf '%s\n' "$$stripped" | grep -qwE '$(MEMBRANE_SYMS)'; then \
	    echo "membrane-check ÉCHEC : $$f nomme un symbole EXCLUSIF du cœur ($(MEMBRANE_SYMS)) — le renderer ne lit que des mots/bandes"; \
	    printf '%s\n' "$$stripped" | grep -nwE '$(MEMBRANE_SYMS)' | head -5; viol=1; \
	  fi; \
	done; \
	if [ $$viol -ne 0 ]; then exit 1; fi; \
	echo "membrane-check OK : $(words $(MEMBRANE_FILES)) fichiers renderer — ni scps_core.h ni symbole-cœur (cloison tenue)"
.PHONY: membrane-check

# ---- calibrate-smoke : le pilote de calibrage (Arc J3) tourne de bout en bout ----
calibrate-smoke: chronicle
	@python3 tools/calibrate.py --param ENTRETIEN_DIV:300:500:200 \
	  --target flux_or_med:-5:20 --target tresor_med::12000 \
	  --sims 1 --years 20 --seeds 7 --jobs 2 --yes
.PHONY: calibrate-smoke

# ---- make test : tous les bancs non-SDL (Arc K3.3) ; rc≠0 si un rouge ----
# membrane-check ET lang-check sont en DÉPENDANCE : les deux cliquets (cloison
# readout→renderer, ratchet de localisation) sont gardés AVANT les bancs, et
# leur rc≠0 stoppe `make test` (propagation native).
test: membrane-check lang-check
	@bash tools/run_tests.sh full
.PHONY: test

# ---- make smoke : feedback RAPIDE (la colonne vertébrale en quelques secondes) ----
# Les deux cliquets + un sous-ensemble de bancs (worldgen/éco/IA + bornes audit &
# langue). Pour la boucle serrée du dev ; le gardien COMPLET reste `make test`.
smoke: membrane-check lang-check
	@bash tools/run_tests.sh smoke
.PHONY: smoke

# ---- make full-test : LE GARDIEN LOURD (porte avant un push moteur) ----
# La suite COMPLÈTE, puis le juge de paix du déterminisme, puis ASan+UBSan muets
# sur un run court. Tout ce qui doit être vert avant de toucher au cœur.
full-test: test determinism golden asan
	@echo "── full-test : ASan+UBSan sur un run court (doit rester muet) ──"
	@./chronicle_asan 7 1 20 6 12 >/dev/null
	@echo "full-test OK : bancs + déterminisme + ASan/UBSan tous verts"
.PHONY: full-test
