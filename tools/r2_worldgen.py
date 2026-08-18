# -*- coding: utf-8 -*-
# TEST R² (proposition joueur 2026-08-11) : l'état final d'un monde est-il ÉCRIT dans
# son relief ? Régression OLS de chaque métrique de fin de sim sur les variables de
# worldgen (archétype one-hot + 6 scalaires + tailles). R² haut = déterminisme
# géographique ; R² bas = l'histoire fabrique ses propres issues.
# Pur python (pas de numpy) : X'X (k×k, k≈15) résolu par Gauss avec pivot partiel.
import io, os, re, sys, glob

D = sys.argv[1] if len(sys.argv) > 1 else 'sweep_r2_2026-08-11'

RX_HEAD = re.compile(r'graine (\d+) — archétype « ([^»]+) » : (\d+) continents · terres ([\d.]+) · âge ([\d.]+) · érosion ([\d.]+) · relief ([\d.]+) · temp ([\d.]+) · humidité ([\d.]+)')
RX_HIER = re.compile(r'hiérarchie\.\.\.\s+ok \((\d+) rég\. (\d+) pays')

def grab(pat, txt, cast=float, last=True, default=None):
    m = re.findall(pat, txt)
    if not m: return default
    return cast(m[-1] if last else m[0])

rows = []
for path in sorted(glob.glob(os.path.join(D, 'seed_*.log'))):
    t = io.open(path, encoding='utf-8', errors='replace').read()
    h = RX_HEAD.search(t)
    if not h: continue
    hi = RX_HIER.search(t)
    feat = {
        'arch': h.group(2).strip(),
        'n_cont_cible': float(h.group(3)), 'terres': float(h.group(4)),
        'age_geo': float(h.group(5)), 'erosion': float(h.group(6)),
        'relief': float(h.group(7)), 'temp': float(h.group(8)), 'humid': float(h.group(9)),
        'n_reg': float(hi.group(1)) if hi else 0.0,
        'n_pays': float(hi.group(2)) if hi else 0.0,
    }
    # ── les issues (dernière occurrence = fin de sim) ──
    out = {
        'pop_finale_k':    grab(r'population : (\d+)k', t),
        'guerres':         grab(r'guerres déclenchées \(total\) \.+ (\d+)', t),
        'identites':       grab(r'identités culturelles : (\d+) nommée', t),
        'gen_max':         grab(r'gén max (\d+)', t),
        'bascules_ethos':  grab(r'(\d+) bascule\(s\) d\'éthos ENDOGÈNE', t),
        'cristallisations':grab(r'(\d+) cristallisation\(s\) culturelle\(s\) par contact', t),
        'jumeaux_pct':     None,
        'tech_pct':        grab(r'arbre déverrouillé / empire \. (\d+)%', t),
        'IPM':             grab(r'IPM final moyen ([\d.]+)', t),
        'esclaves':        grab(r'(\d+) âme\(s\) servile', t),
        'affranchis':      grab(r'(\d+) affranchissement', t),
        'substrats':       grab(r'(\d+) substrat', t),
        'coups':           grab(r'(\d+) coup\(s\)', t),
        'secessions':      grab(r'(\d+) sécession\(s\)', t),
        'batailles':       grab(r'(\d+) livrées', t),
        'rt_elite':        grab(r'Élite ([\d.]+)\s*$', t) or grab(r'· Élite ([\d.]+)', t),
    }
    mj = re.findall(r'(\d+) jumeaux convergents', t)
    mp = re.findall(r'langue \(couronnes, (\d+) paires', t)
    if mj and mp and int(mp[-1]) > 0:
        out['jumeaux_pct'] = 100.0*int(mj[-1])/int(mp[-1])
    rows.append((feat, out))

print("mondes parsés : %d" % len(rows))
if len(rows) < 30:
    print("trop peu de mondes pour une régression honnête — relancer plus tard"); sys.exit(0)

archs = sorted({f['arch'] for f, _ in rows})
scal = ['n_cont_cible','terres','age_geo','erosion','relief','temp','humid','n_reg','n_pays']
def xrow(f):
    return [1.0] + [1.0 if f['arch']==a else 0.0 for a in archs[1:]] + [f[s] for s in scal]
K = 1 + (len(archs)-1) + len(scal)

def solve(A, b):
    n = len(A)
    M = [A[i][:] + [b[i]] for i in range(n)]
    for c in range(n):
        p = max(range(c, n), key=lambda r: abs(M[r][c]))
        if abs(M[p][c]) < 1e-9: return None      # colinéaire → pas de solution stable
        M[c], M[p] = M[p], M[c]
        d = M[c][c]
        M[c] = [v/d for v in M[c]]
        for r in range(n):
            if r != c and M[r][c] != 0.0:
                fct = M[r][c]
                M[r] = [M[r][j] - fct*M[c][j] for j in range(n+1)]
    return [M[i][n] for i in range(n)]

metrics = [k for k in rows[0][1].keys()]
print("\n%-18s %8s %6s   (features : intercept + %d archétypes + %s)" % ("MÉTRIQUE","R²","n", len(archs)-1, ",".join(scal)))
print("─"*76)
res = []
for m in metrics:
    data = [(xrow(f), o[m]) for f, o in rows if o.get(m) is not None]
    n = len(data)
    if n < 30: continue
    X = [d[0] for d in data]; y = [float(d[1]) for d in data]
    ybar = sum(y)/n
    sst = sum((v-ybar)**2 for v in y)
    if sst < 1e-9: continue
    XtX = [[sum(X[i][a]*X[i][b] for i in range(n)) for b in range(K)] for a in range(K)]
    for d0 in range(K): XtX[d0][d0] += 1e-6      # ridge epsilon (stabilité, quasi-OLS)
    Xty = [sum(X[i][a]*y[i] for i in range(n)) for a in range(K)]
    beta = solve(XtX, Xty)
    if beta is None: continue
    sse = sum((y[i] - sum(beta[a]*X[i][a] for a in range(K)))**2 for i in range(n))
    r2 = 1.0 - sse/sst
    res.append((r2, m, n))
for r2, m, n in sorted(res, reverse=True):
    verdict = "GÉOGRAPHIE" if r2 > 0.5 else ("mixte" if r2 > 0.25 else "HISTOIRE")
    print("%-18s %8.3f %6d   %s" % (m, r2, n, verdict))
print("\nLecture : R² = part de la variance de la métrique expliquée par le worldgen seul")
print("(archétype + terres/âge/érosion/relief/temp/humidité + tailles). Haut = l'issue")
print("est écrite dans le relief ; bas = le monde fabrique son histoire.")
