extends RefCounted
## Frame — les DIMENSIONS du cadre d'écran (barres pleine largeur/hauteur). Source
## UNIQUE, préchargée par les barres ET les panneaux pour rester cohérents quand on
## redimensionne la fenêtre. `const Frame = preload("res://ui/frame.gd")`.

const TOPBAR_H := 48.0      ## bandeau haut (pleine largeur) — 48 px : cellules CK3 (valeur EMPILÉE sur delta)
const BOTTOMBAR_H := 66.0   ## bandeau bas (pleine largeur) — porte mode & zoom (agrandi, retour joueur)
const SIDEBAR_W := 64.0     ## rail gauche (pleine hauteur) — boutons 52 px (retour joueur « très petits »)
const MARGIN := 8.0
