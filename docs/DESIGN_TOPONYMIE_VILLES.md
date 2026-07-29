# DESIGN — Toponymie des villes

> Statut : **PROPOSITION — aucune implémentation**
>
> Objet : remplacer les noms génériques ou artificiels des villes par des
> toponymes déterministes, lisibles et cohérents avec leur localisation, leur
> culture fondatrice et leur contexte historique réellement observable.

## 1. Décisions de périmètre

- Une **ville** possède un nom distinct de celui de sa province et de sa région.
- Dans le modèle actuel, la ville est le bourg d'une région, situé dans sa
  province représentative. Son identité doit donc appartenir à la région ; la
  province représentative reste son ancrage géographique.
- Le nom est créé **une seule fois**, à la genèse ou à la fondation de la ville.
- Une conquête ou un changement d'éthos ne régénère jamais automatiquement le
  nom.
- La localisation fournit le sens principal.
- L'éthos influence le vocabulaire, sans transformer chaque nom en slogan.
- Un marqueur historique n'est autorisé que si le moteur possède déjà l'état
  qui le prouve.
- Les ressources ne participent **jamais** au nom.
- Aucun exonyme, dynastie, saint, fondateur ou historique supplémentaire n'est
  introduit dans cette première version.

## 2. État actuel à remplacer

Le moteur contient plusieurs systèmes sans source commune :

- `gen_province_names` attribue encore `Prov.N` aux provinces ;
- `gen_region_names` produit des descriptions comme « Bois Doré » et quatre
  anciennes variantes de langue ;
- `place_make_name` assemble une racine, un suffixe et une terminaison pour les
  noms de pays et de peuples ;
- `geo_names.gd` nomme les fleuves, forêts, lacs et massifs uniquement pour
  l'affichage ;
- le bandeau de ville récupère actuellement le nom de région via
  `province_info`.

Le nouveau système ne doit pas remplacer un syllabaire par un syllabaire plus
grand. Il doit produire un sens vérifiable, puis traduire ce sens en toponyme.

## 3. Entrées déjà détectables

Le générateur peut travailler sans nouvel état historique.

### Géographie

- biome dominant ;
- latitude et altitude ;
- côte ;
- aptitude portuaire de la région ;
- estuaire ;
- fleuve et débit présents dans les cellules ;
- lac ;
- forme insulaire calculable depuis les cellules ;
- frontière avec un autre propriétaire ;
- province représentative de la région.

### Société et politique

- culture dominante et `culture_id` ;
- éthos dominant ;
- rôle politique : empire, cité-État ou peuple libre ;
- capitale ;
- colonisation et ferveur fondatrice ;
- édifices réellement construits ;
- cicatrices et reconstruction déjà présentes dans l'économie provinciale.

### Entrées explicitement ignorées

- ressources brutes ;
- stocks et prix ;
- manufactures ;
- richesse présente ;
- puissance militaire abstraite ;
- noms ou faits historiques qui ne sont pas stockés.

## 4. Principe : sens, novlang, nom

La génération suit trois étapes :

1. choisir une **clé sémantique de localisation** ;
2. ajouter éventuellement une **clé d'éthos** et un **marqueur historique
   prouvé** ;
3. traduire les clés en morphèmes selon le jitter de novlang de la culture,
   puis lisser leur jonction.

Exemple :

```text
LOCALISATION = ESTUAIRE
ÉTHOS        = MERCANTILE
HISTOIRE     = FONDATION_RÉCENTE

sens         = « nouveau marché de l'embouchure »
morphèmes    = NOVA + MERC + AVRE
nom lissé    = Novamercavre ou, sur une forme courte, Novavre
```

La traduction sert à contrôler la cohérence du nom. Elle n'est pas une histoire
inventée.

## 5. Jitter de novlang

Chaque sens possède plusieurs réalisations phonétiques :

| Sens | Variantes | Traduction |
|---|---|---|
| Nouveau | Novi-, Nova-, Novo-, Nouv-, Neuv-, Neu-, Nav- | nouvelle fondation |
| Libre | Fran-, Franc-, Frey-, Frei-, Liv-, Lib- | indépendant, libre |
| Fort | Cast-, Castr-, Kast-, Car-, Ker-, Gard- | fortification |
| Marché | Marc-, Merc-, Mark-, March-, Merg- | marché, échange |
| Cour | Cour-, Cur-, Kor-, Kar-, Caur- | siège administratif |
| Refuge | Hav-, Haf-, Hal-, Av-, Eir- | havre, refuge |
| Ordre | Reg-, Rig-, Rek-, Cad-, Kat- | règle, organisation |
| Honneur | Bren-, Bran-, Brann-, Fran-, Ser- | renommée, serment |
| Frontière | Marc-, March-, Mark-, Gard-, Ward- | marche frontalière |
| Grandeur | Grand-, Gran-, Gra-, Haut-, Alt- | centre majeur |
| Sacré | Sanct-, Sant-, Sain-, Sen-, Sacr- | lieu religieux |
| Savoir | Sav-, Sap-, Vig-, Veil-, Clair- | savoir, observation |

Le jitter n'est pas un tirage indépendant à chaque ville.

```text
forme de culture = hash(culture_id, clé sémantique)
variante locale  = hash(province_id, clé sémantique)

80 % : forme principale de la culture
20 % : forme locale de la même famille phonétique
```

Ainsi, une culture peut employer principalement `Nova-` :

- Novavre ;
- Novabrive ;
- Novamont.

Une autre culture exprimera le même sens avec :

- Neuvavre ;
- Neubriv ;
- Neumont.

Le sens reste toujours « nouvelle fondation ». La culture conquérante ne
retraduit pas le nom.

## 6. Lexique géographique

La forme exacte est choisie par le jitter de novlang. Une culture conserve ses
choix dominants afin que ses villes paraissent apparentées.

| Condition détectable | Préfixes | Suffixes | Mots autonomes | Traduction |
|---|---|---|---|---|
| Estuaire | Aber-, Abar-, Avr-, Aver-, Embr- | -avre, -aber, -mund | Embouchure, Havre | embouchure maritime |
| Fleuve important | Av-, Ava-, Avar-, Evr-, Nant- | -rive, -avon, -nant | Rive, Eaux | rivière ou vallée fluviale |
| Gué ou passage étroit | Briv-, Brev-, Rit-, Brod- | -brive, -furt, -brod | Gué, Pont | passage d'eau |
| Côte et rade favorable | Mor-, Mar-, Cal-, Kal-, Vik- | -mer, -cale, -vik | Anse, Havre, Cap | baie ou rade naturelle |
| Port effectivement construit | Port-, Por-, Haf- | -port, -haven, -hafen | Port, Quai | port aménagé |
| Rive d'un lac | Lim-, Limn-, Lann-, Len-, Mer- | -lac, -mere, -lann | Lac, Eaux | établissement lacustre |
| Île | Ins-, Inis-, Yn-, Nis-, Holm- | -île, -nis, -holm | Île | établissement insulaire |
| Montagne ou sommet | Mont-, Mon-, Mund-, Tor-, Dorn- | -mont, -tor, -berg | Mont, Pic, Crête | hauteur majeure |
| Collines | Dun-, Don-, Doun-, Bel-, Ben- | -bel, -dun, -col | Coteau, Tertre | hauteur habitée |
| Vallée | Val-, Vau-, Wal-, Dol-, Nant- | -val, -combe, -dol | Val, Combe | vallée |
| Col | Col-, Pas-, Porth- | -col, -passe, -porte | Col, Porte | passage montagneux |
| Forêt ou bois | Silv-, Selv-, Syl-, Vern-, Wald- | -sylve, -bois, -wald | Bois, Forêt | couvert forestier |
| Clairière habitable | Sart-, Sarth-, Ess-, Ros-, Rod- | -sart, -clair, -rode | Essart, Clairière | terre défrichée |
| Marais, tourbière ou mangrove | Pal-, Pall-, Fagn-, Vagn-, Marn- | -pal, -fagne, -marais | Fagne, Marais | terre humide |
| Plaine ou terre agricole | Camp-, Champ-, Kam-, Prat-, Feld- | -champ, -pré, -feld | Champ, Plaine | terre ouverte fertile |
| Steppe, savane ou terre sèche | Land-, Lann-, Causs-, Daur-, Step- | -lande, -causse, -step | Lande, Plateau | terre ouverte sèche |
| Désert | Sab-, Sabr-, Aren-, Aran-, Erg- | -sable, -dune, -erg | Dune, Erg | désert |
| Climat froid ou glacier | Nev-, Nevr-, Gel-, Ghel-, Giv- | -neige, -gel, -givre | Névé, Gel | froid permanent |
| Volcan | Cendr-, Sendr-, Pyr-, Pir-, Sulf- | -cendre, -feu, -puy | Cendre, Puy | volcanisme |

## 7. Lexique d'éthos

L'éthos ne remplace jamais la localisation. Son morphème n'intervient
normalement que dans environ un nom sur quatre. Son poids augmente si un état
local le confirme.

| Éthos | Préfixes | Suffixes | Mots autonomes | Traduction |
|---|---|---|---|---|
| Dominateur | Cast-, Bast-, Gard-, Haut- | -fort, -castel, -garde, -mur | Fort, Bastion, Citadelle | puissance et fortification |
| Honneur | Fran-, Bren-, Bran-, Ser- | -halle, -brande, -franc, -ser | Halle, Serment, Bannière | assemblée et renommée |
| Ordre | Reg-, Cad-, Clos-, Met- | -clos, -enceinte, -rang, -met | Clos, Place, Enceinte | règle et mesure |
| Bureaucrate | Cour-, Cur-, Sig-, Chan- | -court, -chanc, -siège, -greffe | Cour, Siège, Chancellerie | domaine et administration |
| Mercantile | Marc-, Merc-, Bors-, Carr- | -march, -vic, -port, -foire | Marché, Comptoir, Quai | commerce et circulation |
| Pacifiste | Hav-, Hal-, Eir-, Len- | -havre, -repos, -paix, -accord | Havre, Refuge, Concorde | refuge et conciliation |

### Renforcements conditionnels

```text
IF éthos = Dominateur AND ville frontalière
    renforcer FORT, GARDE et BASTION

IF éthos = Dominateur
   AND EDI_GARNISON ou EDI_FORTERESSE ou EDI_CITADELLE
    autoriser Cast-, -fort et -castel à poids fort

IF éthos = Honneur AND rôle = POLITY_WILD
    renforcer HALLE, FRANC et SERMENT

IF éthos = Ordre
   AND EDI_GARNISON ou EDI_TRIBUNAL
    renforcer CLOS, ENCEINTE et REG-

IF éthos = Bureaucrate AND is_capital
    renforcer COUR-, -court et SIÈGE

IF éthos = Bureaucrate
   AND EDI_CHANCELLERIE ou EDI_TRIBUNAL
    autoriser Chan-, Sig- et -chanc

IF éthos = Mercantile AND estuary
    renforcer les compositions MARC + AVRE et MERC + ABER

IF éthos = Mercantile
   AND EDI_MARCHE ou EDI_COMPTOIR ou EDI_TRADE_CENTER
    autoriser MARCHÉ, COMPTOIR, -march et -vic

IF éthos = Pacifiste
   AND EDI_SANCTUAIRE ou EDI_TEMPLE ou EDI_CATHEDRALE
    renforcer HAVRE, REFUGE et EIR-

IF éthos = Pacifiste AND aptitude portuaire élevée
    renforcer Hav-, Haf- et -havre
```

## 8. Marqueurs historiques autorisés

Ces marqueurs sont tirés uniquement au moment où le nom est attribué.

| État existant | Morphèmes autorisés | Traduction |
|---|---|---|
| `ferveur > 0.5` | Novi-, Nova-, Novo-, Nouv-, Neuv-, Neu-, Nav-, -neuve | fondation récente |
| `is_capital` | Grand-, Gran-, Haut-, Alt-, Cour-, -cour, -siège | siège politique réel |
| `POLITY_CITY_STATE` | Fran-, Franc-, Frei-, Libre-, -franche | cité indépendante |
| `POLITY_WILD` | Libre-, Fran-, Foyer-, -franc | établissement d'un peuple libre |
| Frontière avec un autre propriétaire | Marc-, March-, Gard-, Ward-, -marche, -garde | marche politique réelle |
| Garnison, forteresse ou citadelle construite | Fort-, Cast-, Castel-, Garde-, -fort | centre fortifié réel |
| Marché, comptoir ou centre commercial construit | Marc-, Merc-, Bors-, -march, -vic | centre commercial réel |
| Port ou port marchand construit | Port-, Por-, Hav-, -port | port réel |
| Sanctuaire, temple ou cathédrale construite | Sanct-, Sant-, Sacr-, -temple | centre religieux réel |
| Académie, bibliothèque, monastère ou observatoire construit | Sav-, Sap-, Vig-, Clair-, -veille | centre de savoir réel |
| `reconstruction > 0.5` lors de l'attribution initiale d'un nom manquant | Re-, Neuve-, -rebâtie | reconstruction effective |

`reconstruction` ne renomme jamais une ville déjà nommée. Cette condition ne
vaut que pour une attribution initiale, par exemple lors de la migration d'une
ancienne sauvegarde dépourvue de noms de villes.

## 9. Marqueurs interdits

Les états suivants seraient des inventions et ne doivent pas apparaître :

- `Vieux-`, `Ancien-` : l'âge de la ville n'est pas suivi ;
- `Saint-X` : aucun saint ni personnage canonisé n'est stocké ;
- `Roi-X`, `Reine-X`, `Maison-X` : aucune dynastie fondatrice n'est disponible ;
- `Victoire-`, `Conquête-`, `Reconquise-` : aucune bataille n'est reliée à la
  fondation ;
- `Ruines-`, `Cité perdue-` : aucun historique de ruine n'est conservé ;
- `Première-`, `Originelle-` : l'ordre historique des fondations n'est pas
  disponible ;
- `Rouge-`, `Martyre-`, `Sanglante-` depuis `revolt_scar` : une cicatrice
  récente ne prouve pas l'origine d'un nom ;
- `Banqueroute-`, `Pillée-`, `Balafrée-` : ces états sont transitoires et ne
  doivent pas devenir des noms permanents.

## 10. Sélection de la localisation

```text
IF province non colonisée
    aucun nom de ville

IF estuary
    localisation = ESTUAIRE
ELSE IF EDI_PORT construit
    localisation = PORT
ELSE IF fleuve important
    localisation = FLEUVE ou GUÉ
ELSE IF rive de lac
    localisation = LAC
ELSE IF coastal AND aptitude portuaire élevée
    localisation = RADE
ELSE IF île
    localisation = ÎLE
ELSE IF montagne, sommet ou hautes terres
    localisation = MONTAGNE ou COL
ELSE IF collines
    localisation = COLLINE
ELSE IF forêt
    localisation = FORÊT ou CLAIRIÈRE
ELSE IF marais
    localisation = MARAIS
ELSE IF plaine ou terre agricole
    localisation = PLAINE
ELSE IF steppe, savane ou terre sèche
    localisation = LANDE
ELSE IF désert
    localisation = DÉSERT
ELSE IF climat froid
    localisation = FROID
ELSE IF volcan
    localisation = VOLCAN
```

Ordre de priorité général :

```text
embouchure
> port construit
> fleuve ou lac
> rade ou île
> relief
> biome
```

## 11. Assemblage

Chaque ville reçoit :

```text
1 racine de localisation obligatoire
+ 0 ou 1 morphème d'éthos
+ 0 ou 1 marqueur historique valide
```

Gabarits autorisés :

```text
[racine de lieu] + [suffixe d'établissement]
[préfixe d'éthos] + [racine de lieu]
[marqueur historique] + [racine de lieu]
[mot autonome] de [racine de lieu]
[racine] sur [nom de fleuve]       seulement pour désambiguïser
```

Contraintes :

- deux à quatre syllabes ;
- un seul sens principal ;
- pas plus d'un marqueur historique ;
- pas plus d'un morphème d'éthos ;
- aucun empilement de synonymes ;
- aucune référence à une ressource ;
- le résultat doit tenir dans la capacité de stockage retenue pour le nom de
  ville.

Combinaisons interdites :

```text
Port + Havre
Mont + Berg
Fort + Castel
Neuve + Nouvelle
Marc + Marché
Cour + Siège
```

## 12. Lissage phonétique

Le lissage est déterministe :

```text
Nova + Avre     -> Novavre
Merc + Avre     -> Mercavre
Cast + Brive    -> Casbrive
Reg + Sart      -> Ressart
Cour + Nant     -> Cournant
Hav + Mor       -> Halmor ou Havmor selon la novlang
```

Règles :

- fusionner deux voyelles identiques ;
- retirer la dernière consonne d'un préfixe devant un groupe imprononçable ;
- autoriser une assimilation locale (`gm -> mm`, `gs -> ss`, `dt -> t`) ;
- éviter plus de deux consonnes consécutives ;
- conserver l'accentuation et les graphèmes propres à la novlang choisie ;
- ne jamais relancer un tirage après sauvegarde.

## 13. Exemples de sortie

| Conditions | Nom | Traduction |
|---|---|---|
| Mercantile + estuaire + marché | Mercavre | marché de l'embouchure |
| Dominateur + gué + forteresse | Casbrive | fort du gué |
| Bureaucrate + vallée + capitale | Courval | siège administratif de la vallée |
| Pacifiste + rade naturelle | Halmor | refuge maritime |
| Honneur + colline + peuple libre | Frandun | établissement libre de la colline |
| Ordre + clairière agricole | Ressart | essart ordonné |
| Colonie récente + fleuve | Novavre | nouvelle fondation de l'eau |
| Colonie récente d'une autre novlang + fleuve | Neuvavre | nouvelle fondation de l'eau |
| Cité-État sur un col | Francol | cité indépendante du col |
| Centre commercial côtier | Marcvik | marché de la baie |

## 14. Invariants de comportement

- Deux villes d'une même culture partagent une parenté sonore.
- Deux cultures peuvent traduire le même sens différemment.
- Les petites variations locales ne détruisent pas cette parenté.
- Les doublons mondiaux sont autorisés.
- Un doublon proche reçoit une désambiguïsation géographique, pas un numéro.
- Le nom survit aux conquêtes, changements d'éthos et assimilations.
- Le système n'invente jamais une histoire que le moteur ne peut pas prouver.
- Les ressources et la conjoncture économique restent totalement absentes de la
  toponymie.

