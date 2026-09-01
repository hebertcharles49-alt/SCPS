/*
 * scps_missions.c — LES DESSEINS (cf. scps_missions.h pour la doctrine).
 *
 * P1 livre le FRAMEWORK + la BRANCHE DU SOL, le pilote qui le prouve. Propriété
 * golden forte : la branche NE TIRE AUCUN xs32 — toutes ses cibles dérivent de
 * la seule géographie (BFS prov_adj, valeur développée de la province, rancune)
 * et tous ses départages sont des index croissants. Le flux rng du monde est
 * donc INTACT même si la branche était générée pour tous ; en P1 elle ne l'est
 * que pour le joueur (gate human_player), et le golden est intact deux fois.
 *
 * GRAIN : cible = pid, condition = prov[pid].owner. On ne lit JAMAIS
 * region[].owner (agrégat reconstruit) et on ne route JAMAIS un chemin joueur
 * par econ_region_rep_province. Seule la REVENDICATION est région-grain — c'est
 * le seul grain que le moteur donne à un claim (fab_region[][]).
 */
#include "scps_missions.h"
#include "scps_tune.h"      /* registre J : DESSEIN_* + les clés portées par les remises */
#include "scps_provlog.h"   /* LE FIL : une ligne quand une cible BASCULE (display, write-only) */
#include <string.h>
#include <math.h>

/* ====================================================================== */
/* LE CANAL DATÉ — le miroir de process lu par dessein_mult                */
/* ====================================================================== */
/* Motif decree_mult : les sites de lecture (scps_diplo.c, scps_statecraft.c)
 * n'ont pas le MissionsState. On tient donc un MIROIR statique de l'ANNÉE de
 * scellage de chaque remise, rafraîchi depuis l'état SÉRIALISÉ à chaque clôture
 * (missions_tick) et juste après un chargement (missions_boons_sync). Le miroir
 * lui-même n'est JAMAIS sérialisé : il se reconstruit intégralement, ce qui
 * évite un second état à revalider. */
static int16_t g_boon_year[SCPS_MAX_COUNTRY][DBOON_COUNT];
static int16_t g_boon_now = 0;
static bool    g_boon_ready = false;

/* La VALEUR de chaque remise (annexe : branche du Sol). Une seule par clé —
 * le clamp composé [0.60, 1.60] (H2bis étendu aux Desseins) est appliqué à la
 * lecture, pour que l'ajout d'une seconde branche portant la même clé ne puisse
 * jamais faire dériver le site. */
static const float DBOON_M[DBOON_COUNT] = {
    1.60f,   /* DBOON_FAB_VALID_DAYS        — « Le rival » : les torts consignés (5 → 8 ans) */
    0.75f,   /* DBOON_ANNEX_SOFT_SCAR       — « Pacification » */
    0.80f,   /* DBOON_ANNEX_YEARS_PER_PRICE — « Les marches » */
    1.30f,   /* DBOON_OPINION_VASSAL        — « Premier vassal » : le crédit du serment */
    0.85f,   /* DBOON_ANNEX_MIN_INTEGRATION — « Trois vassaux » */
};
/* Quel ÉCHELON porte quelle remise (voie comprise) — la table unique lue par
 * missions_boons_sync ET par le versement au scellage. rung/voie == -1 : joker. */
static const struct { int8_t rung, voie; } DBOON_SRC[DBOON_COUNT] = {
    { DESS_RUNG_RIVAL, -1 },
    { DESS_RUNG_4, DESS_VOIE_CONQUETE },
    { DESS_RUNG_5, DESS_VOIE_CONQUETE },
    { DESS_RUNG_4, DESS_VOIE_VASSALISATION },
    { DESS_RUNG_5, DESS_VOIE_VASSALISATION },
};

void missions_boons_sync(const MissionsState *ms, int year){
    memset(g_boon_year, 0xFF, sizeof g_boon_year);   /* 0xFFFF = -1 : aucune remise */
    g_boon_now = (int16_t)((year<-32000)?-32000:(year>32000)?32000:year);
    g_boon_ready = true;
    if (!ms) return;
    for (int c=0;c<SCPS_MISSIONS_MAX;c++){
        const Dessein *d = &ms->d[c][DESS_SOL];
        if (!d->gen) continue;
        for (int k=0;k<DBOON_COUNT;k++){
            int r = DBOON_SRC[k].rung;
            if (DBOON_SRC[k].voie>=0 && d->voie != DBOON_SRC[k].voie) continue;
            if (r<0 || r>=DESSEIN_RUNGS) continue;
            g_boon_year[c][k] = d->sealed_year[r];
        }
    }
}

float dessein_mult(int cid, DesseinBoon k){
    if (!g_boon_ready) return 1.f;                     /* aucun Dessein n'a jamais vécu dans ce process */
    if (cid<0 || cid>=SCPS_MAX_COUNTRY) return 1.f;
    if ((int)k<0 || (int)k>=DBOON_COUNT) return 1.f;
    int sy = g_boon_year[cid][k];
    if (sy < 0) return 1.f;                            /* échelon non scellé */
    float dur = tune_f("DESSEIN_BOON_YEARS", 20.f);
    if (dur <= 0.f) return 1.f;                        /* kill-switch : toutes les remises muettes */
    if ((float)(g_boon_now - sy) >= dur) return 1.f;   /* la fenêtre est passée — ZÉRO accumulateur décrémenté */
    float m = DBOON_M[k];
    return (m<0.60f) ? 0.60f : (m>1.60f) ? 1.60f : m;  /* clamp H2bis étendu aux Desseins */
}

/* ── LE PRIX DU PIVOT (hook) ─────────────────────────────────────────────
 * BRANCHÉ SUR L'INFLUENCE AU MERGE : le module d'influence politique (§3) n'a
 * pas encore atterri dans cet arbre. Le stub accepte, l'orchestrateur y câble
 * le débit de DESSEIN_PIVOT_INFLUENCE (20, plat). */
bool dessein_pivot_pay(int cid){
    (void)cid;
    return true;   /* BRANCHÉ SUR L'INFLUENCE AU MERGE */
}

/* ====================================================================== */
/* PETITE GÉOGRAPHIE — tout est pur, borné, sans rng                       */
/* ====================================================================== */
static bool prov_ok(const WorldEconomy *e, int pid){
    return e && pid>=0 && pid<e->n_prov && pid<SCPS_MAX_PROV && e->prov[pid].active;
}
static bool prov_mine(const WorldEconomy *e, int pid, int cid){
    return prov_ok(e,pid) && e->prov[pid].owner==cid;
}
/* Un pays VIT s'il tient au moins une province ACTIVE (grain province — jamais
 * region[].owner, qui n'est qu'un reflet reconstruit à la clôture). */
static bool country_alive(const WorldEconomy *e, const World *w, int cid){
    if (!e||!w||cid<0||cid>=w->n_countries) return false;
    if (w->country[cid].role==POLITY_UNCLAIMED) return false;
    int n=e->n_prov; if (n>SCPS_MAX_PROV) n=SCPS_MAX_PROV;
    for (int p=0;p<n;p++) if (e->prov[p].owner==cid && e->prov[p].active) return true;
    return false;
}
/* La VALEUR d'une province au sens du Dessein — le miroir PROVINCE-GRAIN de
 * diplo_province_price (« bâti + prospérité + population », scps_diplo.h §5).
 * La doctrine interdit de router un chemin joueur par un lecteur région ; on lit
 * donc les MÊMES ingrédients, sur prov[]. Pure et sans rng. */
static float prov_value(const ProvinceEconomy *pe){
    float pop=0.f; for (int k=0;k<CLASS_COUNT;k++) pop+=pe->strata[k].pop;
    const ProvBuild *b=&pe->build;
    float built = b->K_inst+b->H_coerc+b->P_open+b->PE_infra+b->food_cap+b->faith+b->savoir+b->port;
    return pop*0.01f + built + pe->prosperity;
}

/* BFS MULTI-SOURCE sur l'adjacence de PROVINCES depuis TOUT le territoire de
 * `cid` : hops[p] = nombre de sauts jusqu'à la province p (0 = elle est à moi,
 * -1 = inatteignable). Scratch de module (jamais sérialisé, réécrit à chaque
 * appel : déterministe, et 1664 int16 ne tiennent pas confortablement sur la
 * pile à côté du reste). ⚠ prov_adj est un POINTEUR TAS rebâti au chargement :
 * cette fonction n'est appelée que depuis la clôture mensuelle, jamais pendant
 * un load (annexe N1). */
static int16_t g_hops[SCPS_MAX_PROV];
static int16_t g_queue[SCPS_MAX_PROV];
static bool bfs_from_country(const WorldEconomy *e, int cid){
    if (!e || !e->prov_adj) return false;
    int n=e->n_prov; if (n>SCPS_MAX_PROV) n=SCPS_MAX_PROV;
    for (int p=0;p<n;p++) g_hops[p]=-1;
    int head=0, tail=0;
    for (int p=0;p<n;p++) if (e->prov[p].owner==cid && e->prov[p].active){ g_hops[p]=0; g_queue[tail++]=(int16_t)p; }
    if (tail==0) return false;
    while (head<tail){
        int a=g_queue[head++];
        int16_t h=(int16_t)(g_hops[a]+1);
        const uint8_t *row = e->prov_adj + (size_t)a*SCPS_MAX_PROV;
        for (int b=0;b<n;b++){
            if (!row[b] || g_hops[b]>=0 || !e->prov[b].active) continue;
            g_hops[b]=h; g_queue[tail++]=(int16_t)b;
        }
    }
    return true;
}
/* La DISTANCE (en sauts) de mon territoire au territoire de `other`, après un
 * bfs_from_country(cid). INT16_MAX si l'autre est hors d'atteinte. */
static int hops_to_country(const WorldEconomy *e, int other){
    int n=e->n_prov; if (n>SCPS_MAX_PROV) n=SCPS_MAX_PROV;
    int best=32767;
    for (int p=0;p<n;p++)
        if (e->prov[p].owner==other && e->prov[p].active && g_hops[p]>=0 && g_hops[p]<best) best=g_hops[p];
    return best;
}

/* ====================================================================== */
/* LA BRANCHE DU SOL — résolution des cibles                               */
/* ====================================================================== */
/* Le RIVAL : argmax rancor[cid][b] parmi les pays VIVANTS (repli : le plus
 * proche). Départage cid croissant. Latché à l'échelon 2 ; un rival MORT est
 * remplacé (re-tirage déterministe) et ne revient jamais. */
static int sol_pick_rival(const World *w, const WorldEconomy *e, const DiploState *dp, int cid){
    int best=-1; float bestr=0.f;
    for (int b=0;b<w->n_countries && b<SCPS_MAX_COUNTRY;b++){
        if (b==cid || !country_alive(e,w,b)) continue;
        float r = dp ? diplo_rancor(dp,cid,b) : 0.f;
        if (r>bestr){ bestr=r; best=b; }
    }
    if (best>=0) return best;
    /* Repli : le plus PROCHE (le voisin qu'on ne peut pas ignorer). */
    if (!bfs_from_country(e,cid)) return -1;
    int bh=32767;
    for (int b=0;b<w->n_countries && b<SCPS_MAX_COUNTRY;b++){
        if (b==cid || !country_alive(e,w,b)) continue;
        int h=hops_to_country(e,b);
        if (h<bh){ bh=h; best=b; }
    }
    return best;
}
/* T2 « Expansion » : la plus proche province ÉTRANGÈRE OU VIERGE. Ordre :
 * sauts croissants, habitabilité décroissante, pid croissant. */
static int sol_pick_march(const WorldEconomy *e, int cid){
    if (!bfs_from_country(e,cid)) return -1;
    int n=e->n_prov; if (n>SCPS_MAX_PROV) n=SCPS_MAX_PROV;
    int best=-1, bh=32767; float bhab=-1.f;
    for (int p=0;p<n;p++){
        const ProvinceEconomy *pe=&e->prov[p];
        if (!pe->active || pe->owner==cid || g_hops[p]<0) continue;
        int h=g_hops[p];
        if (h<bh || (h==bh && pe->habitability>bhab)){ bh=h; bhab=pe->habitability; best=p; }
    }
    return best;
}
/* T3 « Le rival » : SA province de plus forte valeur, l'adjacence à moi d'abord
 * (une marche, pas un rêve d'outre-monde). Départage pid croissant. */
static int sol_pick_rival_march(const WorldEconomy *e, int cid, int rival){
    if (rival<0) return -1;
    if (!bfs_from_country(e,cid)) return -1;
    int n=e->n_prov; if (n>SCPS_MAX_PROV) n=SCPS_MAX_PROV;
    int best=-1; float bv=-1.f; int bh=32767;
    for (int p=0;p<n;p++){
        const ProvinceEconomy *pe=&e->prov[p];
        if (!pe->active || pe->owner!=rival || g_hops[p]<0) continue;
        int adj = (g_hops[p]<=1) ? 0 : 1;          /* l'adjacente PASSE devant la lointaine */
        float v = prov_value(pe);
        if (adj<bh || (adj==bh && v>bv)){ bh=adj; bv=v; best=p; }
    }
    return best;
}
/* « Les marches » (5A) : les TROIS provinces étrangères ADJACENTES à moi de plus
 * forte valeur. Départage pid croissant. Écrit trio[] (les vides restent -1). */
static void sol_pick_three_marches(const WorldEconomy *e, int cid, int16_t out[3]){
    out[0]=out[1]=out[2]=-1;
    if (!bfs_from_country(e,cid)) return;
    int n=e->n_prov; if (n>SCPS_MAX_PROV) n=SCPS_MAX_PROV;
    float bv[3]={-1.f,-1.f,-1.f};
    for (int p=0;p<n;p++){
        const ProvinceEconomy *pe=&e->prov[p];
        if (!pe->active || pe->owner==cid || g_hops[p]!=1) continue;
        float v=prov_value(pe);
        for (int k=0;k<3;k++)
            if (v>bv[k]){                             /* insertion triée — valeur ↓, pid ↑ à égalité stricte */
                for (int j=2;j>k;j--){ bv[j]=bv[j-1]; out[j]=out[j-1]; }
                bv[k]=v; out[k]=(int16_t)p; break;
            }
    }
}
/* « Premier vassal / Trois vassaux » : les pays LIÉABLES (vivants, libres OU
 * déjà mes vassaux) les plus PROCHES, V1 épinglé en tête s'il vit. Départage
 * cid croissant. */
static void sol_pick_oaths(const World *w, const WorldEconomy *e, const DiploState *dp,
                           int cid, int pin, int16_t out[3]){
    out[0]=out[1]=out[2]=-1;
    if (!bfs_from_country(e,cid)) return;
    int bh[3]={32767,32767,32767};
    for (int b=0;b<w->n_countries && b<SCPS_MAX_COUNTRY;b++){
        if (b==cid || b==pin || !country_alive(e,w,b)) continue;
        int suz = dp ? diplo_suzerain(dp,b) : -1;
        if (suz>=0 && suz!=cid) continue;            /* déjà sous une autre couronne : hors vivier */
        int h=hops_to_country(e,b);
        if (h>=32767) continue;
        for (int k=0;k<3;k++)
            if (h<bh[k]){
                for (int j=2;j>k;j--){ bh[j]=bh[j-1]; out[j]=out[j-1]; }
                bh[k]=h; out[k]=(int16_t)b; break;
            }
    }
    if (pin>=0 && country_alive(e,w,pin)){           /* V1 garde sa place : c'est LUI qu'on a nommé */
        out[2]=out[1]; out[1]=out[0]; out[0]=(int16_t)pin;
    }
}
/* « Pacification » (4A) : T3 s'il est à moi, sinon MA province la plus MARQUÉE
 * par une annexion (la plaie la plus fraîche — c'est elle qu'il s'agit de
 * refermer). Départage pid croissant. */
static int sol_pick_scar(const WorldEconomy *e, int cid, int t3){
    if (prov_mine(e,t3,cid)) return t3;
    int n=e->n_prov; if (n>SCPS_MAX_PROV) n=SCPS_MAX_PROV;
    int best=-1; float bs=-1.f;
    for (int p=0;p<n;p++){
        const ProvinceEconomy *pe=&e->prov[p];
        if (!pe->active || pe->owner!=cid) continue;
        float s = (pe->annex_scar > pe->revolt_scar) ? pe->annex_scar : pe->revolt_scar;
        if (s>bs){ bs=s; best=p; }
    }
    return best;
}

/* La RÉGION d'une province — le seul grain que le Fil (FeedEntry.region) et les
 * Annales savent nommer (un toponyme est région-grain : « la marche de {ville} »). */
static int reg_of(const World *w, int pid){
    return (w && pid>=0 && pid<w->n_provinces) ? w->province[pid].region : -1;
}
/* La province CAPITALE courante du pays (grain province, jamais la région). */
static int cap_prov(const World *w, const WorldEconomy *e, int cid){
    if (!w||cid<0||cid>=w->n_countries) return -1;
    int cp=w->country[cid].capital_prov;
    return prov_ok(e,cp) ? cp : -1;
}

/* ── LA CIBLE DE L'ÉCHELON COURANT EST-ELLE ENCORE LÀ ? ──────────────────
 * L'invalidation est la DESTRUCTION, JAMAIS l'inconvénient : une province
 * passée à un allié RESTE la cible (la cible est la terre, pas le drapeau — il
 * faudra la prendre). Seules comptent la province ENGLOUTIE (!active) et la
 * couronne MORTE. */
static bool sol_targets_valid(const World *w, const WorldEconomy *e, const DiploState *dp,
                              const Dessein *d, int cid){
    switch (d->rung){
      case DESS_RUNG_UNIFICATION: return prov_ok(e, d->tpid[DESS_RUNG_UNIFICATION]);
      case DESS_RUNG_EXPANSION:   return prov_ok(e, d->tpid[DESS_RUNG_EXPANSION]);
      case DESS_RUNG_RIVAL:       return country_alive(e,w,d->rival) && prov_ok(e, d->tpid[DESS_RUNG_RIVAL]);
      case DESS_RUNG_PIVOT:       return true;
      case DESS_RUNG_4:
        if (d->voie==DESS_VOIE_CONQUETE) return prov_ok(e, d->tpid[DESS_RUNG_4]);
        return country_alive(e,w,d->trio[0]);
      case DESS_RUNG_5:
        if (d->voie==DESS_VOIE_CONQUETE)
            return prov_ok(e,d->trio[0]) && prov_ok(e,d->trio[1]) && prov_ok(e,d->trio[2]);
        return country_alive(e,w,d->trio[0]) && country_alive(e,w,d->trio[1]) && country_alive(e,w,d->trio[2]);
      case DESS_RUNG_6:
        if (d->voie==DESS_VOIE_CONQUETE) return country_alive(e,w,d->rival) && prov_ok(e, d->tpid[DESS_RUNG_6]);
        for (int k=0;k<3;k++)
            if (country_alive(e,w,d->trio[k]) && dp && diplo_suzerain(dp,d->trio[k])==cid) return true;
        return false;
      case DESS_RUNG_7:           return prov_ok(e, d->tpid[DESS_RUNG_UNIFICATION]);
      default: return true;
    }
}

/* RÉSOUT (ou RE-RÉSOUT) la cible de l'échelon courant sur le monde COURANT,
 * avec la règle d'origine. Renvoie true si quelque chose a CHANGÉ (le Fil en
 * porte une ligne : le joueur voit la cible bouger). */
static bool sol_resolve(const World *w, WorldEconomy *e, const DiploState *dp,
                        Dessein *d, int cid){
    /* On compare l'AVANT/APRÈS sur TOUTES les cibles (et pas seulement celle de
     * l'échelon courant) : un échelon qui PATIENTE re-tente sa résolution chaque
     * mois et ne doit pas remplir le Fil de lignes identiques. */
    Dessein snap = *d;
    switch (d->rung){
      case DESS_RUNG_UNIFICATION:
      case DESS_RUNG_7:
        d->tpid[DESS_RUNG_UNIFICATION] = (int16_t)cap_prov(w,e,cid);
        break;
      case DESS_RUNG_EXPANSION:
        d->tpid[DESS_RUNG_EXPANSION] = (int16_t)sol_pick_march(e,cid);
        break;
      case DESS_RUNG_RIVAL:
        if (!country_alive(e,w,d->rival)) d->rival = (int16_t)sol_pick_rival(w,e,dp,cid);
        d->tpid[DESS_RUNG_RIVAL] = (int16_t)sol_pick_rival_march(e,cid,d->rival);
        break;
      case DESS_RUNG_PIVOT: break;
      case DESS_RUNG_4:
        if (d->voie==DESS_VOIE_CONQUETE)
            d->tpid[DESS_RUNG_4] = (int16_t)sol_pick_scar(e,cid,d->tpid[DESS_RUNG_RIVAL]);
        else {
            int pin = country_alive(e,w,d->rival) ? d->rival : -1;
            int16_t t[3]; sol_pick_oaths(w,e,dp,cid,pin,t);
            d->trio[0]=t[0];                                /* V1 = le rival s'il vit, sinon le plus proche */
        }
        break;
      case DESS_RUNG_5:
        if (d->voie==DESS_VOIE_CONQUETE) sol_pick_three_marches(e,cid,d->trio);
        else {
            int pin = (d->trio[0]>=0 && country_alive(e,w,d->trio[0])) ? d->trio[0] : -1;
            sol_pick_oaths(w,e,dp,cid,pin,d->trio);
        }
        break;
      case DESS_RUNG_6:
        if (d->voie==DESS_VOIE_CONQUETE){
            if (!country_alive(e,w,d->rival)) d->rival = (int16_t)sol_pick_rival(w,e,dp,cid);
            int cp = (d->rival>=0 && d->rival<w->n_countries) ? w->country[d->rival].capital_prov : -1;
            d->tpid[DESS_RUNG_6] = (int16_t)(prov_ok(e,cp) ? cp : -1);
        } else {
            int pin = (d->trio[0]>=0 && country_alive(e,w,d->trio[0])) ? d->trio[0] : -1;
            sol_pick_oaths(w,e,dp,cid,pin,d->trio);          /* la Toile s'est défaite : on la retisse */
        }
        break;
      default: break;
    }
    return (snap.rival != d->rival)
        || memcmp(snap.tpid, d->tpid, sizeof d->tpid) != 0
        || memcmp(snap.trio, d->trio, sizeof d->trio) != 0;
}

/* ====================================================================== */
/* LES CONDITIONS — des prédicats d'ÉTAT RÉEL, au grain province           */
/* ====================================================================== */
/* La part du CONTINENT de ma capitale que je tiens (7A : possédé · 7B :
 * possédé OU vassal de moi). Provinces ACTIVES au dénominateur. */
static float sol_hegemony_frac(const World *w, const WorldEconomy *e, const DiploState *dp,
                               int cid, bool count_vassals){
    int cp=cap_prov(w,e,cid); if (cp<0 || cp>=w->n_provinces) return 0.f;
    int cont = w->province[cp].continent; if (cont<0) return 0.f;
    int n=e->n_prov; if (n>SCPS_MAX_PROV) n=SCPS_MAX_PROV;
    if (n>w->n_provinces) n=w->n_provinces;
    int tot=0, mine=0;
    for (int p=0;p<n;p++){
        if (!e->prov[p].active || w->province[p].continent!=cont) continue;
        tot++;
        int o=e->prov[p].owner;
        if (o==cid) mine++;
        else if (count_vassals && o>=0 && dp && diplo_suzerain(dp,o)==cid) mine++;
    }
    return (tot>0) ? (float)mine/(float)tot : 0.f;
}

static bool sol_condition(const World *w, const WorldEconomy *e, const DiploState *dp,
                          const Dessein *d, int cid){
    switch (d->rung){
      case DESS_RUNG_UNIFICATION: {
        /* « Unification » : chaque feu de la vallée répond au même ban — toutes les
         * provinces ACTIVES de la région-capitale possédées ET colonisées. */
        int cp=d->tpid[DESS_RUNG_UNIFICATION];
        if (!prov_ok(e,cp) || cp>=w->n_provinces) return false;
        int reg=w->province[cp].region; if (reg<0 || reg>=w->n_regions) return false;
        const Region *rg=&w->region[reg];
        int seen=0;
        for (int k=0;k<rg->n_provinces;k++){
            int pid=rg->province_ids[k];
            if (!prov_ok(e,pid)) continue;
            seen++;
            if (e->prov[pid].owner!=cid || !e->prov[pid].colonized) return false;
        }
        return seen>0;
      }
      case DESS_RUNG_EXPANSION:
        /* « Expansion » : la borne est déplacée d'un cran — par N'IMPORTE QUELLE
         * voie (colonisation, conquête, digestion, héritage). */
        return prov_mine(e, d->tpid[DESS_RUNG_EXPANSION], cid);
      case DESS_RUNG_RIVAL:
        /* « Le rival » : l'épée OU le serment (la répétition générale du pivot). */
        return prov_mine(e, d->tpid[DESS_RUNG_RIVAL], cid)
            || (d->rival>=0 && dp && diplo_suzerain(dp,d->rival)==cid);
      case DESS_RUNG_PIVOT:
        /* Le pivot est OUVERT dès l'échelon précédent scellé ; c'est le SCEAU qui
         * exige la preuve d'usage de la voie choisie (missions_seal). */
        return true;
      case DESS_RUNG_4:
        if (d->voie==DESS_VOIE_CONQUETE){
            /* « Pacification » : la plaie REFERMÉE, pas le drapeau planté. */
            int pid=d->tpid[DESS_RUNG_4];
            return prov_mine(e,pid,cid) && e->prov[pid].annex_scar < 0.05f
                                        && e->prov[pid].revolt_scar < 0.05f;
        }
        return d->trio[0]>=0 && dp && diplo_suzerain(dp,d->trio[0])==cid;
      case DESS_RUNG_5:
        if (d->voie==DESS_VOIE_CONQUETE){
            for (int k=0;k<3;k++) if (!prov_mine(e,d->trio[k],cid)) return false;
            return true;
        }
        for (int k=0;k<3;k++){
            if (d->trio[k]<0) return false;
            if (!dp || diplo_suzerain(dp,d->trio[k])!=cid) return false;
        }
        return true;
      case DESS_RUNG_6:
        if (d->voie==DESS_VOIE_CONQUETE)
            /* « Capitale rivale » : la PROPRIÉTÉ — l'occupation ne suffit pas
             * (le siège n'est pas la paix). */
            return prov_mine(e, d->tpid[DESS_RUNG_6], cid);
        /* « Intégration » : v_integration ≥ 1.0 SANS exiger l'annexion. */
        for (int k=0;k<3;k++){
            int v=d->trio[k];
            if (v<0 || !dp || diplo_suzerain(dp,v)!=cid) continue;
            if (dp->v_integration[v] >= 1.f) return true;
        }
        return false;
      case DESS_RUNG_7: {
        float need = tune_f("DESSEIN_SOL_HEGEMON_FRAC", 0.40f);
        return sol_hegemony_frac(w,e,dp,cid, d->voie==DESS_VOIE_VASSALISATION) >= need;
      }
      default: return false;
    }
}

/* ====================================================================== */
/* LES RÉCOMPENSES                                                         */
/* ====================================================================== */
/* LA REVENDICATION NOMMÉE, GRATUITE (le coût d'or de fabrication est SAUTÉ,
 * la fenêtre reste normale).
 * ⚠ fab_state[a][b] est un slot UNIQUE PAR PAIRE : on ne pose QUE si FAB_NONE.
 * Sinon la revendication est RETENUE (claim_pend) et re-tentée à chaque clôture
 * — JAMAIS on n'écrase une intrigue que le joueur a payée. */
static void sol_try_claim(DiploState *dp, const World *w, const WorldEconomy *e,
                          Dessein *d, int cid){
    int pid=d->claim_pid;
    if (!dp || !d->claim_pend || !prov_ok(e,pid) || pid>=w->n_provinces){ return; }
    int b=e->prov[pid].owner;
    if (b<0 || b>=SCPS_MAX_COUNTRY || b==cid){ d->claim_pend=0; d->claim_pid=-1; return; }
    if (dp->fab_state[cid][b] != FAB_NONE) return;      /* slot occupé : on RETIENT, on n'écrase pas */
    int reg=w->province[pid].region;
    if (reg<0 || reg>=w->n_regions){ d->claim_pend=0; d->claim_pid=-1; return; }
    dp->fab_state [cid][b] = FAB_READY;                 /* MÛRE d'emblée : le Dessein a fait le travail */
    dp->fab_days  [cid][b] = tune_f("FAB_VALID_DAYS",1825.f) * dessein_mult(cid, DBOON_FAB_VALID_DAYS);
    dp->fab_cb    [cid][b] = (int8_t)CB_TERRITORIAL;
    dp->fab_region[cid][b] = (int16_t)reg;
    d->claim_pend=0; d->claim_pid=-1;
}
static void sol_arm_claim(DiploState *dp, const World *w, const WorldEconomy *e,
                          Dessein *d, int cid, int pid){
    if (!prov_ok(e,pid)) return;
    d->claim_pend=1; d->claim_pid=(int16_t)pid;
    sol_try_claim(dp,w,e,d,cid);
}
/* L'EMBAUCHE GRATUITE au siège de la branche (précédent EVID_CONSEIL_A2) : le
 * MEILLEUR candidat du vivier de la génération courante, sans coût.
 * statecraft_council_hire lève DÉJÀ la faction du recruté — on ne double pas le
 * hook ; et il refuse net un siège POURVU (P1-4 : une nomination n'écrase jamais
 * un titulaire sans renvoi explicite) : la faveur est alors sans effet, et c'est
 * la règle du Conseil, pas un oubli. */
static void sol_free_hire(Statecraft *sc, uint32_t seed, int cid, int year){
    int seat = dessein_seat_of(DESS_SOL);
    if (!sc || seat<0) return;
    if (statecraft_council_seated(sc,cid,seat) >= 0) return;
    int gen = statecraft_council_gen(year);
    int best=-1, bt=0;
    for (int sl=0; sl<SC_COUNCIL_CANDS; sl++){
        int t = statecraft_council_cand_tier(seed,cid,seat,sl,gen);
        if (t>bt){ bt=t; best=sl; }
    }
    if (best>=0) statecraft_council_hire(sc, seed, cid, seat, best, gen);
}
/* LA COORDONNÉE BÂTIE : on écrit le CHAMP SOURCE de la province NOMMÉE — jamais
 * un « bonus » plat posé à côté du moteur (discipline « on lit des coordonnées,
 * on n'assigne pas de modificateur » : ici c'est de la densité institutionnelle
 * RÉALISÉE, exactement ce qu'un édifice achevé dépose). */
static void sol_build_add(WorldEconomy *e, int pid, float dk, float dh){
    if (!prov_ok(e,pid)) return;
    e->prov[pid].build.K_inst  += dk;
    e->prov[pid].build.H_coerc += dh;
}

/* ====================================================================== */
/* API                                                                     */
/* ====================================================================== */
void missions_init(MissionsState *ms){
    if (!ms) return;
    memset(ms,0,sizeof *ms);
    for (int c=0;c<SCPS_MISSIONS_MAX;c++) for (int b=0;b<DESS_BRANCH_COUNT;b++){
        Dessein *d=&ms->d[c][b];
        d->rival=-1; d->claim_pid=-1;
        for (int k=0;k<DESSEIN_RUNGS;k++){ d->tpid[k]=-1; d->sealed_year[k]=-1; }
        for (int k=0;k<3;k++) d->trio[k]=-1;
    }
    missions_boons_sync(ms, 0);
}

const Dessein *dessein_of(const MissionsState *ms, int cid, int branch){
    if (!ms||cid<0||cid>=SCPS_MISSIONS_MAX||branch<0||branch>=DESS_BRANCH_COUNT) return NULL;
    const Dessein *d=&ms->d[cid][branch];
    return d->gen ? d : NULL;
}
int dessein_display_slot(int rung, int voie){
    if (rung<0 || rung>=DESSEIN_RUNGS) return -1;
    if (rung<DESS_RUNG_4) return rung;
    return (voie==DESS_VOIE_VASSALISATION) ? rung+4 : rung;
}
bool dessein_is_pivot (int rung){ return rung==DESS_RUNG_PIVOT; }
bool dessein_is_finale(int rung){ return rung==DESS_RUNG_7; }
/* La branche du Sol est TERRITORIALE ⇒ siège Royaume (1). Le Conseil n'a que
 * trois sièges (Savoir/Royaume/Ouvrages) — il n'y a pas de siège de la Guerre. */
int dessein_seat_of(int branch){ return (branch==DESS_SOL) ? 1 : -1; }

int dessein_target_pid(const Dessein *d){
    if (!d || d->rung<0 || d->rung>=DESSEIN_RUNGS) return -1;
    switch (d->rung){
      case DESS_RUNG_UNIFICATION:
      case DESS_RUNG_7:           return d->tpid[DESS_RUNG_UNIFICATION];
      case DESS_RUNG_EXPANSION:   return d->tpid[DESS_RUNG_EXPANSION];
      case DESS_RUNG_RIVAL:       return d->tpid[DESS_RUNG_RIVAL];
      case DESS_RUNG_4:           return (d->voie==DESS_VOIE_CONQUETE) ? d->tpid[DESS_RUNG_4] : -1;
      case DESS_RUNG_5:           return (d->voie==DESS_VOIE_CONQUETE) ? d->trio[0] : -1;
      case DESS_RUNG_6:           return (d->voie==DESS_VOIE_CONQUETE) ? d->tpid[DESS_RUNG_6] : -1;
      default: return -1;
    }
}
int dessein_target_cid(const Dessein *d, const DiploState *dp, int cid){
    if (!d || d->rung<0 || d->rung>=DESSEIN_RUNGS) return -1;
    if (d->rung==DESS_RUNG_RIVAL || (d->rung==DESS_RUNG_6 && d->voie==DESS_VOIE_CONQUETE))
        return d->rival;
    if (d->voie!=DESS_VOIE_VASSALISATION) return -1;
    if (d->rung==DESS_RUNG_4 || d->rung==DESS_RUNG_5) return d->trio[0];
    if (d->rung==DESS_RUNG_6){
        /* « Intégration » : la cible est CELUI QUI MÛRIT — réévaluée à chaque
         * lecture, jamais latchée. Départage : le premier du trio. */
        int best=-1; float bi=-1.f;
        for (int k=0;k<3;k++){
            int v=d->trio[k];
            if (v<0 || !dp || diplo_suzerain(dp,v)!=cid) continue;
            if (dp->v_integration[v] > bi){ bi=dp->v_integration[v]; best=v; }
        }
        return best;
    }
    return -1;
}

void missions_tick(MissionsState *ms, World *w, WorldEconomy *econ,
                   DiploState *dp, int year, int human_player){
    if (!ms || !w || !econ) return;
    missions_boons_sync(ms, year);          /* le miroir des remises suit l'année, toujours */
    /* P1 — JOUEUR SEUL : la chronique (human=-1) ne génère, ne résout et ne
     * détecte RIEN. Golden intact par construction (motif décrets/pending). */
    int cid = human_player;
    if (cid<0 || cid>=w->n_countries || cid>=SCPS_MISSIONS_MAX) return;
    if (!country_alive(econ,w,cid)) return;

    Dessein *d = &ms->d[cid][DESS_SOL];
    if (!d->gen){
        /* ATTRIBUTION : la branche du Sol est TOUJOURS attribuée (annexe :
         * « Éligibilité : TOUJOURS »). Aucun tirage — la géographie suffit. */
        d->gen=1; d->rung=DESS_RUNG_UNIFICATION; d->voie=DESS_VOIE_AUCUNE;
        d->tpid[DESS_RUNG_UNIFICATION] = (int16_t)cap_prov(w,econ,cid);
    }
    if (d->rung>=DESSEIN_RUNGS){ d->ready=0; return; }   /* branche ACHEVÉE */

    /* LES PREUVES D'USAGE du pivot — des LATCHES : une fois acquise, la preuve
     * ne se reperd pas (on a été ce roi-là une fois).
     * ⚠ TROUVAILLE : `conq_value[a][b]` (le budget de score dépensé, que le
     * design nommait) est SOLDÉ par diplo_make_peace À L'INTÉRIEUR MÊME de
     * diplo_settle — il est donc TOUJOURS nul vu d'une clôture mensuelle,
     * inutilisable comme prédicat. On lit la trace DURABLE du même fait :
     * `rancor[b][moi]`, incrémentée de RANCOR_PER_LOSS au SEUL site
     * settle_transfer (« une province arrachée par traité »), et qui décroît
     * lentement — la clôture du mois la voit à coup sûr. */
    if (!d->proof_a && dp)
        for (int b=0;b<w->n_countries && b<SCPS_MAX_COUNTRY;b++)
            if (b!=cid && diplo_rancor(dp,b,cid)>0.f){ d->proof_a=1; break; }
    if (!d->proof_b && dp)
        for (int b=0;b<w->n_countries && b<SCPS_MAX_COUNTRY;b++)
            if (b!=cid && diplo_suzerain(dp,b)==cid){ d->proof_b=1; break; }

    /* RÉSOLUTION : seulement si la cible courante a été DÉTRUITE (ou n'a jamais
     * été posée). Une cible passée à un tiers RESTE la cible. Aucune cible
     * atteignable ⇒ l'échelon PATIENTE, re-tenté à la clôture suivante. */
    if (!sol_targets_valid(w,econ,dp,d,cid)){
        if (sol_resolve(w,econ,dp,d,cid))
            feed_push(FEED_DESSEIN, -1, cid, reg_of(w, dessein_target_pid(d)),
                      dessein_display_slot(d->rung, d->voie));
    }
    /* Une revendication RETENUE re-tente sa pose dès que le slot se libère. */
    sol_try_claim(dp,w,econ,d,cid);

    d->ready = sol_condition(w,econ,dp,d,cid) ? 1 : 0;
}

int missions_seal(MissionsState *ms, World *w, WorldEconomy *econ,
                  DiploState *dp, Statecraft *sc, uint32_t seed, int year,
                  int cid, int branch, int rung, int voie){
    if (!ms||!w||!econ) return 0;
    if (cid<0||cid>=SCPS_MISSIONS_MAX||cid>=w->n_countries) return 0;
    if (branch<0||branch>=DESS_BRANCH_COUNT) return 0;
    Dessein *d=&ms->d[cid][branch];
    if (!d->gen || d->rung>=DESSEIN_RUNGS) return 0;
    if (rung != d->rung) return 0;                       /* on ne scelle QUE l'échelon courant */
    if (!d->ready) return 0;                             /* la condition n'est pas remplie */
    if (!country_alive(econ,w,cid)) return 0;

    if (dessein_is_pivot(rung)){
        /* LE PIVOT — irréversible, l'autre voie s'éteint. Prix UNIFORME (20
         * d'influence, D6.4), et une PREUVE D'USAGE : un pivot est un choix de
         * campagne, pas un achat au guichet. */
        if (voie!=DESS_VOIE_CONQUETE && voie!=DESS_VOIE_VASSALISATION) return 0;
        if (voie==DESS_VOIE_CONQUETE      && !d->proof_a) return 0;
        if (voie==DESS_VOIE_VASSALISATION && !d->proof_b) return 0;
        if (!dessein_pivot_pay(cid)) return 0;
        d->voie=(int8_t)voie;
    }
    d->sealed_year[rung] = (int16_t)((year<-1)?-1:(year>4096)?4096:year);

    /* ── LA RÉCOMPENSE (canaux §2.4 — jamais un lingot d'or) ─────────────── */
    int cp = cap_prov(w,econ,cid);
    switch (rung){
      case DESS_RUNG_UNIFICATION:
        sol_build_add(econ, cp, 0.6f, 0.f);              /* « le ban du sol » : K_inst +0.6 sur la capitale */
        break;
      case DESS_RUNG_EXPANSION: {
        int t2=d->tpid[DESS_RUNG_EXPANSION];
        if (prov_ok(econ,t2)) econ->prov[t2].reconstruction = 1.0f;   /* ~10 ans, décroissance native */
        break; }
      case DESS_RUNG_RIVAL: break;                       /* la remise DATÉE est portée par sealed_year */
      case DESS_RUNG_PIVOT: break;                       /* le choix EST la récompense */
      case DESS_RUNG_4:
        if (d->voie==DESS_VOIE_CONQUETE){
            int t=d->tpid[DESS_RUNG_4];
            if (prov_ok(econ,t)) econ->prov[t].reconstruction = 1.0f;
        }
        break;                                           /* la remise DATÉE (scar / opinion) suit sealed_year */
      case DESS_RUNG_5: break;
      case DESS_RUNG_6:
        sol_free_hire(sc, seed, cid, year);              /* le siège Royaume, sans coût ni grief */
        break;
      case DESS_RUNG_7:
        if (d->voie==DESS_VOIE_VASSALISATION)
            sol_build_add(econ, cp, 2.0f, 0.f);          /* « La Chambre des serments » — aucune garnison */
        else
            sol_build_add(econ, cp, 1.0f, 1.0f);         /* « La Porte du sol » */
        break;
      default: break;
    }

    /* ── L'ÉCHELON SUIVANT : on AVANCE, on RÉSOUT, puis on arme la
     *    REVENDICATION qui pointe la marche d'après (la grammaire EU4 : la
     *    récompense rend le chemin PRATICABLE). ─────────────────────────── */
    d->rung=(int8_t)(rung+1);
    d->ready=0;
    if (d->rung<DESSEIN_RUNGS){
        sol_resolve(w,econ,dp,d,cid);
        if (rung==DESS_RUNG_EXPANSION)                   /* claim sur la région de T3 */
            sol_arm_claim(dp,w,econ,d,cid,d->tpid[DESS_RUNG_RIVAL]);
        if (rung==DESS_RUNG_5 && d->voie==DESS_VOIE_CONQUETE)   /* claim sur la région de la capitale rivale */
            sol_arm_claim(dp,w,econ,d,cid,d->tpid[DESS_RUNG_6]);
        d->ready = sol_condition(w,econ,dp,d,cid) ? 1 : 0;
    }
    missions_boons_sync(ms, year);                       /* la remise fraîche est LUE dès ce tick */
    feed_push(FEED_DESSEIN, -1, cid, reg_of(w, dessein_target_pid(d)),
              dessein_display_slot(rung, d->voie));
    return 1;
}
