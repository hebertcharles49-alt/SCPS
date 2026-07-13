extends Node
## NavigationHub — historique de VUES, sans connaissance des panneaux ni du moteur.

const InfoRef = preload("res://ui/info_ref.gd")
const HISTORY_LIMIT := 48
const RECENT_LIMIT := 24
const PIN_LIMIT := 16
const COMPARE_KINDS := [InfoRef.COUNTRY, InfoRef.PROVINCE, InfoRef.CORPS, InfoRef.RESOURCE]

signal navigate_requested(request: Dictionary)
signal history_changed(can_back: bool, can_forward: bool)
signal memory_changed()

var _back: Array[Dictionary] = []
var _forward: Array[Dictionary] = []
var _current: Dictionary = {}
var _recent: Array[Dictionary] = []
var _pins: Array[Dictionary] = []
var _compare: Array[Dictionary] = []

func go(request: Dictionary, record_history: bool = true) -> bool:
	var k := InfoRef.request_key(request)
	if k == "":
		return false
	var clean: Dictionary = request.duplicate(true)
	# La mémoire est une surface de travail, pas un objet consulté : l'ouvrir ne doit
	# ni écraser la vue courante à épingler, ni polluer l'historique des objets.
	if String((clean.get("ref", {}) as Dictionary).get("kind", "")) == InfoRef.MEMORY:
		navigate_requested.emit(clean)
		return true
	if InfoRef.request_key(_current) == k:
		# Un second clic doit tout de même rendre la vue visible si elle a été fermée.
		navigate_requested.emit(clean)
		return true
	if record_history and not _current.is_empty():
		_back.append(_current.duplicate(true))
		if _back.size() > HISTORY_LIMIT:
			_back.pop_front()
		_forward.clear()
	_current = clean
	_remember(clean)
	navigate_requested.emit(clean.duplicate(true))
	_emit_history()
	return true

func back() -> bool:
	if _back.is_empty():
		return false
	if not _current.is_empty():
		_forward.append(_current.duplicate(true))
	_current = _back.pop_back()
	_remember(_current)
	navigate_requested.emit(_current.duplicate(true))
	_emit_history()
	return true

func forward() -> bool:
	if _forward.is_empty():
		return false
	if not _current.is_empty():
		_back.append(_current.duplicate(true))
	_current = _forward.pop_back()
	_remember(_current)
	navigate_requested.emit(_current.duplicate(true))
	_emit_history()
	return true

func clear() -> void:
	_back.clear()
	_forward.clear()
	_current.clear()
	_emit_history()

func can_back() -> bool:
	return not _back.is_empty()

func can_forward() -> bool:
	return not _forward.is_empty()

func current_request() -> Dictionary:
	return _current.duplicate(true)

func recent_requests() -> Array[Dictionary]:
	return _recent.duplicate(true)

func pinned_requests() -> Array[Dictionary]:
	return _pins.duplicate(true)

func comparison_requests() -> Array[Dictionary]:
	return _compare.duplicate(true)

func is_pinned(request: Dictionary = {}) -> bool:
	var target := _current if request.is_empty() else request
	var key := InfoRef.request_key(target)
	for item in _pins:
		if InfoRef.request_key(item) == key:
			return true
	return false

func toggle_pin(request: Dictionary = {}) -> bool:
	var target := _current if request.is_empty() else request
	var key := InfoRef.request_key(target)
	if key == "":
		return false
	for i in range(_pins.size()):
		if InfoRef.request_key(_pins[i]) == key:
			_pins.remove_at(i)
			memory_changed.emit()
			return false
	_pins.push_front(target.duplicate(true))
	if _pins.size() > PIN_LIMIT:
		_pins.pop_back()
	memory_changed.emit()
	return true

func remove_pin(index: int) -> bool:
	if index < 0 or index >= _pins.size():
		return false
	_pins.remove_at(index)
	memory_changed.emit()
	return true

func add_compare(request: Dictionary = {}) -> bool:
	var target := _current if request.is_empty() else request
	var ref = target.get("ref", {})
	if not (ref is Dictionary) or not COMPARE_KINDS.has(String(ref.get("kind", ""))):
		return false
	var key := InfoRef.request_key(target)
	for item in _compare:
		if InfoRef.request_key(item) == key:
			return true
	if not _compare.is_empty():
		var first_ref: Dictionary = _compare[0].get("ref", {})
		if String(first_ref.get("kind", "")) != String(ref.get("kind", "")):
			_compare.clear()
	if _compare.size() >= 2:
		_compare.remove_at(1)
	_compare.append(target.duplicate(true))
	memory_changed.emit()
	return true

func remove_compare(index: int) -> bool:
	if index < 0 or index >= _compare.size():
		return false
	_compare.remove_at(index)
	memory_changed.emit()
	return true

func clear_compare() -> void:
	_compare.clear()
	memory_changed.emit()

func clear_memory() -> void:
	_recent.clear()
	_pins.clear()
	_compare.clear()
	memory_changed.emit()

func save_memory(slot: int) -> bool:
	if slot < 0:
		return false
	var cfg := ConfigFile.new()
	cfg.set_value("memory", "recent", _recent)
	cfg.set_value("memory", "pins", _pins)
	cfg.set_value("memory", "compare", _compare)
	return cfg.save("user://campaign_memory_%d.cfg" % slot) == OK

func load_memory(slot: int) -> bool:
	if slot < 0:
		return false
	var cfg := ConfigFile.new()
	if cfg.load("user://campaign_memory_%d.cfg" % slot) != OK:
		clear_memory()
		return false
	_recent = _valid_requests(cfg.get_value("memory", "recent", []), RECENT_LIMIT)
	_pins = _valid_requests(cfg.get_value("memory", "pins", []), PIN_LIMIT)
	_compare = _valid_requests(cfg.get_value("memory", "compare", []), 2)
	for i in range(_compare.size() - 1, -1, -1):
		var restored_ref: Dictionary = _compare[i].get("ref", {})
		if not COMPARE_KINDS.has(String(restored_ref.get("kind", ""))):
			_compare.remove_at(i)
	# Une comparaison restaurée doit rester homogène ; sinon elle repart vide.
	if _compare.size() == 2:
		var a: Dictionary = _compare[0].get("ref", {})
		var b: Dictionary = _compare[1].get("ref", {})
		if String(a.get("kind", "")) != String(b.get("kind", "")):
			_compare.clear()
	memory_changed.emit()
	return true

func _remember(request: Dictionary) -> void:
	var key := InfoRef.request_key(request)
	for i in range(_recent.size() - 1, -1, -1):
		if InfoRef.request_key(_recent[i]) == key:
			_recent.remove_at(i)
	_recent.push_front(request.duplicate(true))
	if _recent.size() > RECENT_LIMIT:
		_recent.pop_back()
	memory_changed.emit()

func _valid_requests(raw, limit: int) -> Array[Dictionary]:
	var out: Array[Dictionary] = []
	if not (raw is Array):
		return out
	for item in raw:
		if item is Dictionary and InfoRef.request_key(item) != "":
			var ref: Dictionary = (item as Dictionary).get("ref", {})
			if String(ref.get("kind", "")) == InfoRef.MEMORY:
				continue
			out.append((item as Dictionary).duplicate(true))
			if out.size() >= limit:
				break
	return out

func _emit_history() -> void:
	history_changed.emit(can_back(), can_forward())
