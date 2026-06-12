# SCPS — Tech Forks & Émergences Culturelles
## Design Document v3 — Ancrage moteur

> M0 — versé TEL QUEL : ce document est la SOURCE DE VÉRITÉ de l'arc M.
> Périmètre v1 = §23.2 (7 items) ; §23.3 = INTERDIT en v1.

---
## Changelog v2 → v3
Corrections de cohérence avec le cœur SCPS vérifié (`scps_core`, `scps_agency`, `scps_econ`, `scps_ai`, `scps_factions`, `scps_tech`) :
1. **§18 réécrit** — la cloche maison (gaussienne) supprimée : on réutilise `scps_bell` (parabole, nulle aux extrêmes). Le gate réaction/fracture devient `scps_metabolisation`, déjà dans le moteur.
2. **Vocabulaire P** — `P = perméabilité` (coût de défection des élites), **pas** la pression de contact. La pression est `I`. Corrigé partout.
3. **Boucle salpêtre** (§10) — le salpêtre alimente déjà la poudre (`BLD_POWDERMILL`) ET l'alambic : une ressource, deux doctrines opposées. Nommé explicitement.
4. **Chaînes arcanes auto-contenues** (§10) — chaque pôle a sa propre chaîne : ressource brute → tier1 → ressource intermédiaire → tier2. Pas de dépendance cross-pôle. Martial : fer céleste → Forge de Runes → Forge Céleste. Ordre : cristal → Atelier du Mage → Tour d'Ivoire (produit essence). Fluide : salpêtre → Alambic → Grand Alambic (produit essence purifiée, alimente l'Alchimiste).
5. **Hystérésis du pôle** (§8) — verrou de 360 j + fork bâti non reconvertible, pour empêcher l'oscillation des successeurs en cours de partie.
6. **Couche Arcane** (§9.9) — c'est la couche **manufacture** (`BLD_*`), pas édifice. Le proto vertical traverse les deux couches volontairement.
7. **Souche déjà nommée** — le type C est déjà `SpeciesArchetype` ; seules les constantes disent `RACE_`. Rename minimal.
8. **`PROF_*` déjà en code** (§20) — la cristallisation étendra le prédicat des `SyncNode` au lieu d'un système parallèle.
9. **Renommages** — Entrepôt des Flux → Marché aux grains ; Creuset des Simples/Grand Athanor → Alambic/Grand Alambic ; Pisteur → Harceleur.
10. **Contrainte masque** — `edi_built` est `uint32_t`, `EDIFICE_COUNT=20`. V1 (+5 édifices) passe. La matrice complète impose `uint64_t` (prérequis d'extension, pas de v1).

---
## 0. Contraintes de design
1. **Lisibilité** : le nom du nœud dit sa fonction.
2. **Expression culturelle** : deux civilisations résolvent le même problème par des formes différentes.
3. **Rareté matérielle** : l'accès profond dépend du monde, pas d'un verrou abstrait.
4. **Émergence** : certaines formes n'apparaissent qu'au contact d'autres formes.

---
## 1. Principe fondateur
**Nom = fonction.** Le joueur sait ce qu'il recherche sans lire la description.
L'arbre concentrique existant (3 thèmes × 3 fonctions, `scps_tech`) reste la **spine Ordre**. Deux pôles l'encadrent :
- **Martial** : force, discipline, mobilisation militaire.
- **Ordre** : institution, conservation, métabolisation.
- **Fluide** : circulation, échange, adaptation.
Le pôle dominant est lu depuis la distribution des factions (`scps_factions`), localement quand c'est possible. La spine Ordre reste accessible à tous : les forks sont préférentiels, pas exclusifs.

---
## 2. Vocabulaire : Souche, pas Race
Le type C est **déjà** `SpeciesArchetype` (`scps_species.h`). Seules les constantes (`RACE_ELFE`…`RACE_ORQUE`) gardent le préfixe. Côté design : **Souche / Espèce**. Côté code, à la prochaine touche : alias `tech_species_affinity` (l'ABI reste).
La souche n'oriente **pas** une intelligence morale ou politique. Elle oriente des affinités matérielles : physiologie, biotope, traditions techniques héritées. La politique vient des factions et de l'éthos.
Acceptable : une espèce amphibie comprend mieux les routes fluviales ; une cavernicole travaille tôt la pierre et les réseaux souterrains ; une nomade développe la mobilité et le ravitaillement léger.
À éviter : « cette souche est faite pour dominer », « inférieure en savoir ».

---
## 3. Les trois signaux fusionnés
| Signal | Oriente | Mécanisme |
|---|---|---|
| **Souche** | Thème | `tech_race_affinity()` (compat) → `tech_species_affinity()` |
| **Éthos** | Fonction | `ETHOS_FN[ETHOS][FUNC]` (nouveau, dans `scps_ai`) |
| **Credo** | Appétit faustien | `faustian_appetite = CREDO_FAUST[credo] * (0.6 + 0.08 * valeurs)` |
```c
score(node) = desirability(node)
            * species_theme_affinity(species, node.theme)
            * ethos_function_weight(ethos, node.func)
            * (node.faustian ? faustian_appetite(credo, valeurs) : 1.0f)
            * material_depth_factor(node, region);
```
La souche pousse vers un **domaine**. L'éthos vers une **fonction**. Le credo décide si l'on paie le coût du feu. La matière décide jusqu'où l'on va.

---
## 4. Deux natures de gate
### 4.1 Gate d'éthos — quelle fourche
L'éthos décide quelle forme émerge naturellement. Maritime : Martial → **Arsenal** · Ordre → **Amirauté** · Fluide → **Port marchand**. Les trois restent concevables ; sortir de sa pente coûte une friction.
### 4.2 Gate de ressource — quelle profondeur
La ressource rare décide jusqu'où l'on va dans la fourche. La rareté **est** le gate : ne pas ajouter de verrou abstrait si la carte le fait déjà.

---
## 5. Pôles technologiques
| Pôle | Désir | Force | Faiblesse | Risque long terme |
|---|---|---|---|---|
| **Martial** | Contraindre | Mobilisation, choc | Métabolisation basse | Brèche, dépendance à la guerre |
| **Ordre** | Stabiliser | Institution, continuité | Inertie | Sclérose, retard adaptatif |
| **Fluide** | Circuler | Richesse, diffusion | H bas, vulnérabilité militaire | Capture privée, dissolution |
Aucun pôle n'est « le bon » : chacun est fort dans sa logique, dangereux dans son excès.

---
## 6. Table ETHOS_FN
Réside dans `scps_ai.c` (seul consommateur, près de `AI_TECH_PENCHANT`). **Indexée sur l'enum réel** `TechFunction` (`scps_tech.h` : `FN_PRODUCTION=0, FN_ARMEE, FN_RENFORCEMENT`) — initialiseurs désignés obligatoires pour éviter l'inversion de colonnes.
```c
static const float ETHOS_FN[ETHOS_COUNT][FN_COUNT] = {
    [ETHOS_DOMINATEUR]  = { [FN_ARMEE]=2.5f, [FN_RENFORCEMENT]=0.8f, [FN_PRODUCTION]=0.6f },
    [ETHOS_HONNEUR]     = { [FN_ARMEE]=2.2f, [FN_RENFORCEMENT]=1.0f, [FN_PRODUCTION]=0.6f },
    [ETHOS_ORDRE]       = { [FN_ARMEE]=1.2f, [FN_RENFORCEMENT]=2.0f, [FN_PRODUCTION]=1.0f },
    [ETHOS_BUREAUCRATE] = { [FN_ARMEE]=0.7f, [FN_RENFORCEMENT]=2.2f, [FN_PRODUCTION]=1.3f },
    [ETHOS_MERCANTILE]  = { [FN_ARMEE]=0.7f, [FN_RENFORCEMENT]=1.0f, [FN_PRODUCTION]=2.3f },
    [ETHOS_PACIFISTE]   = { [FN_ARMEE]=0.4f, [FN_RENFORCEMENT]=1.2f, [FN_PRODUCTION]=2.4f },
};
```
Le Pacifiste n'est pas « faible » : il refuse l'armée directe mais devient puissant par production, démographie, dépendances.

---
## 7. Lecture du pôle dominant
Lu localement. Une région frontalière, portuaire ou sacrée peut dériver du pôle impérial. S'ancre sur `faction_weights_of(provs, n, out)` (provinces arbitraires) et `faction_effective_distribution` (pays). Aucune structure nouvelle de factions.
```c
typedef enum { POLE_MARTIAL=0, POLE_ORDRE, POLE_FLUIDE, POLE_COUNT } TechPole;
/* poids normalisés depuis les factions de la région */
p.martial += w[FAC_CONQUERANT] + 0.8f*w[FAC_GARDIEN];
p.ordre   += w[FAC_LEGISTE]    + 0.8f*w[FAC_COMMUNAUTAIRE];
p.fluide  += w[FAC_MARCHAND];
```
**`FAC_TRANSGRESSEUR` est volontairement absent du pôle** : il est orthogonal, il nourrit `faustian_appetite`, pas la fourche. Ne pas le mapper Martial par réflexe.
Égalité : (1) capitale → pôle impérial ; (2) portuaire → Fluide ; (3) frontalière/militarisée → Martial ; (4) sinon → Ordre.

---
## 8. Succession des édifices + hystérésis
`edifice_succ()` (linéaire aujourd'hui, switch dans `scps_agency.c`) reçoit une variante contextuelle. **Entrepôt et Comptoir restent universels, non forkés** (poses indépendantes dont l'IA dépend déjà).
```c
Edifice edifice_fork_successor(Edifice base, TechPole pole);   /* EDIFICE_COUNT si base non forkée */
Edifice edifice_next_buildable_ctx(const WorldEconomy*, int region, Edifice base);
```
**Hystérésis (requise dès v1)** — sans elle, une région 45/40 oscille entre forks en pleine construction :
- Le pôle dominant doit tenir **≥ 360 jours** avant que les forks le relisent (stocker `last_pole` + `pole_since_day` par région).
- **Un fork bâti ne se reconvertit jamais** : dès qu'un membre d'une famille est dans `edi_built`, les frères sont bloqués pour cette région (extension de `edifice_build_blocked`). Reconversion = démolition (hors v1).
C'est le **coût de réorientation** (et il réalise proprement le coût Ordre de §22.2 — pas « lenteur de recherche », l'Ordre tient la spine Savoir·Prod et cherche vite).

---
## 9. Matrice des bâtiments
### 9.1 Gouvernance
| Martial | Ordre | Fluide |
|---|---|---|
| Conseil de guerre → École de guerre | Chancellerie → Académie | Guilde → Chambre de commerce |
### 9.2 Coercition
| Martial | Ordre | Fluide |
|---|---|---|
| Donjon → Tour de siège | Forteresse → Citadelle | Rempart communal → Enceinte |
Martial tient par garnison et projection ; Ordre par architecture centralisée ; Fluide par défense civique (moins puissante, moins coûteuse).
### 9.3 Maritime
| Martial | Ordre | Fluide |
|---|---|---|
| Arsenal | Amirauté | Port marchand |
### 9.4 Foi
| Martial | Ordre | Fluide |
|---|---|---|
| Temple de guerre → Grand Temple de guerre | Temple → Cathédrale | Sanctuaire des Routes → Grand Sanctuaire des Routes |
### 9.5 Savoir
| Martial | Ordre | Fluide |
|---|---|---|
| Bibliothèque militaire → Collège des Maréchaux | Bibliothèque → Monastère | Observatoire → Université des Routes |
`Université` seule est trop générique ; `Université des Routes` dit la fonction fluide.
### 9.6 Subsistance
| Martial | Ordre | Fluide |
|---|---|---|
| Réserve des Armées → Dépôt de Siège | Grenier → Irrigation → Aqueduc | Moulin → **Marché aux grains** |
(`Marché aux grains` remplace `Entrepôt des Flux` : fonction claire, pas de collision avec l'Entrepôt universel.)
### 9.7 Commerce
| Martial | Ordre | Fluide |
|---|---|---|
| Armurerie de Réserve | Marché → Halle | Comptoir de luxe → Relais commercial |
### 9.8 Finances
| Martial | Ordre | Fluide |
|---|---|---|
| Trésor de butin | Banque | Maison de change |
Cas cité-État fluide (forte autonomie) : **Chambre des Prêts**. Le Martial n'a pas de finance abstraite stable au départ — son équivalent est le butin, la solde, la dette militaire.
### 9.9 Arcane — **couche manufacture (`BLD_*`), pas édifice**
| Martial | Ordre | Fluide |
|---|---|---|
| Forge de Runes → Forge Céleste | Atelier du Mage → Tour d'Ivoire | **Alambic → Grand Alambic** |
Forge Céleste = `BLD_CELESTIAL_FORGE` (existe). Atelier du Mage = `BLD_MAGE_WORKSHOP` (existe). Alambic = **nouveau** `BLD_*`.

---
## 10. Arcane : ressources rares + boucles inter-pôles
| Pôle | Ressource brute | Chaîne | Produit intermédiaire | Apex |
|---|---|---|---|---|
| Martial | Fer céleste | Forge de Runes | — | Forge Céleste (armes enchantées, +flux fort) |
| Ordre | Cristal arcanique | Atelier du Mage | Essence | Tour d'Ivoire (+dK, +flux modéré) |
| Fluide | Salpêtre | Alambic | Essence purifiée | Grand Alambic (+dEco, −flux, alimente l'Alchimiste) |
Chaque chaîne est **auto-contenue** : ressource brute → tier 1 → ressource intermédiaire → tier 2. Pas de dépendance cross-pôle.
**Salpêtre contesté.** Le salpêtre alimente déjà `BLD_POWDERMILL` (salpêtre+charbon→poudre, gate du Foudrier) **et** l'Alambic. Une ressource, deux usages de signe opposé sur le flux : Martial (+flux via poudre), Fluide (−flux via alambic). Marché sous tension, embargos, guerres pour les gisements.

---
## 11. Matrice des unités — double PFC
Chaque unité est définie par trois axes indépendants. Le PFC se joue sur deux dimensions simultanément.
### 11.1 Les trois axes
| Axe | Valeurs | Bat | Perd contre |
|---|---|---|---|
| **Arme** | Distance · Haste · Contact | Distance → Haste · Haste → Contact lourd · Contact → Distance | — |
| **Armure** | Sans · Légère · Lourde | Lourde résiste Contact et Distance légère | Haste perce l'armure |
| **Vitesse** | Rapide · Standard · Lourd · Monté | Rapide flanque le Lourd · Monté écrase Distance avant tir | Haste contre Monté · Lourd absorbe le Rapide |
### 11.2 Axe magique — Destruction / Lien / Corruption
Les trois apex arcanes définissent trois types qui s'insèrent dans le double PFC. Chaque type contre 2/3 de l'arbre.
| Type | Bâtiment source | Unité | Bat | Perd contre |
|---|---|---|---|---|
| **Destruction** | Forge Céleste | Chaman | Contact·Lourde · Haste·Légère (ignore armure) | Rapide · Distance |
| **Lien** | Tour d'Ivoire | Sorcier | Monté · Rapide (annule vitesse) | Contact·Std · Armure lourde |
| **Corruption** | Grand Alambic | Alchimiste | Contact·Lourde·discipliné · Haste martiale (ronge moral) | Distance · Rapide |
### 11.3 Table des unités (Arme × Armure × Vitesse × Pôle)
| Arme | Armure | Vitesse | Martial | Spine | Fluide |
|---|---|---|---|---|---|
| Distance | Sans | Rapide | — | — | Harceleur |
| Distance | Légère | Standard | Foudrier *(gate: poudre)* | Archer | — |
| Distance | Légère | Standard | Arbalétrier lourd | Arbalétrier | Traqueur |
| Haste | Sans | Standard | — | Piquier | Milice |
| Haste | Légère | Standard | Hallebardier | — | — |
| Contact | Légère | Standard | Berserker | Épéiste | Lame Franche |
| Contact | Lourde | Standard | Lancier de choc | Lancier | Garde d'escorte |
| Contact | Lourde | Monté | Cavalerie cuirassée | Cavalerie lourde | — |
| Dist/Contact | Légère | Monté | Cavalerie de raid | Cavalerie légère | Cavalerie légère rapide |
| Destruction | — | Standard | **Chaman** | — | — |
| Lien | — | Standard | — | **Sorcier** | — |
| Corruption | — | Standard | — | — | **Alchimiste** |
### 11.4 Lecture par pôle
**Martial** : armure lourde + haste ou monté lourd + Destruction. Perce, charge, ignore l'armure adverse. Coûts : formation, flux, usure.
**Ordre** : spine standard sur tous les slots + Lien. Contrôle de zone, annule les avantages de mobilité.
**Fluide** : sans/légère + rapide + distance + Corruption. Harcèle, esquive, dégrade le moral à distance.

---
## 12. Doctrines d'unités
Chaque variante = position dans l'espace Arme × Armure × Vitesse + coût caché.
**Martial.** Hallebardier (Haste·Légère·Std — anti-cav, coût formation) · Foudrier (Dist·Légère·Std — armor-pierce, gate poudre) · Arbalétrier lourd (Dist·Légère·Lourd — armor-pierce max, mvt réduit) · Berserker (Cont·Légère·Std — choc élite, moral instable) · Lancier de choc (Cont·Lourde·Std — brise-moral, mvt réduit) · Cavalerie cuirassée (Cont·Lourde·Monté — choc, coût élevé) · Cavalerie de raid (Légère·Monté — pillage, discipline basse) · **Chaman** (Destruction — ignore armure, +flux fort, gate Forge Céleste).
**Ordre.** Stable sur tous les slots. **Sorcier** (Lien — annule vitesse/cavalerie, contrôle de zone, gate Tour d'Ivoire).
**Fluide.** Harceleur (Dist·Sans·Rapide — tir en retraite, fragile en ligne) · Traqueur (Dist·Légère·Std — poursuite) · Milice (Haste·Sans·Std — défense civique, mauvaise en campagne) · Lame Franche (Cont·Légère·Std — mercenaire, coût or/tick) · Garde d'escorte (Cont·Lourde·Std — commandement, faible choc) · Cavalerie légère rapide (Légère·Monté — mobilité max, moral réduit) · **Alchimiste** (Corruption — ronge moral et discipline, −flux, gate Grand Alambic + salpêtre).

---
## 13. Tags culturels
≤ 10 tags en v1 (au-delà, l'équilibrage devient opaque). Même idiome que les tags d'Element Dungeon.
```c
#define TAG_MARITIME   (1u<<0)  /* naval, routes mer */
#define TAG_COMMERCE   (1u<<1)  /* réseaux d'échange */
#define TAG_MARTIAL    (1u<<2)  /* doctrine militaire */
#define TAG_SAVOIR     (1u<<3)  /* transfert de connaissance */
#define TAG_FOI        (1u<<4)  /* influence religieuse */
#define TAG_MOBILE     (1u<<5)  /* mobilité / nomadisme */
#define TAG_REGISTRE   (1u<<6)  /* administration / bureaucratie */
#define TAG_ARCANE     (1u<<7)  /* flux magique */
#define TAG_FORTIFIE   (1u<<8)  /* posture défensive */
#define TAG_RAVITAIL   (1u<<9)  /* logistique / ravitaillement */
```

---
## 14. EDI_FORK_TAGS
```c
typedef struct { uint32_t tags; float dK, dL, dEco, dMil, dH, flux; } EdificeForkDef;
```
```c
/* GOUVERNANCE */
FORK_CONSEIL_GUERRE  { TAG_MARTIAL|TAG_REGISTRE, .dMil=1.2f, .dH=0.4f,  .dK=0.3f }
FORK_CHANCELLERIE    { TAG_REGISTRE,             .dK=1.5f,   .dL=0.8f             }
FORK_GUILDE          { TAG_COMMERCE|TAG_REGISTRE,.dK=0.8f,   .dEco=1.0f           }
/* COERCITION */
FORK_DONJON          { TAG_MARTIAL|TAG_FORTIFIE, .dMil=0.8f, .dH=1.0f             }
FORK_FORTERESSE      { TAG_FORTIFIE,             .dH=2.0f                          }
FORK_REMPART         { TAG_FORTIFIE,             .dH=0.5f,   .dL=0.5f             }
/* MARITIME */
FORK_ARSENAL         { TAG_MARITIME|TAG_MARTIAL, .dMil=2.0f                        }
FORK_AMIRAUTE        { TAG_MARITIME|TAG_REGISTRE,.dMil=0.8f, .dK=0.5f             }
FORK_PORT_MARCHAND   { TAG_MARITIME|TAG_COMMERCE,.dEco=1.5f                        }
/* FOI */
FORK_TEMPLE_GUERRE   { TAG_FOI|TAG_MARTIAL,      .dL=0.8f,   .dMil=0.6f           }
FORK_TEMPLE          { TAG_FOI,                  .dL=1.5f                          }
FORK_SANCT_ROUTES    { TAG_FOI|TAG_COMMERCE,     .dL=0.8f,   .dEco=0.6f           }
/* SAVOIR */
FORK_BIBLIO_MIL      { TAG_SAVOIR|TAG_MARTIAL,   .dMil=0.8f, .dK=0.5f             }
FORK_BIBLIOTHEQUE    { TAG_SAVOIR,               .dK=1.2f                          }
FORK_OBSERVATOIRE    { TAG_SAVOIR,               .dK=1.0f,   .flux=-0.2f           }
/* SUBSISTANCE */
FORK_RESERVE_ARMEES  { TAG_RAVITAIL|TAG_MARTIAL, .dMil=0.5f                        }
FORK_GRENIER         { TAG_RAVITAIL,             /* food_cap géré par l'éco */      }
FORK_MARCHE_GRAINS   { TAG_RAVITAIL|TAG_COMMERCE,.dEco=0.4f                        }
/* ARCANE (manufacture) */
FORK_FORGE_RUNES     { TAG_ARCANE|TAG_MARTIAL,   .dMil=1.5f, .flux=1.2f            }
FORK_ATELIER_MAGE    { TAG_ARCANE|TAG_SAVOIR,    .dK=0.3f,   .flux=0.8f            }
FORK_ALAMBIC         { TAG_ARCANE|TAG_COMMERCE,  .dEco=0.5f, .flux=-0.3f           }
```

---
## 15. Émergence culturelle : cristallisation
Deux cultures au contact peuvent produire une solution inaccessible à chacune seule. Cela arrive par voisinage, route marchande, guerre, dépendance économique, migration d'artisans — pas par décision abstraite.

---
## 16. Deux moteurs
### 16.1 Composants → alliage
Deux profils apportent des fonctions **différentes**. Assemblage inédit.
| Contact | Produit |
|---|---|
| Maritime + minière | Chantier naval de haute mer |
| Pastorale + bureaucratique | Corps de cavalerie organisé |
| Arcane + logistique | Alchimie de campagne |
Ni les marins sans métal, ni les mineurs sans débouché maritime.
### 16.2 Fusion de formes
Deux profils répondent à la **même** fonction par des formes différentes. Forme syncrétique.
| Contact | Produit |
|---|---|
| Cercle de pierres + Temple | Précinct aligné |
| Foi itinérante + commerce | Temple-comptoir |
| Savoir martial + tradition savante | Codification de doctrine |
Même besoin, deux réponses, troisième forme.

---
## 17. Règle de sélection
```c
if      (functions_are_different(a,b))     engine = CRYSTAL_COMPONENT_ALLOY;
else if (same_function_diff_form(a,b))     engine = CRYSTAL_FORM_FUSION;
else                                       engine = CRYSTAL_NONE;
```
Contact trop faible → rien. Trop proche → assimilation molle. Trop distant → fracture.

---
## 18. Distance & qualité — **réutilise le moteur, ne le réinvente pas**
La cloche existe : `scps_bell(D̄) = D̄(10−D̄)/25` (parabole, **nulle aux extrêmes** : rien à échanger à 0, rien de métabolisable à 10). Ne pas écrire de gaussienne — elle n'est nulle nulle part et autoriserait une cristallisation entre jumeaux, ce que §17 interdit.
| D̄ | Effet |
|---|---|
| ≈ 0 | Même monde mental, pas d'innovation (bell = 0) |
| ≈ 5 | Altérité féconde (bell = 1.0) |
| → 10 | Incompréhension, fracture (bell = 0) |
Le gate réaction/fracture est aussi déjà dans le moteur — `scps_metabolisation(P, D∞, K) = σ(0.8(P−D∞)+0.35(K−5))` :
```c
qualite_cristallisation = scps_bell(D_bar) * scps_metabolisation(P, D_inf, K);
```
`P` = **perméabilité** (coût de défection des élites), `D∞` = distance irréductible (max sur les 4 axes de contenu, langue exclue). **Les deux fonctions qui calculent la PE calculent la cristallisation** : la prospérité de contact et l'émergence de contact sont le même phénomène.

---
## 19. Paires de tags déclenchantes
```c
TAG_MOBILE   + TAG_REGISTRE  -> Corps de cavalerie organisé   /* M1 */
TAG_MARITIME + TAG_SAVOIR    -> Distillerie portuaire          /* M1 */
TAG_FOI      + TAG_COMMERCE  -> Temple-comptoir                /* M2 */
TAG_MARTIAL  + TAG_SAVOIR    -> Codification de doctrine       /* M2 */
TAG_ARCANE   + TAG_RAVITAIL  -> Alchimie de campagne           /* M1 */
TAG_MARITIME + TAG_MARTIAL   -> Doctrine d'abordage            /* M1 */
TAG_COMMERCE + TAG_REGISTRE  -> Comptabilité de réseau         /* M2 */
TAG_FORTIFIE + TAG_SAVOIR    -> Architecture bastionnée        /* M1 */
```
Chaque paire = un **template** ; l'instance (nom, stats, coût, lieu, produit) est générée depuis les deux cultures en contact.

---
## 20. Profondeur de contact — **étendre `SyncNode`, pas dupliquer**
`Profondeur {PROF_SURFACE..PROF_SECRET}`, `arch_depth[]` et `tech_sync_tick` (latch permanent, idempotent) existent déjà (`scps_tech`). La cristallisation v-next **étend le prédicat des `SyncNode`** (paire de tags au lieu d'archétype-source unique). Même latch, mêmes bancs.
| Canal | Profondeur | Diffuse |
|---|---|---|
| Comptoir / route | Surface | Bâtiments commerciaux, biens finis, rumeurs |
| Frontière / guerre / foi | Métier | Techniques militaires, rites, doctrine |
| Gouvernance digérée | Profond | Institutions, droit, fiscalité |
| Archétype intime | Secret | Arcane, savoir interdit, mythes |
Une route marchande crée un `Temple-comptoir`, pas une réforme du droit impérial.

---
## 21. Trajectoires SCPS par pôle
| Pôle | Nœuds favoris | Deltas | Trajectoire |
|---|---|---|---|
| **Martial** | ARMÉE tous thèmes, faustien | +dMil, +dH, +flux, −dK | Brèche rapide. `dereal` explose tôt. |
| **Ordre** | RENFORT Société, PROD Savoir | +dK, +dL, +dF | Tall, durable. Absorbe le flux. |
| **Fluide** | PROD tous thèmes | +dEco, +PE, −dH, −flux | Riche, mobile, vulnérable militairement. |

---
## 22. Anti-dominance
**22.1 Martial** — gagner vite, pas durer gratuitement : hausse du flux, usure démographique, dépendance au butin, diplomatie dégradée, frontières radicalisées, retard de métabolisation.
**22.2 Ordre** — durer, pas tout absorber sans prix : **coût de réorientation** (cf. §8), coût administratif, inertie aux chocs, réformes difficiles, élites conservatrices, faible adaptation aux routes nouvelles. (Pas « lenteur de recherche » : l'Ordre tient la spine Savoir·Prod.)
**22.3 Fluide** — enrichir, pas perdre les guerres. *Capacités* : acheter mercenaires/paix, créer des dépendances, déplacer capital/flux/trésor, **vendre la stabilité** (essence purifiée puits-de-flux, cf. §10), corrompre, reconstruire vite. *Coûts* : H bas, loyauté conditionnelle, influence privée, vulnérabilité aux blocus, difficulté à lever du lourd.

---
## 23. v1 : périmètre
### 23.1 Prototype vertical — trois familles, **deux couches**
1. **Maritime** (édifice) : Arsenal / Amirauté / Port marchand → teste `edifice_succ_ctx` + hystérésis.
2. **Savoir** (édifice) : Bibliothèque militaire / Bibliothèque / Observatoire → teste le routing + la spine universelle.
3. **Arcane** (**manufacture**) : Forge Céleste / Atelier du Mage / Alambic → teste le gate ressource+tech et le puits-de-flux. **Ne passe pas par `edifice_succ_ctx`.**
### 23.2 Contenu v1 strict
1. `ETHOS_FN[ETHOS][FN]` (`scps_ai`).
2. Lecture du pôle depuis factions (`region_pole`).
3. `edifice_succ_ctx()` + hystérésis (360 j, fork non reconvertible).
4. Scoring tech IA remplaçant le biais plat de thème.
5. +5 édifices forkés (Arsenal, Amirauté, Port marchand, Bibliothèque militaire, Observatoire) — Monastère réutilisé pour Ordre·Savoir.
6. +1 manufacture (Alambic) + bien « essence purifiée » + sa consommation puits-de-flux.
7. Tests déterministes Martial / Ordre / Fluide.
### 23.3 Hors v1
Cristallisation procédurale complète · rétroaction tech → valeurs · navires forkés · manufactures forkées (au-delà de l'Alambic) · syncrétisme apatride · noms générés · > 10 tags · migration `uint64_t` du masque `edi_built` (prérequis de la matrice complète, pas du proto).

---
## 24. Tests minimaux
```c
/* pôle -> fork maritime */
assert(edifice_succ_ctx(EDI_PORT, region_martial) == EDI_ARSENAL);
assert(edifice_succ_ctx(EDI_PORT, region_ordre)   == EDI_AMIRAUTE);
assert(edifice_succ_ctx(EDI_PORT, region_fluide)  == EDI_PORT_MARCHAND);
/* spine accessible à tous */
assert(can_build(EDI_BIBLIOTHEQUE, civ_martial));
assert(can_build(EDI_BIBLIOTHEQUE, civ_ordre));
assert(can_build(EDI_BIBLIOTHEQUE, civ_fluide));
/* gate ressource (manufacture) */
assert( can_build_bld(BLD_CELESTIAL_FORGE, has_fer_celeste));
assert(!can_build_bld(BLD_CELESTIAL_FORGE, no_fer_celeste));
/* IA : éthos -> fonction (argmax ETHOS_FN) */
assert(ai_prefers_func(civ_dominateur,  FN_ARMEE));
assert(ai_prefers_func(civ_bureaucrate, FN_RENFORCEMENT));
assert(ai_prefers_func(civ_mercantile,  FN_PRODUCTION));
/* flux arcane + puits */
assert(flux_delta(BLD_CELESTIAL_FORGE) > flux_delta(BLD_MAGE_WORKSHOP));
assert(flux_delta(BLD_ALAMBIC) < 0.0f);
assert(arcane_charge_after_consume_purified() < arcane_charge_before());
/* hystérésis : flip avant 360 j ne relink pas ; fork bâti persiste */
assert(edifice_succ_ctx_held(EDI_PORT, region_flip_200j) == fork_avant_flip);
assert(fork_built_survives_pole_flip());
```

---
## 25. Chronique
Chaque fork produit une phrase de causalité.
- « Les maîtres de guerre de {REGION} transforment les quais en Arsenal. »
- « La Chancellerie de {CAPITALE} impose une doctrine maritime commune : l'Amirauté naît. »
- « Les marchands de {PORT} obtiennent privilèges et entrepôts : le Port marchand devient le cœur de la cité. »
- « Le Fer céleste est confié aux forgerons-runiers. La victoire se rapproche ; le réel, moins stable. »
- « Le Salpêtre distillé permet de réduire les accidents de flux, mais les guildes réclament leur part. »
```c
chronicle_tech_fork(civ, region, fork_id, cause_flags);
/* CAUSE_FACTION_DOMINANCE | CAUSE_RESOURCE_RARE | CAUSE_BORDER_PRESSURE
   | CAUSE_TRADE_CONTACT | CAUSE_FAUSTIAN_CREDO | CAUSE_WAR_EMERGENCY */
```
Le joueur lit la causalité, pas seulement le résultat.

---
## 26. Règles de nommage
Fonctionnel · Polarisé · Court (2–4 mots). Si le nom ne dit pas ce que ça fait, renommer.
Forts : Arsenal, Amirauté, Port marchand, Conseil de guerre, Chambre de commerce, Temple-comptoir, Forge de Runes, Alambic. À surveiller : Marché aux grains (ok), Grand Alambic (ambiance > clarté, acceptable car apparié à l'Alchimiste).

---
## 27. Résumé opérationnel
- Souche → inertie matérielle (thème).
- Éthos → préférence fonctionnelle (`ETHOS_FN`).
- Credo → tolérance au risque faustien.
- Ressource → profondeur d'accès.
- Contact → émergence (cristallisation, hors v1).
