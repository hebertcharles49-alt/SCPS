extends RefCounted
## Epithet — dérive un SURNOM du règne depuis le CONTENU des Annales (comptage de
## kinds, déterministe : mêmes annales → même épithète). Display-only, pur GDScript,
## zéro logique de sim (on ne fait que COMPTER ce que la façade a déjà sélectionné).
## AnnalKind (scps/scps_events.h) : 0=dilemme 1=cicatrice 2=âge 3=guerre gagnée
## 4=guerre perdue 5=sécession 6=hégémon brisé (réservé) 7=monument 8=fin/merveille.

const K_DILEMME := 0
const K_CICATRICE := 1
const K_AGE := 2
const K_GUERRE_GAGNEE := 3
const K_GUERRE_PERDUE := 4
const K_SECESSION := 5
const K_HEGEMON_BRISE := 6
const K_MONUMENT := 7
const K_FIN := 8

## rend l'épithète (sans « le »/« la » — déjà inclus) à partir d'un annals() complet
## ou d'une TRANCHE (récap d'âge). `n` < 3 ⇒ toujours « le Discret » (rien à raconter).
static func derive(entries: Array) -> String:
	if entries.size() < 3:
		return "le Discret"
	var n_guerre := 0
	var n_perdue := 0
	var n_cicatrice := 0
	var n_dilemme := 0
	var n_monument := 0
	var n_fin := 0
	var n_secession := 0
	for e in entries:
		match int(e.get("kind", -1)):
			K_GUERRE_GAGNEE: n_guerre += 1
			K_GUERRE_PERDUE: n_perdue += 1
			K_CICATRICE: n_cicatrice += 1
			K_DILEMME: n_dilemme += 1
			K_MONUMENT: n_monument += 1
			K_FIN: n_fin += 1
			K_SECESSION: n_secession += 1
	var total := entries.size()
	var n_guerres_totales := n_guerre + n_perdue

	# LA FIN DU MONDE / L'ASCENSION domine tout — le fait le plus lourd du règne.
	if n_fin > 0:
		return "le Visionnaire" if n_monument >= n_guerres_totales else "l'Apocalyptique"
	# BEAUCOUP de guerres + BEAUCOUP de cicatrices : la conquête a un prix — le Sanglant.
	if n_guerres_totales >= 5 and n_cicatrice >= 3:
		return "le Sanglant"
	if n_guerre >= 4 and n_guerre > n_perdue * 2:
		return "le Conquérant"
	if n_perdue >= 3 and n_perdue > n_guerre:
		return "l'Assiégé"
	if n_secession >= 2:
		return "le Fragmenté"
	if n_monument >= 3:
		return "le Bâtisseur"
	if n_dilemme >= 6 and n_guerres_totales <= 1:
		return "le Prudent"
	if n_dilemme >= 4:
		return "l'Arbitre"
	if n_cicatrice >= 4:
		return "le Marqué"
	if float(n_guerres_totales) / float(maxi(total, 1)) > 0.5:
		return "le Belliqueux"
	if total <= 5:
		return "le Discret"
	return "le Chroniqué"

## la tranche d'annales dont l'année est STRICTEMENT postérieure à `since_year`
## (récap d'âge : ce qui s'est passé DEPUIS le dernier engagement).
static func slice_since(entries: Array, since_year: int) -> Array:
	var out := []
	for e in entries:
		if int(e.get("year", -1000000)) > since_year:
			out.append(e)
	return out
