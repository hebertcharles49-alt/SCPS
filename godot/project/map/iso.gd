extends RefCounted
## Iso — la PROJECTION isométrique partagée (terrain mesh + overlay + picking), pour
## que tout vive dans le MÊME espace iso. 2:1 diamant ; la HAUTEUR soulève (volume).
##   monde (wx, wy, h∈[0,1]) → écran iso (x, y) :
##     x = (wx - wy)              · y = (wx + wy) * 0.5  -  h * H
## La PROFONDEUR de tri (arrière→avant) = (wx + wy) : plus la somme est grande, plus c'est
## DEVANT (en bas de l'écran). Le shader de terrain duplique la MÊME formule (H ci-dessous).

const H := 90.0   ## relèvement vertical par unité de hauteur (le « volume » iso)

## monde → iso (avec hauteur). `h` ∈ [0,1].
static func proj(wx: float, wy: float, h: float) -> Vector2:
	return Vector2(wx - wy, (wx + wy) * 0.5 - h * H)

## profondeur de tri (arrière → avant) d'un point monde.
static func depth(wx: float, wy: float) -> float:
	return wx + wy

## inverse PLAT (h=0) : iso → monde (pour le picking ; la hauteur est ignorée, suffisant).
##   wx = iso_y + iso_x * 0.5   ·   wy = iso_y - iso_x * 0.5
static func unproj(ix: float, iy: float) -> Vector2:
	return Vector2(iy + ix * 0.5, iy - ix * 0.5)
