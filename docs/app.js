(() => {
  const BOOT_MS = 1500;
  const FRAME_MS = 500; // The splash's frame time on the board
  const SCROLL_PX = 80;
  const SEEN_HELP_KEY = "seen-help";

  const $ = (id) => document.getElementById(id);
  const device = $("device");
  const screenMsg = $("screen-msg");
  const shot = $("shot");
  const install = $("install");
  const activate = $("activate");
  const cartsEl = $("carts");
  const info = document.querySelector(".info");
  const help = $("help");

  // Opened as a file, the page shows its games but cannot fetch a manifest to flash with
  const fromFile = location.protocol === "file:";
  const usbOk = "serial" in navigator && window.isSecureContext && !fromFile;
  let games = [];
  let current = -1;
  let booted = false;

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
      booted = true;
      showScreen(false);
    }, BOOT_MS);
  }

  function showScreen(flicker) {
    if (!booted) {
      return;
    }
    if (fromFile) {
      screenMsg.hidden = false;
      screenMsg.classList.add("band");
      screenMsg.innerHTML = "<strong>ABERTO DO DISCO</strong><span>Para gravar: python3 -m http.server -d docs</span>";
    } else if (!usbOk) {
      screenMsg.hidden = false;
      screenMsg.innerHTML = !("serial" in navigator)
        ? "<strong>SEM USB AQUI</strong><span>Abra no Chrome ou no Edge, num computador.</span>"
        : "<strong>SEM HTTPS</strong><span>A gravação só funciona em HTTPS ou localhost.</span>";
      return;
    }
    shot.classList.add("show");
    if (flicker) {
      restartAnimation(shot, "flicker");
    }
  }

  function restartAnimation(el, cls) {
    el.classList.remove(cls);
    void el.offsetWidth;
    el.classList.add(cls);
  }

  function renderCarts() {
    cartsEl.innerHTML = "";
    games.forEach((game, i) => {
      const cart = document.createElement("button");
      cart.className = "cart";
      cart.setAttribute("role", "tab");
      cart.style.setProperty("--cart", game.accent);
      cart.style.setProperty("--i", i);
      if (game.thumb) cart.style.setProperty("--thumb", game.thumb);
      cart.innerHTML =
        '<span class="cart-body"><span class="cart-shell"><span class="cart-label"><img alt="">' +
        '<span class="cart-title"><span class="n"></span><small></small></span></span>' +
        '<span class="cart-pins"></span></span></span>';
      cart.querySelector("img").src = game.image;
      cart.querySelector(".n").textContent = game.name;
      cart.querySelector("small").textContent = game.version ? `v ${game.version}` : "em breve";
      cart.addEventListener("click", () => select(i));
      cart.addEventListener("animationend", (ev) => {
        if (ev.animationName === "insert") cart.classList.remove("inserting");
      });
      cartsEl.append(cart);
    });
  }

  function select(i) {
    if (games.length === 0) {
      return;
    }
    i = (i + games.length) % games.length;
    const changed = i !== current;
    current = i;
    const game = games[i];

    document.documentElement.style.setProperty("--accent", game.accent);
    $("name").textContent = game.name;
    $("tagline").textContent = game.tagline;
    $("desc").textContent = game.description;
    $("version").textContent = game.version ? `versão ${game.version} · ${game.date}` : "ainda não publicado";
    shot.src = game.image;
    shot.alt = `Tela do ${game.name}`;
    info.scrollTop = 0;

    const ready = Boolean(game.manifest);
    if (ready) {
      install.setAttribute("manifest", game.manifest);
    } else {
      install.removeAttribute("manifest");
    }
    activate.disabled = !ready || !usbOk;
    activate.classList.toggle("off", !ready || !usbOk);
    activate.setAttribute("aria-label", ready ? `Instalar ${game.name}` : `${game.name} ainda não publicado`);
    $("confirm-label").textContent = fromFile ? "só via http" : !usbOk ? "sem usb" : ready ? "instalar" : "em breve";

    [...cartsEl.children].forEach((cart, n) => cart.setAttribute("aria-selected", String(n === i)));
    history.replaceState(null, "", `#${game.id}`);
    if (changed) {
      restartAnimation(cartsEl.children[i], "inserting");
      device.classList.remove("flipped");
      $("flip").setAttribute("aria-pressed", "false");
      showScreen(true);
    }
  }

  function press(dir) {
    const cap = document.querySelector(`.cap[data-dir="${dir}"]`);
    cap.classList.add("pressed");
    setTimeout(() => cap.classList.remove("pressed"), 120);
    if (dir === "left") select(current - 1);
    if (dir === "right") select(current + 1);
    if (dir === "up") info.scrollBy({ top: -SCROLL_PX });
    if (dir === "down") info.scrollBy({ top: SCROLL_PX });
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

  games = Array.isArray(window.GAMES) ? window.GAMES : [];
  if (games.length > 0) {
    renderCarts();
    const linked = games.findIndex((g) => `#${g.id}` === location.hash);
    select(linked >= 0 ? linked : 0);
  } else {
    $("name").textContent = "Nenhum jogo";
    $("desc").textContent = "Rode ./publish.sh para compilar os jogos e montar a lista.";
  }

  boot();
})();
