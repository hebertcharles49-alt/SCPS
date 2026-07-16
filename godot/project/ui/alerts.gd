extends Control
## ALERTES (façon EU4/CK3) — la pile des « ÉLÉMENTS EN ATTENTE du gameplay », rendue
## en liste par le ledger droit. Chaque alerte garde un CODE COULEUR par domaine :
##   ÉTATIQUE violet (conseil vacant, âge à engager) · ARMÉE rouge (guerre sans levée/ost)
##   · SOCIAL vert (édifice constructible) · SAVOIR bleu (aucune recherche) · FOI doré
##   (fondation prête). Clic = ouvre le panneau concerné (ou exécute le geste) ; survol =
## tooltip. Display-only : tout est LU de la façade, les clics émettent des signaux —
## main câble les panneaux. Recalculé au tick (queue_redraw), jamais un état propre.
## LE JOURNAL (`_journal`/`journal_rows()`) est la seconde sortie de cette RÉSOLUTION
## UNIQUE : un ring persistant (JOURNAL_MAX) où TOUTE notification colorée — fil moteur
## ET conditions — atterrit à son APPARITION, rendu par empire_sidebar.gd (section
## JOURNAL). Aucune notification n'existe qu'en éphémère (règle joueur).

const VKit = preload("res://ui/vkit.gd")
const UIKit = preload("res://ui/uikit.gd")
const Frame = preload("res://ui/frame.gd")
## worst_shortage() — même dérivation que la cellule « déficit » du bloc ÉCONOMIE
## de la topbar (UI-2) : un seul calcul, préchargé statiquement (DRY).
const Topbar = preload("res://ui/topbar.gd")

signal open_tab(i: int)     ## onglet de la sidebar (3 = Marché · 4 = Armée · 7 = Conseil)
signal open_tech
signal open_construct
signal open_religion
signal goto_region(r: int)  ## centre la carte sur la région de l'alerte (siège, famine, révolte)
signal popup_requested(e: Dictionary)  ## évènement MAJEUR → le popup OYEZ OYEZ (pause + boutons)
signal age_recap_requested             ## chip d'âge cliqué → l'ÉCRAN DE CHAPITRE (récap, pause)
signal open_tech_metab                 ## chip « métabolisation prête » cliqué → ouvre l'arbre tech
signal ledger_changed                  ## la bande droite redessine sa liste de notifications

const COL_ETAT   := Color(0.55, 0.38, 0.66)   ## violet — étatique
const COL_ARMEE  := Color(0.72, 0.28, 0.24)   ## rouge — armée
const COL_SOCIAL := Color(0.44, 0.62, 0.36)   ## vert — social/développement
const COL_SAVOIR := Color(0.37, 0.54, 0.70)   ## bleu — savoir
const COL_FOI    := Color(0.79, 0.64, 0.30)   ## doré — foi
const COL_ECO    := Color(0.78, 0.52, 0.22)   ## orange — économie/commerce

const CHIP := 30.0
const GAP := 6.0
const LABELW := 280.0  ## la colonne du LABEL visible (« letters » RimWorld : lisible sans hover)
const FEED_MAX := 8   ## évènements gardés à l'écran (les plus récents ; clic = acquitté)
const COL_COMPACT := Color(0.70, 0.62, 0.40)   ## bronze neutre — « le reste attend, hors vue »
const JOURNAL_MAX := 200   ## le JOURNAL (empire_sidebar.gd, section JOURNAL) : ring persistant

## AUDIT UI 1.4 (« alertes vs fenêtres majeures ») : Main expose major_open() (une des
## fenêtres de lecture/décision — tech/éco/codex/construction/détail/diplomatie/annales/
## chapitre/épilogue/religion — est ouverte) ; alerts.gd n'a pas de référence à Main, donc
## un Callable posé par main.gd juste après avoir instancié ce panneau (cf. main.gd).
var major_open_fn: Callable = Callable()
var _major_open := false

## LA TABLE DU FIL (FeedKind → présentation) — AJOUTER UN ÉVÈNEMENT = une ligne ici
## (+ la valeur enum + le feed_push au site d'observation, cf. scps_provlog.h).
## fmt : {a}/{b} = pays · {r} = région · {y} = an.
const FEED_KINDS := {
	1: {"icon": "dipl_rivalry",   "col": COL_ARMEE, "fmt": "GUERRE — {a} entre en guerre contre nous (an {y})"},
	2: {"icon": "dipl_alliance",  "col": COL_ETAT,  "fmt": "PAIX signée avec {a} (an {y})"},   # tip enrichi du VERDICT (score {v}) dans _poll_feed
	3: {"icon": "alert_siege",    "col": COL_ARMEE, "fmt": "Une place est TOMBÉE — {a} occupe la région {r} (an {y})"},
	4: {"icon": "stability_shield", "col": COL_ARMEE, "fmt": "Région {r} REPRISE par nos armes (an {y})"},
	5: {"icon": "alert_warning",  "col": COL_ARMEE, "fmt": "PILLAGE — la région {r} a été mise à sac (an {y})"},
	6: {"icon": "alert_revolt",   "col": COL_ETAT,  "fmt": "RÉVOLTE — un soulèvement éclate en région {r} (an {y})"},   # {a} = "Rebelles de X" si la guerre civile est INCARNÉE (sinon générique) — cf. _poll_feed
	7: {"icon": "settlement_cluster", "col": COL_ETAT, "fmt": "SÉCESSION — {a} proclame son indépendance (an {y})"},
	8: {"icon": "stability_shield", "col": COL_ARMEE, "fmt": "BATAILLE GAGNÉE contre {b} en région {r} (an {y})"},
	9: {"icon": "alert_warning",  "col": COL_ARMEE, "fmt": "BATAILLE PERDUE contre {b} — l'ost est brisé (région {r}, an {y})"},
	10: {"icon": "alert_event_bell", "col": COL_ETAT, "fmt": "{label} — région {r} (an {y})"},   # ÉVÈNEMENT du directeur
	11: {"icon": "alert_warning", "col": COL_ARMEE, "fmt": "BATAILLE INDÉCISE contre {b} en région {r} (an {y})"},
}
## kinds MAJEURS → popup OYEZ OYEZ (pause + boutons adaptatifs) au lieu d'un chip.
const POPUP_KINDS := [1, 2, 6, 7, 10]   # guerre · paix (verdict) · révolte · sécession · directeur

var _alerts := []    ## [{icon, col, tip, act, …}] conditions, recalculées à chaque _refresh
var _events := []    ## [{icon, col, tip, seq}] fil transient (clic = acquitté)
var _seen_seq := 0   ## dernier seq lu du fil
var _ledger_mode := false

## LE JOURNAL — ring PERSISTANT (JOURNAL_MAX), le plus récent en TÊTE (push_front) ;
## display-only, jamais sérialisé. Alimenté par CHAQUE notification colorée : les
## évènements du fil (_poll_feed, TOUS les kinds — chip ET popup) et les conditions
## dès leur APPARITION (_journal_track_conditions, anti-spam par clé stable). Toute
## notification qui apparaît en chip/popup DOIT donc s'y retrouver — aucune n'existe
## qu'en éphémère (règle joueur).
var _journal := []
var _journal_seq := 0
var _prev_cond_keys := {}   ## clés des conditions actives au refresh précédent (edge-detection)

## Les notifications vivent dans le ledger droit. Le nœud conserve collecte, polling
## et routage des actions, mais ne dessine plus une seconde colonne flottante sur la carte.
func set_ledger_mode(on: bool) -> void:
	_ledger_mode = on
	_refresh()

func ledger_rows() -> Array:
	return _stack().duplicate(true)

func ledger_short(al: Dictionary) -> String:
	return _short(String(al.get("tip", "")))

## LE JOURNAL — la liste persistante (la plus récente en tête) montrée par la bande
## droite (empire_sidebar.gd, section JOURNAL). Duplique le tableau (comme
## ledger_rows) — le lecteur ne mute jamais l'état interne.
func journal_rows() -> Array:
	return _journal.duplicate(true)

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	Sim.generated.connect(func():
		_journal.clear(); _journal_seq = 0; _prev_cond_keys.clear()
		_refresh())
	Sim.ticked.connect(func(_y): _refresh())
	get_viewport().size_changed.connect(_refresh)
	_refresh.call_deferred()

## AUDIT 1.4 : `major_open` bascule sur un simple `.visible = false` posé par un tas de
## sites différents (fermeture par ✕, par Échap, par un signal…) — aucun d'eux ne
## PRÉVIENT alerts.gd. On le SURVEILLE donc à chaque frame (le Callable est bon marché)
## et on ne redessine/retaille que lors d'un CHANGEMENT effectif.
func _process(_dt: float) -> void:
	if not major_open_fn.is_valid():
		return
	var mo := bool(major_open_fn.call())
	if mo != _major_open:
		_refresh()

## recalcule la pile + repositionne le contrôle (il n'occupe QUE sa colonne de chips —
## jamais un plein-écran qui mangerait les clics de la carte).
func _refresh() -> void:
	# GATE : tant que la PARTIE n'a pas commencé (menu/setup), aucune alerte ni popup —
	# le monde de fond tourne pour la vitrine, ses évènements ne concernent pas le joueur.
	if not Sim.game_on:
		_alerts = []
		_events = []
		if Sim.world != null and Sim.world.has_method("feed_poll"):
			for ev in Sim.world.feed_poll(_seen_seq):   # on JETTE le fil pré-partie (acquitté)
				_seen_seq = maxi(_seen_seq, int(ev["seq"]))
		visible = false
		ledger_changed.emit()
		return
	_alerts = _collect()
	_journal_track_conditions(_alerts)
	_poll_feed()
	_major_open = major_open_fn.is_valid() and bool(major_open_fn.call())
	if _ledger_mode:
		visible = false
		ledger_changed.emit()
		return
	visible = true
	var n := _rows().size()
	var vw := get_viewport_rect().size.x
	# décalé à GAUCHE de l'empire-sidebar (bande droite permanente en jeu) ; la colonne
	# porte désormais le LABEL visible (façon « letters » RimWorld) + le chip
	position = Vector2(vw - CHIP - LABELW - 10.0 - (Frame.LEDGER_W + 6.0 if Sim.game_on else 0.0), Frame.TOPBAR_H + 10.0)
	size = Vector2(CHIP + LABELW, maxf(1.0, n * (CHIP + GAP)))
	visible = n > 0
	queue_redraw()

## VOIE ÉVÈNEMENTS : poll incrémental du fil moteur → chips TRANSIENTS
## (clic gauche = lieu si localisé ; clic droit = acquittement seul).
func _poll_feed() -> void:
	var w = Sim.world
	if w == null or not w.has_method("feed_poll"):
		return
	for ev in w.feed_poll(_seen_seq):
		_seen_seq = maxi(_seen_seq, int(ev["seq"]))
		var kind := int(ev["kind"])
		if not FEED_KINDS.has(kind):
			continue   # kind inconnu du front : silencieux (l'ajout = une ligne dans FEED_KINDS)
		# ÉVÈNEMENT DU DIRECTEUR : filtre de PERTINENCE — ma région, ou mon pays
		if kind == 10:
			var mine := false
			var me: int = w.player()
			var evr := int(ev["region"])
			if evr >= 0:
				mine = (int(w.region_owner(evr)) == me)
			else:
				mine = (int(ev.get("a_id", -1)) == me)
			if not mine:
				continue
		var k: Dictionary = FEED_KINDS[kind]
		var tip := String(k["fmt"]).replace("{a}", String(ev["a"])).replace("{b}", String(ev["b"])) \
			.replace("{r}", str(int(ev["region"]))).replace("{y}", str(int(ev["year"]))) \
			.replace("{label}", String(ev.get("label", "")))
		if kind == 2:
			# la PAIX porte le SCORE DE GUERRE final (±100, notre point de vue) → le VERDICT
			var sc := int(ev.get("v", 0))
			var verdict := "guerre GAGNÉE" if sc >= 10 else ("guerre PERDUE" if sc <= -10 else "paix blanche")
			tip += " — %s (score %+d)" % [verdict, sc]
		if kind == 8 or kind == 9 or kind == 11:
			tip += " — " + _battle_losses_text(int(ev.get("v", 0)))
		if kind == 6 and int(ev.get("a_id", -1)) >= 0:
			# GUERRE CIVILE INCARNÉE (scps_revolt.c spawn_rebel_polity) : {a} porte déjà le
			# nom du rebelle ("Rebelles de <héritage>") — le fil le NOMME au lieu du générique.
			tip += " — %s" % String(ev["a"])
		# JOURNAL — TOUTE notification du fil y atterrit (chip ET popup, AVANT le tri
		# ci-dessous) : rien n'existe qu'en éphémère.
		var jregion := int(ev.get("region", -1))
		_push_journal({"icon": k["icon"], "col": k["col"], "tip": tip, "year": int(ev["year"]),
			"region": jregion, "act": "goto" if jregion >= 0 else "", "kind": kind})
		if kind == 1:
			Sound.play("moment_war_horn")   # le COR : une guerre nous est déclarée
		if kind in POPUP_KINDS:
			popup_requested.emit(_popup_of(kind, ev, tip))   # MAJEUR → OYEZ OYEZ (pause)
			continue
		var action := _feed_event_action(kind, int(ev.get("region", -1)))
		var entry := {"icon": k["icon"], "col": k["col"], "seq": int(ev["seq"])}
		entry.merge(action)
		entry["tip"] = tip + ("  (clic : y aller · clic droit : acquitter)" if not action.is_empty() else "  (clic : acquitter)")
		_events.append(entry)
	while _events.size() > FEED_MAX:
		_events.pop_front()   # bornés : les plus récents restent

## bâtit le POPUP d'un kind majeur : titre + corps + BOUTONS ADAPTATIFS à la situation.
func _popup_of(kind: int, ev: Dictionary, tip: String) -> Dictionary:
	var reg := int(ev["region"])
	var btns := []
	var title := ""
	match kind:
		1:
			title = "LA GUERRE !"
			btns = [{"label": "Voir la diplomatie", "act": "diplo"},
				{"label": "Lever l'ost", "act": "army"}, {"label": "Vu", "act": "close"}]
		2:
			title = "LA PAIX EST SIGNÉE"
			btns = [{"label": "Vu", "act": "close"}]
		6:
			# guerre civile INCARNÉE (a_id≥0) : le titre NOMME le rebelle ("Rebelles de X").
			title = String(ev["a"]) if int(ev.get("a_id", -1)) >= 0 else "RÉVOLTE !"
			btns = [{"label": "Y aller", "act": "goto", "region": reg},
				{"label": "Réprimer", "act": "repress", "region": reg}, {"label": "Vu", "act": "close"}]
		7:
			title = "SÉCESSION !"
			btns = [{"label": "Vu", "act": "close"}]
		10:
			title = String(ev.get("label", "Évènement"))
			if reg >= 0:
				btns = [{"label": "Y aller", "act": "goto", "region": reg}, {"label": "Vu", "act": "close"}]
			else:
				btns = [{"label": "Vu", "act": "close"}]
	return {"title": title, "body": tip, "buttons": btns, "kind": kind}

## LA COLLECTE : chaque « élément en attente » du gameplay, lu de la façade.
func _collect() -> Array:
	var out := []
	var w = Sim.world
	if w == null or not w.has_method("country_council"):
		return out
	var me: int = w.player()
	if me < 0:
		return out
	# ÉTATIQUE — siège(s) du conseil VACANT(S) (la pool par générations est toujours pleine)
	var vac := 0
	for seat in w.country_council(me):
		if not bool(seat["filled"]):
			vac += 1
	if vac > 0:
		out.append({"icon": "menu_council", "col": COL_ETAT, "act": "council",
			"tip": "%d siège(s) du conseil VACANT(S) — recruter un candidat (clic : onglet Conseil)" % vac})
	# ÉTATIQUE — un âge s'est levé et n'est pas engagé
	if w.has_method("age_state"):
		var ag: Dictionary = w.age_state()
		if int(ag.get("age", -1)) >= 0 and not bool(ag.get("engaged", true)):
			out.append({"icon": "politics_crown", "col": COL_ETAT, "act": "age",
				"tip": "Un âge s'est levé : %s — clic pour l'ENGAGER (une fois par âge)" % String(ag.get("name", ""))})
	# SAVOIR — aucune recherche en cours
	var rs: Dictionary = w.research_status()
	if int(rs.get("target", -1)) < 0:
		out.append({"icon": "knowledge_book", "col": COL_SAVOIR, "act": "tech",
			"tip": "Aucune RECHERCHE en cours — clic : choisir une cible dans l'arbre"})
	# SOCIAL — au moins un édifice CONSTRUCTIBLE (débloqué + or suffisant)
	var ci: Dictionary = w.country_info(me)
	var gold: float = float(ci.get("or", 0))
	for b in w.building_roster(me):
		if bool(b.get("debloque", false)) and float(b.get("gold", 1e18)) <= gold:
			out.append({"icon": "action_build", "col": COL_SOCIAL, "act": "construct",
				"tip": "Un ÉDIFICE est constructible (ex. %s, %d or) — clic : panneau Construction" % [String(b.get("nom", "")), int(b.get("gold", 0))]})
			break
	# ARMÉE — EN GUERRE : levée à zéro, ou pas d'armée de campagne déployée
	var at_war := false
	for rel in w.country_relations(me):
		if bool(rel.get("at_war", false)):
			at_war = true
			break
	if at_war:
		var a: Dictionary = w.country_army(me)
		if int(a.get("levy", 0)) <= 0:
			out.append({"icon": "menu_army", "col": COL_ARMEE, "act": "army",
				"tip": "EN GUERRE et levée à ZÉRO — clic : monter la levée (onglet Armée)"})
		elif not bool(w.army_info(me).get("active", false)):
			out.append({"icon": "menu_army", "col": COL_ARMEE, "act": "army",
				"tip": "EN GUERRE sans armée de campagne — clic : onglet Armée (puis « Attaquer ici » sur la cible)"})
	# FOI — la fondation est PRÊTE (1er édifice religieux bâti, pas encore de foi)
	if w.has_method("religion_founding_ready") and int(w.religion_founding_ready(me)) == 1:
		out.append({"icon": "faith_candle", "col": COL_FOI, "act": "religion",
			"tip": "Votre peuple a bâti son premier sanctuaire — clic : FONDER la foi"})
	# ── CONDITIONS MOTEUR (un seul appel C : révolte · famine · siège · prix · conso) ──
	if w.has_method("player_alerts"):
		var pa: Dictionary = w.player_alerts()
		if int(pa.get("revolt_region", -1)) >= 0:
			out.append({"icon": "alert_revolt", "col": COL_ETAT, "act": "goto",
				"region": int(pa["revolt_region"]),
				"tip": "La région %d GRONDE (agitation %d) — réprimer, assimiler ou apaiser (clic : y aller)" % [int(pa["revolt_region"]), int(pa["revolt_agit"])]})
		if int(pa.get("famine_region", -1)) >= 0:
			out.append({"icon": "alert_famine", "col": COL_SOCIAL, "act": "goto",
				"region": int(pa["famine_region"]),
				"tip": "FAMINE — la région %d ne mange qu'à %d %% (greniers, import, colonie vivrière) (clic : y aller)" % [int(pa["famine_region"]), int(pa["famine_pct"])]})
		if int(pa.get("siege_region", -1)) >= 0:
			out.append({"icon": "alert_siege", "col": COL_ARMEE, "act": "goto",
				"region": int(pa["siege_region"]),
				"tip": "SIÈGE — %s assiège notre région %d ! Lever l'ost (clic : y aller)" % [String(pa["siege_by"]), int(pa["siege_region"])]})
		if int(pa.get("price_good", -1)) >= 0:
			out.append({"icon": "alert_shortage", "col": COL_ECO, "act": "market",
				"tip": "PRIX EXORBITANT — %s à ×%.1f de l'ancre au marché (clic : onglet Marché)" % [String(pa["price_name"]), float(pa["price_x10"]) / 10.0]})
		if int(pa.get("conso_good", -1)) >= 0:
			out.append({"icon": "alert_shortage", "col": COL_ECO, "act": "market",
				"tip": "BIEN INTROUVABLE — %s est demandé mais ni produit ni en stock (clic : onglet Marché)" % String(pa["conso_name"])})
	# ÉCONOMIE — PÉNURIE CRITIQUE (retour joueur UI-2 : « Fer : rupture dans 12 jours »
	# remonte en alerte explicite) : moins de 30 jours de couverture au rythme actuel,
	# même dérivation que la cellule « déficit » de la topbar (Topbar.worst_shortage).
	var short := Topbar.worst_shortage(w, me)
	if not short.is_empty() and int(short["days"]) < 30:
		out.append({"icon": "alert_shortage", "col": COL_ECO, "act": "market",
			"tip": "%s : rupture dans %d jours au rythme actuel (clic : onglet Marché)" % [
				String(short["name"]), int(short["days"])]})
	# UI-MONNAIE (2026-07-16) — U4 : LES ÉVÉNEMENTS MONÉTAIRES. CONDITIONS polées (motif
	# ci-dessus, pas un nouveau canal moteur) : lecteurs PURS (country_bankruptcy_scar/
	# country_debase_frac), l'édge-detection existante (_journal_track_conditions) fait
	# le travail « apparition → une ligne au journal ». Découvertes d'or (M7, EVID_GOLD_
	# DISCOVERY) : DÉJÀ un dilemme à part entière (pending_event/player_event_choice —
	# « Proclamer la découverte »), plus visible qu'une ligne de journal — non dupliqué ici.
	if w.has_method("country_bankruptcy_scar") and float(w.country_bankruptcy_scar(me)) > 0.01:
		out.append({"icon": "alert_warning", "col": COL_ECO, "act": "market",
			"tip": "BANQUEROUTE — tes créanciers saisissent une part de ta production (cicatrice active, onglet Monnaie)"})
	if w.has_method("country_debase_frac") and float(w.country_debase_frac(me)) > 0.001:
		out.append({"icon": "alert_warning", "col": COL_ECO, "act": "market",
			"tip": "DÉBASE EN COURS — sur-frappe payée en confiance (onglet Monnaie)"})
	# ADVERSAIRES — parcourt les pays CONNUS SEULEMENT (motif at_war ci-dessus, jamais
	# le voile de brouillard) : banqueroute/débase d'un voisin, en MOTS, sans navigation
	# (intel de lecture seule — rien à « aller faire » chez un autre pays).
	if w.has_method("country_bankruptcy_scar") or w.has_method("country_debase_frac"):
		for rel in w.country_relations(me):
			var rcid := int(rel.get("country", -1))
			if rcid < 0:
				continue
			var rnm := String(rel.get("name", "?"))
			if w.has_method("country_bankruptcy_scar") and float(w.country_bankruptcy_scar(rcid)) > 0.01:
				out.append({"icon": "alert_warning", "col": COL_ECO, "act": "",
					"tip": "%s traverse une BANQUEROUTE — ses créanciers saisissent sa production" % rnm})
			if w.has_method("country_debase_frac") and float(w.country_debase_frac(rcid)) > 0.001:
				out.append({"icon": "alert_warning", "col": COL_ECO, "act": "",
					"tip": "%s DÉBASE sa monnaie — sur-frappe au-delà de la parité" % rnm})
	return out

## VOIE MÉTABOLISATION (V1b) : tech_panel.gd notifie qu'un héritage NON natif vient
## d'atteindre tier 3 (digestion pleine) — chip transient discret, même motif que le fil
## moteur (`_events`) mais poussé DIRECTEMENT (ce n'est pas un feed C, c'est un latch
## GDScript côté tech_panel). Clic = ouvre l'arbre tech sur la bande de métabolisation.
func push_metab_ready(nom: String) -> void:
	_seen_seq += 1   # partage la numérotation de seq (clic = acquitté, comme le fil moteur)
	var tip := "Métabolisation : %s prête (clic : voir l'arbre)" % nom
	_events.append({"icon": "knowledge_book", "col": COL_SAVOIR, "tip": tip,
		"seq": _seen_seq, "act": "tech_metab"})
	while _events.size() > FEED_MAX:
		_events.pop_front()
	_push_journal({"icon": "knowledge_book", "col": COL_SAVOIR, "tip": tip,
		"year": int(Sim.world.year()) if Sim.world != null and Sim.world.has_method("year") else 0,
		"region": -1, "act": "tech_metab"})
	_refresh()

## empile UNE entrée au journal (ring, la plus récente en tête) — le SEUL point
## d'écriture (fil moteur, métabolisation, conditions) pour garder la borne unique.
func _push_journal(entry: Dictionary) -> void:
	entry["jseq"] = _journal_seq
	_journal_seq += 1
	_journal.push_front(entry)
	while _journal.size() > JOURNAL_MAX:
		_journal.pop_back()

## clé STABLE d'une condition (ignore ses CHIFFRES, qui dérivent d'un tick à l'autre —
## « 3 siège(s) vacant(s) » et « 2 siège(s) vacant(s) » sont LA MÊME condition en
## cours : une seule entrée au journal, à son APPARITION, pas une par tick/variation).
func _cond_key(al: Dictionary) -> String:
	var stripped := ""
	for ch in String(al.get("tip", "")):
		if ch < "0" or ch > "9":
			stripped += ch
	return "%s|%d|%s" % [String(al.get("act", "")), int(al.get("region", -1)), stripped]

## ÉDGE-DETECTION des conditions (_alerts) : une condition qui vient d'APPARAÎTRE
## (clé absente du refresh précédent) rejoint le journal ; une condition qui PERSISTE
## ne réécrit rien (sinon le journal se remplirait au tick, pas à l'évènement).
func _journal_track_conditions(alerts: Array) -> void:
	var cur := {}
	var yr := int(Sim.world.year()) if Sim.world != null and Sim.world.has_method("year") else 0
	for al in alerts:
		var k := _cond_key(al)
		cur[k] = true
		if not _prev_cond_keys.has(k):
			var e: Dictionary = al.duplicate(true)
			e["year"] = yr
			_push_journal(e)
	_prev_cond_keys = cur

## la pile AFFICHÉE : les ÉVÈNEMENTS (récents en tête, transients) puis les CONDITIONS.
func _stack() -> Array:
	var st := []
	for i in range(_events.size() - 1, -1, -1):   # le plus récent d'abord
		st.append(_events[i])
	st.append_array(_alerts)
	return st

## AUDIT 1.4 : les LIGNES effectivement affichées. Fenêtre majeure FERMÉE → une ligne par
## alerte/évènement (pile normale, INCHANGÉE — « fermeture ⇒ pile restaurée telle quelle »
## est vrai PAR CONSTRUCTION : rien n'est perdu, tout est recalculé de `_alerts`/`_events`).
## Fenêtre majeure OUVERTE → les items « CRITIQUES » (`item.critical == true`) restent un
## par un, le RESTE se replie en UNE ligne compteur « N ⚠ ». Aucune alerte n'est typée
## critique aujourd'hui : les décisions réellement urgentes (guerre/révolte/sécession/
## directeur, `event_dialog.gd`/`event_popup.gd`) ouvrent déjà leur PROPRE modale
## au-dessus de tout, indépendamment de major_open — rien dans CETTE pile n'a besoin
## d'échapper au repli. Le champ reste lu pour qu'un futur type d'alerte puisse s'y
## soustraire sans toucher ce fichier.
func _rows() -> Array:
	var st := _stack()
	if _ledger_mode or not _major_open:
		var out := []
		for al in st:
			out.append({"kind": "chip", "data": al})
		return out
	var out := []
	var collapsed := []
	for al in st:
		if bool(al.get("critical", false)):
			out.append({"kind": "chip", "data": al})
		else:
			collapsed.append(al)
	if collapsed.size() > 0:
		out.append({"kind": "compact", "items": collapsed})
	return out

## label COURT dérivé du tip (coupé au tiret/parenthèse, tronqué) — le texte VISIBLE
## de la « letter » (façon RimWorld : la notification se lit sans survol).
func _short(tip: String) -> String:
	var s := tip
	var cut := s.find(" — ")
	if cut < 0:
		cut = s.find(" (")
	if cut < 0:
		cut = s.find(" : ")
	if cut > 0:
		s = s.substr(0, cut)
	# 42 au lieu de 26 : « 3 siège(s) du conseil VAC… » se lisait tronqué (retour
	# joueur 2026-07-10) — le cartouche s'élargit à son texte, on peut le laisser dire.
	if s.length() > 42:
		s = s.substr(0, 41) + "…"
	return s

## un CHIP normal (alerte/évènement) à la ligne `y` — factorisé de l'ancien _draw()
## unique pour être partagé avec la vue « fenêtre majeure » (items critiques, 1.4).
func _draw_chip(al: Dictionary, y: float) -> void:
	var c: Color = al["col"]
	# LE CARTOUCHE-LABEL (letters RimWorld) : texte lisible sans hover, aligné à
	# droite contre le chip, fond sombre translucide + liseré au domaine.
	var lab := _short(String(al.get("tip", "")))
	if lab != "":
		var lw := VKit.text_w(lab, VKit.FS_SMALL) + 14.0
		var lr := Rect2(LABELW - lw, y + 3, lw, CHIP - 6)
		VKit.fill(self, lr, Color(0.07, 0.055, 0.04, 0.88))
		VKit.box(self, lr, Color(c, 0.75))
		VKit.text(self, Vector2(lr.position.x + 7, y + 7), VKit.COL_PARCH, lab, VKit.FS_SMALL)
	var r := Rect2(LABELW, y, CHIP, CHIP)
	VKit.fill(self, r, VKit.COL_PANEL)
	draw_rect(r, c, false, 2.0)                       # le CODE COULEUR : liseré épais du domaine
	draw_rect(Rect2(LABELW, y + CHIP - 4, CHIP, 4), c)  # + socle plein (lisible en périphérie)
	UIKit.draw_icon(self, String(al["icon"]), Vector2(LABELW + 5, y + 3), 20)
	if al.has("seq"):                                  # ÉVÈNEMENT : pastille « neuf » en coin
		draw_circle(Vector2(LABELW + CHIP - 4, y + 4), 3.0, Color(0.95, 0.90, 0.75))

## AUDIT 1.4 : le COMPTEUR compact « N ⚠ » qui remplace la pile ordinaire quand une
## fenêtre majeure est ouverte — même ancrage/gabarit qu'un chip normal (couleur neutre,
## pas de code-domaine puisqu'il en résume plusieurs).
func _draw_compact(n: int, y: float) -> void:
	var lab := "%d ⚠" % n
	var lw := VKit.text_w(lab, VKit.FS_SMALL) + 14.0
	var lr := Rect2(LABELW - lw, y + 3, lw, CHIP - 6)
	VKit.fill(self, lr, Color(0.07, 0.055, 0.04, 0.88))
	VKit.box(self, lr, Color(COL_COMPACT.r, COL_COMPACT.g, COL_COMPACT.b, 0.75))
	VKit.text(self, Vector2(lr.position.x + 7, y + 7), VKit.COL_PARCH, lab, VKit.FS_SMALL)
	var r := Rect2(LABELW, y, CHIP, CHIP)
	VKit.fill(self, r, VKit.COL_PANEL)
	draw_rect(r, COL_COMPACT, false, 2.0)
	draw_rect(Rect2(LABELW, y + CHIP - 4, CHIP, 4), COL_COMPACT)
	UIKit.draw_icon(self, "alert_warning", Vector2(LABELW + 5, y + 3), 20)

func _draw() -> void:
	# AUDIT 1.4 : lu ICI, à chaque redessin — le seul autre lecteur (_process) ne fait que
	# détecter le CHANGEMENT pour forcer ce redessin, la valeur qui compte est celle-ci.
	_major_open = major_open_fn.is_valid() and bool(major_open_fn.call())
	var y := 0.0
	for row in _rows():
		if String(row["kind"]) == "compact":
			_draw_compact((row["items"] as Array).size(), y)
		else:
			_draw_chip(row["data"], y)
		y += CHIP + GAP

func _gui_input(event: InputEvent) -> void:
	if not (event is InputEventMouseButton and event.pressed):
		return
	var rows := _rows()
	var idx := int(event.position.y / (CHIP + GAP))
	if idx < 0 or idx >= rows.size():
		return
	var row: Dictionary = rows[idx]
	if String(row["kind"]) == "compact":
		# le compteur RÉSUME plusieurs alertes — rien d'univoque à cibler ; fermer la
		# fenêtre majeure restaure la pile cliquable telle quelle (aucune perte).
		accept_event()
		return
	var al: Dictionary = row["data"]
	activate_ledger(al, event.button_index)
	accept_event()

## Action publique utilisée par les lignes du ledger. Les clics gardent exactement la
## sémantique des anciennes letters : gauche = agir/acquitter, droite = balayer l'évènement.
func activate_ledger(al: Dictionary, button_index: int) -> void:
	# CLIC DROIT = BALAYER (letters RimWorld) : un évènement transient se dismisse sans
	# agir ; les CONDITIONS persistantes (alerte de fond) restent — elles disent un état.
	if button_index == MOUSE_BUTTON_RIGHT:
		if al.has("seq"):
			for i in range(_events.size()):
				if int(_events[i]["seq"]) == int(al["seq"]):
					_events.remove_at(i)
					break
			Sound.play("ui_click")
			_refresh()
		return
	if button_index != MOUSE_BUTTON_LEFT:
		return
	Sound.play("ui_click")   # le son du CLIC sur la notification (comme tout clic)
	if al.has("seq"):
		# ÉVÈNEMENT : le clic ACQUITTE (et centre la carte si localisé)
		for i in range(_events.size()):
			if int(_events[i]["seq"]) == int(al["seq"]):
				_events.remove_at(i)
				break
	_route_action(al)
	_refresh()

## Le JOURNAL est un HISTORIQUE : le clic route la MÊME action que la notification
## d'origine (centrer la carte, ouvrir le panneau…) mais n'ACQUITTE rien — la ligne
## reste (c'est une trace, pas une pile à vider). Clic gauche seulement (pas de
## « balayage » d'une entrée déjà passée).
func activate_journal(al: Dictionary, button_index: int) -> void:
	if button_index != MOUSE_BUTTON_LEFT:
		return
	Sound.play("ui_click")
	_route_action(al)

## LE ROUTAGE D'ACTION — commun au ledger transient ET au journal permanent. Les deux
## vocabulaires d'`act` (évènement : tech_metab/goto ; condition : council/army/
## market/tech/construct/religion/goto/age) sont DISJOINTS ⇒ un seul `match` couvre
## les deux sans collision (ex-duplication entre les branches has("seq")/sinon).
func _route_action(al: Dictionary) -> void:
	match String(al.get("act", "")):
		"tech_metab":
			open_tech_metab.emit()
		"goto":
			goto_region.emit(int(al.get("region", -1)))
		"council":
			open_tab.emit(7)
		"army":
			open_tab.emit(4)
		"market":
			open_tab.emit(3)
		"tech":
			open_tech.emit()
		"construct":
			open_construct.emit()
		"religion":
			open_religion.emit()
		"age":
			# le clic n'ENGAGE plus directement : il ouvre l'ÉCRAN DE CHAPITRE (récap
			# d'âge, monde en pause) — c'est LÀ que le verbe s'émet, ou pas (« Plus tard »).
			age_recap_requested.emit()

## HOVER natif : le tooltip de l'alerte survolée — ou (fenêtre majeure ouverte, ligne
## compteur) les tips des alertes repliées, CONCATÉNÉS (audit 1.4 : « hover = les tips
## concaténés »).
func _get_tooltip(at_position: Vector2) -> String:
	var rows := _rows()
	var idx := int(at_position.y / (CHIP + GAP))
	if idx < 0 or idx >= rows.size():
		return ""
	var row: Dictionary = rows[idx]
	if String(row["kind"]) == "compact":
		var items: Array = row["items"]
		var parts := PackedStringArray()
		for it in items:
			parts.append("• " + String((it as Dictionary).get("tip", "")))
		return "%d alerte(s) en attente (fenêtre ouverte) :\n%s" % [items.size(), "\n".join(parts)]
	return String((row["data"] as Dictionary).get("tip", ""))

static func _grp(n: int) -> String:
	var s := str(absi(n))
	var out := ""
	var count := 0
	for i in range(s.length() - 1, -1, -1):
		out = s[i] + out
		count += 1
		if count % 3 == 0 and i > 0: out = " " + out
	return ("-" if n < 0 else "") + out

static func _feed_event_action(kind: int, region: int) -> Dictionary:
	if region >= 0 and kind in [3, 4, 5, 8, 9, 11]:
		return {"act": "goto", "region": region}
	return {}

static func _battle_losses_text(packed: int) -> String:
	var ours := (packed & 0xffff) * 100
	var theirs := ((packed >> 16) & 0xffff) * 100
	return "pertes confirmées : nous %s · ennemi %s" % [_grp(ours), _grp(theirs)]
