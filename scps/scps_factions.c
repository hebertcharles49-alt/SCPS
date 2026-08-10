/*
 * scps_factions.c — LES FACTIONS PAR ÉTHOS (passe 1/N)
 *
 * Le spectre (six factions = axes IA + Communautaire) et son enracinement dans
 * les groupes culturels. Aucune mutation du moteur ici : on LIT les groupes et
 * on en tire un profil de factions. Les passes suivantes feront agir ce profil.
 */
#include "scps_factions.h"
#include <stdio.h>
#include "scps_heritage.h"   /* Heritage */
#include <string.h>         /* memset (reset des stances) */

/* K2 — faction_name() a MIGRÉ au readout (membrane : le moteur n'expose que l'enum). */

float class_clout(SocialClass k){
    /* Qui gouverne compte : l'élite pèse bien plus que la masse laborieuse. */
    switch (k){
        case CLASS_ELITE:    return 3.0f;
        case CLASS_BOURGEOIS:return 1.6f;
        default:             return 1.0f;   /* CLASS_LABORER */
    }
}

/* ══ LE SOCLE DE CLASSE (décision joueur 2026-08-06 : « merge communautaire avec le
 * peuple, marchand avec bourgeois, fanatique avec élite, et ainsi de suite ») ══
 * Le courant d'éthos n'est plus seulement culturel : la POSITION SOCIALE penche —
 * le laboureur vers le bien-commun (sa survie est collective), le bourgeois vers
 * l'échange (sa fortune est le marché), l'élite vers l'orthodoxie et la gloire
 * (son rang est l'ordre établi). La culture MODULE par-dessus (un laboureur
 * clanique reste belliqueux). L'esclave n'a pas de voix (socle nul — sa colère
 * passe par la révolte servile, pas la politique). FAC_CLASS_W pèse le socle. */
void class_ethos_base(SocialClass k, float w[FAC_COUNT]){
    for (int i=0;i<FAC_COUNT;i++) w[i]=0.f;
    switch (k){
        case CLASS_LABORER:   w[FAC_COMMUNAUTAIRE]=1.0f;                          break;
        case CLASS_BOURGEOIS: w[FAC_MARCHAND]=1.0f;                               break;
        case CLASS_ELITE:     w[FAC_GARDIEN]=0.6f; w[FAC_CONQUERANT]=0.4f;        break;
        default: break;                              /* CLASS_SLAVE : sans voix */
    }
}
/* le penchant d'UN membre de classe k d'une culture c : socle + culture, normalisé */
void group_ethos_lean_k(const PopCulture *c, SocialClass k, float w[FAC_COUNT]){
    group_ethos_lean(c, w);                          /* la culture (déjà Σ=1) */
    float base[FAC_COUNT]; class_ethos_base(k, base);
    float cw = tune_f("FAC_CLASS_W", 1.0f);
    float s=0.f;
    for (int i=0;i<FAC_COUNT;i++){ w[i]+=cw*base[i]; s+=w[i]; }
    if (s>0.f) for (int i=0;i<FAC_COUNT;i++) w[i]/=s;
}
void group_ethos_lean(const PopCulture *c, float w[FAC_COUNT]){
    for (int f=0; f<FAC_COUNT; f++) w[f]=0.f;
    if (!c){ w[FAC_COMMUNAUTAIRE]=1.f; return; }

    /* 1) SOCLE — l'éthos de la culture donne la direction première. */
    switch (c->ethos){
        case ETHOS_DOMINATEUR:  w[FAC_CONQUERANT]+=1.0f; w[FAC_TRANSGRESSEUR]+=0.3f; break;
        case ETHOS_HONNEUR:     w[FAC_CONQUERANT]+=0.9f; w[FAC_GARDIEN]+=0.2f;       break; /* gloire + code */
        case ETHOS_ORDRE:       w[FAC_GARDIEN]+=0.7f;    w[FAC_LEGISTE]+=0.4f;       break; /* hiérarchie, tradition */
        case ETHOS_BUREAUCRATE: w[FAC_LEGISTE]+=1.0f;    break;
        case ETHOS_MERCANTILE:  w[FAC_MARCHAND]+=1.0f;   break;
        case ETHOS_PACIFISTE:   w[FAC_COMMUNAUTAIRE]+=1.0f; break;
        default: break;
    }
    /* 2) SIGNATURE de heritage — le penchant inné du peuple (§2).
     * ÉQUILIBRAGE 2026-07-10 (docs/EQUILIBRAGE_CULTURE_FOI_2026-07-10.md §HÉRITAGES) :
     * normalisées à Σ≈0.60 chacune (avant : sommes disparates 0.6-0.9 qui sur-pesaient
     * certains héritages dans le spectre de factions). Adaptatif RENFORCÉ (0.2/0.2→0.3/0.3,
     * désormais la moitié du poids du Clanique, cohérent avec son rôle d'intégrateur). */
    switch (c->heritage){
        case HERITAGE_CLANIQUE:    w[FAC_CONQUERANT]+=0.35f;    w[FAC_TRANSGRESSEUR]+=0.25f; break; /* guerre + interdit */
        case HERITAGE_METALLURGISTE:     w[FAC_LEGISTE]+=0.30f;       w[FAC_TRANSGRESSEUR]+=0.30f; break; /* forge à runes */
        case HERITAGE_AGRAIRE: w[FAC_MARCHAND]+=0.25f;      w[FAC_COMMUNAUTAIRE]+=0.35f; break;
        case HERITAGE_MECANISTE:    w[FAC_MARCHAND]+=0.35f;      w[FAC_COMMUNAUTAIRE]+=0.25f; break; /* négoce, bien commun */
        case HERITAGE_ESOTERIQUE:     w[FAC_TRANSGRESSEUR]+=0.35f; w[FAC_GARDIEN]+=0.25f;       break; /* arcane + tradition */
        case HERITAGE_ADAPTATIF:   w[FAC_MARCHAND]+=0.30f;      w[FAC_LEGISTE]+=0.30f;       break; /* l'intégrateur */
        default: break;
    }
    /* 3) CREDO — la ferveur nourrit les Gardiens ; la tolérance, l'ouverture. */
    if      (c->credo==CREDO_PURIFICATEUR) w[FAC_GARDIEN]+=0.7f;
    else if (c->credo==CREDO_EVANGELISTE)  w[FAC_GARDIEN]+=0.4f;
    else { w[FAC_MARCHAND]+=0.15f; w[FAC_COMMUNAUTAIRE]+=0.15f; }   /* pluraliste : tolère, s'ouvre */

    /* normalise → un PROFIL de penchants (Σ=1). */
    float s=0.f; for (int f=0;f<FAC_COUNT;f++) s+=w[f];
    if (s>0.f) for (int f=0;f<FAC_COUNT;f++) w[f]/=s;
    else w[FAC_COMMUNAUTAIRE]=1.f;
}

/* ---- Agrégation : Σ groupes (pop pondérée par le poids de CLASSE × penchant) --- *
 * Le poids de classe vient désormais de la composition ÉMERGENTE du groupe (§pop
 * précise) : Σ_classe pop_by_class · clout(classe). Une vague de PROMOTIONS (plus de
 * Nobles, clout ×3) PÈSE donc plus lourd — bâtir déplace la politique interne. Repli
 * sur count·clout(klass) si l'émergence n'a pas encore couru. */
static void accumulate(const ProvincePop *pp, double acc[FAC_COUNT]){
    static const SocialClass POLK[3]={CLASS_LABORER,CLASS_BOURGEOIS,CLASS_ELITE};
    for (int i=0; i<pp->n_groups; i++){
        const PopGroup *g=&pp->groups[i];
        if (g->count<=0) continue;
        /* MERGE COURANTS × CLASSES : chaque strate du groupe pèse avec SON penchant
         * (socle de classe + culture) — les laboureurs d'un groupe tirent vers le
         * bien-commun pendant que ses élites tirent vers l'orthodoxie. Une vague de
         * promotions ne change plus seulement le POIDS, elle change la DIRECTION. */
        bool emerged=false;
        for (int kc=0;kc<3;kc++){
            double wgt=(double)g->pop_by_class[POLK[kc]]*(double)class_clout(POLK[kc]);
            if (wgt<=0.0) continue;
            emerged=true;
            float lean[FAC_COUNT]; group_ethos_lean_k(&g->culture, POLK[kc], lean);
            for (int f=0; f<FAC_COUNT; f++) acc[f] += wgt * lean[f];
        }
        if (!emerged){                                            /* repli pré-émergence */
            float lean[FAC_COUNT]; group_ethos_lean_k(&g->culture, g->klass, lean);
            double wgt=(double)g->count*(double)class_clout(g->klass);
            for (int f=0; f<FAC_COUNT; f++) acc[f] += wgt * lean[f];
        }
    }
}

static EthosFaction finalize(double acc[FAC_COUNT], float out[FAC_COUNT]){
    double s=0.0; for (int f=0;f<FAC_COUNT;f++) s+=acc[f];
    int dom=FAC_COMMUNAUTAIRE; double best=-1.0;
    for (int f=0; f<FAC_COUNT; f++){
        out[f] = (s>0.0) ? (float)(acc[f]/s) : (f==FAC_COMMUNAUTAIRE?1.f:0.f);
        if (out[f] > best){ best=out[f]; dom=f; }
    }
    return (EthosFaction)dom;
}

EthosFaction faction_weights_of(const ProvincePop *provs, int n, float out[FAC_COUNT]){
    double acc[FAC_COUNT]={0};
    for (int p=0; p<n; p++) accumulate(&provs[p], acc);
    return finalize(acc, out);
}

/* M2 (design §7) — LE PÔLE D'UNE RÉGION, lu des poids de factions. Transgresseur
 * EXCLU (orthogonal — il nourrit l'appétit faustien, pas la fourche). Tie-breaks
 * §7 dans l'ordre : capitale → pôle impérial · portuaire → Fluide · frontalière →
 * Martial · sinon → Ordre. */
TechPole faction_pole_of(const float wgt[FAC_COUNT], int imperial_pole, bool port, bool border){
    float martial = wgt[FAC_CONQUERANT] + 0.8f*wgt[FAC_GARDIEN];
    float ordre   = wgt[FAC_LEGISTE]    + 0.8f*wgt[FAC_COMMUNAUTAIRE];
    float fluide  = wgt[FAC_MARCHAND];
    const float EPSL = 0.02f;                          /* l'égalité : à deux centièmes près */
    float top = martial; if (ordre>top) top=ordre; if (fluide>top) top=fluide;
    int n_top = (martial>=top-EPSL) + (ordre>=top-EPSL) + (fluide>=top-EPSL);
    if (n_top<=1){
        if (martial>=top) return POLE_MARTIAL;
        if (fluide >=top) return POLE_FLUIDE;
        return POLE_ORDRE;
    }
    if (imperial_pole>=0 && imperial_pole<POLE_COUNT) return (TechPole)imperial_pole;  /* capitale */
    if (port)   return POLE_FLUIDE;
    if (border) return POLE_MARTIAL;
    return POLE_ORDRE;
}

EthosFaction country_faction_weights(const World *w, const WorldEconomy *econ, int cid,
                                     float out[FAC_COUNT]){
    double acc[FAC_COUNT]={0};
    /* RE-KEY PROVINCE : .pop est PROVINCE-OWNED — econ->region[r].pop n'est qu'un miroir de
     * LA SEULE province représentative (capitale, sinon la plus peuplée) de chaque région,
     * pas un agrégat de toute la région. Ce poids alimente TOUT le comportement IA effectif
     * (w_expand/w_trade/w_build/w_faith/w_faustian, cf. faction_effective_weights) : il doit
     * voir TOUTE la diversité du pays (chaque province, chaque groupe), pas seulement un
     * représentant par région — on scanne donc econ->prov[] (pattern a, comme
     * econ_country_metabolized), province par province. */
    if (cid>=0 && econ){
        int nprov=econ->n_prov; if (nprov>SCPS_MAX_PROV) nprov=SCPS_MAX_PROV;
        for (int p=0; p<nprov; p++)
            if (econ->prov[p].owner==cid && econ->prov[p].culture.settled)
                accumulate(&econ->prov[p].pop, acc);
    }
    (void)w;
    return finalize(acc, out);
}

/* ---- L'éthos effectif (§3) : la distribution → les cinq axes w_* ------- */
EthosWeights faction_effective_weights(const float w[FAC_COUNT]){
    EthosWeights e;
    /* Chaque axe = la part de sa faction. Le Communautaire RETIENT les aventures
     * (il bride expand & faustian — le bien-commun contre l'extraction/la guerre). */
    float restraint = w[FAC_COMMUNAUTAIRE];
    e.w_expand   = w[FAC_CONQUERANT]    * (1.f - 0.6f*restraint);
    e.w_trade    = w[FAC_MARCHAND];
    e.w_build    = w[FAC_LEGISTE];
    e.w_faith    = w[FAC_GARDIEN];
    e.w_faustian = w[FAC_TRANSGRESSEUR] * (1.f - 0.6f*restraint);
    return e;
}

/* ---- Cohésion vs fracture de valeurs (§6) ----------------------------- */
float faction_fracture(const float w[FAC_COUNT]){
    /* « Contesté » de la direction : la seconde faction talonne-t-elle la première ?
     * Une tête écrasante → 0 ; deux fortes au coude-à-coude (45/40) → ~1. Pondéré
     * par le poids cumulé des deux têtes (une paralysie de métallurgistes ne paralyse rien). */
    float s1=0.f, s2=0.f;
    for (int f=0; f<FAC_COUNT; f++){
        if (w[f] > s1){ s2=s1; s1=w[f]; }
        else if (w[f] > s2){ s2=w[f]; }
    }
    if (s1 <= 0.f) return 0.f;
    float contested = s2 / s1;            /* 0 (tête seule) .. 1 (au coude-à-coude) */
    float mass      = s1 + s2;            /* la dispute doit peser dans le pays */
    float fr = contested * mass;
    return fr<0.f ? 0.f : (fr>1.f ? 1.f : fr);
}
float faction_cohesion(const float w[FAC_COUNT]){ return 1.f - faction_fracture(w); }

/* ---- Opposition de valeurs & tension de coup (§5) --------------------- */
float faction_opposition(EthosFaction a, EthosFaction b){
    if (a==b) return 0.f;
    /* Table SYMÉTRIQUE des oppositions de valeurs (§1 « Oppose »).
     * Ordre : Conquérant, Marchand, Légiste, Gardien, Transgresseur, Communautaire. */
    static const float O[FAC_COUNT][FAC_COUNT] = {
        /* C */ { 0.f, 0.6f, 0.4f, 0.2f, 0.2f, 1.0f },
        /* M */ { 0.6f, 0.f, 0.2f, 0.9f, 0.5f, 0.3f },
        /* L */ { 0.4f, 0.2f, 0.f, 0.3f, 0.9f, 0.3f },
        /* G */ { 0.2f, 0.9f, 0.3f, 0.f, 1.0f, 0.5f },
        /* T */ { 0.2f, 0.5f, 0.9f, 1.0f, 0.f, 0.9f },
        /* U */ { 1.0f, 0.3f, 0.3f, 0.5f, 0.9f, 0.f },
    };
    if (a<0||a>=FAC_COUNT||b<0||b>=FAC_COUNT) return 0.f;
    return O[a][b];
}

/* P2 — Opp(F) : la faction la plus opposée à F dans la matrice (§5). Même
 * convention de départage que faction_coup_tension : premier maximum en
 * balayage croissant 0..FAC_COUNT-1 (déterministe). */
EthosFaction faction_most_opposed(EthosFaction f){
    if (f<0||f>=FAC_COUNT) return (EthosFaction)(-1);
    int best=-1; float bv=-1.f;
    for (int x=0; x<FAC_COUNT; x++){
        if (x==(int)f) continue;
        float o = faction_opposition(f, (EthosFaction)x);
        if (o>bv){ bv=o; best=x; }
    }
    return (EthosFaction)best;
}

float faction_coup_tension(const float w[FAC_COUNT], EthosFaction *out){
    int dom=0; for (int f=1; f<FAC_COUNT; f++) if (w[f]>w[dom]) dom=f;   /* la direction effective */
    float best=0.f; int bf=dom;
    for (int f=0; f<FAC_COUNT; f++){
        if (f==dom) continue;
        float t = w[f] * faction_opposition((EthosFaction)f, (EthosFaction)dom);  /* fort ET opposé */
        if (t>best){ best=t; bf=f; }
    }
    if (out) *out=(EthosFaction)bf;
    return best;
}

/* ===================================================================== */
/* LES LEVIERS COMME DES VOTES (§4) — stance par pays (état module)        */
/* ===================================================================== */
#define LEVER_BIAS_CAP  0.45f   /* une stance ne renverse pas la démographie, elle l'infléchit */
#define COUP_GRIEF_W    0.25f   /* poids du grief de politique dans la tension de coup (NUDGE, pas flot) */
static float g_lever_bias [SCPS_MAX_COUNTRY][FAC_COUNT];   /* la STANCE de la couronne — reste pays-grain :
                                                            * c'est SA politique, pas une propriété du peuple */
/* ══ GLISSEMENT SUR LES PEUPLES (décision joueur 2026-08-06) ══════════════════════
 * g_lever_grief et g_capture SUPPRIMÉS : la rancœur politique et la capture d'État
 * vivent désormais SUR les PopGroup (ethos_grief / state_grip) — c'est un peuple
 * précis, quelque part, qui rumine ou se gorge, pas une colonne de tableau. Les
 * lecteurs gardent leurs signatures via un BIND de contexte (motif g_tech_cache) :
 * scps_sim pose le monde au tick, les bancs le posent sur leurs fixtures. Sans
 * bind : lecture 0, écriture no-op (permissif, comme econ_country_has_tier). */
static const World  *g_fw = NULL;
static WorldEconomy *g_fe = NULL;
void faction_bind(const World *w, WorldEconomy *e){ g_fw=w; g_fe=e; }
/* le COURANT d'un groupe : son penchant d'éthos dominant */
#define FAC_TINT_MIN 0.15f   /* la teinte minimale pour PORTER un courant (grief/capture) */
/* le penchant d'un groupe — le MÉLANGE PONDÉRÉ de ses classes (pop_by_class ×
 * clout) : les élites d'un groupe paysan portent leur teinte gardienne AU PRORATA —
 * sans ça, le courant d'un ministre-élite n'avait AUCUN porteur dans un pays jeune
 * (mesuré : loyauté figée à 92, rot 0.00 — canal mort). Repli klass pré-émergence. */
static void group_lean_full(const PopGroup *g, float out[FAC_COUNT]){
    static const SocialClass POLK[3]={CLASS_LABORER,CLASS_BOURGEOIS,CLASS_ELITE};
    double acc[FAC_COUNT]={0}; double tot=0.0;
    for (int kc=0;kc<3;kc++){
        double wt=(double)g->pop_by_class[POLK[kc]]*(double)class_clout(POLK[kc]);
        if (wt<=0.0) continue;
        float lk[FAC_COUNT]; group_ethos_lean_k(&g->culture, POLK[kc], lk);
        for (int i=0;i<FAC_COUNT;i++) acc[i]+=wt*lk[i];
        tot+=wt;
    }
    if (tot<=0.0){
        /* PRÉ-ÉMERGENCE : le groupe porte la STRUCTURE SOCIALE NOMINALE (les parts de
         * départ 80/15/5 de CLASS_SHARE, clout-pondérées) — pas une classe unique.
         * Sans ça, un monde jeune n'a AUCUNE teinte d'élite : le courant d'un
         * ministre-élite n'avait aucun porteur (mesuré : ping grievance 0.000,
         * loyauté figée, rot mort). Un peuple a toujours ses notables en germe. */
        static const float NOMINAL[3]={0.80f,0.15f,0.05f};   /* laboureur/bourgeois/élite */
        for (int kc=0;kc<3;kc++){
            /* la classe POSÉE (g->klass) est RENFORCÉE dans la structure nominale : un
             * groupe déclaré « élite » (fixtures, seed spécialisé) reste dominé par sa
             * condition déclarée, sans perdre ses notables/bras en germe. */
            float part = NOMINAL[kc] + ((POLK[kc]==g->klass) ? 0.50f : 0.f);
            double wt=(double)part*(double)class_clout(POLK[kc]);
            float lk[FAC_COUNT]; group_ethos_lean_k(&g->culture, POLK[kc], lk);
            for (int i=0;i<FAC_COUNT;i++) acc[i]+=wt*lk[i];
            tot+=wt;
        }
    }
    for (int i=0;i<FAC_COUNT;i++) out[i]=(float)(acc[i]/tot);
}
static void group_lean_full(const PopGroup *g, float out[FAC_COUNT]);
static EthosFaction group_fac(const PopGroup *g){
    float lean[FAC_COUNT]; group_lean_full(g, lean);   /* le mélange de ses classes */
    int bf=0; for (int k=1;k<FAC_COUNT;k++) if (lean[k]>lean[bf]) bf=k;
    return (EthosFaction)bf;
}
/* itère les groupes d'un pays : cb(groupe, courant, poids-pop) ; poids total rendu */
#define FOR_COUNTRY_GROUPS(cid, G, FAC, BODY) do {     if (g_fe){         int NP_=g_fe->n_prov; if (NP_>SCPS_MAX_PROV) NP_=SCPS_MAX_PROV;         for (int p_=0;p_<NP_;p_++){             ProvinceEconomy *pe_=&g_fe->prov[p_];             if (!pe_->active || !pe_->colonized || pe_->owner!=(cid)) continue;             for (int i_=0;i_<pe_->pop.n_groups;i_++){                 PopGroup *G=&pe_->pop.groups[i_];                 if (G->count<=0) continue;                 EthosFaction FAC=group_fac(G);                 BODY             }         }     } } while(0)

/* §C3 — CAPTURE DE L'ÉTAT : chaque concession gorge la faction gagnante. S'ACCUMULE,
 * décroît très lentement, ne rebondit pas. La somme = le « rot » (0..1) : moins
 * d'efficacité noble, K creusé. Lue à l'écran comme l'indice de Corruption (0-100). */
static float g_capture[SCPS_MAX_COUNTRY][FAC_COUNT];
#define CAPTURE_PER_CONCESSION 0.16f  /* recalé (0.045 tableau → 0.16 prorata-teinte : lean gagnant ~0.5 × moyenne clout ~0.5 = même échelle de rot) */
#define CAPTURE_LEVER          0.06f  /* … qui gagne aussi en POUVOIR (un vote tenu) */
#define CAPTURE_MAX            0.85f  /* plafond du rot : un État jamais 100 % capturé */
#define CAPTURE_DECAY_FRAC     0.04f  /* la capture décroît à 4 % du rythme du grief (lent) */

void faction_save(FILE *f){
    /* v100 : seule la STANCE se sérialise ici — grief/capture voyagent AVEC les
     * groupes (section ECON). La section FACT rétrécit ⇒ un save antérieur est
     * d'une « ère antérieure » (refusé net par le contrôle de version). */
    fwrite(g_lever_bias, sizeof g_lever_bias, 1,f);
}
bool faction_load(FILE *f){
    return fread(g_lever_bias, sizeof g_lever_bias, 1,f)==1;
}
void faction_levers_reset(void){
    memset(g_lever_bias, 0, sizeof g_lever_bias);
    /* les champs de groupes appartiennent à ECON : remis à zéro par SA genèse
     * (memset des groupes) — si un monde est lié, on nettoie aussi (bancs). */
    if (g_fe){
        int NP=g_fe->n_prov; if (NP>SCPS_MAX_PROV) NP=SCPS_MAX_PROV;
        for (int p=0;p<NP;p++)
            for (int i=0;i<g_fe->prov[p].pop.n_groups;i++){
                g_fe->prov[p].pop.groups[i].ethos_grief=0.f;
                g_fe->prov[p].pop.groups[i].state_grip =0.f;
            }
    }
}
/* Une concession ACCORDÉE : la faction gagnante se gorge (capture↑) et gagne du
 * pouvoir (un vote). Le calme acheté aujourd'hui est une dette de demain. */
void faction_concede(int cid, EthosFaction winner){
    if (cid<0||cid>=SCPS_MAX_COUNTRY||winner<0||winner>=FAC_COUNT) return;
    /* la concession GORGE les peuples du courant gagnant — par tête (l'échelle ne
     * dépend pas de la taille du courant, comme l'ancien tableau) */
    /* la concession gorge chaque peuple AU PRORATA de sa teinte du courant gagnant */
    FOR_COUNTRY_GROUPS(cid, g, gf, { (void)gf;
        float lean[FAC_COUNT]; group_lean_full(g, lean);
        g->state_grip += CAPTURE_PER_CONCESSION*lean[winner];
        if (g->state_grip>1.f) g->state_grip=1.f;
    });
    faction_lever_apply(cid, winner, CAPTURE_LEVER);           /* le captor monte en pouvoir */
}
/* MONNAIE M3h — LA DÉBASE : le même accumulateur g_capture, un incrément CONTINU
 * (l'appelant décide du rythme, ∝ dt×niveau de débase) au lieu du saut fixe d'une
 * concession — AUCUN faction_lever_apply ici (ce n'est pas un vote gagné, juste
 * l'enrichissement passif des initiés). Plafonné par CAPTURE_MAX via faction_capture_
 * total (la somme brute peut dépasser le plafond en interne, comme faction_concede). */
void faction_capture_add(int cid, EthosFaction fac, float amount){
    if (cid<0||cid>=SCPS_MAX_COUNTRY||fac<0||fac>=FAC_COUNT||amount<=0.f) return;
    FOR_COUNTRY_GROUPS(cid, g, gf, { (void)gf;
        float lean[FAC_COUNT]; group_lean_full(g, lean);
        g->state_grip += amount*lean[fac];
        if (g->state_grip>1.f) g->state_grip=1.f;
    });
}
/* Le « rot » 0..1 : part de l'État capturée (toutes factions), plafonnée. */
/* la capture d'un COURANT = la moyenne pondérée (pop) du grip de ses peuples ;
 * le « rot » du pays = Σ courants — même échelle que l'ancien tableau. */
static void country_capture_by(int cid, float out[FAC_COUNT]){
    double wsum[FAC_COUNT]={0}, gsum[FAC_COUNT]={0};
    FOR_COUNTRY_GROUPS(cid, g, gf, { (void)gf;
        float lean[FAC_COUNT]; group_lean_full(g, lean);
        for (int k2=0;k2<FAC_COUNT;k2++){
            double wt=(double)g->count*(double)lean[k2];
            wsum[k2]+=wt; gsum[k2]+=wt*(double)g->state_grip;
        }
    });
    for (int k=0;k<FAC_COUNT;k++) out[k]=(wsum[k]>0.0)?(float)(gsum[k]/wsum[k]):0.f;
}
float faction_capture_total(int cid){
    /* le « rot » = la prise du courant LE PLUS GORGÉ : l'État est capturé par SON
     * captor — un courant d'un tiers du pouvoir peut pourrir l'État entier (les
     * offices qu'il tient suffisent). max plutôt que moyenne : une moyenne diluait
     * la capture dans la masse saine et l'indice ne bougeait plus (mesuré). */
    if (cid<0||cid>=SCPS_MAX_COUNTRY) return 0.f;
    float by[FAC_COUNT]; country_capture_by(cid, by);
    float s=0.f; for (int k=0;k<FAC_COUNT;k++) if (by[k]>s) s=by[k];
    return s<0.f?0.f:(s>CAPTURE_MAX?CAPTURE_MAX:s);
}
/* La métrique CORRUPTION (0-100) — le visage chiffré de la capture (l'écran). */
int faction_corruption_0_100(int cid){ return (int)(100.f*faction_capture_total(cid)+0.5f); }
/* I5 — L'AUDIT DES OFFICES : l'État RÉPRIME la capture (−20 points de corruption,
 * raboté au prorata sur toutes les factions). Renvoie la corruption AVANT (0-100) —
 * l'appelant (qui tient le trésor + la légitimité) en tire le coût et l'effet sur L. */
int faction_audit(int cid){
    if (cid<0||cid>=SCPS_MAX_COUNTRY) return 0;
    int before = faction_corruption_0_100(cid);
    float raw = faction_capture_total(cid);
    if (raw>1e-4f){
        float keep = (raw-0.20f)/raw; if (keep<0.f) keep=0.f;   /* −0.20 de capture = −20 points */
        FOR_COUNTRY_GROUPS(cid, g, gf, { (void)gf; g->state_grip*=keep; });
    }
    return before;
}
/* La faction qui TIENT l'État (capture la plus haute) — pour le survol. */
EthosFaction faction_captor(int cid){
    if (cid<0||cid>=SCPS_MAX_COUNTRY) return FAC_COMMUNAUTAIRE;
    float by[FAC_COUNT]; country_capture_by(cid, by);
    int best=0; for (int k=1;k<FAC_COUNT;k++) if (by[k]>by[best]) best=k;
    return (EthosFaction)best;
}
void faction_lever_apply(int cid, EthosFaction advanced, float strength){
    if (cid<0||cid>=SCPS_MAX_COUNTRY||advanced<0||advanced>=FAC_COUNT||strength<=0.f) return;
    float b=g_lever_bias[cid][advanced]+strength; if (b>LEVER_BIAS_CAP) b=LEVER_BIAS_CAP;
    g_lever_bias[cid][advanced]=b;                              /* le courant aligné gagne du poids */
    /* … et les PEUPLES s'aigrissent PAR TEINTE — chaque groupe rumine à hauteur de
     * son opposition PONDÉRÉE à la politique (Σ lean×opposition), pas seulement son
     * courant dominant : le peuple ésotérique porte la rancœur transgressive même
     * si sa tête penche gardienne. */
    FOR_COUNTRY_GROUPS(cid, g, gf, { (void)gf;
        float lean[FAC_COUNT]; group_lean_full(g, lean);
        float opp=0.f;
        for (int k2=0;k2<FAC_COUNT;k2++)
            if (k2!=(int)advanced) opp += lean[k2]*faction_opposition((EthosFaction)k2,advanced);
        /* FAC_TINT_GAIN : la teinte DILUE (l'opposition pondérée d'un peuple mêlé vaut
         * ~la moitié de l'opposition franche du tableau d'antan) — le gain retient
         * l'ÉCHELLE historique du canal grief→loyauté (cible (1−grief)×100). */
        float gr=g->ethos_grief + strength*opp*tune_f("FAC_TINT_GAIN",2.0f);
        g->ethos_grief = gr>1.f ? 1.f : gr;
    });
}
void faction_levers_decay(float rate){
    float r = rate<0.f?0.f:(rate>1.f?1.f:rate);
    float k  = 1.f - r;                                         /* la stance non entretenue s'efface */
    float kc = 1.f - r*CAPTURE_DECAY_FRAC;                      /* §C3 : la capture décroît TRÈS lentement */
    for (int c=0;c<SCPS_MAX_COUNTRY;c++) for (int fk=0;fk<FAC_COUNT;fk++) g_lever_bias[c][fk]*=k;
    if (g_fe){
        int NP=g_fe->n_prov; if (NP>SCPS_MAX_PROV) NP=SCPS_MAX_PROV;
        for (int p=0;p<NP;p++)
            for (int i=0;i<g_fe->prov[p].pop.n_groups;i++){
                PopGroup *g=&g_fe->prov[p].pop.groups[i];
                g->ethos_grief*=k; g->state_grip*=kc;
            }
    }
}
/* LE COURANT D'UNE CLASSE DANS UN PAYS (statecraft porté par la pop, 2026-08-06) :
 * l'agrégat des penchants des peuples de cette classe — le courant des élites de CE
 * pays, pas une abstraction. Sans bind ou sans peuple de la classe : le SOCLE seul
 * (élite→Gardien, bourgeois→Marchand, laboureur→Communautaire). */
EthosFaction faction_class_current(int cid, SocialClass k){
    double acc[FAC_COUNT]={0}; bool any=false;
    if (cid>=0 && cid<SCPS_MAX_COUNTRY){
        FOR_COUNTRY_GROUPS(cid, g, gf, { (void)gf;
            if (g->pop_by_class[k]<=0) continue;
            any=true;
            float lean[FAC_COUNT]; group_ethos_lean_k(&g->culture, k, lean);
            for (int i=0;i<FAC_COUNT;i++) acc[i]+=(double)g->pop_by_class[k]*lean[i];
        });
    }
    if (!any){ float base[FAC_COUNT]; class_ethos_base(k, base);
               int b=0; for (int i=1;i<FAC_COUNT;i++) if (base[i]>base[b]) b=i;
               return (EthosFaction)b; }
    int b=0; for (int i=1;i<FAC_COUNT;i++) if (acc[i]>acc[b]) b=i;
    return (EthosFaction)b;
}
float faction_grievance(int cid, EthosFaction fac){
    /* SANS SEUIL : la moyenne des colères pondérée par la TEINTE du courant — les
     * poids se normalisent, la lecture n'est jamais 0 dès qu'un peuple rumine
     * (un seuil tuait les teintes minoritaires : l'élite nominale plafonne ~3 %). */
    if (cid<0||cid>=SCPS_MAX_COUNTRY||fac<0||fac>=FAC_COUNT) return 0.f;
    double wsum=0.0, gsum=0.0;
    FOR_COUNTRY_GROUPS(cid, g, gf, { (void)gf;
        float lean[FAC_COUNT]; group_lean_full(g, lean);
        double wt=(double)g->count*(double)lean[fac];
        wsum+=wt; gsum+=wt*(double)g->ethos_grief;
    });
    return (wsum>0.0) ? (float)(gsum/wsum) : 0.f;
}
void faction_grievance_add(int cid, EthosFaction fac, float amount){
    /* aigrir « les Gardiens » aigrit le PAYS — la lecture par courant pondère par
     * teinte : la colère est une, sa couleur vient de qui la porte. */
    if (cid<0||cid>=SCPS_MAX_COUNTRY||fac<0||fac>=FAC_COUNT) return;
    (void)fac;
    FOR_COUNTRY_GROUPS(cid, g, gf, { (void)gf;
        float gr=g->ethos_grief+amount;
        g->ethos_grief = gr<0.f?0.f:(gr>1.f?1.f:gr);
    });
}
/* ⚠ SUPPRIMÉ (raccord 8, Âges sans ordre imposé, docs/AGES_FINS_2026-07-11.md) :
 * age_patron()/faction_age_engage() — l'ancien « engagement d'âge » (§7) posait un
 * vote de faction MONDIAL (une même faction-patronne pour TOUS les pays, quel que
 * soit leur rapport matériel à l'âge) ET un bonus de SATISFACTION (« la cohésion du
 * régime »). La règle commune de la refonte est explicite : « AUCUN âge ne donne de
 * satisfaction » et « les leviers de factions ne s'appliquent que dans les pays
 * MATÉRIELLEMENT concernés ». Les nouveaux leviers scopés vivent désormais dans
 * scps_events.c (age_lever_exchange/_discovery/_empires/_breach/_lumieres/
 * _soulevements/_tyrans), appelés UNE fois à l'avènement de CHAQUE âge — pas un
 * mécanisme générique par pays. Le verbe joueur CMD_AGE_ENGAGE (scps_sim.c) reste
 * mais devient une pure NOTIFICATION D'AVÈNEMENT (accusé de réception, sans effet
 * moteur) — cf. TROUVAILLES.md pour le détail du choix. */
void faction_levers_on_coup(int cid){
    /* Le coup a basculé le régime : la rancœur accumulée se DÉCHARGE (sinon le pays
     * recouve aussitôt — un coup tous les deux ans). La pression politique est purgée. */
    if (cid<0||cid>=SCPS_MAX_COUNTRY) return;
    FOR_COUNTRY_GROUPS(cid, g, gf, { (void)gf; g->ethos_grief=0.f; });
}

EthosFaction faction_effective_distribution(const World *w, const WorldEconomy *econ,
                                            int cid, float out[FAC_COUNT]){
    float base[FAC_COUNT]; country_faction_weights(w, econ, cid, base);
    double s=0.0;
    for (int f=0; f<FAC_COUNT; f++){
        float bias = (cid>=0&&cid<SCPS_MAX_COUNTRY) ? g_lever_bias[cid][f] : 0.f;
        float v = base[f] + bias; if (v<0.f) v=0.f;
        out[f]=v; s+=v;
    }
    int dom=FAC_COMMUNAUTAIRE; float best=-1.f;
    for (int f=0; f<FAC_COUNT; f++){
        out[f] = (s>0.0)?(float)(out[f]/s):(f==FAC_COMMUNAUTAIRE?1.f:0.f);
        if (out[f]>best){ best=out[f]; dom=f; }
    }
    return (EthosFaction)dom;
}

float faction_coup_tension_c(const World *w, const WorldEconomy *econ,
                             int cid, EthosFaction *out){
    /* La tension reste ancrée sur la distribution de BASE (la démographie, niveau §5,
     * borné) — surtout PAS sur la dominante biaisée par les leviers, sinon favoriser
     * un éthos rend ses opposés chroniquement aliénés → coups en boucle. Les leviers
     * n'AJOUTENT qu'un grief BORNÉ (∝ politique), purgé par un coup réussi. */
    float base[FAC_COUNT];
    country_faction_weights(w, econ, cid, base);
    return faction_coup_breakdown(cid, base, NULL, out);
}

float faction_coup_breakdown(int cid, const float base[FAC_COUNT],
                             float out[FAC_COUNT], EthosFaction *alienated){
    if (!base){
        if (out) for (int f=0; f<FAC_COUNT; f++) out[f]=0.f;
        if (alienated) *alienated=FAC_COMMUNAUTAIRE;
        return 0.f;
    }
    int dom=0;
    for (int f=1; f<FAC_COUNT; f++) if (base[f]>base[dom]) dom=f;
    float best=0.f; int bf=dom;
    for (int f=0; f<FAC_COUNT; f++){
        float t=0.f;
        if (f!=dom){
            float grief = faction_grievance(cid, (EthosFaction)f);   /* lu des PEUPLES du courant */
            t = base[f]*faction_opposition((EthosFaction)f,(EthosFaction)dom) + COUP_GRIEF_W*grief;
        }
        if (out) out[f]=t;
        if (t>best){ best=t; bf=f; }
    }
    if (alienated) *alienated=(EthosFaction)bf;
    return best;
}
