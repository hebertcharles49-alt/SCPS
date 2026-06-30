extends RefCounted
## Frame — les DIMENSIONS du cadre d'écran (barres pleine largeur/hauteur). Source
## UNIQUE, préchargée par les barres ET les panneaux pour rester cohérents quand on
## redimensionne la fenêtre. `const Frame = preload("res://ui/frame.gd")`.

const TOPBAR_H := 38.0      ## bandeau haut (pleine largeur)
const BOTTOMBAR_H := 50.0   ## bandeau bas (pleine largeur) — porte mode & zoom
const SIDEBAR_W := 48.0     ## rail gauche (pleine hauteur, entre les deux bandeaux)
const MARGIN := 8.0
