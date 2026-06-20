# Tuiles iso de terrain

Dépose ici un atlas **4×4 de 256×128** par biome : `<clé_biome>.png` (1024×512).
Clés & schéma (dual-grid 16 masques, priorités, eau) : voir **`godot/ASSETS_ISO.md`**.

Tant que ce dossier ne contient aucun atlas, le sol reste en rendu **procédural**
(repli automatique). Dès qu'un atlas de biome terre est présent, le sol passe en tuiles.
