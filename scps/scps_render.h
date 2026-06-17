/*
 * scps_render.h — API de rendu de la carte
 *
 * Point d'extension : ajouter des ViewMode ici, implémenter dans scps_render.c.
 * Le renderer ne modifie jamais le World — lecture seule.
 */
#ifndef SCPS_RENDER_H
#define SCPS_RENDER_H

#include "scps_types.h"

/* ---- Modes de vue ---------------------------------------------------- */
typedef enum {
    VIEW_TERRAIN = 0,   /* Terrain physique (biomes + hillshading) */
    VIEW_POLITICAL,     /* Territoires (provinces) colorés + terrain */
    VIEW_REGIONS,       /* Régions colorées */
    VIEW_COUNTRIES,     /* Pays colorés */
    VIEW_CONTINENTS,    /* Continents (masses géographiques) */
    VIEW_HEIGHT,        /* Altimétrie (niveaux de gris) */
    VIEW_FERTILITY,     /* Potentiel de civilisation (heatmap) */
    VIEW_MOISTURE,      /* Précipitations / humidité (bleu=humide) */
    VIEW_TEMPERATURE,   /* Température (rouge=chaud) */
    VIEW_RESOURCES,     /* Bien commercial par province */
    VIEW_HABITABILITY,  /* Habitabilité : rouge=mort, jaune=marginal, vert=fertile */
    VIEW_CULTURE,       /* Culture dominante par région (teinte fournie par l'appelant) */
    VIEW_FAITH,         /* Foi dominante par région (teinte fournie par l'appelant) */
    VIEW_COUNT
} ViewMode;

extern const char *VIEW_NAMES[VIEW_COUNT];

/* ---- Paramètres de rendu --------------------------------------------- */
typedef struct {
    float    cam_ox, cam_oy;   /* offset de caméra en cellules-monde */
    float    cam_scale;        /* pixels par cellule */
    int      selected_prov;    /* province surlignée (-1=aucune) */
    bool     show_rivers;
    bool     show_borders;
    bool     show_grid;        /* debug : grille des cellules */
    /* Teinte PAR RÉGION (ARGB), fournie par l'appelant pour VIEW_CULTURE/VIEW_FAITH
     * (la membrane : le viewer calcule les couleurs diégétiques depuis l'éco et les
     * passe ; le renderer ne lit aucun flottant SCPS). NULL = pas de teinte. */
    const uint32_t *region_tint;
    /* OCCUPATION (brief terrain) : couleur de l'OCCUPANT par région (ARGB), 0 = libre.
     * Posée en HACHURE sur la carte politique (la région est TENUE, pas possédée — la
     * propriété ne change qu'à la paix). Membrane : le viewer lit dp->occupier, passe
     * des couleurs de pays ; le renderer ne lit aucun flottant SCPS. NULL = aucune. */
    const uint32_t *occupier_tint;
    /* N3.1 — true : l'appelant trace les frontières politiques en STROKES espace
     * écran (largeur constante au zoom) par-dessus ce rendu → le bake N'EN PEINT
     * PLUS (sinon double trait). false (défaut) : comportement historique — bake
     * 1 cellule (minicarte, outils headless, captures sans strokes). */
    bool screen_strokes;
    /* VUE ISOMÉTRIQUE (display-only) : incline le rendu (rotation 45° + écrasement
     * vertical 2:1) autour du centre de la fenêtre. Le renderer INVERSE la projection
     * (chaque pixel écran → point « plat » → monde) → la fenêtre reste REMPLIE. Le viewer
     * projette ses surcouches (décors/bordures/labels) avec les MÊMES facteurs (ci-dessous)
     * et inverse pour le picking. false = top-down historique. */
    bool iso;
} RenderParams;

/* Facteurs de la projection iso (partagés viewer ⇄ renderer pour rester cohérents).
 * (dx,dy) = point plat relatif au pivot ; écran = pivot + (dx−dy)·KX, (dx+dy)·KY. 2:1. */
#define ISO_KX 0.78f
#define ISO_KY 0.39f

/* ---- Rendu principal --------------------------------------------------
 * Remplit pixels[pw×ph] (format ARGB8888) avec la carte rendue selon
 * le mode demandé.  Thread-safe (lecture seule sur World).
 */
void render_map(const World      *w,
                uint32_t         *pixels,
                int               pw, int ph,
                const RenderParams *p,
                ViewMode          mode);

#endif /* SCPS_RENDER_H */
