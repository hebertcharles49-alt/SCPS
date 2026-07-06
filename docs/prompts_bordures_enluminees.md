# Bordures de carte enluminées — prompts CODEX (par âge + écrans de fin)

## Spécifications techniques (toutes les images)
- **Format** : PNG 2048×2048, **centre entièrement transparent** (alpha) — seule la bordure est peinte.
- **9-slice** : coins ornés ~320×320 px ; les bords entre les coins doivent être **répétables** (motif périodique, pas de composition unique) pour l'étirement à toute résolution.
- **Épaisseur** : bord ~140-180 px (les coins peuvent déborder à ~320).
- **Style commun** (l'ancre DA, à inclure dans chaque prompt) : *medieval illuminated manuscript border on aged parchment, sepia and walnut ink linework, faded gold leaf accents, antique atlas cartouche style, hand-drawn engraving texture, muted earthy pigments (slate, rust, olive, ochre), burnt darkened edges, no text, no watermark, transparent center*.
- **Négatif** : pas de texte, pas de figures photoréalistes, pas de couleurs néon, centre vide.

---

## Les bordures par ÂGE (le cadre de la carte change quand l'âge se lève)

### 0. L'Aube (défaut, avant le premier âge)
> Medieval illuminated manuscript border on aged parchment, sepia and walnut ink, faded gold accents, antique atlas style, transparent center. A SIMPLE double-fillet frame with restrained scrollwork corners, small compass roses in each corner, thin knotwork along the edges, sparse and humble ornamentation — a young world, mostly empty margins, repeating edge pattern, no text.

### 1. L'Âge du Commerce
> Same base style. Border of grapevine scrollwork threaded with hanging COINS and small merchant scales, corner medallions of lateen-sail merchant ships and tied rope knots, tiny amphorae and bolts of cloth woven into the repeating edge vines, warm ochre and faded gold dominant, no text.

### 2. L'Âge de la Raison
> Same base style. Border of precise GEOMETRIC interlace — compasses, set squares and astrolabes in the corner medallions, thin Euclidean diagram flourishes (circles, triangles, dotted construction lines) repeating along the edges, restrained slate-blue ink accents over sepia, an air of measured order, no text.

### 3. L'Âge des Empires
> Same base style. Border of laurel garlands and fluted column segments, corner medallions of eagles atop standards and crossed ceremonial spears, a repeating frieze of small round shields and helmets along the edges, rust-red and antique gold accents, imperial gravitas, no text.

### 4. L'Âge de la Brèche
> Same base style, CORRUPTED: the illuminated scrollwork begins mid-edge as normal vine ornament then progressively UNRAVELS — lines left unfinished, arcane glyphs bleeding faint violet beneath the sepia ink, hairline cracks in the parchment leaking a dim glow at the corners, geometry subtly wrong (impossible knots), the margins « se déréalisent », no text.

### 5. L'Âge des Lumières
> Same base style. Border of engraved RADIATING light beams from the corner medallions (open books, oil lanterns, candles), repeating edge pattern of quills, printed-page motifs and small suns, brighter parchment, gold leaf more present, hopeful clarity, no text.

### 6. L'Âge des Soulèvements
> Same base style, agitated: border of interlaced PIKES and scythes, BROKEN CHAINS as the repeating edge motif, corner medallions of raised fists holding torches drawn as woodcut figures, rough hurried linework as if engraved in haste, rust and charcoal accents, no text.

### 7. L'Âge de l'Ordre de Fer
> Same base style, oppressive: border of riveted IRON BANDS and heavy nail heads, taut chains and portcullis grids as the repeating edge motif, corner medallions of closed helmets and manacles, cold slate-grey ink over darkened parchment, rigid perfect regularity — beauty turned carceral, no text.

---

## Les bordures des ÉCRANS DE FIN (épilogue / page finale)

### 8. L'Engloutissement (EAU)
> Same base style, DROWNING: illuminated WAVES climb and swallow the lower border, the sepia ink visibly DILUTES and bleeds downward like wet parchment, corner medallions of abyssal fish and nautilus shells, whirlpool spirals at the bottom corners, the top edge still dry and orderly — the world sinking from below, slate-blue washes, no text.

### 9. Le Grand Hiver (FROID)
> Same base style, FROZEN: delicate FROST LACE creeping inward from all edges, ice crystals and bare black branches as the repeating motif, corner medallions of snowflake rosettes, the parchment BLEACHED pale and the ink faded as if the page itself is freezing, faint blue-white highlights, stillness, no text.

### 10. Les Ronces (RONCES)
> Same base style, OVERGROWN: the vegetal illumination has turned MONSTROUS — thorned brambles devour the frame, black leaves and venomous flowers strangle the corner medallions, tendrils intrude into the transparent center at the edges, dense dark olive and brown-black ink, the ornament eating the page, no text.

### 11. Le Sang (SANG)
> Same base style, EXHAUSTED: a frieze of BROKEN swords and cracked shields along the edges, dark terracotta stains soaking into the parchment like dried blood, corner medallions of empty helmets and unstrung bows, parts of the ornament FADING as if the page is forgetting the names it held, rust and charcoal, weary not furious, no text.

### 12. L'Ascension (MERVEILLE)
> Same base style, TRANSCENDENT: aged-GOLD dominant, perfect sacred geometry (interlocking gears and star polygons, dwemer-like precision) forming the lower border, then progressively DISSOLVING into fine gold particles rising toward the top edge — the top corners left luminously UNFINISHED, the frame itself departing, ambiguous glory, no text.

---

## Câblage prévu (quand les assets arrivent)
- `godot/project/art/borders/age_{aube,commerce,raison,empires,breche,lumieres,soulevements,ordrefer}.png` + `fin_{eau,froid,ronces,sang,ascension}.png`.
- Un `ui/border_art.gd` (motif exact d'`event_art.gd` : mapping + cache + fallback l'Aube) ; affichage en NinePatchRect plein viewport sous l'UI, au-dessus de la carte ; swap au signal d'âge (le même point que le récap/page-turn) et sur l'écran d'épilogue.
