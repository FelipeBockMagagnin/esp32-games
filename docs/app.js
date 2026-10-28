(() => {
  const BOOT_MS = 1500;
  const FRAME_MS = 500; // The splash's frame time on the board
  const SCROLL_PX = 80;

  const $ = (id) => document.getElementById(id);
  const screenMsg = $("screen-msg");
  const shot = $("shot");
  const install = $("install");
  const activate = install.querySelector('[slot="activate"]');
  const cartsEl = $("carts");
  const info = document.querySelector(".info");

  const usbOk = "serial" in navigator && window.isSecureContext;
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
    if (!usbOk) {
      screenMsg.hidden = false;
      screenMsg.innerHTML = window.isSecureContext
        ? "<strong>SEM USB AQUI</strong><span>Abra no Chrome ou no Edge, num computador.</span>"
        : "<strong>SEM HTTPS</strong><span>A gravação só funciona em HTTPS ou localhost.</span>";
      return;
    }
    shot.classList.add("show");
    if (flicker) {
      shot.classList.remove("flicker");
      void shot.offsetWidth; // Restarts the animation
      shot.classList.add("flicker");
    }
  }

  function renderCarts() {
    cartsEl.innerHTML = "";
    games.forEach((game, i) => {
      const cart = document.createElement("button");
      cart.className = "cart";
      cart.setAttribute("role", "tab");
      cart.style.setProperty("--cart-accent", game.accent);
      cart.innerHTML = `<span class="grip"></span><span class="label"><b></b><small></small></span>`;
      cart.querySelector("b").textContent = game.name;
      cart.querySelector("small").textContent = game.version ? `v ${game.version}` : "em breve";
      cart.addEventListener("click", () => select(i));
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
    $("version").textContent = game.version
      ? `versão ${game.version} · ${game.date}`
      : "ainda não publicado";
    shot.src = game.image;
    shot.alt = `Tela do ${game.name}`;
    info.scrollTop = 0;

    const ready = Boolean(game.manifest);
    if (ready) {
      install.setAttribute("manifest", game.manifest);
    } else {
      install.removeAttribute("manifest");
    }
    activate.disabled = !ready;
    activate.classList.toggle("off", !ready);
    activate.setAttribute("aria-label", ready ? `Instalar ${game.name}` : `${game.name} ainda não publicado`);
    $("confirm-label").textContent = !usbOk ? "SEM USB" : ready ? "INSTALAR" : "EM BREVE";

    [...cartsEl.children].forEach((cart, n) => cart.setAttribute("aria-selected", String(n === i)));
    history.replaceState(null, "", `#${game.id}`);
    if (changed) {
      showScreen(true);
    }
  }

  function press(dir) {
    const pad = document.querySelector(`.pad[data-dir="${dir}"]`);
    pad.classList.add("pressed");
    setTimeout(() => pad.classList.remove("pressed"), 120);
    if (dir === "left") select(current - 1);
    if (dir === "right") select(current + 1);
    if (dir === "up") info.scrollBy({ top: -SCROLL_PX });
    if (dir === "down") info.scrollBy({ top: SCROLL_PX });
  }

  document.querySelectorAll(".pad[data-dir]").forEach((pad) =>
    pad.addEventListener("click", () => press(pad.dataset.dir))
  );

  document.addEventListener("keydown", (ev) => {
    const dirs = { ArrowLeft: "left", ArrowRight: "right", ArrowUp: "up", ArrowDown: "down" };
    if (!dirs[ev.key] || ev.altKey || ev.ctrlKey || ev.metaKey || document.querySelector("ewt-install-dialog")) {
      return;
    }
    ev.preventDefault();
    press(dirs[ev.key]);
  });

  // The LED lights while the install dialog is talking to the board
  new MutationObserver(() => {
    $("led").classList.toggle("on", Boolean(document.querySelector("ewt-install-dialog")));
  }).observe(document.body, { childList: true });

  if (!usbOk) {
    activate.classList.add("off");
  }

  fetch("games.json")
    .then((r) => r.json())
    .then((list) => {
      games = list;
      renderCarts();
      const linked = games.findIndex((g) => `#${g.id}` === location.hash);
      select(linked >= 0 ? linked : 0);
    })
    .catch(() => {
      $("name").textContent = "Erro";
      $("desc").textContent = "Não foi possível carregar a lista de jogos.";
    });

  boot();
})();
