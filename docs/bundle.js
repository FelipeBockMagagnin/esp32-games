// Several games on one board: the flash image for a collection, written in the browser for the
// games picked. The launcher (../esp32-launcher) reads it back; its CLAUDE.md and Catalog.h
// describe the layout and the catalog format, and the two must change together.
(() => {
  const FLASH_SIZE = 0x400000;
  const APP_ALIGN = 0x10000;
  const SECTOR = 0x1000;
  const TABLE_SIZE = 0xc00;
  // The default table's, so every game's save survives moving between one game and several
  const NVS = { offset: 0x9000, size: 0x5000 };
  const OTADATA = { offset: 0xe000, size: 0x2000 };
  const CATALOG_OFFSET = 0x10000;
  const CATALOG_TYPE = 0x40;
  const MAX_GAMES = 16;
  const ENTRY_SIZE = 80;
  const HEADER_SIZE = 8;
  const ICON = 64;
  const THUMB_BYTES = ICON * ICON * 2;

  const roundUp = (n, step) => Math.ceil(n / step) * step;

  function catalogSize(count) {
    return roundUp(HEADER_SIZE + count * (ENTRY_SIZE + THUMB_BYTES), SECTOR);
  }

  // Where everything goes for these games ({id, size} in install order); `fits` is false
  // when they overrun the flash
  function layout(launcherSize, games) {
    const catalog = { offset: CATALOG_OFFSET, size: catalogSize(games.length) };
    const launcher = { offset: roundUp(catalog.offset + catalog.size, APP_ALIGN), size: roundUp(launcherSize, APP_ALIGN) };
    let next = launcher.offset + launcher.size;
    const slots = games.map((g) => {
      const slot = { id: g.id, offset: next, size: roundUp(g.size, APP_ALIGN) };
      next += slot.size;
      return slot;
    });
    return { catalog, launcher, slots, end: next, flashSize: FLASH_SIZE, fits: next <= FLASH_SIZE && games.length <= MAX_GAMES };
  }

  function writeLabel(bytes, at, text, length) {
    for (let i = 0; i < length; i++) {
      bytes[at + i] = i < text.length ? text.charCodeAt(i) & 0x7f : 0;
    }
  }

  function partitionTable(plan) {
    const rows = [
      ["nvs", 0x01, 0x02, NVS],
      ["otadata", 0x01, 0x00, OTADATA],
      ["catalog", CATALOG_TYPE, 0x00, plan.catalog],
      ["launcher", 0x00, 0x00, plan.launcher],
      ...plan.slots.map((s, i) => [s.id, 0x00, 0x10 + i, s]),
    ];
    const bytes = new Uint8Array(TABLE_SIZE).fill(0xff);
    const view = new DataView(bytes.buffer);
    rows.forEach(([label, type, subtype, part], i) => {
      const at = i * 32;
      bytes[at] = 0xaa;
      bytes[at + 1] = 0x50;
      bytes[at + 2] = type;
      bytes[at + 3] = subtype;
      view.setUint32(at + 4, part.offset, true);
      view.setUint32(at + 8, part.size, true);
      writeLabel(bytes, at + 12, label, 16);
      view.setUint32(at + 28, 0, true);
    });
    const md5At = rows.length * 32;
    bytes[md5At] = 0xeb;
    bytes[md5At + 1] = 0xeb;
    bytes.set(md5(bytes.subarray(0, md5At)), md5At + 16);
    return bytes;
  }

  // The GLCD font on the board has no accents
  function ascii(text) {
    return text.normalize("NFD").replace(/[̀-ͯ]/g, "").replace(/[^\x20-\x7e]/g, "");
  }

  function rgb565(hex) {
    const n = parseInt(hex.replace("#", ""), 16);
    return ((n >> 19) & 0x1f) << 11 | ((n >> 10) & 0x3f) << 5 | ((n >> 3) & 0x1f);
  }

  // games: {id, name, tagline, accent, thumb} with thumb a 64x64 RGB565 icon or null
  function catalog(games) {
    const bytes = new Uint8Array(catalogSize(games.length)).fill(0xff);
    const view = new DataView(bytes.buffer);
    bytes.set([0x47, 0x43, 0x41, 0x54, 2, games.length, 0, 0]);
    let thumbAt = HEADER_SIZE + games.length * ENTRY_SIZE;
    games.forEach((g, i) => {
      const at = HEADER_SIZE + i * ENTRY_SIZE;
      writeLabel(bytes, at, g.id, 16);
      writeLabel(bytes, at + 16, ascii(g.name), 24);
      writeLabel(bytes, at + 40, ascii(g.tagline), 32);
      view.setUint16(at + 72, rgb565(g.accent), true);
      const thumb = g.thumb && g.thumb.length === THUMB_BYTES ? g.thumb : null;
      bytes[at + 74] = thumb ? ICON : 0;
      bytes[at + 75] = thumb ? ICON : 0;
      view.setUint32(at + 76, thumb ? thumbAt : 0, true);
      if (thumb) {
        bytes.set(thumb, thumbAt);
        thumbAt += THUMB_BYTES;
      }
    });
    return bytes;
  }

  // Blank otadata boots the factory app: a fresh install opens the menu
  function blankOtadata() {
    return new Uint8Array(OTADATA.size).fill(0xff);
  }

  // RFC 1321, for the partition table's checksum entry; Web Crypto has no MD5
  function md5(data) {
    const s = [7, 12, 17, 22, 5, 9, 14, 20, 4, 11, 16, 23, 6, 10, 15, 21];
    const k = Array.from({ length: 64 }, (_, i) => Math.floor(Math.abs(Math.sin(i + 1)) * 2 ** 32) >>> 0);
    const padded = new Uint8Array(roundUp(data.length + 9, 64));
    padded.set(data);
    padded[data.length] = 0x80;
    const bits = new DataView(padded.buffer);
    bits.setUint32(padded.length - 8, (data.length * 8) >>> 0, true);
    bits.setUint32(padded.length - 4, Math.floor(data.length / 0x20000000), true);
    let [a0, b0, c0, d0] = [0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476];
    for (let chunk = 0; chunk < padded.length; chunk += 64) {
      const m = Array.from({ length: 16 }, (_, i) => bits.getUint32(chunk + i * 4, true));
      let [a, b, c, d] = [a0, b0, c0, d0];
      for (let i = 0; i < 64; i++) {
        let f, g;
        if (i < 16) [f, g] = [(b & c) | (~b & d), i];
        else if (i < 32) [f, g] = [(d & b) | (~d & c), (5 * i + 1) % 16];
        else if (i < 48) [f, g] = [b ^ c ^ d, (3 * i + 5) % 16];
        else [f, g] = [c ^ (b | ~d), (7 * i) % 16];
        const shift = s[(i >> 4) * 4 + (i % 4)];
        const sum = (a + f + k[i] + m[g]) >>> 0;
        [a, d, c] = [d, c, b];
        b = (b + ((sum << shift) | (sum >>> (32 - shift)))) >>> 0;
      }
      a0 = (a0 + a) >>> 0;
      b0 = (b0 + b) >>> 0;
      c0 = (c0 + c) >>> 0;
      d0 = (d0 + d) >>> 0;
    }
    const out = new Uint8Array(16);
    const outView = new DataView(out.buffer);
    [a0, b0, c0, d0].forEach((v, i) => outView.setUint32(i * 4, v, true));
    return out;
  }

  const Bundle = { layout, partitionTable, catalog, blankOtadata, md5, FLASH_SIZE, OTADATA };
  if (typeof module !== "undefined") module.exports = Bundle;
  else window.Bundle = Bundle;
})();
