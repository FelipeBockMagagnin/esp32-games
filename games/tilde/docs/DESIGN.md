# ~tilde~ — design

Um pet virtual cozy em estilo kawaii vetorial, para a placa dos jogos dotcompany: ESP32, display ILI9341 240×320, D-pad de 5 botões e buzzer. Vive em `games/tilde` do monorepo, como os outros jogos, e começou do motor do Idle Miner (`games/idle`), por meio do Deepwell (`games/deepwell`).

## Princípios

- **Cozy.** O pet nunca morre nem vai embora. O descuido só o deixa triste ou doente, e cada problema se resolve com uma ação (Meds cura na hora). Não há punição permanente: na pior das hipóteses, o vínculo cai até a metade do pico.
- **Kawaii e vivo.** Formas redondas com antialiasing, cores pastel e tudo sempre um pouquinho em movimento: o pet respira, pisca, passeia, e a sala tem nuvens, passarinho, planta e poeira no sol.
- **Só vive ligado.** A placa não tem relógio quando desligada, então o tempo do pet é tempo ligado. Ela é um bichinho de mesa sempre ligado.
- **O rosto diz o humor.** Nos temas monocromáticos tudo vira um único matiz, então a cor nunca carrega sozinha uma informação.

## O jogo

### Necessidades (0–100, cinco passos na tela)

| | cai de cheio a vazio | repõe com |
|---|---|---|
| **Food** | ~10 h acordado, ~20 h dormindo | Meal (+60, recusado se cheio), Snack (+15 food, +10 joy; 4 seguidos dão dor de barriga) |
| **Joy** | ~8 h acordado; parado dormindo | Play (minigame, +30 a +60), Pet (carinho +25, com cooldown de 5 min) |
| **Rest** | ~4 h acordado | dormir: ~1 h com a lâmpada apagada, ~2 h acesa |
| **Tidy** | 1 fluff a cada ~2,5 h acordado, no máximo 3 | Clean (a vassoura varre a sala) |

Ocultos:
- **Bond** (0–1000): sobe com cada cuidado, e o cuidado dado quando o pet precisava vale 6×. Ganha +4 por hora com tudo em ordem e perde 2 por hora de descuido, nunca abaixo da metade do pico. É o vínculo que escolhe a forma adulta.
- **Doença:** vem de duas necessidades baixas por 2 h acordado, ou de petiscos demais. Doente, a alegria cai 2× mais rápido e o vínculo cresce pela metade. Nada piora enquanto o pet dorme.

### Sono e lâmpada

O pet tem ritmo próprio: dorme sozinho quando o Rest zera. Apagar a lâmpada com o Rest abaixo de 50 põe o pet para dormir. **No escuro, ele dorme a noite toda:** quando descansa, continua dormindo até a lâmpada acender (ou no máximo 10 h). Assim a noite do dono vira a noite do pet. Ao acordar sozinho, ele mesmo acende a lâmpada.

### Humor

`Asleep`, `Unwell`, `Sleepy`, `Sad`, `Okay`, `Content`, `Happy`. O humor decide o rosto, o comportamento (passeia, saltita, chora, sua) e as falas sorteadas a cada 20 s.

### Crescimento (tempo ligado)

```
Egg (3 min; segurar Confirm aquece) → Blip, bebê (3 h) → criança (24 h) → adulto
  bebê que brincou (minigames) mais do que comeu → Tuft, senão → Mochi
  Mochi → Puddle (vínculo ≥ 85) | Pebble
  Tuft  → Nimbus (vínculo ≥ 85) | Sprout
```
Vínculo baixo só dá outro temperamento, nunca uma forma "ruim". Um pet dormindo espera acordar para crescer, para que a mudança seja vista.

**Passar o ovo:** um adulto com 7 dias e vínculo ≥ 600 pode deixar um ovo (em Settings). Ele "guarda uma página" no Album e um ovo novo começa, na geração seguinte.

Pelo simulador (`tools/sim.sh`):
- Quem olha toda hora e brinca ganha um Nimbus; quem dá petiscos ganha um Puddle; com 4 visitas por dia sai um Pebble; quem passa o fim de semana fora ganha um Sprout.
- O dono atento pode passar o ovo por volta do dia 8.
- Com 4 visitas por dia, o pet fica doente ~12% do tempo; com o dono atento, nunca.

### Telas (rotação ◄ ►)

1. **Home:** a sala viva num card arredondado, com o balão de fala do pet. Embaixo ficam os 4 medidores-pílula (comida, alegria, descanso, limpeza) e a barra de 7 botões redondos (Meal, Snack, Play, Clean, Pet, Lamp, Meds), com o nome e a nota do botão escolhido.
2. **Stats:** nome, forma, idade, tempo até crescer, barras completas, vínculo, humor e um retrato.
3. **Play:** os minigames com os recordes.
   - **Hum:** o pet canta uma melodia no buzzer, uma nota por seta, em quatro botões grandes que acendem e quicam (cima é a mais aguda, baixo a mais grave), e o jogador repete. Cada rodada acrescenta uma nota.
   - **Catch:** petiscos caem por 30 s e o pet anda com ◄ ► para pegá-los. Cada um dá comida e alegria.
4. **Album:** as 8 formas em molduras redondas, com as não vistas em silhueta, a nota de cada uma, a geração e as memórias dos pets passados.

Modais:
- **Splash** no boot.
- **Hello!:** o bebê quicando e o nome em blocos de letra arredondados (cima/baixo trocam a letra, esquerda/direita mudam a posição).
- **Settings** (segurar ◄): som, dim, tema, passar o ovo e reset.
- **Confirm**, com "Não" selecionado primeiro.

## Padrão visual

A primeira versão era um terminal CP437; o pet em contornos de caracteres ficava vazio e pouco fofo, e o visual foi trocado por inteiro.

### Desenho
- Rasterizador próprio (`src/gfx/Canvas`): círculos, elipses, retângulos arredondados, linhas de ponta redonda, arcos e triângulos com antialiasing, mais gradientes e transparência.
- O quadro inteiro é redesenhado a cada 33 ms, em 10 faixas de 32 linhas enviadas por DMA. Uma faixa que não mudou não é reenviada.
- A fonte é a Nunito ExtraBold (OFL) em 11, 13 e 20 px, com alfa de 4 bits. Os símbolos são ícones vetoriais: coração, estrela, lua, brilho, gota, nota, folha, cruz, tigela, bala, bola, vassoura, lâmpada e setas.

### Paleta

| tema | caráter |
|---|---|
| **Cream** (padrão) | creme claro, rosa e menta |
| **Cocoa** | escuro e quente, para a noite |
| **Mint** | menta e pêssego |
| **Lavender** | lilás e azul, sonhador |

Os slots cobrem: fundo, card, tinta (3 tons), parede (gradiente), chão, tapete, céu de dia e de noite, o tom da noite, 5 cores de corpo, olho, brilho, rosa (bochecha, comida), dourado (estrelas, alegria), água (descanso, lágrimas), folha (plantas, limpeza), coral (alerta), destaque (seleção) e sombra.

### O pet
Cada forma é uma linha de dados: o tamanho e a cor do corpo, os olhos e o traço próprio. O corpo é uma elipse com contorno macio, brilho e sombra no chão.

| forma | traço |
|---|---|
| Egg | oval com manchas, dorme lá dentro |
| Blip | bolinha azul com broto |
| Mochi | redondo rosa com o "nó" de bolinho |
| Tuft | lilás com orelhas pontudas |
| Puddle | geleia azul com gotas que pingam |
| Pebble | pedrinha menta com musgo |
| Nimbus | nuvem branca que flutua |
| Sprout | amarelo com duas folhas grandes |

**Rosto** (vale para todas as formas):

| | |
|---|---|
| olhos | abertos com brilho, arregalados, `^ ^` feliz, `u u` fechados, piscando, tristes com sobrancelha, espiral (doente), `> <` carinho, estrela (encantado) |
| boca | sorriso, `w`, aberta, triste, mastigando, biquinho, reta |
| bochechas | o blush varia com o humor; na birra elas incham |

### Animação
- **Squash & stretch** por mola amortecida: respiração; pulo com agachamento, esticada no ar e amassada na queda; mordidas; carinho que espreme.
- **Comportamentos** sorteados pelo humor: passear (com passinhos), sentar, olhar em volta, bocejar, dançar com notas, quicar, cheirar a planta (que balança), perseguir a poeira, suspirar, cochilar. Dormindo, ele fica achatado e solta `z`.
- **Reações**: olha para o botão escolhido, comemora quando a Home abre, faz birra numa recusa, se assusta com fluff novo, se espreguiça ao acordar.
- **Cuidados**: a tigela aparece com um pop e a comida some a cada mordida; corações em leque no carinho; a vassoura deixa brilhos; cruzinhas no remédio.
- **Eclosão**: o ovo treme, racha, estoura em confete e o bebê quica.
- **Crescer**: o corpo pisca em branco e estoura em brilhos.
- **Sala viva**: nuvens e um passarinho de dia; lua e estrelas quando o pet dorme; cortinas balançando; a lâmpada pisca ao acender e lança um cone de luz; poeira no raio de sol; a sala escurece em 1,5 s quando a lâmpada apaga.
- **Interface**: o header é uma pílula, com avisos que deslizam; os medidores enchem suavemente e pulsam quando baixos; o botão selecionado cresce com mola; as falas aparecem num balão que surge com um pop.

## Fora da v1
- Moeda dos minigames e loja de decoração e chapéus.
- Um terceiro minigame.
- Clima na janela.
- RTC opcional, para o tempo passar desligado.
- Mais formas por linha.
