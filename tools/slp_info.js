// Quick SLP parser — extracts frame sizes and tries to dump as raw PNGs
// Usage: node slp_info.js <file.slp> [palette.pal]
const fs = require('fs');
const path = require('path');

const file = process.argv[2];
if (!file) { console.log("Usage: node slp_info.js <file.slp>"); process.exit(1); }

const buf = fs.readFileSync(file);
const ver = buf.toString('ascii', 0, 4);
const num_frames = buf.readUInt32LE(4);
const comment = buf.toString('ascii', 8, 32).replace(/\0/g, '');
console.log(`Version: ${ver}  Frames: ${num_frames}  Comment: ${comment}`);

const frames = [];
for (let i = 0; i < num_frames; i++) {
    const off = 32 + i * 32;
    const f = {
        cmd_table:    buf.readUInt32LE(off + 0),
        outline_table:buf.readUInt32LE(off + 4),
        palette_off:  buf.readUInt32LE(off + 8),
        properties:   buf.readUInt32LE(off + 12),
        width:        buf.readInt32LE(off + 16),
        height:       buf.readInt32LE(off + 20),
        hotspot_x:    buf.readInt32LE(off + 24),
        hotspot_y:    buf.readInt32LE(off + 28),
    };
    frames.push(f);
    console.log(`  Frame ${i}: ${f.width}×${f.height}  hotspot(${f.hotspot_x},${f.hotspot_y})  props=0x${f.properties.toString(16)}  outline=0x${f.outline_table.toString(16)}  cmd=0x${f.cmd_table.toString(16)}`);
}
