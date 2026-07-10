extends Control
## DATE CHIP — la date seule (« Jour X · mois Y · an Z »), rafraîchie CHAQUE JOUR
## simulé. Extraite du topbar (2026-07-10) : lui reste à la cadence MENSUELLE
## anti-danse pour les chiffres — la date, elle, doit s'écouler jour par jour
## (elle sautait par paquets entre deux redraws mensuels/clics). Display-only.

const VKit = preload("res://ui/vkit.gd")

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP   # porte son tooltip (raccourcis temps)
	tooltip_text = "Espace : pause · +/− : vitesse"
	Sim.ticked.connect(func(_y): queue_redraw())
	Sim.generated.connect(queue_redraw)

func _draw() -> void:
	var w = Sim.world
	if w == null or not w.has_method("day_of_year"):
		return
	var doy := int(w.day_of_year())
	var txt := "Jour %d · mois %d · an %d" % [(doy % 30) + 1, mini(doy / 30 + 1, 12), int(w.year())]
	VKit.text(self, Vector2(0, (size.y - 18.0) * 0.5), VKit.COL_PARCH, txt)
