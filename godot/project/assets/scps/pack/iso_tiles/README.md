# Sol iso — une tuile propre par biome

- `biomes/<clé>.png` — **une tuile nette par biome** (256×128, ex. `plains.png`, `forest.png`,
  `desert.png`…). Le renderer peint la tuile du biome de chaque cellule **par-dessus le blend
  procédural** (qui adoucit les bords + fait l'eau et les côtes). Falaises assombries = barrière.
- **L'art se remplace au slot** : les tuiles ici sont générées (propres, lisibles) ; tu peux les
  remplacer par un pack **CC0** (Kenney « Isometric Landscape ») ou tes propres tuiles IA **par
  biome** (une tuile = un biome, PAS un canevas continu — ça ne tuile pas).

Clés = ordre de l'enum Biome (`scps/scps_types.h`). Modèle complet + falaises : **`godot/ASSETS_ISO.md`**.

Dossier `biomes/` vide ⇒ sol **procédural** seul (repli automatique).
