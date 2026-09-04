extends Control
## ALERTES (façon EU4/CK3) — la pile des « ÉLÉMENTS EN ATTENTE du gameplay », rendue
## en liste par le ledger droit. Chaque alerte garde un CODE COULEUR par domaine :
##   ÉTATIQUE violet (conseil vacant, âge à engager) · ARMÉE rouge (guerre sans levée/ost)
##   · SOCIAL vert (édifice constructible) · SAVOIR bleu (aucune recherche) · FOI doré
##   (fondation prête). Clic = ouvre le panneau concerné (ou exécute le geste) ; survol =
## tooltip. Display-only : tout est LU de la façade, les clics émettent des signaux —
## main câble les panneaux. NŒUD DATA-ONLY : il collecte et route, il ne DESSINE plus rien
## (le rendu flottant sur carte a été retiré au profit de la bande droite, empire_sidebar.gd).
## LE JOURNAL (`_journal`/`journal_rows()`) est la seconde sortie de cette RÉSOLUTION
## UNIQUE : un ring persistant (JOURNAL_MAX) où TOUTE notification colorée — fil moteur
## ET conditions — atterrit à son APPARITION, rendu par empire_sidebar.gd (section
## JOURNAL). Aucune notification n'existe qu'en éphémère (règle joueur).

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

const FEED_MAX := 8   ## évènements gardés dans le fil transient (les plus récents ; clic = acquitté)
const JOURNAL_MAX := 200   ## le JOURNAL (empire_sidebar.gd, section JOURNAL) : ring persistant

## LA TABLE DU FIL (FeedKind → présentation) — AJOUTER UN ÉVÈNEMENT = une ligne ici
## (+ la valeur enum + le feed_push au site d'observation, cf. scps_provlog.h).
## fmt : {a}/{b} = pays · {r} = LE NOM du lieu (façade region_label, jamais un index) · {y} = an.
## icônes de GENRE (lot11_systeme jrn_*, campagne 2, 2026-08-26) : 12 genres livrés
## (guerre/bataille/siège/révolte/sécession/colonisation/découverte/conseil/foi/
## commerce/catastrophe/trésor) mappés sur les 11 FeedKind moteur (scps_provlog.h) —
## PAIX (kind 2) et PILLAGE (kind 5) n'ont PAS d'équivalent parmi les 12 (« paix »/
## « pillage » absents de la liste livrée) : gardés sur leur ancienne icône (repli
## ICON2 de icon(), cf. TROUVAILLES 2026-08-26 Restes) plutôt qu'un mauvais choix forcé.
const FEED_KINDS := {
	1: {"icon": "jrn_guerre",     "col": COL_ARMEE, "fmt": "GUERRE — {a} entre en guerre contre nous (an {y})"},
	2: {"icon": "dipl_alliance",  "col": COL_ETAT,  "fmt": "PAIX signée avec {a} (an {y})"},   # tip enrichi du VERDICT (score {v}) dans _poll_feed
	3: {"icon": "jrn_siege",      "col": COL_ARMEE, "fmt": "Une place est TOMBÉE — {a} occupe {r} (an {y})"},
	4: {"icon": "jrn_bataille",   "col": COL_ARMEE, "fmt": "{r} REPRISE par nos armes (an {y})"},
	5: {"icon": "alert_warning",  "col": COL_ARMEE, "fmt": "PILLAGE — {r} a été mise à sac (an {y})"},
	6: {"icon": "jrn_revolte",    "col": COL_ETAT,  "fmt": "RÉVOLTE — un soulèvement éclate à {r} (an {y})"},   # {a} = "Rebelles de X" si la guerre civile est INCARNÉE (sinon générique) — cf. _poll_feed
	7: {"icon": "jrn_secession",  "col": COL_ETAT, "fmt": "SÉCESSION — {a} proclame son indépendance (an {y})"},
	8: {"icon": "jrn_bataille",   "col": COL_ARMEE, "fmt": "BATAILLE GAGNÉE contre {b} à {r} (an {y})"},
	9: {"icon": "jrn_bataille",   "col": COL_ARMEE, "fmt": "BATAILLE PERDUE contre {b} — l'ost est brisé à {r} (an {y})"},
	10: {"icon": "jrn_catastrophe", "col": COL_ETAT, "fmt": "{label} — {r} (an {y})"},   # ÉVÈNEMENT du directeur (inondation/peste/creuset…)
	11: {"icon": "jrn_bataille",  "col": COL_ARMEE, "fmt": "BATAILLE INDÉCISE contre {b} à {r} (an {y})"},
}
## kinds MAJEURS → popup OYEZ OYEZ (pause + boutons adaptatifs) au lieu d'un chip.
const POPUP_KINDS := [1, 2, 6, 7, 10]   # guerre · paix (verdict) · révolte · sécession · directeur

var _alerts := []    ## [{icon, col, tip, act, …}] conditions, recalculées à chaque _refresh
var _events := []    ## [{icon, col, tip, seq}] fil transient (clic = acquitté)
var _seen_seq := 0   ## dernier seq lu du fil

## LE JOURNAL — ring PERSISTANT (JOURNAL_MAX), le plus récent en TÊTE (push_front) ;
## display-only, jamais sérialisé. Alimenté par CHAQUE notification colorée : les
## évènements du fil (_poll_feed, TOUS les kinds — chip ET popup) et les conditions
## dès leur APPARITION (_journal_track_conditions, anti-spam par clé stable). Toute
## notification qui apparaît en chip/popup DOIT donc s'y retrouver — aucune n'existe
## qu'en éphémère (règle joueur).
var _journal := []
var _journal_seq := 0
var _prev_cond_keys := {}   ## clés des conditions actives au refresh précédent (edge-detection)

## COMPAT : ce nœud pouvait jadis dessiner une colonne flottante sur la carte (ledger_mode
## false) OU déléguer à la bande droite (true) ; le rendu flottant a été retiré, la bande
## droite est l'unique surface. Conservé pour les appelants (main.gd, journal_audit.gd).
func set_ledger_mode(_on: bool) -> void:
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
	visible = false                             # nœud DATA-ONLY : il ne s'affiche jamais
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	Sim.generated.connect(func():
		_journal.clear(); _journal_seq = 0; _prev_cond_keys.clear()
		_refresh())
	Sim.ticked.connect(func(_y): _refresh())
	_refresh.call_deferred()

## recalcule la RÉSOLUTION (conditions + fil transient + journal) puis notifie la bande
## droite (empire_sidebar.gd). Data-only : aucun visible/position/redraw ici.
func _refresh() -> void:
	# GATE : tant que la PARTIE n'a pas commencé (menu/setup) OU en MODE OBSERVATEUR,
	# aucune alerte ni popup — le monde de fond tourne pour la vitrine, ses évènements
	# ne concernent pas le joueur. OBSERVATEUR (2026-07-30) : `_collect()` lit
	# `w.player()` qui garde le SLOT DE FOCUS (empire 0, cf. scps_set_observer) même
	# sans joueur humain — sans ce gate, les conditions d'un empire IA (conseil vacant,
	# guerre, pénurie…) s'afficheraient comme SI c'était « nous ». Les évènements
	# WAR/PEACE/BATTLE/etc du fil moteur sont déjà gatés côté C (human_player>=0,
	# scps_sim.c) mais PAS le FEED_DIRECTOR (évènements du directeur, tous pays
	# confondus, filtrés seulement par feed_set_focus=player()) — ce gate GDScript
	# est donc la SEULE protection contre un popup « OYEZ OYEZ » usurpé en observateur.
	if not Sim.game_on or _observing():
		_alerts = []
		_events = []
		if Sim.world != null and Sim.world.has_method("feed_poll"):
			for ev in Sim.world.feed_poll(_seen_seq):   # on JETTE le fil (acquitté, jamais affiché)
				_seen_seq = maxi(_seen_seq, int(ev["seq"]))
		ledger_changed.emit()
		return
	_alerts = _collect()
	_journal_track_conditions(_alerts)
	_poll_feed()
	ledger_changed.emit()

## même check que main.gd::_observing() — dupliqué ici (ce script enfant n'a pas de
## référence à Main), cf. TROUVAILLES « Menu audio + mode observateur ».
func _observing() -> bool:
	return Sim.world != null and Sim.world.has_method("is_observer") and Sim.world.is_observer()

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
		# W2-7 (rapport joueur F5) : {r} est LE LIEU, jamais son index moteur. La façade
		# rend déjà le nom (toponyme → province → repli) dans « region_name » ; le repli
		# sur l'index ne sert qu'aux DLL d'avant cette vague.
		var rname := String(ev.get("region_name", ""))
		if rname == "":
			rname = str(int(ev["region"]))
		var tip := String(k["fmt"]).replace("{a}", String(ev["a"])).replace("{b}", String(ev["b"])) \
			.replace("{r}", rname).replace("{y}", str(int(ev["year"]))) \
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

## LE NOM D'UN LIEU (W2-7, rapport joueur F5) — un seul appel façade (`region_label` :
## toponyme → nom de province → mot de repli), jamais l'index moteur. Repli sur l'index
## pour une DLL d'avant cette vague, seul cas où un nombre peut encore paraître.
func _region_label(r: int) -> String:
	if r < 0:
		return ""
	var w = Sim.world
	if w != null and w.has_method("region_label"):
		var nm := String(w.region_label(r))
		if nm != "":
			return nm
	return str(r)

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
	# W2-7 (rapport joueur F8) : le corps du popup REPRENAIT le titre mot pour mot
	# (« La forêt brûle » / « La forêt brûle — Marbrive (an 4) »). Le titre porte déjà
	# le nom de l'évènement : le corps ne garde que ce qui s'y AJOUTE (le lieu, l'an).
	var body := tip
	if title != "" and body.begins_with(title):
		body = body.substr(title.length()).lstrip(" —·-")
	return {"title": title, "body": body, "buttons": btns, "kind": kind}


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
		out.append({"icon": "jrn_conseil", "col": COL_ETAT, "act": "council",
			"tip": "%d siège(s) du conseil VACANT(S) — recruter un candidat (clic : onglet Conseil)" % vac})
	# ÉTATIQUE — un âge s'est levé et n'est pas engagé
	if w.has_method("age_state"):
		var ag: Dictionary = w.age_state()
		if int(ag.get("age", -1)) >= 0 and not bool(ag.get("engaged", true)):
			out.append({"icon": "politics_crown", "col": COL_ETAT, "act": "age",
				"tip": "Un âge s'est levé : %s — clic : lire le chapitre" % String(ag.get("name", ""))})
	# SAVOIR — aucune recherche en cours
	var rs: Dictionary = w.research_status()
	if int(rs.get("target", -1)) < 0:
		out.append({"icon": "jrn_decouverte", "col": COL_SAVOIR, "act": "tech",
			"tip": "Aucune RECHERCHE en cours — clic : choisir une cible dans l'arbre"})
	# SOCIAL — au moins un édifice CONSTRUCTIBLE (débloqué + or suffisant)
	var ci: Dictionary = w.country_info(me)
	var gold: float = float(ci.get("or", 0))
	for b in w.building_roster(me):
		if bool(b.get("debloque", false)) and float(b.get("gold", 1e18)) <= gold:
			out.append({"icon": "action_build", "col": COL_SOCIAL, "act": "construct",
				"tip": "Un ÉDIFICE est constructible (ex. %s, %d couronnes) — clic : panneau Construction" % [String(b.get("nom", "")), int(b.get("gold", 0))]})
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
		out.append({"icon": "jrn_foi", "col": COL_FOI, "act": "religion",
			"tip": "Votre peuple a bâti son premier sanctuaire — clic : FONDER la foi"})
	# ── CONDITIONS MOTEUR (un seul appel C : révolte · famine · siège · prix · conso) ──
	if w.has_method("player_alerts"):
		var pa: Dictionary = w.player_alerts()
		if int(pa.get("revolt_region", -1)) >= 0:
			out.append({"icon": "jrn_revolte", "col": COL_ETAT, "act": "goto",
				"region": int(pa["revolt_region"]),
				"tip": "%s GRONDE (agitation %d) — réprimer, assimiler ou apaiser (clic : y aller)" % [_region_label(int(pa["revolt_region"])), int(pa["revolt_agit"])]})
		if int(pa.get("famine_region", -1)) >= 0:
			out.append({"icon": "jrn_catastrophe", "col": COL_SOCIAL, "act": "goto",
				"region": int(pa["famine_region"]),
				"tip": "FAMINE — %s ne mange qu'à %d %% (greniers, import, colonie vivrière) (clic : y aller)" % [_region_label(int(pa["famine_region"])), int(pa["famine_pct"])]})
		if int(pa.get("siege_region", -1)) >= 0:
			out.append({"icon": "jrn_siege", "col": COL_ARMEE, "act": "goto",
				"region": int(pa["siege_region"]),
				"tip": "SIÈGE — %s assiège %s ! Lever l'ost (clic : y aller)" % [String(pa["siege_by"]), _region_label(int(pa["siege_region"]))]})
		if int(pa.get("price_good", -1)) >= 0:
			out.append({"icon": "jrn_commerce", "col": COL_ECO, "act": "market",
				"tip": "PRIX EXORBITANT — %s à ×%.1f de l'ancre au marché (clic : onglet Marché)" % [String(pa["price_name"]), float(pa["price_x10"]) / 10.0]})
		if int(pa.get("conso_good", -1)) >= 0:
			out.append({"icon": "jrn_commerce", "col": COL_ECO, "act": "market",
				"tip": "BIEN INTROUVABLE — %s est demandé mais ni produit ni en stock (clic : onglet Marché)" % String(pa["conso_name"])})
	# ÉCONOMIE — PÉNURIE CRITIQUE (retour joueur UI-2 : « Fer : rupture dans 12 jours »
	# remonte en alerte explicite) : moins de 30 jours de couverture au rythme actuel,
	# même dérivation que la cellule « déficit » de la topbar (Topbar.worst_shortage).
	var short := Topbar.worst_shortage(w, me)
	if not short.is_empty() and int(short["days"]) < 30:
		out.append({"icon": "jrn_commerce", "col": COL_ECO, "act": "market",
			"tip": "%s : rupture dans %d jours au rythme actuel (clic : onglet Marché)" % [
				String(short["name"]), int(short["days"])]})
	# UI-MONNAIE (2026-07-16) — U4 : LES ÉVÉNEMENTS MONÉTAIRES. CONDITIONS polées (motif
	# ci-dessus, pas un nouveau canal moteur) : lecteurs PURS (country_bankruptcy_scar/
	# country_debase_frac), l'édge-detection existante (_journal_track_conditions) fait
	# le travail « apparition → une ligne au journal ». Découvertes d'or (M7, EVID_GOLD_
	# DISCOVERY) : DÉJÀ un dilemme à part entière (pending_event/player_event_choice —
	# « Proclamer la découverte »), plus visible qu'une ligne de journal — non dupliqué ici.
	if w.has_method("country_bankruptcy_scar") and float(w.country_bankruptcy_scar(me)) > 0.01:
		out.append({"icon": "jrn_tresor", "col": COL_ECO, "act": "market",
			"tip": "BANQUEROUTE — tes créanciers saisissent une part de ta production (cicatrice active, onglet Monnaie)"})
	if w.has_method("country_debase_frac") and float(w.country_debase_frac(me)) > 0.001:
		out.append({"icon": "jrn_tresor", "col": COL_ECO, "act": "market",
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
				out.append({"icon": "jrn_tresor", "col": COL_ECO, "act": "",
					"tip": "%s traverse une BANQUEROUTE — ses créanciers saisissent sa production" % rnm})
			if w.has_method("country_debase_frac") and float(w.country_debase_frac(rcid)) > 0.001:
				out.append({"icon": "jrn_tresor", "col": COL_ECO, "act": "",
					"tip": "%s DÉBASE sa monnaie — sur-frappe au-delà de la parité" % rnm})
	return out

## VOIE MÉTABOLISATION (V1b) : tech_panel.gd notifie qu'un héritage NON natif vient
## d'atteindre tier 3 (digestion pleine) — chip transient discret, même motif que le fil
## moteur (`_events`) mais poussé DIRECTEMENT (ce n'est pas un feed C, c'est un latch
## GDScript côté tech_panel). Clic = ouvre l'arbre tech sur la bande de métabolisation.
func push_metab_ready(nom: String) -> void:
	_seen_seq += 1   # partage la numérotation de seq (clic = acquitté, comme le fil moteur)
	var tip := "Métabolisation : %s prête (clic : voir l'arbre)" % nom
	_events.append({"icon": "jrn_decouverte", "col": COL_SAVOIR, "tip": tip,
		"seq": _seen_seq, "act": "tech_metab"})
	while _events.size() > FEED_MAX:
		_events.pop_front()
	_push_journal({"icon": "jrn_decouverte", "col": COL_SAVOIR, "tip": tip,
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

## label COURT dérivé du tip (coupé au tiret/parenthèse, tronqué) — le texte VISIBLE
## de la « letter » (façon RimWorld : la notification se lit sans survol).
func _short(tip: String) -> String:
	var s := tip
	# W2-7 (rapport joueur F9) : la coupe au PREMIER « — » jetait exactement ce qui
	# distingue deux lignes — le LIEU et le camp (« Une place est TOMBÉE » ×7, toutes
	# identiques). On ne coupe plus qu'à la parenthèse (l'an, déjà en préfixe de ligne) ;
	# le reste tient à la longueur.
	var cut := s.find(" (")
	if cut > 0:
		s = s.substr(0, cut)
	# 42 au lieu de 26 : « 3 siège(s) du conseil VAC… » se lisait tronqué (retour
	# joueur 2026-07-10) — le cartouche s'élargit à son texte, on peut le laisser dire.
	# W2-7 (F18) : la troncature tombe sur un ESPACE, jamais au milieu d'un mot.
	if s.length() > 42:
		var head := s.substr(0, 41)
		var sp := head.rfind(" ")
		s = (head.substr(0, sp) if sp > 20 else head)
		# … et jamais un séparateur orphelin juste avant les points de suspension
		# (« conseil VACANT(S) —… »).
		s = s.rstrip(" —·:,-") + "…"
	return s

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
