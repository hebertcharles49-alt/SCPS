#!/usr/bin/env python3
# gen_content.py — SCALAFFOLDAGE de CONTENU pour SCPS (palier 3 du modtools).
#
# Le moteur est statiquement dimensionné (déterminisme + save byte-identique) : ajouter
# une RESSOURCE ne peut pas se faire à chaud — c'est « éditer le code + 1 recompile ».
# Cet outil ne MODIFIE PAS le code : il LIT un manifeste et IMPRIME les lignes EXACTES à
# coller (enum + toutes les tables), pour ne RIEN oublier, plus la checklist (bump SAVE,
# golden). Règle d'or respectée : on AJOUTE en QUEUE d'enum (jamais au milieu).
#
#   python tools/gen_content.py tools/content_example.tsv
#
# Format du manifeste (TSV, '#'/vide ignoré) :
#   resource <TAB> <Nom affiché> <TAB> <RES_ENUM> <TAB> price=.. <TAB> yield=.. <TAB> food=0|1
#
# (bâtiments / unités / techs suivent le même motif enum+table — cf. MODDING.md ; non
#  scaffoldés ici pour rester simple et sûr : ils touchent des recettes/prereqs structurels.)

import sys

# sortie UTF-8 (les blocs contiennent →, accents) — robuste sous Windows (cp1252 par défaut).
for _s in (sys.stdout, sys.stderr):
    try: _s.reconfigure(encoding="utf-8")
    except Exception: pass

def kv(tokens):
    d = {}
    for t in tokens:
        if "=" in t:
            k, v = t.split("=", 1)
            d[k.strip()] = v.strip()
    return d

def cflt(s, dflt):
    try: return f"{float(s)}f"      # littéral float C VALIDE (« 12 » → « 12.0f », pas « 12f »)
    except (TypeError, ValueError): return dflt

def gen_resource(name, enum, f):
    price = cflt(f.get("price"), "1.0f")
    yld   = cflt(f.get("yield"), "0.0f")
    food  = f.get("food", "0") in ("1", "true", "yes")
    print(f"\n========== RESSOURCE : {name}  ({enum}) ==========")
    print("// → scps/scps_types.h — enum Resource, JUSTE AVANT RES_COUNT (queue) :")
    print(f"    {enum},")
    print("// → scps/scps_econ.c — table BASE_PRICE[] :")
    print(f"    [{enum}]= {price},")
    print("// → scps/scps_econ.c — table EXTRACT_YIELD[] (0 = non extraite / manufacturée) :")
    print(f"    [{enum}]= {yld},")
    print("// → scps/scps_world.c — resource_name() (switch/table) :")
    print(f'    case {enum}: return "{name}";')
    if food:
        print("// → scps/scps_econ.c — res_is_food() : ajouter le test")
        print(f"    || r=={enum}")
    print("// CHECKLIST (sinon ça ne marche pas) :")
    print("//   [ ] resource_color() (scps_world.c) — une couleur pour la carte/UI")
    print("//   [ ] où elle SPAWNE : biome→raw_cap (econ_init) OU une recette qui la PRODUIT")
    print("//   [ ] qui la CONSOMME : une recette (RECIPE[]) ou un besoin (NEED[])")
    print("//   [ ] bump SAVE_VERSION (scps_save.h) — RES_COUNT change ⇒ structs [RES_COUNT] changent")
    print("//   [ ] make golden-update (re-baseline DÉLIBÉRÉE) + make test")

def main():
    if len(sys.argv) < 2:
        print("usage: python tools/gen_content.py <manifeste.tsv>", file=sys.stderr)
        return 2
    n = 0
    with open(sys.argv[1], encoding="utf-8") as fh:
        for raw in fh:
            line = raw.rstrip("\n\r")
            if not line or line.lstrip().startswith("#"):
                continue
            cols = line.split("\t")
            kind = cols[0].strip()
            if kind == "resource" and len(cols) >= 3:
                gen_resource(cols[1].strip(), cols[2].strip(), kv(cols[3:]))
                n += 1
            else:
                print(f"// (ignoré : type '{kind}' non scaffoldé — cf. MODDING.md §contenu)", file=sys.stderr)
    print(f"\n# {n} objet(s) scaffoldé(s). Colle les blocs ci-dessus, puis recompile.", file=sys.stderr)
    return 0

if __name__ == "__main__":
    sys.exit(main())
