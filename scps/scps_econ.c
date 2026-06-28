/*
 * scps_econ.c — moteur de simulation économique (voir scps_econ.h)
 *
 * Modèle volontairement causal et lisible : chaque chiffre a une raison
 * géographique ou démographique. Aucune valeur n'est posée « au hasard » à
 * la simulation — l'aléa est dans la génération du monde, pas dans l'éco.
 */
#include "scps_econ.h"
#include "scps_tune.h"    /* Arc J : constantes de calibrage surchargeables (SCPS_TUNE) */
#include "scps_world.h"   /* resource_name(), subsistance_for_biome() */
#include "scps_culture.h" /* culture_content_distance() pour la novelty diaspora */
#include "scps_religion.h"/* P4 : nudge démographie/coordonnées par la religion (gated) */
#include "scps_labor.h"   /* capitale_* : la productivité de la capitale booste la prod réelle */
#include "scps_factions.h"/* §C3 : faction_capture_total → le « rot » qui mine l'efficacité noble */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ====================================================================== */
/* TABLES DE CONSTANTES                                                    */
/* ====================================================================== */

/* Prix de base par bien (monnaie/unité). Brutes bon marché, manufacturés
 * chers, luxe très cher. Sert d'ancre au prix de marché. */
static const float BASE_PRICE[RES_COUNT] = {
    [RES_NONE]          = 0.f,
    /* brutes agricoles */
    [RES_GRAIN]         = 1.0f,
    [RES_LIVESTOCK]     = 1.6f,
    [RES_WOOL]          = 1.8f,
    [RES_FISH]          = 1.2f,
    [RES_FUR]           = 3.0f,
    [RES_SALT]          = 2.2f,
    [RES_COTTON]        = 1.9f,
    [RES_SUGAR]         = 2.0f,
    [RES_WOOD]          = 1.0f,
    [RES_FRUIT]         = 1.8f,    /* fruits — douceur commune (repli du vin), comme le sucre */
    [RES_MED_HERBS]     = 3.5f,
    /* brutes minérales */
    [RES_COPPER]        = 2.6f,
    [RES_IRON]          = 2.4f,
    [RES_COAL]          = 1.8f,
    [RES_SULFUR]        = 3.0f,
    [RES_SALTPETER]     = 3.2f,
    [RES_GOLD]          = 8.0f,
    [RES_PRECIOUS_METAL]= 12.0f,
    [RES_PEARL]         = 12.0f,   /* perle : prix habituel d'une ressource précieuse (≈ métal préc.) */
    [RES_MUREX]         = 11.0f,   /* teinture pourpre — rare comme une précieuse (gate du luxe-tissu) */
    [RES_INDIGO]        = 10.0f,   /* teinture bleue — bas-pays chaud (gate alternatif du luxe-tissu) */
    [RES_CLAY]          = 0.6f,    /* E1 : vrac de construction — bon marché, partout un peu */
    [RES_STONE]         = 0.8f,    /* E1 : vrac de construction — le relief le donne */
    /* manufacturés */
    [RES_CLOTH]         = 4.5f,
    [RES_TUNIQUE]       = 5.5f,    /* vêtement fini du commun — étoffe + façon (un cran au-dessus du tissu) */
    [RES_NAVAL_SUPPLIES]= 4.0f,
    [RES_WINE]          = 5.0f,
    [RES_BEER]          = 3.0f,    /* la boisson du commun — moins chère que le vin */
    [RES_PRECIOUS_WARE] = 22.0f,
    [RES_PRECIOUS_CLOTH]= 18.0f,
    [RES_PAPER]         = 5.5f,
    [RES_ARCANE_CRYSTAL]= 16.0f,   /* résidu rare des nœuds telluriques */
    [RES_ESSENCE]       = 34.0f,   /* mana raffiné — très haute valeur */
    [RES_FLUX]          = 12.0f,   /* F3 : salpêtre distillé (Alambic) — intrant du Réplicateur (flux → bois) */
    [RES_ALCHEMIST_KIT] = 34.0f,   /* F1 : nécessaire d'alchimiste (Alambic, secondaire) — soldat alchimiste */
    [RES_ARMS_HEAVY]    = 14.0f,   /* F1 : armes lourdes (fer ×3) */
    [RES_ARMS_RANGED]   = 10.0f,   /* F1 : armes de trait (fer + bois) */
    [RES_FIREARM]       = 16.0f,   /* F1 : armes à feu (cuivre + fer + poudre) */
    [RES_MAGE_STAFF]    = 30.0f,   /* F1 : bâton de mage (atelier de mage, secondaire) */
    [RES_CELESTIAL_IRON]= 20.0f,   /* météorique — très rare */
    [RES_ENCHANTED_ARMS]= 46.0f,   /* armes enchantées — la Forge supérieure */
    [RES_TOOLS]         = 8.5f,    /* outils — le multiplicateur de productivité */
    [RES_ARMS]          = 9.0f,    /* armes & armures — militaire de base */
    [RES_GUNPOWDER]     = 11.0f,   /* poudre — militaire */
    [RES_REMEDE]        = 7.0f,    /* remèdes — santé/confort */
    [RES_POTTERY]       = 3.0f,    /* poterie — confort du commun (vaisselle/tuiles), argile façonnée */
    [RES_STATUE]        = 9.0f,    /* statuaire — confort de prestige (bourgeois/élite), pierre ouvrée */
};

/* ── REFONTE A1 — RENDEMENT D'EXTRACTION PAR OUVRIER (unités/ouvrier/AN) ──────
 * La brute n'est plus extraite ∝ terrain×√pop : elle suit les BRAS. Chaque ouvrier
 * affecté à une brute en tire EXTRACT_YIELD[r] par AN (à geo_eff=1, effort=1). La
 * valeur EST « production annuelle par 100 ouvriers ÷ 100 » (donc grain 800/100 = 8).
 * Au tick (mensuel) on multiplie par dt (=1/12). Les brutes manufacturées (≥
 * RES_PROD_FIRST) ne s'extraient pas (0). Le ×2 historique bois/fer/or est REPLIÉ ici
 * (bois/fer portent leur poids dans le rendement, plus de multiplicateur ad hoc).
 *   🔒 ancrés : grain 8 · poisson 4 · gibier/élevage 3 · bois 0.5 · pierre/argile 0.25.
 *   ◇ dérivés : métaux 0.25-0.40 · fibres/sucre 0.60 · épices/minéraux 0.30-0.40 ·
 *               or 0.08 · rares (perle/teinture/arcane/céleste) 0.03-0.06. */
static const float EXTRACT_YIELD[RES_COUNT] = {
    /* nourriture (interchangeable : grain/poisson/viande) */
    [RES_GRAIN]=8.0f, [RES_FISH]=4.0f, [RES_LIVESTOCK]=3.0f,
    /* vrac de construction & bois de feu — bois 0.5→1.0 : le FEU est un bien DIRECT (pas de
     * manufacture, donc pas de levier `labor`) → c'est le rendement qui cale « 100 emplois → 1000 hab »
     * (demande feu ≈ 105/1000hab/an, 100 ouvriers × 1.0 = 100 → ~1000 hab). */
    [RES_WOOD]=1.00f, [RES_STONE]=0.25f, [RES_CLAY]=0.25f,
    /* métaux & charbon — fer 0.3→0.4 (nourrir la chaîne outils, affamée à 2 %) */
    [RES_IRON]=0.40f, [RES_COAL]=0.40f, [RES_COPPER]=0.25f,
    /* fibres & douceurs (intrants manufacture) */
    [RES_WOOL]=0.60f, [RES_SUGAR]=0.60f, [RES_COTTON]=0.60f,
    /* FRUIT = NOURRITURE (supplément) : 4.0 = « 100 emplois → 400 hab » à la TUILE STANDARD
     * (geo=1) ; les tuiles fruitières ont un raw_cap modeste (≈0.4 plaine, ≈2.6 forêt) → la
     * contribution RÉELLE est geo-modulée (≈36 plaine, ≈230 forêt par 100 emplois). */
    [RES_FRUIT]=4.0f,
    /* épices, minéraux mineurs, fourrure */
    [RES_SALT]=0.30f, [RES_MED_HERBS]=0.30f, [RES_SALTPETER]=0.30f, [RES_SULFUR]=0.30f, [RES_FUR]=0.40f,
    /* précieux & rares (rendement maigre, valeur haute) */
    [RES_GOLD]=0.08f, [RES_PRECIOUS_METAL]=0.05f, [RES_PEARL]=0.05f,
    [RES_MUREX]=0.05f, [RES_INDIGO]=0.06f,
    [RES_ARCANE_CRYSTAL]=0.04f, [RES_CELESTIAL_IRON]=0.03f,
};

/* Recette d'une manufacture : jusqu'à 2 intrants → 1 produit. */
typedef struct {
    Resource in1;  float q1;
    Resource in2;  float q2;   /* in2 = RES_NONE si une seule entrée */
    Resource out;  float qout;
    float    labor;            /* besoin de main-d'œuvre par niveau */
    Resource alt1; float alt1_q;  /* intrant de REPLI pour in1, à SA PROPRE quantité
                                   * (perle pour l'or : 2× le métal par bijou). On puise
                                   * in1 d'abord, le repli ensuite. RES_NONE = aucun. */
    Resource out2; float qout2;   /* F3 : sortie SECONDAIRE (arme arcane) ∝ production. RES_NONE = aucune. */
} Recipe;

static const Recipe RECIPE[BLD_TYPE_COUNT] = {
    /* TEXTILE : laine (ou coton repli) → étoffe. ÉTOFFE = INTERMÉDIAIRE (→ tunique) + bien BOURG
     * direct mineur : on la garde EFFICACE (labor bas) — le puits de bras est sur la TUNIQUE (final). */
    [BLD_TEXTILE]   = { RES_WOOL,  1.5f, RES_NONE,          0.f, RES_CLOTH,          2.8f, 1.0f, RES_COTTON, 1.5f },  /* F4 : laine OU COTON (repli) → étoffe — le coton inerte gagne un débouché */
    [BLD_SAWMILL]   = { RES_WOOD,  2.0f, RES_COPPER,        0.2f, RES_NAVAL_SUPPLIES, 1.0f, 0.8f, RES_NONE, 0.f },  /* M5 : le naval EXIGE du cuivre (clous/doublage) — il ne sort plus sans */
    [BLD_PAPERMILL] = { RES_WOOD,  1.5f, RES_NONE,          0.f, RES_PAPER,          1.0f, 0.7f, RES_NONE, 0.f },
    /* VIN/BIÈRE — BOISSON, le SEUL bien manufacturé actif en EARLY ⇒ le calibrage `labor` qui MORD.
     * LEVIER LABOR (ratios & qout intacts) : labor = 1200·qout/demande_1000 → 100 emplois ≈ 1000 hab.
     * Vin : 1200·1.4/44.7 = 37.6 → labor 38. Bière : 1200·1.0/44.7 = 26.8 → labor 27. */
    [BLD_WINERY]    = { RES_SUGAR, 1.6f, RES_NONE,          0.f, RES_WINE,           1.4f, 38.f, RES_FRUIT, 4.0f },  /* sucre OU FRUIT (repli) → vin. Le fruit est désormais NOURRITURE : son intrant-vin est ÉLEVÉ (4 fruits/vin vs 1.6 sucre) → le vin de fruit est une voie CHÈRE, le fruit reste d'abord de la nourriture */
    [BLD_BREWERY]   = { RES_GRAIN, 1.2f, RES_NONE,          0.f, RES_BEER,           1.0f, 27.f, RES_NONE, 0.f },  /* grain → bière (palier moral des cultures de basse subsistance) */
    /* JOAILLERIE : OR, ou PERLE en repli (2× la quantité par bijou — littoral).
     * Sortie TEMPÉRÉE (1.0→0.5) et intrant plus lourd (1.5→2.0) : l'orfèvrerie
     * surinondait (couv ×170) → on vise un surplus DOUX, pas un raz-de-marée.
     * AUDIT (arc une économie) : « joaillerie sortie 0 » dans les scans courts
     * N'EST PAS un câblage mort — la chaîne est branchée (social_demo §2b le
     * prouve) ; la sortie est GATÉE par la demande d'élite, et le palier STATUT
     * ne se débloque qu'au tier 4 de capitale (≥4000 âmes) : un bien de GRANDES
     * cités — rare tant que la démographie n'y est pas. Design assumé. */
    [BLD_JEWELER]   = { RES_GOLD,  0.8f, RES_NONE,          0.f, RES_PRECIOUS_WARE,  0.5f, 1.2f, RES_PEARL, 1.6f },  /* SURCADENCE : l'or/perle est un FILIGRANE — une once rend assez de bijoux pour servir une cour + dégager un surplus à exporter (ratio 2× perle conservé) */
    /* ÉTOFFE PRÉCIEUSE — désormais GATÉE PAR LA TEINTURE (murex côtier, ou indigo du
     * bas-pays chaud en repli), comme l'orfèvrerie l'est par l'or. Recette 1:4 :
     * 1 teinture + 4 ÉTOFFES → 1 précieuse. Le précieux est ainsi PLAFONNÉ par la
     * teinture (rare), PAS par l'étoffe → quand la teinture manque, l'étoffe REFLUE
     * vers les tuniques (les journaliers servis). in1=teinture, in2=4 étoffes. */
    [BLD_WEAVER_LUX]= { RES_MUREX, 0.1f, RES_CLOTH, 4.0f, RES_PRECIOUS_CLOTH, 1.0f, 1.1f, RES_INDIGO, 0.1f },  /* SURCADENCE : un bain de teinture colore beaucoup → l'étoffe précieuse suit la cour ; la teinture PLACE-gate, l'étoffe 1:4 (surplus après tunique) borne le volume */
    /* TUNIQUE — étoffe → tunique (1:1), le bien fini du commun. LEVIER LABOR : 1200·1.0/42.2 = 28.4
     * → labor 28 (100 emplois ≈ 1000 hab habillés ; l'étoffe-feeder reste efficace en amont). */
    [BLD_TUNIC]     = { RES_CLOTH, 1.0f, RES_NONE,          0.f, RES_TUNIQUE,       1.0f, 28.f, RES_NONE, 0.f },
    /* ARCANE (F3) : on BRÛLE le cristal → ESSENCE (primaire) + BÂTON DE MAGE (secondaire, débloque
     * le mage). Sa combustion nourrit la Brèche (econ_tick → faust_charge_add). */
    [BLD_MAGE_WORKSHOP]={ RES_ARCANE_CRYSTAL, 1.0f, RES_NONE, 0.f, RES_ESSENCE,    1.0f, 1.3f, RES_NONE, 0.f, RES_MAGE_STAFF, 0.2f },
    /* ARCANE militaire (F3) : fer céleste ×2 + charbon → ARMES ENCHANTÉES (l'arme EST le primaire,
     * consommée par la Garde runique). Gate TECH_FORGE_RUNES (F7). */
    [BLD_CELESTIAL_FORGE]={ RES_CELESTIAL_IRON, 2.0f, RES_COAL, 1.0f, RES_ENCHANTED_ARMS, 1.0f, 1.4f, RES_NONE, 0.f },
    /* OUTILS — chaîne DIRECTE (le « métal » intermédiaire est SUPPRIMÉ : seuls les outils l'utilisaient).
     * 1 fer + 1 bois → 3 OUTILS. Input PASSIF ∝ main-d'œuvre (hors panier — efficace). */
    [BLD_TOOLWORKS] = { RES_IRON, 1.0f, RES_WOOD, 1.0f, RES_TOOLS, 3.0f, 0.9f, RES_NONE, 0.f },
    /* CHARBONNIÈRE : 2 bois → 1 charbon. Le charbon minier est rare et co-localisé
     * avec le fer (gate de la fonderie) ; la charbonnière le PRODUIT du bois (abondant)
     * → la fonderie tourne partout où il y a du fer, et la chaîne métal/outils respire. */
    [BLD_CHARCOAL]  = { RES_WOOD,  2.0f, RES_NONE, 0.f, RES_COAL,  1.0f, 0.8f, RES_NONE, 0.f },
    /* §B2 FOREUSE ARCANIQUE : transmute l'ESSENCE en FER en masse (0.5 essence → 8 fer).
     * L'issue faustienne à la famine de fer ; gatée par la tech (charge) + l'essence (rare). */
    [BLD_FOREUSE]   = { RES_ESSENCE, 0.5f, RES_NONE, 0.f, RES_IRON, 8.0f, 1.4f, RES_NONE, 0.f },
    /* F3 — L'ALAMBIC (GATE TECH_ALCHIMIE) : le salpêtre DISTILLÉ donne le FLUX (primaire,
     * intrant du Réplicateur ligneux) + le nécessaire d'ALCHIMISTE (secondaire, débloque le
     * soldat alchimiste). NE QUENCHE PLUS la charge (RETRAIT F-arc). Le salpêtre nourrit DÉJÀ
     * la poudre : une ressource, deux doctrines. */
    [BLD_ALAMBIC]   = { RES_SALTPETER, 1.2f, RES_NONE, 0.f, RES_FLUX, 1.0f, 0.9f, RES_NONE, 0.f, RES_ALCHEMIST_KIT, 0.3f },
    /* FAU2 — LES TRANSMUTEURS : comme la Foreuse (essence → fer), ils stabilisent un bien
     * vital à GROS rendement — l'échappatoire à la famine, payée en CHARGE (chaque spawn). */
    [BLD_REPLICATEUR]={ RES_FLUX,         0.5f, RES_NONE, 0.f, RES_WOOD,  8.0f, 1.4f, RES_NONE, 0.f },  /* flux → BOIS */
    [BLD_CORNE]     = { RES_CELESTIAL_IRON,0.5f, RES_NONE, 0.f, RES_GRAIN, 8.0f, 1.4f, RES_NONE, 0.f },  /* fer céleste → NOURRITURE */
    /* Chaînes militaires de base + santé (compléter le roster de production). */
    [BLD_ARMORY]    = { RES_IRON,      1.2f, RES_NONE, 0.f, RES_ARMS_LIGHT, 1.0f, 1.0f, RES_NONE, 0.f },  /* F2 : armurerie LÉGÈRE (RES_ARMS) */
    /* F2 — FABRIQUES SÉPARÉES (régions spécialisées) : chaque catégorie d'arme = un bâtiment. */
    [BLD_ARMORY_HEAVY]={ RES_IRON,     3.0f, RES_NONE, 0.f, RES_ARMS_HEAVY, 1.0f, 1.1f, RES_NONE, 0.f },  /* fer ×3 → lourdes */
    [BLD_BOWYER]    = { RES_IRON,      1.0f, RES_WOOD, 1.0f, RES_ARMS_RANGED,1.0f, 0.9f, RES_NONE, 0.f },  /* fer + bois → trait */
    [BLD_ARQUEBUS]  = { RES_IRON,      1.0f, RES_GUNPOWDER, 2.0f, RES_FIREARM, 1.0f, 1.1f, RES_COPPER, 1.0f },  /* fer + poudre (cuivre repli) → feu */
    /* CONFORT du brut de bâti — CONSOMMENT argile/pierre (⇒ la demande qui tire leur extraction). */
    [BLD_POTTERY]   = { RES_CLAY,      1.5f, RES_NONE,      0.f,  RES_POTTERY, 1.4f,  46.f, RES_NONE, 0.f },  /* argile → poterie (confort). LEVIER LABOR : 1200·1.4/36.6 = 45.9 → labor 46 */
    [BLD_SCULPTURE] = { RES_STONE,     2.0f, RES_NONE,      0.f,  RES_STATUE,  1.0f,  1.1f, RES_NONE, 0.f },  /* pierre → statuaire (luxe NICHE : demande basse → reste efficace) */
    [BLD_POWDERMILL]= { RES_SALTPETER, 1.0f, RES_COAL, 0.8f, RES_GUNPOWDER, 1.0f, 1.0f, RES_NONE, 0.f },
    [BLD_APOTHECARY]= { RES_MED_HERBS, 1.0f, RES_NONE, 0.f, RES_REMEDE,    1.0f, 0.8f, RES_NONE, 0.f },
};

/* Besoins par tête et par strate (unités/100 hab/tick). Le grain (vivres)
 * est universel ; le reste monte en gamme avec la classe. */
/* Table REVISITÉE. La case RES_WINE = palier MORAL (servi bière/vin selon la
 * préférence) ; la case RES_PRECIOUS_WARE = palier STATUT (orfèvrerie/étoffe
 * précieuse). On allège l'étoffe (en pénurie) et le bois de feu ; tout le reste
 * tend de +10 % via DEMAND_TENSION appliqué à `units` (demande tendue permanente). */
/* ── REFONTE A2 — la BOUCHE est ANNUELLE (1 food = une ration-personne-an) ────
 * La nourriture & le bois de feu sont des besoins ANNUELS, CALIBRÉS à la géographie
 * (le réglage fin passe par le tunable FOOD_NEED, défaut 1.0). La table reste « par
 * 100 hab / TICK » (mensuel, ×12/an). Grain & poisson sont INTERCHANGEABLES (food_sat
 * les agrège l.~1703 ; le pool national les met en commun) ; la nourriture du SPAWN
 * (A5) ancre chaque empire. ⚠ La bouche est l'apparié des hauts rendements A1, MAIS
 * elle reste BORNÉE : un bond ×8 (la cible « 100/100hab » brute) ÉCRASE la fertilité
 * — needs_met (le moteur de croissance, poids 0.85) chute quand la nourriture monopolise
 * le budget des journaliers (les autres paliers passent sous τ). On garde donc une
 * bouche ~3-4× l'ancienne (annuelle, signifiante) sans assécher la vitalité. Le CONFORT
 * (étoffe/vin/papier/sel/remède/fourrure/statut) garde ses valeurs. */
static const float NEED[CLASS_COUNT][RES_COUNT] = {
    [CLASS_LABORER] = {
        [RES_GRAIN]=3.50f, [RES_FISH]=1.00f,     /* A2 : nourriture ANNUELLE (grain+poisson INTERCHANGEABLES, food_sat les agrège) */
        [RES_WOOD]=1.00f,                         /* A2 : bois de FEU annuel (~3× l'ancien 0.35) */
        [RES_WINE]=0.35f, [RES_TUNIQUE]=0.40f,   /* confort INCHANGÉ (bière/vin via préférence) */
        [RES_POTTERY]=0.30f,                      /* poterie : vaisselle/tuiles du foyer — confort (⇒ demande d'argile) */
    },
    [CLASS_BOURGEOIS] = {
        [RES_GRAIN]=4.00f,                        /* A2 : nourriture ANNUELLE */
        [RES_CLOTH]=0.34f, [RES_PAPER]=0.25f, [RES_WINE]=0.30f,
        [RES_SALT]=0.20f, [RES_REMEDE]=0.15f,   /* santé urbaine (apothicaire) — confort INCHANGÉ */
        [RES_POTTERY]=0.25f, [RES_STATUE]=0.12f, /* poterie fine + ornement de pierre — confort (⇒ demande argile/pierre) */
    },
    [CLASS_ELITE] = {
        [RES_GRAIN]=4.00f,                        /* A2 : nourriture ANNUELLE */
        [RES_FUR]=0.12f, [RES_PAPER]=0.12f, [RES_WINE]=0.28f,
        [RES_PRECIOUS_WARE]=0.13f,   /* palier STATUT (orfèvrerie OU étoffe) — confort INCHANGÉ */
        [RES_STATUE]=0.18f,                       /* statuaire de prestige — confort/statut (⇒ demande de pierre) */
    },
};
/* §besoins progressifs — ORDRE de priorité par classe (subsistance → confort → STATUT).
 * Le nombre de besoins COMPTÉS dans la satisfaction = f(niveau de capitale, ∝ pop) : un
 * petit centre n'aspire qu'aux bases (2 besoins), une grande capitale développée à tout
 * le panier (statut compris). Ainsi le luxe se MÉRITE avec le développement — et l'élite
 * d'un bourg n'est pas punie de ne pas avoir d'orfèvrerie. Le palier STATUT vient DERNIER. */
static const Resource NEED_ORDER[CLASS_COUNT][9] = {
    [CLASS_LABORER]   = { RES_GRAIN, RES_WINE, RES_FISH, RES_WOOD, RES_TUNIQUE, RES_POTTERY, RES_NONE },  /* bière (RES_WINE→préférée) en palier moral PRÉCOCE ; poisson = nourriture interchangeable (A2) ; poterie = confort TARDIF */
    [CLASS_BOURGEOIS] = { RES_GRAIN, RES_SALT, RES_CLOTH, RES_REMEDE, RES_WINE, RES_PAPER, RES_POTTERY, RES_STATUE, RES_NONE },
    [CLASS_ELITE]     = { RES_GRAIN, RES_FUR, RES_PAPER, RES_WINE, RES_PRECIOUS_WARE, RES_STATUE, RES_NONE },
};
/* rang de priorité d'un besoin (0 = vital) ; 99 = hors panier (jamais débloqué). */
static int need_rank(int c, Resource r){
    if (c<0||c>=CLASS_COUNT) return 99;
    for (int i=0;i<9 && NEED_ORDER[c][i]!=RES_NONE;i++) if (NEED_ORDER[c][i]==r) return i;
    return 99;
}
/* REFONTE A2 — une SOURCE DE NOURRITURE interchangeable (grain/poisson/viande). FOOD_NEED
 * calibre leur demande à l'échelle du monde (la cible 100/100hab borne la géographie). */
static inline bool res_is_food(Resource r){ return r==RES_GRAIN||r==RES_FISH||r==RES_LIVESTOCK||r==RES_FRUIT; }   /* le FRUIT nourrit aussi (supplément, + en forêt) */

/* Part de chaque strate dans la population à l'initialisation. */
static const float CLASS_SHARE[CLASS_COUNT] = { 0.80f, 0.15f, 0.05f };

/* ---- Le palier MORAL est une VARIANTE culturelle (catalogue des biens) ----
 * Les cultures de basse subsistance (clans, montagnards nains, sauvages orques)
 * brassent la BIÈRE ; les cultures agraires/urbaines (cités, sylve elfique,
 * mercantile) pressent le VIN. Servir la MAUVAISE boisson ne contente qu'à
 * moitié (un nain boude le vin, un orque méprise le verre fin). */
#define DRINK_OFFCULT 0.5f
static inline Resource preferred_drink(const PopCulture *c){
    return (c->subsistance < 5.f) ? RES_BEER : RES_WINE;
}
/* Le palier STATUT (luxe d'élite) est lui aussi une variante : les cultures
 * martiales/pastorales (clans, nains, orques) prisent l'ORFÈVRERIE (torques,
 * runes, totems = bien OUVRÉ → precious_ware) ; les cultures établies/raffinées
 * (cités, sylve, mercantile) prisent l'ÉTOFFE PRÉCIEUSE (soie, fil-de-lune →
 * precious_cloth). Servir le mauvais luxe ne flatte qu'à moitié — l'élite
 * conquise reste sur sa faim (le terreau du coup d'État). */
#define LUXE_OFFCULT 0.5f
static inline Resource preferred_luxe(const PopCulture *c){
    return (c->subsistance < 5.f) ? RES_PRECIOUS_WARE : RES_PRECIOUS_CLOTH;
}

#define TAX_RATE     0.38f   /* RENTE d'élite — visée : équilibre de satisfaction ~60 % (re-dotée) */
#define WAGE_SHARE   0.42f   /* salaires (laborers) — abaissé pour la rente ; profit bourgeois = 0.20 */
/* le reste (1 - TAX - WAGE) = profit bourgeois (résidu 0.25 — reste sain) */
/* §NF — CONSTRUCTION PAR RÉTROACTION NÉGATIVE : un bien en pénurie appelle son
 * producteur, qu'on bâtit spontanément — mais JAMAIS dans le vide (pop + intrant). */
#define NF_SHORTAGE   1.8f   /* prix ≥ 1.8× base = pénurie qui justifie de bâtir le producteur */
#define NF_POP_FLOOR  80.f   /* pop régionale minimale : sous ce seuil, on ne bâtit pas (le vide) */
#define NF_STOCK_MIN  5.0f   /* stock d'intrant comptant comme « approvisionné » (extraction OU import) */
#define NF_SEED_LEVEL 1.0f   /* niveau de NAISSANCE du bâtiment (puis l'expansion §1 le fait croître) */
/* §gate — DEMANDE EFFECTIVE pour les biens DURABLES (orfèvrerie, outils). Ces biens
 * ne se consomment pas comme le pain : sans frein leur manufacture tourne au PLANCHER
 * du market_effort (0.42×) et GORGE le marché (prix planché 0.2×base — orfèvrerie 4.4,
 * outils 1.7 — intrants rares gaspillés, et le prix figé tue le signal de §NF/IA). */
#define GATE_DEMAND_BUFFER 1.25f   /* orfèvrerie : produire jusqu'à 1.25× la demande RÉELLE d'élite (marge croissance + surplus d'export) */
/* OUTILS — INPUT PASSIF de la main-d'œuvre : l'outil est un CAPITAL que les journaliers
 * USENT en travaillant (∝ leur nombre). Ce flux EST la demande effective (il tire le prix,
 * donc le §NF qui bâtit l'atelier et la perception IA) ET draine le stock (usure par
 * l'usage). L'outil n'entre PAS dans le panier : il ne touche QUE la productivité, JAMAIS
 * la satisfaction. Fini le prix planché à demande nulle qui rendait l'outillage inerte. */
#define TOOLS_PER_LABORER  0.05f   /* cible de stock-outil/journalier → pilote le bonus prod_mult (+30 % max). Sous PRIX NATIONAL (uniforme par empire, plus d'artefact régional), on remonte la cible (0.015→0.05) pour un bonus RÉEL ; le prix s'élève en miroir (tension bonus↔prix assumée), mais sans flambée régionale. */
/* §collecte — REFONTE LABOR-BOUND (ressource PAR OUVRIER). La récolte d'une brute =
 * OUVRIERS affectés × EXTRACT_YIELD[r] (rendement/ouvrier/an) × geo_eff (qualité de tuile)
 * × effort de marché (le prix). Plus de √pop : la collecte est LINÉAIRE en bras.
 *  · EXTRACT_GEO_REF : raw_cap de référence donnant geo_eff = 1 (la tuile « standard »).
 *  · EXTRACT_GEO_CAP : plafond de qualité (une tuile exceptionnelle ne va pas à l'infini).
 *  · EXTRACT_LABOR_SHARE : part des JOURNALIERS dédiée à l'extraction (le reste staffe les
 *    manufactures) ; les ouvriers se répartissent entre brutes ∝ geo_eff×prix (la tuile
 *    riche/chère attire). C'est le levier de CALIBRAGE du volume brut. */
#define EXTRACT_GEO_REF      4.5f    /* calibré seed 9 : pop/needs_met = baseline (cf. CLAUDE.md) */
#define EXTRACT_GEO_CAP      3.0f
#define EXTRACT_LABOR_SHARE  0.65f   /* part des journaliers à l'extraction (le reste staffe les manufactures). Le réglage fin du bassin manufacture passera par l'ALLOCATION joueur/IA, pas ce levier global (0.45 testé : n'aide pas la boisson — limitée par la réserve vivrière locale — et baisse un peu la satisfaction). */
/* REFONTE A5 — LA NOURRITURE DU SPAWN (la SEULE règle vivrière de worldgen). La région
 * CAPITALE de chaque empire naît avec un socle de grain (raw_cap), un grenier de départ.
 * Tout le reste de la carte est pure GÉOLOGIE (grain/poisson dans la vocation) + COMMERCE.
 * Valeur = qualité de tuile (raw_cap) : SPAWN_FOOD_RAW/GEO_REF = geo_eff du grenier. */
#define SPAWN_FOOD_RAW       12.0f
#define TECH_RATE    0.010f  /* conversion richesse élite → tech */
#define PRICE_INERTIA 0.65f  /* lissage du prix (0=instantané,1=figé) */
#define EPS          1e-4f

/* Démographie calibrée : la FERTILITÉ suit les BESOINS SATISFAITS (registre J :
 * POP_R_BASE=ln2/100 → ×2/siècle au plancher ; POP_NEEDS_W·needs_met + POP_PROSP_W·
 * prospérité → jusqu'à ×4/siècle au panier plein). Cf. §6 croissance. Les anciens
 * BIRTH_RATE/DEATH_RATE/SOCIETY_BONUS sont RETIRÉS (la base ne multiplie plus food_sat). */

/* Colonisation */
#define COLONY_MIN_POP      500.f   /* pop minimale d'une région pour essaimer  */
#define COLONY_COST_POP     250.f   /* colons détachés (quittent la mère)       */
#define COLONY_SEED_POP     100.f   /* pop installée dans la nouvelle région    */
#define COLONY_FOOD_GATE    0.35f   /* seuil de subsistance pour essaimer        */

/* Migration interne */
#define MIGRATE_RATE        0.02f   /* fraction max de bourgeois/élites migrant/tick */
#define MIGRATE_THRESHOLD   1.30f   /* différentiel de prospérité déclencheur        */
#define DIASPORA_TECH_RATE  0.0008f /* tech/tick par unité de diaspora               */
#define DIASPORA_DECAY      0.98f   /* acculturation : diaspora s'absorbe (~50 ticks) */

/* Relocalisation forcée */
#define RELOC_COERCION_BASE 0.25f   /* pic de coercition de base par évènement       */
#define COERCION_DECAY      0.93f   /* demi-vie ≈ 10 ticks                            */

static inline float clampf(float v,float lo,float hi){return v!=v?lo:(v<lo?lo:(v>hi?hi:v));}

/* TENSION DE DEMANDE : +10 % de besoins partout (appliqué au facteur `units`) → une
 * demande tendue en permanence, le marché presse toujours sur l'offre. */
#define DEMAND_TENSION 1.10f

/* SURPLUS NATUREL : l'effort de production SUIT le prix (on « lit le marché »).
 *   prix/base = 1 (équilibre) → 1.10  (+10 % : un mince surplus s'installe)
 *   bien au plancher (glut)   → ~0.30 (on lève moins, la main-d'œuvre file ailleurs)
 *   bien cher (pénurie)       → 1.50  (on force la production)
 * Appliqué à l'extraction (out & main-d'œuvre) et à la manufacture (cap = niveau·eff). */
static inline float market_effort(float price, float base){
    if (base<=0.f) return 1.f;
    return clampf(0.25f + 0.85f*(price/base), 0.22f, 1.5f);
}

/* §4 (catalogue des biens) — DEMANDE par VARIANTE CULTURELLE. Les biens d'un
 * peuple ne sont pas d'autres biens : ce sont les variantes d'un même palier.
 * Une minorité d'une autre SPHÈRE réclame SES variantes (un orque méprise le
 * verre fin, un nain boude le vin) ; lui servir celles du dominant la satisfait
 * MAL. L'ASSIMILATION (integration↑, via le refactor démographique) fait DÉRIVER
 * sa demande vers la dominante → la pénalité s'efface sur les générations.
 * Renvoie la fraction de pop « mal servie » [0..1] (0 si province homogène). */
/* Recette d'un bâtiment (intrants → extrant) — exposée pour la perception IA
 * (détecter un raffineur qui tourne à vide faute d'intrant). */
void building_recipe(BuildingType b, Resource *in1, Resource *in2, Resource *out){
    if (b<0 || b>=BLD_TYPE_COUNT){ if(in1)*in1=RES_NONE; if(in2)*in2=RES_NONE; if(out)*out=RES_NONE; return; }
    if (in1) *in1=RECIPE[b].in1;
    if (in2) *in2=RECIPE[b].in2;
    if (out) *out=RECIPE[b].out;
}
/* Intrant ALTERNATIF (repli) d'un bâtiment — exposé pour l'UI d'allocation (choix d'intrant). */
Resource building_alt_input(BuildingType b){
    return (b>=0 && b<BLD_TYPE_COUNT) ? RECIPE[b].alt1 : RES_NONE;
}

float econ_off_culture_fraction(const ProvincePop *pp){
    if (!pp || pp->n_groups<=1) return 0.f;
    int dom=-1; long best=-1;
    for (int i=0;i<pp->n_groups;i++) if (pp->groups[i].count>best){ best=pp->groups[i].count; dom=i; }
    if (dom<0) return 0.f;
    Sphere doms = pp->groups[dom].origin_sphere;
    long total=0; float off=0.f;
    for (int i=0;i<pp->n_groups;i++){
        total += pp->groups[i].count;
        float sd   = sphere_distance(doms, pp->groups[i].origin_sphere)/7.f; /* normalisé 0..1 */
        float mism = sd * (1.f - clampf(pp->groups[i].integration,0.f,1.f)); /* l'assimilation efface */
        off += mism * (float)pp->groups[i].count;
    }
    return (total>0)? off/(float)total : 0.f;
}

const char *social_class_name(SocialClass c) {
    static const char *N[CLASS_COUNT]={"Laborers","Bourgeois","Élites"};
    return (c>=0&&c<CLASS_COUNT)?N[c]:"?";
}
const char *building_name(BuildingType b) {
    static const char *N[BLD_TYPE_COUNT]={
        [BLD_TEXTILE]="Manufacture textile",[BLD_SAWMILL]="Scierie navale",[BLD_PAPERMILL]="Papeterie",
        [BLD_WINERY]="Domaine viticole",[BLD_BREWERY]="Brasserie",[BLD_JEWELER]="Joaillerie",
        [BLD_WEAVER_LUX]="Atelier d'étoffe précieuse",[BLD_MAGE_WORKSHOP]="Atelier de mage",
        [BLD_CELESTIAL_FORGE]="Forge céleste",[BLD_TOOLWORKS]="Atelier d'outillage",[BLD_ALAMBIC]="Alambic",
        [BLD_ARMORY]="Armurerie légère",[BLD_POWDERMILL]="Poudrière",[BLD_APOTHECARY]="Apothicaire",
        [BLD_TUNIC]="Atelier de tunique",[BLD_CHARCOAL]="Charbonnière",[BLD_FOREUSE]="Foreuse arcanique",
        [BLD_REPLICATEUR]="Réplicateur ligneux",[BLD_CORNE]="Corne divine",
        [BLD_ARMORY_HEAVY]="Armurerie lourde",[BLD_BOWYER]="Atelier d'arc",[BLD_ARQUEBUS]="Arquebuserie",
        [BLD_POTTERY]="Poterie",[BLD_SCULPTURE]="Atelier de sculpture",
    };
    return (b>=0&&b<BLD_TYPE_COUNT&&N[b])?N[b]:"?";
}

/* ====================================================================== */
/* INITIALISATION                                                         */
/* ====================================================================== */

/* Ajoute une manufacture de type t si absente, et renvoie son index. */
static int region_ensure_building(RegionEconomy *re, BuildingType t) {
    for (int i=0;i<re->n_bld;i++) if (re->bld[i].type==t) return i;
    if (re->n_bld>=ECON_MAX_BLD) return -1;
    int i=re->n_bld++;
    re->bld[i].type=t; re->bld[i].level=0.f; re->bld[i].workers=0.f;
    return i;
}

/* Injecte une population répartie en strates dans une région (peuplement
 * initial ou arrivée de colons). N'écrase pas les manufactures/prix. */
static void econ_seed_population(RegionEconomy *re, float total_pop) {
    for (int c=0;c<CLASS_COUNT;c++) {
        re->strata[c].pop         = total_pop*CLASS_SHARE[c];
        re->strata[c].wealth      = re->strata[c].pop * (c==CLASS_ELITE?6.f:c==CLASS_BOURGEOIS?2.f:0.5f);
        re->strata[c].satisfaction= 0.5f;
    }
}

/* §11.4 — LIMITEUR DE PRODUCTION joueur : cap par (pays,ressource). -1 = ∞ (désactivé). */
static float g_prod_cap[SCPS_MAX_COUNTRY][RES_COUNT];
/* (RE)CONSTRUIT l'adjacence de régions (terre, 4-connexe) : un lien ssi AUCUNE des
 * deux régions n'est infranchissable (glaciers/déserts/RONCES/MER = barrières). Zéroïe
 * d'abord — réutilisable après une carve (capstone §27 : côtes & barrières déplacées). */
void econ_build_adjacency(WorldEconomy *e, const World *w) {
    memset(e->adj, 0, sizeof e->adj);
    static const int DX4[4]={1,-1,0,0}, DY4[4]={0,0,1,-1};
    for (int y=0;y<SCPS_H;y++) for (int x=0;x<SCPS_W;x++) {
        int ra=w->cell[scps_idx(x,y)].region;
        if (ra<0) continue;
        for (int d=0;d<4;d++) {
            int nx=x+DX4[d], ny=y+DY4[d];
            if (nx<0||nx>=SCPS_W||ny<0||ny>=SCPS_H) continue;
            int rb=w->cell[scps_idx(nx,ny)].region;
            if (rb<0||rb==ra) continue;
            if (!e->region[ra].impassable && !e->region[rb].impassable) {
                e->adj[ra][rb]=1; e->adj[rb][ra]=1;
            }
        }
    }
}

/* GAMEPLAY — GARANTIE DES BRUTES DE BASE PRÈS DU JOUEUR : la worldgen tire les brutes SELON LE BIOME
 * (argile aux terres d'eau, pierre au relief, fer/bois aux gisements) — par malchance, la capitale
 * peut en manquer à portée. On FORCE une tuile de chaque (argile, pierre, FER, BOIS) dans le RAYON 1-2
 * de la capitale (via l'adjacence éco) : le joueur ne doit JAMAIS être privé de construction NI d'outils.
 * IDEMPOTENT (présent dans le rayon ⇒ on ne force rien) ; APRÈS econ_init (adjacence) + les capitales. */
void econ_guarantee_player_construction(WorldEconomy *e, const World *w, int player_cid){
    if (!e || !w || player_cid<0 || player_cid>=w->n_countries) return;
    int cp = w->country[player_cid].capital_prov;
    if (cp<0 || cp>=w->n_provinces) return;
    int caphr = w->province[cp].region;
    if (caphr<0 || caphr>=e->n_regions || caphr>=SCPS_MAX_REG) return;
    int N = (e->n_regions < SCPS_MAX_REG) ? e->n_regions : SCPS_MAX_REG;
    static bool inrad[SCPS_MAX_REG];
    for (int r=0;r<N;r++) inrad[r]=false;
    inrad[caphr]=true;                                        /* rayon 0 : la capitale */
    for (int r=0;r<N;r++) if (e->adj[caphr][r]){ inrad[r]=true;   /* rayon 1 */
        for (int r2=0;r2<N;r2++) if (e->adj[r][r2]) inrad[r2]=true; }  /* rayon 2 */
    const float amt = tune_f("PLAYER_GUARANTEE_RAW", 4.f);
    const Resource gg[4] = { RES_CLAY, RES_STONE, RES_IRON, RES_WOOD };   /* les 4 brutes de base à portée */
    for (int i=0;i<4;i++){ Resource g=gg[i];
        int present=0, target=-1;
        for (int r=0;r<N;r++){ if(!inrad[r] || !e->region[r].active) continue;
            if (e->region[r].raw_cap[g] >= 1.f){ present=1; break; }
            if (target<0 && r!=caphr) target=r;              /* préfère un VOISIN distinct (capitale garde sa vocation) */
        }
        if (!present){
            if (target<0) target=caphr;                      /* aucun voisin franchissable → la capitale elle-même */
            e->region[target].raw_cap[g] = amt;              /* « Clay + X » / « Pierre + Y » */
        }
    }
}

/* CAPSTONE §27 FROID — recompute la fertilité vivrière depuis la carte REFROIDIE.
 * Le socle de grain (raw_cap[RES_GRAIN], posé à l'init = cap_pop/100 × (1.15+0.70·hab),
 * floor anti-famine 1.15×) est RE-dérivé de l'habitabilité COURANTE des cellules (biome
 * × confort thermique). Tant que hab ≥ 0.30 la formule d'init est conservée À L'IDENTIQUE
 * (zéro choc en jeu normal) ; SOUS 0.30 (terre gelée) elle PLONGE proportionnellement →
 * le grain tombe sous la conso → food_sat < 0.35 → la pop décline (la famine ÉMERGE de
 * la chaîne, pas d'un modificateur plat). build.food_cap (grenier/irrigation) est INTACT
 * (canal séparé). N'agit que sur les régions vivantes (owner≥0, franchissables). */
void econ_cold_refresh(WorldEconomy *e, const World *w) {
    static float hab_sum[SCPS_MAX_REG]; static int cnt[SCPS_MAX_REG];
    memset(hab_sum, 0, sizeof hab_sum); memset(cnt, 0, sizeof cnt);
    for (int i=0;i<SCPS_N;i++){
        const Cell *c=&w->cell[i];
        if (c->height < SEA_LEVEL) continue;            /* terre seule */
        int r=c->region; if (r<0 || r>=e->n_regions || r>=SCPS_MAX_REG) continue;
        float hcell = biome_habitability(c->biome, c->temperature, c->height);
        if (c->coast && hcell < 0.32f) hcell = 0.32f;   /* PLANCHER CÔTIER : la côte reste vivable (Chili) */
        hab_sum[r] += hcell;
        cnt[r]++;
    }
    for (int r=0;r<e->n_regions && r<SCPS_MAX_REG;r++){
        if (cnt[r]==0) continue;
        RegionEconomy *re=&e->region[r];
        if (re->owner<0 || re->impassable) continue;
        float hab = hab_sum[r]/(float)cnt[r];
        re->habitability = hab;
        float fac = (hab >= 0.30f) ? (1.15f + 0.70f*hab)               /* jeu normal : IDENTIQUE à l'init */
                                   : (1.15f + 0.70f*hab) * (hab/0.30f);  /* gel : plonge vers 0 */
        /* REFONTE (carte nue géologique) : le froid RÉDUIT le grain de la tuile (vocation/spawn),
         * il n'en AJOUTE jamais — on borne par la valeur existante. Quand le gel plonge fac sous
         * elle, le grain tombe → l'extraction vivrière s'effondre → famine. (L'ancien overwrite
         * cap_pop-based plantait du grain partout, incohérent avec le food géologique du refonte.) */
        float cold_grain = (re->cap_pop/100.f) * fac;
        if (cold_grain < re->raw_cap[RES_GRAIN]) re->raw_cap[RES_GRAIN] = cold_grain;
    }
}

void econ_init(WorldEconomy *e, const World *w) {
    for (int c=0;c<SCPS_MAX_COUNTRY;c++) for (int g=0;g<RES_COUNT;g++) g_prod_cap[c][g]=-1.f;
    memset(e,0,sizeof(*e));
    econ_mobility_reset();              /* E0.7 : RAZ mobilité de classe (par partie/sim) */
    e->n_regions=w->n_regions;
    e->tick=0;
    e->ipm=1.f; e->ipm_ref=0.f;         /* §C : IPM neutre, référence non encore captée */

    /* ---- Passe 1 : capacité et habitabilité de chaque région ------------- *
     * reg_hab = habitabilité moyenne pondérée par la surface (province).
     * reg_cap = capacité brute, MULTIPLIÉE par reg_hab → les zones glaciaires
     * et les déserts hyperarides ont cap_pop ≈ 0, reflétant la réalité.     */
    float reg_cap[SCPS_MAX_REG]={0};
    float reg_hab[SCPS_MAX_REG]={0};
    bool  reg_impass[SCPS_MAX_REG]={0};   /* zone morte (déterminée ici, RÉUTILISÉE en Passe 3) */
    float cty_cap[SCPS_MAX_COUNTRY]={0};
    /* Une région PORTEUSE de la capitale d'un empire/cité ne peut être déclarée morte : la capitale est
     * posée sur sa province habitable (choisie pour l'eau+nourriture), mais le poids de rôle préfère les
     * sièges CÔTIERS — la région agrège alors assez de provinces mortes (côte/glacier) pour franchir le
     * seuil d'infranchissabilité. Sans garde, un empire (voire le JOUEUR) naît SANS aucune région
     * colonisée (capitale inactive). On exonère donc la région-siège du verdict de zone morte. */
    bool is_cap[SCPS_MAX_REG]={0};
    for (int c=0;c<w->n_countries;c++){
        PolityRole rl=w->country[c].role;
        if (rl!=POLITY_PLAYER && rl!=POLITY_ANTAGONIST && rl!=POLITY_CITY_STATE) continue;
        int cp=w->country[c].capital_prov;
        int cr=(cp>=0&&cp<w->n_provinces)? w->province[cp].region : -1;
        if (cr>=0&&cr<w->n_regions) is_cap[cr]=true;
    }
    for (int rid=0; rid<w->n_regions; rid++) {
        const Region *rg=&w->region[rid];
        e->region[rid].import_margin = 1.f;          /* I6 : marché 1:1 par défaut (intertrade l'ajuste) */
        e->region[rid].import_toll_region = -1;
        e->region[rid].last_pole = 1;                /* M3 : POLE_ORDRE par défaut (sinon memset→MARTIAL) */
        e->region[rid].pole_since_day = 0;
        float cap=0.f, area=0.f, hab_w=0.f, dead_area=0.f;
        for (int k=0;k<rg->n_provinces;k++) {
            int pid=rg->province_ids[k];
            if (pid<0||pid>=w->n_provinces) continue;
            const Province *pv=&w->province[pid];
            float a = (float)pv->area;
            /* Intensité agricole du biome (même source de vérité que l'axe
             * subsistance culturel) → capacité d'accueil brute. */
            float subs = subsistance_for_biome(pv->biome_dominant);
            cap  += a * (0.25f + 0.75f*clampf(subs/10.f,0.f,1.f));
            hab_w += pv->habitability * a;
            area += a;
            if (pv->habitability <= 0.25f) dead_area += a;   /* SEUIL D'ACCÈS : ≤25 % = inaccessible (glacier/pic/volcan/montagne escarpée/désert/froid) */
        }
        if (area<1.f) continue;
        float hab = hab_w / area;   /* habitabilité pondérée par surface */
        reg_hab[rid] = hab;
        reg_cap[rid] = cap * hab;   /* la capacité est nulle pour les zones mortes */
        /* SEUIL D'ACCÈS UNIFIÉ (≤25 %) : une région est inaccessible si elle est
         * MAJORITAIREMENT sous le seuil (≥50 % d'aire ≤25 %) OU si sa moyenne tombe
         * elle-même ≤25 %. Remplace les ex-seuils ad-hoc (0.01 / 35 % / 12 %). La
         * région-siège reste exemptée (la capitale tient sur sa province habitable). */
        reg_impass[rid] = ((dead_area/area >= 0.50f) || (hab <= 0.25f)) && !is_cap[rid];
        int cid=rg->country;
        /* La capacité-pays ne compte que les terres VIVABLES → la cible (Passe 2)
         * se répartit sur les seules régions actives : aucune part « fuite » dans
         * une zone morte. cap_pop_sum vaut alors EXACTEMENT Σ cibles. */
        if (cid>=0 && cid<SCPS_MAX_COUNTRY && !reg_impass[rid]) cty_cap[cid]+=reg_cap[rid];
    }

    /* ---- Passe 2 : capacité d'accueil par RÔLE (Q6 re-baseline) -----------
     * La capacité d'accueil VIT dans les polités RÉELLES, pas dans la friche :
     *   EMPIRE     → 8000 (graine 4000 dans la capitale → DOUBLE à terme) ;
     *   CITÉ-ÉTAT  → 4000 (graine 2000 répartie         → double) ;
     *   VIERGE     → 200  (frontière colonisable, négligeable pour le total).
     * 6×8000 + 12×4000 = 96 000 : la population mondiale CONVERGE vers ~96k
     * par la seule croissance (pas de bidouille du taux), la guerre ne fait que
     * REDISTRIBUER cette capacité (conquête) sans la créer. */
    float empire_cap = tune_f("EMPIRE_CAP", 13000.f);
    float city_cap   = tune_f("CITY_CAP",    6500.f);
    float cty_target[SCPS_MAX_COUNTRY]={0};
    for (int cid=0; cid<SCPS_MAX_COUNTRY && cid<w->n_countries; cid++) {
        if (cty_cap[cid]<=0.f) continue;
        switch (w->country[cid].role) {
            case POLITY_PLAYER:
            case POLITY_ANTAGONIST: cty_target[cid]=empire_cap; break;
            case POLITY_CITY_STATE: cty_target[cid]=city_cap;   break;
            default:                cty_target[cid]= 200.f;      break;  /* friche vierge */
        }
    }

    /* ---- Passe 3 : peuplement de chaque région --------------------------- */
    for (int rid=0; rid<w->n_regions; rid++) {
        RegionEconomy *re=&e->region[rid];
        const Region *rg=&w->region[rid];

        float area_sum=0.f;
        for (int k=0;k<rg->n_provinces;k++) {
            int pid=rg->province_ids[k];
            if (pid<0||pid>=w->n_provinces) continue;
            area_sum += w->province[pid].area;
        }
        /* Zone morte / infranchissable (glacier, pic, volcan, désert hyperaride) :
         * déjà tranchée en Passe 1 (≥35 % d'aire à habitabilité nulle, ou moyenne
         * < 12 %) et RÉUTILISÉE ici — même verdict que le calcul de cty_cap, donc
         * aucune part de cible ne fuite dans une région qu'on déclare ensuite morte. */
        bool is_impass = reg_impass[rid];

        re->habitability = reg_hab[rid];
        re->is_capital   = is_cap[rid];   /* région-siège : EXEMPTE du malus d'habitabilité (province de départ) */
        if (area_sum<1.f || reg_cap[rid]<=0.f || is_impass) {
            re->active     = false;
            re->impassable = is_impass;
            re->colonized  = false;
            re->owner      = -1;
            continue;
        }
        re->active=true;
        re->impassable=false;
        re->colonized=false;
        re->owner=-1;

        /* Capacité d'accueil : pop cible à terme (sert au peuplement initial
         * de la capitale et de plafond souple à la croissance). */
        int cid=rg->country;
        float total_pop;
        if (cid>=0 && cid<SCPS_MAX_COUNTRY && cty_cap[cid]>0.f)
            total_pop = cty_target[cid] * reg_cap[rid] / cty_cap[cid];
        else
            total_pop = 40.f + reg_cap[rid]*12.f;
        re->cap_pop = total_pop;

        /* Population : laissée à ZÉRO par défaut. Le monde démarre vide ;
         * seules la capitale du joueur et quelques cités-états seront
         * peuplées (voir plus bas). Tout le reste est colonisable. */
        for (int c=0;c<CLASS_COUNT;c++) {
            re->strata[c].pop         = 0.f;
            re->strata[c].wealth      = 0.f;
            re->strata[c].satisfaction= 0.5f;
        }
        re->food_sat=0.5f; re->society_sat=0.5f;

        /* ---- Capacité d'extraction : héritée des ressources brutes des
         *      provinces. Chaque province « pose » sa ressource dominante. */
        bool coastal=false; int wooded=0;
        for (int k=0;k<rg->n_provinces;k++) {
            int pid=rg->province_ids[k];
            if (pid<0||pid>=w->n_provinces) continue;
            const Province *pv=&w->province[pid];
            if (pv->coastal) coastal=true;
            { Biome bw=pv->biome_dominant; if (bw==BIO_FOREST||bw==BIO_WOODS||bw==BIO_JUNGLE) wooded++; }
            /* débit proportionnel à la surface (P3.18 : la SPÉCIALISATION — le brut
             * DOMINANT de la province est franc, la 2e brute mineure ; le reste vient
             * du COMMERCE, plus jamais du sol). */
            float base = 1.5f + pv->area*0.05f;
            Resource r=pv->resource;
            if (r>RES_NONE && r<RES_PROD_FIRST) re->raw_cap[r] += base*1.5f;   /* P3.18 : dominante franche */
            Resource r2=pv->resource2;                      /* §6b : 2e brute, mineure ×0.4 */
            if (r2>RES_NONE && r2<RES_PROD_FIRST) re->raw_cap[r2] += base*0.5f;
            /* E1 — matériaux de construction LUS de la géo : la pierre sort du
             * relief, l'argile des terres d'eau. Francs là où la terre les donne. */
            Biome bd=pv->biome_dominant;
            if (bd==BIO_HILLS||bd==BIO_HIGHLANDS||bd==BIO_MOUNTAINS||bd==BIO_PEAK
                ||bd==BIO_VOLCANO||pv->height_avg>0.55f)
                re->raw_cap[RES_STONE] += base*0.5f;
            if (bd==BIO_MARSH||bd==BIO_BOG||bd==BIO_MANGROVE)
                re->raw_cap[RES_CLAY]  += base*0.5f;
            /* FRUIT — vergers/cueillette : FORÊT/BOIS/JUNGLE SEULEMENT (plus « partout ») → le fruit
             * ne vole PAS les bras d'extraction au grain dans les régions céréalières (la bière survit).
             * Nourriture de substitution (food-fill, plus bas) + repli du VIN. Protégé de la coupe. */
            if (bd==BIO_FOREST||bd==BIO_WOODS||bd==BIO_JUNGLE) re->raw_cap[RES_FRUIT] += base*0.65f;
        }

        /* Subsistance locale : vivres et bois de feu dimensionnés pour couvrir
         * ~90% de la population, laissant la satisfaction refléter les biens
         * supérieurs et non une famine universelle. */
        float subsist = total_pop / 100.f;
        /* PLUS de socle vivrier UNIVERSEL (anti-famine retiré, à la demande) : la
         * NOURRITURE suit la GÉOLOGIE comme toute brute (les terres fertiles ont le
         * grain dans leur resource/resource2 ; une province « bois+argile » n'a PAS de
         * grain). Le vivrier de base est au SPAWN (stock de la cité-état, CS_TRADE_POOL ;
         * la capitale d'empire en stock) et au COMMERCE — pas extrait par chaque tuile.
         * ⚠ Des FAMINES sont possibles (assumé pour l'instant). subsist sert encore aux
         * socles de cité-état (plus bas). */
        re->coastal = coastal;                       /* lu par la marine (rade) et l'agency (gate du Port) */
        re->estuary = false;                         /* posé au balayage des cellules ci-dessous */
        /* Dons géo SÉLECTIFS (gibier/halieutique) : ~1/3 des régions BOISÉES (majorité de
         * provinces forestières) et ~1/3 des CÔTIÈRES — tirage DÉTERMINISTE par région. */
        { unsigned hg=(unsigned)rid*2654435761u;
          if (rg->n_provinces>0 && wooded*2>=rg->n_provinces && (hg%3u)==0u) re->prov_geo |= PROVF_GIBIER;
          if (coastal && ((hg>>8)%3u)==0u)                                   re->prov_geo |= PROVF_HALIEUTIQUE; }

        /* ──────────────────────────────────────────────────────────────────────
         * MISE À NU — À L'EXCEPTION DES CITÉS-ÉTATS.
         *   · EMPIRE / JOUEUR : la carte naît NUE (terrain + gisements seuls,
         *     vocation régionale, ZÉRO bâtiment) ; l'IA/agency élèvent les
         *     manufactures DANS LE TEMPS (econ_build_tick §NF + chantiers payés).
         *   · CITÉ-ÉTAT : EXEMPTÉE — elle TIENT le marché mondial (#5), elle est
         *     l'ATELIER du monde où les empires pompent leurs biens. Elle naît donc
         *     ÉQUIPÉE comme avant la mise à nu : socles de matière, voiles arcanes,
         *     MANUFACTURES implantées au gisement, niveaux dimensionnés.
         * (cid = rg->country, connu l.462 ; owner pas encore posé sur re ici.) */
        bool is_city_state = (cid>=0 && cid<w->n_countries
                              && w->country[cid].role==POLITY_CITY_STATE);
        if (is_city_state){
            /* — socles de matière MINIMES (la cité-état file/scie/bâtit dès l'an 0) — */
            re->raw_cap[RES_WOOD]  += subsist * 0.12f;
            re->raw_cap[RES_CLAY]  += subsist * 0.08f;
            re->raw_cap[RES_STONE] += subsist * 0.05f;
            if (coastal) re->raw_cap[RES_FISH] += subsist * 0.10f;
            /* — ARCANE : cristal des nœuds telluriques (nœud riche 1/4 + voile diffus 0.2) — */
            if (re->raw_cap[RES_SULFUR]>0.f || re->raw_cap[RES_PRECIOUS_METAL]>0.f){
                re->raw_cap[RES_ARCANE_CRYSTAL] += 0.2f;                                   /* voile diffus */
                if (((uint32_t)(rid*2654435761u) % 4u)==0u) re->raw_cap[RES_ARCANE_CRYSTAL] += 1.0f;  /* + nœud riche */
            }
            /* — Fer céleste : météorique (nœud riche 1/9 + voile diffus 0.2) — */
            if (re->raw_cap[RES_IRON]>0.f){
                re->raw_cap[RES_CELESTIAL_IRON] += 0.2f;                                   /* voile diffus */
                if (((uint32_t)(rid*40503u+7u) % 9u)==0u) re->raw_cap[RES_CELESTIAL_IRON] += 0.8f;     /* + nœud riche */
            }
            /* — MANUFACTURES implantées là où l'intrant est extrait (cohérence de la chaîne) — */
            if (re->raw_cap[RES_WOOL] > 0.f){ region_ensure_building(re,BLD_TEXTILE);
                                              region_ensure_building(re,BLD_TUNIC); }  /* la tunique naît où l'on file */
            if (re->raw_cap[RES_WOOD] > 0.f) {
                region_ensure_building(re,BLD_SAWMILL);
                region_ensure_building(re,BLD_PAPERMILL);
                region_ensure_building(re,BLD_CHARCOAL);   /* charbon DU BOIS */
            }
            if (re->raw_cap[RES_SUGAR] > 0.f) region_ensure_building(re,BLD_WINERY);
            if (re->raw_cap[RES_GRAIN] > 0.f) region_ensure_building(re,BLD_BREWERY);   /* la bière naît du grain */
            if (re->raw_cap[RES_GOLD] > 0.f || re->raw_cap[RES_PEARL] > 0.f)
                region_ensure_building(re,BLD_JEWELER);
            if (re->raw_cap[RES_MUREX] > 0.f || re->raw_cap[RES_INDIGO] > 0.f)
                region_ensure_building(re,BLD_WEAVER_LUX);
            if (re->raw_cap[RES_IRON] > 0.f && re->raw_cap[RES_WOOD] > 0.f)
                region_ensure_building(re,BLD_TOOLWORKS);   /* fer + bois → outils (DIRECT) */
            if (re->raw_cap[RES_ARCANE_CRYSTAL] > 0.5f) region_ensure_building(re,BLD_MAGE_WORKSHOP);   /* nœud riche seul */
            if (re->raw_cap[RES_CELESTIAL_IRON] > 0.5f) region_ensure_building(re,BLD_CELESTIAL_FORGE);
            if (re->raw_cap[RES_IRON] > 0.f) region_ensure_building(re,BLD_ARMORY);
            if (re->raw_cap[RES_SALTPETER] > 0.f && re->raw_cap[RES_COAL] > 0.f)
                region_ensure_building(re,BLD_POWDERMILL);
            if (re->raw_cap[RES_MED_HERBS] > 0.f) region_ensure_building(re,BLD_APOTHECARY);
            /* Niveau initial : dimensionné sur la capacité d'accueil (infrastructure latente). */
            float invest = re->cap_pop*CLASS_SHARE[CLASS_BOURGEOIS];
            for (int i=0;i<re->n_bld;i++)
                re->bld[i].level = 0.5f + invest*0.01f;
        } else {
            /* EMPIRE / JOUEUR — nœuds stratégiques RARES (plus de voile diffus), puis VOCATION. */
            if (re->raw_cap[RES_SULFUR]>0.f || re->raw_cap[RES_PRECIOUS_METAL]>0.f){
                if (((uint32_t)(rid*2654435761u) % 4u)==0u) re->raw_cap[RES_ARCANE_CRYSTAL] += 1.0f;  /* nœud riche SEUL */
            }
            if (re->raw_cap[RES_IRON]>0.f){
                if (((uint32_t)(rid*40503u+7u) % 9u)==0u) re->raw_cap[RES_CELESTIAL_IRON] += 0.8f;     /* nœud riche SEUL */
            }
            /* WORLDGEN NE POSE AUCUN BÂTIMENT pour l'empire : la carte naît NUE — l'IA/agency
             * élèvent les manufactures DANS LE TEMPS (plus d'implantation au gisement). */
        }
        /* VOCATION — « 2 BRUTES PAR PROVINCE », SANS EXCEPTION (empire ET CITÉ-ÉTAT).
         * On ne garde que les REGION_RAW_KEEP=2 brutes EXTRAITES les plus FORTES (plus le
         * vivrier et les stratégiques rares, protégés) ; la longue traîne — agrégée des
         * provinces, socles de cité-état, voiles diffus — TOMBE. Le manquant vient du
         * COMMERCE (pool national + routes). La cité-état n'est PLUS exemptée du plafond
         * d'EXTRACTION : sa richesse vient de son gros STOCK de base (CS_TRADE_POOL), non
         * d'extraire davantage ; ses manufactures (posées plus haut, sur le raw AVANT coupe)
         * restent l'atelier du monde. (Coupe DÉPLACÉE hors du if/else → frappe TOUT le monde.) */
        {
            int keep = (int)tune_f("REGION_RAW_KEEP", 2.f);
            bool prot[RES_COUNT]; for (int g=0;g<RES_COUNT;g++) prot[g]=false;
            /* le GRAIN n'est PLUS protégé : il concourt comme toute brute (food = géologie).
             * Seuls les stratégiques RARES (fer céleste / cristal arcanique) restent protégés
             * — sinon la chaîne faustienne/endgame perdrait sa matière (ce n'est pas un
             * ajustement d'éco : c'est garder l'intrant rare de la tech). */
            prot[RES_CELESTIAL_IRON]=prot[RES_ARCANE_CRYSTAL]=true;
            /* CONSTRUCTION — argile & pierre PROTÉGÉES de la coupe : le brut de bâti SUIT la géo
             * (argile aux terres d'eau, pierre au relief) au lieu d'être ÉCRASÉ par les 2 brutes
             * dominantes. Source géologique BON MARCHÉ (extraction, pas la seule manufacture) ; les
             * RAW-WORKS restent le SUPPLÉMENT des régions pauvres + la chaîne confort. */
            prot[RES_CLAY]=prot[RES_STONE]=true;
            /* FRUIT PROTÉGÉ — « un peu partout » : le repli vin doit survivre à la coupe (vocation
             * mineure base·0.20) sinon le fruit n'existe nulle part et la winery-alt est morte. */
            prot[RES_FRUIT]=true;
            for (int k=0;k<keep;k++){
                int best=-1; float bv=0.f;
                for (int g=1;g<RES_PROD_FIRST;g++){ if (prot[g]||re->raw_cap[g]<=0.f) continue;
                    if (re->raw_cap[g]>bv){ bv=re->raw_cap[g]; best=g; } }
                if (best<0) break;
                prot[best]=true;
            }
            for (int g=1;g<RES_PROD_FIRST;g++) if (!prot[g]) re->raw_cap[g]=0.f;   /* la traîne tombe */
        }

        /* ---- Prix & stock de départ. */
        for (int r=0;r<RES_COUNT;r++) {
            re->price[r]=BASE_PRICE[r];
            re->stock[r]=0.f;
        }
    }

    /* ---- ESTUAIRES (commerce asym. §4) : la charnière fleuve ⇄ mer — là où le
     * vrac d'un bassin versant converge. Une cellule de CÔTE au débit notable
     * fait de sa région un entrepôt naturel (la bande Carrefour y montera). */
    for (int i=0;i<SCPS_N;i++){
        const Cell *c=&w->cell[i];
        if (!c->coast || c->river<40) continue;
        int r=c->region;
        if (r>=0 && r<e->n_regions){
            /* E1 : la plaine alluviale d'un estuaire est une argilière naturelle —
             * RESTAURÉE pour les CITÉS-ÉTATS seules (exemptées de la mise à nu) ; pour
             * l'empire l'argile vient de la GÉO (terres d'eau) soumise à la vocation. */
            int rc = w->region[r].country;
            bool cs = (rc>=0 && rc<w->n_countries && w->country[rc].role==POLITY_CITY_STATE);
            if (cs && !e->region[r].estuary) e->region[r].raw_cap[RES_CLAY] += 1.5f;
            e->region[r].estuary=true;
        }
    }

    /* ---- Adjacence de régions (terre, 4-connexe) pour la colonisation ----
     * Extraite en econ_build_adjacency (réutilisée par le recalcul du capstone
     * §27 : une carve eau/ronces déplace côtes et barrières). */
    econ_build_adjacency(e, w);

    /* ---- Peuplement initial : la GRAINE PAR-POLITÉ, le reste vierge -------- *
     * Re-baseline — la pop an-0 est SEMÉE PAR ENTITÉ (plus de total plat 48k réparti) :
     *   EMPIRE     → EMPIRE_SEED (4000) ;
     *   CITÉ-ÉTAT  → CITY_SEED   (2000) ;
     *   (WILD      → 2/empire · WILD_POP ≈ 750, plus bas).
     * La graine d'une polité se répartit UNIFORMÉMENT sur ses régions ACTIVES, bornée par
     * le plancher ½·cap_pop (la terre nue = eff_cap quand rien n'est bâti → pas de famine
     * d'amorçage). À l'an-0 nul ne domine DANS une polité ; la DIVERGENCE vient ENSUITE du
     * bâti. La capacité VISÉE (apex) reste EMPIRE_CAP/CITY_CAP (Passe 2) : la pop CROÎT de
     * sa graine vers son apex. La friche vierge reste à zéro (frontière à coloniser).
     * Membrane : on n'ajoute pas un bonus, on AMORCE la pop sous son plafond.
     *   an-0 ≈ n·EMPIRE_SEED + nCS·CITY_SEED + 2n·WILD_POP. */
    float empire_seed = tune_f("EMPIRE_SEED", 4000.f);
    float city_seed   = tune_f("CITY_SEED",   2000.f);
    for (int cid=0; cid<w->n_countries; cid++) {
        const Country *ct=&w->country[cid];
        PolityRole role=ct->role;
        float pol_seed;
        if      (role==POLITY_PLAYER || role==POLITY_ANTAGONIST) pol_seed=empire_seed;
        else if (role==POLITY_CITY_STATE)                        pol_seed=city_seed;
        else continue;
        /* Σ cap_pop des régions ACTIVES = clé de répartition (∝ capacité → la capitale, la
         * plus capable, en prend le plus). La graine de la polité se RÉPARTIT EXACTEMENT
         * (Σ parts = pol_seed PILE, aucune perte au plancher) : la pop an-0 est ainsi LOCKÉE
         * sur la formule n·EMPIRE_SEED + nCS·CITY_SEED + 2n·WILD_POP. Comme l'apex visé
         * (EMPIRE_CAP/CITY_CAP) > 2·graine, chaque part reste SOUS ½·cap_pop (anti-famine). */
        float capsum=0.f;
        for (int ri=0; ri<ct->n_regions; ri++){
            int rid=ct->region_ids[ri];
            if (rid>=0&&rid<e->n_regions&&e->region[rid].active) capsum+=e->region[rid].cap_pop;
        }
        if (capsum<=0.f) continue;
        for (int ri=0; ri<ct->n_regions; ri++){
            int rid=ct->region_ids[ri];
            if (rid<0||rid>=e->n_regions) continue;
            RegionEconomy *re=&e->region[rid];
            if (!re->active) continue;
            econ_seed_population(re, pol_seed * (re->cap_pop/capsum));   /* ∝ capacité, Σ = pol_seed PILE */
            re->colonized=true;
            re->owner=(int16_t)cid;
        }
    }

    /* REFONTE A5 — LA NOURRITURE DU SPAWN (la SEULE règle vivrière ; le reste = GÉOLOGIE).
     * Chaque EMPIRE naît avec une base vivrière sur sa région-CAPITALE : un socle de grain
     * qui en fait un GRENIER de départ (posé APRÈS la coupe de vocation → protégé). Les
     * autres régions tirent leur nourriture de la GÉOLOGIE (grain/poisson dans leur
     * vocation) et du COMMERCE (pool national + routes). Pas de socle UNIVERSEL : un empire
     * né sur terre stérile dépend de sa capitale et de ses échanges (la « Mali » qui commerce). */
    {
        float spawn_food = tune_f("SPAWN_FOOD_RAW", SPAWN_FOOD_RAW);
        for (int cid=0; cid<w->n_countries; cid++){
            PolityRole role=w->country[cid].role;
            if (role!=POLITY_PLAYER && role!=POLITY_ANTAGONIST) continue;
            int cp=w->country[cid].capital_prov;
            int cr=(cp>=0&&cp<w->n_provinces)? w->province[cp].region : -1;
            if (cr<0||cr>=e->n_regions||!e->region[cr].active) continue;
            if (e->region[cr].raw_cap[RES_GRAIN] < spawn_food)
                e->region[cr].raw_cap[RES_GRAIN] = spawn_food;   /* grenier de spawn (vocation vivrière garantie) */
        }
    }

    /* POOL TRADABLE DES CITÉS-ÉTATS (2026-06-16) : chaque cité-état naît avec une RÉSERVE
     * de matières BRUTES — CS_TRADE_POOL (1000) de BOIS / FER / ARGILE / PIERRE — sur sa
     * région-pivot. Le marché mondial (ses Centres, #5) la revend aux EMPIRES nés NUS : le
     * trio du bâti (bois/pierre/argile des chantiers) + le fer des outils/armes ont enfin
     * une SOURCE, l'empire importe de quoi élever ses manufactures au lieu de stagner au
     * plancher ½·cap_pop. (Posé APRÈS la remise à zéro des stocks de l.« Prix & stock ».) */
    {
        float pool = tune_f("CS_TRADE_POOL", 1000.f);
        for (int cid=0; cid<w->n_countries; cid++){
            if (w->country[cid].role!=POLITY_CITY_STATE) continue;
            for (int r=0; r<e->n_regions; r++){
                RegionEconomy *re=&e->region[r];
                if (re->owner!=cid || !re->active) continue;
                re->stock[RES_GRAIN] += pool;   /* + NOURRITURE de base (la cité-état nourrit le marché) */
                re->stock[RES_WOOD]  += pool;
                re->stock[RES_IRON]  += pool;
                re->stock[RES_CLAY]  += pool;
                re->stock[RES_STONE] += pool;
                break;   /* une réserve par cité-état (sa première région active = pivot) */
            }
        }
    }

    /* ──────────────────────────────────────────────────────────────────────
     * HAMEAUX LIBRES (POLITY_WILD) — les PEUPLES LIBRES. Pour CHAQUE empire on PLANTE
     * WILD_PER_PLAYABLE hameaux sur les régions VIERGES viables les plus PROCHES — BFS
     * multi-source BORNÉ à WILD_SPAWN_HOPS (un rayon de 2-3 tuiles autour du spawn : assez
     * près pour tuer le « siècle d'inertie » — 2 objectifs voisins dès l'an 0 —, jamais à
     * l'autre bout du monde). nearest-first → les plus proches d'abord. Chaque hameau :
     * WILD_POP (graine EXACTE), plafond WILD_CAP, réserve WILD_HOARD. WILD_PER_PLAYABLE=0 → aucun. */
    {
        int wild_cid=-1;
        for (int c=0;c<w->n_countries;c++) if (w->country[c].role==POLITY_WILD){ wild_cid=c; break; }
        int per=(int)tune_f("WILD_PER_PLAYABLE",2.f), hops=(int)tune_f("WILD_SPAWN_HOPS",3.f);
        if (wild_cid>=0 && per>0 && hops>0){
            float wpop=tune_f("WILD_POP",750.f), wvar=tune_f("WILD_POP_VAR",0.f);
            float wcap=tune_f("WILD_CAP",1600.f), whoard=tune_f("WILD_HOARD",60.f), wfood=tune_f("WILD_FOOD",8.f);
            /* Pose un hameau libre sur la région WS (graine exacte WILD_POP, sous ½·WILD_CAP). */
            #define WILD_PLANT(WS) do { \
                int ws_=(WS); RegionEconomy *wre=&e->region[ws_]; \
                wre->owner=(int16_t)wild_cid; wre->colonized=true; wre->culture.settled=true; \
                wre->cap_pop=wcap; \
                uint32_t hh=(uint32_t)ws_*2654435761u + (uint32_t)cid*40503u; \
                hh ^= hh>>13; hh *= 0x85ebca6bu; hh ^= hh>>16; \
                float jit=(wvar>0.f)? ((float)(hh % 2001u)/1000.f - 1.f)*wvar : 0.f; /* WILD_POP_VAR=0 → 0 (graine LOCKÉE) */ \
                econ_seed_population(wre, fminf(fmaxf(wpop+jit, 50.f), wcap*0.5f)); \
                wre->raw_cap[RES_GRAIN]=fmaxf(wre->raw_cap[RES_GRAIN], wfood); /* raw food FORCÉE : le hameau se nourrit */ \
                for (int g=1; g<RES_PROD_FIRST; g++) if (wre->raw_cap[g]>0.f) wre->stock[g]+=whoard; \
            } while(0)
            for (int cid=0; cid<w->n_countries; cid++){
                PolityRole role=w->country[cid].role;
                if (role!=POLITY_PLAYER && role!=POLITY_ANTAGONIST) continue;
                static int dist[SCPS_MAX_REG], q[SCPS_MAX_REG];   /* BFS multi-source depuis les régions de cid */
                for (int r=0;r<e->n_regions && r<SCPS_MAX_REG;r++) dist[r]=-1;
                int qh=0, qt=0;
                for (int r=0;r<e->n_regions && r<SCPS_MAX_REG;r++)
                    if (e->region[r].owner==cid){ dist[r]=0; q[qt++]=r; }
                int got=0;
                /* BFS BORNÉ au rayon hops (2-3 tuiles), nearest-first : les régions vierges
                 * viables LES PLUS PROCHES du spawn d'abord. */
                while (qh<qt && got<per){
                    int r=q[qh++];
                    if (dist[r]>=hops) continue;
                    for (int s=0;s<e->n_regions && s<SCPS_MAX_REG && got<per;s++){
                        if (!e->adj[r][s] || dist[s]>=0) continue;
                        dist[s]=dist[r]+1;
                        RegionEconomy *re=&e->region[s];
                        if (re->active && !re->colonized && !re->impassable && re->cap_pop>0.f){
                            WILD_PLANT(s); got++;
                        }
                        if (dist[s]<hops) q[qt++]=s;
                    }
                }
            }
            #undef WILD_PLANT
        }
    }

    if (getenv("SCPS_CAPDIAG")) {
        double capsum=0, seedsum=0; int nact=0, nrole[4]={0};
        for (int r=0;r<e->n_regions;r++){
            if (e->region[r].active){ capsum+=e->region[r].cap_pop; nact++; }
            for (int c=0;c<CLASS_COUNT;c++) seedsum+=e->region[r].strata[c].pop;
        }
        for (int c=0;c<w->n_countries;c++){ int rr=w->country[c].role; if(rr>=0&&rr<4) nrole[rr]++; }
        int n_wild=0; for (int r=0;r<e->n_regions;r++) if (e->region[r].colonized){
            int o=e->region[r].owner; if (o>=0&&o<w->n_countries&&w->country[o].role==POLITY_WILD) n_wild++; }
        fprintf(stderr,"[CAPDIAG] active=%d cap_pop_sum=%.0f seed_pop=%.0f | PLAYER=%d ANTAG=%d CS=%d UNCL=%d | hameaux WILD=%d\n",
                nact, capsum, seedsum, nrole[0],nrole[1],nrole[2],nrole[3], n_wild);
    }
}

/* ====================================================================== */
/* SIMULATION — un tick                                                   */
/* ====================================================================== */

/* §7 — Tolérance fiscale par ÉTHOS × classe : le SEUIL (×satisfaction) au-delà
 * duquel on FUIT l'impôt et l'on gronde. La culture chiffre la stratégie fiscale
 * (un Mercantile n'étrangle pas ses bourgeois ; un Bureaucrate extrait partout ;
 * un Dominateur essore la masse mais pas l'élite). */
float econ_tax_tolerance(Ethos e, SocialClass c){
    static const float T[ETHOS_COUNT][CLASS_COUNT] = {
        /*               Laborer Bourgeois Élite */
        /* DOMINATEUR */ {0.60f,  0.40f,   0.25f},
        /* HONNEUR    */ {0.55f,  0.40f,   0.22f},
        /* ORDRE      */ {0.55f,  0.52f,   0.42f},
        /* BUREAUCRATE*/ {0.60f,  0.60f,   0.58f},
        /* MERCANTILE */ {0.45f,  0.28f,   0.42f},
        /* PACIFISTE  */ {0.30f,  0.30f,   0.30f},
    };
    if (e<0||e>=ETHOS_COUNT||c<0||c>=CLASS_COUNT) return 0.40f;
    return T[e][c];
}
#define STATE_TAX_AMBITION 0.42f   /* le taux que l'État VISE (l'éthos décide ce qui rentre) */
#define K_TAX_AGIT         0.85f   /* poids de la surtaxe sur la satisfaction (la grogne) */
/* §B — DÉ-STÉRILISER LE TRÉSOR + FERMER LE CISEAU OFFRE/DEMANDE.
 *  STATE_SPEND_RATE : part ANNUELLE du trésor que l'État REDÉPENSE (×dt/tick) — il ne
 *  hoarde plus, il circule ; réglé pour un trésor à l'ÉQUILIBRE (≈ TAX/SPEND × richesse),
 *  pas ×16. PAYROLL_FRACTION : part de la dépense versée en GAGES aux classes (le reste
 *  subventionne l'expansion §1). EXPANSION_PRESSION_CAP : l'expansion d'une manufacture
 *  suit le signal-prix de son bien (prix/base), plafonné — l'offre RÉAGIT à la pénurie. */
#define STATE_SPEND_RATE       0.30f
#define PAYROLL_FRACTION       0.60f
#define BASE_EXPANSION         0.20f   /* §1 : vitesse d'expansion d'une manufacture, ∝ pénurie */
#define EXPANSION_PRESSION_CAP 5.0f    /* pénurie max prise en compte (prix/base − 1, plafonné) */

/* §NF v2 — CONSTRUCTION DÉMANDE-MENÉE, DÉLIÉE DU GISEMENT. L'ancien §NF (dans econ_init,
 * prix=0 → inerte) ne tournait JAMAIS : toute manufacture était clouée au gisement local
 * par l'implantation géographique. Ici, CHAQUE TICK : une manufacture s'élève là où son
 * OUTPUT manque (prix ≥ pénurie) et où il y a des BRAS, dès lors que son INTRANT existe
 * QUELQUE PART dans le royaume (le commerce l'achemine) — PLUS sur le gisement local. Le
 * bâtiment suit donc le MARCHÉ (demande + main-d'œuvre), pas l'extraction. region_ensure_
 * building est idempotent → sûr à appeler chaque tick ; le prix qui retombe éteint le signal. */
#define NF_REALM_MIN 0.5f   /* intrant « fournissable » s'il est extrait/produit ≥ ça quelque part dans le pays */
static void econ_build_tick(WorldEconomy *e){
    /* 1. disponibilité d'intrant À L'ÉCHELLE DU PAYS (extraction + offre, n'importe où). */
    static float owner_avail[SCPS_MAX_COUNTRY][RES_COUNT];
    for (int o=0;o<SCPS_MAX_COUNTRY;o++) for (int g=0;g<RES_COUNT;g++) owner_avail[o][g]=0.f;
    for (int r=0;r<e->n_regions;r++){
        RegionEconomy *re=&e->region[r];
        if (!re->active || !re->colonized || re->owner<0 || re->owner>=SCPS_MAX_COUNTRY) continue;
        for (int g=1; g<RES_COUNT; g++) owner_avail[re->owner][g] += re->raw_cap[g] + re->supply[g];
    }
    /* 2. par région PEUPLÉE : bâtir le producteur d'un bien LOCALEMENT en pénurie, si son
     *    intrant existe dans le royaume (ou déjà importé en stock). Plus de gate raw LOCAL. */
    for (int r=0;r<e->n_regions;r++){
        RegionEconomy *re=&e->region[r];
        if (!re->active || !re->colonized || re->owner<0 || re->owner>=SCPS_MAX_COUNTRY) continue;
        float rpop = re->strata[CLASS_LABORER].pop + re->strata[CLASS_BOURGEOIS].pop + re->strata[CLASS_ELITE].pop;
        if (rpop < NF_POP_FLOOR) continue;
        const float *avail = owner_avail[re->owner];
        for (int b=0;b<BLD_TYPE_COUNT;b++){
            const Recipe *rc=&RECIPE[b];
            if (rc->out<=RES_NONE || rc->out>=RES_COUNT || BASE_PRICE[rc->out]<=0.f) continue;
            if (b==BLD_FOREUSE && !re->tech_foreuse) continue;                   /* §B2 : foreuse gatée par la tech faustienne */
            if (b==BLD_ALAMBIC && !re->tech_alchimie) continue;                  /* F3 : alambic gaté par TECH_ALCHIMIE */
            if (b==BLD_REPLICATEUR && !re->tech_replicateur) continue;           /* FAU4 : gate TECH_TRANSMUTATION */
            if (b==BLD_CORNE && !re->tech_corne) continue;                       /* FAU4 : gate TECH_FORGE_RUNES */
            if (b==BLD_ARQUEBUS && !re->tech_arquebus) continue;                 /* F7 : gate TECH_POUDRIERE */
            if (re->price[rc->out] < BASE_PRICE[rc->out]*NF_SHORTAGE) continue;   /* output pas en pénurie ICI */
            bool feed1 = (rc->in1==RES_NONE)
                      || avail[rc->in1] > NF_REALM_MIN || re->stock[rc->in1] >= NF_STOCK_MIN
                      || (rc->alt1!=RES_NONE && (avail[rc->alt1] > NF_REALM_MIN || re->stock[rc->alt1] >= NF_STOCK_MIN));
            bool feed2 = (rc->in2==RES_NONE)
                      || avail[rc->in2] > NF_REALM_MIN || re->stock[rc->in2] >= NF_STOCK_MIN;
            if (!feed1 || !feed2) continue;        /* le royaume ne sait pas le nourrir → on ne bâtit pas à vide */
            int bi=region_ensure_building(re,(BuildingType)b);
            if (bi>=0 && re->bld[bi].level < NF_SEED_LEVEL) re->bld[bi].level = NF_SEED_LEVEL;
        }
    }
}

/* §B1 — pousse le bonus de PRODUCTION du pays propriétaire vers ses régions. prod_pct
 * (production) et eff_pct (efficacité d'emploi) se cumulent dans un seul multiplicateur
 * sur prod_mult — gain modeste, partout, NON faustien. Appelé avant econ_tick. */
void econ_apply_country_tech(WorldEconomy *e, const TechState *ts, int n_ts){
    if (!e) return;
    for (int r=0;r<e->n_regions;r++){
        RegionEconomy *re=&e->region[r];
        int o=re->owner;
        re->tech_prod = (ts && o>=0 && o<n_ts)
                      ? 1.f + tech_prod_bonus(&ts[o]) + tech_eff_bonus(&ts[o])
                      : 1.f;
        re->tech_foreuse = (ts && o>=0 && o<n_ts) ? ts[o].unlocked[TECH_FOREUSE] : false;  /* §B2 : gate de BLD_FOREUSE */
        re->tech_alchimie = (ts && o>=0 && o<n_ts) ? ts[o].unlocked[TECH_ALCHIMIE] : false; /* F3 : gate de BLD_ALAMBIC */
        re->tech_replicateur = (ts && o>=0 && o<n_ts) ? ts[o].unlocked[TECH_TRANSMUTATION] : false; /* FAU4 : gate de BLD_REPLICATEUR */
        re->tech_corne = (ts && o>=0 && o<n_ts) ? ts[o].unlocked[TECH_FORGE_RUNES] : false;          /* FAU4 : gate de BLD_CORNE */
        re->tech_arquebus = (ts && o>=0 && o<n_ts) ? ts[o].unlocked[TECH_POUDRIERE] : false;          /* F7 : gate de BLD_ARQUEBUS */
    }
}

/* ── E0.7 — MOBILITÉ DE CLASSE (le dégel) ────────────────────────────────────
 * CLASS_SHARE {0.80,0.15,0.05} n'est qu'un DÉPART. Chaque mois, la RICHESSE fait
 * monter (journalier→bourgeois→élite), la MISÈRE fait descendre. La richesse SUIT
 * l'individu (transfert pro-rata). La promotion vers bourgeois exige des
 * manufactures (un débouché). Lecteurs : panier (coût des besoins/tête, capté au
 * tick) + satisfaction (deux mois bas → on déclasse). */
/* ── E1bis.10 — ENTRETIEN par région (la loi continue) ───────────────────────
 * Le modèle agrège les édifices en deltas (pas de registre par-édifice) : on lit
 * donc l'INFRASTRUCTURE bâtie (Σ deltas) comme proxy du prix de revient cumulé.
 * entretien/j = build_gold / ENTRETIEN_DIV (l'édifice se repaie en ~13 mois) ;
 * IMPAYÉ (trésor ≤ 0) → la région se met EN FRICHE (prod entaillée) jusqu'à
 * régularisation. Calibré pour qu'un revenu de taxes typique tienne l'entretien. */
#define ENTRETIEN_DIV         400.f
#define BUILD_GOLD_PER_DELTA  35.f    /* delta de ProvBuild → or de revient (proxy d'audit) */
#define FRICHE_FACTOR         0.6f    /* production entaillée tant que l'entretien n'est pas payé */
/* H7 — ENCADREMENT DES MANUFACTURES (la racine du robinet d'or) : chaque niveau de
 * manufacture active coûte 0.05 or/jour × IPM. Les 55 scieries ne tournent plus gratis. */
#define MANUF_UPKEEP_DAY      0.05f   /* encadrement impayé → la région passe en friche (0.6× prod), comme l'entretien */
/* G0.4 — TRAIN DE VIE DE COUR : le trésor est PAR RÉGION (un pays en a plusieurs) →
 * seuil par région calé pour mordre au-delà d'un trésor-pays ~10k (le faste freine le
 * hoarding). 1 %/mois du surplus. */
#define COURT_FLOOR         4000.f
#define COURT_RATE          0.010f
/* I3 — ADMIN : coût mensuel d'un pays = ADMIN_BASE × n_régions^ADMIN_EXP × IPM
 * (2 rég ≈ 1 · 16 rég ≈ 15 · 25 rég ≈ 27). Réparti sur ses régions. */
#define ADMIN_BASE             0.4f
#define ADMIN_EXP              1.3f
/* I3bis — RÉSERVE D'EXPLOITATION : le trésor d'une région ne descend JAMAIS sous ce
 * plancher par les dépenses (entretien, redépense d'État) — un État garde toujours de
 * quoi fonctionner (bâtir un grenier, amorcer une route). Sans cette réserve, une thune
 * fine se vide à 0 et s'enferme : à 0 on ne bâtit plus rien → famine/friche perpétuelle
 * (prod 0.6× → moins d'impôt → friche). Distinct du seuil de HOARDING (COURT_FLOOR), bien
 * plus haut, au-dessus duquel mordent les ponctions anti-thésaurisation (faste/admin/IPM). */
#define SINK_FLOOR           500.f
#define DEF_UPKEEP_MULT      1.5f    /* I3 — la famille défensive (H) s'entretient ×1.5 */
static bool g_friche[SCPS_MAX_REG];   /* E1bis.10 : région en friche (entretien/encadrement impayé) */
static long g_n_friche;               /* télémétrie : régions en friche au dernier tick */
long econ_friche_count(void){ return g_n_friche; }

/* M6 (forks §14) — la table des deltas de flux arcanes (design, lue par le banc).
 * Le PUITS (alambic, négatif) est branché dans econ_tick : l'essence purifiée
 * consommée NEUTRALISE la charge arcane (0.5 charge par unité). L'accrual mage
 * existant reste inchangé (calibré par econ_arcane_demo) — la table dit le SIGNE
 * et l'ORDRE (forge > mage > 0). F-arc : l'alambic n'est PLUS un puits (0). */
float econ_bld_flux_delta(BuildingType b){
    switch(b){
        case BLD_CELESTIAL_FORGE: return  1.2f;
        case BLD_MAGE_WORKSHOP:   return  0.8f;
        case BLD_ALAMBIC:         return  0.f;   /* F-arc RETRAIT : plus un puits — distillation neutre */
        case BLD_FOREUSE:         return  1.0f;  /* FAU2 transmuteur (fer) — source de charge */
        case BLD_REPLICATEUR:     return  0.6f;  /* FAU2 transmuteur (bois) */
        case BLD_CORNE:           return  1.2f;  /* FAU2 transmuteur (nourriture) — fer céleste, le plus profond */
        default:                  return  0.f;
    }
}
/* M6 — la MATIÈRE gate l'arcane (design §4.2 : la rareté EST le verrou). */
bool econ_bld_can_build(const WorldEconomy *e, int region, BuildingType b){
    if (!e || region<0 || region>=e->n_regions) return false;
    const RegionEconomy *re=&e->region[region];
    switch(b){
        case BLD_CELESTIAL_FORGE: return re->raw_cap[RES_CELESTIAL_IRON] > 0.f;
        case BLD_MAGE_WORKSHOP:   return re->raw_cap[RES_ARCANE_CRYSTAL] > 0.f;
        case BLD_ALAMBIC:         return re->raw_cap[RES_SALTPETER]      > 0.f;
        case BLD_CORNE:           return re->raw_cap[RES_CELESTIAL_IRON] > 0.f;  /* FAU2 : la Corne exige le fer céleste */
        default:                  return true;                                   /* Réplicateur : flux produit (feed-check) */
    }
}
/* FAU0 #4 — LE PRÉDICAT « TRANSMUTEUR FAUSTIEN » (un seul point de vérité) : les trois
 * échappatoires qui transmutent un bien vital en payant la charge. La décrue passive (FAU1)
 * et les compteurs de conso (FAU0 #3) s'y accrochent, pas des if type==... épars. */
bool bld_is_faustian(BuildingType b){
    return b==BLD_FOREUSE || b==BLD_REPLICATEUR || b==BLD_CORNE;
}
/* F-arc ARSENAL — une SORTIE d'ARMEMENT (les 7 biens militaires : armes légères/lourdes/trait/feu,
 * armes enchantées, bâton de mage, kit d'alchimiste). Sert à reconnaître une manufacture D'ARMES :
 * sa sortie verse ×MANUF_ARMS_MULT au STOCK (l'arsenal que la levée pompe), sans toucher au marché. */
static int res_is_arm(int r){
    return r==RES_ARMS || r==RES_ARMS_HEAVY || r==RES_ARMS_RANGED || r==RES_FIREARM
        || r==RES_MAGE_STAFF || r==RES_ALCHEMIST_KIT || r==RES_ENCHANTED_ARMS;
}
/* FAU0 #2 — LE HOOK DE CHARGE UNIQUE : un seul robinet que TOUT faustien appelle (atelier de
 * mage, les 3 transmuteurs, et plus tard la merveille du capstone). Ajoute à l'activité du tick
 * (arcane_charge) ; l'accumulation/décrue vers l'entropie cumulée (faust_charge) est au tick. */
void faust_charge_add(RegionEconomy *re, float amount){
    if (re && amount>0.f) re->arcane_charge += amount;
}

/* F-arc — LA POMPE D'ARMES, branchée par l'APPLICATION (chronicle/viewer) via econ_set_arms_pump :
 * un crochet (WorldEconomy*, region, good, want)→prélevé qui source les armes propre→Centre→mondial.
 * Le MOTEUR ne dépend donc PAS du sous-système commerce (les bancs unitaires gardent le stock propre
 * seul) ; seule l'app câble le marché des cités-états. */
static float (*g_arms_pump)(WorldEconomy*, int, int, float, float) = NULL;
void econ_set_arms_pump(float (*pump)(WorldEconomy*, int, int, float, float)){ g_arms_pump = pump; }

void econ_set_prod_cap(int c,int g,float v){ if(c>=0&&c<SCPS_MAX_COUNTRY&&g>RES_NONE&&g<RES_COUNT) g_prod_cap[c][g]=v; }
float econ_prod_cap(int c,int g){ return (c>=0&&c<SCPS_MAX_COUNTRY&&g>RES_NONE&&g<RES_COUNT)?g_prod_cap[c][g]:-1.f; }

/* F6 (Option B) — CONSOMME `need` armes MACRO (RES_*) du stock de l'empire (région par région),
 * REGISTRE la demande (→ la fabrique produit → consomme le FER) et RENVOIE la quantité prélevée
 * (plafonnée par le stock). UN SEUL ROBINET : levée (warhost) ET renfort (campaign) y passent. */
long econ_arms_take(WorldEconomy *econ, int cid, Resource arm, long need){
    if (!econ || need<=0 || arm<=RES_NONE || arm>=RES_COUNT) return 0;
    int nown=0; for (int r=0;r<econ->n_regions;r++) if(econ->region[r].owner==cid) nown++;
    float dshare = nown ? (float)need/(float)nown : 0.f;
    long got=0;
    for (int r=0;r<econ->n_regions;r++){
        if (econ->region[r].owner!=cid) continue;
        econ->region[r].demand[arm] += dshare;   /* F8 BOOTSTRAP : la DEMANDE (want) — la fabrique bâtit/produit même stock VIDE → consomme le fer */
        if (got>=need) continue;
        /* #5 — POMPE D'ARMES (branchée par l'app via econ_set_arms_pump) : stock PROPRE de la région
         * → Centre de la cité-état la + proche → réseau mondial. Les cités-états (armuriers) fournissent
         * l'arme spécialisée que la région ne fabrique pas. Sans pompe (bancs unitaires) : stock PROPRE
         * seul — le comportement d'origine, sans dépendance au sous-système commerce. */
        if (g_arms_pump) got += (long)g_arms_pump(econ, r, (int)arm, (float)(need-got), econ->region[r].price[arm]);
        else { long take=(long)fminf((float)(need-got), fmaxf(0.f,econ->region[r].stock[arm]));
            econ->region[r].stock[arm]-=(float)take; got+=take; }
    }
    return got;
}

/* F-arc (bâti DÉLIBÉRÉ des manufactures) — le TIER de capitale (T1-7) requis pour poser une
 * manufacture : les fabriques avancées (armes lourdes/feu, arcane, transmuteurs) exigent une ville
 * mûre (le joueur spawn T4). Les manufactures de base : T1 (suite hameau). */
int bld_min_tier(BuildingType b){
    switch(b){
        case BLD_ARMORY_HEAVY: case BLD_BOWYER:                 return 2;
        case BLD_ARQUEBUS: case BLD_MAGE_WORKSHOP:              return 3;
        case BLD_CELESTIAL_FORGE: case BLD_ALAMBIC:             return 4;
        case BLD_FOREUSE: case BLD_REPLICATEUR: case BLD_CORNE: return 5;
        default:                                                return 1;   /* manufactures de base */
    }
}
/* POSER une manufacture DÉLIBÉRÉMENT (le joueur/IA la choisit ; pas d'auto-bâti). L'appelant a vérifié
 * le tier + payé l'or. Renvoie true si bâtie (ou déjà présente). */
bool econ_build_manufacture(WorldEconomy *econ, int region, BuildingType b){
    if (!econ || region<0 || region>=econ->n_regions) return false;
    RegionEconomy *re=&econ->region[region];
    int bi=region_ensure_building(re, b);
    if (bi<0) return false;
    /* une fabrique DÉLIBÉRÉE (payée) naît SUBSTANTIELLE — un vrai atelier, pas une semence : elle
     * produit assez pour armer des régiments (la prod plafonne de toute façon sur l'intrant + les bras). */
    if (re->bld[bi].level < 5.f) re->bld[bi].level = 5.f;
    return true;
}

/* I0 — L'INSTRUMENT : décomposition du flux d'or par empire. */
static double g_flux[SCPS_MAX_COUNTRY][FX_COUNT];
void econ_flux_add(int cid, FluxComp comp, float amount){
    if (cid<0||cid>=SCPS_MAX_COUNTRY||comp<0||comp>=FX_COUNT) return;
    g_flux[cid][comp] += amount;
}
double econ_flux_get(int cid, FluxComp comp){
    return (cid>=0&&cid<SCPS_MAX_COUNTRY&&comp>=0&&comp<FX_COUNT) ? g_flux[cid][comp] : 0.0;
}
void econ_flux_reset(void){ memset(g_flux,0,sizeof g_flux); }
const char *econ_flux_name(FluxComp comp){
    static const char *N[FX_COUNT]={
        "taxes","export","péages+",
        "entretien","cour","admin","encadr.",
        "soldes","marine","audits","péages−","invest.","conseil","import" };
    return (comp>=0&&comp<FX_COUNT)?N[comp]:"?";
}
float econ_base_price(Resource r){ return (r>RES_NONE && r<RES_COUNT)? BASE_PRICE[r] : 0.f; }
/* Le FOND de matière de bâti qu'une région garde avant d'exporter le surplus (cf. gate de chantier).
 * 0 pour tout ce qui n'est pas un matériau d'édifice (table EDIFICES de scps_agency). Tunable. */
float econ_build_reserve(Resource r){
    switch(r){
        case RES_WOOD: case RES_CLAY: case RES_STONE:   /* le TRIO — seuls matériaux d'édifice (bois/pierre/argile) */
            return tune_f("BUILD_RESERVE_BULK", 60.f);
        default: return 0.f;
    }
}

/* Q1 — LE CONSEIL (I7) : multiplicateurs PAR PAYS, rafraîchis chaque tick par la
 * couche sim depuis l'état conseil (statecraft). Transitoire (pas en SAVE — l'état
 * persistant, lui, vit dans Statecraft). seat : 0=Savoir 1=Société 2=Industrie.
 * Valeur ≤0 (jamais posée) = neutre 1.0. LECTEUR des valeurs existantes, jamais une pose. */
static float g_council_mult[SCPS_MAX_COUNTRY][3];
void econ_set_council_mult(int cid, int seat, float m){
    if (cid<0||cid>=SCPS_MAX_COUNTRY||seat<0||seat>=3) return;
    g_council_mult[cid][seat]=m;
}
static inline float council_m(int owner, int seat){
    if (owner<0||owner>=SCPS_MAX_COUNTRY) return 1.f;
    float v=g_council_mult[owner][seat];
    return (v>0.f)? v : 1.f;
}

#define PROMOTE_RATE 0.005f       /* 0.5 %/mois max (∝ richesse excédentaire) */
/* Seuil d'accession : « 3× le panier » (brief E0.7). Mais l'éco cale les journaliers
 * à ~60 % de satisfaction (peu de surplus) : à 3× la montée journalier→bourgeois ne
 * s'amorce jamais. CALIBRÉ à la PREUVE (part bourgeoise qui dérive vers le haut en
 * région industrieuse) — la borne sert l'effet, pas la lettre. */
#define PROMOTE_BASKET_MULT       1.4f   /* journalier → bourgeois : 1.4× le panier */
#define PROMOTE_BASKET_MULT_ELITE 2.5f   /* B4 — bourgeois → élite : 2.5× (l'élite se MÉRITE,
                                          * elle ne se gonfle plus à 13 % au même seuil que B) */
#define PROMOTE_SAT_GATE          0.50f  /* B4 — jamais de promotion VERS une strate dont la
                                          * satisfaction est < 50 % (on ne grossit pas une élite
                                          * misérable — la cause du « 13 % à la pire satisfaction ») */
static float   g_basket_pc[SCPS_MAX_REG][CLASS_COUNT];   /* panier/tête capté au tick */
static uint8_t g_lowsat_streak[SCPS_MAX_REG][CLASS_COUNT];/* mois consécutifs de sat < 30 % */
void econ_mobility_reset(void){
    memset(g_basket_pc,0,sizeof g_basket_pc);
    memset(g_lowsat_streak,0,sizeof g_lowsat_streak);
    memset(g_friche,0,sizeof g_friche); g_n_friche=0;
}
static void mobility_move(RegionEconomy *re, int from, int to, float frac){
    float pop=re->strata[from].pop; if (pop<1.f || frac<=0.f) return;
    float moved=pop*frac; if (moved<0.01f) return;
    float wmoved=re->strata[from].wealth*(moved/pop);     /* la richesse SUIT */
    re->strata[from].pop-=moved; re->strata[from].wealth-=wmoved;
    re->strata[to].pop  +=moved; re->strata[to].wealth  +=wmoved;
}
/* plafond DOUX d'une classe (part de pop) : sans lui, l'accession court sur 250 ans
 * jusqu'à 30-40 % de noblesse. Le taux s'éteint quand la cible approche son plafond.
 * L'élite a droit à de la marge (les bâtiments type K — admin/capitale — EMPLOIENT
 * des nobles : + d'élite est normal) ; le plafond ne coupe que l'emballement. */
#define SHARE_CAP_BOURGEOIS 0.32f
#define SHARE_CAP_ELITE     0.11f   /* B4 — resserré (0.20→0.11) : l'élite visait 5-9 %, pas 13-20 % */
static void mobility_tick_region(RegionEconomy *re, int rid){
    bool manuf = re->n_bld>0;                              /* manufactures actives = débouché */
    float totp = re->strata[0].pop+re->strata[1].pop+re->strata[2].pop; if (totp<1.f) totp=1.f;
    /* PROMOTIONS : wealth/tête > seuil → fraction ∝ excédent, ÉTEINTE au plafond doux. */
    for (int k=0;k<2;k++){
        int from=(k==0)?CLASS_LABORER:CLASS_BOURGEOIS, to=from+1;
        if (from==CLASS_LABORER && !manuf) continue;       /* sans atelier, pas d'accession bourgeoise */
        /* B4 — la porte de SATISFACTION : on ne promeut JAMAIS vers une strate
         * déjà sous 50 % (sinon on gonfle une élite misérable — le rouge du brief). */
        if (re->strata[to].satisfaction < PROMOTE_SAT_GATE) continue;
        float pop=re->strata[from].pop; if (pop<1.f) continue;
        float wpc=re->strata[from].wealth/pop;
        /* B4 — seuils SÉPARÉS : 1.4× pour J→B, 2.5× pour B→É (l'élite se mérite). */
        float mult=(k==0)?PROMOTE_BASKET_MULT:PROMOTE_BASKET_MULT_ELITE;
        float thr=mult*fmaxf(g_basket_pc[rid][from],0.05f);
        if (wpc<=thr) continue;
        float excess=clampf((wpc-thr)/thr,0.f,1.f);
        float rate=(k==0)?PROMOTE_RATE:PROMOTE_RATE*0.2f;  /* bourgeois→élite : ÷5 */
        float cap=(to==CLASS_ELITE)?SHARE_CAP_ELITE:SHARE_CAP_BOURGEOIS;
        float damp=clampf(1.f - (re->strata[to].pop/totp)/cap, 0.f, 1.f);  /* plafond doux */
        mobility_move(re, from, to, rate*excess*damp*council_m(re->owner,1));  /* Q1 : siège Société */
    }
    /* DÉMOTIONS : satisfaction < 30 % DEUX mois de suite → on redescend (∝ vif). */
    for (int from=CLASS_ELITE; from>=CLASS_BOURGEOIS; from--){
        if (re->strata[from].satisfaction < 0.30f){
            if (g_lowsat_streak[rid][from]<255) g_lowsat_streak[rid][from]++;
        } else { g_lowsat_streak[rid][from]=0; continue; }
        if (g_lowsat_streak[rid][from] >= 2) mobility_move(re, from, from-1, PROMOTE_RATE*2.f);
    }
}

/* §C — bornes de l'inflation : douce et bornée (jamais d'emballement). */
#define IPM_LO       0.85f   /* déflation plancher (−15 %) — bande DOUCE (effet volontairement modeste) */
#define IPM_HI       1.35f   /* inflation plafond (+35 %)  — un rescale uniforme est presque inerte ; on borne court */
#define IPM_INERTIA  0.9987f /* l'IPM répond sur ~1,5 an (demi-vie) */
#define IPM_REF_INER 0.9996f /* la TENDANCE glisse sur ~5 ans : l'IPM mesure l'ÉCART à la tendance,
                              * pas le stock — un afflux d'or BRUSQUE (Filon/Débase) le fait monter
                              * puis il mean-reverte ; le thésaurisation LENTE ne compte pas (la
                              * référence la rattrape). Sans ça, or(stock)/pib(flux) dérive sans fin. */

float econ_world_ipm(const WorldEconomy *e){ return (e && e->ipm>0.f)? e->ipm : 1.f; }

/* Pool empire d'un bien : Σ stock des régions de même owner. Ce que le joueur POSSÈDE
 * (hors import) — la topbar/Stocks le lisent pour ne pas mentir. */
long econ_empire_stock(const WorldEconomy *e, int owner, Resource g){
    if (!e || owner<0 || g<=RES_NONE || g>=RES_COUNT) return 0;
    int n=e->n_regions; if(n>SCPS_MAX_REG)n=SCPS_MAX_REG;
    double s=0.0;
    for (int r=0;r<n;r++) if (e->region[r].owner==owner) s+=e->region[r].stock[g];
    return (long)s;
}

/* or NET d'un pays = Σ trésor de ses régions (négatif = dette). Partagé chronicle/credit. */
double econ_country_gold(const WorldEconomy *e, int c){
    if (!e) return 0.0;
    double g=0.0; int n=e->n_regions; if(n>SCPS_MAX_REG)n=SCPS_MAX_REG;
    for(int r=0;r<n;r++) if(e->region[r].owner==c) g+=e->region[r].treasury;
    return g;
}

void econ_tick(WorldEconomy *e, float dt) {
    if (dt<=0.f) dt=1.f;
    e->tick++;
    g_n_friche=0;                      /* E1bis.10 : recompte les régions en friche ce tick */

#if SCPS_IPM
    /* §C — INFLATION MONÉTAIRE (un seul interrupteur). L'IPM = indice des prix :
     * trop d'OR (Σtrésor) pour trop peu de BIENS (ΣPIB) → les prix montent. On lit
     * l'état AGRÉGÉ du tick précédent, normalisé par le ratio de RÉFÉRENCE capté au
     * 1er tick → IPM≈1 au départ ; glisse lentement, reste borné. Le couplage aux
     * événements (§F) ÉMERGE : le Filon/la Débase injectent de l'or → IPM↑ ; les
     * Moissons font des biens → IPM↓ — aucun hook dédié. */
    { double gold=0.0, goods=0.0;
      for (int r=0;r<e->n_regions;r++){ if (!e->region[r].colonized) continue;
          gold  += e->region[r].treasury;
          goods += e->region[r].gdp; }
      if (goods>1.0){
          float ratio = (float)(gold/goods);
          if (e->ipm_ref<=0.f) e->ipm_ref = ratio;                          /* amorce la tendance */
          float target = clampf(ratio/e->ipm_ref, IPM_LO, IPM_HI);          /* écart à la tendance */
          e->ipm     = clampf(e->ipm*IPM_INERTIA + target*(1.f-IPM_INERTIA), IPM_LO, IPM_HI);
          e->ipm_ref = e->ipm_ref*IPM_REF_INER + ratio*(1.f-IPM_REF_INER);  /* la tendance suit, lentement */
      }
    }
#endif

    /* I3 — ADMIN : compte les régions par pays (le multiplicateur de TAILLE :
     * l'hégémon paie sa bureaucratie, les petits respirent). */
    int rcount[SCPS_MAX_COUNTRY]={0};
    for (int r=0;r<e->n_regions && r<SCPS_MAX_REG;r++){
        if (e->region[r].colonized && e->region[r].owner>=0 && e->region[r].owner<SCPS_MAX_COUNTRY)
            rcount[e->region[r].owner]++;
        /* K4b — le timer anti-saccage décroît pour TOUTE région (c'est un compteur, pas de
         * la production) : une province non colonisée mais sacquée se rouvre quand même
         * après ~5 ans. (Avant : décrément gaté par colonized → jamais pour l'incolonisée.) */
        e->region[r].pillage_cd = fmaxf(0.f, e->region[r].pillage_cd - dt);
    }

    /* ====================================================================
     * STOCK NATIONAL (le pool d'empire) — « toute ressource produite va dans le
     * stock de SON empire ». On AGRÈGE les stocks régionaux en un pool par pays :
     * l'extraction y dépose, la manufacture & la consommation Y PUISENT (la
     * matière d'une province nourrit l'atelier d'une autre — fin de la
     * fragmentation qui bloquait les chaînes). La MAIN-D'ŒUVRE reste LOCALE (on ne
     * staffe pas une fabrique avec les bras d'ailleurs). En clôture de tick, le
     * pool est REDISTRIBUÉ aux régions au prorata de la population (Σ re->stock =
     * pool) → les lecteurs externes (intertrade/Centres, viewer, butin de guerre,
     * save) gardent une vue régionale cohérente, sans réécriture des 280 sites.
     * ── PRIX NATIONAL (refonte) : le prix n'est PLUS soldé par-région (pop-share) — ce qui
     * créait un ARTEFACT spatial (un bien fait dans 1 région flambait dans les autres : outils
     * à 51×). Il est soldé UNE FOIS par empire sur l'offre/demande NATIONALES (supply_nat/
     * demand_nat ci-dessous) vs le pool → MÊMES paliers ⇒ ratio invariant à l'échelle : ni
     * artefact régional, ni effondrement d'effort (le piège évité était le MÉLANGE demande
     * régionale / stock national). Le prix national est PROJETÉ sur re->price de chaque région
     * (matérialisation, comme re->stock). Empire mono-région ⇒ national = local : IDENTIQUE.
     * Région ISOLÉE (owner<0, fixtures) ⇒ prix soldé LOCALEMENT (repli inchangé). */
    float pool[SCPS_MAX_COUNTRY][RES_COUNT];
    memset(pool, 0, sizeof pool);
    /* accumulateurs NATIONAUX (statiques = hors pile) du tick courant, pour le prix national. */
    static float supply_nat[SCPS_MAX_COUNTRY][RES_COUNT], demand_nat[SCPS_MAX_COUNTRY][RES_COUNT];
    memset(supply_nat, 0, sizeof supply_nat);
    memset(demand_nat, 0, sizeof demand_nat);
    float epop[SCPS_MAX_COUNTRY]={0}, elab[SCPS_MAX_COUNTRY]={0}, ecap[SCPS_MAX_COUNTRY]={0};
    for (int r=0;r<e->n_regions && r<SCPS_MAX_REG;r++){
        RegionEconomy *ar=&e->region[r];
        if (!ar->active || !ar->colonized) continue;
        int o=ar->owner; if (o<0||o>=SCPS_MAX_COUNTRY) continue;
        for (int g=0;g<RES_COUNT;g++) pool[o][g]+=ar->stock[g];
        epop[o]+=ar->strata[CLASS_LABORER].pop+ar->strata[CLASS_BOURGEOIS].pop+ar->strata[CLASS_ELITE].pop;
        elab[o]+=ar->strata[CLASS_LABORER].pop+ar->strata[CLASS_BOURGEOIS].pop;   /* bassin de travail NATIONAL = journaliers + bourgeois */
        ecap[o]+=ECON_STOCK_CAP_BASE+ECON_STOCK_CAP_ENTREPOT*(float)ar->n_entrepot;
    }
    /* OUTILS — l'usure du PARC NATIONAL se fait UNE fois/tick (un ×0.97 par-région
     * sur un pool partagé le décaierait N fois). tools_pc lira ce parc déjà usé. */
    for (int c=0;c<SCPS_MAX_COUNTRY;c++) if (epop[c]>0.f) pool[c][RES_TOOLS]*=0.97f;

    /* REFONTE A0/A2 — les leviers éco labor-bound, lus UNE fois/tick (SCPS_TUNE-ables). */
    const float ext_geo_ref   = tune_f("EXTRACT_GEO_REF",     EXTRACT_GEO_REF);
    const float ext_geo_cap   = tune_f("EXTRACT_GEO_CAP",     EXTRACT_GEO_CAP);
    const float ext_lab_share = tune_f("EXTRACT_LABOR_SHARE", EXTRACT_LABOR_SHARE);
    const float food_need     = tune_f("FOOD_NEED",           1.0f);   /* A2 : calibrage de la bouche vivrière */

    for (int rid=0; rid<e->n_regions && rid<SCPS_MAX_REG; rid++) {
        RegionEconomy *re=&e->region[rid];
        if (!re->active || !re->colonized) continue;

        float supply[RES_COUNT]={0}, demand[RES_COUNT]={0};
        /* « 100 emplois = 100 emplois, qu'ils soient artisans ou bourgeois » : le BASSIN de
         * main-d'œuvre = JOURNALIERS + BOURGEOIS (l'élite, classe dirigeante, ne travaille pas).
         * Extraction ET manufacture y puisent. (Re-baseline : le bassin grandit ~+19 %.) */
        float labor_avail = re->strata[CLASS_LABORER].pop + re->strata[CLASS_BOURGEOIS].pop;
        float labor_used  = 0.f;
        float gdp         = 0.f;
        float wage_pool   = 0.f;   /* → laborers */
        float profit_pool = 0.f;   /* → bourgeois */
        float tax_pool    = 0.f;   /* → rente d'élite */
        float over_tax[CLASS_COUNT]={0};   /* surtaxe par classe (grogne, §6) */
        /* STOCK NATIONAL : cette région opère sur le pool de SON empire (matière
         * fongible) ; sa PART de population (pshare) sert à solder le prix à
         * l'échelle locale (le signal-effort ne s'effondre pas). elab_ = la
         * main-d'œuvre de TOUT l'empire (le parc d'outils est national). */
        int    owner_ = re->owner;
        float *S      = (owner_>=0 && owner_<SCPS_MAX_COUNTRY) ? pool[owner_] : re->stock;
        float  rp_    = re->strata[CLASS_LABORER].pop+re->strata[CLASS_BOURGEOIS].pop+re->strata[CLASS_ELITE].pop;
        float  pshare = (owner_>=0 && owner_<SCPS_MAX_COUNTRY && epop[owner_]>EPS) ? rp_/epop[owner_] : 1.f;
        float  elab_  = (owner_>=0 && owner_<SCPS_MAX_COUNTRY && elab[owner_]>0.f) ? elab[owner_] : labor_avail;
        /* OUTILS = le MULTIPLICATEUR de productivité : le parc NATIONAL (par tête de
         * l'empire) booste l'extraction ET la manufacture (rendements décroissants,
         * +30% max). Les outils s'USENT (usure nationale, ci-dessus). */
        float tools_pc  = S[RES_TOOLS] / (elab_*0.1f + 1.f);
        float prod_mult = 1.f + 0.30f*(1.f - 1.f/(1.f + tools_pc));   /* lit le parc national (déjà usé ce tick) */
        /* OUTILS — INPUT PASSIF de la main-d'œuvre : les journaliers veulent être ÉQUIPÉS ∝
         * leur nombre, mais SATURANT (on n'outille pas au-delà de l'utile). La demande
         * effective est le COMBLEMENT du déficit d'outillage vers ce palier — elle tire le
         * prix (donc le §NF qui bâtit l'atelier + la perception IA), se RÉOUVRE chaque tick
         * par l'usure, et ne touche QUE prod_mult (ci-dessus), JAMAIS la satisfaction. */
        {
            float tools_target = labor_avail * TOOLS_PER_LABORER;            /* stock-outil VISÉ ∝ bras */
            demand[RES_TOOLS] += fmaxf(0.f, tools_target - S[RES_TOOLS]*pshare);  /* déficit (part régionale du parc) à combler */
        }
        /* §C3 : le « rot » de l'État (capture par concession) mine l'efficacité NOBLE —
         * une élite gorgée gouverne mal : moins de productivité de capitale, moins de
         * recherche. Lu à l'écran en Corruption. Source : faction_capture_total. */
        float rot = (re->owner>=0)? faction_capture_total(re->owner) : 0.f;
        /* CAPITALE (scps_labor) : sa PRODUCTIVITÉ (+5 %/tier servi) booste la vraie
         * production, au-delà des outils — mais le bonus est rongé par (1−rot). */
        {
            long rpop = (long)rp_;   /* pop TOTALE (labor_avail inclut désormais les bourgeois — ne pas les recompter) */
            int  ctier = capitale_max_tier(rpop);
            long nob   = capitale_admin_pop(ctier); if (nob>rpop) nob=rpop;
            float cap_bonus = (capitale_prodmult(ctier, nob) - 1.f) * (1.f - rot);
            prod_mult *= (1.f + cap_bonus);
        }
        prod_mult *= (re->tech_prod>0.f ? re->tech_prod : 1.f);   /* §B1 : techs de PRODUCTION du pays (outils/capitale + SAVOIR-FAIRE) */
        /* CÔTE BALAFRÉE (course §4) : la production de la province pillée est
         * entaillée ~1 an ; l'immunité au raid décroît en parallèle. */
        if (re->balafre_days>0.f){ re->balafre_days-=dt*365.f; prod_mult*=0.5f; }
        if (re->raid_cd_days>0.f)  re->raid_cd_days-=dt*365.f;
        if (rid<SCPS_MAX_REG && g_friche[rid]) prod_mult*=FRICHE_FACTOR;  /* E1bis.10 : entretien IMPAYÉ → friche */
        /* UTILITÉ DE L'HABITABILITÉ — la terre RUDE produit moins : malus = (1−hab)·HAB_MALUS_K
         * (habitabilité 50 % → −10 % de prod). Lit la COORDONNÉE (re->habitability), n'assigne
         * aucun modificateur plat. La région-SIÈGE (province de départ) en est EXEMPTÉE. */
        if (!re->is_capital)
            prod_mult *= fmaxf(0.f, 1.f - (1.f - re->habitability) * tune_f("HAB_MALUS_K", 0.20f));

        /* ---- 1. EXTRACTION = LABOR-BOUND (ressource PAR OUVRIER, refonte A0) ---
         * out[r] = OUVRIERS[r] × YIELD[r] × geo_eff[r] × effort(prix) × prod_mult.
         *   · geo_eff = qualité de tuile (raw_cap/REF, plafonné) — un multiplicateur, pas
         *     la base ; raw_cap n'est plus le rendement mais la RICHESSE qui module.
         *   · ouvriers : la part EXTRACT_LABOR_SHARE des journaliers, répartie entre les
         *     brutes ∝ geo_eff×prix (la tuile riche/chère attire les bras). Le reste des
         *     journaliers reste pour les manufactures (labor_used le réserve).
         *   · plus de ×2 bois/fer/or (replié dans YIELD).
         * La production est ainsi LINÉAIRE en main-d'œuvre (elle SUIT la pop) ; le commerce
         * comble ce que la géologie locale ne donne pas (vocation : 2 brutes/région). */
        /* ALLOCATION JOUEUR/IA — si alloc_on, on répartit labor_avail (journaliers+bourgeois)
         * par les POIDS du joueur : extraction ET manufacture partagent UN budget (somme des
         * poids = alloc_total). Sinon, alloc_total reste 0 et le split AUTO ci-dessous opère. */
        float alloc_total = 0.f;
        if (re->alloc_on){
            for (int r=1;r<RES_PROD_FIRST;r++) if (re->raw_cap[r]>0.f) alloc_total += (float)re->alloc_raw[r];
            for (int i=0;i<re->n_bld;i++){ int t=re->bld[i].type; if(t>=0&&t<BLD_TYPE_COUNT) alloc_total += (float)re->alloc_bld[t]; }
        }
        float egeo[RES_COUNT], eeff[RES_COUNT], ew[RES_COUNT], ewsum=0.f;
        for (int r=1;r<RES_PROD_FIRST;r++){
            ew[r]=0.f;
            if (re->raw_cap[r]<=0.f) continue;
            egeo[r] = clampf(re->raw_cap[r]/ext_geo_ref, 0.f, ext_geo_cap);          /* qualité ∈ [0..CAP] */
            eeff[r] = market_effort(re->price[r], BASE_PRICE[r]);                     /* l'effort suit le prix */
            ew[r]   = egeo[r]*eeff[r];                                               /* poids d'allocation des bras */
            ewsum  += ew[r];
        }
        float L_ext = labor_avail*ext_lab_share;   /* main-d'œuvre dédiée à l'extraction (mode AUTO) */
        for (int r=1;r<RES_PROD_FIRST;r++){
            if (re->raw_cap[r]<=0.f) continue;
            float workers;
            if (re->alloc_on){                                          /* OVERRIDE : poids joueur, budget commun */
                if (alloc_total<=EPS || re->alloc_raw[r]==0) continue;   /* rien alloué à cette brute */
                workers = labor_avail * (float)re->alloc_raw[r] / alloc_total;
            } else {                                                    /* AUTO : ∝ geo×prix, part EXTRACT_LABOR_SHARE */
                if (ewsum<=EPS || ew[r]<=0.f) continue;
                workers = L_ext*ew[r]/ewsum;                            /* bras affectés à la brute r */
            }
            /* EXPLOITATION (modificateur provincial à construire) : +RAW_BOOST_PER_TIER par palier sur
             * l'extraction de CETTE brute. Multiplie `out` (= bras × rendement) → l'effet SCALE sur les
             * bras (plus d'ouvriers = boost absolu plus grand), même logique qu'une manufacture. */
            int bt = re->raw_boost[r];                                   /* palier d'exploitation (clampé : save forgée) */
            { int maxt=(int)tune_f("RAW_BOOST_MAX_TIER",8.f); if (bt>maxt) bt=maxt; }
            float rboost = 1.f + tune_f("RAW_BOOST_PER_TIER",0.05f)*(float)bt;
            float out = workers*EXTRACT_YIELD[r]*dt*egeo[r]*eeff[r]*prod_mult*rboost;  /* /ouvrier/an × dt × qualité × prix × outils × exploitation */
            labor_used += workers;
            S[r] += out;                                               /* dépôt au STOCK NATIONAL */
            supply[r]    += out;
            float value = out*re->price[r];
            gdp += value;
            wage_pool   += value*WAGE_SHARE;
            profit_pool += value*(1.f-WAGE_SHARE-TAX_RATE);
            tax_pool    += value*TAX_RATE;
        }

        /* ---- 2. MANUFACTURE -------------------------------------------- */
        /* FAU1/RETRAIT — la DÉCRUE PASSIVE de l'entropie CUMULÉE (transmuteurs) : aucun
         * stabilisateur actif ; elle refonte lentement chaque tick (CHARGE_DECAY ≪ accumulation
         * sous spawn soutenu) → dabbler puis cesser = récupérable, spawn continu = fracture.
         * L'IMMÉDIAT (arcane_charge : mage + spawn du tick) est remis à zéro (per-tick, comme avant). */
        re->faust_charge = fminf(1.0e6f, fmaxf(0.f, re->faust_charge - tune_f("CHARGE_DECAY", 0.04f)));   /* P1 : plafond anti-dérive-inf (jamais atteint en jeu : ~unités avant la fin §27 ; borne juste l'accumulateur non clampé) */
        re->arcane_charge=0.f;
        for (int i=0;i<re->n_bld;i++) {
            Building *b=&re->bld[i];
            const Recipe *rc=&RECIPE[b->type];
            /* ALLOCATION — bâtiment FERMÉ par le joueur (poids 0) : aucune sortie, aucun
             * intrant consommé, aucun bras employé. (alloc_on=0 ⇒ jamais fermé.) */
            if (re->alloc_on && re->alloc_bld[b->type]==0){ b->workers=0.f; continue; }
            /* CHOIX D'INTRANT (override) : bld_input==1 FORCE le repli (alt1) comme seul intrant
             * (pas de fallback vers in1). Sinon comportement par défaut (in1 d'abord, alt1 en repli). */
            Resource e_in1=rc->in1; float e_q1=rc->q1; Resource e_alt=rc->alt1; float e_altq=rc->alt1_q;
            if (re->alloc_on && rc->alt1!=RES_NONE && re->bld_input[b->type]==1){
                e_in1=rc->alt1; e_q1=rc->alt1_q; e_alt=RES_NONE;
            }
            /* Production cible = niveau ; bornée par intrants en stock et
             * par la main-d'œuvre restante. */
            /* cap = niveau × effort de marché (SURPLUS NATUREL : on lit le prix sortie). */
            float cap = b->level * market_effort(re->price[rc->out], BASE_PRICE[rc->out]);
            float lim = cap;
            if (e_in1!=RES_NONE){
                float out_in1 = S[e_in1]/fmaxf(e_q1,EPS);   /* sortie possible via l'intrant primaire (pool national) */
                if (e_alt!=RES_NONE) out_in1 += S[e_alt]/fmaxf(e_altq,EPS);  /* + repli (perle…) */
                lim=fminf(lim, out_in1);
            }
            if (rc->in2!=RES_NONE) lim=fminf(lim, S[rc->in2]/fmaxf(rc->q2,EPS));
            /* RÉSERVE VIVRIÈRE : le grain NOURRIT avant de se brasser. On ne brasse
             * que le SURPLUS au-delà du besoin alimentaire — du besoin de TOUT l'empire
             * (le grenier est national) : une province ne brasse pas la faim d'une autre. */
            if (rc->in1==RES_GRAIN || rc->in2==RES_GRAIN){
                float pop = (owner_>=0 && owner_<SCPS_MAX_COUNTRY) ? epop[owner_]
                          : re->strata[CLASS_LABORER].pop + re->strata[CLASS_BOURGEOIS].pop
                          + re->strata[CLASS_ELITE].pop;
                float reserve = pop/100.f * 5.0f * food_need;   /* REFONTE A2 : on protège le besoin VIVRIER (≈4/100hab × calibrage) + marge — on ne brasse QUE le surplus */
                float spare   = fmaxf(0.f, S[RES_GRAIN] - reserve);
                float gq = (rc->in1==RES_GRAIN)?rc->q1:rc->q2;
                lim = fminf(lim, spare/fmaxf(gq,EPS));
            }
            /* §gate — ORFÈVRERIE bornée à la DEMANDE EFFECTIVE. NB : l'orfèvrerie n'est PAS
             * engorgée mais un LUXE de STATUT à variante culturelle — seules les élites
             * MARTIALES (preferred_luxe) la réclament ; les raffinées prennent l'étoffe
             * précieuse (voie murex). Le gate borne la sortie à la demande réelle pour ne
             * pas brûler l'OR rare (désormais nourri, voie or martiale) en biens que nul ne
             * veut : nul chez les raffinées (demande ~0 → sortie 0), servie chez les martiales.
             * lim est en « lots » → on convertit la sortie voulue par qout·prod_mult. (Les
             * outils, eux, sont régulés par leur demande passive ∝ main-d'œuvre.) */
            if (rc->out==RES_PRECIOUS_WARE){
                float gap = re->demand[rc->out]*GATE_DEMAND_BUFFER - S[rc->out];
                lim = fminf(lim, fmaxf(0.f,gap)/fmaxf(rc->qout*prod_mult,EPS));
            }
            /* LIMITEUR JOUEUR (§11.4) : cap/ressource. Au plafond → lim=0 → continue AVANT la conso
               (intrants NON consommés = libérés). −1 = désactivé. Vise le STOCK réel (arms_mult inclus). */
            if (owner_>=0 && owner_<SCPS_MAX_COUNTRY){
                float pc = g_prod_cap[owner_][rc->out];
                if (pc >= 0.f){
                    float per_lim = rc->qout*prod_mult
                                  * ((res_is_arm(rc->out)&&rc->out!=RES_ENCHANTED_ARMS)?tune_f("MANUF_ARMS_MULT",10.f):1.f);
                    lim = fminf(lim, fmaxf(0.f, pc - S[rc->out])/fmaxf(per_lim, EPS));
                }
            }
            if (lim<=0.f){ b->workers=0.f; continue; }
            float want_labor=rc->labor*cap;
            /* AUTO : la manufacture pioche dans la main-d'œuvre RESTANTE (gloutonne, par ordre).
             * OVERRIDE : chaque bâtiment reçoit SON budget = part de poids du bassin total. */
            float avail = re->alloc_on
                ? ((alloc_total>EPS) ? labor_avail*(float)re->alloc_bld[b->type]/alloc_total : 0.f)
                : (labor_avail-labor_used);
            float lratio=(want_labor>0.f)?clampf(avail/want_labor,0.f,1.f):0.f;
            lim=fminf(lim, cap*lratio);
            if (lim<=0.f){ b->workers=0.f; continue; }

            /* Consomme intrants, produit sortie (valeur ajoutée = sortie − intrants).
             * in1 d'abord, puis le repli alt1 à SA quantité (perle = 2× l'or/bijou). */
            float val_in =0.f;
            if (e_in1!=RES_NONE){
                float out1=fminf(lim, S[e_in1]/fmaxf(e_q1,EPS));   /* part faite avec l'intrant primaire (pool) */
                float g1=out1*e_q1;
                S[e_in1]-=g1; demand[e_in1]+=g1; val_in+=g1*re->price[e_in1];
                float rem=lim-out1;
                if (rem>0.f && e_alt!=RES_NONE){
                    float ga=rem*e_altq;
                    S[e_alt]-=ga; demand[e_alt]+=ga; val_in+=ga*re->price[e_alt];
                }
            }
            if (rc->in2!=RES_NONE){ S[rc->in2]-=lim*rc->q2; demand[rc->in2]+=lim*rc->q2; val_in+=lim*rc->q2*re->price[rc->in2]; }
            float out=lim*rc->qout*prod_mult;   /* outils → productivité */
            out *= (1.f - 0.5f*re->revolt_scar); /* la cicatrice de révolte ronge la production */
            /* F-arc ARSENAL — la manufacture d'ARMES verse ×MANUF_ARMS_MULT au STOCK (l'arsenal que
             * la levée POMPE : recrutement = stock/POP_PER_UNIT). Le marché (supply → prix), la valeur
             * ajoutée (PIB plus bas) et la charge faustienne restent sur la sortie de BASE `out` → l'éco
             * & la Brèche INCHANGÉES ; seul l'arsenal de guerre enfle, ce qu'il faut pour les régiments.
             * EXCLU : les armes enchantées — SEUL armement que diplo_mil_power lit (multiplicateur de
             * qualité) ; les gonfler ×10 emballe la course aux armements (guerres). Les autres sont du
             * pur carburant de levée, invisibles à la puissance militaire perçue. */
            float arms_mult = (res_is_arm(rc->out)   && rc->out !=RES_ENCHANTED_ARMS) ? tune_f("MANUF_ARMS_MULT", 10.f) : 1.f;
            S[rc->out]+=out*arms_mult;
            supply[rc->out]+=out;
            /* F3 — SORTIE SECONDAIRE (arme arcane : kit alchimiste, bâton de mage) ∝ production. */
            if (rc->out2!=RES_NONE){ float o2=lim*rc->qout2*prod_mult*(1.f-0.5f*re->revolt_scar);
                float m2 = (res_is_arm(rc->out2) && rc->out2!=RES_ENCHANTED_ARMS) ? tune_f("MANUF_ARMS_MULT", 10.f) : 1.f;
                S[rc->out2]+=o2*m2; supply[rc->out2]+=o2; }
            b->workers=rc->labor*lim;
            labor_used+=b->workers;
            /* FAU0/FAU2 — LA CHARGE FAUSTIENNE (hook UNIQUE faust_charge_add) : le mage (essence)
             * ET les transmuteurs (chaque spawn ∝ output = le VOLUME) nourrissent la Brèche. Les
             * transmuteurs comptent aussi le rare consommé (capteur caché du capstone §27). */
            if (b->type==BLD_MAGE_WORKSHOP) faust_charge_add(re, out);   /* arcane ordinaire : IMMÉDIAT seul (per-tick, comme avant) */
            else if (bld_is_faustian(b->type)){
                float spawn = out * tune_f("FAUST_SPAWN_CHARGE", 0.15f);
                faust_charge_add(re, spawn);     /* l'IMMÉDIAT (flux du tick) */
                re->faust_charge += spawn;       /* FAU1 : et l'entropie CUMULÉE (la pente vers la Brèche) */
                int k=(b->type==BLD_FOREUSE)?0:(b->type==BLD_REPLICATEUR)?1:2;   /* essence · flux · fer céleste */
                re->faust_consumed[k] += lim*rc->q1;
            }
            /* F-arc RETRAIT — LE PUITS-DE-FLUX EST COUPÉ : l'Alambic ne quenche PLUS la charge.
             * AUCUN mécanisme ACTIF ne fait reculer la charge par design (modèle Stellaris) ;
             * seule la décrue PASSIVE (CHARGE_DECAY, FAU0/FAU1) la grignote hors péché. */

            /* Valeur ajoutée = valeur sortie − valeur intrants */
            float val_out=out*re->price[rc->out];
            float va=fmaxf(0.f, val_out-val_in);
            gdp += va;
            wage_pool   += va*WAGE_SHARE;
            profit_pool += va*(1.f-WAGE_SHARE-TAX_RATE);
            tax_pool    += va*TAX_RATE;
        }
        re->gdp=gdp;

        /* ---- 3. REVENUS : salaire / profit / RENTE (l'élite vit de la rente) */
        re->strata[CLASS_LABORER].wealth   += wage_pool;
        re->strata[CLASS_BOURGEOIS].wealth += profit_pool;
        re->strata[CLASS_ELITE].wealth     += tax_pool;   /* rente, PAS l'impôt d'État */

        /* ---- 3b. IMPÔT D'ÉTAT (§6-7) : par classe, taux VISÉ borné par le SEUIL
         * = tolérance(éthos,classe) × (0.4 + 0.6·satisfaction du tick passé).
         * Au-delà : ÉVASION (le net BAISSE) + grogne (la satisfaction chutera).
         * La boucle : un peuple CONTENT sous un éthos TOLÉRANT paie fort ;
         * surtaxer un peuple mécontent ne rapporte pas — contenter d'abord. */
        float coll[CLASS_COUNT]={0}, coll_tot=0.f;     /* §B : ce que CHAQUE classe a versé (pour le rendre) */
        for (int c=0;c<CLASS_COUNT;c++){
            PopStratum *st=&re->strata[c];
            float sat   = clampf(st->satisfaction,0.f,1.f);
            float seuil = econ_tax_tolerance(re->culture.ethos,(SocialClass)c)*(0.40f+0.60f*sat);
            float evasion   = clampf(STATE_TAX_AMBITION - seuil, 0.f, 1.f);
            float collected = STATE_TAX_AMBITION * st->wealth * (1.f-evasion) * dt;
            if (collected>st->wealth) collected=st->wealth;
            st->wealth   -= collected;
            re->treasury += collected;
            coll[c]=collected; coll_tot+=collected;
            over_tax[c]   = (STATE_TAX_AMBITION>seuil)?(STATE_TAX_AMBITION-seuil):0.f;
        }
        re->over_tax = clampf(over_tax[CLASS_LABORER], 0.f, 1.f);   /* grief des laboureurs → révolte */
        if (re->owner>=0) econ_flux_add(re->owner, FX_TAX, coll_tot);   /* I0 : la ligne des taxes */

        /* E1bis.10 — ENTRETIEN : l'infra bâtie se paie chaque tick ; impayé → FRICHE.
         * G0.4 : l'entretien suit l'IPM (un monde cher coûte plus cher à tenir). */
        float ipmf = (e->ipm>0.f)? e->ipm : 1.f;
        float opf  = tune_f("SINK_FLOOR", SINK_FLOOR);    /* I3bis — plancher de SUBSISTANCE (friche) */
        float hof  = tune_f("COURT_FLOOR", COURT_FLOOR);  /* seuil de HOARDING : les ponctions ne mordent qu'au-dessus */
        if (rid<SCPS_MAX_REG){
            /* I3 — DÉFENSIF : la famille Garnison/Forteresse/Citadelle (re->build.H_coerc)
             * s'entretient ×1.5 (remparts à réparer, garnisons à nourrir) ; le reste suit
             * la loi commune. (On lit le delta H agrégé : pas besoin de la liste d'édifices.)
             * I4 : faith + savoir COMPTENT désormais — les monuments (Cathédrale, Académie,
             * Monastère) se paient chaque jour (G0.3.3 les fait enfin monter). */
            float infra = re->build.K_inst + re->build.H_coerc*DEF_UPKEEP_MULT + re->build.P_open
                        + re->build.PE_infra + re->build.food_cap + re->build.port
                        + re->build.faith + re->build.savoir;
            /* ENTRETIEN DE BASE — maintenir l'infra bâtie. PAS d'IPM ici : la subsistance
             * ne paie pas la surtaxe d'un monde cher. L'entretien ne mord QUE le SURPLUS
             * au-dessus de la réserve d'exploitation : un État garde toujours de quoi
             * fonctionner (bâtir un grenier, amorcer une route), il ne se vide pas jusqu'au
             * dernier sou en friche perpétuelle (0.6× prod → moins d'impôt → friche → spirale).
             * La FRICHE ne frappe que le SURBÂTI : quand l'entretien DÉPASSE le surplus
             * disponible, l'État a trop construit pour ses moyens → la prod s'entaille. */
            float base_up = (infra*BUILD_GOLD_PER_DELTA/tune_f("ENTRETIEN_DIV",ENTRETIEN_DIV)) * 365.f * dt;
            /* FRICHE = SURBÂTI CATASTROPHIQUE : l'entretien dépasse TOUT le trésor (pas juste
             * le surplus au-dessus de la réserve) — l'État ne peut littéralement pas tenir son
             * infra. Une région qui repose sur sa réserve d'exploitation (peu d'infra, peu
             * d'impôt) n'est PAS en friche : elle sous-finance sans la falaise de prod. */
            bool fr = (base_up > re->treasury);
            float surplus = re->treasury - opf;
            float paid_up = (surplus > 0.f) ? fminf(base_up, surplus) : 0.f;
            re->treasury -= paid_up;                                       /* payé du surplus, la réserve tient */
            if (re->owner>=0) econ_flux_add(re->owner, FX_UPKEEP, -paid_up);  /* I0 : entretien édifices */
            g_friche[rid] = fr;
            if (fr) g_n_friche++;
            /* SURCOÛTS ANTI-HOARDING (G0.4 surtaxe IPM sur l'entretien + H7 encadrement des
             * manufactures) : du SURPLUS au-dessus du seuil de HOARDING SEULEMENT — jamais une
             * réserve d'exploitation. Un trésor qui GONFLE paie un monde cher et ses
             * manufactures ; une bourse de fonctionnement (le bas de laine qui bâtit) non. */
            if (re->treasury > hof){
                float mlev=0.f; for (int i=0;i<re->n_bld;i++) mlev += re->bld[i].level;
                float surcharge = base_up*(ipmf-1.f)                                  /* la part IPM de l'entretien */
                                + mlev*tune_f("MANUF_UPKEEP_DAY",MANUF_UPKEEP_DAY)*365.f*dt*ipmf;
                if (surcharge>0.f){ float pay=fminf(surcharge, re->treasury - hof); re->treasury -= pay;
                    if (re->owner>=0) econ_flux_add(re->owner, FX_ENCADR, -pay); }  /* I0 : surtaxe IPM + encadrement */
            }
        }
        /* G0.4 — le FASTE de cour : au-delà de 10k, 0.5 %/mois du surplus se dépense
         * (frein au hoarding — un trésor qui gonfle finance le prestige). */
        { float cf=tune_f("COURT_FLOOR",COURT_FLOOR);
          if (re->treasury > cf){ float court=(re->treasury - cf) * tune_f("COURT_RATE",COURT_RATE) * (dt*12.f);
              re->treasury -= court; if (re->owner>=0) econ_flux_add(re->owner, FX_COURT, -court); } }
        /* I3 — ADMIN : la part de cette région dans la bureaucratie du pays. Total pays =
         * base × n^exp ; par région = base × n^(exp−1). Croît avec la TAILLE (×IPM). Du
         * SURPLUS au-dessus du seuil de hoarding : l'admin pèse sur les grands trésors, pas
         * sur le bas de laine qui finance les chantiers (sinon l'État ne bootstrappe jamais). */
        if (rid<SCPS_MAX_REG && re->owner>=0 && re->owner<SCPS_MAX_COUNTRY && re->treasury>hof){
            int nreg=rcount[re->owner]; if (nreg<1) nreg=1;
            float admin = tune_f("ADMIN_BASE",ADMIN_BASE)
                        * powf((float)nreg, tune_f("ADMIN_EXP",ADMIN_EXP)-1.f) * ipmf * (dt*12.f);
            float before=re->treasury; re->treasury = fmaxf(hof, re->treasury - admin);
            econ_flux_add(re->owner, FX_ADMIN, -(before - re->treasury));   /* I0 : la ligne admin */
        }

        /* §B (TRÉSOR MORT) — l'État REDÉPENSE : il ne hoarde plus, il CIRCULE. Une masse
         * salariale réabonde la richesse des classes AU PRORATA de l'impôt qu'elles ont
         * versé (les hautes classes, les plus ponctionnées, récupèrent le plus → pouvoir
         * d'achat restauré → satisfaction, et le drain C1 sur l'élite est réparé). Le
         * solde subventionne l'expansion (§1). Sans cette sortie, le trésor ×16 asséchait
         * les classes à richesse ~0 → 15 % de satisfaction même quand les biens existent. */
        float depense = re->treasury * STATE_SPEND_RATE * dt;
        /* I3bis — la redépense LAISSE la réserve d'exploitation : un État ne se vide pas
         * jusqu'au dernier sou (sinon, à trésor 0, il ne peut plus rien bâtir — pas même
         * un grenier — et s'enferme dans la famine). Il circule le SURPLUS, garde de quoi
         * fonctionner. */
        float spendable = re->treasury - opf;
        if (depense > spendable) depense = spendable;
        if (depense < 0.f) depense = 0.f;
        re->treasury -= depense;
        float payroll = depense * PAYROLL_FRACTION;
        if (coll_tot > 1e-6f)
            for (int c=0;c<CLASS_COUNT;c++)
                re->strata[c].wealth += payroll * (coll[c]/coll_tot);   /* on rend à chacun ∝ sa contribution */
        /* le solde (depense − payroll) a quitté le trésor en DÉPENSE PUBLIQUE (armée,
         * travaux) : il ne s'agit plus de hoarder. L'expansion (§1) est, elle, portée par
         * le signal-prix — le pouvoir d'achat rendu ici en est le carburant indirect. */

        /* §besoins progressifs — combien de besoins (par ordre de priorité) sont ACTIFS
         * dans cette région : f(niveau de capitale, que la POP débloque). Petit centre →
         * 2 besoins (les bases) ; grande capitale → tout le panier (statut compris). Un
         * besoin non encore débloqué ne crée NI demande NI manque de satisfaction. */
        long rpop_nd = (long)(re->strata[CLASS_LABORER].pop + re->strata[CLASS_BOURGEOIS].pop
                            + re->strata[CLASS_ELITE].pop);
        int active_needs = 1 + capitale_max_tier(rpop_nd);   /* tier 1 → 2 besoins, +1 par tier */

        /* ---- 4. DEMANDE de consommation par strate ---------------------
         * §2 (CORRECTIF D'INTÉGRATION) : la demande des paliers VARIANTES suit la
         * PRÉFÉRENCE culturelle, EXACTEMENT comme la satisfaction (étape 5). Le
         * besoin inscrit dans la case canonique (RES_WINE = palier moral,
         * RES_PRECIOUS_WARE = palier statut) est ROUTÉ vers la variante préférée
         * (bière/vin, orfèvrerie/étoffe précieuse). Sans cela, bière & étoffe
         * précieuse restent à demande nulle (prix planché, jamais tirées par le
         * marché) tandis que vin & orfèvrerie portent TOUTE la demande (prix
         * plafond, pénurie structurelle) → l'élite ne reçoit pas son statut → coup. */
        for (int c=0;c<CLASS_COUNT;c++) {
            float units=re->strata[c].pop/100.f*DEMAND_TENSION;   /* /100 hab, tendu +10 % */
            for (int r=0;r<RES_COUNT;r++) {
                float need=NEED[c][r];
                if (need<=0.f) continue;
                if (need_rank(c,(Resource)r) >= active_needs) continue;   /* besoin pas encore débloqué */
                if (res_is_food((Resource)r)) need*=food_need;            /* A2 : calibrage de la bouche */
                Resource tgt=(Resource)r;
                if      (r==RES_WINE)          tgt=preferred_drink(&re->culture);
                else if (r==RES_PRECIOUS_WARE) tgt=preferred_luxe(&re->culture);
                demand[tgt]+=need*units;
            }
        }

        /* ---- 5. MARCHÉ : prix puis allocation au budget ---------------- */
        /* Prix : converge vers base × IPM × demande/(stock+offre). Le facteur IPM
         * (§C) est 1.0 quand SCPS_IPM=0 → multiplieur identité, effet RETIRÉ. */
        float infl = (e->ipm>0.f)? e->ipm : 1.f;   /* garde-fou : jamais 0 (econ non initialisé) */
        if (owner_ < 0){
            /* ISOLÉ (hors empire / fixture banc) : prix soldé LOCALEMENT sur re->stock (pshare=1) — INCHANGÉ. */
            for (int r=0;r<RES_COUNT;r++) {
                if (BASE_PRICE[r]<=0.f) continue;
                float avail=S[r]*pshare+supply[r];
                float target=BASE_PRICE[r]*infl*clampf(demand[r]/(avail+EPS),0.2f,6.f);
                re->price[r]=re->price[r]*PRICE_INERTIA + target*(1.f-PRICE_INERTIA);
                re->price[r]=clampf(re->price[r],BASE_PRICE[r]*0.15f,BASE_PRICE[r]*8.f);
            }
        } else {
            /* EMPIRE : on ACCUMULE l'offre/demande NATIONALES ; le prix est soldé UNE FOIS par pays
             * après la boucle (prix UNIFORME par empire → fin de l'artefact spatial). re->price garde
             * le prix national du tick PRÉCÉDENT (la conso ci-dessous le lit, causalité décalée d'1 tick). */
            for (int r=0;r<RES_COUNT;r++){ supply_nat[owner_][r]+=supply[r]; demand_nat[owner_][r]+=demand[r]; }
        }

        /* Satisfaction par strate : fraction des besoins effectivement
         * achetée, pondérée par la solvabilité (budget vs coût). On sert
         * d'abord les vivres, puis le reste. Le stock disponible plafonne. */
        /* Suivi de la couverture RÉELLE par palier (pas la satisfaction globale) :
         * food_got mesure les VIVRES effectivement servis, soc_got le reste. */
        float r_food_need=0.f, r_food_got=0.f, r_soc_need=0.f, r_soc_got=0.f;
        /* needs_met : fraction du panier COMPLET (toutes les entrées NEED, pas seulement
         * les actives) dont la couverture got≥τ — pop-pondérée sur les classes. Pilote la
         * fertilité (couverture BRUTE : on ne soustrait PAS la surtaxe fiscale). */
        float tau=tune_f("NEEDS_MET_TAU",0.5f), nmsum=0.f, nmpop=0.f;
        for (int c=0;c<CLASS_COUNT;c++) {
            float units=re->strata[c].pop/100.f*DEMAND_TENSION;   /* /100 hab, tendu +10 % */
            if (units<=0.f){ re->strata[c].satisfaction=0.f; continue; }
            float budget=re->strata[c].wealth;
            float need_w=0.f, met_w=0.f;   /* pondération par valeur du besoin */
            float comfort_joy=0.f;         /* BONUS poterie/statuaire CONSOMMÉES (luxe qui ÉLÈVE, hors panier) */
            int   nbasket=0, nsat=0;       /* catégories du panier total · satisfaites (got≥τ) */
            for (int rr=0;rr<RES_COUNT;rr++)
                if (NEED[c][rr]>0.f && rr!=RES_POTTERY && rr!=RES_STATUE) nbasket++;   /* panier COMPLET hors confort-bonus */
            for (int r=0;r<RES_COUNT;r++) {
                float need=NEED[c][r]*units*(res_is_food((Resource)r)?food_need:1.f);   /* A2 : calibrage de la bouche */
                if (need<=0.f) continue;
                if (need_rank(c,(Resource)r) >= active_needs) continue;   /* §progressif : besoin pas encore débloqué → ne pèse pas */
                /* ── CONFORT-BONUS (poterie/statuaire) : un LUXE qui ÉLÈVE le bonheur quand SERVI,
                 * SANS pénaliser quand absent (hors panier) — ⇒ « bonheur up ». La demande, elle, est
                 * générée par la boucle DEMANDE (plus haut) → le marché les produit, consommant
                 * argile/pierre (la demande qui entretient les raw-works). */
                if (r==RES_POTTERY || r==RES_STATUE){
                    float can_stock=clampf(S[r]/(need+EPS),0.f,1.f);
                    float cost=need*can_stock*re->price[r];
                    float can_buy=(cost>0.f)?clampf(budget/cost,0.f,1.f):1.f;
                    float got=can_stock*can_buy;
                    S[r]-=need*got; budget-=need*got*re->price[r];
                    comfort_joy += tune_f("COMFORT_JOY",0.08f) * got;   /* luxe SERVI → bonheur (par bien) */
                    continue;                                            /* HORS panier : aucune pénalité si absent */
                }
                /* ── Palier MORAL (boisson) : VARIANTE culturelle bière/vin ──
                 * On sert la boisson PRÉFÉRÉE de la culture locale d'abord ; la
                 * mauvaise ne comble qu'à moitié (un nain boude le vin). */
                if (r==RES_WINE){
                    float w_d=BASE_PRICE[RES_WINE]*need;   /* valeur du palier (réf. vin) */
                    need_w+=w_d;
                    Resource pref=preferred_drink(&re->culture);
                    Resource alt =(pref==RES_BEER)?RES_WINE:RES_BEER;
                    float cs_p=clampf(S[pref]/(need+EPS),0.f,1.f);
                    float cost_p=need*cs_p*re->price[pref];
                    float cb_p=(cost_p>0.f)?clampf(budget/cost_p,0.f,1.f):1.f;
                    float got_p=cs_p*cb_p;
                    S[pref]-=need*got_p; budget-=need*got_p*re->price[pref];
                    float rem=1.f-got_p;                   /* comblé par la mauvaise boisson */
                    float cs_a=clampf(S[alt]/(need*rem+EPS),0.f,1.f)*rem;
                    float cost_a=need*cs_a*re->price[alt];
                    float cb_a=(cost_a>0.f)?clampf(budget/cost_a,0.f,1.f):1.f;
                    float got_a=cs_a*cb_a;
                    S[alt]-=need*got_a; budget-=need*got_a*re->price[alt];
                    float got=clampf(got_p + DRINK_OFFCULT*got_a, 0.f, 1.f);
                    met_w+=w_d*got; r_soc_need+=need; r_soc_got+=need*got;
                    if (got>=tau) nsat++;
                    continue;
                }
                /* ── Palier STATUT (luxe d'élite) : VARIANTE culturelle ──
                 * orfèvrerie (martial) OU étoffe précieuse (raffiné) ; le mauvais
                 * luxe ne flatte qu'à moitié (l'élite conquise reste sur sa faim). */
                if (r==RES_PRECIOUS_WARE){
                    Resource pref=preferred_luxe(&re->culture);
                    Resource alt =(pref==RES_PRECIOUS_WARE)?RES_PRECIOUS_CLOTH:RES_PRECIOUS_WARE;
                    float w_l=BASE_PRICE[pref]*need; need_w+=w_l;
                    float cs_p=clampf(S[pref]/(need+EPS),0.f,1.f);
                    float cost_p=need*cs_p*re->price[pref];
                    float cb_p=(cost_p>0.f)?clampf(budget/cost_p,0.f,1.f):1.f;
                    float got_p=cs_p*cb_p;
                    S[pref]-=need*got_p; budget-=need*got_p*re->price[pref];
                    float rem=1.f-got_p;
                    float cs_a=clampf(S[alt]/(need*rem+EPS),0.f,1.f)*rem;
                    float cost_a=need*cs_a*re->price[alt];
                    float cb_a=(cost_a>0.f)?clampf(budget/cost_a,0.f,1.f):1.f;
                    float got_a=cs_a*cb_a;
                    S[alt]-=need*got_a; budget-=need*got_a*re->price[alt];
                    float got=clampf(got_p + LUXE_OFFCULT*got_a, 0.f, 1.f);
                    met_w+=w_l*got; r_soc_need+=need; r_soc_got+=need*got;
                    if (got>=tau) nsat++;
                    continue;
                }
                float w=BASE_PRICE[r]*need;          /* importance ~ valeur */
                need_w+=w;
                float can_stock=clampf(S[r]/(need+EPS),0.f,1.f);
                float cost=need*can_stock*re->price[r];
                float can_buy=(cost>0.f)?clampf(budget/cost,0.f,1.f):1.f;
                float got=can_stock*can_buy;
                if (got>=tau) nsat++;
                /* consomme stock & budget (pool national) */
                S[r]-=need*got;
                budget-=need*got*re->price[r];
                met_w+=w*got;
                /* couverture par palier : les vivres VS le reste */
                if (r==RES_GRAIN||r==RES_FISH||r==RES_LIVESTOCK){ r_food_need+=need; r_food_got+=need*got; }
                else                                            { r_soc_need +=need; r_soc_got +=need*got; }
            }
            re->strata[c].wealth=fmaxf(0.f,budget);
            float basket=(need_w>0.f)?met_w/need_w:0.5f;
            /* la surtaxe (§6) gronde : elle ABAISSE la satisfaction → agitation */
            /* CICATRICE D'ANNEXION (étage 3d) : la plaie douce frappe la STABILITÉ — elle ABAISSE
             * la satisfaction (donc l'agitation monte) sans toucher la croissance (≠ revolt_scar). */
            re->strata[c].satisfaction=clampf(basket + comfort_joy - over_tax[c]*K_TAX_AGIT
                                              - re->annex_scar*tune_f("ANNEX_SAT_W",0.5f), 0.f, 1.f);
            if (rid<SCPS_MAX_REG) g_basket_pc[rid][c]=(units>0.f)?need_w/units:0.f;  /* E0.7 : panier/tête */
            float nm_c=(nbasket>0)?(float)nsat/(float)nbasket:0.f;   /* part BRUTE du panier couverte */
            nmsum += nm_c*re->strata[c].pop; nmpop += re->strata[c].pop;
        }
        re->needs_met = (nmpop>0.f)? clampf(nmsum/nmpop,0.f,1.f) : 0.f;   /* pilote la fertilité (avant la croissance) */
        if (rid<SCPS_MAX_REG) mobility_tick_region(re, rid);   /* E0.7 — le dégel des classes */

        /* FRUIT — NOURRITURE DE SUBSTITUTION : comble le déficit vivrier RÉSIDUEL (grain/poisson
         * non couverts) avec le fruit du pool national, là où il pousse (forêt/tempéré). Crée la
         * DEMANDE de fruit (fin du pile-up) ET relève food_sat — un FILET vivrier géo-sensible, qui
         * n'ajoute AUCUN besoin (il ne fait que substituer le manque). 1 fruit = 1 unité de manque. */
        {
            float food_gap = r_food_need - r_food_got;
            if (food_gap > EPS && S[RES_FRUIT] > EPS){
                float fill = fminf(food_gap, S[RES_FRUIT]);
                S[RES_FRUIT]      -= fill;
                demand[RES_FRUIT] += fill;
                r_food_got        += fill;
            }
        }

        /* ---- 6. MISE À JOUR : démographie, tech, satisfaction générale - */
        /* food_sat = la couverture VIVRIÈRE RÉELLE (et non la satisfaction
         * globale) → plus de nourriture = plus de croissance, fin de la famine. */
        {
            re->food_sat   = (r_food_need>0.f)?clampf(r_food_got/r_food_need,0.f,1.f):0.5f;
            re->society_sat= (r_soc_need >0.f)?clampf(r_soc_got /r_soc_need ,0.f,1.f):0.5f;
            /* §4 — pénalité OFF-CULTURE : une minorité d'une autre sphère est MAL
             * servie par les biens (confort/moral/luxe) de la culture dominante.
             * Frappe la satisfaction SOCIALE — PAS les vivres (food_sat épargné,
             * universel) → la survie/croissance ne sont pas punies par la
             * diversité ; l'assimilation efface la pénalité. */
            re->society_sat *= (1.f - 0.60f*econ_off_culture_fraction(&re->pop));
        }

        /* FERTILITÉ = f(BESOINS SATISFAITS). Doublement ~40 ans au PLANCHER (besoins
         * vides), ~20 ans au PANIER PLEIN : net = r_base·(1+demo)·(1+B), avec
         * B∈[0,1] = part du panier couverte (needs_met, 0.85) + prospérité normalisée (0.15).
         * food_s ne MULTIPLIE plus la base — il ne sert qu'au pic de famine (<0.35) ; soc_s
         * n'est plus lu. Plafond souple (cap_factor, plus bas) inchangé. */
        float food_s = re->food_sat;
        /* Démographie modulée par les TRADITIONS de l'empire (Prolifique → + de naissances ;
         * Lent à croître → moins) — INDÉPENDANT de l'héritage (qui ne fait que les noms).
         * IA : tirées au hasard par empire ; joueur : SA composition (culture_build_for). */
        SpeciesBuild sb_demo = culture_build_for((uint32_t)(re->owner<0?0:re->owner));
        float demo = build_leviers(&sb_demo).demographie;
        /* MODIFICATEURS PROVINCIAUX diégétiques → entrée DÉMO (pas un bonus plat) : la
         * TERRE D'ABONDANCE repeuple les régions sous-remplies & nourries (le rebond des
         * low seeds). Auto-ciblé → les régions pleines (seeds riches) reçoivent 0. */
        { ProvModHit pm[PMOD_COUNT]; int npm=provmod_collect(re, pm, PMOD_COUNT);
          for (int i=0;i<npm;i++) demo += pm[i].demo_bonus; }
        /* RELIGION (P4) : le canal RC_POPGROWTH (Fécondité/Offrande) nudge la natalité
         * via la MÊME entrée DÉMO. GATED → aucun effet sans religion (golden intact). */
        if (re->owner>=0 && religion_of_country(re->owner) >= 0)
            demo += religion_country_acc(re->owner)->ch[RC_POPGROWTH];
        float r_base  = tune_f("POP_R_BASE", 0.01733f);   /* ln2/40 = ×2/40ans plancher (vitalité) */
        float prosp_n = clampf((re->prosperity - tune_f("POP_PROSP_MID",0.2f))
                              / tune_f("POP_PROSP_SPAN",1.8f), 0.f, 1.f);   /* PIB/tête → [0,1] (bande haute ≈2.0) */
        float bonus   = tune_f("POP_PROSP_W",0.15f)*prosp_n
                      + tune_f("POP_NEEDS_W",0.85f)*re->needs_met;          /* B ∈ [0,1] */
        /* COUPLAGE SATISFACTION ASYMÉTRIQUE : une province CONTENTE croît plus vite (part au-dessus
         * de 0.5 SEULEMENT) ; la satisfaction basse ne soustrait RIEN (un peuple nourri mais grognon
         * croît quand même → pas de creusement du creux des low seeds, et la reprise est PRIMÉE). */
        bonus += tune_f("POP_SAT_W",0.20f) * fmaxf(0.f, re->satisfaction - 0.5f);
        float net_growth = r_base*(1.f+demo)*(1.f+bonus);                   /* ×2 plancher → ×4 au plein */
        if (food_s < 0.35f)
            net_growth -= (0.35f - food_s) * 0.12f;   /* pic de mortalité famine */
        net_growth = clampf(net_growth, -0.10f, 0.06f);
        /* CICATRICE DE RÉVOLTE : une province récemment soulevée se développe mal —
         * −50 % de croissance tant que la plaie n'est pas refermée (fade ~4 ans). */
        /* RECONSTRUCTION (lot 2) : une cicatrice PROFONDE amorce la renaissance — chargée
         * pendant la crise, libérée à mesure que la plaie se referme (recon·(1−scar)). */
        if (re->revolt_scar > 0.5f) re->reconstruction = 1.f;
        re->revolt_scar    = fmaxf(0.f, re->revolt_scar    - 0.25f*dt);
        re->ferveur        = fmaxf(0.f, re->ferveur        - tune_f("PROVMOD_FERVEUR_DECAY",0.067f)*dt);
        re->reconstruction = fmaxf(0.f, re->reconstruction - tune_f("PROVMOD_RECON_DECAY",  0.10f )*dt);
        re->annex_scar     = fmaxf(0.f, re->annex_scar     - tune_f("ANNEX_SCAR_DECAY",    0.20f )*dt);  /* étage 3d : ~5 ans */
        /* (K4b : pillage_cd décrémenté plus haut, pour TOUTE région — pas seulement colonisée.) */
        net_growth *= (1.f - 0.5f*re->revolt_scar);
        /* UTILITÉ DE L'HABITABILITÉ — la terre RUDE peuple moins vite : même malus que la prod,
         * (1−hab)·HAB_MALUS_K, EXEMPTANT la région-siège (province de départ). */
        if (!re->is_capital)
            net_growth *= fmaxf(0.f, 1.f - (1.f - re->habitability) * tune_f("HAB_MALUS_K", 0.20f));
        net_growth *= dt;   /* cumulatif → suit le pas (mensuel : 1/12 d'an) */

        float total_pop_now=0.f;
        for (int c=0;c<CLASS_COUNT;c++) total_pop_now+=re->strata[c].pop;
        /* Q6 — la CAPACITÉ VIENT DU DÉVELOPPEMENT. Plancher = ½·cap_pop (la terre nue) ;
         * les LOGEMENTS bâtis la doublent vers son plein (cap_pop = la taille nourrie) :
         *   · MANUFACTURES uniquement (pas académie/marché…) : +HOUSE_MANUF par niveau,
         *     plafonné à ½·cap_pop (≈ 25 ateliers·100 quand cap_pop≈5000) ;
         *   · le GRENIER garde son rôle NOURRITURE (food_cap·250), pas logement.
         * Bâtir = la seule façon de remplir la moitié haute → la pop SUIT le bâti. */
        /* eff_cap UNIFIÉ via le helper (Q6 + BONUS CONFORT poterie/statuaire = −15 % besoin logement). */
        float eff_cap = econ_region_effcap(re);
        float cap_factor = fmaxf(0.f, 1.f - total_pop_now/(eff_cap*1.1f));
        net_growth *= cap_factor;

        float satsum=0.f, popsum=0.f;
        for (int c=0;c<CLASS_COUNT;c++) {
            PopStratum *st=&re->strata[c];
            st->pop *= 1.f + net_growth;
            if (st->pop<1.f) st->pop=1.f;
            satsum+=st->satisfaction*st->pop; popsum+=st->pop;
        }
        re->satisfaction=(popsum>0.f)?satsum/popsum:0.f;
        /* l'insatisfaction off-culture pèse sur la satisfaction GÉNÉRALE (donc
         * prospérité/légitimité/impôt) — mais food_sat reste intact (la survie). */
        re->satisfaction *= (1.f - 0.45f*econ_off_culture_fraction(&re->pop));
        /* RELIGION (P8) : une région de foi MINORITAIRE (≠ celle de son pays, après
         * fracture/schisme) gronde — la D-interne religieuse ABAISSE la satisfaction →
         * alimente l'agitation/sécession (système existant). GATED → aucun effet sans
         * religion (chronique : religion_of_region ≡ -1 ⇒ golden intact). */
        if (re->owner>=0){
            int rrg=religion_of_region(rid);
            if (rrg>=0 && rrg!=religion_of_country(re->owner) && !religion_region_stabilized(rid))
                re->satisfaction = fmaxf(0.f, re->satisfaction - tune_f("RELIG_MINORITY_SAT",0.15f));
        }
        re->prosperity = re->gdp/(popsum+1.f);

        /* Tech : les élites convertissent richesse × satisfaction en savoir. La
         * bibliothèque/le monastère BÂTI (densité de savoir) accélère la cadence. */
        PopStratum *el=&re->strata[CLASS_ELITE];
        float savoir_mult = 1.f + 0.25f*re->build.savoir;   /* +25 % de recherche / point bâti */
        re->tech += el->wealth*TECH_RATE*el->satisfaction*savoir_mult*council_m(re->owner,0)*(1.f-rot)*dt;  /* §C3 ; Q1 siège Savoir */

        /* §1 (CISEAU) — l'offre SUIT le signal-prix : chaque manufacture s'étend ∝ la
         * PÉNURIE de son bien (prix AU-DESSUS de la base). À l'équilibre (prix=base) elle
         * stagne ; en pénurie criante (prix→plafond) elle croît vite → l'offre rattrape la
         * demande qui gonfle avec la population, au lieu de ramper à plat (+0.05 plafonné).
         * Rétroaction NÉGATIVE intrinsèque : plus de capacité → prix qui retombe → expansion
         * qui ralentit → pas de snowball (et la surextension→sécession reste l'anti-runaway
         * territorial, intacte). L'État y verse sa subvention (§2) via le pouvoir d'achat. */
        for (int i=0;i<re->n_bld;i++){
            const Recipe *rc=&RECIPE[re->bld[i].type];
            Resource out = rc->out;
            if (out<=RES_NONE || out>=RES_COUNT || BASE_PRICE[out]<=0.f) continue;
            /* §1b — n'étend QUE le maillon RÉELLEMENT saturé : on ne lit pas le prix de
             * l'intrant (un goulet de QUANTITÉ ne fait pas monter son prix — le marché du
             * métal solde au peu qu'on extrait), mais le TAUX D'UTILISATION effectif. Une
             * manufacture qui tourne loin sous sa capacité est bornée en amont (intrant) ou
             * en bras : l'étendre serait vain (cas outillage métal-borné → level runaway). */
            float meff = market_effort(re->price[out], BASE_PRICE[out]);
            float cap  = re->bld[i].level * meff;
            float lim_est = (rc->labor>0.f)? re->bld[i].workers/rc->labor : cap;
            if (cap > EPS && lim_est < 0.70f*cap) continue;   /* sous-utilisé → goulet en amont, pas ici */
            float pression = clampf(re->price[out]/BASE_PRICE[out] - 1.f, 0.f, EXPANSION_PRESSION_CAP);
            re->bld[i].level += BASE_EXPANSION * pression * council_m(re->owner,2) * dt;   /* ∝ pénurie ; Q1 siège Industrie */
        }

        for (int r=0;r<RES_COUNT;r++){ re->supply[r]=supply[r]; re->demand[r]=demand[r]; }

        /* E2 §11 — LE PLAFOND DE STOCK (Σ des caps régionaux) et la décrue des
         * périssables s'appliquent au POOL NATIONAL une fois/tick, en clôture (ci-dessous,
         * hors de cette boucle) : un ×0.85 ou un plafond par-région sur un stock partagé
         * le décaierait/raboterait N fois. */

        /* Diaspora : bonus tech par innovation culturelle accumulée.
         * S'absorbe progressivement (acculturation, demi-vie ~50 ticks). */
        if (re->diaspora_pop > 0.f) {
            re->tech += re->diaspora_pop * DIASPORA_TECH_RATE * re->diaspora_innovation;
            re->diaspora_pop       *= DIASPORA_DECAY;
            re->diaspora_innovation*= DIASPORA_DECAY;
            if (re->diaspora_pop < 0.5f) {
                re->diaspora_pop=0.f; re->diaspora_innovation=0.f;
            }
        }

        /* Coercition : décroît exponentiellement (demi-vie ≈ 10 ticks).
         * La relocalisation forcée et certains événements la relèvent. */
        re->coercion *= COERCION_DECAY;
        if (re->coercion < 0.005f) re->coercion=0.f;

    }

    /* PRIX NATIONAL — soldé UNE FOIS par empire sur demande/(pool+offre) NATIONALES (mêmes
     * paliers ⇒ ratio invariant à l'échelle : ni artefact spatial, ni effondrement d'effort),
     * puis PROJETÉ sur re->price de toutes ses régions (matérialisation). Inertie amorcée du prix
     * national du tick précédent (re->price, uniforme par empire). Itération par index = stable
     * (déterminisme). Les régions ISOLÉES (owner<0) ont déjà soldé leur prix localement, plus haut. */
    {
        float infl = (e->ipm>0.f)? e->ipm : 1.f;
        static float pn[SCPS_MAX_COUNTRY][RES_COUNT];   /* prix national soldé (hors pile) */
        bool done[SCPS_MAX_COUNTRY]; for (int c=0;c<SCPS_MAX_COUNTRY;c++) done[c]=false;
        for (int rid=0; rid<e->n_regions && rid<SCPS_MAX_REG; rid++){
            RegionEconomy *re=&e->region[rid];
            if (!re->active || !re->colonized) continue;
            int c=re->owner; if (c<0||c>=SCPS_MAX_COUNTRY) continue;
            if (!done[c]){
                done[c]=true;
                for (int r=0;r<RES_COUNT;r++){
                    if (BASE_PRICE[r]<=0.f){ pn[c][r]=re->price[r]; continue; }
                    float avail  = pool[c][r] + supply_nat[c][r];                  /* offre NATIONALE (stock + production) */
                    float target = BASE_PRICE[r]*infl*clampf(demand_nat[c][r]/(avail+EPS),0.2f,6.f);
                    float p      = re->price[r]*PRICE_INERTIA + target*(1.f-PRICE_INERTIA);  /* amorce = prix national t-1 */
                    pn[c][r]     = clampf(p, BASE_PRICE[r]*0.15f, BASE_PRICE[r]*8.f);
                }
            }
            for (int r=0;r<RES_COUNT;r++) re->price[r]=pn[c][r];   /* PROJECTION : toutes les régions de l'empire = même prix */
        }
    }

    /* STOCK NATIONAL — CLÔTURE : plafond du pool (Σ des caps régionaux : Entrepôts
     * bâtis), puis décrue des périssables (×0.85, le surplus s'évapore — fin de
     * l'accumulation infinie), UNE fois par empire. */
    for (int c=0;c<SCPS_MAX_COUNTRY;c++){
        if (epop[c]<=0.f) continue;
        for (int g=1;g<RES_COUNT;g++){
            if (pool[c][g]>ecap[c]) pool[c][g]=ecap[c];
            pool[c][g]*=0.85f;
        }
    }
    /* REDISTRIBUTION du pool aux régions au PRORATA de leur population (post-croissance,
     * pour Σ re->stock = pool exactement) → intertrade/Centres, viewer, butin de guerre
     * et save voient un stock régional cohérent, sans toucher leurs 280 sites. */
    {
        float epop2[SCPS_MAX_COUNTRY]={0};
        for (int rid=0; rid<e->n_regions && rid<SCPS_MAX_REG; rid++){
            RegionEconomy *re=&e->region[rid];
            if (!re->active || !re->colonized) continue;
            int o=re->owner; if (o<0||o>=SCPS_MAX_COUNTRY) continue;
            epop2[o]+=re->strata[CLASS_LABORER].pop+re->strata[CLASS_BOURGEOIS].pop+re->strata[CLASS_ELITE].pop;
        }
        for (int rid=0; rid<e->n_regions && rid<SCPS_MAX_REG; rid++){
            RegionEconomy *re=&e->region[rid];
            if (!re->active || !re->colonized) continue;
            int o=re->owner; if (o<0||o>=SCPS_MAX_COUNTRY) continue;
            if (epop2[o]<=EPS) continue;   /* empire sans population : on ne redistribue pas (laisse le stock en l'état) */
            float rp=re->strata[CLASS_LABORER].pop+re->strata[CLASS_BOURGEOIS].pop+re->strata[CLASS_ELITE].pop;
            float share=rp/epop2[o];
            for (int g=0;g<RES_COUNT;g++) re->stock[g]=pool[o][g]*share;
        }
    }
    econ_build_tick(e);   /* §NF v2 — la construction suit le MARCHÉ (demande + bras), plus le gisement */
}

/* ====================================================================== */
/* PRÉVISION ÉCONOMIQUE (pipeline IA, étage 1)                             */
/* ====================================================================== */
/* Conso ANNUELLE par tête d'un bien (toutes classes pondérées). Lue de NEED. */
float econ_conso_per_capita_year(Resource g){
    if (g<=RES_NONE || g>=RES_COUNT) return 0.f;
    float per100=0.f;
    for (int c=0;c<CLASS_COUNT;c++) per100 += NEED[c][g]*CLASS_SHARE[c];
    float fn = res_is_food(g) ? tune_f("FOOD_NEED",1.f) : 1.f;
    return per100/100.f * 12.f * DEMAND_TENSION * fn;   /* /100hab/tick → /hab/an, demande tendue, calibrage food */
}

/* Forecast d'un pays : runway/shortfall/déficit-structurel par flux, depuis les SEULES
 * coordonnées du moteur (pop, raw_cap, demande, offre, stock, eff_cap, needs_met). Aucune
 * hiérarchie de criticité codée — la criticité ÉMERGE du prix × runway × manque (étage 2). */
void econ_country_forecast(const WorldEconomy *e, int cid, float horizon, EconForecast *out){
    if (!out) return;
    memset(out, 0, sizeof *out);
    for (int g=0; g<RES_COUNT; g++) out->runway[g]=1.0e9f;
    out->food_runway=1.0e9f;
    if (!e || cid<0 || cid>=SCPS_MAX_COUNTRY) return;
    const float geo_ref   = tune_f("EXTRACT_GEO_REF",     EXTRACT_GEO_REF);
    const float geo_cap   = tune_f("EXTRACT_GEO_CAP",     EXTRACT_GEO_CAP);
    const float lab_share = tune_f("EXTRACT_LABOR_SHARE", EXTRACT_LABOR_SHARE);
    double P0=0, effcap=0, nm_w=0;
    double sup[RES_COUNT], dem[RES_COUNT], stk[RES_COUNT], pot[RES_COUNT];
    for (int g=0; g<RES_COUNT; g++){ sup[g]=dem[g]=stk[g]=pot[g]=0.0; }
    for (int r=0; r<e->n_regions; r++){
        const RegionEconomy *re=&e->region[r];
        if (re->owner!=cid || !re->active || !re->colonized) continue;
        double p=re->strata[0].pop+re->strata[1].pop+re->strata[2].pop;
        double ec=econ_region_effcap(re);
        P0+=p; effcap+=ec; nm_w+=re->needs_met*p;
        for (int g=1; g<RES_COUNT; g++){
            sup[g]+=re->supply[g]*12.0;   /* tick mensuel → annuel */
            dem[g]+=re->demand[g]*12.0;
            stk[g]+=re->stock[g];
        }
        /* POTENTIEL (pour le déficit STRUCTUREL) : la prod MAX si la région mettait son plein
         * labor (au plein eff_cap) sur la brute — borne OPTIMISTE du « jamais assez ». INCLUT le
         * boost d'EXPLOITATION (raw_boost) : sinon le forecast ne « voit » jamais l'effet des paliers
         * déjà posés → la pénurie paraît éternelle et l'IA sur-bâtit. Le runway RÉCUPÈRE désormais
         * à mesure qu'on boost → la décision se RÉGULE d'elle-même. */
        for (int g=1; g<RES_PROD_FIRST; g++){
            if (re->raw_cap[g]<=0.f) continue;
            double geo=re->raw_cap[g]/geo_ref; if (geo>geo_cap) geo=geo_cap;
            double boost = 1.0 + (double)tune_f("RAW_BOOST_PER_TIER",0.05f)*(double)re->raw_boost[g];
            pot[g]+= ec*0.8*lab_share * EXTRACT_YIELD[g] * geo * boost;
        }
    }
    if (P0<1.0) return;
    out->pop=(float)P0; out->eff_cap=(float)effcap;
    float nm=(float)(nm_w/P0);
    float r=tune_f("POP_R_BASE",0.01733f)*(1.f+0.85f*nm);   /* taux annualisé approx (la fertilité moteur) */
    if (effcap <= P0*1.05) r*=0.2f;                          /* proche du plafond → la croissance s'éteint */
    out->growth_r=r;
    double lnr=log(1.0+(r>1e-4f?r:1e-4f));
    double grow=pow(1.0+r, horizon);
    for (int g=1; g<RES_COUNT; g++){
        double d0=dem[g], s0=sup[g], k=stk[g];
        if (d0<=1e-3 && s0<=1e-3 && k<=1e-3) continue;       /* flux inerte */
        /* CAPACITÉ de production : pour une brute = le POTENTIEL (au plein labor) ; pour un
         * manufacturé = l'offre courante (capacité = ateliers, non projetée ici). L'offre SUIT
         * la pop (plus de bras) jusqu'à ce plafond, la demande AUSSI → un flux en équilibre RESTE
         * en équilibre. Le mur n'est que STRUCTUREL : quand la demande dépasse la CAPACITÉ. */
        double cap_g = (g<RES_PROD_FIRST) ? pot[g] : s0;
        double dh = d0*grow;
        out->shortfall_proj[g]=(float)(dh - (cap_g<dh?cap_g:dh));   /* ce que la capacité NE couvre pas à l'horizon */
        if (cap_g >= d0){
            double hr=(d0>1e-3)? cap_g/d0 : 1e9;             /* la demande croît jusqu'à la capacité */
            double rw=(hr>1.0)? log(hr)/lnr : 0.0;
            rw += (d0>1e-3)? k/d0 : 0.0;                     /* + le coussin de stock (en années) */
            out->runway[g]=(float)rw;
        } else {                                             /* déjà au-delà de la capacité : le stock draine */
            double drain=d0-s0;
            out->runway[g]=(drain>1e-3)? (float)(k/drain) : 0.f;
        }
        if (out->runway[g]<0.f) out->runway[g]=0.f;
        if (out->runway[g]>1.0e9f) out->runway[g]=1.0e9f;
        if (g<RES_PROD_FIRST){                               /* STRUCTUREL : potentiel < conso au plein eff_cap */
            double need_full=(double)econ_conso_per_capita_year((Resource)g)*effcap;
            out->struct_deficit[g]=(pot[g] < need_full*0.95) ? 1 : 0;
        }
    }
    /* CONSTRUCTION — argile/pierre/bois : leur demande est LATENTE (les chantiers qui les veulent
     * sont EUX-MÊMES gatés faute de matière → le flux paraît « inerte », jamais flaggé par la boucle
     * ci-dessus, et le brut de bâti n'est pas une conso PAR TÊTE). On les traite À PART : un royaume
     * dont la CAPACITÉ de brut de bâti (potentiel géo + offre manufacturée + un peu de stock) est sous
     * le seuil RAW_WORKS_NEED est en déficit STRUCTUREL — c'est LE signal qui arme les RAW-WORKS (four
     * à brique/carrière/scierie) dans le pipeline IA. Réutilise le MÊME seuil que le §NF (cohérent). */
    {
        const int trio[3]={RES_CLAY,RES_STONE,RES_WOOD};
        float need=tune_f("RAW_WORKS_NEED",60.f);
        for (int i=0;i<3;i++){ int g=trio[i];
            /* SIGNAL = matière BÂTISSABLE réelle (stock + surplus net), PAS la capacité géologique :
             * un empire riche en raw_cap BOIS mais dont le feu BRÛLE tout (offre < demande) a un stock
             * ~0 → il est en déficit de bois DE BÂTI, même « capacité » haute. C'est ce déficit-là qui
             * arme la scierie ; raw_cap induisait en erreur (le bois de vocation file au feu). */
            double buildable = stk[g] + (sup[g] > dem[g] ? sup[g]-dem[g] : 0.0);
            if (buildable < need){
                out->struct_deficit[g]=1;
                float sf=(float)(need-buildable);
                if (sf > out->shortfall_proj[g]) out->shortfall_proj[g]=sf;
            }
        }
    }
    /* FOOD agrégé (grain+poisson+viande+FRUIT interchangeables) — l'existentiel. */
    {
        double fd=dem[RES_GRAIN]+dem[RES_FISH]+dem[RES_LIVESTOCK]+dem[RES_FRUIT];
        double fpot=pot[RES_GRAIN]+pot[RES_FISH]+pot[RES_LIVESTOCK]+pot[RES_FRUIT];
        double fs=sup[RES_GRAIN]+sup[RES_FISH]+sup[RES_LIVESTOCK]+sup[RES_FRUIT];
        double fk=stk[RES_GRAIN]+stk[RES_FISH]+stk[RES_LIVESTOCK]+stk[RES_FRUIT];
        if (fd>1e-3){
            if (fpot>=fd){ double hr=fpot/fd; double rw=(hr>1.0)?log(hr)/lnr:0.0; rw+=fk/fd; out->food_runway=(float)rw; }
            else { double drain=fd-fs; out->food_runway=(drain>1e-3)?(float)(fk/drain):0.f; }
        }
    }
}

/* ====================================================================== */
/* COLONISATION                                                            */
/* ====================================================================== */
/* Joueur/Antagoniste : essaiment vers toute région vierge adjacente.
 * Cité-État        : essaime uniquement vers ses propres régions vacantes.
 * Dans les deux cas : au plus une fondation par polité par tick.           */

static void colonize_from(WorldEconomy *e, int src_rid, int dst_rid, int cid);
void econ_colonize_from(WorldEconomy *e, int src_rid, int dst_rid, int cid){
    if (src_rid<0||src_rid>=e->n_regions||dst_rid<0||dst_rid>=e->n_regions) return;
    colonize_from(e,src_rid,dst_rid,cid);
}
static void colonize_from(WorldEconomy *e, int src_rid, int dst_rid, int cid) {
    RegionEconomy *src=&e->region[src_rid];
    RegionEconomy *dst=&e->region[dst_rid];
    float spop=0.f; for(int c=0;c<CLASS_COUNT;c++) spop+=src->strata[c].pop;
    float take=fminf(COLONY_COST_POP, spop*0.25f);
    for (int c=0;c<CLASS_COUNT;c++)
        src->strata[c].pop -= take*(src->strata[c].pop/fmaxf(spop,EPS));
    /* DISPATCH conservatif (terre) : les colons détachés ARRIVENT — on essaime
     * tout ce qu'on a prélevé (plancher = graine minimale), pas de saignée du
     * convoi → la colonisation REDISTRIBUE la pop sans la détruire (la mer, elle,
     * garde son surcoût ×2 via econ_colonize_overseas, prélevé en amont). */
    econ_seed_population(dst, fmaxf(take, COLONY_SEED_POP));
    dst->colonized=true;
    dst->culture.settled=true;   /* la culture de biome (gen_population) s'active */
    dst->owner=(int16_t)cid;
    dst->ferveur=1.f;            /* FERVEUR FONDATRICE (lot 2) : la jeune colonie a faim d'avenir */
}

/* L5 — COLONIE OUTRE-MER : mêmes PORTES que l'essaimage terrestre (pop, vivres,
 * cible vierge), mais le COÛT EN POP est ×2 — le convoi maritime saigne la
 * mère-patrie deux fois plus. L'appelant (harnais) a vérifié Port + coque +
 * portée de courants ; ici vivent les portes et le prix. */
bool econ_colonize_overseas(WorldEconomy *e, int src_rid, int dst_rid, int cid){
    if (src_rid<0||src_rid>=e->n_regions||dst_rid<0||dst_rid>=e->n_regions) return false;
    RegionEconomy *src=&e->region[src_rid], *dst=&e->region[dst_rid];
    if (!dst->active || dst->colonized) return false;
    if (!src->colonized || src->owner!=cid) return false;
    float spop=0.f; for (int c=0;c<CLASS_COUNT;c++) spop+=src->strata[c].pop;
    if (spop<COLONY_MIN_POP*2.f || src->food_sat<COLONY_FOOD_GATE) return false;  /* ×2 : il faut le double */
    float extra=fminf(COLONY_COST_POP, spop*0.25f);     /* la 2e ponction (coût ×2) */
    for (int c=0;c<CLASS_COUNT;c++)
        src->strata[c].pop -= extra*(src->strata[c].pop/fmaxf(spop,EPS));
    colonize_from(e, src_rid, dst_rid, cid);            /* la 1re ponction + la fondation */
    return true;
}

int econ_colonize_tick(WorldEconomy *e, const World *w, int skip_cid) {
    int founded=0;

    for (int cid=0; cid<w->n_countries; cid++) {
        if (cid==skip_cid) continue;          /* P2.14 : le JOUEUR colonise à la MAIN (action explicite) */
        const Country *ct=&w->country[cid];
        PolityRole role=ct->role;

        if (role==POLITY_PLAYER || role==POLITY_ANTAGONIST) {
            /* COLONISATION NEEDS-AWARE (pipeline IA, étage 3a) : la cible vaut ce qu'elle COMBLE.
             * score(dst) = Σ_g max(0,shortfall_PROJETÉ[g]) × dst->raw_cap[g] × prix[g] (la valeur
             * ÉMERGE — aucune hiérarchie codée). Le gate vivrier (food_sat) et COLONY_MIN_POP sont
             * LEVÉS vers une tuile-DÉFICIT (runway court / déficit structurel) → expédition de survie :
             * « je colonise le grenier vide même affamé, il va me nourrir ». Anti-spirale : food
             * critique + aucune source au gate normal → on FORCE une colonie de survie vers la food. */
            float proj_h = tune_f("AI_PROJ_HORIZON",25.f);
            EconForecast fc; econ_country_forecast(e, cid, proj_h, &fc);
            float safety = tune_f("AI_SAFETY_HORIZON",12.f);
            float geo_ref = tune_f("EXTRACT_GEO_REF",EXTRACT_GEO_REF);
            float geo_cap = tune_f("EXTRACT_GEO_CAP",EXTRACT_GEO_CAP);
            float needs_w = tune_f("AI_COLONY_NEEDS_W",1.5f);
            bool  food_crit = fc.food_runway < safety;
            float survive_min = tune_f("COLONY_SURVIVE_SEED",0.5f)*COLONY_MIN_POP;
            int best_src=-1, best_dst=-1; float best_score=-1.f;   /* colonisation au gate NORMAL */
            int surv_src=-1, surv_dst=-1; float surv_score=-1.f;   /* expédition de SURVIE (gate levé) */
            for (int rs=0; rs<e->n_regions; rs++) {
                RegionEconomy *src=&e->region[rs];
                if (!src->colonized || src->owner!=cid) continue;
                float spop=0.f; for(int c=0;c<CLASS_COUNT;c++) spop+=src->strata[c].pop;
                bool normal_ok  = (spop>=COLONY_MIN_POP && src->food_sat>=COLONY_FOOD_GATE);
                bool survive_ok = (spop>=survive_min);
                if (!normal_ok && !survive_ok) continue;
                for (int rd=0; rd<e->n_regions; rd++) {
                    if (!e->adj[rs][rd]) continue;
                    RegionEconomy *dst=&e->region[rd];
                    if (!dst->active || dst->colonized) continue;
                    /* score de BASE = expansion vers la CAPACITÉ (préserve la croissance saine, le
                     * comportement d'avant : la pop ne s'effondre pas). Un STEER needs-aware NORMALISÉ
                     * biaise vers une tuile RICHE d'un flux à déficit URGENT (runway<SAFETY ou
                     * structurel) — la valeur ÉMERGE du prix —, SANS que le volume brut écrase la
                     * capacité (borne geo_eff × prime de prix). Sans urgence → steer=0 → capacité pure. */
                    float base = dst->cap_pop*0.001f + (spop-COLONY_MIN_POP)*0.0005f + src->food_sat;
                    float steer=0.f;
                    for (int g=1; g<RES_PROD_FIRST; g++){
                        if (dst->raw_cap[g]<=0.f) continue;
                        if (fc.runway[g] < safety || fc.struct_deficit[g]){
                            float rich = clampf(dst->raw_cap[g]/geo_ref, 0.f, geo_cap);            /* ≈ geo_eff du gisement */
                            float val  = src->price[g]/fmaxf(BASE_PRICE[g],0.1f);                  /* prime de prix (valeur émergente) */
                            steer += rich*val;
                        }
                    }
                    float score = base + needs_w*steer;
                    if (normal_ok && score>best_score){ best_score=score; best_src=rs; best_dst=rd; }
                    /* ANTI-SPIRALE (étage 3a) : la meilleure tuile VIVRIÈRE à portée, gate levé —
                     * réservée à la crise FOOD (sinon les colonies de survie draineraient les petites
                     * sources hors crise et la pop s'effondrerait). Une seule, quand rien d'autre. */
                    bool food_tile = (dst->raw_cap[RES_GRAIN]>0.f || dst->raw_cap[RES_FISH]>0.f
                                    || dst->raw_cap[RES_LIVESTOCK]>0.f);
                    if (survive_ok && food_tile && base>surv_score){ surv_score=base; surv_src=rs; surv_dst=rd; }
                }
            }
            int csrc=best_src, cdst=best_dst;
            if (csrc<0 && food_crit){ csrc=surv_src; cdst=surv_dst; }   /* anti-spirale : SEULEMENT en crise vivrière */
            if (csrc>=0 && cdst>=0) {
                colonize_from(e, csrc, cdst, cid);
                founded++;
            }

        } else if (role==POLITY_CITY_STATE) {
            /* Ne peut coloniser que ses propres régions vacantes adjacentes à
             * une de ses régions déjà peuplées. */
            int best_src=-1, best_dst=-1; float best_score=-1.f;
            for (int ri=0; ri<ct->n_regions; ri++) {
                int rs=ct->region_ids[ri];
                if (rs<0||rs>=e->n_regions) continue;
                RegionEconomy *src=&e->region[rs];
                if (!src->colonized || src->owner!=cid) continue;
                float spop=0.f; for(int c=0;c<CLASS_COUNT;c++) spop+=src->strata[c].pop;
                if (spop<COLONY_MIN_POP || src->food_sat<COLONY_FOOD_GATE) continue;
                /* Cibles : uniquement les régions sœurs du même pays */
                for (int rj=0; rj<ct->n_regions; rj++) {
                    int rd=ct->region_ids[rj];
                    if (rd<0||rd>=e->n_regions||!e->adj[rs][rd]) continue;
                    RegionEconomy *dst=&e->region[rd];
                    if (!dst->active || dst->colonized) continue;
                    float score = dst->cap_pop*0.001f + spop*0.0005f;
                    if (score>best_score){ best_score=score; best_src=rs; best_dst=rd; }
                }
            }
            if (best_src>=0 && best_dst>=0) {
                colonize_from(e, best_src, best_dst, cid);
                founded++;
            }
        }
    }
    return founded;
}

/* ====================================================================== */
/* MIGRATION INTERNE                                                       */
/* ====================================================================== */
/* Principe : les bourgeois et élites migrent vers les régions adjacentes
 * plus prospères. La migration crée de la DIASPORA dans la destination, dont
 * l'apport d'innovation (tech) dépend de la DISTANCE CULTURELLE entre la
 * population d'origine et la population hôte (et non de l'écart de richesse).
 *
 * Les laborers ne migrent pas spontanément ; ils sont l'objet de relocalisation
 * forcée (econ_relocate_pop).                                                  */

int econ_migrate_tick(WorldEconomy *e, const World *w) {
    (void)w;  /* adj est dans e ; w réservé pour extensions futures */
    int flows=0;

    for (int rs=0; rs<e->n_regions; rs++) {
        RegionEconomy *src=&e->region[rs];
        if (!src->colonized || src->gdp<=0.f) continue;

        for (int rd=0; rd<e->n_regions; rd++) {
            if (!e->adj[rs][rd]) continue;
            RegionEconomy *dst=&e->region[rd];
            if (!dst->colonized) continue;

            /* Différentiel de prospérité : dst doit être significativement
             * plus riche pour attirer des migrants. */
            float pros_src = src->prosperity + 0.01f;  /* éviter div/0 */
            float pros_dst = dst->prosperity;
            if (pros_dst <= pros_src * MIGRATE_THRESHOLD) continue;

            /* Taux de migration proportionnel au différentiel, plafonné. */
            float ratio = fminf(MIGRATE_RATE,
                                (pros_dst/pros_src - 1.f) * 0.04f);
            float migrated=0.f;

            for (int cl=CLASS_BOURGEOIS; cl<CLASS_COUNT; cl++) {
                float mv = src->strata[cl].pop * ratio;
                if (mv < 0.5f) continue;
                /* Transfert pop + richesse proportionnelle */
                float wfrac = mv / fmaxf(src->strata[cl].pop, 1.f);
                float wmv   = src->strata[cl].wealth * wfrac;
                src->strata[cl].pop    -= mv;
                src->strata[cl].wealth -= wmv;
                dst->strata[cl].pop    += mv;
                dst->strata[cl].wealth += wmv;
                migrated += mv;
            }

            if (migrated < 0.5f) continue;

            /* Effet diaspora : la nouveauté apportée est la DISTANCE CULTURELLE
             * (L∞ de contenu, [0..1]) entre population source et hôte. Deux
             * populations proches n'innovent pas en se mêlant ; deux populations
             * éloignées fécondent l'hôte (au prix d'une friction d'intégration). */
            const PopCulture *csrc = &src->culture;
            const PopCulture *cdst = &dst->culture;
            Culture tmp_src = { .valeurs=csrc->valeurs, .subsistance=csrc->subsistance,
                                .parente=csrc->parente,  .religion=csrc->religion };
            Culture tmp_dst = { .valeurs=cdst->valeurs, .subsistance=cdst->subsistance,
                                .parente=cdst->parente,  .religion=cdst->religion };
            float novelty = culture_content_distance(&tmp_src, &tmp_dst) / 10.f; /* [0..1] */
            dst->diaspora_pop         += migrated;
            dst->diaspora_innovation  += migrated * novelty;
            flows++;
        }
    }
    return flows;
}

/* ====================================================================== */
/* RELOCALISATION FORCÉE                                                   */
/* ====================================================================== */
/* Déplace `amount` habitants (surtout laborers) de src → dst.
 * Génère un pic de coercition dans la source, proportionnel à la fraction
 * déplacée. La destination subit un léger choc d'accueil (stress social).  */

void econ_relocate_pop(WorldEconomy *e, int src_rid, int dst_rid, float amount) {
    if (src_rid<0||src_rid>=e->n_regions||dst_rid<0||dst_rid>=e->n_regions) return;
    RegionEconomy *src=&e->region[src_rid];
    RegionEconomy *dst=&e->region[dst_rid];
    if (!src->colonized||!dst->colonized||amount<1.f) return;

    float src_pop=0.f;
    for (int c=0;c<CLASS_COUNT;c++) src_pop+=src->strata[c].pop;
    float take=fminf(amount, src_pop*0.5f);  /* limite : 50% de la source */
    if (take<1.f) return;

    /* Prélève d'abord les laborers (80%), puis les bourgeois si insuffisant. */
    float lab_take = fminf(take*0.8f, src->strata[CLASS_LABORER].pop);
    float bou_take = fminf(take - lab_take, src->strata[CLASS_BOURGEOIS].pop);
    src->strata[CLASS_LABORER].pop   -= lab_take;
    src->strata[CLASS_BOURGEOIS].pop -= bou_take;
    dst->strata[CLASS_LABORER].pop   += lab_take;
    dst->strata[CLASS_BOURGEOIS].pop += bou_take;

    /* Coercition source : proportionnelle à la fraction déplacée.
     * La destination a un léger choc de réception (10% du spike source). */
    float frac = take / fmaxf(src_pop, 1.f);
    src->coercion = fminf(1.f, src->coercion + RELOC_COERCION_BASE * frac * 4.f);
    dst->coercion = fminf(1.f, dst->coercion + RELOC_COERCION_BASE * 0.10f);
}

/* ====================================================================== */
/* AFFICHAGE CONSOLE                                                       */
/* ====================================================================== */

void econ_print_region(const WorldEconomy *e, const World *w, int region_id) {
    if (region_id<0||region_id>=e->n_regions) return;
    const RegionEconomy *re=&e->region[region_id];
    const Region *rg=&w->region[region_id];
    const char *status = re->impassable ? "[INFRANCHISSABLE]"
                       : re->active    ? ""
                       :                 "[inactive]";
    printf("\n┌─ Région #%d  « %s »  (tick %d) %s  hab=%.0f%%\n",
           region_id, rg->name[0]?rg->name:"—", e->tick,
           status, re->habitability*100.f);
    if (!re->active) return;

    printf("│ Population & classes\n");
    float tot=0.f; for(int c=0;c<CLASS_COUNT;c++) tot+=re->strata[c].pop;
    for (int c=0;c<CLASS_COUNT;c++) {
        const PopStratum *st=&re->strata[c];
        printf("│   %-10s  pop %7.0f (%4.1f%%)  richesse %8.0f  satisf %3.0f%%\n",
               social_class_name(c), st->pop, tot>0?100.f*st->pop/tot:0.f,
               st->wealth, st->satisfaction*100.f);
    }
    printf("│ Satisfaction générale : %.0f%%   PIB %.0f   Trésor %.0f   Tech %.1f\n",
           re->satisfaction*100.f, re->gdp, re->treasury, re->tech);
    if (re->diaspora_pop > 0.5f || re->coercion > 0.005f)
        printf("│ Diaspora : %.0f hab  innov %.2f  coercition %.0f%%\n",
               re->diaspora_pop, re->diaspora_innovation, re->coercion*100.f);

    printf("│ Manufactures\n");
    if (re->n_bld==0) printf("│   (aucune)\n");
    for (int i=0;i<re->n_bld;i++) {
        const Building *b=&re->bld[i];
        printf("│   %-28s niv %4.1f  emploi %6.0f\n",
               building_name(b->type), b->level, b->workers);
    }

    printf("│ Marché (biens actifs)\n");
    printf("│   %-18s %8s %8s %8s %8s\n","bien","prix","offre","demande","stock");
    for (int r=1;r<RES_COUNT;r++) {
        if (re->supply[r]<0.01f && re->demand[r]<0.01f && re->stock[r]<0.01f) continue;
        printf("│   %-18s %8.2f %8.1f %8.1f %8.1f\n",
               resource_name((Resource)r), re->price[r],
               re->supply[r], re->demand[r], re->stock[r]);
    }
    printf("└────────────────────────────────────────────\n");
}

void econ_print_summary(const WorldEconomy *e, const World *w) {
    double pop=0, tech=0, gdp=0, treas=0; float satw=0;
    int active=0, best=-1; float best_gdp=-1.f;
    for (int rid=0; rid<e->n_regions; rid++) {
        const RegionEconomy *re=&e->region[rid];
        if (!re->active) continue;
        active++;
        float rp=0.f; for(int c=0;c<CLASS_COUNT;c++) rp+=re->strata[c].pop;
        pop+=rp; tech+=re->tech; gdp+=re->gdp; treas+=re->treasury;
        satw+=re->satisfaction*rp;
        if (re->gdp>best_gdp){ best_gdp=re->gdp; best=rid; }
    }
    int impass=0;
    for (int rid=0;rid<e->n_regions;rid++) if (e->region[rid].impassable) impass++;
    printf("\n══ SOMMAIRE MONDE  (tick %d) ══\n", e->tick);
    printf("  régions actives : %d / %d  (dont %d infranchissables)\n",
           active, e->n_regions, impass);
    printf("  population tot. : %.0f\n", pop);
    printf("  satisf. moyenne : %.0f%%\n", pop>0?100.f*satw/pop:0.f);
    printf("  PIB cumulé      : %.0f\n", gdp);
    printf("  trésor (taxes)  : %.0f\n", treas);
    printf("  tech cumulée    : %.1f\n", tech);
    if (best>=0)
        printf("  région la plus riche : #%d « %s » (PIB %.0f)\n",
               best, w->region[best].name[0]?w->region[best].name:"—", best_gdp);
}

void econ_prodcap_save(FILE *f){ fwrite(g_prod_cap,sizeof g_prod_cap,1,f); }
bool econ_prodcap_load(FILE *f){ return fread(g_prod_cap,sizeof g_prod_cap,1,f)==1; }
