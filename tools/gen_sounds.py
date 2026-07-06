#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
gen_sounds.py — le générateur SON DE SCPS (synthèse procédurale, hors-ligne, déterministe).

MODE D'EMPLOI (régénération) :
    /d/MSYS2/mingw64/bin/python3 tools/gen_sounds.py
    (ou : python3 tools/gen_sounds.py --out godot/project/audio --seed 20260706)

    Écrit ~20 fichiers WAV (16-bit PCM ; UI/moments mono 44.1 kHz, ambiances stéréo
    bouclées 16 kHz — descendues pour le budget de dépôt, cf. AMB_SR) dans
    godot/project/audio/. Aucune dépendance externe (numpy absent de cet environnement
    MSYS2 → tout est écrit en pur stdlib : array/math/wave/random). Determinism : chaque
    son a un seed fixe (SEED + offset nommé) → deux runs produisent des fichiers
    BYTE-IDENTIQUES. Ce script ne touche AUCUN fichier C ni logique de simulation —
    horloge murale pure, le déterminisme du moteur n'entend rien.

    Ajouter un son : écrire une fonction `def snd_xxx(sr) -> Wave`, l'enregistrer dans
    RECIPES en bas de fichier avec son nom de fichier exact, relancer le script.

    Vérification automatique (--check, activé par défaut après génération) : durée,
    crête dBFS, RMS, et pour les ambiances la « couture » de boucle (discontinuité
    au point de bouclage, doit rester sous un seuil - cf. `loop_seam_score`).

Voir TROUVAILLES.md pour les recettes de synthèse qui marchent (réverb Schroeder,
loop-seam, Karplus-Strong amorti, FM inharmonique) — à réutiliser pour des sons futurs.
"""

import argparse
import array
import io
import math
import os
import random
import sys
import wave

# console Windows en cp1252 par défaut : force UTF-8 en sortie pour éviter les
# UnicodeEncodeError sur les accents/⚠ (n'affecte que l'affichage, pas les WAV).
if hasattr(sys.stdout, "buffer"):
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")

SR = 44100


# ───────────────────────── conteneur audio minimal ─────────────────────────

class Wave:
    """Buffer audio en float, mono ou stéréo (liste de canaux — 1 ou 2)."""

    def __init__(self, channels):
        # channels : liste de listes de float (-1..1), une par canal
        self.ch = channels

    @staticmethod
    def mono(samples):
        return Wave([samples])

    @staticmethod
    def stereo(left, right):
        return Wave([left, right])

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
    for i in range(n):
        fc = cutoff_hz(i) if callable(cutoff_hz) else cutoff_hz
        fc = max(1.0, min(fc, sr * 0.49))
        a = math.exp(-2.0 * math.pi * fc / sr)
        y = (1.0 - a) * x[i] + a * y
        out[i] = y
    return out


def onepole_highpass(x, cutoff_hz, sr=SR):
    lp = onepole_lowpass(x, cutoff_hz, sr)
    return [x[i] - lp[i] for i in range(len(x))]


def bandpass_sweep(x, f_lo, f_hi, sr=SR):
    """Passe-bande balayé : centre linéairement de f_lo à f_hi sur la durée du buffer.
    Implémenté par une paire passe-bas cascadée (large) moins un passe-bas (étroit),
    puis un passe-haut lent — assez pour un « sifflement/froissement » organique."""
    n = len(x)

    def center(i):
        t = i / max(1, n - 1)
        return f_lo + (f_hi - f_lo) * t

    lo = onepole_lowpass(x, lambda i: center(i) * 1.5, sr)
    hi = onepole_highpass(lo, lambda i: center(i) * 0.6, sr)
    return hi


def biquad_bandpass(x, freq, q, sr=SR):
    """Biquad passe-bande RBJ (constant peak gain) — utile pour des formants fixes."""
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
    x1 = x2 = y1 = y2 = 0.0
    out = [0.0] * len(x)
    for i, xi in enumerate(x):
        y = b0 * xi + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
        out[i] = y
        x2, x1 = x1, xi
        y2, y1 = y1, y
    return out


def karplus_strong(seconds, freq, rng, damping=0.996, brightness=0.5, seed_noise=None, sr=SR):
    """Corde/percussion en bois pincée — Karplus-Strong classique.
    brightness ∈ [0,1] : mélange filtre moyenneur (sombre) / quasi passe-tout (clair)."""
    n = n_samples(seconds)
    period = max(2, int(round(sr / freq)))
    buf = seed_noise if seed_noise is not None else [rng.uniform(-1.0, 1.0) for _ in range(period)]
    buf = list(buf[:period]) + [0.0] * max(0, period - len(buf))
    out = [0.0] * n
    idx = 0
    prev = buf[0]
    for i in range(n):
        cur = buf[idx]
        # filtre moyenneur amorti (brightness mélange avec le signal brut)
        avg = (cur + prev) * 0.5
        newv = damping * (brightness * avg + (1.0 - brightness) * cur)
        out[i] = cur
        buf[idx] = newv
        prev = cur
        idx = (idx + 1) % period
    return out


def adsr(n, sr, a, d, s_level, s_dur, r, curve=1.6):
    """Enveloppe ADSR souple (courbes exponentielles, pas de coudes durs)."""
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
    start_lvl = s_level
    for k in range(nr_eff):
        t = k / max(1, nr_eff)
        out[i + k] = start_lvl * ((1.0 - t) ** curve)
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
    return [math.tanh(v * d) / math.tanh(d) for v in x]


def delay_line_comb(x, delay_samples, feedback, damp=0.2):
    n = len(x)
    out = [0.0] * n
    buf = [0.0] * max(1, delay_samples)
    idx = 0
    lp = 0.0
    for i in range(n):
        d = buf[idx]
        lp = d * (1.0 - damp) + lp * damp
        out[i] = x[i] + d
        buf[idx] = x[i] + lp * feedback
        idx = (idx + 1) % len(buf)
    return out


def allpass(x, delay_samples, g=0.5):
    n = len(x)
    out = [0.0] * n
    buf = [0.0] * max(1, delay_samples)
    idx = 0
    for i in range(n):
        bufout = buf[idx]
        vn = x[i] + -g * bufout
        out[i] = bufout + g * vn
        buf[idx] = vn
        idx = (idx + 1) % len(buf)
    return out


# réverb de Schroeder : 4 combs en parallèle + 2 allpass en série — la « queue de salle ».
_COMB_MS = [29.7, 37.1, 41.1, 43.7]
_AP_MS = [5.0, 1.7]


def schroeder_reverb(x, sr=SR, room=0.78, damp=0.25, wet=0.28, predelay_ms=8.0):
    pre = n_samples(predelay_ms / 1000.0)
    xin = [0.0] * pre + x
    combs = []
    for ms in _COMB_MS:
        d = max(1, int(round(sr * ms / 1000.0)))
        combs.append(delay_line_comb(xin, d, room, damp))
    n = max(len(c) for c in combs)
    wetsig = [0.0] * n
    for c in combs:
        for i, v in enumerate(c):
            wetsig[i] += v * 0.25
    for ms in _AP_MS:
        d = max(1, int(round(sr * ms / 1000.0)))
        wetsig = allpass(wetsig, d, 0.5)
    dry = pad_to(x, n)
    return [dry[i] * (1.0 - wet) + wetsig[i] * wet for i in range(n)]


def crossfade_loop(mono_or_stereo_chan, fade_seconds, declick_ms=6.0):
    """Boucle INVISIBLE : la queue est mélangée dans la tête (equal-power crossfade),
    le tout raccourci de `fade_seconds` — le fichier résultant boucle sans clic quand
    Godot le rejoue en AudioStreamWAV loop=true (fin -> début continu). Un DÉCLIC final
    (`declick_ms`) force en plus la toute fin de buffer à converger vers l'échantillon
    de tête (rampe linéaire sur les derniers ms) — le crossfade equal-power seul ne
    garantit pas une continuité PONCTUELLE sur un bruit non périodique (mesuré : la
    couture pouvait encore sauter de ~0.1-0.24 sur amb_sea/amb_wind/drone_hiver ; ce
    second étage la ramène sous le seuil sur TOUS les cas testés)."""
    nfade = n_samples(fade_seconds)
    n = len(mono_or_stereo_chan)
    if nfade * 2 >= n:
        nfade = max(1, n // 4)
    head = mono_or_stereo_chan[:nfade]
    tail = mono_or_stereo_chan[n - nfade:]
    body = mono_or_stereo_chan[nfade:n - nfade]
    out_head = [0.0] * nfade
    for i in range(nfade):
        t = i / max(1, nfade - 1)
        fout = math.cos(t * math.pi * 0.5)   # equal-power
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
    """Discontinuité au point de bouclage (fin→début) — delta d'échantillon, normalisé
    à l'amplitude du signal. Doit rester petit (< ~0.08) pour une boucle inaudible."""
    if len(chan) < 2:
        return 0.0
    last = chan[-1]
    first = chan[0]
    amp = max(1e-6, max(abs(v) for v in chan[-256:] + chan[:256]))
    return abs(last - first) / amp


def downsample_wave(wv: Wave, src_sr, dst_sr, loop=False):
    """Décime après anti-repliement (passe-bas 1 pôle sous Nyquist cible) — utilisé
    pour les fichiers d'ambiance (16 kHz) et de moment (22.05 kHz) afin de tenir le
    budget de poids du dépôt (~20 sons, < 4 Mo au total).
    ⚠ `loop=True` (ambiances bouclées) : le filtre anti-repliement est PRÉ-CHAUFFÉ
    avec la QUEUE du canal (préfixe jeté ensuite) puis la couture est re-déclickée —
    sans ça, l'état zéro du filtre au 1er échantillon ROUVRE la couture de boucle
    que crossfade_loop avait fermée (mesuré : seam 0.05→0.34 selon le son)."""
    if dst_sr >= src_sr:
        return wv
    ratio = src_sr / dst_sr
    cutoff = dst_sr * 0.45
    warm = 8192 if loop else 0
    out_ch = []
    for c in wv.ch:
        src = (c[-warm:] + c) if warm else c
        filtered = onepole_lowpass(src, cutoff, sr=src_sr)
        if warm:
            filtered = filtered[warm:]
        n_out = int(len(filtered) / ratio)
        resampled = [0.0] * n_out
        for i in range(n_out):
            src_pos = i * ratio
            i0 = int(src_pos)
            frac = src_pos - i0
            i1 = min(i0 + 1, len(filtered) - 1)
            resampled[i] = filtered[i0] * (1.0 - frac) + filtered[i1] * frac
        if loop and len(resampled) > 128:
            # re-déclic post-resample : rampe des ~6 derniers ms vers l'échantillon 0
            ndc = max(2, int(dst_sr * 0.006))
            delta = resampled[0] - resampled[-1]
            base = len(resampled) - ndc
            for i in range(ndc):
                t = (i + 1) / ndc
                resampled[base + i] += delta * t
        out_ch.append(resampled)
    return Wave(out_ch)


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
    """LE TICK — un « tock » de bois mat très doux : Karplus-Strong grave amorti
    (pas une corde qui sonne, un coup MAT) + un souffle très bref. Discret,
    écoutable des heures : pas de harmonique perçante, decay rapide."""
    rng = random.Random(seed)
    dur = 0.11
    n = n_samples(dur)
    freq = 148.0
    body = karplus_strong(dur, freq, rng, damping=0.985, brightness=0.12)
    body = onepole_lowpass(body, 2600.0)
    env = adsr(n, SR, a=0.001, d=0.05, s_level=0.0, s_dur=0.0, r=0.03, curve=1.2)
    body = apply_env(body, env)
    breath = white_noise(0.02, rng)
    benv = adsr(len(breath), SR, a=0.0005, d=0.015, s_level=0.0, s_dur=0.0, r=0.004, curve=1.0)
    breath = apply_env(onepole_lowpass(breath, 3500.0), benv)
    breath = pad_to(breath, n)
    sig = mix(body, [b * 0.18 for b in breath])
    sig = soft_saturate(sig, 1.2)
    wv = Wave.mono(pad_to(sig, n)).normalize_to(-16.0)
    return wv


def snd_ui_tick_year(seed):
    """Variante an-nouveau : le tock + une harmonique de cloche LOINTAINE (petite FM,
    decay long mais discret — reste dans le même registre feutré)."""
    rng = random.Random(seed)
    tick = snd_ui_tick(seed).ch[0]
    dur = 0.9
    n = n_samples(dur)
    bell = fm_bell_partial(dur, 660.0, rng, decay=2.6, mod_index=0.6, mod_ratio=2.756)
    benv = adsr(n, SR, a=0.004, d=0.15, s_level=0.35, s_dur=0.05, r=0.6, curve=1.6)
    bell = apply_env(bell, benv)
    bell = onepole_lowpass(bell, 3200.0)
    sig = mix(pad_to(tick, n), [b * 0.10 for b in bell])
    sig = schroeder_reverb(sig, wet=0.16, room=0.7)
    wv = Wave.mono(pad_to(sig, n)).normalize_to(-16.0)
    return wv


def snd_ui_parchment_open(seed):
    return _parchment_rustle(seed, dur=0.55, rising=True)


def snd_ui_parchment_close(seed):
    return _parchment_rustle(seed, dur=0.45, rising=False)


def _parchment_rustle(seed, dur, rising):
    """Froissement de papier : bruit haute-fréquence filtré à enveloppe IRRÉGULIÈRE
    (plusieurs micro-bosses aléatoires, pas une seule enveloppe lisse) — c'est ce qui
    fait « papier » plutôt que « souffle »."""
    rng = random.Random(seed)
    n = n_samples(dur)
    base = white_noise(dur, rng)
    hp = onepole_highpass(base, 1800.0)
    bp = biquad_bandpass(hp, 5200.0, 0.9)
    sig = mix([h * 0.55 for h in hp], [b * 0.55 for b in bp])
    # enveloppe irrégulière : somme de petites bosses aléatoires (crépitement du papier)
    env = [0.0] * n
    n_bumps = 14
    for _ in range(n_bumps):
        center = rng.uniform(0.05, 0.95) * n
        width = rng.uniform(0.008, 0.05) * n
        amp = rng.uniform(0.3, 1.0)
        for i in range(max(0, int(center - width * 3)), min(n, int(center + width * 3))):
            d = (i - center) / max(1.0, width)
            env[i] += amp * math.exp(-0.5 * d * d)
    mx = max(env) if env else 1.0
    env = [e / mx for e in env]
    overall = adsr(n, SR, a=0.02 if rising else 0.005, d=0.05, s_level=0.7, s_dur=dur * 0.4,
                   r=dur * 0.35, curve=1.3)
    for i in range(n):
        env[i] *= overall[i]
    sig = apply_env(pad_to(sig, n), env)
    wv = Wave.mono(sig).normalize_to(-17.0)
    return wv


def snd_ui_quill(seed):
    """Gratte de plume : 3 courtes variantes de bruit filtré à grain fin, concaténées
    (le fichier livré est UNE piste avec les 3 gratouillis séquencés)."""
    rng = random.Random(seed)
    parts = []
    for k in range(3):
        dur = rng.uniform(0.09, 0.16)
        n = n_samples(dur)
        base = white_noise(dur, rng)
        hp = onepole_highpass(base, 3200.0 + k * 400)
        bp = biquad_bandpass(hp, 6500.0 + k * 500, 3.0)
        sig = mix([h * 0.4 for h in hp], [b * 0.7 for b in bp])
        # micro-grain : plusieurs pics très courts (le crissement de la pointe)
        env = [0.0] * n
        n_bumps = int(dur * 90)
        for _ in range(n_bumps):
            center = rng.uniform(0.0, 1.0) * n
            width = rng.uniform(0.002, 0.01) * n
            amp = rng.uniform(0.2, 1.0)
            for i in range(max(0, int(center - width * 3)), min(n, int(center + width * 3))):
                d = (i - center) / max(1.0, width)
                env[i] += amp * math.exp(-0.5 * d * d)
        mx = max(env) if env and max(env) > 0 else 1.0
        env = [e / mx for e in env]
        sig = apply_env(sig, env)
        parts.append(sig)
        parts.append(silence(0.05))
    sig = []
    for p in parts:
        sig += p
    wv = Wave.mono(sig).normalize_to(-18.0)
    return wv


def snd_ui_seal(seed):
    """Cachet de cire : impact sourd (percussion grave, bruit filtré très bas +
    Karplus mat) suivi d'un squish (bruit filtré, enveloppe molle qui s'étale)."""
    rng = random.Random(seed)
    dur = 0.42
    n = n_samples(dur)
    thump_noise = white_noise(0.05, rng)
    thump = onepole_lowpass(thump_noise, 220.0)
    thump_env = adsr(len(thump), SR, a=0.001, d=0.04, s_level=0.0, s_dur=0.0, r=0.03, curve=1.0)
    thump = apply_env(thump, thump_env)
    wood = karplus_strong(0.08, 90.0, rng, damping=0.97, brightness=0.05)
    wood_env = adsr(len(wood), SR, a=0.001, d=0.05, s_level=0.0, s_dur=0.0, r=0.02, curve=1.0)
    wood = apply_env(wood, wood_env)
    squish_noise = white_noise(dur, rng)
    squish = onepole_lowpass(squish_noise, 900.0)
    squish = onepole_highpass(squish, 120.0)
    squish_env = adsr(n, SR, a=0.03, d=0.1, s_level=0.25, s_dur=0.12, r=0.18, curve=1.8)
    squish = apply_env(pad_to(squish, n), squish_env)
    sig = mix(pad_to(thump, n), [w * 0.8 for w in pad_to(wood, n)], [s * 0.5 for s in squish])
    sig = soft_saturate(sig, 1.4)
    wv = Wave.mono(sig).normalize_to(-15.0)
    return wv


def snd_ui_deny(seed):
    """Tampon sec — impact bois mat, PAS un buzzer : Karplus-Strong très amorti +
    bref souffle. Court, net, un peu plus dur que ui_tick (le refus)."""
    rng = random.Random(seed)
    dur = 0.16
    n = n_samples(dur)
    body1 = karplus_strong(dur, 110.0, rng, damping=0.975, brightness=0.08)
    body2 = karplus_strong(dur, 118.0, rng, damping=0.975, brightness=0.08)
    body = mix([b * 0.7 for b in body1], [b * 0.5 for b in body2])
    body = onepole_lowpass(body, 1800.0)
    env = adsr(n, SR, a=0.0008, d=0.05, s_level=0.0, s_dur=0.0, r=0.05, curve=1.0)
    body = apply_env(pad_to(body, n), env)
    thud_noise = white_noise(0.04, rng)
    thud = onepole_lowpass(thud_noise, 400.0)
    thud_env = adsr(len(thud), SR, a=0.0005, d=0.03, s_level=0.0, s_dur=0.0, r=0.02, curve=1.0)
    thud = apply_env(thud, thud_env)
    sig = mix(body, [t * 0.5 for t in pad_to(thud, n)])
    sig = soft_saturate(sig, 1.5)
    wv = Wave.mono(sig).normalize_to(-14.0)
    return wv


def fm_bell_partial(dur, base_freq, rng, decay=1.5, mod_index=1.4, mod_ratio=1.4):
    """Un partiel de cloche/métal FM simple : porteuse modulée, decay exponentiel."""
    n = n_samples(dur)
    out = [0.0] * n
    mod_freq = base_freq * mod_ratio
    phase_c = 0.0
    phase_m = 0.0
    dphc = 2.0 * math.pi * base_freq / SR
    dphm = 2.0 * math.pi * mod_freq / SR
    for i in range(n):
        t = i / SR
        env = math.exp(-decay * t)
        idx = mod_index * env
        s = math.sin(phase_c + idx * math.sin(phase_m))
        out[i] = s * env
        phase_c += dphc
        phase_m += dphm
    return out


def snd_ui_coin(seed):
    """2-3 pièces : métal FM inharmonique bref, plusieurs impacts décalés."""
    rng = random.Random(seed)
    n_coins = rng.randint(2, 3)
    total_dur = 0.05 + n_coins * 0.09
    n = n_samples(total_dur)
    sig = [0.0] * n
    for k in range(n_coins):
        t0 = k * rng.uniform(0.05, 0.09)
        dur = 0.22
        base = rng.uniform(1450.0, 2200.0)
        partial1 = fm_bell_partial(dur, base, rng, decay=22.0, mod_index=2.2, mod_ratio=2.41)
        partial2 = fm_bell_partial(dur, base * 1.62, rng, decay=28.0, mod_index=1.6, mod_ratio=1.19)
        p = mix([a * 0.7 for a in partial1], [a * 0.4 for a in partial2])
        p = onepole_highpass(p, 700.0)
        off = n_samples(t0)
        for i, v in enumerate(p):
            if off + i < n:
                sig[off + i] += v
    sig = soft_saturate(sig, 1.1)
    wv = Wave.mono(sig).normalize_to(-16.0)
    return wv


def snd_ui_scroll_tick(seed):
    """Cliquet léger de déroulant : micro-clic bois + souffle très bref."""
    rng = random.Random(seed)
    dur = 0.045
    n = n_samples(dur)
    click = karplus_strong(dur, 900.0, rng, damping=0.95, brightness=0.2)
    click = onepole_bandish(click)
    env = adsr(n, SR, a=0.0003, d=0.02, s_level=0.0, s_dur=0.0, r=0.01, curve=1.0)
    click = apply_env(pad_to(click, n), env)
    wv = Wave.mono(click).normalize_to(-19.0)
    return wv


def onepole_bandish(x):
    hp = onepole_highpass(x, 600.0)
    return onepole_lowpass(hp, 5000.0)


# ───────────────────────── recettes MOMENTS ─────────────────────────

def snd_moment_page_turn(seed):
    """LE swoosh papier de la page qui se tourne : bruit balayé (passe-bande montant
    puis redescendant) + froissement — 1.2 s, calé sur l'anim existante (rise ~0.8s,
    turn ~1.2s dans page_turn.gd)."""
    rng = random.Random(seed)
    dur = 1.2
    n = n_samples(dur)
    noise = white_noise(dur, rng)
    swoosh = bandpass_sweep(noise, 300.0, 4200.0)
    # bosse d'enveloppe : monte puis redescend (le geste physique du feuillet)
    env = [0.0] * n
    for i in range(n):
        t = i / n
        env[i] = math.sin(math.pi * t) ** 0.7
    swoosh = apply_env(swoosh, env)
    rustle = pink_noise(dur, rng)
    rustle = onepole_highpass(rustle, 2200.0)
    # enveloppe irrégulière superposée (crépitement du papier pendant le geste)
    renv = [0.0] * n
    for _ in range(24):
        center = rng.uniform(0.05, 0.95) * n
        width = rng.uniform(0.01, 0.04) * n
        amp = rng.uniform(0.3, 1.0)
        for i in range(max(0, int(center - width * 3)), min(n, int(center + width * 3))):
            d = (i - center) / max(1.0, width)
            renv[i] += amp * math.exp(-0.5 * d * d)
    mxr = max(renv) if renv and max(renv) > 0 else 1.0
    renv = [e / mxr * env[i] for i, e in enumerate(renv)]
    rustle = apply_env(rustle, renv)
    sig = mix([s * 0.65 for s in swoosh], [r * 0.55 for r in rustle])
    sig = schroeder_reverb(sig, wet=0.12, room=0.55)
    wv = Wave.mono(pad_to(sig, n)).normalize_to(-16.0)
    return wv


def snd_moment_age_bell(seed):
    """Cloche grave FM, partiels DÉSACCORDÉS, longue queue réverb — l'avènement."""
    rng = random.Random(seed)
    dur = 4.5
    n = n_samples(dur)
    base = 110.0
    # partiels inharmoniques typiques d'une cloche (ratios non entiers, désaccordés)
    ratios = [1.0, 1.997, 2.411, 3.01, 3.997, 5.43]
    decays = [0.55, 0.7, 0.95, 1.3, 1.7, 2.1]
    amps = [1.0, 0.55, 0.42, 0.30, 0.20, 0.14]
    sig = [0.0] * n
    for r, dcy, a in zip(ratios, decays, amps):
        p = fm_bell_partial(dur, base * r, rng, decay=dcy, mod_index=1.1, mod_ratio=1.41)
        for i in range(n):
            sig[i] += p[i] * a
    strike_noise = white_noise(0.05, rng)
    strike = onepole_lowpass(strike_noise, 900.0)
    strike_env = adsr(len(strike), SR, a=0.0005, d=0.04, s_level=0.0, s_dur=0.0, r=0.02, curve=1.0)
    strike = apply_env(strike, strike_env)
    for i, v in enumerate(strike):
        if i < n:
            sig[i] += v * 0.5
    sig = soft_saturate(sig, 1.05)
    sig = schroeder_reverb(sig, wet=0.38, room=0.85, damp=0.3, predelay_ms=15.0)
    wv = Wave.mono(pad_to(sig, n)).normalize_to(-15.0)
    return wv


def snd_moment_war_horn(seed):
    """Corne lointaine : sciage filtré (dents de scie basses) + formants (bandpass
    fixes qui imitent le pavillon d'un cor) + réverb longue (lointaine)."""
    rng = random.Random(seed)
    dur = 2.6
    n = n_samples(dur)
    freq = 98.0
    saw = _sawtooth(dur, freq)
    # léger vibrato/instabilité (souffle humain)
    saw2 = _sawtooth(dur, freq * 1.006)
    sig = mix([s * 0.6 for s in saw], [s * 0.4 for s in saw2])
    sig = biquad_bandpass(sig, 220.0, 1.4)
    f2 = biquad_bandpass(mix([s * 0.6 for s in saw], [s * 0.4 for s in saw2]), 640.0, 2.2)
    sig = mix(sig, [f * 0.35 for f in f2])
    env = adsr(n, SR, a=0.25, d=0.3, s_level=0.75, s_dur=dur * 0.45, r=dur * 0.35, curve=1.4)
    sig = apply_env(pad_to(sig, n), env)
    sig = soft_saturate(sig, 1.3)
    sig = schroeder_reverb(sig, wet=0.45, room=0.88, damp=0.4, predelay_ms=25.0)
    sig = onepole_lowpass(sig, 2600.0)   # lointain : on écrête le haut-médium
    wv = Wave.mono(pad_to(sig, n)).normalize_to(-16.0)
    return wv


def _sawtooth(dur, freq, sr=SR):
    n = n_samples(dur)
    out = [0.0] * n
    phase = 0.0
    dphase = freq / sr
    for i in range(n):
        out[i] = 2.0 * (phase - math.floor(phase + 0.5))
        phase += dphase
    return out


def snd_moment_battle_drums(seed):
    """Tambours graves ×3 coups : peau = bruit passe-bas percuté (enveloppe percussive
    par coup, léger décalage/variation entre les 3 pour le naturel)."""
    rng = random.Random(seed)
    hits_t = [0.0, 0.42, 0.80]
    dur = 1.55
    n = n_samples(dur)
    sig = [0.0] * n
    for k, t0 in enumerate(hits_t):
        hdur = 0.5
        noise = white_noise(hdur, rng)
        skin = onepole_lowpass(noise, 140.0 + rng.uniform(-10, 10))
        skin = onepole_highpass(skin, 40.0)
        henv = adsr(len(skin), SR, a=0.001, d=0.09, s_level=0.05, s_dur=0.05, r=0.28, curve=1.7)
        skin = apply_env(skin, henv)
        # un peu de Karplus grave pour le "corps" de la caisse
        body = karplus_strong(hdur, 62.0 + k * 3, rng, damping=0.965, brightness=0.05)
        benv = adsr(len(body), SR, a=0.001, d=0.12, s_level=0.0, s_dur=0.0, r=0.2, curve=1.5)
        body = apply_env(body, benv)
        hit = mix([s * 0.85 for s in skin], [b * 0.55 for b in body])
        off = n_samples(t0)
        for i, v in enumerate(hit):
            if off + i < n:
                sig[off + i] += v * (1.0 - 0.08 * k)
    sig = soft_saturate(sig, 1.4)
    sig = schroeder_reverb(sig, wet=0.22, room=0.7, damp=0.35)
    wv = Wave.mono(pad_to(sig, n)).normalize_to(-15.0)
    return wv


def snd_moment_treason(seed):
    """Corde grave pincée qui CASSE + silence — la trahison. Karplus grave amorti qui
    est brutalement coupé (le "snap") suivi d'un bref bruit de rupture puis silence net."""
    rng = random.Random(seed)
    dur_before = 0.9
    dur_snap = 0.12
    dur_after = 0.35
    string = karplus_strong(dur_before, 72.0, rng, damping=0.997, brightness=0.35)
    senv = adsr(len(string), SR, a=0.01, d=0.2, s_level=0.55, s_dur=dur_before * 0.4, r=0.15, curve=1.3)
    string = apply_env(string, senv)
    snap_noise = white_noise(dur_snap, rng)
    snap = onepole_highpass(snap_noise, 1800.0)
    snap_env = adsr(len(snap), SR, a=0.0005, d=0.015, s_level=0.0, s_dur=0.0, r=0.06, curve=1.0)
    snap = apply_env(snap, snap_env)
    # queue résiduelle très courte puis silence complet (pas de réverb qui "adoucit")
    tail_noise = white_noise(dur_after, rng)
    tail = onepole_lowpass(tail_noise, 500.0)
    tail_env = adsr(len(tail), SR, a=0.0, d=0.05, s_level=0.0, s_dur=0.0, r=0.25, curve=2.2)
    tail = apply_env(tail, tail_env)
    sig = string + [s * 1.1 for s in snap] + [t * 0.22 for t in tail]
    sig = soft_saturate(sig, 1.15)
    wv = Wave.mono(sig).normalize_to(-16.0)
    return wv


def snd_moment_ascension(seed):
    """Accord additif cristallin ascendant, 4 s — pinceau de partiels purs (sinus)
    montant en fréquence + en registre, chœur d'harmoniques propres (PAS désaccordées,
    contraste voulu avec les cloches/métal des autres moments : ici c'est PUR)."""
    rng = random.Random(seed)
    dur = 4.0
    n = n_samples(dur)
    # accord majeur qui monte d'une octave sur toute la durée (arpège continu ascendant)
    base_freqs = [220.0, 277.18, 329.63, 440.0, 554.37]  # A3 chord-ish, quinte au-dessus incluse
    sig = [0.0] * n
    for k, f0 in enumerate(base_freqs):
        phase = 0.0
        delay = k * 0.12
        amp_env = adsr(n, SR, a=0.3 + delay, d=0.4, s_level=0.8, s_dur=dur * 0.5, r=dur * 0.3, curve=1.2)
        partial = [0.0] * n
        for i in range(n):
            t = i / SR
            glide = 1.0 + 1.0 * min(1.0, max(0.0, (t - delay) / max(0.1, dur - delay)))
            f = f0 * glide
            phase += 2.0 * math.pi * f / SR
            partial[i] = math.sin(phase) * amp_env[i]
        for i in range(n):
            sig[i] += partial[i] * (0.5 - 0.06 * k)
    shimmer = pink_noise(dur, rng)
    shimmer = onepole_highpass(shimmer, 6000.0)
    shenv = adsr(n, SR, a=1.0, d=0.5, s_level=0.5, s_dur=dur * 0.4, r=dur * 0.4, curve=1.4)
    shimmer = apply_env(pad_to(shimmer, n), shenv)
    sig = mix(sig, [s * 0.08 for s in shimmer])
    sig = schroeder_reverb(sig, wet=0.42, room=0.9, damp=0.15, predelay_ms=20.0)
    wv = Wave.mono(pad_to(sig, n)).normalize_to(-15.0)
    return wv


# ───────────────────────── recettes AMBIANCES (stéréo, bouclées) ─────────────────────────

def _decorrelated_stereo(mono_l_seed_fn, dur, rng_l, rng_r):
    left = mono_l_seed_fn(dur, rng_l)
    right = mono_l_seed_fn(dur, rng_r)
    return left, right


def snd_amb_wind(seed):
    """Vent : bruit rose balayé lent, stéréo DÉCORRÉLÉE (deux générations indépendantes,
    seeds différents) — largeur stéréo naturelle sans phase-cancel au mono-sum."""
    dur = 6.5
    fade = 1.1
    rng_l = random.Random(seed * 2 + 1)
    rng_r = random.Random(seed * 2 + 2)
    left = pink_noise(dur, rng_l)
    right = pink_noise(dur, rng_r)

    def gust_filter(x, rng):
        n = len(x)
        # LFO lent qui module le cutoff (rafales) — période irrégulière (somme de sinus)
        out_cut = []
        for i in range(n):
            t = i / SR
            c = 700.0 + 500.0 * (0.5 + 0.5 * math.sin(2 * math.pi * 0.07 * t + 1.3)) \
                + 260.0 * (0.5 + 0.5 * math.sin(2 * math.pi * 0.021 * t + 0.4))
            out_cut.append(c)
        return onepole_lowpass(x, lambda i: out_cut[i])

    left = gust_filter(left, rng_l)
    right = gust_filter(right, rng_r)
    left = onepole_highpass(left, 60.0)
    right = onepole_highpass(right, 60.0)
    left = crossfade_loop(left, fade)
    right = crossfade_loop(right, fade)
    wv = Wave.stereo(left, right).normalize_to(-22.0)
    return wv


def snd_amb_sea(seed):
    """Ressac : vagues = enveloppes LENTES et irrégulières sur bruit filtré, stéréo
    décorrélée."""
    dur = 6.5
    fade = 1.1
    rng_l = random.Random(seed * 2 + 11)
    rng_r = random.Random(seed * 2 + 12)

    def make(rng):
        base = white_noise(dur, rng)
        surf = onepole_lowpass(base, 1400.0)
        surf = onepole_highpass(surf, 90.0)
        n = len(surf)
        # vagues : somme de bosses lentes à espacement quasi-périodique mais JITTÉ
        env = [0.15] * n
        t = 0.0
        while t < dur:
            period = rng.uniform(3.2, 5.4)
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
        return apply_env(surf, env)

    left = make(rng_l)
    right = make(rng_r)
    left = crossfade_loop(left, fade)
    right = crossfade_loop(right, fade)
    wv = Wave.stereo(left, right).normalize_to(-22.0)
    return wv


def snd_amb_crowd(seed):
    """Rumeur de foule : voix = bruit à FORMANTS multiples superposés (plusieurs
    bandpass à des fréquences vocales typiques, modulés lentement et indépendamment
    par petite voix) → indistinct, jamais un mot reconnaissable."""
    dur = 6.5
    fade = 1.0
    rng_l = random.Random(seed * 2 + 21)
    rng_r = random.Random(seed * 2 + 22)
    formants = [(700, 1.5), (1200, 1.8), (2400, 1.3), (350, 2.0)]

    def make(rng, n_voices=7):
        n = n_samples(dur)
        sig = [0.0] * n
        for v in range(n_voices):
            src = white_noise(dur, rng)
            voice = [0.0] * n
            for f0, q in formants:
                jitter = rng.uniform(0.85, 1.15)
                bp = biquad_bandpass(src, f0 * jitter, q)
                for i in range(n):
                    voice[i] += bp[i] * 0.28
            # enveloppe de babil : LFO irrégulier par voix (parole intermittente)
            lfo_speed = rng.uniform(2.2, 4.2)
            phase0 = rng.uniform(0, 6.28)
            for i in range(n):
                t = i / SR
                m = 0.5 + 0.5 * math.sin(2 * math.pi * lfo_speed * t + phase0)
                m = m ** 1.6
                sig[i] += voice[i] * (0.3 + 0.7 * m) * (1.0 / n_voices)
        return sig

    left = make(rng_l)
    right = make(rng_r)
    left = onepole_lowpass(left, 3400.0)
    right = onepole_lowpass(right, 3400.0)
    left = crossfade_loop(left, fade)
    right = crossfade_loop(right, fade)
    wv = Wave.stereo(left, right).normalize_to(-22.0)
    return wv


def snd_amb_entropy(seed):
    """Drone d'inquiétude : basses BATTANTES (deux sinus très proches en fréquence →
    battement lent) + bruit granuleux — le volume est piloté côté jeu (entropie), le
    fichier lui-même reste à niveau constant."""
    dur = 6.5
    fade = 1.1
    n = n_samples(dur)
    rng_l = random.Random(seed * 2 + 31)
    rng_r = random.Random(seed * 2 + 32)

    def make(rng, f0):
        sig = [0.0] * n
        f1 = f0
        f2 = f0 * 1.013   # battement lent (~0.8 Hz d'écart perçu à cette fréquence)
        p1 = p2 = 0.0
        for i in range(n):
            p1 += 2 * math.pi * f1 / SR
            p2 += 2 * math.pi * f2 / SR
            sig[i] = 0.5 * math.sin(p1) + 0.5 * math.sin(p2)
        grain = white_noise(dur, rng)
        grain = onepole_bandish(grain)
        grain = onepole_lowpass(grain, 900.0)
        return mix([s * 0.7 for s in sig], [g * 0.35 for g in grain])

    left = make(rng_l, 55.0)
    right = make(rng_r, 55.3)
    left = crossfade_loop(left, fade)
    right = crossfade_loop(right, fade)
    wv = Wave.stereo(left, right)
    wv = soft_saturate_wave(wv, 1.1)
    wv.normalize_to(-22.0)
    return wv


def soft_saturate_wave(wv, drive):
    return Wave([soft_saturate(c, drive) for c in wv.ch])


def snd_drone_ronces(seed):
    """Craquements de bois lents (Karplus très grave, ré-excité de temps en temps) +
    nappe sombre (bruit filtré très bas, statique)."""
    dur = 6.5
    fade = 1.1
    n = n_samples(dur)
    rng_l = random.Random(seed * 2 + 41)
    rng_r = random.Random(seed * 2 + 42)

    def make(rng):
        nappe = pink_noise(dur, rng)
        nappe = onepole_lowpass(nappe, 220.0)
        sig = [v * 0.5 for v in nappe]
        # craquements espacés de façon irrégulière
        t = 0.0
        while t < dur - 0.5:
            t += rng.uniform(0.7, 2.3)
            if t >= dur - 0.3:
                break
            crk = karplus_strong(0.3, rng.uniform(60, 110), rng, damping=0.965, brightness=0.05)
            cenv = adsr(len(crk), SR, a=0.001, d=0.1, s_level=0.0, s_dur=0.0, r=0.15, curve=1.4)
            crk = apply_env(crk, cenv)
            off = n_samples(t)
            for i, v in enumerate(crk):
                if off + i < n:
                    sig[off + i] += v * rng.uniform(0.3, 0.7)
        return sig

    left = make(rng_l)
    right = make(rng_r)
    left = crossfade_loop(left, fade)
    right = crossfade_loop(right, fade)
    wv = Wave.stereo(left, right).normalize_to(-22.0)
    return wv


def snd_drone_hiver(seed):
    """Nappe cristalline (partiels hauts, purs, très étirés) + vent fin (bruit filtré
    aigu et discret)."""
    dur = 6.5
    fade = 1.1
    n = n_samples(dur)
    rng_l = random.Random(seed * 2 + 51)
    rng_r = random.Random(seed * 2 + 52)

    def make(rng, base):
        sig = [0.0] * n
        ratios = [1.0, 2.01, 3.03, 4.98]
        for r in ratios:
            p = 0.0
            f = base * r
            amp = 1.0 / (r * 1.4)
            for i in range(n):
                p += 2 * math.pi * f / SR
                sig[i] += math.sin(p) * amp
        wind = white_noise(dur, rng)
        wind = onepole_highpass(wind, 5200.0)
        wind = onepole_lowpass(wind, 9000.0)
        return mix([s * 0.22 for s in sig], [w * 0.18 for w in wind])

    left = make(rng_l, 330.0)
    right = make(rng_r, 331.5)
    left = crossfade_loop(left, fade)
    right = crossfade_loop(right, fade)
    wv = Wave.stereo(left, right).normalize_to(-22.0)
    return wv


def snd_drone_eau(seed):
    """Basses liquides (bruit filtré très bas, modulé lentement) + gouttes réverbérées
    (impacts courts espacés aléatoirement, chacun passé dans la réverb Schroeder)."""
    dur = 6.5
    fade = 1.1
    n = n_samples(dur)
    rng_l = random.Random(seed * 2 + 61)
    rng_r = random.Random(seed * 2 + 62)

    def make(rng):
        base = pink_noise(dur, rng)
        liquid = onepole_lowpass(base, 260.0)
        # modulation lente du volume (houle profonde)
        for i in range(n):
            t = i / SR
            m = 0.6 + 0.4 * math.sin(2 * math.pi * 0.05 * t)
            liquid[i] *= m
        sig = [v * 0.6 for v in liquid]
        t = 0.0
        while t < dur - 0.3:
            t += rng.uniform(0.4, 1.6)
            if t >= dur - 0.2:
                break
            drop = fm_bell_partial(0.25, rng.uniform(900, 1700), rng, decay=18.0, mod_index=0.8, mod_ratio=3.1)
            drop = schroeder_reverb(drop, wet=0.5, room=0.6, damp=0.4)
            off = n_samples(t)
            for i, v in enumerate(drop):
                if off + i < n:
                    sig[off + i] += v * rng.uniform(0.15, 0.35)
        return sig

    left = make(rng_l)
    right = make(rng_r)
    left = crossfade_loop(left, fade)
    right = crossfade_loop(right, fade)
    wv = Wave.stereo(left, right).normalize_to(-22.0)
    return wv


def snd_drone_sang(seed):
    """Tambours étouffés très lents (Karplus/bruit passe-bas, très amortis, espacés
    comme un pouls) + souffle (bruit rose très filtré, statique, sombre)."""
    dur = 6.5
    fade = 1.1
    n = n_samples(dur)
    rng_l = random.Random(seed * 2 + 71)
    rng_r = random.Random(seed * 2 + 72)

    def make(rng):
        breath = pink_noise(dur, rng)
        breath = onepole_lowpass(breath, 180.0)
        sig = [v * 0.45 for v in breath]
        t = 0.4
        period = 1.15   # pouls lent régulier (étouffé)
        while t < dur - 0.3:
            noise = white_noise(0.35, rng)
            skin = onepole_lowpass(noise, 90.0)
            henv = adsr(len(skin), SR, a=0.002, d=0.15, s_level=0.0, s_dur=0.0, r=0.2, curve=1.8)
            skin = apply_env(skin, henv)
            off = n_samples(t)
            for i, v in enumerate(skin):
                if off + i < n:
                    sig[off + i] += v * 0.55
            t += period + rng.uniform(-0.05, 0.05)
        return sig

    left = make(rng_l)
    right = make(rng_r)
    left = crossfade_loop(left, fade)
    right = crossfade_loop(right, fade)
    wv = Wave.stereo(left, right).normalize_to(-22.0)
    return wv


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

AMBIANCE_NAMES = {n for n in RECIPES if n.startswith("amb_") or n.startswith("drone_")}


AMB_SR = 16000      # ambiances bouclées (bruit/drones graves) : 16 kHz — budget dépôt
MOMENT_SR = 22050   # moments (cloche/corne/tambours, réverb) : 22.05 kHz suffit


def gen_all(out_dir, base_seed, names=None):
    os.makedirs(out_dir, exist_ok=True)
    todo = names if names else list(RECIPES.keys())
    results = {}
    for name in todo:
        fn = RECIPES[name]
        # seed PAR NOM (indépendant de l'ordre/du sous-ensemble --only régénéré)
        seed = base_seed + sum((k + 1) * ord(c) for k, c in enumerate(name))
        wv = fn(seed)
        path = os.path.join(out_dir, name)
        if name in AMBIANCE_NAMES:
            wv = downsample_wave(wv, SR, AMB_SR, loop=True)
            write_wav(path, wv, sr=AMB_SR)
        elif name.startswith("moment_"):
            wv = downsample_wave(wv, SR, MOMENT_SR)
            write_wav(path, wv, sr=MOMENT_SR)
        else:
            write_wav(path, wv)
        results[name] = path
    return results


def zcr_hz(path):
    """Taux de passage par zéro (Hz) — proxy bon marché du CENTRE spectral : deux sons
    au même « spectre » ont un ZCR voisin ; la colonne rend la VARIÉTÉ vérifiable."""
    with wave.open(path, 'rb') as f:
        nch = f.getnchannels()
        n = f.getnframes()
        fr = f.getframerate()
        raw = f.readframes(n)
    a = array.array('h')
    a.frombytes(raw)
    chan0 = [a[i * nch] for i in range(n)]
    crossings = 0
    for i in range(1, n):
        if (chan0[i - 1] < 0) != (chan0[i] < 0):
            crossings += 1
    dur = n / float(fr)
    return crossings / (2.0 * max(0.001, dur))


def check_all(out_dir, names=None):
    todo = names if names else list(RECIPES.keys())
    print("\n%-28s %8s %8s %8s %10s %8s" % ("fichier", "durée(s)", "crête", "RMS", "loop-seam", "ZCR(Hz)"))
    print("-" * 80)
    problems = []
    for name in todo:
        path = os.path.join(out_dir, name)
        if not os.path.exists(path):
            problems.append("%s: MANQUANT" % name)
            continue
        dur, pk, rms, nchan = read_wav_peak_rms(path)
        seam = ""
        seam_val = None
        if name in AMBIANCE_NAMES:
            with wave.open(path, 'rb') as f:
                n = f.getnframes()
                nch = f.getnchannels()
                raw = f.readframes(n)
            a = array.array('h')
            a.frombytes(raw)
            chan0 = [a[i * nch] / 32768.0 for i in range(n)]
            seam_val = loop_seam_score(chan0)
            seam = "%.4f" % seam_val
            if seam_val > 0.08:
                problems.append("%s: loop-seam élevé (%.4f)" % (name, seam_val))
        print("%-28s %8.2f %8.1f %8.1f %10s %8.0f" % (name, dur, lin_to_db(pk), lin_to_db(rms), seam, zcr_hz(path)))
        if pk < 1e-4:
            problems.append("%s: silence (crête ~0)" % name)
        if pk > 1.0:
            problems.append("%s: clip (crête > 0 dBFS)" % name)
        expect_mono = name not in AMBIANCE_NAMES
        if expect_mono and nchan != 1:
            problems.append("%s: attendu mono, trouvé %d canaux" % (name, nchan))
        if not expect_mono and nchan != 2:
            problems.append("%s: attendu stéréo, trouvé %d canaux" % (name, nchan))
    print("-" * 70)
    total_bytes = sum(os.path.getsize(os.path.join(out_dir, n)) for n in todo
                       if os.path.exists(os.path.join(out_dir, n)))
    print("Total : %.2f Mo pour %d fichiers" % (total_bytes / (1024 * 1024), len(todo)))
    if problems:
        print("\n⚠ PROBLÈMES :")
        for p in problems:
            print(" - " + p)
        return False
    print("\nToutes les métriques sont propres.")
    return True


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", default=os.path.join(os.path.dirname(__file__), "..", "godot", "project", "audio"))
    ap.add_argument("--seed", type=int, default=20260706)
    ap.add_argument("--only", nargs="*", default=None, help="ne (re)générer que ces fichiers")
    ap.add_argument("--no-check", action="store_true")
    args = ap.parse_args()

    out_dir = os.path.abspath(args.out)
    names = args.only
    print("Génération dans %s (seed=%d)…" % (out_dir, args.seed))
    gen_all(out_dir, args.seed, names)
    print("Fait : %d fichier(s).\n" % (len(names) if names else len(RECIPES)))
    if not args.no_check:
        ok = check_all(out_dir, names)
        if not ok:
            raise SystemExit(1)


if __name__ == "__main__":
    main()
