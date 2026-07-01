// SLP 2.0N decoder → BMP (no dependencies needed)
// Handles 8-bit (palette) and 32-bit (BGRA) frames
// Usage: node slp_decode.js <file.slp> <outdir>
const fs = require('fs');
const path = require('path');

// AoE2 default palette (50500) — 256 RGB entries, most common colors
// This is a standard approximation; exact colors may differ slightly
const PAL = buildDefaultPalette();

function buildDefaultPalette() {
    // Generate a reasonable AoE2-ish palette (the real one is in 50500.bina)
    // We'll try to load it from the game files first
    const p = new Uint8Array(256 * 3);
    // Fallback: greyscale + earth tones
    for (let i = 0; i < 256; i++) {
        p[i*3+0] = i; p[i*3+1] = i; p[i*3+2] = i;
    }
    return p;
}

function tryLoadPalette(opts) {
    const paths = opts.paths || [];
    for (const pp of paths) {
        if (fs.existsSync(pp)) {
            const data = fs.readFileSync(pp);
            // Check if it's a JASC palette
            const txt = data.toString('ascii', 0, Math.min(20, data.length));
            if (txt.startsWith('JASC-PAL')) {
                const lines = data.toString().split(/\r?\n/);
                const n = parseInt(lines[2]) || 256;
                const pal = new Uint8Array(256 * 3);
                for (let i = 0; i < Math.min(n, 256); i++) {
                    const parts = (lines[3 + i] || '').split(/\s+/);
                    if (parts.length >= 3) {
                        pal[i*3+0] = parseInt(parts[0]) || 0;
                        pal[i*3+1] = parseInt(parts[1]) || 0;
                        pal[i*3+2] = parseInt(parts[2]) || 0;
                    }
                }
                console.log(`  Loaded palette: ${pp} (${n} entries)`);
                return pal;
            }
            // Binary palette: 256 * 4 bytes (RGBX)
            if (data.length >= 1024) {
                const pal = new Uint8Array(256 * 3);
                for (let i = 0; i < 256; i++) {
                    pal[i*3+0] = data[i*4+0];
                    pal[i*3+1] = data[i*4+1];
                    pal[i*3+2] = data[i*4+2];
                }
                console.log(`  Loaded palette: ${pp} (binary)`);
                return pal;
            }
        }
    }
    return null;
}

function writeBMP(fname, w, h, pixels) {
    // pixels = Uint8Array of w*h*4 (RGBA, top-to-bottom)
    const rowBytes = w * 4;
    const padRow = (rowBytes + 3) & ~3;
    const imgSize = padRow * h;
    const fileSize = 54 + imgSize;
    const buf = Buffer.alloc(fileSize);
    // BMP header
    buf.write('BM', 0);
    buf.writeUInt32LE(fileSize, 2);
    buf.writeUInt32LE(54, 10); // offset to pixel data
    // DIB header (BITMAPINFOHEADER)
    buf.writeUInt32LE(40, 14);
    buf.writeInt32LE(w, 18);
    buf.writeInt32LE(-h, 22); // negative = top-down
    buf.writeUInt16LE(1, 26);  // planes
    buf.writeUInt16LE(32, 28); // bits per pixel
    buf.writeUInt32LE(0, 30);  // compression (BI_RGB)
    buf.writeUInt32LE(imgSize, 34);
    // pixel data (BGRA)
    for (let y = 0; y < h; y++) {
        for (let x = 0; x < w; x++) {
            const si = (y * w + x) * 4;
            const di = 54 + y * padRow + x * 4;
            buf[di + 0] = pixels[si + 2]; // B
            buf[di + 1] = pixels[si + 1]; // G
            buf[di + 2] = pixels[si + 0]; // R
            buf[di + 3] = pixels[si + 3]; // A
        }
    }
    fs.writeFileSync(fname, buf);
}

function decodeSLP(file, outdir, palette) {
    const buf = fs.readFileSync(file);
    const ver = buf.toString('ascii', 0, 4);
    const numFrames = buf.readUInt32LE(4);
    const base = path.basename(file, '.slp');
    console.log(`${base}: ${ver}, ${numFrames} frames`);

    let extracted = 0;
    for (let fi = 0; fi < numFrames; fi++) {
        const off = 32 + fi * 32;
        const cmdOff = buf.readUInt32LE(off + 0);
        const outlineOff = buf.readUInt32LE(off + 4);
        const props = buf.readUInt32LE(off + 12);
        const w = buf.readInt32LE(off + 16);
        const h = buf.readInt32LE(off + 20);

        if (w <= 2 || h <= 2) continue; // skip placeholders

        const is32 = (props & 0x10000) !== 0;
        const pixels = new Uint8Array(w * h * 4); // RGBA, transparent

        // Read outline table
        const outlines = [];
        for (let y = 0; y < h; y++) {
            const left = buf.readUInt16LE(outlineOff + y * 4);
            const right = buf.readUInt16LE(outlineOff + y * 4 + 2);
            outlines.push({ left, right });
        }

        // Read command offsets
        const cmdOffsets = [];
        for (let y = 0; y < h; y++) {
            cmdOffsets.push(buf.readUInt32LE(cmdOff + y * 4));
        }

        // Decode each row
        for (let y = 0; y < h; y++) {
            if (outlines[y].left === 0xFFFF || outlines[y].right === 0xFFFF) continue;

            let x = outlines[y].left;
            let pos = cmdOffsets[y];
            let safety = 0;

            while (x < w && pos < buf.length && safety < 10000) {
                safety++;
                const cmd = buf[pos++];

                if (cmd === 0x0F) break; // end of row

                const lowbits = cmd & 0x03;

                if (is32) {
                    // 32-bit commands
                    if (lowbits === 0x00) {
                        // Lesser block copy
                        let count = (cmd >> 2) + 1;
                        if (cmd === 0x00) { // extended
                            count = buf[pos] | (buf[pos+1] << 8); pos += 2;
                        }
                        for (let i = 0; i < count && x < w; i++, x++) {
                            const pi = (y * w + x) * 4;
                            pixels[pi+2] = buf[pos++]; // B→R
                            pixels[pi+1] = buf[pos++]; // G
                            pixels[pi+0] = buf[pos++]; // R→B
                            pixels[pi+3] = buf[pos++]; // A
                            if (pixels[pi+3] === 0) pixels[pi+3] = 255; // treat 0 alpha as opaque for indexed
                        }
                    } else if (lowbits === 0x01) {
                        // Lesser skip
                        let count = (cmd >> 2) + 1;
                        if (cmd === 0x01) {
                            count = buf[pos] | (buf[pos+1] << 8); pos += 2;
                        }
                        x += count;
                    } else if (lowbits === 0x02) {
                        // Lesser block fill
                        let count = (cmd >> 2) + 1;
                        if (cmd === 0x02) {
                            count = buf[pos] | (buf[pos+1] << 8); pos += 2;
                        }
                        const b = buf[pos++], g = buf[pos++], r = buf[pos++], a = buf[pos++];
                        for (let i = 0; i < count && x < w; i++, x++) {
                            const pi = (y * w + x) * 4;
                            pixels[pi+0] = r; pixels[pi+1] = g; pixels[pi+2] = b;
                            pixels[pi+3] = a || 255;
                        }
                    } else if (lowbits === 0x03) {
                        // Extended commands
                        const ecmd = cmd >> 2;
                        if (ecmd === 0) { // xflip
                            x++;
                        } else if (ecmd === 1) { // fill player color
                            let count = buf[pos++];
                            const b = buf[pos++], g = buf[pos++], r = buf[pos++], a = buf[pos++];
                            for (let i = 0; i < count && x < w; i++, x++) {
                                const pi = (y * w + x) * 4;
                                pixels[pi+0] = r; pixels[pi+1] = g; pixels[pi+2] = b; pixels[pi+3] = a || 255;
                            }
                        } else {
                            break; // unknown
                        }
                    }
                } else {
                    // 8-bit palette commands
                    if (lowbits === 0x00) {
                        let count = cmd >> 2;
                        if (count === 0) { count = buf[pos++]; }
                        for (let i = 0; i < count && x < w; i++, x++) {
                            const idx = buf[pos++];
                            const pi = (y * w + x) * 4;
                            pixels[pi+0] = palette[idx*3+0];
                            pixels[pi+1] = palette[idx*3+1];
                            pixels[pi+2] = palette[idx*3+2];
                            pixels[pi+3] = 255;
                        }
                    } else if (lowbits === 0x01) {
                        let count = cmd >> 2;
                        if (count === 0) { count = buf[pos++]; }
                        x += count; // skip (transparent)
                    } else if (lowbits === 0x02) {
                        let count = cmd >> 2;
                        if (count === 0) { count = buf[pos++]; }
                        const idx = buf[pos++];
                        for (let i = 0; i < count && x < w; i++, x++) {
                            const pi = (y * w + x) * 4;
                            pixels[pi+0] = palette[idx*3+0];
                            pixels[pi+1] = palette[idx*3+1];
                            pixels[pi+2] = palette[idx*3+2];
                            pixels[pi+3] = 255;
                        }
                    } else if (lowbits === 0x03) {
                        const ecmd = cmd >> 2;
                        if (ecmd === 0) {
                            // outline (player color placeholder)
                            const pi = (y * w + x) * 4;
                            pixels[pi+0] = 0; pixels[pi+1] = 100; pixels[pi+2] = 200; pixels[pi+3] = 255;
                            x++;
                        } else if (ecmd === 1) {
                            // fill player color
                            let count = buf[pos++];
                            const idx = buf[pos++];
                            for (let i = 0; i < count && x < w; i++, x++) {
                                const pi = (y * w + x) * 4;
                                pixels[pi+0] = palette[idx*3+0];
                                pixels[pi+1] = palette[idx*3+1];
                                pixels[pi+2] = palette[idx*3+2];
                                pixels[pi+3] = 255;
                            }
                        } else {
                            break;
                        }
                    }
                }
            }
        }

        const fname = path.join(outdir, `${base}_f${String(fi).padStart(2,'0')}_${w}x${h}.bmp`);
        writeBMP(fname, w, h, pixels);
        extracted++;
    }
    console.log(`  → ${extracted} frames extracted`);
}

// Main
const file = process.argv[2];
const outdir = process.argv[3] || './clf_out';
if (!file) { console.log("Usage: node slp_decode.js <file.slp|dir> <outdir>"); process.exit(1); }

if (!fs.existsSync(outdir)) fs.mkdirSync(outdir, { recursive: true });

// Try to load AoE2 palette — cliff-specific first, then standard 50500
const gameDir = 'D:/Steam/steamapps/common/Age2HD';
let pal = tryLoadPalette({
    paths: [
        path.join(gameDir, 'resources/_common/dat/clf_pal.pal'),
        path.join(gameDir, 'resources/_common/drs/interface/50500.bina'),
    ]
});
if (!pal) pal = PAL;

if (fs.statSync(file).isDirectory()) {
    for (const f of fs.readdirSync(file).filter(f => f.endsWith('.slp'))) {
        decodeSLP(path.join(file, f), outdir, pal);
    }
} else {
    decodeSLP(file, outdir, pal);
}
console.log(`\nOutput: ${outdir}`);
