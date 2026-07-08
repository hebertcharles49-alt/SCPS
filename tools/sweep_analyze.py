#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""sweep_analyze.py — le DEEPDIVE clinique d'un giga sweep chronicle.

Parse sweep_giga/seed_*.log (blocs par-sim « ── Sim N (graine X) — … ── » + ligne
d'archétype « [scps] graine X — archétype « … » » qui PRÉCÈDE chaque bloc), extrait
~30 métriques par sim dans sweep_giga/sims.tsv, puis imprime les DISTRIBUTIONS
(min/p25/méd/moy/p75/max), les coupes PAR ARCHÉTYPE, la table des FINS §27, les
mécanismes à ZÉRO et les anomalies. Stdlib pur (MSYS2 mingw64 python).

Usage : python tools/sweep_analyze.py [dossier=sweep_giga]
"""
import os, re, sys, statistics as st
try:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")   # console Windows cp1252
except Exception:
    pass

DIR = sys.argv[1] if len(sys.argv) > 1 else "sweep_giga"

SIM_HDR = re.compile(r"── Sim (\d+) \(graine (\d+)\) — (\d+) empires? · (\d+) cités?-états? · (\d+) continents? · (\d+) régions? ──")
ARCH    = re.compile(r"\[scps\] graine (\d+) — archétype « ([^»]+) »")
FIN     = re.compile(r"§27 FIN : ([A-ZÉÈÀ' ]+?) \(an (\d+)\)")

def num(s):
    try: return float(s.replace(" ", ""))
    except Exception: return None

# (nom_colonne, regex, groupe(s)) — un extracteur par métrique, tolérant à l'absence.
EX = [
    ("guerres_terr",   re.compile(r"guerres motivées : (\d+) territoriale")),
    ("guerres_eco",    re.compile(r"guerres motivées : \d+ territoriale\(s\) · (\d+) économique")),
    ("guerres_subj",   re.compile(r"(\d+) subjugation")),
    ("guerres_relig",  re.compile(r"subjugation · (\d+) religieuse")),
    ("batailles",      re.compile(r"batailles : (\d+) livrée")),
    ("soulev_allumes", re.compile(r"soulèvements : (\d+) allumés")),
    ("secessions",     re.compile(r"soulèvements : \d+ allumés → (\d+) sécession")),
    ("coups",          re.compile(r"sécession\(s\) · (\d+) coup")),
    ("soulev_ecrases", re.compile(r"(\d+) écrasé\(s\) \(")),
    ("morts_soulev",   re.compile(r"écrasé\(s\) \((\d+) morts au combat\)")),
    ("pillages_n",     re.compile(r"pillage réel : (\d+) pillage")),
    ("pillage_pris",   re.compile(r"pillage réel : \d+ pillage\(s\) · (\d+) or-équiv\. pris")),
    ("pillage_vise",   re.compile(r"pris sur (\d+) visés")),
    ("pillage_pct",    re.compile(r"visés \((\d+) % de la cible")),
    ("ames_deportees", re.compile(r"(\d+) âme\(s\) déportée")),
    ("ames_serviles",  re.compile(r"esclavage : (\d+) âme\(s\) servile")),
    ("affranchis",     re.compile(r"(\d+) affranchissement")),
    ("raids",          re.compile(r"course : (\d+) raid")),
    ("raids_or",       re.compile(r"course : \d+ raid\(s\) \((\d+) or pillés\)")),
    ("coques",         re.compile(r"mer : (\d+) coque\(s\) bâtie")),
    ("routes_mer",     re.compile(r"(\d+) route\(s\) maritime\(s\)")),
    ("hegemon_reg",    re.compile(r"hégémon \(A5\) : 1er empire (\d+) rég")),
    ("stab_plancher",  re.compile(r"Stabilité plancher (\d+)")),
    ("ipm",            re.compile(r"IPM final (\d+\.\d+)")),
    ("arbre_pct",      re.compile(r"arbre : (\d+)% déverrouillé")),
    ("metab_max",      re.compile(r"max (\d+\.?\d*)% → \+")),
    ("refugies_fuites",re.compile(r"réfugiés : (\d+) fuite")),
    ("refugies_retours",re.compile(r"(\d+) retour\(s\)")),
    # ÂMES (volumes 2026-07-08) — absentes des logs pré-volume (extracteur tolérant)
    ("fuites_ames",    re.compile(r"fuite\(s\) de guerre \((\d+) âmes\)")),
    ("retours_ames",   re.compile(r"retour\(s\) \((\d+) âmes\)")),
    ("brassage_ames",  re.compile(r"flux de pacte migratoire \((\d+) âmes")),
    ("recherche_n",    re.compile(r"recherche : (\d+) nœuds déverrouillés")),
    ("dilemmes_w1",    re.compile(r"dilemmes \(lots 1-2\) : (\d+) W1")),
    ("dilemmes_relig", re.compile(r"(\d+) religieux")),
    ("indep",          re.compile(r"→ (\d+) indépendance")),
    ("renversements",  re.compile(r"(\d+) RENVERSEMENT")),
    # ── COMMERCE / ÉCONOMIE (deepdive 2) ──
    ("pool_moy",       re.compile(r"puissance commerciale : pool moy (\d+\.?\d*)/mois")),
    ("volume_marche",  re.compile(r"(\d+) volume tiré du marché")),
    ("asym_aval",      re.compile(r"commerce asym\. : aval (\d+\.?\d*)")),
    ("asym_amont",     re.compile(r"vs amont (\d+\.?\d*)")),
    ("hubs_pct",       re.compile(r"hubs : (\d+)% du commerce mondial")),
    ("prov_colonisees",re.compile(r"expansion : (\d+) prov colonisées")),
    ("prov_transferees",re.compile(r"(\d+) prov TRANSFÉRÉES")),
    ("alliances",      re.compile(r"diplomatie : (\d+) pacte\(s\) d'alliance")),
    ("colon_fondations",re.compile(r"colonisation : (\d+) fondation")),
    ("colon_survie",   re.compile(r"dont (\d+) de survie")),
    ("peage_or",       re.compile(r"péage CUMULÉ (\d+) or")),
    ("entropie",       re.compile(r"faustien : entropie monde (\d+)")),
    ("conso_foreuse",  re.compile(r"conso foreuse (\d+)")),
    ("conso_corne",    re.compile(r"corne (\d+)")),
    ("esclaves_pool",  re.compile(r"(\d+) au pool des Centres")),
    ("affranchis2",    re.compile(r"pool des Centres · (\d+) affranchissement")),
    ("brassage_flux",  re.compile(r"brassage : (\d+) flux de pacte")),
    ("interceptions",  re.compile(r"marine : \d+ marin\(s\) embarqués - (\d+) interception")),
    ("debiteurs",      re.compile(r"dette : (\d+) débiteur")),
]

def parse_log(path):
    sims, arch_by_seed = [], {}
    cur, buf = None, []
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = ARCH.search(line)
            if m: arch_by_seed[m.group(1)] = m.group(2).strip()
            h = SIM_HDR.search(line)
            if h:
                if cur is not None: sims.append((cur, "".join(buf)))
                cur = {"sim": int(h.group(1)), "graine": h.group(2),
                       "empires": int(h.group(3)), "cites": int(h.group(4)),
                       "continents": int(h.group(5)), "regions": int(h.group(6))}
                buf = []
            elif cur is not None:
                buf.append(line)
    if cur is not None: sims.append((cur, "".join(buf)))
    out = []
    for meta, text in sims:
        row = dict(meta)
        row["archetype"] = arch_by_seed.get(meta["graine"], "?")
        f = FIN.search(text)
        row["fin"] = f.group(1).strip() if f else ""
        row["fin_an"] = int(f.group(2)) if f else None
        row["craque"] = 1 if "CRAQUÉ" in text else 0
        for name, rx in EX:
            m = rx.search(text)
            row[name] = num(m.group(1)) if m else None
        # pop monde = Σ « (Nk hab) » de la ligne continents
        pops = re.findall(r"\((\d+)k hab\)", text)
        row["pop_k"] = sum(int(p) for p in pops) if pops else None
        out.append(row)
    return out

def dist(vals):
    v = sorted(x for x in vals if x is not None)
    if not v: return "  (aucune donnée)"
    q = lambda p: v[min(len(v)-1, int(p*len(v)))]
    return f"n={len(v)}  min={v[0]:g}  p25={q(.25):g}  méd={q(.5):g}  moy={st.mean(v):.2f}  p75={q(.75):g}  max={v[-1]:g}"

def main():
    logs = sorted(f for f in os.listdir(DIR) if f.startswith("seed_") and f.endswith(".log"))
    rows, incomplete = [], []
    for lg in logs:
        path = os.path.join(DIR, lg)
        txt = open(path, encoding="utf-8", errors="replace").read()
        if "hégémon MORTEL (A5)" not in txt:
            incomplete.append(lg); continue
        rows.extend([dict(r, log=lg) for r in parse_log(path)])
    cols = ["log","sim","graine","archetype","empires","cites","continents","regions",
            "pop_k","fin","fin_an","craque","hegemon_reg","stab_plancher","ipm","arbre_pct"] \
           + [n for n,_ in EX if n not in ("hegemon_reg","stab_plancher","ipm","arbre_pct")]
    with open(os.path.join(DIR,"sims.tsv"),"w",encoding="utf-8") as f:
        f.write("\t".join(cols)+"\n")
        for r in rows:
            f.write("\t".join("" if r.get(c) is None else str(r.get(c,"")) for c in cols)+"\n")
    print(f"=== GIGA SWEEP — {len(rows)} sims parsées sur {len(logs)} logs "
          f"({'INCOMPLETS: '+', '.join(incomplete) if incomplete else 'tous complets'}) ===\n")
    print("── DISTRIBUTIONS (toutes sims) ──")
    for c in ["pop_k","empires","regions","guerres_terr","guerres_eco","guerres_subj","guerres_relig",
              "batailles","soulev_allumes","secessions","coups","morts_soulev","pillages_n",
              "pillage_pris","pillage_pct","ames_deportees","ames_serviles","raids","raids_or",
              "coques","routes_mer","hegemon_reg","stab_plancher","ipm","arbre_pct","metab_max",
              "refugies_fuites","refugies_retours","fuites_ames","retours_ames","brassage_ames",
              "recherche_n","dilemmes_w1","indep","renversements",
              "pool_moy","volume_marche","asym_aval","asym_amont","hubs_pct","prov_colonisees",
              "prov_transferees","alliances","colon_fondations","colon_survie","peage_or","entropie",
              "conso_foreuse","conso_corne","esclaves_pool","affranchis2","brassage_flux",
              "interceptions","debiteurs"]:
        print(f"  {c:16s} {dist([r.get(c) for r in rows])}")
    print("\n── FINS §27 (type × an) ──")
    fins = {}
    for r in rows:
        k = r["fin"] or "(aucune)"
        fins.setdefault(k, []).append(r.get("fin_an"))
    for k, v in sorted(fins.items(), key=lambda x:-len(x[1])):
        ans = [a for a in v if a]
        rng = f" · ans {min(ans)}–{max(ans)}" if ans else ""
        print(f"  {k:24s} {len(v):3d} sims{rng}")
    print(f"\n── HÉGÉMON : CRAQUÉ {sum(r['craque'] for r in rows)}/{len(rows)} sims ──")
    print("\n── PAR ARCHÉTYPE (n · pop méd · guerres terr méd · craqué % · fins) ──")
    arch = {}
    for r in rows: arch.setdefault(r["archetype"], []).append(r)
    for a, rs in sorted(arch.items(), key=lambda x:-len(x[1])):
        med = lambda c: (st.median([x[c] for x in rs if x.get(c) is not None]) if any(x.get(c) is not None for x in rs) else 0)
        nf = sum(1 for x in rs if x["fin"])
        print(f"  {a:22s} n={len(rs):3d}  pop {med('pop_k'):5.0f}k  guerres {med('guerres_terr'):4.0f}  "
              f"craqué {100*sum(x['craque'] for x in rs)/len(rs):3.0f}%  fins {nf}")
    print("\n── MÉCANISMES À ZÉRO (somme = 0 sur toutes les sims — morts ?) ──")
    for c in [n for n,_ in EX]:
        vals=[r.get(c) for r in rows if r.get(c) is not None]
        if vals and sum(vals)==0: print(f"  ⚠ {c} = 0 partout ({len(vals)} sims)")
    print("\n(sims.tsv écrit — la matrice complète pour les coupes fines)")

if __name__ == "__main__":
    main()
