#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
calibrate.py — LE PILOTE (Arc J3).

Lance N chroniques en parallèle sur une grille de constantes (SCPS_TUNE) et
désigne le point qui TIENT les bandes de preuve. Python 3, stdlib SEULEMENT.

Exemple (la chasse au flux d'or) :
  tools/calibrate.py \
    --param ENTRETIEN_DIV:200:600:100 \
    --param MANUF_UPKEEP_DAY:0.03:0.07:0.02 \
    --target flux_or_med:-5:20 --target tresor_med::12000 \
    --target acc540:6:15 \
    --sims 3 --years 150 --seeds 7,23,42 --jobs 4

Mêmes graines pour CHAQUE point (la différence mesurée est celle du paramètre).
Le binaire chronicle est mono-thread → parallélisme par PROCESSUS.
"""
import argparse, csv, itertools, os, subprocess, sys, time, statistics, hashlib
from concurrent.futures import ProcessPoolExecutor, as_completed

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
CHRONICLE = os.path.join(ROOT, "chronicle")
CACHE = os.path.join(HERE, "results.csv")


def frange(lo, hi, step):
    """Plage inclusive, robuste au flottant."""
    if step <= 0:
        return [lo]
    out, v, i = [], lo, 0
    while v <= hi + 1e-9:
        out.append(round(v, 6))
        i += 1
        v = lo + i * step
    return out


def parse_param(s):
    # NAME:min:max:step
    name, lo, hi, step = s.split(":")
    return name, frange(float(lo), float(hi), float(step))


def parse_target(s):
    # metric:lo:hi  (lo ou hi vide = non borné)
    metric, lo, hi = s.split(":")
    return metric, (float(lo) if lo else None), (float(hi) if hi else None)


def tune_string(point):
    """point = dict NAME->val → 'NAME=val,NAME=val' (ordre stable)."""
    return ",".join(f"{k}={v:g}" for k, v in point.items())


def binary_mtime():
    try:
        return str(int(os.path.getmtime(CHRONICLE)))
    except OSError:
        return "0"


def cache_key(tune, seed, sims, years):
    return "|".join([binary_mtime(), tune, str(seed), str(sims), str(years)])


# ---- un job : une chronique (un point × une graine) → la ligne SUMMARY -----
def run_one(args):
    tune, seed, sims, years = args
    csv_path = f"/tmp/scps_cal_{os.getpid()}_{abs(hash((tune, seed)))}.csv"
    try:
        os.remove(csv_path)
    except OSError:
        pass
    env = dict(os.environ)
    if tune:
        env["SCPS_TUNE"] = tune
    env["SCPS_CSV"] = csv_path
    try:
        subprocess.run([CHRONICLE, str(seed), str(sims), str(years)],
                       env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                       check=True)
    except subprocess.CalledProcessError as e:
        return (tune, seed, None, f"chronicle exit {e.returncode}")
    row = None
    try:
        with open(csv_path) as f:
            for r in csv.DictReader(f):
                if r.get("row") == "SUMMARY":
                    row = r
    except OSError:
        pass
    try:
        os.remove(csv_path)
    except OSError:
        pass
    if row is None:
        return (tune, seed, None, "pas de SUMMARY")
    return (tune, seed, {k: row[k] for k in row}, None)


def load_cache():
    c = {}
    if os.path.exists(CACHE):
        with open(CACHE) as f:
            for r in csv.DictReader(f):
                c[r["__key"]] = r
    return c


def save_cache(cache):
    if not cache:
        return
    cols = ["__key"] + [k for k in next(iter(cache.values())) if k != "__key"]
    with open(CACHE, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=cols)
        w.writeheader()
        for r in cache.values():
            w.writerow(r)


def score_point(metrics, targets):
    """Renvoie (nb_bandes_tenues, distance_normalisée_aux_ratées, détail)."""
    held, dist, detail = 0, 0.0, []
    for metric, lo, hi in targets:
        v = metrics.get(metric)
        ok = v is not None
        if ok and lo is not None and v < lo:
            ok = False
        if ok and hi is not None and v > hi:
            ok = False
        if v is not None and (lo is not None or hi is not None):
            span = (abs(hi) if hi is not None else 0) + (abs(lo) if lo is not None else 0) + 1.0
            if lo is not None and v < lo:
                dist += (lo - v) / span
            if hi is not None and v > hi:
                dist += (v - hi) / span
        held += 1 if ok else 0
        detail.append((metric, v, ok))
    return held, dist, detail


def main():
    ap = argparse.ArgumentParser(description="SCPS — balayage de grille de calibrage")
    ap.add_argument("--param", action="append", default=[], help="NAME:min:max:step (1 ou 2)")
    ap.add_argument("--target", action="append", default=[], help="metric:lo:hi (bornes vides = non borné)")
    ap.add_argument("--sims", type=int, default=3)
    ap.add_argument("--years", type=int, default=150)
    ap.add_argument("--seeds", default="7,23,42")
    ap.add_argument("--jobs", type=int, default=0, help="0 = min(coeurs,4)")
    ap.add_argument("--yes", action="store_true", help="pas de confirmation même si long")
    ap.add_argument("--force", action="store_true", help="ignore le cache")
    ap.add_argument("--dump", default=None, help="écrit la grille complète dans ce CSV")
    a = ap.parse_args()

    if not os.path.exists(CHRONICLE):
        sys.exit(f"binaire absent : {CHRONICLE} (fais `make chronicle`)")
    if not a.param:
        sys.exit("au moins un --param requis")

    params = [parse_param(p) for p in a.param]
    targets = [parse_target(t) for t in a.target]
    seeds = [int(s) for s in a.seeds.split(",")]
    jobs = a.jobs if a.jobs > 0 else min(os.cpu_count() or 1, 4)

    # grille = produit cartésien des paramètres
    names = [n for n, _ in params]
    grids = [vals for _, vals in params]
    points = [dict(zip(names, combo)) for combo in itertools.product(*grids)]

    # ---- estimation (un run d'étalonnage 1×5 ans) -------------------------
    t0 = time.time()
    run_one((tune_string(points[0]) if points else "", seeds[0], 1, 5))
    s_per = (time.time() - t0) / 5.0
    tasks_total = len(points) * len(seeds)
    est = tasks_total * a.sims * a.years * s_per / max(jobs, 1)
    print(f"grille : {len(points)} point(s) × {len(seeds)} graine(s) = {tasks_total} runs "
          f"({a.sims}×{a.years} ans) · {jobs} job(s) · ~{est:.0f}s ({est/60:.1f} min) estimé")
    if est > 1800 and not a.yes:
        if input("  > 30 min — continuer ? [o/N] ").strip().lower() not in ("o", "y", "oui"):
            sys.exit("annulé")

    # ---- planification (avec cache) ---------------------------------------
    cache = {} if a.force else load_cache()
    todo, cached = [], {}
    for pt in points:
        tune = tune_string(pt)
        for sd in seeds:
            k = cache_key(tune, sd, a.sims, a.years)
            if k in cache:
                cached[(tune, sd)] = cache[k]
            else:
                todo.append((tune, sd, a.sims, a.years))

    print(f"  {len(cached)} en cache · {len(todo)} à calculer")
    results = {}  # (tune, seed) -> metrics dict
    for (tune, sd), r in cached.items():
        results[(tune, sd)] = r
    if todo:
        with ProcessPoolExecutor(max_workers=jobs) as ex:
            futs = {ex.submit(run_one, t): t for t in todo}
            done = 0
            for fut in as_completed(futs):
                tune, sd, metrics, err = fut.result()
                done += 1
                if err:
                    print(f"  [!] {tune or '(défaut)'} graine {sd}: {err}", file=sys.stderr)
                    continue
                results[(tune, sd)] = metrics
                k = cache_key(tune, sd, a.sims, a.years)
                row = dict(metrics); row["__key"] = k
                cache[k] = row
                if done % max(1, len(todo) // 10) == 0 or done == len(todo):
                    print(f"  … {done}/{len(todo)}", file=sys.stderr)
        save_cache(cache)

    # ---- agrégation par point (médiane sur les graines) -------------------
    numeric = ["flux_or_med", "tresor_med", "ipm_final", "ipm_pic", "acc360", "acc540",
               "acc960", "ratio_poursuite", "batailles", "top_event_share",
               "n_stab", "n_destab", "acharnement", "hegemon_cracked"]
    rows = []
    for pt in points:
        tune = tune_string(pt)
        agg = {}
        for col in numeric:
            vals = []
            for sd in seeds:
                m = results.get((tune, sd))
                if m and m.get(col) not in (None, ""):
                    v = float(m[col])
                    if v != -1.0:           # -1 = « jamais » → exclu de la médiane
                        vals.append(v)
            agg[col] = statistics.median(vals) if vals else -1.0
        held, dist, detail = score_point(agg, targets)
        rows.append((pt, agg, held, dist, detail))

    rows.sort(key=lambda r: (-r[2], r[3]))   # plus de bandes tenues, puis plus proche

    # ---- tableau console ---------------------------------------------------
    print("\n── résultats (triés : bandes tenues ↓, distance ↑) ──")
    metric_cols = [t[0] for t in targets] or ["flux_or_med", "tresor_med", "acc540"]
    head = "  " + " · ".join(names) + "   |  " + "  ".join(f"{m:>14}" for m in metric_cols) + "   bandes"
    print(head)
    for pt, agg, held, dist, detail in rows:
        pv = " · ".join(f"{pt[n]:g}" for n in names)
        mv = "  ".join(f"{agg.get(m, -1):>14.3g}" for m in metric_cols)
        marks = "".join("✓" if ok else "✗" for _, _, ok in detail)
        print(f"  {pv:<16} |  {mv}   {held}/{len(targets)} {marks}")

    # ---- heatmap ASCII pour une grille 2D ---------------------------------
    if len(names) == 2:
        nx, ny = names
        xs = sorted({pt[nx] for pt, *_ in rows})
        ys = sorted({pt[ny] for pt, *_ in rows})
        smap = {(pt[nx], pt[ny]): held for pt, _, held, *_ in rows}
        glyph = " .:-=+*#@"
        mx = max(targets and len(targets) or 1, 1)
        print(f"\n── heatmap du score (lignes {ny} ↓, colonnes {nx} →) ──")
        print("      " + " ".join(f"{x:>5g}" for x in xs))
        for y in ys:
            line = " ".join(f"{glyph[min(len(glyph)-1, int((smap.get((x,y),0)/mx)*(len(glyph)-1)))]:>5}" for x in xs)
            print(f"  {y:>4g} {line}")

    # ---- meilleur point ----------------------------------------------------
    best_pt, best_agg, best_held, best_dist, _ = rows[0]
    print(f"\n★ MEILLEUR POINT : {best_held}/{len(targets)} bandes tenues"
          + (f" (distance {best_dist:.3f})" if best_held < len(targets) else " — TOUTES"))
    print("   SCPS_TUNE=\"" + tune_string(best_pt) + "\"")

    # ---- dump optionnel ----------------------------------------------------
    if a.dump:
        with open(a.dump, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(names + numeric + ["bandes_tenues"])
            for pt, agg, held, _, _ in rows:
                w.writerow([pt[n] for n in names] + [agg.get(c, -1) for c in numeric] + [held])
        print(f"   grille complète → {a.dump}")


if __name__ == "__main__":
    main()
