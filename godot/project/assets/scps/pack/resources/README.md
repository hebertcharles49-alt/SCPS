# Sprites de ressources (assets/scps/pack/resources/)

UIKit.resource_sprite(res_id, nom) charge ici, dans l'ordre :
1. `<res_id>.png`   — par INDICE d'enum Resource (0 = RES_NONE/vide, 1 = grain, …)
2. `%03d.png`       — variante zéro-paddée (`013.png`)
3. `<clé_du_nom>.png` (repli par nom normalisé : minuscules, accents ôtés, espaces→_)
sinon repli sur une icône du pack UI (grain/pierre/or/outils…), sinon le TEXTE.

Le NOM de la ressource s'affiche au SURVOL dans tous les cas.

Ordre de l'enum (cf. scps_types.h, Resource) : 0 vide · 1 grain · 2 bétail · 3 laine ·
4 poisson · 5 fourrure · 6 sel · 7 coton · 8 sucre · 9 bois · 10 herbes · 11 cuivre ·
12 fer · 13 charbon · 14 soufre · 15 salpêtre · 16 or · 17 métal précieux · 18 perle ·
19 cristal arcanique · 20 fer céleste · 21 murex · 22 indigo · 23 argile · 24 pierre · …
(suite : étoffe, fournitures navales, vin, bière, … métal, outils, essence, armes…)
