#ifndef SCPS_ARMY_H
#define SCPS_ARMY_H
/*
 * scps_army.h — LES ARMÉES : recrutement, armes, pierre-feuille-ciseaux, combat au dé
 *
 * « Créer une armée n'est pas juste appuyer sur un bouton. » Une unité = 100 pop
 * d'une CLASSE (élite→cavalerie, commun→piétaille) + des ARMES FABRIQUÉES
 * (matériaux → armes, même logique que les bâtiments) + du temps. Pas d'armes en
 * stock → pas de levée. La pop enrôlée RESTE dans le pool (scps_labor) : un
 * enrôlement est une affectation, pas un retrait.
 *
 * Le combat est un PIERRE-FEUILLE-CISEAUX à grande échelle, en doublon (réseau de
 * contres redondant), résolu par des JETS DE DÉ pondérés par les contres ET les
 * stats. Le dé apporte l'incertitude ; le matchup et les stats penchent la balance.
 * Le contre PRIME sur la qualité brute — un mur de piquiers bon marché brise une
 * cavalerie d'élite.
 */
#include "scps_labor.h"   /* LaborEcon : pop par classe, matériaux, armes fabriquées */
#include "scps_tech.h"    /* TechState : l'arbre qui façonne la doctrine d'armée (§3) */

#define POP_PER_UNIT     100   /* enrôlement par paquets de 100 */
#define ARMY_MAX_UNITS   32

/* ---- Types d'unité ---------------------------------------------------- */
typedef enum {
    U_PIQUIER=0, U_LANCIER, U_EPEISTE, U_ARCHER, U_ARBALETE,
    U_CAV_LEGERE, U_CAV_LOURDE, U_MAGE,
    /* F5 — extension du roster (appendus avant U_COUNT : indices préservés, SAVE bump) */
    U_HALLEBARDIER,  /* lourd, anti-cavalerie ← RES_ARMS_HEAVY */
    U_ARQUEBUSIER,   /* feu, perce l'armure ← RES_FIREARM */
    U_ALCHIMISTE,    /* soutien Fluide ← RES_ALCHEMIST_KIT */
    U_GARDE_RUNIQUE, /* élite arcane mêlée (affichée « Chaman ») ← RES_ENCHANTED_ARMS */
    /* Roster 22 (spec design) — 10 unités appendues AVANT U_COUNT : indices des 12
     * préservés (SAVE bump pour la taille du tampon d'armes, pas pour les indices). */
    U_ARBALETE_LOURDE, /* tir perce-armure lourd à pavois (tech) ← RES_ARMS_RANGED */
    U_BERSERKER,       /* choc-suicide, torse nu ← RES_ARMS_HEAVY */
    U_LANCIER_CHOC,    /* lance lourde anti-TOUTE-cavalerie + percée ← RES_ARMS_HEAVY */
    U_MILICE,          /* masse paysanne, armes de fortune ← RES_NONE (adaptable, ~gratuit) */
    U_HARCELEUR,       /* escarmouche évasive, anti-poursuite ← RES_ARMS_RANGED */
    U_TRAQUEUR,        /* tir d'embuscade de terrain ← RES_ARMS_RANGED */
    U_LAME_FRANCHE,    /* mercenaire polyvalent, soldé en OR ← RES_ARMS_LIGHT */
    U_GARDE_ESCORTE,   /* ancre défensive, moral incassable ← RES_ARMS_HEAVY */
    U_CAV_CUIRASSEE,   /* apex du choc, masse pure (tech tardive) ← RES_ARMS_HEAVY */
    U_CAV_RAID,        /* raid, pillage, anti-tireur ← RES_ARMS_LIGHT */
    U_COUNT
} UnitType;

/* ---- Armes fabriquées (matériaux → arme) ------------------------------ */
typedef enum {
    W_PIQUE=0, W_LANCE, W_EPEE, W_ARC, W_ARBALETE, W_MONTURE_L, W_MONTURE_H, W_BATON,
    W_HALLEBARDE, W_ARQUEBUSE, W_ALCHIMIE, W_RUNES,   /* F5 (appendus avant W_COUNT) */
    /* Roster 22 — armes des 10 unités appendues (NOMS pour l'UI ; un slot de tampon
     * de combat chacun → W_COUNT croît, d'où le SAVE bump). */
    W_ARBALETE_LOURDE, W_HACHE, W_LANCE_LOURDE, W_FORTUNE, W_ARC_COURT,
    W_ARC_CHASSE, W_EPEE_LOUEE, W_PERTUISANE, W_MONTURE_CUIRASSEE, W_MONTURE_RAID,
    W_COUNT
} ArmWeapon;

/* Définition statique d'un type d'unité. */
typedef struct {
    const char *name;
    LaborClass  from;      /* classe sociale prélevée (§2) */
    ArmWeapon   weapon;    /* arme requise (§1) */
    /* stats (§5) */
    float discipline;      /* +% dégâts infligés ET réduction des dégâts reçus */
    float moral;           /* capacité d'encaisse PAR paquet de 100 */
    float mouvement;       /* débordement (flanc) + vitesse de carte */
    float commandement;    /* abaisse le seuil du jet de dé */
} UnitDef;

/* ---- Une unité levée -------------------------------------------------- */
typedef struct {
    UnitType type;
    long     count;          /* en paquets de 100 */
    float    moral_courant;  /* la réserve de moral qui s'épuise au combat */
} Unit;

/* ---- §3 : LA DOCTRINE — ce que la TECH fait à une armée ---------------- */
/* L'arbre (scps_tech) façonne la guerre par sa fonction ARMÉE, thème par thème.
 * Le poids d'un nœud ∝ sa PROFONDEUR (tier) : le bord faustien pèse le plus. */
typedef struct {
    float weapon_power;   /* FORGE·Armée   : multiplie les dégâts (meilleures armes ; ≥1) */
    float moral_mul;      /* SOCIÉTÉ·Armée : multiplie la réserve de moral (tient plus ; ≥1) */
    float arcane_power;   /* SAVOIR·Armée  : multiplie les dégâts du MAGE (l'arcane ; ≥1) */
    float firearm_power;  /* APEX Arquebuse runique : multiplie les dégâts de l'ARQUEBUSIER (≥1) */
    bool  can_summon;     /* SAVOIR·Armée faustien : l'INVOCATION déverrouillée */
} ArmyDoctrine;

typedef struct {
    Unit units[ARMY_MAX_UNITS];
    int  n_units;
    long weapons[W_COUNT];               /* stock d'armes fabriquées */
    long pop_by_class_in_army[LAB_CLASS_COUNT];   /* affectées (toujours dans le pool labor) */
    ArmyDoctrine doctrine;               /* §3 : façonnée par la tech (neutre par défaut) */
} ArmyState;

/* ---- Phases d'une bataille dans le temps (§2) ------------------------- */
/* Une bataille n'est pas un instant : elle a un ARC. Le CHOC (l'échange) use le
 * moral ; quand une armée rompt vient le RETRAIT (elle se dérobe) puis, si le
 * vainqueur est plus rapide, la POURSUITE — la curée où tombe le gros des morts. */
typedef enum { PH_CHOC=0, PH_RETRAIT, PH_POURSUITE, PH_COUNT } BattlePhase;

/* ---- Résultat de bataille --------------------------------------------- */
typedef struct {
    int   winner;   /* -1 = A gagne, +1 = B gagne, 0 = nul */
    int   routA, routB;
    int   rounds;
    /* §2 : la bataille dans le temps */
    float       days;        /* durée totale — les deux armées sont CLOUÉES ce temps */
    BattlePhase last_phase;  /* phase atteinte (choc / retrait / poursuite) */
    int         pursued;     /* paquets de 100 fauchés à la poursuite (le vaincu) */
} BattleResult;

/* ===================================================================== */
/* API                                                                   */
/* ===================================================================== */
const UnitDef     *unit_def(UnitType t);
const char        *unit_name(UnitType t);
const char        *weapon_name(ArmWeapon wp);
TechId             unit_tech_gate(UnitType t);                 /* F7 : la tech qui débloque l'unité (TECH_COUNT = aucune) */
bool               unit_recruitable(const TechState *ts, UnitType t);  /* F7 : recrutable (tech acquise) ? */
Resource           unit_res_arm(UnitType t);                   /* F5/F6 : la catégorie d'arme MACRO (RES_*) de l'unité */

void army_init(ArmyState *a);

/* Peut-on lever `count` unités de ce type ? (pop libre de la bonne CLASSE +
 * armes en stock). Renvoie false sinon — ce n'est pas un bouton. */
bool army_can_recruit(const ArmyState *a, const LaborEcon *e, UnitType t, long count);
/* Lève l'unité : prélève la pop (affectée, PAS retirée du pool) et consomme les
 * armes du tampon (rempli au préalable depuis le marché macro). Renvoie le nb levé. */
long army_recruit(ArmyState *a, LaborEcon *e, UnitType t, long count);

/* ---- Le pierre-feuille-ciseaux (§3) ----------------------------------- */
/* >1 si `a` contre `b`, <1 si `a` est contré, 1 neutre. */
float matchup(UnitType a, UnitType b);

/* ---- Le combat au dé (§4) --------------------------------------------- */
/* Le jet réussit-il ? (dé d20 + commandement ≥ seuil). */
bool  arm_hit(float commandement, int roll_d20);
/* Dégâts d'un contact réussi : contre × discipline_a × terrain, réduits par la
 * discipline de b. (Exposé pour vérifier la pondération.) */
float arm_damage(UnitType a, UnitType b, long count_a, float disc_b, float terrain);
/* Résout une bataille : appariement, débordement par le mouvement, contacts au
 * dé, le moral s'épuise, rupture quand il tombe ; l'armée la plus rompue recule.
 * terrainA module l'efficacité de A (forêt favorise l'embuscade, plaine la
 * cavalerie — lien couche d'action). rng = graine (xorshift) avancée en place. */
BattleResult resolve_battle(ArmyState *A, ArmyState *B, float terrainA, uint32_t *rng);

/* ===================================================================== */
/* §1 — LE DÉPLACEMENT SUR LE TERRAIN                                     */
/* ----------------------------------------------------------------------
 * Le terrain décide du TEMPS. Un sommet ne se traverse pas ; une plaine se
 * dévore ; une forêt et un marais ralentissent ; une rivière se franchit
 * lentement et à découvert. On avance au pas du convoi (l'unité la plus lente),
 * et la marche use : le désert assoiffe, le marais épuise. Tout est en JOURS —
 * le combat (résolu en manches) s'inscrira dans la même horloge (§2).        */
/* ===================================================================== */

/* Terrain infranchissable (glacier, pic, océan, volcan) : aucune armée n'y
 * passe ; army_step_days y renvoie l'infini. */
bool  terrain_impassable(Biome b);

/* Multiplicateur de vitesse du terrain (1 = référence plaine cultivée). Le
 * relief (height 0..1) ralentit en plus : grimper coûte. ≤0 = infranchissable. */
float terrain_move_factor(Biome b, float height);

/* Taux d'attrition JOURNALIER de la marche (fraction des effectifs perdue par
 * jour sur ce terrain) : désert et marais saignent, la plaine épargne. */
float march_attrition_rate(Biome b);

/* Vitesse de carte de l'armée = celle de son unité LA PLUS LENTE (pas du
 * convoi). 0 si l'armée est vide ou sans unité vivante. */
float army_slowest_move(const ArmyState *a);

/* Vitesse de l'unité LA PLUS RAPIDE (la pointe qui poursuit, §2). 0 si vide. */
float army_fastest_move(const ArmyState *a);

/* Nom diégétique d'une phase de bataille (Choc / Retrait / Poursuite). */
const char *battle_phase_name(BattlePhase ph);

/* ---- §3 : la cascade technologique → la doctrine ---------------------- *
 *   FORGE·Armée   → de meilleures ARMES (les coups mordent plus fort)
 *   SOCIÉTÉ·Armée → ORGANISATION & MORAL (l'armée tient avant de rompre)
 *   SAVOIR·Armée  → l'ARCANE (le mage frappe plus fort) et, au bord faustien,
 *                   l'INVOCATION (une armée sans pop). Plus le nœud est PROFOND
 *                   (tier), plus il pèse — le bord faustien décuple.            */
ArmyDoctrine army_doctrine_base(void);          /* neutre : tout à 1, pas d'invocation */
ArmyDoctrine army_doctrine(const TechState *t); /* lue depuis l'arbre déverrouillé d'un empire */

/* Jours pour qu'une armée franchisse une case de biome `to` (hauteur `height`).
 * `river_crossing` : on franchit un cours d'eau (lent, à découvert) ;
 * `on_route` : une route porte la marche (plus vite). INFINITY si le terrain
 * est infranchissable ou l'armée incapable de bouger. */
float army_step_days(const ArmyState *a, Biome to, float height,
                     bool river_crossing, bool on_route);

/* Applique l'attrition d'une marche de `days` jours sur le biome `b` : ampute
 * chaque unité de la fraction perdue (paquets de 100, jamais sous 0). Renvoie
 * le total de paquets fondus en route. */
long  army_march_attrition(ArmyState *a, Biome b, float days);

/* ===================================================================== */
/* LE SIÈGE — combien de temps pour CONTRÔLER une province               */
/* ----------------------------------------------------------------------
 * Marcher dans une province SANS défense, c'est l'affaire de 14 jours : on
 * plante le drapeau. Défendue, c'est un SIÈGE qui s'étire — au plus 2 ANS —
 * d'autant plus long que la place est FORTIFIÉE, qu'elle a des VIVRES pour tenir,
 * et que le TERRAIN l'abrite. Même horloge en JOURS que la marche (§1) et la
 * bataille (§2) : prendre une terre coûte du temps, pas un clic.              */
/* ===================================================================== */

/* Multiplicateur défensif du terrain (≥1) : le relief qui FREINE la marche (§1)
 * ABRITE le défenseur. La plaine n'aide pas (1) ; la montagne tient (≈1.8 +
 * relief) ; forêt, marais et hauteurs abritent un peu. */
float terrain_defense_mult(Biome b, float height);

/* Jours pour CONTRÔLER une province :
 *   defense_level ≤ 0  → 14 jours (aucune défense : on entre, on plante le drapeau).
 *   sinon → (base + fortification·defense_level + vivres·food_months)·def_mult,
 *           borné à 2 ANS (730 jours).
 * food_months = mois de vivres stockés (la garnison tient tant qu'elle mange) ;
 * def_mult = terrain & multiplicateurs divers (cf. terrain_defense_mult). */
float siege_days(float defense_level, float food_months, float def_mult);

/* Bonus de COMBAT du défenseur selon le terrain (multiplicateur ≥1) : la pente et
 * le couvert paient AU CHOC (distinct du siège, qui ne fait qu'allonger le temps).
 * Coline +5 %, montagne +20 % ; le reste entre les deux (forêt/jungle = couvert,
 * marais = approche brisée) ; plaine = rien. À passer côté défenseur dans
 * resolve_battle (terrainA). */
float terrain_combat_bonus(Biome b);

#endif /* SCPS_ARMY_H */
