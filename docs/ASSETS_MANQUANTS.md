# Audit des assets — état & liste pour Codex (2026-07-07)

## Verdict : rien de critique n'a été supprimé — les têtes de conseillers sont là

Le système d'assets est un jeu de **32 planches** (sprite sheets) sous
`godot/project/assets/scps/ui/parch/` (≈500 vignettes, cellules 256² alpha, DA
parchemin/gravure). UIKit (`ui/uikit.gd`) y pioche par nom de planche.

**Présent et fonctionnel** (rien à demander) :
- **Têtes des conseillers** → `sheet13_advisors_01..16` (8 bustes × variante féminine),
  chargées par `UIKit.advisor_portrait(seat, fem)`, affichées dans le tiroir Conseil
  (`sidebar_drawer.gd:264`). **Elles n'ont pas été supprimées ni déplacées hors d'atteinte.**
- Édifices (`sheet06/07` + doublon `assets/scps/pack/buildings/EDI_*.png`), manufactures
  (`sheet08` + complément `sheet27`), unités (`sheet09/10`), blasons de faction (`sheet14`),
  tech (`sheet05`), chrome UI (`assets/scps/ui/chrome/*`), polices (`assets/fonts/*`).
- **16 chips de ressources BRUTES** (planche 19, via `PARCH_RES`) : grain, poisson, bétail,
  fruits, bois, pierre, argile, fer, cuivre, or, sel, laine, fourrure, charbon, cristal
  arcanique, fer céleste.

**Les 7 `res://*.png` « manquants » (globe, plateau_map, series2_a/b, sidebar1, ui1,
heraldry_contact)** sont des chemins de SORTIE de scripts-sonde headless (`*_audit.gd`,
`shot.gd`) — des captures écrites, pas des assets lus. **Aucun impact jeu.**

**Orphelins** (présents mais plus référencés depuis la carte 100 % procédurale) :
`art/map_stamps/**` (~130 PNG). Suppressibles un jour, mais inoffensifs.

---

## LE SEUL VRAI TROU : ~31 icônes de ressources TRANSFORMÉES

Ce que tu as vu : le dossier `assets/scps/pack/resources/` ne contient qu'un `README.md`.
C'est le canal SECONDAIRE (repli par indice d'enum) ; le canal PRIMAIRE (chips planche 19)
ne couvre que les **16 brutes** ci-dessus. Les **~31 biens transformés/secondaires** n'ont
AUCUNE chip → ils s'affichent en **TEXTE** (le nom au survol) ou une icône générique de repli.

C'est ça, la liste à faire produire par Codex.

### La liste (par thème, pour batcher)
| Thème | Ressources sans icône |
|---|---|
| Agricole / teinture | coton · sucre · herbes médicinales · murex (pourpre) · indigo (bleu) |
| Minéral rare | soufre · salpêtre · métal précieux (mithril) · perle |
| Boisson / textile fini | eau-de-vie · bière · tunique · étoffe précieuse · bien précieux (porcelaine) |
| Transformé civil | papier · outils · poterie · statuaire · remèdes · fournitures navales |
| Militaire | armes légères · armes lourdes · armes de trait · armes à feu (poudre à part) · poudre · armes enchantées |
| Arcane | essence · essence purifiée · flux · nécessaire d'alchimiste · bâton de mage |

(≈31 items. Certains — cf. `RES_FALLBACK` dans uikit.gd — retombent sur une icône
générique ; les autres tombent en texte. Aucun n'a de vignette DÉDIÉE.)

### Le prompt CODEX (à lui donner)
> Génère une série d'icônes de ressources dans le style d'un ATLAS ANCIEN / manuscrit
> enluminé : chaque icône = UN objet iconique unique, centré, gravure à l'encre sépia sur
> fond TRANSPARENT (alpha), lavis terreux discrets (ardoise, rouille, ocre, olive), bords
> légèrement brunis, aucun texte, aucune bordure de cadre. Format PNG 256×256, sujet
> occupant ~70 % de la cellule, lisible en petit (≤32 px à l'écran). Cohérent avec des
> chips de ressources brutes existantes (grain, fer, bois gravés au trait). Un fichier par
> ressource : coton (bourre), sucre (pain de sucre), herbes médicinales (bouquet), murex
> (coquillage pourpre), indigo (feuilles/teinture bleue), soufre (cristaux jaunes), salpêtre
> (efflorescence/tonneau), métal précieux (lingot mithril luisant), perle (nacre), eau-de-vie
> (fiole/alambic), bière (chope), tunique (vêtement plié), étoffe précieuse (rouleau de tissu
> ouvré), bien précieux (vase de porcelaine), papier (feuilles/rame), outils (marteau+tenaille),
> poterie (jarre), statuaire (buste sculpté), remèdes (fiole d'apothicaire), fournitures navales
> (cordage+goudron), armes légères (épée courte), armes lourdes (hallebarde), armes de trait
> (arc+flèches), armes à feu (arquebuse), poudre (baril de poudre), armes enchantées (épée
> runique luisante), essence (lueur de mana en flacon), essence purifiée (cristal distillé),
> flux (volute d'énergie), nécessaire d'alchimiste (mortier+fioles), bâton de mage (bâton à gemme).

### Intégration (2 options, zéro casse)
1. **Par INDICE (drop-in, aucun code)** : nommer chaque PNG `<indice_enum>.png` et le poser
   dans `assets/scps/pack/resources/` — `UIKit.resource_sprite` les charge par indice
   automatiquement (après la chip planche 19). L'indice exact = ordre de l'enum `Resource`
   dans `scps/scps_types.h` (je le calculerai au moment de brancher).
2. **Par CHIP (cohérence planche)** : les ajouter à une planche `sheet33_resources_manuf` et
   étendre le dict `PARCH_RES` (une ligne par ressource) — plus homogène avec les 16 brutes.

---

## À demander à Codex, en une phrase
« ~31 icônes de ressources transformées (liste ci-dessus), style atlas ancien / gravure
sépia sur alpha, 256×256, un objet par icône — pour compléter les 16 ressources brutes
déjà présentes. » Les têtes de conseillers, édifices, unités et blasons sont **déjà là**.
