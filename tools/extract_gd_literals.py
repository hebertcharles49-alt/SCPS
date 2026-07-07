#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
extract_gd_literals.py — recense les littéraux FACE-JOUEUR restants dans le
chrome Godot (godot/project/**/*.gd) et produit docs/i18n_backlog.csv : le
carnet de route de la migration tr() (lot E « version English »).

USAGE (depuis la racine du dépôt) :
    python tools/extract_gd_literals.py [--root godot/project] [--out docs/i18n_backlog.csv]

(⚠ Windows de ce poste : python n'est pas dans le PATH — lancer avec
 D:/MSYS2/mingw64/bin/python.exe ; le script lui-même est stdlib pur, portable.)

HEURISTIQUE (approximative, ASSUMÉE — un backlog, pas un compilateur) :
  · scan LIGNE PAR LIGNE ; les lignes-commentaire (#) sont ignorées ; les
    chaînes multi-lignes ne sont pas suivies (limitation documentée).
  · NIVEAU 1 — une ligne est un CONTEXTE D'AFFICHAGE si elle matche :
        .text = / +=   ·  tooltip_text  ·  placeholder_text  ·  add_item(
        set_text(      ·  window_title  ·  .title =          ·  add_tab(
        VKit.text( / text_map( / section( / row(   (le mode immédiat maison)
    (c'est là que le joueur LIT : Label/Button/tooltips/dessin VKit…)
    → tout littéral y est retenu s'il contient une lettre hors format.
  · NIVEAU 2 — HORS contexte (tableaux de noms TAB_NAME, args de helpers…),
    un littéral n'est retenu que s'il ressemble à de la PROSE : une lettre
    ACCENTUÉE, ou une ESPACE + un mot de ≥3 lettres — et que la ligne n'est
    pas de l'outillage (push_error/push_warning/print/assert/preload/load).
  · dans les deux niveaux, un littéral est REJETÉ s'il est :
        - déjà traduit : à l'intérieur d'un appel tr(...)
        - un chemin (res:// · user:// · uid://) ou une extension (.png…)
        - une clé technique : identifiant tout-minuscules (clé de Dictionary,
          "font_color"…), CLÉ_MAJUSCULE (T_X/STR_X), nom à la CamelCase
        - du pur format : plus aucune lettre une fois %s/%d/%.1f retirés
  · les fichiers *_audit.gd, shot*.gd (probes headless) et addons/ (plugins
    tiers) sont EXCLUS : outillage/tierce-partie, pas le chrome du jeu.

SORTIE : CSV « fichier,ligne,chaine,cle_suggeree » (UTF-8) + un compte total
sur stdout. La clé suggérée est T_<FICHIER>_<PREMIERS_MOTS> (translitérée
ASCII) — une SUGGESTION à relire, pas un contrat.
"""
import argparse
import csv
import os
import re
import sys
import unicodedata

# ── contextes d'affichage (là où le joueur lit) ────────────────────────────
CONTEXT_RE = re.compile(
    r"(\.text\s*(=|\+=))|tooltip_text|placeholder_text|\badd_item\s*\(|"
    r"\bset_text\s*\(|window_title|\.title\s*=|\badd_tab\s*\(|"
    r"\b(VKit|V)\.(text|text_map|section|row)\s*\("
)
# lignes d'OUTILLAGE (console/dev, jamais le joueur) — exclues du niveau 2
DEV_RE = re.compile(r"push_error|push_warning|printerr|\bprint\s*\(|\bassert\s*\(|preload\s*\(|\bload\s*\(")
# prose « niveau 2 » : lettre accentuée OU espace + mot de ≥3 lettres
ACCENT_RE = re.compile(r"[À-ÖØ-öø-ÿ]")
SPACE_WORD_RE = re.compile(r"\s")
WORD3_RE = re.compile(r"[A-Za-zÀ-ÿ]{3}")
# littéral "…" (échappements gérés) ; on capture aussi ce qui PRÉCÈDE pour
# détecter un tr( déjà posé.
STRING_RE = re.compile(r'"((?:[^"\\]|\\.)*)"')
FMT_RE = re.compile(r"%[sdif]|%\.\d+f|%0?\d*[sdif]")
IDENT_LOWER_RE = re.compile(r"^[a-z_][a-z0-9_]*$")      # clé de dict / propriété
IDENT_UPPER_RE = re.compile(r"^[A-Z][A-Z0-9_]*$")       # T_X / STR_X / CONSTANTE
CAMEL_RE = re.compile(r"^[A-Z][a-zA-Z0-9]*$")           # NomDeClasse / NodeName


def is_player_facing(lit: str, before: str) -> bool:
    """Juge un littéral extrait d'une ligne-contexte."""
    if not lit:
        return False
    if re.search(r"\btr\s*\($", before.rstrip()):        # déjà traduit : tr("…")
        return False
    if lit.startswith(("res://", "user://", "uid://")):
        return False
    if lit.startswith(".") and " " not in lit:            # ".png", ".cfg"…
        return False
    if IDENT_LOWER_RE.match(lit) or IDENT_UPPER_RE.match(lit) or CAMEL_RE.match(lit):
        return False
    stripped = FMT_RE.sub("", lit)
    if not re.search(r"[A-Za-zÀ-ÿ]", stripped):           # pur format / ponctuation
        return False
    return True


def suggest_key(path: str, lit: str) -> str:
    """T_<FICHIER>_<PREMIERS_MOTS> translitéré ASCII (suggestion, pas contrat)."""
    stem = os.path.splitext(os.path.basename(path))[0]
    stem = re.sub(r"_(panel|root|detail|drawer)$", "", stem)
    txt = unicodedata.normalize("NFKD", lit)
    txt = "".join(c for c in txt if not unicodedata.combining(c))
    words = re.findall(r"[A-Za-z0-9]+", txt)[:4]
    core = "_".join(w.upper() for w in words) or "TXT"
    return ("T_" + stem.upper() + "_" + core)[:48]


def looks_like_prose(lit: str) -> bool:
    """Niveau 2 : accent, ou espace + un mot de ≥3 lettres."""
    if ACCENT_RE.search(lit):
        return True
    return bool(SPACE_WORD_RE.search(lit) and WORD3_RE.search(lit))


def scan_file(path: str):
    out = []
    try:
        with open(path, "r", encoding="utf-8") as f:
            for no, line in enumerate(f, 1):
                s = line.strip()
                if s.startswith("#"):
                    continue
                in_context = bool(CONTEXT_RE.search(line))
                if not in_context and DEV_RE.search(line):
                    continue                      # outillage console : pas le joueur
                for m in STRING_RE.finditer(line):
                    lit = m.group(1)
                    before = line[: m.start()]
                    if not is_player_facing(lit, before):
                        continue
                    if in_context or looks_like_prose(lit):
                        out.append((no, lit))
    except OSError as e:
        print(f"  [warn] illisible : {path} ({e})", file=sys.stderr)
    return out


def excluded(rel: str) -> bool:
    base = os.path.basename(rel)
    if base.endswith("_audit.gd") or base.startswith("shot"):
        return True
    parts = rel.replace("\\", "/").split("/")
    return "addons" in parts


def main() -> int:
    ap = argparse.ArgumentParser(description="backlog i18n du chrome Godot")
    ap.add_argument("--root", default="godot/project")
    ap.add_argument("--out", default="docs/i18n_backlog.csv")
    args = ap.parse_args()

    rows = []
    for dirpath, _dirs, files in os.walk(args.root):
        for fn in sorted(files):
            if not fn.endswith(".gd"):
                continue
            full = os.path.join(dirpath, fn)
            rel = os.path.relpath(full, args.root).replace("\\", "/")
            if excluded(rel):
                continue
            for no, lit in scan_file(full):
                rows.append((rel, no, lit, suggest_key(rel, lit)))

    rows.sort(key=lambda r: (r[0], r[1]))
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    with open(args.out, "w", encoding="utf-8", newline="") as f:
        w = csv.writer(f)
        w.writerow(["fichier", "ligne", "chaine", "cle_suggeree"])
        w.writerows(rows)

    nfiles = len({r[0] for r in rows})
    # sortie console en ASCII pur : la console Windows (cp1252) refuse certains
    # caractères ; le CSV, lui, est UTF-8.
    print("[i18n] %d litteraux face-joueur probables dans %d fichiers -> %s"
          % (len(rows), nfiles, args.out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
