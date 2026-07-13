extends RefCounted
## Classement textuel partagé par le Codex et la palette Ctrl+K.
## Pur et déterministe : aucune lecture du monde, aucune mutation.

static func normalize(text: String) -> String:
	var s := text.strip_edges().to_lower()
	for pair in [["à", "a"], ["â", "a"], ["ä", "a"], ["á", "a"],
		["ç", "c"], ["é", "e"], ["è", "e"], ["ê", "e"], ["ë", "e"],
		["î", "i"], ["ï", "i"], ["í", "i"], ["ô", "o"], ["ö", "o"],
		["ù", "u"], ["û", "u"], ["ü", "u"], ["œ", "oe"], ["æ", "ae"]]:
		s = s.replace(pair[0], pair[1])
	for mark in ["·", "—", "–", "-", "/", "(", ")", "[", "]", ":", ";", ",", ".", "'", "’", "\""]:
		s = s.replace(mark, " ")
	while s.contains("  "):
		s = s.replace("  ", " ")
	return s.strip_edges()

## -1 = aucun résultat. Le titre exact/prefixe domine, puis la sous-chaîne,
## puis la présence de tous les mots dans le texte étendu.
static func score(query: String, title: String, text: String = "") -> int:
	var q := normalize(query)
	if q == "":
		return 1
	var nt := normalize(title)
	var hay := normalize(title + " " + text)
	if nt == q:
		return 1200
	if nt.begins_with(q):
		return 1000 - mini(nt.length() - q.length(), 100)
	var title_pos := nt.find(q)
	if title_pos >= 0:
		return 850 - mini(title_pos, 100)
	var pos := hay.find(q)
	if pos >= 0:
		return 700 - mini(pos, 150)
	var total := 520
	for token in q.split(" ", false):
		var token_pos := hay.find(token)
		if token_pos < 0:
			return -1
		total -= mini(token_pos / 8, 25)
	return total
