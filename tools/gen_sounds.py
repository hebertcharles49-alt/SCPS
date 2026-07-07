#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
gen_sounds.py — le générateur SON DE SCPS (synthèse procédurale, hors-ligne, déterministe).

═══════════════════════════════════════════════════════════════════════════════
NOTES DE RECETTE — « RENDRE LES SONS ORGANIQUES » (refonte 2026-07, à verser à
TROUVAILLES.md). L'ancienne banque sonnait TROP PROCÉDURALE (bips/souffles de
synthé). Les 7 techniques appliquées ici, son par son, et POURQUOI elles marchent :

1. VARIANTES + JITTER AU JEU (le plus gros gain perçu). Les sons FRÉQUENTS
   (ui_tick, ui_quill, ui_seal, ui_coin, ui_scroll_tick, ui_parchment_open/close)
   sont générés en 3-4 VARIANTES (fichiers `nom_1/2/3.wav`), chacune avec une graine
   distincte → spectre/RMS RÉELLEMENT différents (pas 3 copies). Au JEU, sound.gd
   tire une variante au hasard + applique un pitch (±4 %) et un gain (±1.5 dB)
   aléatoires À CHAQUE lecture. Un son joué 100×/session cesse de sonner « machine ».

2. EXCITATION RÉALISTE (transitoire). Un clic unique excitant un résonateur = « bip ».
   On excite avec une MICRO-RAFALE : plusieurs micro-impacts (5-30 ms) — un tampon de
   cire, une plume, un tock de bois sont des DIZAINES de micro-contacts. `micro_burst`.

3. SYNTHÈSE MODALE (banc de résonateurs). FM/KS purs sonnent verre/plastique. Un banc
   de 4-8 biquads passe-bande haut-Q aux fréquences INHARMONIQUES d'un vrai corps
   (bois ~1/2.76/5.4/8.9 ; métal/cloche : partiels étirés désaccordés), chaque mode sa
   décroissance propre (aigus s'éteignent vite), excité par la transitoire (2). `modal_bank`.

4. SYNTHÈSE GRANULAIRE (textures). Un bruit filtré = « souffle de synthé ». Les textures
   (parchemin, vent, mer, foule, ronces) sont reconstruites de CENTAINES de micro-grains
   (5-40 ms) aux positions/hauteurs/amplitudes aléatoires. Parchemin = micro-craquements
   de fibres ; foule = grains de formants vocaux désaccordés indistincts. `granular_texture`.

5. MODULATION 1/f (pas de LFO sinus). Les LFO propres sonnent électroniques. On module
   hauteur/filtre/amplitude par une MARCHE ALÉATOIRE filtrée (bruit rose lissé) = le
   micro-tremblement du vivant. Drift ±0.5 % sur les tenues (drones, cloches). `rand_walk`.

6. AIR & ESPACE (44.1 kHz + réverb dense). Tout en 44.1 kHz (l'air HF fait l'organique ;
   fini le 16 kHz « téléphone »). Ambiances stéréo DÉCORRÉLÉES (grains G≠D → largeur).
   Réverb = FDN 8 lignes (queue dense) + IR passe-bas croissant (l'air absorbe les aigus)
   → « pose » le son dans un lieu. `fdn_reverb`.

7. SATURATION DOUCE (tanh) sur les corps → chaleur (harmoniques paires), sans clipper.

BUDGET : 44.1 k + variantes gonflent. Cible ≤ ~10 Mo. Ambiances = 44.1 kHz stéréo
mais RACCOURCIES (~7-8 s, boucle invisible) pour tenir le budget ; downsample retiré.
Total mesuré : voir la ligne « Total » de --check.

MÉTRIQUES (les oreilles = script, cf. check_all) : richesse spectrale (bandes FFT
étalées), non-répétition (autocorrélation d'ambiance sans pic de période fort), densité
d'onsets (rustles), variantes réellement distinctes (spectre/RMS), couture de boucle
sans clic, peaks/RMS sains (UI ~-16 dBFS, amb ~-22, 0 clip, 0 silence).
═══════════════════════════════════════════════════════════════════════════════

MODE D'EMPLOI (régénération) :
    /d/MSYS2/mingw64/bin/python3 tools/gen_sounds.py
    (ou : python3 tools/gen_sounds.py --out godot/project/audio --seed 20260706)

    Écrit les WAV (16-bit PCM, 44.1 kHz) dans godot/project/audio/. Aucune dépendance
    externe REQUISE (numpy détecté et utilisé s'il est présent — sinon pur stdlib
    array/math/wave/random ; cet environnement MSYS2 n'a PAS numpy). Determinism : seed
    PAR NOM → deux runs produisent des fichiers BYTE-IDENTIQUES. Ce script ne touche
    AUCUN fichier C ni logique de simulation — horloge murale pure.

    Ajouter un son : écrire `def snd_xxx(seed) -> Wave`, l'enregistrer dans RECIPES.
    Ajouter des variantes : lister le nom dans VARIANTS (n variantes) — le pipeline
    génère nom_1.wav … nom_n.wav avec des graines décalées.
"""

import argparse
import array
import io
import math
import os
import random
import sys
import wave

# numpy accélère FORTEMENT (biquads/convolution) mais n'est pas requis. On le détecte.
try:
    import numpy as _np
    HAVE_NUMPY = True
except Exception:
    _np = None
    HAVE_NUMPY = False

# console Windows en cp1252 par défaut : force UTF-8 en sortie pour éviter les
# UnicodeEncodeError sur les accents/⚠ (n'affecte que l'affichage, pas les WAV).
if hasattr(sys.stdout, "buffer"):
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")

SR = 44100


# ───────────────────────── conteneur audio minimal ─────────────────────────

class Wave:
    """Buffer audio en float, mono ou stéréo (liste de canaux — 1 ou 2)."""

    def __init__(self, channels):
        self.ch = channels

    @staticmethod
    def mono(samples):
        return Wave([list(samples)])

    @staticmethod
    def stereo(left, right):
        return Wave([list(left), list(right)])

    @property
    def n(self):
        return len(self.ch[0])

    @property
    def nchan(self):
        return len(self.ch)

    def gain(self, g):
        for c in self.ch:
            for i in range(len(c)):
                c[i] *= g
        return self

    def peak(self):
        m = 0.0
        for c in self.ch:
            for v in c:
                av = abs(v)
                if av > m:
                    m = av
        return m

    def rms(self):
        s = 0.0
        n = 0
        for c in self.ch:
            for v in c:
                s += v * v
                n += 1
        return math.sqrt(s / max(1, n))

    def normalize_to(self, peak_dbfs):
        pk = self.peak()
        if pk < 1e-9:
            return self
        target = db_to_lin(peak_dbfs)
        self.gain(target / pk)
        return self


def db_to_lin(db):
    return 10.0 ** (db / 20.0)


def lin_to_db(lin):
    if lin < 1e-12:
        return -240.0
    return 20.0 * math.log10(lin)


# ───────────────────────── primitives de synthèse ─────────────────────────

def n_samples(seconds):
    return int(round(seconds * SR))


def silence(seconds):
    return [0.0] * n_samples(seconds)


def white_noise(seconds, rng):
    n = n_samples(seconds)
    return [rng.uniform(-1.0, 1.0) for _ in range(n)]


def pink_noise(seconds, rng):
    """Bruit rose via le filtre de Paul Kellet (approximation classique, 3 pôles)."""
    n = n_samples(seconds)
    b0 = b1 = b2 = 0.0
    out = [0.0] * n
    for i in range(n):
        w = rng.uniform(-1.0, 1.0)
        b0 = 0.99765 * b0 + w * 0.0990460
        b1 = 0.96300 * b1 + w * 0.2965164
        b2 = 0.57000 * b2 + w * 1.0526913
        out[i] = (b0 + b1 + b2 + w * 0.1848) * 0.2
    return out


def onepole_lowpass(x, cutoff_hz, sr=SR):
    """Passe-bas 1 pôle (RC simple) — cutoff CONSTANT ou callable(i)->hz."""
    n = len(x)
    out = [0.0] * n
    y = 0.0
    cst = not callable(cutoff_hz)
    if cst:
        fc = max(1.0, min(cutoff_hz, sr * 0.49))
        a = math.exp(-2.0 * math.pi * fc / sr)
        oma = 1.0 - a
        for i in range(n):
            y = oma * x[i] + a * y
            out[i] = y
        return out
    for i in range(n):
        fc = max(1.0, min(cutoff_hz(i), sr * 0.49))
        a = math.exp(-2.0 * math.pi * fc / sr)
        y = (1.0 - a) * x[i] + a * y
        out[i] = y
    return out


def onepole_highpass(x, cutoff_hz, sr=SR):
    lp = onepole_lowpass(x, cutoff_hz, sr)
    return [x[i] - lp[i] for i in range(len(x))]


def biquad_bandpass(x, freq, q, sr=SR):
    """Biquad passe-bande RBJ (constant peak gain)."""
    w0 = 2.0 * math.pi * freq / sr
    alpha = math.sin(w0) / (2.0 * q)
    cosw0 = math.cos(w0)
    b0 = alpha
    b1 = 0.0
    b2 = -alpha
    a0 = 1.0 + alpha
    a1 = -2.0 * cosw0
    a2 = 1.0 - alpha
    b0, b1, b2 = b0 / a0, b1 / a0, b2 / a0
    a1, a2 = a1 / a0, a2 / a0
    if HAVE_NUMPY:
        return _biquad_np(x, b0, b1, b2, a1, a2)
    x1 = x2 = y1 = y2 = 0.0
    out = [0.0] * len(x)
    for i, xi in enumerate(x):
        y = b0 * xi + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
        out[i] = y
        x2, x1 = x1, xi
        y2, y1 = y1, y
    return out


def _biquad_np(x, b0, b1, b2, a1, a2):
    """Direct-Form II transposé — la récursion reste séquentielle mais l'arithmétique
    par échantillon passe par des scalaires Python natifs (numpy ne vectorise pas un
    IIR récursif ; on garde la boucle mais depuis un array pour éviter le coût liste)."""
    arr = _np.asarray(x, dtype=_np.float64)
    out = _np.empty_like(arr)
    y1 = y2 = x1 = x2 = 0.0
    b0f, b1f, b2f, a1f, a2f = float(b0), float(b1), float(b2), float(a1), float(a2)
    xl = arr.tolist()
    ol = [0.0] * len(xl)
    for i in range(len(xl)):
        xi = xl[i]
        y = b0f * xi + b1f * x1 + b2f * x2 - a1f * y1 - a2f * y2
        ol[i] = y
        x2 = x1
        x1 = xi
        y2 = y1
        y1 = y
    return ol


def rand_walk(n, rng, smooth_hz=6.0, sr=SR):
    """Marche aléatoire LISSÉE = bruit ~1/f borné [-1,1] — le remplaçant organique du
    LFO sinus (technique 5). Sert à moduler hauteur/filtre/amplitude sans régularité
    électronique."""
    raw = [rng.uniform(-1.0, 1.0) for _ in range(n)]
    w = onepole_lowpass(raw, smooth_hz, sr)
    w = onepole_lowpass(w, smooth_hz, sr)  # 2 pôles → plus 1/f, moins de haut
    mx = max(1e-6, max(abs(v) for v in w))
    return [v / mx for v in w]


def micro_burst(seconds, rng, n_impacts, lp_hz=4000.0, hp_hz=200.0, jitter_amp=(0.3, 1.0)):
    """EXCITATION RÉALISTE (technique 2) : une rafale de N micro-impacts (chacun un pic
    de bruit filtré très bref) au lieu d'un clic unique. Le « matériau » (cire, bois,
    plume) est fait de dizaines de micro-contacts."""
    n = n_samples(seconds)
    sig = [0.0] * n
    for _ in range(n_impacts):
        c = rng.uniform(0.0, 1.0) * n
        w = rng.uniform(0.0006, 0.004) * SR   # 0.6-4 ms par impact
        amp = rng.uniform(*jitter_amp)
        lo = max(0, int(c - w * 3))
        hi = min(n, int(c + w * 3))
        for i in range(lo, hi):
            d = (i - c) / max(1.0, w)
            sig[i] += amp * math.exp(-0.5 * d * d) * rng.uniform(-1.0, 1.0)
    sig = onepole_lowpass(sig, lp_hz)
    sig = onepole_highpass(sig, hp_hz)
    return sig


def modal_bank(exciter, modes, sr=SR):
    """SYNTHÈSE MODALE (technique 3) : banc de résonateurs biquad passe-bande haut-Q.
    `modes` = liste de (freq_hz, q, gain, decay_s). Chaque mode = un biquad (résonance)
    × une enveloppe exponentielle propre (les aigus s'éteignent vite). Excité par la
    transitoire `exciter`. C'est ce qui donne le grain « corps réel » (bois/métal)."""
    n = len(exciter)
    out = [0.0] * n
    for (freq, q, gain, decay) in modes:
        if freq >= sr * 0.49:
            continue
        res = biquad_bandpass(exciter, freq, q, sr)
        # enveloppe de décroissance propre au mode (aigus plus courts)
        k = 1.0 / max(1e-4, decay * sr)
        env_a = math.exp(-k)  # décroissance par échantillon
        e = 1.0
        for i in range(n):
            out[i] += res[i] * gain * e
            e *= env_a
    return out


def granular_texture(seconds, rng, grain_ms=(8.0, 30.0), density=180.0,
                     freq_range=(400.0, 3000.0), q=4.0, grain_kind="bp"):
    """SYNTHÈSE GRANULAIRE (technique 4) : reconstruit une texture de CENTAINES de
    micro-grains aux positions/hauteurs/amplitudes aléatoires. `density` = grains/s.
    grain_kind='bp' (bruit passe-bande → froissement/vent), 'ks' (Karplus court →
    craquement de fibre), 'formant' (2-3 bandes vocales → foule). Jamais répétitif."""
    n = n_samples(seconds)
    sig = [0.0] * n
    n_grains = int(density * seconds)
    for _ in range(n_grains):
        gdur = rng.uniform(*grain_ms) / 1000.0
        gn = max(4, n_samples(gdur))
        pos = int(rng.uniform(0.0, 1.0) * n)
        amp = rng.uniform(0.2, 1.0)
        f = math.exp(rng.uniform(math.log(freq_range[0]), math.log(freq_range[1])))
        if grain_kind == "ks":
            base = _ks_short(gn, f, rng)
        elif grain_kind == "formant":
            base = white_noise(gdur, rng)
            v = [0.0] * gn
            for fj in (f, f * rng.uniform(1.8, 2.4), f * rng.uniform(2.9, 3.6)):
                bp = biquad_bandpass(base, min(fj, SR * 0.45), q)
                for i in range(gn):
                    v[i] += bp[i]
            base = v
        else:  # bp
            base = biquad_bandpass(white_noise(gdur, rng), min(f, SR * 0.45), q)
        # fenêtre de Hann (couture douce, pas de clic de grain)
        for i in range(gn):
            wv = 0.5 - 0.5 * math.cos(2.0 * math.pi * i / max(1, gn - 1))
            j = pos + i
            if 0 <= j < n:
                sig[j] += base[i] * wv * amp
    return sig


def _ks_short(n, freq, rng, damping=0.965, brightness=0.2, sr=SR):
    period = max(2, int(round(sr / freq)))
    buf = [rng.uniform(-1.0, 1.0) for _ in range(period)]
    out = [0.0] * n
    idx = 0
    prev = buf[0]
    for i in range(n):
        cur = buf[idx]
        avg = (cur + prev) * 0.5
        buf[idx] = damping * (brightness * avg + (1.0 - brightness) * cur)
        out[i] = cur
        prev = cur
        idx = (idx + 1) % period
    return out


def karplus_strong(seconds, freq, rng, damping=0.996, brightness=0.5, sr=SR):
    n = n_samples(seconds)
    return _ks_short(n, freq, rng, damping, brightness, sr)


def adsr(n, sr, a, d, s_level, s_dur, r, curve=1.6):
    """Enveloppe ADSR souple (courbes exponentielles)."""
    out = [0.0] * n
    na = n_samples(a)
    nd = n_samples(d)
    ns = n_samples(s_dur)
    nr = n_samples(r)
    i = 0
    for k in range(min(na, n - i)):
        out[i + k] = (k / max(1, na)) ** (1.0 / curve)
    i += na
    for k in range(min(nd, max(0, n - i))):
        t = k / max(1, nd)
        out[i + k] = 1.0 - (1.0 - s_level) * (t ** curve)
    i += nd
    for k in range(min(ns, max(0, n - i))):
        out[i + k] = s_level
    i += ns
    remaining = n - i
    nr_eff = min(nr, remaining) if remaining > 0 else 0
    for k in range(nr_eff):
        t = k / max(1, nr_eff)
        out[i + k] = s_level * ((1.0 - t) ** curve)
    return out


def apply_env(x, env):
    return [x[i] * env[i] for i in range(min(len(x), len(env)))]


def mix(*bufs):
    n = max(len(b) for b in bufs)
    out = [0.0] * n
    for b in bufs:
        for i, v in enumerate(b):
            out[i] += v
    return out


def pad_to(x, n):
    if len(x) >= n:
        return x[:n]
    return x + [0.0] * (n - len(x))


def soft_saturate(x, drive=1.0):
    """Saturation douce (tanh) — évite le clip dur, ajoute du corps/de la chaleur."""
    d = max(0.001, drive)
    td = math.tanh(d)
    return [math.tanh(v * d) / td for v in x]


# ───────────────────────── réverb FDN (technique 6) ─────────────────────────

# 8 lignes de retard mutuellement premières (ms) — queue dense, pas de « flutter »
_FDN_MS = [23.1, 29.7, 37.1, 41.1, 43.7, 47.3, 53.9, 59.1]


def fdn_reverb(x, sr=SR, decay=0.82, damp_hz=4200.0, wet=0.3, predelay_ms=10.0):
    """Feedback Delay Network 8 lignes + matrice de Hadamard (mélange orthogonal) +
    amortissement passe-bas par ligne (l'air absorbe les aigus → queue naturelle).
    Bien plus dense/organique que la Schroeder 4-combs (technique 6)."""
    pre = n_samples(predelay_ms / 1000.0)
    xin = [0.0] * pre + list(x)
    N = len(xin)
    delays = [max(1, int(round(sr * ms / 1000.0))) for ms in _FDN_MS]
    K = len(delays)
    bufs = [[0.0] * d for d in delays]
    idx = [0] * K
    lp = [0.0] * K
    # coef d'amortissement passe-bas (par ligne)
    a_damp = math.exp(-2.0 * math.pi * min(damp_hz, sr * 0.49) / sr)
    # gain de feedback par ligne (décroissance ~= decay ; les longues lignes moins)
    g = [decay ** (delays[i] / float(delays[K // 2])) for i in range(K)]
    inv_sqrt = 1.0 / math.sqrt(K)
    out = [0.0] * N
    # Hadamard 8×8 (signes) — mélange orthogonal bon marché
    had = _hadamard8()
    for t in range(N):
        # lecture des lignes
        d = [bufs[k][idx[k]] for k in range(K)]
        # amortissement passe-bas de chaque ligne
        for k in range(K):
            lp[k] = (1.0 - a_damp) * d[k] + a_damp * lp[k]
            d[k] = lp[k]
        # sortie = somme des lignes
        s = 0.0
        for k in range(K):
            s += d[k]
        out[t] = s * inv_sqrt
        # mélange Hadamard + injection de l'entrée + feedback
        xi = xin[t]
        for k in range(K):
            acc = 0.0
            row = had[k]
            for j in range(K):
                acc += row[j] * d[j]
            v = xi + acc * inv_sqrt * g[k]
            bufs[k][idx[k]] = v
            idx[k] = (idx[k] + 1) % delays[k]
    dry = pad_to(list(x), N)
    return [dry[i] * (1.0 - wet) + out[i] * wet for i in range(N)]


_HAD8 = None


def _hadamard8():
    global _HAD8
    if _HAD8 is not None:
        return _HAD8
    h1 = [[1]]

    def kron(a):
        n = len(a)
        b = [[0] * (2 * n) for _ in range(2 * n)]
        for i in range(n):
            for j in range(n):
                b[i][j] = a[i][j]
                b[i][j + n] = a[i][j]
                b[i + n][j] = a[i][j]
                b[i + n][j + n] = -a[i][j]
        return b
    h = h1
    for _ in range(3):  # 2^3 = 8
        h = kron(h)
    _HAD8 = h
    return h


# ───────────────────────── boucle invisible ─────────────────────────

def crossfade_loop(chan, fade_seconds, declick_ms=6.0):
    """Boucle INVISIBLE : la queue est mélangée dans la tête (equal-power crossfade) puis
    un déclic final force la toute fin à converger vers l'échantillon de tête."""
    nfade = n_samples(fade_seconds)
    n = len(chan)
    if nfade * 2 >= n:
        nfade = max(1, n // 4)
    head = chan[:nfade]
    tail = chan[n - nfade:]
    body = chan[nfade:n - nfade]
    out_head = [0.0] * nfade
    for i in range(nfade):
        t = i / max(1, nfade - 1)
        fout = math.cos(t * math.pi * 0.5)
        fin = math.sin(t * math.pi * 0.5)
        out_head[i] = tail[i] * fout + head[i] * fin
    looped = out_head + body
    ndc = min(n_samples(declick_ms / 1000.0), len(looped) - 1)
    if ndc > 1:
        target = looped[0]
        start_val = looped[-1]
        delta = target - start_val
        base = len(looped) - ndc
        for i in range(ndc):
            t = (i + 1) / ndc
            looped[base + i] += delta * t
    return looped


def loop_seam_score(chan):
    if len(chan) < 2:
        return 0.0
    last = chan[-1]
    first = chan[0]
    amp = max(1e-6, max(abs(v) for v in chan[-256:] + chan[:256]))
    return abs(last - first) / amp


# ───────────────────────── I/O WAV ─────────────────────────

def write_wav(path, wv: Wave, sr=SR):
    nchan = wv.nchan
    n = wv.n
    data = array.array('h', [0] * (n * nchan))
    for ci, c in enumerate(wv.ch):
        for i in range(n):
            v = max(-1.0, min(1.0, c[i]))
            data[i * nchan + ci] = int(round(v * 32767.0))
    with wave.open(path, 'wb') as f:
        f.setnchannels(nchan)
        f.setsampwidth(2)
        f.setframerate(sr)
        f.writeframes(data.tobytes())


def read_wav_peak_rms(path):
    with wave.open(path, 'rb') as f:
        nchan = f.getnchannels()
        n = f.getnframes()
        fr = f.getframerate()
        raw = f.readframes(n)
    a = array.array('h')
    a.frombytes(raw)
    floats = [v / 32768.0 for v in a]
    pk = max(abs(v) for v in floats) if floats else 0.0
    rms = math.sqrt(sum(v * v for v in floats) / max(1, len(floats))) if floats else 0.0
    dur = n / float(fr)
    return dur, pk, rms, nchan


# ───────────────────────── recettes UI ─────────────────────────

def snd_ui_tick(seed):
    """LE TICK — un « tock » de bois mat par SYNTHÈSE MODALE : micro-rafale de bois
    (3-5 micro-contacts) excitant un banc de modes inharmoniques de bois (ratios
    ~1/2.76/5.4), aigus courts. Discret, écoutable des heures."""
    rng = random.Random(seed)
    dur = 0.12
    n = n_samples(dur)
    exc = micro_burst(0.008, rng, n_impacts=rng.randint(3, 5), lp_hz=3200.0, hp_hz=120.0)
    exc = pad_to(exc, n)
    f0 = rng.uniform(150.0, 170.0)
    modes = [
        (f0,         26, 1.0,  0.045),
        (f0 * 2.76,  30, 0.42, 0.030),
        (f0 * 5.40,  34, 0.20, 0.018),
        (f0 * 8.93,  38, 0.10, 0.010),
    ]
    body = modal_bank(exc, modes)
    body = onepole_lowpass(body, 3200.0)
    env = adsr(n, SR, a=0.001, d=0.05, s_level=0.0, s_dur=0.0, r=0.05, curve=1.2)
    body = apply_env(body, env)
    sig = soft_saturate(body, 1.15)
    return Wave.mono(pad_to(sig, n)).normalize_to(-16.0)


def snd_ui_tick_year(seed):
    """Variante an-nouveau : le tock modal + une cloche LOINTAINE (banc modal de métal,
    partiels étirés, decay long mais discret, queue FDN)."""
    rng = random.Random(seed)
    tick = snd_ui_tick(seed).ch[0]
    dur = 1.0
    n = n_samples(dur)
    exc = micro_burst(0.004, rng, n_impacts=2, lp_hz=6000.0, hp_hz=400.0)
    exc = pad_to(exc, n)
    f0 = rng.uniform(640.0, 680.0)
    # partiels de cloche : étirés (inharmonicité métallique) et désaccordés
    modes = [
        (f0 * 1.00,  60, 1.0,  0.55),
        (f0 * 2.01,  70, 0.55, 0.40),
        (f0 * 2.79,  80, 0.42, 0.30),
        (f0 * 4.15,  90, 0.26, 0.20),
        (f0 * 5.43, 100, 0.16, 0.13),
    ]
    bell = modal_bank(exc, modes)
    bell = onepole_lowpass(bell, 5000.0)
    sig = mix(pad_to(tick, n), [b * 0.16 for b in bell])
    sig = fdn_reverb(sig, wet=0.18, decay=0.8)
    return Wave.mono(pad_to(sig, n)).normalize_to(-16.0)


def _parchment_rustle(seed, dur, rising):
    """Froissement de papier par SYNTHÈSE GRANULAIRE : des centaines de micro-craquements
    de fibres (grains KS courts + grains passe-bande) modulés par une enveloppe
    irrégulière (technique 4)."""
    rng = random.Random(seed)
    n = n_samples(dur)
    fibers = granular_texture(dur, rng, grain_ms=(3.0, 14.0), density=520.0,
                              freq_range=(1800.0, 7000.0), q=3.0, grain_kind="ks")
    clicks = granular_texture(dur, rng, grain_ms=(2.0, 8.0), density=260.0,
                              freq_range=(2600.0, 9000.0), q=5.0, grain_kind="bp")
    sig = mix([f * 0.7 for f in fibers], [c * 0.5 for c in clicks])
    sig = onepole_highpass(sig, 1400.0)
    # enveloppe de geste : le froissement gonfle puis retombe, modulé 1/f
    overall = adsr(n, SR, a=0.02 if rising else 0.005, d=0.05, s_level=0.75,
                   s_dur=dur * 0.4, r=dur * 0.35, curve=1.3)
    walk = rand_walk(n, rng, smooth_hz=18.0)
    env = [max(0.0, overall[i] * (0.6 + 0.4 * (0.5 + 0.5 * walk[i]))) for i in range(n)]
    sig = apply_env(pad_to(sig, n), env)
    sig = fdn_reverb(sig, wet=0.08, decay=0.55, damp_hz=6000.0)
    return Wave.mono(pad_to(sig, n)).normalize_to(-17.0)


def snd_ui_parchment_open(seed):
    return _parchment_rustle(seed, dur=0.55, rising=True)


def snd_ui_parchment_close(seed):
    return _parchment_rustle(seed, dur=0.45, rising=False)


def snd_ui_quill(seed):
    """Gratte de plume : 3 gratouillis granulaires (grains fins bruités modulés par une
    enveloppe GRANULEUSE, pas lisse — le crissement de la pointe)."""
    rng = random.Random(seed)
    parts = []
    for k in range(3):
        dur = rng.uniform(0.09, 0.16)
        n = n_samples(dur)
        scratch = granular_texture(dur, rng, grain_ms=(1.5, 6.0), density=700.0,
                                   freq_range=(3200.0 + k * 400, 9500.0), q=6.0,
                                   grain_kind="bp")
        # traînée modulée en 1/f (grain irrégulier de la pointe qui accroche)
        walk = rand_walk(n, rng, smooth_hz=45.0)
        env = adsr(n, SR, a=0.006, d=0.03, s_level=0.6, s_dur=dur * 0.4, r=dur * 0.3, curve=1.2)
        env = [max(0.0, env[i] * (0.35 + 0.65 * (0.5 + 0.5 * walk[i]))) for i in range(n)]
        scratch = apply_env(scratch, env)
        parts.append(scratch)
        parts.append(silence(0.05))
    sig = []
    for p in parts:
        sig += p
    return Wave.mono(sig).normalize_to(-18.0)


def snd_ui_seal(seed):
    """Cachet de cire : impact sourd MODAL (micro-rafale d'appui + banc de modes graves
    de bois) puis SQUISH granulaire (la cire qui s'étale, grains bas modulés)."""
    rng = random.Random(seed)
    dur = 0.42
    n = n_samples(dur)
    # l'appui : micro-rafale de plusieurs contacts (le tampon presse la cire)
    exc = micro_burst(0.02, rng, n_impacts=rng.randint(5, 9), lp_hz=1400.0, hp_hz=50.0)
    exc = pad_to(exc, n)
    f0 = rng.uniform(78.0, 96.0)
    modes = [
        (f0,        14, 1.0,  0.06),
        (f0 * 2.4,  18, 0.5,  0.04),
        (f0 * 4.1,  22, 0.22, 0.025),
    ]
    thump = modal_bank(exc, modes)
    tenv = adsr(n, SR, a=0.001, d=0.06, s_level=0.0, s_dur=0.0, r=0.06, curve=1.1)
    thump = apply_env(thump, tenv)
    # le squish : texture granulaire basse qui s'étale
    squish = granular_texture(dur, rng, grain_ms=(6.0, 22.0), density=200.0,
                              freq_range=(140.0, 900.0), q=2.2, grain_kind="bp")
    senv = adsr(n, SR, a=0.03, d=0.1, s_level=0.25, s_dur=0.12, r=0.18, curve=1.8)
    squish = apply_env(pad_to(squish, n), senv)
    sig = mix(thump, [s * 0.4 for s in squish])
    sig = soft_saturate(sig, 1.35)
    return Wave.mono(pad_to(sig, n)).normalize_to(-15.0)


def snd_ui_deny(seed):
    """Tampon sec — double impact bois MODAL très amorti (pas un buzzer). Court, net,
    un peu plus dur que ui_tick (le refus)."""
    rng = random.Random(seed)
    dur = 0.16
    n = n_samples(dur)
    exc = micro_burst(0.006, rng, n_impacts=rng.randint(2, 4), lp_hz=2200.0, hp_hz=90.0)
    exc = pad_to(exc, n)
    f0 = rng.uniform(108.0, 122.0)
    modes = [
        (f0,        20, 1.0,  0.05),
        (f0 * 2.76, 24, 0.45, 0.03),
        (f0 * 5.40, 28, 0.18, 0.015),
    ]
    body = modal_bank(exc, modes)
    body = onepole_lowpass(body, 2000.0)
    env = adsr(n, SR, a=0.0008, d=0.05, s_level=0.0, s_dur=0.0, r=0.06, curve=1.0)
    body = apply_env(pad_to(body, n), env)
    sig = soft_saturate(body, 1.45)
    return Wave.mono(pad_to(sig, n)).normalize_to(-14.0)


def snd_ui_coin(seed):
    """2-3 pièces : chaque impact = micro-rafale (le tintement double) excitant un banc
    modal de métal (partiels inharmoniques désaccordés), décalés dans le temps."""
    rng = random.Random(seed)
    n_coins = rng.randint(2, 3)
    total_dur = 0.05 + n_coins * 0.10
    n = n_samples(total_dur)
    sig = [0.0] * n
    for k in range(n_coins):
        t0 = 0.02 + k * rng.uniform(0.06, 0.10)
        cdur = 0.28
        cn = n_samples(cdur)
        exc = micro_burst(0.003, rng, n_impacts=rng.randint(2, 3), lp_hz=9000.0, hp_hz=1200.0)
        exc = pad_to(exc, cn)
        f0 = rng.uniform(1500.0, 2300.0)
        modes = [
            (f0,        90, 1.0,  0.20),
            (f0 * 1.62, 110, 0.55, 0.15),
            (f0 * 2.41, 130, 0.35, 0.10),
            (f0 * 3.17, 150, 0.18, 0.06),
        ]
        p = modal_bank(exc, modes)
        p = onepole_highpass(p, 700.0)
        off = n_samples(t0)
        for i, v in enumerate(p):
            if off + i < n:
                sig[off + i] += v
    sig = soft_saturate(sig, 1.1)
    return Wave.mono(sig).normalize_to(-16.0)


def snd_ui_scroll_tick(seed):
    """Cliquet léger de déroulant : micro-clic bois modal (2 modes) très bref."""
    rng = random.Random(seed)
    dur = 0.05
    n = n_samples(dur)
    exc = micro_burst(0.002, rng, n_impacts=rng.randint(1, 3), lp_hz=6000.0, hp_hz=500.0)
    exc = pad_to(exc, n)
    f0 = rng.uniform(820.0, 980.0)
    modes = [
        (f0,        24, 1.0,  0.016),
        (f0 * 2.76, 28, 0.35, 0.010),
    ]
    click = modal_bank(exc, modes)
    click = onepole_highpass(click, 500.0)
    click = onepole_lowpass(click, 6500.0)
    env = adsr(n, SR, a=0.0003, d=0.02, s_level=0.0, s_dur=0.0, r=0.02, curve=1.0)
    click = apply_env(pad_to(click, n), env)
    return Wave.mono(click).normalize_to(-19.0)


# ───────────────────────── recettes MOMENTS ─────────────────────────

def snd_moment_page_turn(seed):
    """Le swoosh de la page qui se tourne : texture granulaire large-bande balayée
    (grains dont la fréquence monte puis descend = le geste du feuillet) + froissement
    de fibres, 1.2 s, queue FDN."""
    rng = random.Random(seed)
    dur = 1.2
    n = n_samples(dur)
    # grains à fréquence pilotée par le geste (montée-descente) : on module la bande
    swoosh = [0.0] * n
    n_grains = int(360 * dur)
    for _ in range(n_grains):
        pos_t = rng.uniform(0.0, 1.0)
        pos = int(pos_t * n)
        gdur = rng.uniform(4.0, 16.0) / 1000.0
        gn = max(4, n_samples(gdur))
        arc = math.sin(math.pi * pos_t) ** 0.7    # bosse du geste
        fc = 300.0 + 3900.0 * arc
        amp = rng.uniform(0.2, 1.0) * (0.3 + 0.7 * arc)
        base = biquad_bandpass(white_noise(gdur, rng), min(fc, SR * 0.45), 2.5)
        for i in range(gn):
            wv = 0.5 - 0.5 * math.cos(2.0 * math.pi * i / max(1, gn - 1))
            j = pos + i
            if 0 <= j < n:
                swoosh[j] += base[i] * wv * amp
    rustle = granular_texture(dur, rng, grain_ms=(3.0, 12.0), density=300.0,
                              freq_range=(2200.0, 8000.0), q=3.0, grain_kind="ks")
    arc_env = [math.sin(math.pi * (i / n)) ** 0.7 for i in range(n)]
    rustle = apply_env(rustle, arc_env)
    sig = mix([s * 0.7 for s in swoosh], [r * 0.4 for r in rustle])
    sig = fdn_reverb(sig, wet=0.12, decay=0.6)
    return Wave.mono(pad_to(sig, n)).normalize_to(-16.0)


def snd_moment_age_bell(seed):
    """Grande cloche grave par SYNTHÈSE MODALE de métal : partiels inharmoniques étirés
    et désaccordés, drift 1/f de hauteur (le battement vivant), longue queue FDN."""
    rng = random.Random(seed)
    dur = 4.5
    n = n_samples(dur)
    base = 108.0 + rng.uniform(-2, 2)
    # la frappe : micro-rafale (le battant sur le bronze — plusieurs micro-contacts)
    exc = micro_burst(0.006, rng, n_impacts=rng.randint(3, 6), lp_hz=6000.0, hp_hz=200.0)
    exc = pad_to(exc, n)
    # partiels de cloche réels (hum, prime, tierce, quinte, nominal…) désaccordés
    ratios = [1.00, 2.00, 2.40, 3.00, 4.00, 5.38, 6.67]
    decays = [2.6, 2.1, 1.7, 1.3, 0.95, 0.7, 0.5]
    amps = [1.0, 0.62, 0.48, 0.34, 0.24, 0.16, 0.10]
    qs = [70, 90, 110, 130, 150, 170, 190]
    modes = []
    for r, dcy, a, q in zip(ratios, decays, amps, qs):
        det = 1.0 + rng.uniform(-0.004, 0.004)   # léger désaccord par partiel
        modes.append((base * r * det, q, a, dcy))
    bell = modal_bank(exc, modes)
    # drift 1/f de hauteur global (chorus lent) via un second banc très légèrement décalé
    exc2 = pad_to(micro_burst(0.006, rng, n_impacts=3, lp_hz=6000.0, hp_hz=200.0), n)
    modes2 = [(m[0] * 1.002, m[1], m[2] * 0.5, m[3]) for m in modes]
    bell = mix(bell, modal_bank(exc2, modes2))
    bell = onepole_lowpass(bell, 6500.0)
    sig = soft_saturate(bell, 1.05)
    sig = fdn_reverb(sig, wet=0.36, decay=0.9, damp_hz=3800.0, predelay_ms=18.0)
    return Wave.mono(pad_to(sig, n)).normalize_to(-15.0)


def snd_moment_war_horn(seed):
    """Corne lointaine : dents de scie graves (2 voix légèrement désaccordées + drift 1/f
    = le souffle humain instable) filtrées par des formants de pavillon (banc de bandes
    fixes), longue queue FDN (le lointain)."""
    rng = random.Random(seed)
    dur = 2.6
    n = n_samples(dur)
    freq = 98.0
    # drift 1/f de hauteur (l'instabilité du souffleur)
    drift = rand_walk(n, rng, smooth_hz=3.0)
    saw = _sawtooth_drift(dur, freq, drift, 0.008)
    saw2 = _sawtooth_drift(dur, freq * 1.006, drift, 0.006)
    src = mix([s * 0.6 for s in saw], [s * 0.4 for s in saw2])
    # formants de pavillon (banc de bandes) — le « corps » du cor
    f1 = biquad_bandpass(src, 220.0, 1.4)
    f2 = biquad_bandpass(src, 480.0, 2.0)
    f3 = biquad_bandpass(src, 720.0, 2.6)
    sig = mix(f1, [f * 0.5 for f in f2], [f * 0.3 for f in f3])
    env = adsr(n, SR, a=0.25, d=0.3, s_level=0.75, s_dur=dur * 0.45, r=dur * 0.35, curve=1.4)
    sig = apply_env(pad_to(sig, n), env)
    sig = soft_saturate(sig, 1.3)
    sig = fdn_reverb(sig, wet=0.42, decay=0.9, damp_hz=3000.0, predelay_ms=25.0)
    sig = onepole_lowpass(sig, 2800.0)
    return Wave.mono(pad_to(sig, n)).normalize_to(-16.0)


def _sawtooth_drift(dur, freq, drift, depth, sr=SR):
    n = n_samples(dur)
    out = [0.0] * n
    phase = 0.0
    for i in range(n):
        f = freq * (1.0 + depth * drift[i])
        out[i] = 2.0 * (phase - math.floor(phase + 0.5))
        phase += f / sr
    return out


def snd_moment_battle_drums(seed):
    """Tambours graves ×3 : chaque coup = micro-rafale (la baguette sur la peau, pas un
    clic) excitant un banc modal de fût (mode fondamental + harmoniques de membrane) +
    peau bruitée. Variation entre les 3 pour le naturel."""
    rng = random.Random(seed)
    hits_t = [0.0, 0.42, 0.80]
    dur = 1.55
    n = n_samples(dur)
    sig = [0.0] * n
    for k, t0 in enumerate(hits_t):
        hdur = 0.55
        hn = n_samples(hdur)
        exc = micro_burst(0.006, rng, n_impacts=rng.randint(3, 6), lp_hz=1200.0, hp_hz=35.0)
        exc = pad_to(exc, hn)
        f0 = 60.0 + k * 3 + rng.uniform(-3, 3)
        # modes de membrane frappée (inharmoniques : 1, 1.59, 2.14, 2.30, 2.65…)
        modes = [
            (f0,         10, 1.0,  0.14),
            (f0 * 1.59,  14, 0.5,  0.09),
            (f0 * 2.14,  18, 0.28, 0.06),
            (f0 * 2.92,  22, 0.15, 0.04),
        ]
        body = modal_bank(exc, modes)
        benv = adsr(hn, SR, a=0.001, d=0.12, s_level=0.05, s_dur=0.05, r=0.28, curve=1.6)
        body = apply_env(body, benv)
        # peau : bruit passe-bas percussif
        skin = onepole_lowpass(white_noise(hdur, rng), 150.0 + rng.uniform(-10, 10))
        skin = onepole_highpass(skin, 40.0)
        senv = adsr(hn, SR, a=0.001, d=0.08, s_level=0.03, s_dur=0.03, r=0.20, curve=1.7)
        skin = apply_env(skin, senv)
        hit = mix(body, [s * 0.55 for s in skin])
        off = n_samples(t0)
        for i, v in enumerate(hit):
            if off + i < n:
                sig[off + i] += v * (1.0 - 0.08 * k)
    sig = soft_saturate(sig, 1.4)
    sig = fdn_reverb(sig, wet=0.22, decay=0.72, damp_hz=3200.0)
    return Wave.mono(pad_to(sig, n)).normalize_to(-15.0)


def snd_moment_treason(seed):
    """Corde grave pincée qui CASSE + silence — la trahison. Corde modale tenue,
    brutalement coupée par un SNAP granulaire (rupture de fibres), puis silence net."""
    rng = random.Random(seed)
    dur_before = 0.9
    string = karplus_strong(dur_before, 72.0, rng, damping=0.997, brightness=0.35)
    senv = adsr(len(string), SR, a=0.01, d=0.2, s_level=0.55, s_dur=dur_before * 0.4,
                r=0.15, curve=1.3)
    string = apply_env(string, senv)
    # le snap : rupture = petite rafale de micro-craquements aigus
    snap = granular_texture(0.12, rng, grain_ms=(1.0, 4.0), density=600.0,
                            freq_range=(1800.0, 9000.0), q=5.0, grain_kind="ks")
    snap = onepole_highpass(snap, 1600.0)
    snap_env = adsr(len(snap), SR, a=0.0005, d=0.02, s_level=0.0, s_dur=0.0, r=0.07, curve=1.0)
    snap = apply_env(snap, snap_env)
    # queue résiduelle très courte puis silence complet
    tail = onepole_lowpass(white_noise(0.35, rng), 500.0)
    tail_env = adsr(len(tail), SR, a=0.0, d=0.05, s_level=0.0, s_dur=0.0, r=0.25, curve=2.2)
    tail = apply_env(tail, tail_env)
    sig = string + [s * 1.1 for s in snap] + [t * 0.22 for t in tail]
    sig = soft_saturate(sig, 1.15)
    return Wave.mono(sig).normalize_to(-16.0)


def snd_moment_ascension(seed):
    """Accord additif cristallin ascendant, 4 s — chœur de partiels PURS (sinus) qui
    montent d'une octave, avec drift 1/f de hauteur (le chatoiement vivant, pas figé) +
    shimmer granulaire aigu, longue queue FDN lumineuse."""
    rng = random.Random(seed)
    dur = 4.0
    n = n_samples(dur)
    base_freqs = [220.0, 277.18, 329.63, 440.0, 554.37]
    sig = [0.0] * n
    for k, f0 in enumerate(base_freqs):
        phase = 0.0
        delay = k * 0.12
        amp_env = adsr(n, SR, a=0.3 + delay, d=0.4, s_level=0.8, s_dur=dur * 0.5,
                       r=dur * 0.3, curve=1.2)
        drift = rand_walk(n, rng, smooth_hz=1.5)
        partial = [0.0] * n
        for i in range(n):
            t = i / SR
            glide = 1.0 + 1.0 * min(1.0, max(0.0, (t - delay) / max(0.1, dur - delay)))
            f = f0 * glide * (1.0 + 0.003 * drift[i])
            phase += 2.0 * math.pi * f / SR
            partial[i] = math.sin(phase) * amp_env[i]
        for i in range(n):
            sig[i] += partial[i] * (0.5 - 0.06 * k)
    shimmer = granular_texture(dur, rng, grain_ms=(10.0, 40.0), density=90.0,
                               freq_range=(5000.0, 11000.0), q=6.0, grain_kind="bp")
    shenv = adsr(n, SR, a=1.0, d=0.5, s_level=0.5, s_dur=dur * 0.4, r=dur * 0.4, curve=1.4)
    shimmer = apply_env(pad_to(shimmer, n), shenv)
    sig = mix(sig, [s * 0.10 for s in shimmer])
    sig = fdn_reverb(sig, wet=0.4, decay=0.92, damp_hz=6500.0, predelay_ms=20.0)
    return Wave.mono(pad_to(sig, n)).normalize_to(-15.0)


# ───────────────────────── recettes AMBIANCES (stéréo, 44.1 kHz, bouclées) ────

# ambiances : 44.1 kHz stéréo mais RACCOURCIES (budget ≤10 Mo) — l'air HF fait
# l'organique. La boucle invisible retire 2·AMB_FADE ⇒ durée effective ~5.6 s ; le
# grain granulaire/1-f rend la boucle inaudible malgré la brièveté (autocorr < 0.1).
AMB_DUR = 6.9
AMB_FADE = 1.15


def snd_amb_wind(seed):
    """Vent : bruit rose granulaire balayé par des rafales 1/f (cutoff piloté par une
    marche aléatoire lente, pas un LFO), stéréo DÉCORRÉLÉE (grains G≠D → largeur)."""
    rng_l = random.Random(seed * 2 + 1)
    rng_r = random.Random(seed * 2 + 2)

    def make(rng):
        n = n_samples(AMB_DUR)
        base = pink_noise(AMB_DUR, rng)
        # rafales : cutoff = marche 1/f entre ~400 et ~1600 Hz
        walk = rand_walk(n, rng, smooth_hz=0.35)
        cut = [700.0 + 600.0 * (0.5 + 0.5 * walk[i]) for i in range(n)]
        gust = onepole_lowpass(base, lambda i: cut[i])
        gust = onepole_highpass(gust, 60.0)
        # grains de sifflement épars (le vent qui accroche une arête)
        whistle = granular_texture(AMB_DUR, rng, grain_ms=(20.0, 40.0), density=6.0,
                                   freq_range=(1200.0, 3000.0), q=8.0, grain_kind="bp")
        # amplitude globale modulée 1/f (les bourrasques)
        amp = rand_walk(n, rng, smooth_hz=0.18)
        env = [0.5 + 0.5 * (0.5 + 0.5 * amp[i]) for i in range(n)]
        sig = [gust[i] * env[i] + whistle[i] * 0.25 for i in range(n)]
        return sig

    left = crossfade_loop(make(rng_l), AMB_FADE)
    right = crossfade_loop(make(rng_r), AMB_FADE)
    return Wave.stereo(left, right).normalize_to(-22.0)


def snd_amb_sea(seed):
    """Ressac : vagues = enveloppes LENTES et irrégulières (marche 1/f) sur bruit
    granulaire d'écume, stéréo décorrélée."""
    rng_l = random.Random(seed * 2 + 11)
    rng_r = random.Random(seed * 2 + 12)

    def make(rng):
        n = n_samples(AMB_DUR)
        surf = onepole_lowpass(white_noise(AMB_DUR, rng), 1400.0)
        surf = onepole_highpass(surf, 90.0)
        # écume : grains aigus épars pendant la crête de vague
        foam = granular_texture(AMB_DUR, rng, grain_ms=(8.0, 25.0), density=40.0,
                                freq_range=(2000.0, 6000.0), q=4.0, grain_kind="bp")
        # vagues : bosses lentes à espacement JITTÉ
        env = [0.15] * n
        t = 0.0
        while t < AMB_DUR:
            period = rng.uniform(3.0, 5.2)
            width = period * 0.55
            center = t + period * 0.5
            amp = rng.uniform(0.5, 1.0)
            ci = center * SR
            wi = width * SR
            for i in range(max(0, int(ci - wi * 2)), min(n, int(ci + wi * 2))):
                d = (i - ci) / max(1.0, wi)
                env[i] += amp * math.exp(-0.5 * d * d) * 0.85
            t += period
        mxv = max(env)
        env = [e / mxv for e in env]
        sig = [(surf[i] + foam[i] * 0.4) * env[i] for i in range(n)]
        return sig

    left = crossfade_loop(make(rng_l), AMB_FADE)
    right = crossfade_loop(make(rng_r), AMB_FADE)
    return Wave.stereo(left, right).normalize_to(-22.0)


def snd_amb_crowd(seed):
    """Rumeur de foule : voix = GRAINS DE FORMANTS vocaux (2-3 bandes) désaccordés et
    superposés (technique 4 granulaire, grain_kind='formant') → indistinct, jamais un
    mot. Densité modérée, modulation 1/f par voix, stéréo décorrélée."""
    rng_l = random.Random(seed * 2 + 21)
    rng_r = random.Random(seed * 2 + 22)

    def make(rng):
        n = n_samples(AMB_DUR)
        # deux couches de grains-formants à densité modérée = brouhaha
        babble = granular_texture(AMB_DUR, rng, grain_ms=(60.0, 160.0), density=22.0,
                                  freq_range=(250.0, 900.0), q=5.0, grain_kind="formant")
        babble2 = granular_texture(AMB_DUR, rng, grain_ms=(80.0, 200.0), density=16.0,
                                   freq_range=(300.0, 1200.0), q=6.0, grain_kind="formant")
        sig = [babble[i] * 0.7 + babble2[i] * 0.6 for i in range(n)]
        sig = onepole_lowpass(sig, 3200.0)
        # respiration 1/f du brouhaha (des pics d'agitation)
        amp = rand_walk(n, rng, smooth_hz=0.4)
        sig = [sig[i] * (0.6 + 0.4 * (0.5 + 0.5 * amp[i])) for i in range(n)]
        return sig

    left = crossfade_loop(make(rng_l), AMB_FADE)
    right = crossfade_loop(make(rng_r), AMB_FADE)
    return Wave.stereo(left, right).normalize_to(-22.0)


def snd_amb_entropy(seed):
    """Drone d'inquiétude : basses BATTANTES (deux sinus proches + drift 1/f → battement
    vivant) + grains graves granuleux — le volume est piloté côté jeu (entropie)."""
    rng_l = random.Random(seed * 2 + 31)
    rng_r = random.Random(seed * 2 + 32)

    def make(rng, f0):
        n = n_samples(AMB_DUR)
        drift = rand_walk(n, rng, smooth_hz=0.6)
        sig = [0.0] * n
        p1 = p2 = 0.0
        for i in range(n):
            d = 1.0 + 0.004 * drift[i]
            p1 += 2 * math.pi * f0 * d / SR
            p2 += 2 * math.pi * f0 * 1.013 * d / SR
            sig[i] = 0.5 * math.sin(p1) + 0.5 * math.sin(p2)
        grain = granular_texture(AMB_DUR, rng, grain_ms=(15.0, 40.0), density=30.0,
                                 freq_range=(120.0, 700.0), q=3.0, grain_kind="bp")
        return [sig[i] * 0.7 + grain[i] * 0.4 for i in range(n)]

    left = crossfade_loop(make(rng_l, 55.0), AMB_FADE)
    right = crossfade_loop(make(rng_r, 55.3), AMB_FADE)
    wv = Wave.stereo(left, right)
    wv = Wave([soft_saturate(c, 1.1) for c in wv.ch])
    return wv.normalize_to(-22.0)


def snd_drone_ronces(seed):
    """Craquements de bois lents (grains KS graves épars, ré-excités irrégulièrement) +
    nappe sombre (bruit filtré très bas modulé 1/f)."""
    rng_l = random.Random(seed * 2 + 41)
    rng_r = random.Random(seed * 2 + 42)

    def make(rng):
        n = n_samples(AMB_DUR)
        nappe = onepole_lowpass(pink_noise(AMB_DUR, rng), 220.0)
        amp = rand_walk(n, rng, smooth_hz=0.25)
        nappe = [nappe[i] * (0.6 + 0.4 * (0.5 + 0.5 * amp[i])) for i in range(n)]
        sig = [v * 0.5 for v in nappe]
        # craquements : grains KS graves longs, espacés irrégulièrement
        t = 0.0
        while t < AMB_DUR - 0.5:
            t += rng.uniform(0.6, 2.2)
            if t >= AMB_DUR - 0.4:
                break
            cn = n_samples(0.32)
            crk = _ks_short(cn, rng.uniform(58, 110), rng, damping=0.965, brightness=0.05)
            cenv = adsr(cn, SR, a=0.001, d=0.1, s_level=0.0, s_dur=0.0, r=0.18, curve=1.4)
            crk = apply_env(crk, cenv)
            off = n_samples(t)
            for i, v in enumerate(crk):
                if off + i < n:
                    sig[off + i] += v * rng.uniform(0.3, 0.7)
        return sig

    left = crossfade_loop(make(rng_l), AMB_FADE)
    right = crossfade_loop(make(rng_r), AMB_FADE)
    return Wave.stereo(left, right).normalize_to(-22.0)


def snd_drone_hiver(seed):
    """Nappe cristalline (partiels hauts purs très étirés, drift 1/f = souffle du gel) +
    vent fin granulaire aigu et discret."""
    rng_l = random.Random(seed * 2 + 51)
    rng_r = random.Random(seed * 2 + 52)

    def make(rng, base):
        n = n_samples(AMB_DUR)
        sig = [0.0] * n
        ratios = [1.0, 2.01, 3.03, 4.98, 6.12]
        for r in ratios:
            p = 0.0
            drift = rand_walk(n, rng, smooth_hz=0.8)
            amp = 1.0 / (r * 1.4)
            f = base * r
            for i in range(n):
                p += 2 * math.pi * f * (1.0 + 0.002 * drift[i]) / SR
                sig[i] += math.sin(p) * amp
        wind = granular_texture(AMB_DUR, rng, grain_ms=(20.0, 45.0), density=25.0,
                                freq_range=(5200.0, 9500.0), q=6.0, grain_kind="bp")
        return [sig[i] * 0.22 + wind[i] * 0.16 for i in range(n)]

    left = crossfade_loop(make(rng_l, 330.0), AMB_FADE)
    right = crossfade_loop(make(rng_r, 331.5), AMB_FADE)
    return Wave.stereo(left, right).normalize_to(-22.0)


def snd_drone_eau(seed):
    """Basses liquides (bruit filtré très bas modulé 1/f, houle profonde) + gouttes
    modales réverbérées (impacts courts espacés, chacun passé dans le FDN)."""
    rng_l = random.Random(seed * 2 + 61)
    rng_r = random.Random(seed * 2 + 62)

    def make(rng):
        n = n_samples(AMB_DUR)
        liquid = onepole_lowpass(pink_noise(AMB_DUR, rng), 260.0)
        walk = rand_walk(n, rng, smooth_hz=0.3)
        liquid = [liquid[i] * (0.6 + 0.4 * (0.5 + 0.5 * walk[i])) for i in range(n)]
        sig = [v * 0.6 for v in liquid]
        # gouttes : petits corps modaux réverbérés
        t = 0.0
        while t < AMB_DUR - 0.3:
            t += rng.uniform(0.4, 1.6)
            if t >= AMB_DUR - 0.25:
                break
            dn = n_samples(0.22)
            exc = pad_to(micro_burst(0.002, rng, n_impacts=1, lp_hz=9000.0, hp_hz=800.0), dn)
            f0 = rng.uniform(900, 1700)
            drop = modal_bank(exc, [(f0, 120, 1.0, 0.06), (f0 * 2.7, 140, 0.4, 0.03)])
            drop = fdn_reverb(drop, wet=0.5, decay=0.6, damp_hz=5000.0)
            off = n_samples(t)
            for i, v in enumerate(drop):
                if off + i < n:
                    sig[off + i] += v * rng.uniform(0.15, 0.32)
        return sig

    left = crossfade_loop(make(rng_l), AMB_FADE)
    right = crossfade_loop(make(rng_r), AMB_FADE)
    return Wave.stereo(left, right).normalize_to(-22.0)


def snd_drone_sang(seed):
    """Tambours étouffés très lents (corps modal grave très amorti, espacé comme un
    pouls jitté 1/f) + souffle (bruit rose très filtré, sombre, modulé 1/f)."""
    rng_l = random.Random(seed * 2 + 71)
    rng_r = random.Random(seed * 2 + 72)

    def make(rng):
        n = n_samples(AMB_DUR)
        breath = onepole_lowpass(pink_noise(AMB_DUR, rng), 180.0)
        amp = rand_walk(n, rng, smooth_hz=0.2)
        breath = [breath[i] * (0.6 + 0.4 * (0.5 + 0.5 * amp[i])) for i in range(n)]
        sig = [v * 0.45 for v in breath]
        t = 0.4
        period = 1.15
        while t < AMB_DUR - 0.3:
            hn = n_samples(0.35)
            exc = pad_to(micro_burst(0.01, rng, n_impacts=rng.randint(2, 4),
                                     lp_hz=600.0, hp_hz=30.0), hn)
            f0 = 48.0 + rng.uniform(-3, 3)
            thud = modal_bank(exc, [(f0, 8, 1.0, 0.12), (f0 * 1.6, 12, 0.4, 0.07)])
            henv = adsr(hn, SR, a=0.002, d=0.15, s_level=0.0, s_dur=0.0, r=0.16, curve=1.8)
            thud = apply_env(thud, henv)
            off = n_samples(t)
            for i, v in enumerate(thud):
                if off + i < n:
                    sig[off + i] += v * 0.6
            t += period + rng.uniform(-0.06, 0.06)
        return sig

    left = crossfade_loop(make(rng_l), AMB_FADE)
    right = crossfade_loop(make(rng_r), AMB_FADE)
    return Wave.stereo(left, right).normalize_to(-22.0)


# ───────────────────────── registre & pipeline ─────────────────────────

RECIPES = {
    "ui_tick.wav":              snd_ui_tick,
    "ui_tick_year.wav":         snd_ui_tick_year,
    "ui_parchment_open.wav":    snd_ui_parchment_open,
    "ui_parchment_close.wav":   snd_ui_parchment_close,
    "ui_quill.wav":             snd_ui_quill,
    "ui_seal.wav":              snd_ui_seal,
    "ui_deny.wav":              snd_ui_deny,
    "ui_coin.wav":              snd_ui_coin,
    "ui_scroll_tick.wav":       snd_ui_scroll_tick,
    "moment_page_turn.wav":     snd_moment_page_turn,
    "moment_age_bell.wav":      snd_moment_age_bell,
    "moment_war_horn.wav":      snd_moment_war_horn,
    "moment_battle_drums.wav":  snd_moment_battle_drums,
    "moment_treason.wav":       snd_moment_treason,
    "moment_ascension.wav":     snd_moment_ascension,
    "amb_wind.wav":             snd_amb_wind,
    "amb_sea.wav":              snd_amb_sea,
    "amb_crowd.wav":            snd_amb_crowd,
    "amb_entropy.wav":          snd_amb_entropy,
    "drone_ronces.wav":         snd_drone_ronces,
    "drone_hiver.wav":          snd_drone_hiver,
    "drone_eau.wav":            snd_drone_eau,
    "drone_sang.wav":           snd_drone_sang,
}

# Sons FRÉQUENTS déclinés en N variantes (round-robin au jeu). Le pipeline génère
# `nom_1.wav … nom_N.wav` avec des graines décalées → spectres/RMS réellement distincts.
# sound.gd tire une variante au hasard + pitch/gain jitter à chaque lecture (technique 1).
VARIANTS = {
    "ui_tick.wav":            4,
    "ui_quill.wav":           3,
    "ui_seal.wav":            3,
    "ui_coin.wav":            3,
    "ui_scroll_tick.wav":     4,
    "ui_parchment_open.wav":  3,
    "ui_parchment_close.wav": 3,
}

AMBIANCE_NAMES = {n for n in RECIPES if n.startswith("amb_") or n.startswith("drone_")}

# sons TEXTURE dont on EXIGE une large bande spectrale (souffle/froissement/foule) — par
# opposition à la percussion modale (tick/deny/coin/cloche…) et aux DRONES (nappes
# tonales de fin, narrow-band PAR NATURE), tous deux légitimement étroits.
_BROADBAND_NAMES = {
    "ui_parchment_open.wav", "ui_parchment_close.wav", "ui_quill.wav",
    "moment_page_turn.wav",
    "amb_wind.wav", "amb_sea.wav", "amb_crowd.wav",   # ambiances bruitées
}
# les DRONES (drone_*) + amb_entropy sont des NAPPES tonales tenues : un « motif
# répétitif » à l'échelle macro EST le drone (steady par design) ⇒ exempts d'autocorr.
_STEADY_DRONE_NAMES = {n for n in RECIPES if n.startswith("drone_")} | {"amb_entropy.wav"}


def _variant_names(base):
    """base 'ui_tick.wav', n → ['ui_tick_1.wav', …] (ou [base] si pas de variante)."""
    n = VARIANTS.get(base, 0)
    if n <= 0:
        return [base]
    stem = base[:-4]
    return ["%s_%d.wav" % (stem, k + 1) for k in range(n)]


# Le jeu est passé à 100 % de VRAIS ENREGISTREMENTS (façon Paradox : un clic = un son,
# aucune nappe synthé). Ce générateur est DÉPRÉCIÉ : chaque nom ci-dessous est soit un
# son réel (audio/), soit un son RETIRÉ (drones/entropie/ambiances/coin/deny — jugés
# « moches »). gen_all() les saute TOUS → il ne régénère rien qui clobberait ou
# ré-encombrerait le dossier. (Retirer un nom le rend de nouveau générable en synthé.)
REAL_RECORDINGS = {
    # réels, en place
    "ui_tick.wav", "ui_parchment_open.wav", "ui_parchment_close.wav",
    "ui_quill.wav", "ui_seal.wav",
    "moment_page_turn.wav", "moment_age_bell.wav", "moment_war_horn.wav",
    "moment_battle_drums.wav",
    # retirés (procéduraux — ne PAS régénérer)
    "ui_tick_year.wav", "ui_scroll_tick.wav", "ui_coin.wav", "ui_deny.wav",
    "moment_ascension.wav", "moment_treason.wav",
    "amb_wind.wav", "amb_sea.wav", "amb_crowd.wav", "amb_entropy.wav",
    "drone_eau.wav", "drone_hiver.wav", "drone_ronces.wav", "drone_sang.wav",
}


def gen_all(out_dir, base_seed, names=None):
    os.makedirs(out_dir, exist_ok=True)
    todo = names if names else list(RECIPES.keys())
    results = {}
    for name in todo:
        if name in REAL_RECORDINGS:
            continue   # un vrai son occupe déjà ce nom — ne pas l'écraser
        fn = RECIPES[name]
        outputs = _variant_names(name)
        for vi, outname in enumerate(outputs):
            # seed PAR NOM DE SORTIE (variante vi distincte → spectre réellement autre)
            seed = base_seed + sum((k + 1) * ord(c) for k, c in enumerate(outname)) + vi * 104729
            wv = fn(seed)
            path = os.path.join(out_dir, outname)
            write_wav(path, wv, sr=SR)
            results[outname] = path
    return results


def _all_output_names(names=None):
    todo = names if names else list(RECIPES.keys())
    out = []
    for name in todo:
        out.extend(_variant_names(name))
    return out


# ───────────────────────── métriques (les « oreilles ») ─────────────────────

def _read_chan0(path):
    with wave.open(path, 'rb') as f:
        nch = f.getnchannels()
        n = f.getnframes()
        fr = f.getframerate()
        raw = f.readframes(n)
    a = array.array('h')
    a.frombytes(raw)
    chan0 = [a[i * nch] / 32768.0 for i in range(n)]
    return chan0, fr, nch


def zcr_hz(path):
    """Taux de passage par zéro (Hz) — proxy du CENTRE spectral (variété)."""
    chan0, fr, nch = _read_chan0(path)
    n = len(chan0)
    crossings = 0
    for i in range(1, n):
        if (chan0[i - 1] < 0) != (chan0[i] < 0):
            crossings += 1
    dur = n / float(fr)
    return crossings / (2.0 * max(0.001, dur))


def spectral_bands(path, nbands=6):
    """Énergie répartie sur `nbands` bandes log (proxy de RICHESSE spectrale : un son
    riche étale son énergie, un bip la concentre). Renvoie (bandes normalisées, entropie
    spectrale 0-1). DFT bon marché sur une fenêtre de 8192 (numpy FFT si présent)."""
    chan0, fr, nch = _read_chan0(path)
    n = len(chan0)
    if n < 512:
        return [1.0], 0.0
    N = 8192 if n >= 8192 else 1 << (n.bit_length() - 1)
    # centre de la fenêtre (les moments ont l'attaque au début, les ambiances partout)
    start = max(0, (n - N) // 2)
    win = chan0[start:start + N]
    # Hann
    win = [win[i] * (0.5 - 0.5 * math.cos(2 * math.pi * i / (N - 1))) for i in range(N)]
    if HAVE_NUMPY:
        spec = _np.abs(_np.fft.rfft(_np.asarray(win)))
        mags = spec.tolist()
    else:
        mags = _dft_mag(win)
    half = len(mags)
    # bandes log de ~60 Hz à Nyquist
    lo_hz, hi_hz = 60.0, fr * 0.5
    edges = [lo_hz * (hi_hz / lo_hz) ** (b / nbands) for b in range(nbands + 1)]
    bands = [0.0] * nbands
    for k in range(1, half):
        f = k * fr / float(N)
        for b in range(nbands):
            if edges[b] <= f < edges[b + 1]:
                bands[b] += mags[k] * mags[k]
                break
    total = sum(bands) or 1.0
    norm = [b / total for b in bands]
    # entropie spectrale normalisée (1 = énergie parfaitement étalée)
    ent = 0.0
    for p in norm:
        if p > 1e-9:
            ent -= p * math.log(p)
    ent /= math.log(nbands)
    return norm, ent


def _dft_mag(win):
    """DFT naïve mais décimée : on ne calcule que ~256 bins (assez pour l'énergie de
    bande), sinon O(N²) sur 8192 est trop lent en pur Python."""
    N = len(win)
    nbins = 256
    mags = [0.0] * (N // 2 + 1)
    for bi in range(nbins):
        k = int(bi * (N // 2) / nbins)
        re = im = 0.0
        # sous-échantillonne la fenêtre (1 pt sur 4) pour la vitesse — proxy suffisant
        step = 4
        w = 2.0 * math.pi * k / N
        for t in range(0, N, step):
            re += win[t] * math.cos(w * t)
            im -= win[t] * math.sin(w * t)
        mags[k] = math.sqrt(re * re + im * im)
    return mags


def autocorr_peak(path, min_period_s=0.3, max_period_s=3.0):
    """Pic d'autocorrélation dans [min,max] s (hors lag 0) — proxy de RÉPÉTITION d'une
    ambiance. Un pic FORT = motif périodique audible (mauvais). On décime lourdement
    (le signal est stochastique, on cherche une périodicité macro)."""
    chan0, fr, nch = _read_chan0(path)
    n = len(chan0)
    if n < fr:
        return 0.0
    dec = 64
    x = chan0[::dec]
    m = len(x)
    mean = sum(x) / m
    x = [v - mean for v in x]
    energy = sum(v * v for v in x) or 1.0
    lo = int(min_period_s * fr / dec)
    hi = min(m - 1, int(max_period_s * fr / dec))
    best = 0.0
    for lag in range(lo, hi):
        s = 0.0
        for i in range(m - lag):
            s += x[i] * x[i + lag]
        r = abs(s) / energy
        if r > best:
            best = r
    return best


def count_onsets(path, thresh=2.5):
    """Nombre d'onsets (transitoires) — proxy de DENSITÉ pour les rustles. Détecte les
    hausses brusques d'énergie d'enveloppe."""
    chan0, fr, nch = _read_chan0(path)
    n = len(chan0)
    win = max(1, int(fr * 0.003))
    env = []
    acc = 0.0
    for i in range(n):
        acc += chan0[i] * chan0[i]
        if (i + 1) % win == 0:
            env.append(math.sqrt(acc / win))
            acc = 0.0
    if len(env) < 3:
        return 0
    onsets = 0
    for i in range(1, len(env)):
        prev = env[i - 1] + 1e-6
        if env[i] / prev > thresh and env[i] > 0.01:
            onsets += 1
    return onsets


def check_all(out_dir, names=None):
    outputs = _all_output_names(names)
    print("\n%-26s %7s %7s %7s %8s %6s %6s %7s" % (
        "fichier", "durée", "crête", "RMS", "loop", "ZCR", "ent", "onsets"))
    print("-" * 82)
    problems = []
    # pour la vérif « variantes distinctes » : regrouper les variantes par base
    variant_groups = {}
    for base, cnt in VARIANTS.items():
        if names and base not in names:
            continue
        variant_groups[base] = _variant_names(base)

    band_rms = {}
    for name in outputs:
        path = os.path.join(out_dir, name)
        if not os.path.exists(path):
            problems.append("%s: MANQUANT" % name)
            continue
        dur, pk, rms, nchan = read_wav_peak_rms(path)
        base_amb = _variant_base(name) in AMBIANCE_NAMES
        seam = ""
        if base_amb:
            chan0, fr, nch = _read_chan0(path)
            seam_val = loop_seam_score(chan0)
            seam = "%.3f" % seam_val
            if seam_val > 0.08:
                problems.append("%s: loop-seam élevé (%.4f)" % (name, seam_val))
        _, ent = spectral_bands(path)
        onsets = count_onsets(path)
        band_rms[name] = rms
        print("%-26s %7.2f %7.1f %7.1f %8s %6.0f %6.2f %7d" % (
            name, dur, lin_to_db(pk), lin_to_db(rms), seam, zcr_hz(path), ent, onsets))
        if pk < 1e-4:
            problems.append("%s: silence (crête ~0)" % name)
        if rms < 0.001:
            problems.append("%s: quasi-silence (rms<0.001)" % name)
        if pk >= 0.999:
            problems.append("%s: clip (crête ≥ 0.999)" % name)
        # richesse spectrale : on n'EXIGE une large bande que des TEXTURES (parchemin,
        # page, quill, ambiances). Une percussion modale (tick, deny, coin, seal, drums,
        # cloche, corne) est LÉGITIMEMENT tonale/étroite — c'est ce qui la fait « corps »
        # et non « souffle ». On ne flague donc l'étroitesse que pour les textures.
        if _variant_base(name) in _BROADBAND_NAMES and ent < 0.42:
            problems.append("%s: spectre étroit pour une texture (entropie %.2f)" % (name, ent))
        expect_stereo = base_amb
        if expect_stereo and nchan != 2:
            problems.append("%s: attendu stéréo, trouvé %d canaux" % (name, nchan))
        if not expect_stereo and nchan != 1:
            problems.append("%s: attendu mono, trouvé %d canaux" % (name, nchan))

    # autocorrélation des ambiances (non-répétition)
    amb_outputs = [n for n in outputs if _variant_base(n) in AMBIANCE_NAMES]
    if amb_outputs:
        print("\nNON-RÉPÉTITION (autocorr pic, < ~0.5 souhaité) :")
        for name in amb_outputs:
            path = os.path.join(out_dir, name)
            if os.path.exists(path):
                ac = autocorr_peak(path)
                # les drones tonaux sont steady par design → pas de flag de répétition
                steady = _variant_base(name) in _STEADY_DRONE_NAMES
                flag = "" if steady else (" ⚠" if ac > 0.6 else "")
                tag = " (drone, steady)" if steady else ""
                print("  %-24s %.3f%s%s" % (name, ac, flag, tag))
                if ac > 0.6 and not steady:
                    problems.append("%s: autocorr élevé (%.3f) — motif répétitif" % (name, ac))

    # variantes réellement distinctes ?
    if variant_groups:
        print("\nVARIANTES DISTINCTES (RMS + centre spectral doivent différer) :")
        for base, vs in variant_groups.items():
            zcrs = []
            rmss = []
            for v in vs:
                p = os.path.join(out_dir, v)
                if os.path.exists(p):
                    zcrs.append(zcr_hz(p))
                    rmss.append(band_rms.get(v, 0.0))
            if len(zcrs) >= 2:
                zspread = (max(zcrs) - min(zcrs)) / max(1.0, sum(zcrs) / len(zcrs))
                rspread = (max(rmss) - min(rmss)) / max(1e-6, sum(rmss) / len(rmss))
                ok = zspread > 0.02 or rspread > 0.03
                print("  %-22s ZCR spread %.1f%%  RMS spread %.1f%%  %s" % (
                    base, zspread * 100, rspread * 100, "OK" if ok else "⚠ trop proches"))
                if not ok:
                    problems.append("%s: variantes trop proches (spectres quasi-identiques)" % base)

    total_bytes = sum(os.path.getsize(os.path.join(out_dir, n)) for n in outputs
                      if os.path.exists(os.path.join(out_dir, n)))
    print("\nTotal : %.2f Mo pour %d fichiers" % (total_bytes / (1024 * 1024), len(outputs)))
    if problems:
        print("\n⚠ PROBLÈMES :")
        for p in problems:
            print(" - " + p)
        return False
    print("\nToutes les métriques sont propres.")
    return True


def _variant_base(outname):
    """'ui_tick_2.wav' → 'ui_tick.wav' si c'est une variante connue, sinon outname."""
    if outname in RECIPES:
        return outname
    stem = outname[:-4]
    if "_" in stem:
        head, tail = stem.rsplit("_", 1)
        if tail.isdigit() and (head + ".wav") in VARIANTS:
            return head + ".wav"
    return outname


def main():
    ap = argparse.ArgumentParser(description="Générateur SON de SCPS (organique).")
    ap.add_argument("--out", default=os.path.join(os.path.dirname(__file__), "..", "godot", "project", "audio"))
    ap.add_argument("--seed", type=int, default=20260706)
    ap.add_argument("--only", nargs="*", default=None, help="ne (re)générer que ces recettes (nom.wav)")
    ap.add_argument("--no-check", action="store_true")
    args = ap.parse_args()

    out_dir = os.path.abspath(args.out)
    names = args.only
    print("Génération dans %s (seed=%d, numpy=%s)…" % (out_dir, args.seed, HAVE_NUMPY))
    gen_all(out_dir, args.seed, names)
    n_out = len(_all_output_names(names))
    print("Fait : %d fichier(s).\n" % n_out)
    if not args.no_check:
        ok = check_all(out_dir, names)
        if not ok:
            raise SystemExit(1)


if __name__ == "__main__":
    main()
