(() => {
  const BOOT_MS = 1500;
  const FRAME_MS = 500; // The splash's frame time on the board
  const SCROLL_PX = 48;
  const SEEN_HELP_KEY = "seen-help";
  const COMBO_KEY = "combo";
  const SLOT = 0x10000;
  const DEFAULT_ICON = [56, 96, 2];

  // The shelves, in page order, from games/categories.json
  const CATEGORIES = Array.isArray(window.CATEGORIES) ? window.CATEGORIES : [];

  const $ = (id) => document.getElementById(id);
  const device = $("device");
  const screenMsg = $("screen-msg");
  const menu = $("menu");
  const install = $("install");
  const activate = $("activate");
  const comboInstall = $("combo-install");
  const comboActivate = $("combo-activate");
  const help = $("help");

  // Opened as a file, the page shows its games but cannot fetch a manifest to flash with
  const fromFile = location.protocol === "file:";
  const usbOk = "serial" in navigator && window.isSecureContext && !fromFile;
  const games = Array.isArray(window.GAMES) ? window.GAMES : [];
  const ready = (game) => Boolean(game.manifest && game.size);
  const tiles = new Map();
  const thumbs = new Map();
  // Page order, which is also the menu's order on the board
  const order = [];
  const selected = new Set();
  let shown = null;
  let manifestUrls = [];
  let build = 0;

  const slotSize = (bytes) => Math.ceil(bytes / SLOT) * SLOT;
  const mb = (bytes) => `${(bytes / 1048576).toLocaleString("pt-BR", { maximumFractionDigits: 1 })} MB`;
  const picked = () => order.filter((g) => selected.has(g.id));
  const layout = (list) => Bundle.layout(window.LAUNCHER.size, list);
  // A single game goes on alone, in the default table's app slot
  const fits = (list) => list.length <= 1 || layout(list).fits;

  function boot() {
    const hood = $("hood");
    let frame = 0;
    const timer = setInterval(() => {
      frame ^= 1;
      hood.src = `img/brand/hood_${frame}.png`;
    }, FRAME_MS);
    setTimeout(() => {
      clearInterval(timer);
      $("boot").classList.add("gone");
      menu.classList.add("show");
      showWarning();
    }, BOOT_MS);
  }

  function showWarning() {
    const warning = fromFile
      ? "<strong>ABERTO DO DISCO</strong><span>Para gravar: python3 -m http.server -d docs</span>"
      : !("serial" in navigator)
      ? "<strong>SEM USB AQUI</strong><span>Abra no Chrome ou no Edge, num computador.</span>"
      : !window.isSecureContext
      ? "<strong>SEM HTTPS</strong><span>A gravação só funciona em HTTPS ou localhost.</span>"
      : "";
    screenMsg.hidden = !warning;
    screenMsg.classList.add("band");
    screenMsg.innerHTML = warning;
  }

  function restartAnimation(el, cls) {
    el.classList.remove(cls);
    void el.offsetWidth;
    el.classList.add(cls);
  }

  // The launcher's icon: a 64 * factor crop of the screen at [x, y]
  function setIcon(el, game) {
    const [x, y, factor] = game.icon || DEFAULT_ICON;
    el.style.backgroundImage = `url("${game.image}")`;
    el.style.setProperty("--ix", x);
    el.style.setProperty("--iy", y);
    el.style.setProperty("--crop", 64 * factor);
  }

  function showGame(game) {
    tiles.forEach((tile, id) => tile.classList.toggle("current", id === game.id));
    if (game === shown) {
      return;
    }
    shown = game;
    document.documentElement.style.setProperty("--accent", game.accent);
    const icon = $("menu-icon");
    setIcon(icon, game);
    icon.hidden = false;
    $("menu-name").textContent = game.name;
    $("menu-tag").textContent = game.tagline;
    $("menu-desc").textContent = game.description;
    $("menu-desc").scrollTop = 0;
    const playable = order.filter(ready);
    $("menu-count").textContent = ready(game) ? `${playable.indexOf(game) + 1}/${playable.length}` : "";
    restartAnimation(menu, "flicker");
  }

  function showPrompt() {
    $("menu-icon").hidden = true;
    $("menu-name").textContent = "Monte sua coletânea";
    $("menu-tag").textContent = "Passe o mouse num jogo";
    $("menu-desc").textContent = "Clique nos jogos para pôr na placa e grave pelo botão vermelho.";
    $("menu-count").textContent = "";
  }

  function renderCategories() {
    const root = $("categories");
    CATEGORIES.forEach((category) => {
      const members = games.filter((g) => g.category === category.id);
      const section = document.createElement("section");
      section.className = "category";
      section.innerHTML = '<h2 class="category-title"><span></span><small></small></h2>';
      section.querySelector("span").textContent = category.name;
      section.querySelector("small").textContent = members.length ? String(members.length) : "";
      if (members.length === 0) {
        const empty = document.createElement("p");
        empty.className = "empty";
        empty.textContent = "Em breve: futuramente esta categoria terá jogos.";
        section.append(empty);
      } else {
        const grid = document.createElement("div");
        grid.className = "tiles";
        members.forEach((game) => grid.append(renderTile(game)));
        section.append(grid);
      }
      order.push(...members);
      root.append(section);
    });
  }

  // aria-disabled rather than disabled, so a game that does not fit still shows on hover
  function renderTile(game) {
    const tile = document.createElement("button");
    tile.className = "tile";
    tile.style.setProperty("--cart", game.accent);
    tile.setAttribute("aria-pressed", "false");
    tile.innerHTML =
      '<span class="icon"></span><span class="tile-text"><span class="tile-name"></span>' +
      '<small class="tile-size"></small></span><span class="check" aria-hidden="true"></span>';
    setIcon(tile.querySelector(".icon"), game);
    tile.querySelector(".tile-name").textContent = game.name;
    tile.querySelector(".tile-size").textContent = ready(game) ? `${slotSize(game.size) / 1024} KB` : "em breve";
    if (!ready(game)) {
      tile.setAttribute("aria-disabled", "true");
    }
    tile.addEventListener("mouseenter", () => showGame(game));
    tile.addEventListener("focus", () => showGame(game));
    tile.addEventListener("click", () => toggle(game));
    tile.addEventListener("animationend", () => tile.classList.remove("refused"));
    tiles.set(game.id, tile);
    return tile;
  }

  function toggle(game) {
    showGame(game);
    if (!ready(game)) {
      return;
    }
    if (selected.has(game.id)) {
      selected.delete(game.id);
    } else if (fits([...picked(), game])) {
      selected.add(game.id);
    } else {
      restartAnimation(tiles.get(game.id), "refused");
      return;
    }
    update();
  }

  function update() {
    const list = picked();
    try {
      localStorage.setItem(COMBO_KEY, JSON.stringify(list.map((g) => g.id)));
    } catch (e) {}

    // What would overflow the flash cannot be picked, so a pick always fits
    let full = false;
    order.filter(ready).forEach((game) => {
      const tile = tiles.get(game.id);
      const on = selected.has(game.id);
      const room = on || fits([...list, game]);
      tile.setAttribute("aria-pressed", String(on));
      tile.setAttribute("aria-disabled", String(!room));
      tile.classList.toggle("full", !room);
      tile.querySelector(".tile-size").textContent = room ? `${slotSize(game.size) / 1024} KB` : "não cabe";
      full = full || !room;
    });

    const plan = layout(list);
    const alone = list.length === 1;
    const system = alone ? SLOT : plan.launcher.offset + plan.launcher.size;
    const used = alone ? SLOT + slotSize(list[0].size) : plan.end;
    const meter = $("meter");
    meter.innerHTML = "";
    const segment = (bytes, color, title) => {
      const seg = document.createElement("span");
      seg.style.width = `${(bytes / plan.flashSize) * 100}%`;
      if (color) seg.style.background = color;
      seg.title = title;
      meter.append(seg);
    };
    segment(system, "", alone ? "Sistema" : "Menu e sistema");
    list.forEach((game) => segment(slotSize(game.size), game.accent, game.name));
    const count = alone ? "1 jogo, sem menu" : `${list.length} jogos`;
    $("meter-label").textContent =
      list.length === 0
        ? `Nenhum jogo ainda · ${mb(plan.flashSize)} na placa`
        : `${count} · ${mb(used)} de ${mb(plan.flashSize)}${full ? " · o resto não cabe" : ""}`;
    $("picked").textContent = list.map((g) => g.name).join(" · ");

    setManifest(list);
  }

  function setManifest(list) {
    const run = ++build;
    setReady(null);
    if (list.length === 0 || !usbOk) {
      return;
    }
    if (list.length === 1) {
      setReady(list[0].manifest, list[0].name);
      return;
    }
    Promise.all(list.map((g) => loadThumb(g.id))).then((pics) => {
      if (run !== build) {
        return;
      }
      manifestUrls.forEach((url) => URL.revokeObjectURL(url));
      manifestUrls = [];
      const blob = (data) => {
        const url = URL.createObjectURL(new Blob([data]));
        manifestUrls.push(url);
        return url;
      };
      const plan = layout(list);
      const chosen = list.map((g, i) => ({ ...g, thumb: pics[i] }));
      const parts = [
        { path: "firmware/launcher/bootloader.bin", offset: 0x1000 },
        { path: blob(Bundle.partitionTable(plan)), offset: 0x8000 },
        { path: blob(Bundle.blankOtadata()), offset: Bundle.OTADATA.offset },
        { path: blob(Bundle.catalog(chosen)), offset: plan.catalog.offset },
        { path: "firmware/launcher/firmware.bin", offset: plan.launcher.offset },
        ...plan.slots.map((slot) => ({ path: `firmware/${slot.id}/firmware.bin`, offset: slot.offset })),
      ];
      const manifest = {
        name: "Coletânea",
        version: list.map((g) => g.id).join(" + "),
        // Asks before erasing; the default keeps NVS, so every game's save survives
        new_install_prompt_erase: true,
        new_install_improv_wait_time: 0,
        builds: [{ chipFamily: "ESP32", parts }],
      };
      setReady(blob(JSON.stringify(manifest)), `a coletânea de ${list.length} jogos`);
    });
  }

  function setReady(manifest, what) {
    [install, comboInstall].forEach((el) =>
      manifest ? el.setAttribute("manifest", manifest) : el.removeAttribute("manifest")
    );
    const on = Boolean(manifest);
    activate.disabled = !on;
    activate.classList.toggle("off", !on);
    activate.setAttribute("aria-label", on ? `Gravar ${what} na placa` : "Escolha os jogos primeiro");
    comboActivate.disabled = !on;
    $("confirm-label").textContent = fromFile ? "só via http" : !usbOk ? "sem usb" : "gravar";
  }

  function loadThumb(id) {
    if (!thumbs.has(id)) {
      thumbs.set(
        id,
        fetch(`firmware/${id}/thumb.bin`)
          .then((r) => (r.ok ? r.arrayBuffer() : null))
          .then((buf) => (buf ? new Uint8Array(buf) : null))
          .catch(() => null)
      );
    }
    return thumbs.get(id);
  }

  // A remembered pick that no longer fits (a game grew) loses its last games
  function restore() {
    let saved = [];
    try {
      saved = JSON.parse(localStorage.getItem(COMBO_KEY) || "[]");
    } catch (e) {}
    if (!Array.isArray(saved)) {
      return;
    }
    order.forEach((game) => {
      if (ready(game) && saved.includes(game.id) && fits([...picked(), game])) selected.add(game.id);
    });
  }

  function step(delta) {
    const playable = order.filter(ready);
    if (playable.length === 0) {
      return;
    }
    const at = playable.indexOf(shown);
    const next = at < 0 ? (delta > 0 ? 0 : playable.length - 1) : (at + delta + playable.length) % playable.length;
    showGame(playable[next]);
  }

  function press(dir) {
    const cap = document.querySelector(`.cap[data-dir="${dir}"]`);
    cap.classList.add("pressed");
    setTimeout(() => cap.classList.remove("pressed"), 120);
    if (dir === "left") step(-1);
    if (dir === "right") step(1);
    if (dir === "up") $("menu-desc").scrollBy({ top: -SCROLL_PX });
    if (dir === "down") $("menu-desc").scrollBy({ top: SCROLL_PX });
  }

  document.querySelectorAll(".cap[data-dir]").forEach((cap) =>
    cap.addEventListener("click", () => press(cap.dataset.dir))
  );

  document.addEventListener("keydown", (ev) => {
    const dirs = { ArrowLeft: "left", ArrowRight: "right", ArrowUp: "up", ArrowDown: "down" };
    if (!dirs[ev.key] || ev.altKey || ev.ctrlKey || ev.metaKey || help.open || document.querySelector("ewt-install-dialog")) {
      return;
    }
    ev.preventDefault();
    press(dirs[ev.key]);
  });

  $("flip").addEventListener("click", () => {
    const flipped = device.classList.toggle("flipped");
    $("flip").setAttribute("aria-pressed", String(flipped));
  });

  // The tutorial
  const openHelp = () => {
    help.showModal();
    try {
      localStorage.setItem(SEEN_HELP_KEY, "1");
    } catch (e) {}
    $("help-open").classList.remove("hint");
  };
  $("help-open").addEventListener("click", openHelp);
  $("help-link").addEventListener("click", openHelp);
  $("help-close").addEventListener("click", () => help.close());
  // A click on the backdrop lands on the dialog itself, outside its content box
  help.addEventListener("click", (ev) => {
    const box = help.getBoundingClientRect();
    const inside = ev.clientX >= box.left && ev.clientX <= box.right && ev.clientY >= box.top && ev.clientY <= box.bottom;
    if (ev.target === help && !inside) help.close();
  });
  let seenHelp = false;
  try {
    seenHelp = localStorage.getItem(SEEN_HELP_KEY) === "1";
  } catch (e) {}
  if (!seenHelp) {
    $("help-open").classList.add("hint");
  }

  // The screen glows while the install dialog is talking to the board
  new MutationObserver(() => {
    device.classList.toggle("flashing", Boolean(document.querySelector("ewt-install-dialog")));
  }).observe(document.body, { childList: true });

  if (fromFile) {
    comboActivate.textContent = "só via http";
  }
  showPrompt();
  if (games.length > 0 && window.LAUNCHER && window.Bundle) {
    renderCategories();
    restore();
    update();
  } else {
    $("menu-name").textContent = "Nenhum jogo";
    $("menu-desc").textContent = "Rode ./publish.sh para compilar os jogos e montar a lista.";
    setReady(null);
  }

  boot();
})();
