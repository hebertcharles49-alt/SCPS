# Audit du code SCPS — 4 septembre 2026

## Synthèse

**Le socle de tests passe, mais il ne suffit pas à garantir la fiabilité du chargement ni celle des diagnostics.** Trois défauts prioritaires concernent les sauvegardes : des données dangereuses sont acceptées, un chargement refusé peut écraser l'état vivant si la copie de secours échoue, et des ordres de la partie précédente survivent au chargement.

L'audit identifie **14 constats actionnables : 3 P1, 10 P2, 1 P3**. Aucun P0 démontré. Les constats statiques, les reproductions exécutées et les hypothèses d'équilibrage sont distingués ci-dessous. Aucune correction n'a été appliquée.

Les derniers sweeps ont effectivement orienté le diagnostic. Ils ne justifient toutefois pas certaines conclusions prises isolément : **zéro coque est attendu avec le combat naval désactivé ; zéro régiment dans le bilan ne compte que la réserve ; zéro Ascension dans une simulation sans joueur ne prouve pas une fonctionnalité morte.**

### État audité et méthode

- Dépôt : `C:/Users/Charl/Desktop/SCPS-main`.
- Révision : `b4c3155bda8073983a443cc06772bd0477ff5f0e`.
- Sauvegarde courante : version 108.
- Inventaire d'intégrité : **377 fichiers versionnés de code, scripts et construction**, soit **8 337 138 octets**, empreintes SHA-256 relevées avant l'audit.
- Compilation et expériences dans une **copie temporaire isolée**. Les sauvegardes de test sont dans cette copie, pas dans la partie utilisateur.
- Lecture approfondie des chemins sensibles et de leurs appelants ; analyse statique Clang des **45 fichiers de production `scps/scps_*.c`**, hors `*_demo.c` ; exécution des 42 bancs ; contrôles déterminisme, référence, sauvegarde et mémoire.
- Examen de la façade C, du pont C++, de la cadence Godot, du chargement/menu/navigation, du système de rapports utilisateur, du Makefile et de l'unique workflow GitHub.

**Portée exacte :** audit transversal de tout le dépôt, avec approfondissement guidé par les risques et les sweeps ; ce n'est ni une certification d'absence de défaut ni une affirmation que chaque ligne des 377 fichiers a reçu une relecture manuelle exhaustive. Les dépendances tierces, tous les shaders et tous les écrans n'ont pas été audités ligne par ligne. Aucun test graphique interactif, export Godot, audit CVE externe ou nouveau sweep de 250 ans n'a été réalisé.

Priorités : P1 = à traiter avant de considérer le comportement concerné fiable ; P2 = défaut fonctionnel, robustesse ou qualité de preuve ; P3 = diagnostic secondaire. La priorité tient compte du scénario d'activation, pas seulement de la gravité théorique.

## 1. Derniers sweeps : ce qui a été réellement lu

Lecture **séquentielle, sans filtrage par mots-clés**, des documents et journaux suivants :

| Document | Couverture |
|---|---|
| [Validation W1/W2 du 04/09](C:/Users/Charl/Desktop/SCPS-main/docs/SWEEP_VALID_W1W2_2026-09-04.md) | Rapport intégral |
| [Sweep doctrines du 02/09](C:/Users/Charl/Desktop/SCPS-main/docs/SWEEP_DOCT_AI_2026-09-02.md) | Rapport intégral |
| [Rapport des correctifs W1/W2](C:/Users/Charl/Desktop/SCPS-main/docs/RAPPORT_CORRECTIFS_SWEEP_2026-09-04.md) | Rapport intégral |
| Manifestes des deux sweeps et résumé du sweep doctrines | Intégraux |
| [W1/W2, essai graine 11, 250 ans](C:/Users/Charl/Desktop/SCPS-main/sweep_valid_W1W2_50x250/essai_s11_y250.log) | **877 lignes, intégral**, inventaire provincial inclus |
| [Doctrines, essai graine 1009, 200 ans](C:/Users/Charl/Desktop/SCPS-main/sweep_doct_ai_10x200/essai_s1009_y200.log) | **637 lignes, intégral**, inventaire provincial inclus |

Les autres journaux bruts ne sont **pas présentés comme intégralement relus**. Leurs statistiques agrégées sont celles des rapports existants, non un nouveau dépouillement indépendant de chaque fichier. Les recherches textuelles dans les sources ont servi à retrouver les producteurs et consommateurs des données ; elles n'ont pas remplacé cette lecture des journaux.

### 1.1 Provenance et complétude

Le dernier sweep annonçait 50 graines × 2 bras × 250 ans, soit 100 exécutions. Il est interrompu : **27 exécutions complètes**, représentant **13 paires complètes et un témoin supplémentaire**, 8 fichiers de retour à 127, et 8 journaux sans fichier de retour. Ces derniers ne doivent pas devenir des simulations valides par défaut. Le code 127 indique un échec de lancement/commande dans ce contexte ; ce n'est pas, à lui seul, un crash du moteur.

Le sweep précédent comporte **20 exécutions complètes**, 10 paires de 200 ans. Les binaires diffèrent :

- 02/09 : SHA-256 `8140fa9293f6f00beea889dd9c1a8dabf007f0af4836341c36198c55127925f5`.
- 04/09 : SHA-256 `2bb5301cb496e1bfc64fafa6f1d7ec38cc79ea2da57a0fa633468017b7b95cc0`.

Les comparaisons entre campagnes combinent donc **une modification de code, une durée différente et un sous-ensemble de graines différent**. Les paires témoin/essai à l'intérieur d'un même sweep sont plus pertinentes pour l'effet des doctrines. Il serait injustifié d'attribuer tout écart entre les deux campagnes à une seule correction.

### 1.2 Ce que les journaux changent dans le diagnostic

| Signal observé | Conclusion retenue après lecture du code |
|---|---|
| Sièges/âmes concordants et aucun groupe invalide dans les 27 runs, selon le rapport W1/W2 | Signal positif de cohérence démographique. Ne prouve pas l'égalité locale entre tous les découpages sociaux. |
| 63 Marbrive sur 20/27 runs, selon ce même rapport | L'événement n'est pas globalement mort. Le zéro de la graine 1009 du sweep précédent ne permettait pas de généraliser. |
| Ligue Dhurganyn, graine 11 : environ 378k habitants, 184 577 or, « 0 rgt » | Le compteur est celui de la réserve, **pas réserve + corps déployés** : voir A09. Pas de preuve suffisante d'un empire totalement désarmé. |
| Zéro coque, zéro combat naval, mais traversées et commerce maritime présents | **Configuration attendue** : `NAVY_COMBAT_ON=0`. La construction IA est derrière ce verrou, et le transport est virtuel dans ce mode. |
| Écart négatif important entre variation du trésor et somme des rubriques FX | Instrumentation incomplète ; un débit effectivement absent du registre est identifié en A08. Cela ne démontre pas à lui seul une création/destruction monétaire fautive. |
| Lecteur fiscal affichant peu ou zéro malgré des taxes encaissées | Formules de lecture différentes du prélèvement réel : A07. Le lecteur est déjà provincial ; le diagnostic « il lit encore un ancien trésor régional » ne correspond pas au code actuel. |
| Prix imprimé `0.000` | Valeur arrondie, pas preuve d'un zéro exact. Distinguer prix projeté, indice et facture réellement payée. |
| ASCENSION absente | La Merveille est une voie joueur ; un sweep sans joueur ne la valide pas. |
| RÉCHAUFFEMENT absent alors que le combustible est suffisant | Le réchauffement est un **repli** ; les fins naturelles peuvent prendre la place avant lui. Pas une sélection équiprobable entre toutes les fins. |
| Divin absent, Faustien très rare, doctrines et idées accumulées | Question d'accessibilité/choix IA à approfondir avec le nombre de pays réellement éligibles ; pas preuve suffisante d'un verrou cassé. |
| Provinces « figées », richesse très inégale et fortes concentrations | Signaux d'équilibrage réels dans les rapports, mais aucune cause unique démontrée ici. Ne pas les convertir en correctifs de taux arbitraires. |

Références pour les modes intentionnellement désactivés : [registre naval](C:/Users/Charl/Desktop/SCPS-main/scps/scps_tune_list.h:801), [construction IA et commentaire OFF](C:/Users/Charl/Desktop/SCPS-main/scps/scps_sim.c:1388), [sélection des fins et repli](C:/Users/Charl/Desktop/SCPS-main/scps/scps_endgame.c:1480), [Merveille](C:/Users/Charl/Desktop/SCPS-main/scps/scps_endgame.c:1212).

Deux exemples lus entièrement :

- **Graine 11, 250 ans** : population mondiale d'environ 1,157 million, fin RONCES an 182, 46 traversées, 20 routes maritimes, zéro coque ; le tableau final affiche trois corps en mouvement, sans permettre d'en attribuer ici la propriété à Dhurganyn. Le résidu comptable annuel final est de l'ordre de −2 245,5 or/mois/empire.
- **Graine 1009, 200 ans** : 608k habitants, GRAND HIVER an 180, 52 traversées et 31 colonies outre-mer malgré zéro coque ; 30 scieries navales ; 139 batailles, un seul décrochage dans cette ancienne version ; zéro Marbrive ; 65 régions pour le premier empire. Les écarts FX et le zéro naval étaient donc déjà présents avant W1/W2.

## 2. Constats prioritaires

### A01 — P1 — La validation des sauvegardes laisse passer des index et tailles dangereux

**Preuve : reproduction exécutée + lecteurs identifiés.**

[Validation centrale](C:/Users/Charl/Desktop/SCPS-main/scps/scps_save.c:268), [validation limitée au nombre d'unités](C:/Users/Charl/Desktop/SCPS-main/scps/scps_save.c:510).

`scps_save_sane` borne les tableaux principaux et le nombre de types d'unités, mais ne valide pas notamment le type de chaque unité, le nombre de manufactures d'une province, et les index de meneur/suzerain de la fronde. Des états volontairement invalides passent :

```text
baseline_sane=1
invalid_unit_type_sane=1 type=122
invalid_build_count_sane=1 count=31 max=30
invalid_fronde_sane=1
```

Or ces données sont utilisées directement :

- [armée](C:/Users/Charl/Desktop/SCPS-main/scps/scps_army.c:532) : `UNITS[a->units[i].type]` ;
- [manufactures](C:/Users/Charl/Desktop/SCPS-main/scps/scps_econ.c:5441) : boucle jusqu'à `n_bld`, puis indexation de `RECIPE` ;
- [fronde](C:/Users/Charl/Desktop/SCPS-main/scps/scps_diplo.c:277) : indexation de `status[ld][s0]`.

**Impact :** une sauvegarde structurellement incohérente mais à empreinte correcte peut être acceptée, puis provoquer des accès hors limites ou des comportements indéfinis. Le contrôle d'empreinte ne remplace pas la validation sémantique. Il s'agit d'une surface de chargement de fichier local ; aucune exploitation distante n'est démontrée.

**À faire :** établir la liste des compteurs, enums et index utilisés après chargement, valider leurs bornes et leurs relations avant utilisation. Ajouter un corpus de mutations sémantiques à empreinte recalculée, pas uniquement des altérations d'octets.

**Test attendu :** chaque cas ci-dessus est rejeté sans modifier l'état vivant ; ajouter les mêmes cas aux corps déployés et aux types de manufacture.

### A02 — P1 — Un chargement refusé peut laisser la partie courante corrompue

**Preuve : injection de panne exécutée.**

[Création du snapshot](C:/Users/Charl/Desktop/SCPS-main/scps/scps_save.c:644), [lecture avant garantie de secours](C:/Users/Charl/Desktop/SCPS-main/scps/scps_save.c:650), [restauration conditionnelle](C:/Users/Charl/Desktop/SCPS-main/scps/scps_save.c:657).

Si `tmpfile()`, l'écriture ou la synchronisation du snapshot échoue, `have_snap` est faux. Le chargement écrase malgré tout le monde courant. Si sa validation échoue ensuite, la fonction renvoie une erreur mais ne peut pas restaurer l'ancien monde. Le commentaire promettant un état intact est alors faux.

Reproduction : sauvegarde contenant `n_prov=SCPS_MAX_PROV+1`, puis échec injecté sur le deuxième `tmpfile()` du chargement :

```text
failed_snapshot_load_rc=1
live_n_prov=1665
expected=804
tmp_calls=2
```

**Impact :** l'utilisateur voit « échec de chargement », mais sa partie en mémoire est déjà altérée. La poursuite de la simulation peut devenir dangereuse.

**À faire :** refuser avant toute lecture mutante si le snapshot n'est pas garanti, ou charger dans un état séparé puis publier cet état après validation. Le retour de la restauration doit lui-même être contrôlé.

**Test attendu :** injecter l'échec de chaque étape de sauvegarde/restauration ; comparer l'état et les caches avant/après le refus.

### A03 — P1 — Les ordres en attente survivent au chargement d'une autre partie

**Preuve : reproduction exécutée et chemin de consommation vérifié.**

[Chargement façade](C:/Users/Charl/Desktop/SCPS-main/scps/scps_api.c:5844), [traitement des ordres](C:/Users/Charl/Desktop/SCPS-main/scps/scps_sim.c:512), [déclaration de guerre](C:/Users/Charl/Desktop/SCPS-main/scps/scps_sim.c:593).

Le chargement restaure les modules, mais ne purge pas `Sim.cmd_n/cmdq`. Une commande de la partie A, déposée pendant la pause, reste donc dans l'instance après le chargement de B. Les identifiants numériques sont alors interprétés dans B au prochain pas.

La reproduction dépose une commande de guerre avant le chargement :

```text
player_state_load_rc=0
retained_commands=1
expected=0
```

**Impact :** une action non demandée sur la partie chargée peut être exécutée : guerre, dépense, construction, etc., dès lors que les nouvelles conditions rendent l'ordre valide. L'expérience a démontré la conservation de la commande ; elle n'a pas lancé une guerre dans une partie utilisateur.

**À faire :** définir une frontière claire au chargement réussi : purge des ordres de l'ancienne session et des attentes UI associées. Ne pas purger la partie courante si le chargement échoue.

**Test attendu :** A en pause avec un ordre en attente → charger B → avancer un jour → aucune action héritée de A.

## 3. Défauts fonctionnels, financiers et de mesure

### A04 — P2 — La cible de recherche du joueur n'est pas sauvegardée

**Preuve : reproduction exécutée.**

[État sérialisé](C:/Users/Charl/Desktop/SCPS-main/scps/scps_save.c:29), [écriture du payload](C:/Users/Charl/Desktop/SCPS-main/scps/scps_save.c:115), [cible consommée par la simulation](C:/Users/Charl/Desktop/SCPS-main/scps/scps_sim.c:1219).

`research_target` est conservé dans `Sim`, mais n'est pas dans le payload. Le chargement garde donc la cible déjà présente dans l'instance ; dans une session neuve, elle reste à −1.

```text
target=-1
expected=34
load_rc=0
```

**Impact :** la recherche sélectionnée n'est pas reprise fidèlement ; inversement, la cible d'une autre partie peut subsister. Les points de recherche et les technologies sont distincts de cette sélection : il ne s'agit pas d'affirmer qu'ils sont tous perdus.

**À faire :** sérialiser et valider la sélection, avec politique de compatibilité explicite. Test joueur actif, sauvegarde au milieu d'une recherche, rechargement dans une instance neuve et dans une instance ayant une autre cible.

### A05 — P2 — Lecture d'une variable non initialisée dans l'interception navale

**Preuve : analyse Clang et branche complète relue.**

[Déclaration et branche sans escorte](C:/Users/Charl/Desktop/SCPS-main/scps/scps_navy.c:693), [lecture de `pr`](C:/Users/Charl/Desktop/SCPS-main/scps/scps_navy.c:720).

`pr` est renseigné par `navy_battle` seulement si l'escorte comporte un navire de guerre. Sans escorte, la victoire est affectée directement à +1, puis `pr` décide tout de même d'une prise.

**Impact :** résultat indéfini sur la capture d'un transport, dépendant potentiellement de la pile et de l'optimisation. Menace pour le déterminisme du mode naval.

**Activation :** interception effective d'un convoi non escorté. La configuration par défaut désactive la construction navale IA ; ce défaut n'explique donc pas le zéro naval des sweeps fournis.

**À faire :** définir explicitement le résultat de prise pour la branche sans bataille ; test sans escorte avec transports restants. Une initialisation doit refléter la règle métier choisie, pas une probabilité inventée.

### A06 — P2 — La coercition d'une fronde écrasée est écrite dans un agrégat éphémère

**Preuve : reproduction exécutée.**

[Résolution de fronde](C:/Users/Charl/Desktop/SCPS-main/scps/scps_diplo.c:328), [reconstruction des régions](C:/Users/Charl/Desktop/SCPS-main/scps/scps_econ.c:1558), [coercition issue des provinces](C:/Users/Charl/Desktop/SCPS-main/scps/scps_econ.c:1617).

Le suzerain victorieux ajoute 0,4 à `econ->region[capreg[v]].coercion`. La vérité persistante est provinciale ; l'agrégation suivante remplace cette valeur par celle des provinces.

```text
fronde_coercion_before_aggregate=0.400
after=0.040
```

Le 0,040 restant vient d'un autre effet provincial du scénario de servage ; il ne conserve pas le supplément punitif de 0,4.

**Impact :** une conséquence annoncée de la victoire disparaît au rafraîchissement suivant. Le régime peut être durci sans la coercition persistante attendue.

**À faire :** appliquer l'effet au périmètre provincial voulu, explicitement choisi : province-capitale, provinces de la région ou vassal entier. Test de persistance après agrégation et sauvegarde.

### A07 — P2 — Les lecteurs fiscaux n'utilisent pas la même formule que le prélèvement

**Preuve : comparaison des chemins de calcul ; pas de nouveau sweep fiscal instrumenté.**

[Lecture nationale par classe](C:/Users/Charl/Desktop/SCPS-main/scps/scps_econ.c:2702), [lecture provinciale](C:/Users/Charl/Desktop/SCPS-main/scps/scps_econ.c:2740), [collecte réelle](C:/Users/Charl/Desktop/SCPS-main/scps/scps_econ.c:4585).

Les lecteurs reconstruisent le revenu à partir du PIB et de fractions fixes. La collecte utilise les revenus effectivement distribués pendant le tick, ajoute un plancher per capita, puis applique l'exonération du panier vital et le plafond de richesse.

Les lecteurs omettent le plancher et l'exonération ; le lecteur national par classe n'applique pas non plus le plafond de richesse que le lecteur provincial applique. Ils ne peuvent donc pas tous représenter la taxe réellement encaissée.

**Impact :** divergence entre panneau provincial, ventilation par classe et journal des recettes ; explications contradictoires pour le joueur et pour les audits.

**Correction de diagnostic :** le lecteur national parcourt déjà `e->prov`. Le problème démontré est la **divergence des formules**, pas un simple oubli de migration région → province.

**À faire :** choisir entre « encaissé sur le dernier mois » et « estimation du prochain mois », les nommer distinctement et partager le calcul lorsque pertinent. Tester les cas revenu nul/plancher actif, exonération, richesse insuffisante et impôt legacy.

### A08 — P2 — Un débit de production réel n'alimente pas les rubriques FX

**Preuve : écriture financière identifiée dans le chemin de clôture.**

[Débit de `pending_buy_debit`](C:/Users/Charl/Desktop/SCPS-main/scps/scps_econ.c:4806).

Le code débite le trésor national, reporte un éventuel manque de financement et alimente `g_pldiag_buyprod`, mais n'ajoute pas ce débit au registre `FX_*`.

**Impact :** la somme des rubriques ne peut pas expliquer intégralement la variation réelle du trésor. Un poste « Autres » calculé par différence peut réconcilier le total affiché sans rendre les causes observables.

**Lien au sweep :** c'est une cause structurelle compatible avec le résidu négatif observé. L'audit ne démontre pas qu'elle explique **la totalité** du résidu. Un débit manquant donne précisément un écart négatif ; le signe constant ne permet pas d'écarter cette hypothèse.

**À faire :** donner une rubrique aux achats de production et réconcilier, sur une même fenêtre et un même périmètre, variation de trésor, transferts et somme des flux. Le registre doit rester distinct d'une preuve de conservation de la monnaie mondiale.

### A09 — P2 — La télémétrie « régiments / limite » ne compte que la réserve

**Preuve : producteur et compteur relus, confrontation au journal graine 11.**

[Impression du bilan](C:/Users/Charl/Desktop/SCPS-main/scps/chronicle.c:2183), [ratio de force](C:/Users/Charl/Desktop/SCPS-main/scps/chronicle.c:2190), [compteur de réserve](C:/Users/Charl/Desktop/SCPS-main/scps/scps_warhost.c:171).

`warhost_units` somme `host->army[cid]`, sans les forces contenues dans les corps de campagne. Le bilan l'affiche comme « rgt / limite » et l'utilise dans sa distribution statistique.

**Impact :** sous-estimation de l'armée totale dès qu'elle est déployée ; faux diagnostic possible « riche mais désarmé » ; comparaison de limite avec un numérateur incomplet.

**À faire :** afficher séparément réserve, déployés et total, puis préciser le périmètre de la limite. Vérifier l'absence de double comptage lors de la formation et de la dissolution des corps.

**Limite :** aucun propriétaire de corps n'a été reconstruit pour prouver que Dhurganyn avait réellement une armée déployée à l'instant du journal. Le diagnostic d'armée totalement nulle est **non établi**, pas réfuté par supposition.

## 4. Tests, construction et robustesse

### A10 — P2 — Le lanceur de bancs peut déclarer vert un programme terminé en erreur

**Preuve : reproduction exécutée sur le script inchangé.**

[Décision de succès](C:/Users/Charl/Desktop/SCPS-main/tools/run_tests.sh:74).

Quand une ligne « X réussis, 0 échoués » existe, le script ignore le code retour, sauf le cas particulier du timeout 124.

Expérience isolée : remplacer les commandes de construction/exécution par des fonctions de shell ; chaque exécution imprime « 1 réussis, 0 échoués » puis retourne 1. Le script existant annonce :

```text
VERTS : 7 · ROUGES : 0 · BUILD ÉCHEC : 0
AUDIT_RUNNER_EXIT=0
```

**Impact :** un crash ou une erreur après le bilan peut être masqué. Cela n'établit pas qu'un des 42 vrais bancs a crashé pendant cet audit ; cela invalide la confiance exclusive dans le récapitulatif.

**À faire :** exiger simultanément un code retour nul et un bilan sans échec ; conserver le diagnostic complet du banc rouge. Ajouter un autotest du lanceur pour erreur après bilan, signal, timeout, bilan absent et échec de compilation.

### A11 — P2 — Le workflow sweep ne garantit pas que les simulations publiées ont réussi

**Preuve : workflow intégral relu.**

[Gates informatifs](C:/Users/Charl/Desktop/SCPS-main/.github/workflows/sweep.yml:30), [processus en arrière-plan](C:/Users/Charl/Desktop/SCPS-main/.github/workflows/sweep.yml:41), [dépouillement](C:/Users/Charl/Desktop/SCPS-main/.github/workflows/sweep.yml:51).

Les gates sont explicitement informatifs et leurs erreurs sont neutralisées. Les simulations sont lancées en arrière-plan, puis attendues par `wait` sans contrôle individuel des PID. Le dépouillement extrait les données présentes et remplace certaines absences par zéro, sans exiger une fin complète ni enregistrer un code retour par run.

**Impact :** un workflow terminé et un rapport publié ne sont pas une preuve que tous les mondes sont allés au terme annoncé. Absence de donnée et valeur zéro peuvent être confondues.

**À faire :** enregistrer PID, code retour, durée, graine, paramètres et empreinte du binaire ; vérifier le bilan terminal et le nombre attendu de runs. Publier les runs incomplets séparément. Conserver les gates informatifs si c'est voulu, mais rendre leur statut explicite.

Le workflow n'est par ailleurs pas une CI générale sur toutes les modifications : son déclenchement automatique est limité à une branche et au fichier-gâchette. Aucun nouveau déclenchement ni publication n'a été effectué pendant l'audit.

### A12 — P2 — Le garde-fou des écritures régionales échoue sur Windows et manque une écriture réelle

**Preuve : échec exécuté et contre-exemple A06.**

[Expression et allowlist](C:/Users/Charl/Desktop/SCPS-main/Makefile:775).

Avec le `rg` natif Windows présent ici, les résultats utilisent `scps\scps_credit.c`. L'allowlist attend `scps/scps_credit.c`. Le contrôle signale donc comme fautives des écritures expressément autorisées.

Indépendamment de cette incompatibilité, le motif ne reconnaît pas `region[capreg[v]].coercion` : les crochets imbriqués arrêtent la capture trop tôt. Il ne suit pas non plus les alias de pointeurs.

**Impact :** faux rouge en environnement Windows, et faux sentiment de protection contre le défaut réel A06.

**À faire :** normaliser les séparateurs ; tester l'outil avec des exemples interdits/autorisés, y compris indices imbriqués et alias. Ne pas présenter une recherche textuelle comme une preuve complète de discipline d'écriture.

### A13 — P2 — Des échecs partiels d'allocation laissent les caches de façade dans un état réutilisable invalide

**Preuve : analyse du flux de contrôle ; panne mémoire non injectée dans Godot.**

[Cache A* terrestre](C:/Users/Charl/Desktop/SCPS-main/scps/scps_api.c:5063), [cache marin partagé](C:/Users/Charl/Desktop/SCPS-main/scps/scps_api.c:5314).

Six buffers sont alloués, mais le test d'initialisation des appels suivants ne regarde que `g_ag`. Si cette première allocation réussit et qu'une autre échoue, le premier appel retourne faux sans remettre l'ensemble à zéro. Le suivant saute l'initialisation puis peut déréférencer un buffer nul.

Un problème voisin subsiste dans les [centroïdes](C:/Users/Charl/Desktop/SCPS-main/scps/scps_api.c:102) : les tailles sont publiées avant la réussite des allocations temporaires. Si un accumulateur manque, le calcul est sauté mais les tailles restent non nulles ; des valeurs anciennes ou non initialisées peuvent être servies.

**Impact :** après pression mémoire, un échec initialement géré peut devenir crash différé ou géométrie invalide.

**À faire :** initialisation atomique des groupes de buffers, libération/remise à zéro sur échec partiel ; ne publier tailles et validité qu'après calcul. Tester chaque rang d'échec d'allocation.

## 5. Interface et contexte de diagnostic

### A14 — P3 — La graine affichée dans le rapport utilisateur ne suit pas le chargement

**Preuve : chemin Godot relu.**

[Affectation lors de la génération](C:/Users/Charl/Desktop/SCPS-main/godot/project/autoload/sim.gd:62), [chargement](C:/Users/Charl/Desktop/SCPS-main/godot/project/autoload/sim.gd:82), [rapport utilisateur](C:/Users/Charl/Desktop/SCPS-main/godot/project/ui/feedback.gd:257).

`Sim.current_seed` est mis à jour par `regenerate`, pas par `load_game`. Le moteur peut donc charger une partie d'une autre graine tandis que le rapport de bug conserve la graine du monde précédent — notamment la graine 9 générée à l'ouverture.

**Impact :** mauvaise reproduction d'un signalement joueur, exactement le type d'erreur qui peut orienter un diagnostic vers le mauvais monde.

**À faire :** relire la graine canonique du moteur après chargement réussi. Test : ouvrir sur 9, charger un emplacement de graine 11, générer un rapport ; il doit indiquer 11.

## 6. Résultats des vérifications de cet audit

Tous les essais ci-dessous ont été exécutés dans la copie temporaire. GCC 16.1.0 et Clang 22.1.7/MSYS2 ont été utilisés.

| Vérification | Résultat | Ce que cela prouve / ne prouve pas |
|---|---|---|
| Construction `chronicle` et `scps_viewer` | Réussie | Moteur et façade C compilent ; pas un build GDExtension complet. |
| Suite complète, délai par banc porté à 420 s | **42 verts / 42**, aucun échec de build | Résultat du lanceur ; réserve A10 sur l'interprétation exclusive de son récapitulatif. |
| `make determinism` | **OK, 5 graines × 12 ans, deux passages** | Reproductibilité du binaire sur cet horizon, pas identité intégrale après chargement. |
| `make golden` | **OK, cinq empreintes identiques au dépôt** | Pas de dérive du scénario court de référence. |
| `--savetest` | **2/2** | Digest A/B identique et altération d'un octet rejetée ; comparaison non exhaustive de l'état. |
| `--fuzztest` | **9/9, 216 octets altérés, sortie 0** | Corpus existant seulement ; A01 montre les mutations sémantiques manquantes. |
| ASan + UBSan | **Graine 7, 20 ans, 6 empires/12 cités : sortie 0**, avec arrêt imposé sur diagnostic | Un scénario court, pas une validation des fins à 180–250 ans ni du mode naval actif. |
| `membrane-check` | **OK, 3 fichiers** | Contrôle structurel ciblé, pas toute l'interface. |
| `lang-check` | **OK, 125 / baseline 125** | Pas un test visuel de traduction FR/EN. |
| `region-write-check` | **Échec** | Incompatibilité de séparateurs Windows reproduite ; A12. |
| Analyse statique Clang | **45 modules parcourus** | Alertes triées, pas assimilées automatiquement à des bugs. |
| Probes sauvegarde/fronde | **Défauts reproduits** | A01, A02, A03, A04, A06. |
| Probe du lanceur | **Faux vert reproduit** | A10. |

Empreintes courtes obtenues :

```text
HASH 7   bddb8872
HASH 108 0600e3e5
HASH 209 b3aba329
HASH 310 30127a4d
HASH 411 c23330c4
```

Les bancs de la suite complète sont : core, monde_reel, readout, heritage, tech, intertrade, routes, save_io, statecraft, pop, army, demography, demography_integ, revolt, social, agency, campaign, factions, econ_tax, econ_culture, econ_arcane, econ_production, missions, influence, doctrines, ai, diplo, warhost, events, structural, forks, prosperity, credit, cap, endgame, audit_eco, lang, scps_api, culture, navy, religion et trade.

Commandes de référence, exécutées **dans la copie isolée**, avec la chaîne MSYS2 correspondante sur le PATH :

```sh
make CC=gcc -j4 chronicle scps
MAKEFLAGS=CC=gcc BANC_TIMEOUT=420 bash tools/run_tests.sh full
make -s CC=gcc determinism golden
./scps_viewer --savetest
./scps_viewer --fuzztest
make -s CC=gcc membrane-check
make -s CC=gcc lang-check
make -s CC=gcc region-write-check
make -s CC=gcc asan
UBSAN_OPTIONS=halt_on_error=1 ASAN_OPTIONS=halt_on_error=1 ./chronicle_asan --hash 7 1 20 6 12
```

Les deux tests du viewer ont choisi leur graine à partir de l'horloge : **1788509301** pour le savetest et **1788509335** pour le fuzztest. Le fuzz a aussi produit des avertissements de discordance des réglages lors des mutations ; son bilan final est 9/9 et son code retour 0. Les probes supplémentaires ont utilisé la graine **7**. La panne de snapshot a été injectée par interception de `tmpfile` au lien, sans changer le chargeur.

### Limites importantes des tests existants

- Le [savetest](C:/Users/Charl/Desktop/SCPS-main/scps/viewer.c:199) compare une chaîne comprenant jour, population et or arrondis, nombre de technologies, hash des propriétaires et quelques compteurs. **Ce n'est pas une comparaison byte-identique de la sauvegarde ni de tout l'état moteur.**
- Ses scénarios ne posent pas de cible de recherche humaine et n'exercent pas les commandes en attente d'une interface en pause ; A03/A04 peuvent rester invisibles.
- Le fuzz existant couvre quelques compteurs et des altérations de fichier. Les mutations d'octets sont normalement interceptées par l'empreinte avant de solliciter toute la validation sémantique.
- Les tests courts ne couvrent pas le siècle tardif : `golden-deep` et `determinism-deep` existent mais n'ont pas été rejoués ici. Ne pas confondre le sweep historique avec un test de la révision actuelle.
- ASan/UBSan ne constituent pas un détecteur général de lecture de variable locale non initialisée ; A05 exige un scénario et un outil adaptés.
- Aucune vérification graphique, de gestion des entrées, des traductions rendues, des shaders sur GPU ou d'export Godot n'est revendiquée.

## 7. Couverture et points non promus en défauts confirmés

| Ensemble | Travail réalisé | Réserve |
|---|---|---|
| Monde, noms, climat, cultures, démographie, travail, révoltes | Compilation, analyse statique, bancs, trajectoires courtes et lecture de deux trajectoires longues existantes | Pas de relecture manuelle exhaustive des générateurs ni de toutes les graines. |
| Économie, commerce, crédit, impôts | Analyse statique, bancs et lecture approfondie des écarts de comptabilité/lecteurs/migration provinciale | Pas de preuve complète de conservation de monnaie à long terme. |
| Armées, campagnes, diplomatie, marine | Analyse statique, bancs, lecteur des bilans, fronde et interception | Combat naval désactivé dans le scénario standard ; besoin de tests dédiés. |
| IA, missions, doctrines, événements, technologie, endgame | Analyse statique et bancs ; vérification des diagnostics « fonctionnalité absente » | Choix d'équilibrage et seuils d'éligibilité non recalibrés. |
| Sauvegardes et orchestration Sim | Relecture approfondie, expériences d'altération et injection de panne | Pas de fuzz structurel exhaustif ni de compatibilité multi-plateforme. |
| Façade C / GDExtension / Godot | Banc API 255/255, inspection du pont et des parcours génération/chargement/navigation/rapport | Pas de lancement de l'éditeur, du jeu ou d'export ; dépendance godot-cpp non réauditée. |
| Outils, Makefile et CI | Exécution des gates, lecture du lanceur/workflow, test du faux vert | Workflow distant non exécuté. |
| Bibliothèques tierces, addons, assets | Inventaire et observation de l'intégration | Pas d'audit indépendant exhaustif ni de recherche de vulnérabilités publiée. |

Observations utiles, mais volontairement non gonflées en constats principaux :

1. **Construction parallèle redondante.** `make chronicle scps -j4` a compilé certains objets deux fois : la cible `scps` lance une construction récursive ([Makefile](C:/Users/Charl/Desktop/SCPS-main/Makefile:129)). Le workflow utilise cette forme. Cela mérite une dépendance directe dans le graphe de build ; aucun objet corrompu n'a été constaté pendant cet audit.
2. **Alertes statiques conditionnelles.** Clang relève aussi des chemins à pointeur diplomatique nul et des lectures supposées indéfinies dans la génération/noms. Les appelants normaux et invariants de taille ne sont pas tous invalidés par ces traces : ces alertes ne sont pas comptées comme pannes de jeu démontrées. Les champs de fronde incohérents rejoignent en revanche A01.
3. **Commentaires et historique périmés.** Plusieurs commentaires de télémétrie réclament des coques non nulles alors que le mode naval est volontairement éteint ; l'historique documentaire décrit plusieurs baselines incompatibles. Distinguer état actuel, expérimentation ancienne et règle de conception éviterait des diagnostics contradictoires.
4. **Mémoire globale.** Plusieurs caches et bindings sont globaux au processus. L'usage courant à une simulation n'est pas une preuve de support de plusieurs simulations simultanées ou de threads. Aucun refactoring multi-instance n'est recommandé sans besoin explicite.
5. **Dépassement de limite militaire.** Un ratio supérieur à 100 % n'est pas automatiquement une erreur : distinguer limite souple, acquisition d'unités et perte de territoire. L'audit ne propose pas de plafonnement arbitraire.
6. **Équilibrage doctrines.** Mesurer les choix parmi les pays éligibles, par moment de décision, avant de conclure à une préférence cassée. Une répartition aléatoire des slots n'est pas un étalon causal suffisant pour une IA dépendant de la géographie et des prérequis.

## 8. Ordre de traitement proposé — aucune mise en œuvre

1. **Fiabiliser le chargement** : validation sémantique, garantie transactionnelle, purge des anciens ordres ; ajouter les tests de panne avant tout changement de format.
2. **Restaurer le contrat joueur** : cible de recherche persistante ; tests de sauvegarde depuis une partie réelle en pause, puis rechargement dans une autre instance.
3. **Fiabiliser la preuve** : code retour des bancs, complétude et provenance des sweeps, garde-fou régional portable, compteur militaire complet.
4. **Réparer les calculs/écritures divergents** : coercition provinciale, lecteurs fiscaux, rubrique du débit de production.
5. **Durcir les modes rares** : interception navale activée et allocations partielles de façade.
6. **Rejouer le long terme sur une révision figée** : mêmes graines, mêmes durées et binaire identifié ; comparer les paires complètes et conserver les échecs comme échecs. Ne recalibrer l'économie ou les doctrines qu'après fiabilisation des mesures.

### Critères minimaux avant de conclure à une validation

- Échec de chargement ⇒ état courant inchangé, y compris caches et commandes.
- Sauvegarde joueur ⇒ cible et progression reprises conformément au contrat.
- Total militaire = réserve + corps, sans double comptage.
- Variation du trésor réconciliée avec les rubriques et transferts sur la même fenêtre.
- Chaque run de sweep a un code retour et un bilan terminal cohérent avec l'horizon demandé.
- Les tests de régression nouveaux reproduisent les défauts avant correction.
- Les tests graphiques et séculaires restent des validations distinctes, explicitement documentées.

## 9. Respect du dépôt

Le dépôt comportait déjà des traductions et captures modifiées, ainsi que des journaux, archives et dossiers non suivis. Ils ont été laissés en place.

**Seul ajout dans le dépôt : ce rapport.** Aucun fichier source, paramètre, sauvegarde utilisateur ou documentation existante n'a été corrigé. Les essais, binaires et fixtures sont restés dans le dossier temporaire d'audit.

Contrôle final : **377/377 fichiers de l'inventaire ont exactement la même taille et la même empreinte SHA-256 qu'au début ; HEAD inchangé**. Le statut Git conserve les modifications préexistantes et ajoute uniquement ce rapport. Les 48 liens locaux du rapport pointent vers des fichiers existants.
