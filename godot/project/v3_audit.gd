extends Node
## v3_audit — probe headless pour le lot V3 : LES LAVIS PAR VARIANTE + LE CÂBLAGE
## SERVILE. Vérifie que sidebar_drawer.gd / iso_ground.gd compilent, que le binding
## servile (manumit/slave_buy/slave_sell/slave_market) fait un aller-retour réel,
## et que variant_map_image()/endgame_region_intensity() sont cohérents et bornés.
##
## Lancer : godot --headless res://v3_audit.tscn -- seed=9,11,42 years=30

func _ready() -> void:
	get_window().size = Vector2i(320, 240)
	_run.call_deferred()

func _seeds() -> Array:
	for a in OS.get_cmdline_user_args():
		if a.begins_with("seed="):
			var out: Array = []
			for s in a.substr(5).split(","):
				out.append(int(s))
			return out
	return [9, 11, 42]

func _years() -> int:
	for a in OS.get_cmdline_user_args():
		if a.begins_with("years="):
			return int(a.substr(6))
	return 30

func _run() -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	if Sim.world == null:
		push_error("v3_audit: pas de monde (libscps absente ?)")
		get_tree().quit(2)
		return
	print("=== V3 AUDIT — lavis par variante + câblage servile ===")
	var total := 0
	# INVARIANT 0 : les panneaux qui CONSOMMENT la membrane compilent.
	for scr in ["res://ui/sidebar_drawer.gd", "res://map/iso_ground.gd", "res://ui/codex.gd"]:
		if load(scr) == null:
			push_error("v3_audit: " + scr + " ne compile pas")
			total += 1
	for sd in _seeds():
		total += _audit_seed(sd, _years())
	print("")
	print("V3 AUDIT OK" if total == 0 else ("V3 AUDIT : " + str(total) + " VIOLATION(S)"))
	get_tree().quit(0 if total == 0 else 1)

func _audit_seed(sd: int, years: int) -> int:
	Sim.regenerate(sd)
	var w = Sim.world
	for _i in range(years):
		w.advance_days(360)
	var me: int = w.player()
	var viol := 0
	var flags := ""

	# ── LOT 1 — LE LAVIS PAR VARIANTE ──────────────────────────────────────
	var eg: Dictionary = w.endgame_info()
	var fin_raw := int(eg.get("fin_raw", 0))
	if fin_raw < 0 or fin_raw > 5:
		viol += 1; flags += " ✗fin_raw(" + str(fin_raw) + ")"
	var nreg: int = w.region_count()
	var any_intense := false
	for r in range(nreg):
		var inten: float = w.endgame_region_intensity(r)
		if inten < 0.0 or inten > 1.0:
			viol += 1; flags += " ✗intensity(" + str(r) + "=" + str(inten) + ")"
		if inten > 0.01:
			any_intense = true
	var vimg: Image = w.variant_map_image()
	if vimg == null:
		viol += 1; flags += " ✗variant_map-null"
	else:
		if fin_raw == 0:
			# aucune fin en cours : coût nul, la carte doit rester tout-0 (pas de lavis fantôme).
			var d: PackedByteArray = vimg.get_data()
			var any_nonzero := false
			for b in d:
				if b != 0:
					any_nonzero = true
					break
			if any_nonzero:
				viol += 1; flags += " ✗variant-non-nul-sans-fin"
		elif any_intense:
			# une fin est en cours ET au moins une région intense : la carte doit le montrer.
			var d2: PackedByteArray = vimg.get_data()
			var any_px := false
			for b2 in d2:
				if b2 > 2:
					any_px = true
					break
			if not any_px:
				viol += 1; flags += " ✗variant-plat-malgre-intensite"

	# ── LOT 2 — LE CÂBLAGE SERVILE ─────────────────────────────────────────
	var mp: Dictionary = w.manumit_preview()
	var souls := int(mp.get("souls", 0))
	if souls < 0:
		viol += 1; flags += " ✗souls-negatif"
	var pct: float = float(mp.get("pct_of_country", 0.0))
	if pct < 0.0 or pct > 100.0:
		viol += 1; flags += " ✗pct-hors-bornes"

	var mk: Dictionary = w.slave_market()
	var total_pool := int(mk.get("total", 0))
	if total_pool < 0:
		viol += 1; flags += " ✗pool-negatif"
	for ln in mk.get("lines", []):
		if String(ln.get("heritage", "")) == "":
			viol += 1; flags += " ✗ligne-heritage-vide"
		if int(ln.get("count", -1)) < 0:
			viol += 1; flags += " ✗ligne-count-negatif"

	# ROUND-TRIP : les 3 verbes s'ENFILENT (le drain tranche, silencieux si refusé —
	# c'est la plomberie qu'on prouve, comme scps_api_demo).
	var cap_prov: int = w.country_capital_province(me)
	var cap_region: int = w.province_region(cap_prov) if cap_prov >= 0 else -1
	if cap_region >= 0:
		if not bool(w.player_manumit()):
			viol += 1; flags += " ✗manumit-refuse-enqueue"
		w.advance_days(2)
		if not bool(w.player_slave_sell(cap_region, 50)):
			viol += 1; flags += " ✗slave_sell-refuse-enqueue"
		w.advance_days(2)
		if not bool(w.player_slave_buy(cap_region, 50)):
			viol += 1; flags += " ✗slave_buy-refuse-enqueue"
		w.advance_days(2)
	else:
		flags += " (pas de capitale — verbes servile ignorés)"

	print("seed ", sd, " an ", w.year(), " | fin_raw ", fin_raw, " | souls ", souls,
		" | pool ", total_pool, (flags if flags != "" else " | OK"))
	return viol
