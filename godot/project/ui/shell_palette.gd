extends RefCounted
## PALETTE SHELL — la source UNIQUE des accents de la famille « cuir sombre » : les écrans
## HORS-PARTIE (menu principal, Options, Nouvelle Partie, Créateur, Feedback…). Cette famille
## est DISTINCTE du thème parchemin in-game (VKit/ParchTheme) : les menus ne sont PAS du
## parchemin, c'est délibéré — ne PAS les brancher sur VKit.
##
## Teinte canonique = famille menu (décision joueur 2026-07-24). Reteinter tout le shell =
## changer ces quatre valeurs ICI, plus rien d'autre. Ne vivent PAS ici (volontairement) :
##   · les ALPHAS de voile (C_BG) et de panneau (C_PANEL) — STRUCTURELS, pas une couleur :
##     plein écran opaque (menu) vs modale translucide sur la carte (créateur) → restent locaux ;
##   · les surcharges de DOMAINE (liseré/titre violet du Créateur de Foi) — restent locales.
const EDGE  := Color(0.79, 0.64, 0.29)   ## or vieilli — liseré (charte parchemin)
const TEXT  := Color(0.88, 0.86, 0.82)   ## texte clair sur cuir sombre
const DIM   := Color(0.66, 0.62, 0.56)   ## texte atténué (labels, légendes)
const TITLE := Color(0.90, 0.76, 0.48)   ## titre or clair
