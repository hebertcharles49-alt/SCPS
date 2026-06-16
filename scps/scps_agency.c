/*
 * scps_agency.c — actions, temps, bâtiments (voir scps_agency.h)
 *
 * Data-driven. Les durées (jours) sont calibrées pour que 250 ans soit l'arc
 * jouable : un Tribunal en ~6 mois, une Citadelle en ~6 ans. Les deltas sont
 * des coordonnées (K/H/P…), jamais des bonus plats.
 */
#include "scps_agency.h"
#include "scps_intertrade.h"   /* #5 : le marché à 2 étages (devis + déplétion des stocks) */
#include "scps_credit.h"       /* un seul livre d'or : le chantier débite l'or national (debt-aware) */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>   /* getenv : diag de gate (SCPS_GATEDIAG) */
#include <string.h>

static const EdificeDef EDIFICES[EDIFICE_COUNT] = {
    /* {name, jours, delta, recette} — E1 : LA LOI PRIX/DURÉE, 4 paliers. LE TRIO bois/pierre/argile
     * SEUL (matériaux RAW, accumulables → le gate de matière trouve toujours de quoi sourcer ; AUCUN
     * intermédiaire manufacturé comme le métal/outils, qui ne stocke pas — foundry→toolworks au même
     * tick → le chantier serait gaté à perpétuité) :
     *   180 j : bois 40 · argile 20             (6 mois — l'édifice du quotidien)
     *   360 j : bois 80 · pierre 40 · argile 20 (12 mois — l'institution qui s'élève)
     *   540 j : pierre dominante + bois/argile  (18 mois — l'œuvre)
     *   960 j : pierre massive + bois/argile    (32 mois — le monument)
     * Même palier ≈ même ordre de prix. L'or payé = Σ qty × prix de marché courant (agency_build_gold,
     * inchangé), × multiplicateurs géo/étendue existants. Deltas E1bis.11 INCHANGÉS. */
    /* Institutionnel → K (ce qui métabolise la distance, tient la diversité). */
    [EDI_TRIBUNAL]     = { "Tribunal",      180, { .K_inst=1.0f }, {{RES_WOOD,RES_CLAY},{40,20}} },
    [EDI_CHANCELLERIE] = { "Chancellerie",  360, { .K_inst=2.5f }, {{RES_WOOD,RES_STONE,RES_CLAY},{80,40,20}} },  /* trio bois/pierre/argile */
    [EDI_ACADEMIE]     = { "Académie",      960, { .K_inst=4.0f, .P_open=0.5f },
                           {{RES_WOOD,RES_STONE,RES_CLAY},{75,190,100}} },  /* le monument : le trio seul */
    /* Coercitif → H (tient l'ordre par la force — ronge L, voie fragile). */
    [EDI_GARNISON]     = { "Garnison",      360, { .H_coerc=1.0f }, {{RES_WOOD,RES_STONE,RES_CLAY},{80,40,20}} },
    [EDI_FORTERESSE]   = { "Forteresse",    540, { .H_coerc=3.0f }, {{RES_WOOD,RES_STONE,RES_CLAY},{20,120,60}} },  /* remparts : pierre dominante */
    [EDI_CITADELLE]    = { "Citadelle",     960, { .H_coerc=6.0f },
                           {{RES_WOOD,RES_STONE,RES_CLAY},{60,220,150}} },  /* martiale : pierre massive */
    /* Ouverture → P (porte d'assimilation, contact, routes maritimes). */
    [EDI_PORT]         = { "Port",          360, { .P_open=1.0f, .port=1.0f }, {{RES_WOOD,RES_STONE,RES_CLAY},{100,40,25}} },  /* quais : bois dominant (trio) */
    [EDI_CARAVANSERAIL]= { "Caravansérail", 180, { .P_open=0.7f }, {{RES_WOOD,RES_CLAY},{45,25}} },  /* +12 % (cours, écuries) */
    /* Prospérité → PE local (capte le carrefour). */
    [EDI_MARCHE]       = { "Marché",        180, { .PE_infra=1.0f }, {{RES_WOOD,RES_CLAY},{40,15}} },
    [EDI_ENTREPOT]     = { "Entrepôt",      180, { .PE_infra=0.7f }, {{RES_WOOD,RES_CLAY},{50,20}} },  /* +25 % bois (halles) */
    /* Croissance → food (nourrit la pop ; l'aqueduc : santé urbaine → croissance). */
    [EDI_GRENIER]      = { "Grenier",       180, { .food_cap=1.0f }, {{RES_WOOD,RES_CLAY,RES_STONE},{40,15,10}} },  /* trio */
    [EDI_IRRIGATION]   = { "Irrigation",    360, { .food_cap=1.5f }, {{RES_WOOD,RES_STONE,RES_CLAY},{90,30,20}} },  /* canaux : bois (trio) */
    [EDI_AQUEDUC]      = { "Aqueduc",       540, { .food_cap=1.2f }, {{RES_WOOD,RES_STONE,RES_CLAY},{20,125,45}} },  /* arches : pierre dominante */
    /* Foi → SOUTIENT L (sacraliser le trône apaise sans réprimer — §4 du catalogue). */
    [EDI_SANCTUAIRE]   = { "Sanctuaire",    180, { .faith=1.0f }, {{RES_WOOD,RES_CLAY},{35,20}} },  /* −12 % (humble) */
    [EDI_TEMPLE]       = { "Temple",        540, { .faith=3.0f }, {{RES_WOOD,RES_STONE,RES_CLAY},{25,100,50}} },  /* pierre dominante (trio) */
    [EDI_CATHEDRALE]   = { "Cathédrale",    960, { .faith=6.5f },
                           {{RES_WOOD,RES_STONE,RES_CLAY},{45,220,120}} },  /* l'éclat de pierre (trio) */
    /* Savoir → recherche (le monastère sacralise ET étudie — §5 du catalogue). */
    [EDI_BIBLIOTHEQUE] = { "Bibliothèque",  360, { .savoir=1.5f }, {{RES_WOOD,RES_STONE,RES_CLAY},{80,35,15}} },  /* trio (le papier pèse peu) */
    [EDI_MONASTERE]    = { "Monastère",     540, { .savoir=2.5f, .faith=1.0f }, {{RES_WOOD,RES_STONE,RES_CLAY},{20,90,50}} },  /* frugal (trio) */
    /* Commerce → PE local (capte le flux ; la banque finance l'État). */
    [EDI_COMPTOIR]     = { "Comptoir",      180, { .PE_infra=0.8f }, {{RES_WOOD,RES_CLAY},{40,20}} },
    [EDI_BANQUE]       = { "Banque",        540, { .PE_infra=1.4f }, {{RES_WOOD,RES_STONE,RES_CLAY},{15,100,60}} },  /* pierre dominante (trio) */
    /* M5 (forks §9.3/§9.5) — LES FOURCHES v1. Maritime (540 j, ↑ Port — le fork
     * REMPLACE le Port, delta port conservé) : l'Arsenal projette (H), l'Amirauté
     * institue (K), le Port marchand capte (PE). Savoir (360 j, BASES alternatives
     * à la Bibliothèque) : la Bibliothèque militaire arme le savoir (H), 
     * l'Observatoire ouvre le ciel (P). Deltas §14 mappés sur ProvBuild. */
    [EDI_ARSENAL]      = { "Arsenal",       540, { .port=1.0f, .H_coerc=1.2f },
                           {{RES_WOOD,RES_STONE,RES_CLAY},{25,110,70}} },
    [EDI_AMIRAUTE]     = { "Amirauté",      540, { .port=1.0f, .K_inst=0.8f, .H_coerc=0.4f },
                           {{RES_WOOD,RES_STONE,RES_CLAY},{25,100,60}} },
    [EDI_PORT_MARCHAND]= { "Port marchand", 540, { .port=1.0f, .PE_infra=1.5f },
                           {{RES_WOOD,RES_STONE,RES_CLAY},{20,90,50}} },
    [EDI_BIBLIO_MIL]   = { "Bibliothèque militaire", 360, { .savoir=1.2f, .H_coerc=0.4f },
                           {{RES_WOOD,RES_STONE,RES_CLAY},{80,35,15}} },
    [EDI_OBSERVATOIRE] = { "Observatoire",  360, { .savoir=1.5f, .P_open=0.3f },
                           {{RES_WOOD,RES_STONE,RES_CLAY},{70,40,20}} },
    /* M2 — LE CENTRE COMMERCIAL : le hub du réseau GLOBAL, bâti COÛTEUX (œuvre côtière/
     * estuaire). Une cité-état EN naît (l'expression de son rôle) ; un empire marchand
     * côtier peut en bâtir un et devenir hub. g_centre DÉRIVE de ce bâti (plus un flag). */
    [EDI_TRADE_CENTER] = { "Centre commercial", 540, { .PE_infra=2.0f, .P_open=0.5f },
                           {{RES_WOOD,RES_STONE,RES_CLAY},{40,160,90}} },
};

const EdificeDef *edifice_def(Edifice e){ return (e>=0&&e<EDIFICE_COUNT)?&EDIFICES[e]:NULL; }
/* K2 — edifice_name() a MIGRÉ au readout (membrane : le moteur n'expose que l'enum
 * Edifice + EDIFICES[].name comme défaut FR de référence ; la traduction vit au readout). */

/* ── E1bis.11 — FAMILLES ↑ ───────────────────────────────────────────────────
 * Le palier précédent d'un édifice familial (EDIFICE_COUNT = base ou singleton). */
Edifice edifice_prev(Edifice e){
    switch(e){
        case EDI_CHANCELLERIE: return EDI_TRIBUNAL;
        case EDI_ACADEMIE:     return EDI_CHANCELLERIE;
        case EDI_FORTERESSE:   return EDI_GARNISON;
        case EDI_CITADELLE:    return EDI_FORTERESSE;
        case EDI_TEMPLE:       return EDI_SANCTUAIRE;
        case EDI_CATHEDRALE:   return EDI_TEMPLE;
        case EDI_MONASTERE:    return EDI_BIBLIOTHEQUE;
        case EDI_ARSENAL: case EDI_AMIRAUTE: case EDI_PORT_MARCHAND:
                               return EDI_PORT;            /* M5 : la fourche ↑ le Port */
        default:               return EDIFICE_COUNT;
    }
}
Edifice edifice_succ(Edifice e){   /* palier SUIVANT (EDIFICE_COUNT = sommet/singleton) */
    switch(e){
        case EDI_TRIBUNAL:    return EDI_CHANCELLERIE;
        case EDI_CHANCELLERIE:return EDI_ACADEMIE;
        case EDI_GARNISON:    return EDI_FORTERESSE;
        case EDI_FORTERESSE:  return EDI_CITADELLE;
        case EDI_SANCTUAIRE:  return EDI_TEMPLE;
        case EDI_TEMPLE:      return EDI_CATHEDRALE;
        case EDI_BIBLIOTHEQUE:return EDI_MONASTERE;
        default:             return EDIFICE_COUNT;
    }
}
/* le palier BÂTISSABLE d'une famille (base `b`) dans la région : on remonte la
 * chaîne tant que le palier est bâti, et l'on renvoie le premier NON bâti (le ↑ à
 * poser), ou EDIFICE_COUNT si le sommet est déjà atteint. */
Edifice edifice_next_buildable(const WorldEconomy *econ, int region, Edifice base){
    if (region<0||region>=econ->n_regions) return base;
    uint32_t built=econ->region[region].edi_built;
    Edifice e=base;
    while (e<EDIFICE_COUNT){
        if (!(built & (1u<<e))) return e;          /* ce palier manque → c'est lui qu'on pose */
        Edifice s=edifice_succ(e);
        if (s>=EDIFICE_COUNT) return EDIFICE_COUNT;  /* sommet déjà bâti */
        e=s;
    }
    return EDIFICE_COUNT;
}
/* ── M3 (forks §8) — LA SUCCESSION CONTEXTUELLE + L'HYSTÉRÉSIS ────────────── */
/* La variante de `base` sous un pôle (EDIFICE_COUNT si base non forkée). Seul le
 * PORT fork son successeur en v1 (§23.1) ; le savoir fork à la BASE (routing IA). */
Edifice edifice_fork_successor(Edifice base, TechPole pole){
    if (base==EDI_PORT){
        switch(pole){
            case POLE_MARTIAL: return EDI_ARSENAL;
            case POLE_ORDRE:   return EDI_AMIRAUTE;
            case POLE_FLUIDE:  return EDI_PORT_MARCHAND;
            default:           return EDIFICE_COUNT;
        }
    }
    return EDIFICE_COUNT;
}
/* Le pôle VALIDÉ d'une région : lu des factions de SA population, tie-breaks §7
 * (capitale → pôle impérial, lu de la capitale du propriétaire ; port ; frontière),
 * et l'HYSTÉRÉSIS — un candidat divergent doit tenir ≥ 360 j avant d'être lu
 * (RegionEconomy.last_pole = validé · pole_since_day = depuis quand on diverge). */
#define POLE_HOLD_DAYS 360
static TechPole region_pole_raw(const World *w, const WorldEconomy *econ, int region, int imperial){
    const RegionEconomy *re=&econ->region[region];
    float wgt[FAC_COUNT];
    faction_weights_of(&re->pop, 1, wgt);
    bool port=(re->build.port>0.f) || (re->edi_built & (1u<<EDI_PORT));
    bool border=false;
    for (int sn=0; sn<econ->n_regions && !border; sn++)
        if (econ->adj[region][sn] && econ->region[sn].owner>=0
            && econ->region[sn].owner!=re->owner) border=true;
    (void)w;
    return faction_pole_of(wgt, imperial, port, border);
}
TechPole edifice_region_pole(const World *w, WorldEconomy *econ, int region, int day){
    if (!econ || region<0 || region>=econ->n_regions) return POLE_ORDRE;
    RegionEconomy *re=&econ->region[region];
    /* le pôle impérial (tie-break capitale) : la capitale du propriétaire, SANS
     * récursion (elle se lit elle-même avec imperial=-1). */
    int imperial=-1;
    if (w && re->owner>=0 && re->owner<w->n_countries){
        int cp=w->country[re->owner].capital_prov;
        int cr=(cp>=0&&cp<w->n_provinces)?w->province[cp].region:-1;
        if (cr==region) imperial=-1;                       /* JE SUIS la capitale */
        else if (cr>=0 && cr<econ->n_regions) imperial=(int)region_pole_raw(w,econ,cr,-1);
    }
    TechPole cur = region_pole_raw(w, econ, region, imperial);
    if ((int)cur == (int)re->last_pole){ re->pole_since_day=0; return cur; }
    if (re->pole_since_day==0){ re->pole_since_day=day>0?day:1; return (TechPole)re->last_pole; }
    if (day - re->pole_since_day >= POLE_HOLD_DAYS){
        re->last_pole=(int8_t)cur; re->pole_since_day=0; return cur;     /* le candidat a TENU */
    }
    return (TechPole)re->last_pole;                        /* il n'a pas tenu 360 j : on garde */
}
Edifice edifice_succ_ctx(const World *w, WorldEconomy *econ, int region, Edifice base, int day){
    return edifice_fork_successor(base, edifice_region_pole(w, econ, region, day));
}

/* membre d'une FAMILLE ↑ (base ou palier) : on suit son masque & on gâte la pose.
 * Les autres (Grenier, Marché, Port, Entrepôt, Comptoir, Caravansérail, Irrigation,
 * Aqueduc, Banque) restent des poses INDÉPENDANTES (s'empilent — l'IA en dépend). */
static bool edi_is_family(Edifice e){
    switch(e){
        case EDI_TRIBUNAL: case EDI_CHANCELLERIE: case EDI_ACADEMIE:
        case EDI_GARNISON: case EDI_FORTERESSE:   case EDI_CITADELLE:
        case EDI_SANCTUAIRE: case EDI_TEMPLE:     case EDI_CATHEDRALE:
        case EDI_BIBLIOTHEQUE: case EDI_MONASTERE: return true;
        case EDI_ARSENAL: case EDI_AMIRAUTE: case EDI_PORT_MARCHAND:
        case EDI_BIBLIO_MIL: case EDI_OBSERVATOIRE: return true;   /* M5 */
        default: return false;
    }
}
/* La pose est-elle BLOQUÉE ? Un membre déjà bâti (on passe à l'↑) ou un ↑ sans son
 * palier précédent → refus. Un singleton n'est jamais bloqué. */
/* M3 (forks §8) — les FRÈRES d'une fourche : dès qu'un membre est bâti dans la
 * région, les autres branches y sont BLOQUÉES (reconversion = démolition, hors v1).
 * Maritime : Arsenal/Amirauté/Port marchand. Savoir : Bibliothèque militaire /
 * Bibliothèque (→ Monastère, la branche Ordre) / Observatoire. */
static uint32_t edi_brothers_mask(Edifice e){
    const uint32_t MAR = (1u<<EDI_ARSENAL)|(1u<<EDI_AMIRAUTE)|(1u<<EDI_PORT_MARCHAND);
    const uint32_t SAV = (1u<<EDI_BIBLIO_MIL)|(1u<<EDI_BIBLIOTHEQUE)|(1u<<EDI_OBSERVATOIRE)|(1u<<EDI_MONASTERE);
    switch(e){
        case EDI_ARSENAL: case EDI_AMIRAUTE: case EDI_PORT_MARCHAND: return MAR;
        case EDI_BIBLIO_MIL: case EDI_BIBLIOTHEQUE: case EDI_OBSERVATOIRE: return SAV;
        default: return 0u;
    }
}
bool edifice_build_blocked(const WorldEconomy *econ, int region, Edifice e){
    if (region<0 || region>=econ->n_regions || !edi_is_family(e)) return false;
    { uint32_t bro = edi_brothers_mask(e);
      if (bro){
          uint32_t built = econ->region[region].edi_built & bro & ~(1u<<e);
          /* un FRÈRE (autre branche) déjà bâti → bloqué — SAUF son propre palier
           * précédent dans la même branche (Bibliothèque → Monastère reste licite). */
          Edifice p0 = edifice_prev(e);
          if (p0 < EDIFICE_COUNT) built &= ~(1u<<p0);
          if (built) return true;
      } }
    uint32_t built = econ->region[region].edi_built;
    if (built & (1u<<e)) return true;                       /* déjà bâti : pas de doublon */
    Edifice p = edifice_prev(e);
    if (p<EDIFICE_COUNT && !(built & (1u<<p))) return true;  /* ↑ sans le palier précédent */
    return false;
}

/* ---- Coût des bâtiments (§1) : matériaux ACHETÉS au marché en or ------- */
#define BUILD_MIN_PRICE 0.20f   /* plancher de prix : même un bien abondant n'est jamais gratuit */
#define IMPORT_TOLL_FRAC 0.30f  /* I6 : part de la marge versée en PÉAGE au hub tiers emprunté */
/* DIAGNOSTIC G0.3 — pourquoi les paliers ne montent pas : compteurs par édifice. */
static long g_edi_made[EDIFICE_COUNT], g_edi_blocked[EDIFICE_COUNT], g_edi_nogold[EDIFICE_COUNT], g_edi_nomat[EDIFICE_COUNT];

/* §7 — l'ÉTENDUE du pays RENCHÉRIT ses institutions (le frein tall/wide qui manquait) :
 * facteur ×(1 + 0.15·n_régions du pays) sur le coût matériaux. Un grand empire paie
 * ses édifices bien plus cher — sa croissance institutionnelle se paie. */
static float agency_extent_mult(const WorldEconomy *econ, int region){
    int owner = econ->region[region].owner;
    if (owner < 0) return 1.f;
    int n=0;
    for (int r=0;r<econ->n_regions;r++) if (econ->region[r].owner==owner) n++;
    return 1.f + 0.15f*(float)n;
}

/* #5 — LE DEVIS DU CHANTIER, marché à 2 étages. Pour chaque matériau, la quantité
 * réellement consommée (qty × étendue) est SOURCÉE au marché EMPIRE-AWARE : matière de
 * l'empire GRATUITE → Centre étranger le plus proche (×marge) → réseau étranger (×marge×2).
 * La couche commerce calcule (intertrade_buy_cost), agency LIT. `base_out` (option) = le NU
 * (marge 1) de la part IMPORTÉE seule (l'empire est gratuit, il n'entre pas dans le nu) →
 * (gold − base_out) = la marge de transport, routée en péage à la cité-état hôte. */
static float agency_build_gold_ex(const WorldEconomy *econ, int region, Edifice e, float *base_out){
    if (base_out) *base_out=0.f;
    if (e<0||e>=EDIFICE_COUNT || !econ || region<0 || region>=econ->n_regions) return 0.f;
    const RegionEconomy *re=&econ->region[region];
    const BuildCost *c=&EDIFICES[e].cost;
    float ext = agency_extent_mult(econ, region);   /* §7 : un grand empire paie plus */
    float gold=0.f, base=0.f;
    for (int k=0;k<BUILD_RES_MAX;k++){
        Resource r=c->res[k];
        if (r<=RES_NONE || r>=RES_COUNT || c->qty[k]<=0.f) continue;
        float price = re->price[r]; if (price < BUILD_MIN_PRICE) price = BUILD_MIN_PRICE;
        float qy = c->qty[k]*ext;
        float ib=0.f;
        gold += intertrade_buy_cost(econ, region, r, qy, price, &ib);   /* import seul facturé (empire gratuit) */
        base += ib;                                                     /* le NU de l'IMPORT (marge 1) → base du péage */
    }
    if (base_out) *base_out=base;
    return gold;
}
float agency_build_gold(const WorldEconomy *econ, int region, Edifice e){
    return agency_build_gold_ex(econ, region, e, NULL);
}

bool agency_build_acct(AgencyState *a, WorldEconomy *econ, const World *w, int region, Edifice e, int owner){
    if (e<0||e>=EDIFICE_COUNT || !econ || region<0 || region>=econ->n_regions) return false;
    if (e==EDI_PORT && !econ->region[region].coastal) return false;   /* un port se bâtit SUR la côte (mer §5) */
    if (e==EDI_TRADE_CENTER && !econ->region[region].coastal && !econ->region[region].estuary)
        return false;                                                 /* M2 : un Centre = un débouché (côtier/estuaire) */
    if (edifice_build_blocked(econ, region, e)){ g_edi_blocked[e]++; return false; }  /* E1bis.11 : ↑ exige le palier précédent (pas de doublon) */
    /* Gate de MATIÈRE (refus sec) : une seule matière introuvable au marché atteignable
     * (propre + Centre le plus proche + réseau des Centres) → chantier REFUSÉ. Pas de file
     * d'attente. Scarce = cher (marges) ; absent partout = pas de chantier. */
    { const BuildCost *c=&EDIFICES[e].cost; float ext=agency_extent_mult(econ,region);
      for (int k=0;k<BUILD_RES_MAX;k++){ Resource r=c->res[k];
          if (r<=RES_NONE||r>=RES_COUNT||c->qty[k]<=0.f) continue;
          float av=intertrade_market_avail(econ,region,r);
          if (c->qty[k]*ext > av+1e-3f){
              if (getenv("SCPS_GATEDIAG")) fprintf(stderr,"[GATE] %s rég %d : res %d besoin %.0f (×ext %.1f) > dispo %.0f\n",
                                                   EDIFICES[e].name, region, (int)r, c->qty[k]*ext, ext, av);
              g_edi_nomat[e]++; return false; }
      } }
    RegionEconomy *re=&econ->region[region];
    float base_gold=0.f;
    float gold = agency_build_gold_ex(econ, region, e, &base_gold);
    /* UN SEUL LIVRE D'OR : on débite l'or NATIONAL de `owner` via le module de crédit
     * (au-delà du trésor → dette, créancier assigné). Bloque seulement au-delà de la
     * ligne de crédit ; sinon le chantier passe et l'or peut filer négatif. */
    if (!credit_can_spend(econ, w, owner, gold)){ g_edi_nogold[e]++; return false; }
    credit_spend(econ, w, owner, gold);
    /* I6/#5 — LE PÉAGE : la marge au-dessus du nu (transport + double taxe mondiale)
     * versée à la cité-état hôte (le hub le plus proche) — transfert, pas destruction :
     * les cités-états deviennent banquières du réseau. */
    if (gold > base_gold + 0.01f && re->import_toll_region >= 0 && re->import_toll_region < econ->n_regions){
        float toll = (gold - base_gold);   /* CONSERVATION : TOUTE la marge → l'hôte (le nu va aux sources via _consume) */
        econ->region[re->import_toll_region].treasury += toll;
        if (re->owner>=0) econ_flux_add(re->owner, FX_TOLL_PAID, -toll);                       /* I0 */
        int tro=econ->region[re->import_toll_region].owner; if (tro>=0) econ_flux_add(tro, FX_TOLL_RECV, toll);
    }
    const BuildCost *c=&EDIFICES[e].cost;          /* … et l'on POMPE les matériaux du marché (vrais stocks) */
    float mult = agency_extent_mult(econ, region); /* §7 : un grand pays consomme plus */
    for (int k=0;k<BUILD_RES_MAX;k++){
        Resource r=c->res[k];
        if (r<=RES_NONE || r>=RES_COUNT || c->qty[k]<=0.f) continue;
        intertrade_market_consume(econ, region, r, c->qty[k]*mult,
                                  (re->price[r]<BUILD_MIN_PRICE?BUILD_MIN_PRICE:re->price[r]));   /* nu → sources étrangères */
    }
    bool ok=agency_order_build(a, region, e);      /* enfile le chantier (durée existante) */
    if (ok) g_edi_made[e]++;
    return ok;
}
bool agency_build(AgencyState *a, WorldEconomy *econ, const World *w, int region, Edifice e){
    return agency_build_acct(a, econ, w, region, e, (econ&&region>=0&&region<econ->n_regions)?econ->region[region].owner:-1);
}
TechId edifice_gate_tech(Edifice e){
    switch (e){
        case EDI_COMPTOIR: return TECH_COMPTOIRS;
        case EDI_ENTREPOT: return TECH_HALLES;
        default:           return TECH_COUNT;
    }
}
bool edifice_unlocked(const TechState *ts, Edifice e){
    if (!ts) return true;
    TechId t=edifice_gate_tech(e);
    return (t>=TECH_COUNT) || ts->unlocked[t];
}

/* Constantes des actions non-bâtiment (calibrables). */
#define CLEAR_DAYS        200
#define EXPLOIT_DAYS      180
#define CLEAR_FOOD_GAIN   1.5f
#define CLEAR_SUBS_TARGET 6.0f    /* mode de vie agricole (FARMER) */
#define CLEAR_SUBS_SHIFT  0.40f   /* fraction du chemin à l'achèvement (le reste dérive) */
#define CLEAR_L_HIT       2.0f
#define EXPLOIT_CAP_GAIN  3.0f

/* coûts SCPS différés (drainés par le harnais vers TechState) + chronique */
static float g_pend_charge[SCPS_MAX_COUNTRY], g_pend_fract[SCPS_MAX_COUNTRY], g_pend_H[SCPS_MAX_COUNTRY];
static int   g_n_repress, g_n_assim, g_n_purge; static long g_purge_dead;
void agency_edi_dump(void){
    fprintf(stderr,"[EDI] par édifice — made / blocked(palier précédent) / nogold :\n");
    for (int e=0;e<EDIFICE_COUNT;e++)
        if (g_edi_made[e]||g_edi_blocked[e]||g_edi_nogold[e]||g_edi_nomat[e])
            fprintf(stderr,"  %-14s %4dj  made=%-5ld blocked=%-6ld nogold=%-6ld nomat=%-6ld\n",
                    EDIFICES[e].name, EDIFICES[e].days, g_edi_made[e], g_edi_blocked[e], g_edi_nogold[e], g_edi_nomat[e]);
}

void agency_init(AgencyState *a){
    memset(a,0,sizeof(*a));
    /* RAZ des coûts différés + de la chronique des leviers (statiques de module) */
    memset(g_pend_charge,0,sizeof g_pend_charge);
    memset(g_pend_fract, 0,sizeof g_pend_fract);
    memset(g_pend_H,     0,sizeof g_pend_H);
    g_n_repress=g_n_assim=g_n_purge=0; g_purge_dead=0;
    memset(g_edi_made,0,sizeof g_edi_made); memset(g_edi_blocked,0,sizeof g_edi_blocked);
    memset(g_edi_nogold,0,sizeof g_edi_nogold); memset(g_edi_nomat,0,sizeof g_edi_nomat);
}

void agency_save(FILE *f){
    fwrite(g_pend_charge,sizeof g_pend_charge,1,f);
    fwrite(g_pend_fract, sizeof g_pend_fract, 1,f);
    fwrite(g_pend_H,     sizeof g_pend_H,     1,f);
    fwrite(&g_n_repress,sizeof g_n_repress,1,f); fwrite(&g_n_assim,sizeof g_n_assim,1,f);
    fwrite(&g_n_purge,sizeof g_n_purge,1,f);     fwrite(&g_purge_dead,sizeof g_purge_dead,1,f);
}
bool agency_load(FILE *f){
    return fread(g_pend_charge,sizeof g_pend_charge,1,f)==1
        && fread(g_pend_fract, sizeof g_pend_fract, 1,f)==1
        && fread(g_pend_H,     sizeof g_pend_H,     1,f)==1
        && fread(&g_n_repress,sizeof g_n_repress,1,f)==1 && fread(&g_n_assim,sizeof g_n_assim,1,f)==1
        && fread(&g_n_purge,sizeof g_n_purge,1,f)==1     && fread(&g_purge_dead,sizeof g_purge_dead,1,f)==1;
}

static bool enqueue(AgencyState *a, ActionKind k, int region, int param, int days){
    if (a->n>=SCPS_MAX_BUILDS) return false;
    BuildOrder *o=&a->order[a->n++];
    o->kind=k; o->region=region; o->param=param;
    o->days_total=days; o->days_done=0; o->active=true;
    return true;
}
bool agency_order_build(AgencyState *a, int region, Edifice e){
    if (e<0||e>=EDIFICE_COUNT) return false;
    return enqueue(a, AGY_BUILD, region, (int)e, EDIFICES[e].days);
}
bool agency_order_clear(AgencyState *a, int region){
    return enqueue(a, AGY_CLEAR, region, 0, CLEAR_DAYS);
}
bool agency_order_exploit(AgencyState *a, int region, Resource res){
    if (res<=RES_NONE||res>=RES_COUNT) return false;
    return enqueue(a, AGY_EXPLOIT, region, (int)res, EXPLOIT_DAYS);
}
#define RELOC_DAYS 90   /* un déplacement de familles prend une saison */
bool agency_order_relocate(AgencyState *a, int region, int dst_region){
    if (region<0 || dst_region<0 || region==dst_region) return false;
    return enqueue(a, AGY_RELOCATE, region, dst_region, RELOC_DAYS);
}
#define COLONIZE_DAYS 180   /* E1 : le convoi colonisateur marche 6 mois */
bool agency_order_colonize(AgencyState *a, int dst_region, int src_region){
    if (dst_region<0 || src_region<0 || dst_region==src_region) return false;
    return enqueue(a, AGY_COLONIZE, dst_region, src_region, COLONIZE_DAYS);
}

/* ── LES TROIS LEVIERS INTÉRIEURS (§2) ─────────────────────────────────────── */
#define REPRESS_DAYS    30
#define ASSIM_DAYS      365
#define PURGE_FRAC_AN   0.12f   /* fraction du groupe qui périt par tranche annuelle */

bool agency_order_repress(AgencyState *a, int region){
    return enqueue(a, AGY_REPRESS, region, 0, REPRESS_DAYS);
}
bool agency_order_assimilate(AgencyState *a, int region, bool creuset){
    return enqueue(a, AGY_ASSIMILATE, region, creuset?1:0, ASSIM_DAYS);
}
bool agency_order_purge(AgencyState *a, int region){
    return enqueue(a, AGY_PURGE, region, 0, AGY_PURGE_YEARS*365);
}
bool agency_drain_levier_costs(int cid, float *charge, float *fracture, float *H){
    if (cid<0||cid>=SCPS_MAX_COUNTRY) return false;
    if (g_pend_charge[cid]<=0.f && g_pend_fract[cid]<=0.f && g_pend_H[cid]<=0.f) return false;
    if (charge)  *charge  =g_pend_charge[cid];
    if (fracture)*fracture=g_pend_fract[cid];
    if (H)       *H       =g_pend_H[cid];
    g_pend_charge[cid]=g_pend_fract[cid]=g_pend_H[cid]=0.f;
    return true;
}
void agency_levier_stats(int *r,int *as,int *p,long *dead){
    if(r) *r=g_n_repress;
    if(as) *as=g_n_assim;
    if(p) *p=g_n_purge;
    if(dead) *dead=g_purge_dead;
}
static void pend_costs(int owner, float ch, float fr, float h){
    if (owner<0||owner>=SCPS_MAX_COUNTRY) return;
    g_pend_charge[owner]+=ch; g_pend_fract[owner]+=fr; g_pend_H[owner]+=h;
}
/* le plus gros groupe MINORITAIRE d'une province (cible de former/purger) ; -1 si homogène */
static int biggest_minority(const ProvincePop *pp){
    if (!pp || pp->n_groups<2) return -1;
    int dom=0; for (int g=1;g<pp->n_groups;g++) if (pp->groups[g].count>pp->groups[dom].count) dom=g;
    int best=-1; long bc=0;
    for (int g=0;g<pp->n_groups;g++){
        if (g==dom) continue;
        if (pp->groups[g].count>bc){ bc=pp->groups[g].count; best=g; }
    }
    return best;
}
/* une TRANCHE annuelle de purge : le groupe meurt par fraction, la province saigne. */
static void purge_slice(WorldEconomy *econ, WorldLegitimacy *wl, int reg){
    RegionEconomy *re=&econ->region[reg];
    int gi=biggest_minority(&re->pop);
    if (gi<0) return;                                  /* plus de minorité : la purge s'éteint */
    PopGroup *pg=&re->pop.groups[gi];
    long dead=(long)((float)pg->count*PURGE_FRAC_AN);
    if (dead<1) dead=pg->count;
    pg->count-=dead; if (pg->count<0) pg->count=0;
    g_purge_dead+=dead;
    /* la population régionale saigne d'autant (strates au prorata) */
    float tot=0.f; for (int c=0;c<CLASS_COUNT;c++) tot+=re->strata[c].pop;
    if (tot>1.f){
        float k=1.f-(float)dead/tot; if (k<0.f) k=0.f;
        for (int c=0;c<CLASS_COUNT;c++) re->strata[c].pop*=k;
    }
    re->coercion=1.f;                                   /* l'état d'exception */
    re->revolt_scar=1.f;                                /* la stabilité plonge des années (décroît lentement) */
    if (reg<SCPS_MAX_REG && wl) wl->L[reg]=fminf(wl->L[reg],1.0f);   /* la légitimité au plancher */
    pend_costs(re->owner, 1.2f, 1.0f, 0.8f);            /* charge faustienne + fracture + H — la Brèche se rapproche */
}
bool agency_cancel(AgencyState *a, int idx){
    if (idx<0 || idx>=a->n || !a->order[idx].active) return false;
    a->order[idx]=a->order[--a->n];   /* révoqué : swap-remove (rien n'est appliqué) */
    return true;
}

static void apply_delta(ProvBuild *b, const ProvBuild *d){
    b->K_inst  += d->K_inst;  b->H_coerc += d->H_coerc;  b->P_open += d->P_open;
    b->port    += d->port;
    b->PE_infra+= d->PE_infra; b->food_cap += d->food_cap;
    b->savoir  += d->savoir;  b->faith   += d->faith;     /* G0.3 : savoir/faith étaient OUBLIÉS — */
}                                                          /* d'où les chaînes bloquées (Biblio→Monastère, Sanct.→Cathédrale) */
static void remove_delta(ProvBuild *b, const ProvBuild *d){   /* E1bis.11 : l'↑ EFFACE le palier précédent */
    b->K_inst  -= d->K_inst;  b->H_coerc -= d->H_coerc;  b->P_open -= d->P_open;
    b->port    -= d->port;
    b->PE_infra-= d->PE_infra; b->food_cap -= d->food_cap;
    b->savoir  -= d->savoir;  b->faith   -= d->faith;
}

static void apply_action(WorldEconomy *econ, WorldLegitimacy *wl, ModifierStack *drift,
                         const BuildOrder *o){
    int reg=o->region;
    if (reg<0 || reg>=econ->n_regions) return;
    RegionEconomy *re=&econ->region[reg];
    switch (o->kind){
        case AGY_BUILD: {
            Edifice e=(Edifice)o->param;
            Edifice prev=edifice_prev(e);                          /* E1bis.11 : ↑ REMPLACE le palier précédent */
            if (prev<EDIFICE_COUNT && (re->edi_built & (1u<<prev))){
                remove_delta(&re->build, &EDIFICES[prev].delta);
                re->edi_built &= ~(1u<<prev);
            }
            apply_delta(&re->build, &EDIFICES[e].delta);
            if (e<32) re->edi_built |= (1u<<e);                    /* on suit l'édifice posé (masque) */
            if (e==EDI_ENTREPOT && re->n_entrepot<250) re->n_entrepot++;   /* E2 §11 : chaque entrepôt +500 de cap */
        } break;
        case AGY_CLEAR:
            re->build.food_cap += CLEAR_FOOD_GAIN;
            /* dérive du substrat vers l'agriculture (impérialisme sur la terre) */
            re->culture.subsistance += (CLEAR_SUBS_TARGET - re->culture.subsistance)*CLEAR_SUBS_SHIFT;
            /* niche forestière (chasseurs/horticulteurs) : leur monde rasé → L↓ */
            if ((re->culture.lifeway==LIFE_HUNTER || re->culture.lifeway==LIFE_HORTICULTURE)
                && wl && reg<SCPS_MAX_REG)
                wl->L[reg] = (wl->L[reg]>CLEAR_L_HIT) ? wl->L[reg]-CLEAR_L_HIT : 0.f;
            break;
        case AGY_EXPLOIT:
            if (o->param>RES_NONE && o->param<RES_COUNT)
                re->raw_cap[o->param] += EXPLOIT_CAP_GAIN;
            break;
        case AGY_RELOCATE:
            /* le convoi arrive : les familles s'installent (coercition à la source,
             * déjà câblée dans econ_relocate_pop — le coût annoncé AVANT l'ordre). */
            econ_relocate_pop(econ, o->region, o->param, (float)AGY_RELOC_POP);
            break;
        case AGY_REPRESS:
            /* MATER : la botte s'abat — Kuran (l'agitation se TAIT, le grief est
             * MASQUÉ : il ressortira amplifié quand la botte se lèvera). H différé. */
            province_apply_coercion(&re->pop, drift, 4.f + re->build.H_coerc);
            re->coercion = fminf(1.f, re->coercion + 0.5f);
            pend_costs(re->owner, 0.f, 0.f, 0.4f);
            g_n_repress++;
            break;
        case AGY_ASSIMILATE: {
            /* FORMER : écoles/missions/magistrats — l'assimilation du plus gros groupe
             * minoritaire s'accélère (creuset = ×2). Frottement : coercition modérée,
             * humeur du groupe dégradée le temps de la conversion. */
            int gi=biggest_minority(&re->pop);
            if (gi>=0){
                PopGroup *pg=&re->pop.groups[gi];
                pg->integration = fminf(1.f, pg->integration + (o->param? 0.50f : 0.25f));
                pg->L = fmaxf(0.f, pg->L - 0.5f);
                re->coercion = fminf(1.f, re->coercion + 0.10f);
                g_n_assim++;
            }
        } break;
        case AGY_PURGE:
            /* la DERNIÈRE tranche (les précédentes tombent aux bornes annuelles
             * dans agency_advance) ; la purge achevée se compte. */
            purge_slice(econ, wl, reg);
            g_n_purge++;
            break;
        case AGY_COLONIZE: {
            /* E1 : le convoi ARRIVE — la région se peuple depuis sa source. Si la
             * source a changé de couronne en route, le convoi s'est perdu. */
            int src=o->param;
            if (src>=0 && src<econ->n_regions && econ->region[src].owner>=0)
                econ_colonize_from(econ, src, reg, econ->region[src].owner);
        } break;
    }
}

void agency_advance(AgencyState *a, World *w, WorldEconomy *econ,
                    WorldLegitimacy *wl, ModifierStack *drift, int days){
    (void)w;
    a->day += days;
    for (int i=a->n-1; i>=0; i--){
        BuildOrder *o=&a->order[i];
        if (!o->active) continue;
        int before=o->days_done;
        o->days_done += days;
        /* PURGE : un PROCESSUS par tranches annuelles VISIBLES (arrêtable en cours
         * au prix du gâchis) — une tranche tombe à chaque borne de 365 j franchie. */
        if (o->kind==AGY_PURGE){
            int y0=before/365, y1=o->days_done/365;
            for (int y=y0; y<y1 && y<AGY_PURGE_YEARS-1; y++)
                purge_slice(econ, wl, o->region);
        }
        if (o->days_done >= o->days_total){
            apply_action(econ, wl, drift, o);
            a->order[i]=a->order[--a->n];   /* achevé : swap-remove */
        }
    }
}

int agency_active_in_region(const AgencyState *a, int region){
    int n=0;
    for (int i=0;i<a->n;i++) if (a->order[i].active && a->order[i].region==region) n++;
    return n;
}
int agency_year(const AgencyState *a){ return a->day / SCPS_DAYS_PER_YEAR; }
