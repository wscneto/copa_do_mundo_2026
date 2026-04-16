# Roteiro de Apresentacao - Copa do Mundo 2026 (Computacao Grafica)

## 1. Objetivo da apresentacao

Mostrar para a turma e para o professor que o projeto cumpre os requisitos da disciplina e aplica, na pratica, os conceitos de Computacao Grafica vistos em sala:

- pipeline grafico em OpenGL 2.0 (modo imediato);
- projecao ortografica e coordenadas 2D;
- primitivas geometricas e render em camadas;
- transformacoes geometricas por algebra vetorial (sem engine);
- fisica basica, colisao e resposta de impacto;
- IA de jogadores;
- som dinamico/procedural;
- item criativo e acabamento visual.

Base de requisitos: [docs/criterios_avaliacao.txt](criterios_avaliacao.txt)

---

## 2. Preparacao antes de apresentar (2 min)

### 2.1 Build e execucao

No terminal:

```bash
make -j4
./worldcup2026
```

### 2.2 Controles para lembrar durante a demo

- `W A S D` ou setas: mover bola
- `SPACE` (segurar e soltar): chute carregavel
- `1`, `2`, `3`: dificuldade (easy/medium/hard)
- `N`: alterna dia/noite
- `R`: reinicia partida
- `ESC`: sair

Referencia no codigo:
- [src/GameInput.cpp#L116](../src/GameInput.cpp#L116)
- [src/GameInput.cpp#L148](../src/GameInput.cpp#L148)
- [README.md](../README.md)

---

## 3. Roteiro cronometrado (dupla)

Tempo total sugerido: **12 a 15 minutos**.

## 3.1 Abertura (1 min) - Pessoa 1

Fala sugerida:

"Nosso jogo foi implementado em C++ com OpenGL 2.0, sem engine pronta. O foco foi aplicar os conceitos da disciplina em um sistema completo: renderizacao, fisica, IA, audio e interacao em tempo real."

Mostrar rapidamente no codigo:
- Ponto de entrada: [src/main.cpp#L1](../src/main.cpp#L1)
- Inicializacao/loop: [src/Game.cpp#L56](../src/Game.cpp#L56)

## 3.2 Requisitos da atividade (2 min) - Pessoa 2

Fala sugerida:

"Aqui estao os requisitos do professor e como cada um foi atendido no projeto."

Abrir:
- Requisitos: [docs/criterios_avaliacao.txt](criterios_avaliacao.txt)
- Evidencias tecnicas: tabela da Secao 4 deste roteiro

## 3.3 Renderizacao e CG aplicada (3 min) - Pessoa 2

Fala sugerida:

"A renderizacao segue ordem de camadas para simular profundidade: estadio, campo, sombras, jogadores, bola e HUD. A projecao ortografica e recalculada para manter proporcao em qualquer resolucao."

Mostrar no codigo:
- Ordem de desenho: [src/GameRender.cpp#L15](../src/GameRender.cpp#L15)
- Projecao/aspect ratio: [src/GameRender.cpp#L61](../src/GameRender.cpp#L61)
- Estadio/torcida procedural: [src/GameRender.cpp#L87](../src/GameRender.cpp#L87)
- Campo oficial (linhas, arcos, areas): [src/GameRender.cpp#L265](../src/GameRender.cpp#L265)
- HUD e placar TV-style: [src/GameRender.cpp#L498](../src/GameRender.cpp#L498)

## 3.4 Fisica, colisao e IA (3 min) - Pessoa 1

Fala sugerida:

"A bola usa integracao temporal com atrito. Colisoes com bordas e traves geram rebote com vetor normal. A IA tem papeis por zona e comportamento diferente em ataque e defesa."

Mostrar no codigo:
- Fisica da bola: [src/GameSimulation.cpp#L10](../src/GameSimulation.cpp#L10)
- Colisao/rebote com trave: [src/GameSimulation.cpp#L53](../src/GameSimulation.cpp#L53)
- IA por papeis e zonas: [src/GameSimulation.cpp#L82](../src/GameSimulation.cpp#L82)
- Colisao entre jogadores: [src/GameSimulation.cpp#L269](../src/GameSimulation.cpp#L269)

## 3.5 Entrada, estado de partida e audio (2 min) - Pessoa 2

Fala sugerida:

"A entrada do usuario controla conducao e chute carregavel. O estado da partida gerencia tempo, placar, posse e fim de jogo. O audio reage aos eventos do jogo."

Mostrar no codigo:
- Controle/chute: [src/GameInput.cpp#L14](../src/GameInput.cpp#L14)
- Gol/placar/reset: [src/Game.cpp#L269](../src/Game.cpp#L269)
- Tempo/fim de jogo: [src/Game.cpp#L297](../src/Game.cpp#L297)
- Posse de bola: [src/Game.cpp#L317](../src/Game.cpp#L317)
- Audio dinamico: [src/AudioSystem.cpp#L437](../src/AudioSystem.cpp#L437)
- Eventos de gol/chute/chance: [src/AudioSystem.cpp#L467](../src/AudioSystem.cpp#L467), [src/AudioSystem.cpp#L487](../src/AudioSystem.cpp#L487), [src/AudioSystem.cpp#L504](../src/AudioSystem.cpp#L504)

## 3.6 Demo ao vivo (3 a 4 min) - Ambos

Sequencia recomendada (na ordem):

1. Iniciar jogo e mostrar HUD, campo e estadio.
2. Mover bola (WASD/setas) para exibir conducao + atrito.
3. Carregar chute no SPACE e soltar; apontar barra de potencia.
4. Fazer um gol para mostrar atualizacao do placar + reset da bola.
5. Pressionar `1`, `2`, `3` e mostrar mudanca da IA/goleiro.
6. Pressionar `N` para mostrar item criativo (dia/noite).
7. Deixar o tempo acabar para mostrar estado de fim de jogo e overlay.

Referencias:
- Render principal: [src/GameRender.cpp#L15](../src/GameRender.cpp#L15)
- Controle da bola/chute: [src/GameInput.cpp#L14](../src/GameInput.cpp#L14)
- Dificuldade: [src/Game.cpp#L224](../src/Game.cpp#L224)
- Fim de jogo: [src/Game.cpp#L297](../src/Game.cpp#L297)

## 3.7 Fechamento (1 min) - Pessoa 1

Fala sugerida:

"O projeto atende os requisitos com implementacao propria dos conceitos da disciplina e organizacao modular, permitindo manutencao e evolucao futura."

---

## 4. Checklist de requisitos com evidencias no codigo

## a) Campo de futebol (retas, circunferencias etc.)

- O que mostrar rodando: gramado, linhas, circulo central, areas, arcos.
- Evidencia no codigo: [src/GameRender.cpp#L265](../src/GameRender.cpp#L265), [src/GameRender.cpp#L782](../src/GameRender.cpp#L782), [src/GameRender.cpp#L772](../src/GameRender.cpp#L772)

## b) Bola movimentada no teclado

- O que mostrar rodando: mover bola com WASD/setas e chutar.
- Evidencia no codigo: [src/GameInput.cpp#L14](../src/GameInput.cpp#L14), [src/GameInput.cpp#L116](../src/GameInput.cpp#L116), [src/GameInput.cpp#L148](../src/GameInput.cpp#L148)

## c) Placar + deteccao de gol + bola no meio

- O que mostrar rodando: gol, placar atualiza e bola reinicia no centro.
- Evidencia no codigo: [src/Game.cpp#L269](../src/Game.cpp#L269), [src/Game.cpp#L137](../src/Game.cpp#L137), [src/GameRender.cpp#L498](../src/GameRender.cpp#L498)

## e/f) Jogadores e torcida

- O que mostrar rodando: jogadores com uniformes e torcida em volta do campo.
- Evidencia no codigo: [src/GameRender.cpp#L431](../src/GameRender.cpp#L431), [src/GameRender.cpp#L87](../src/GameRender.cpp#L87)

## g) Som

- O que mostrar rodando: som ambiente, chute, gol, chance perigosa.
- Evidencia no codigo: [src/AudioSystem.cpp#L375](../src/AudioSystem.cpp#L375), [src/AudioSystem.cpp#L437](../src/AudioSystem.cpp#L437), [src/AudioSystem.cpp#L467](../src/AudioSystem.cpp#L467)

## h) Mecanica de movimentacao dos jogadores atras da bola

- O que mostrar rodando: jogadores perseguindo/posicionando conforme estado do jogo.
- Evidencia no codigo: [src/GameSimulation.cpp#L82](../src/GameSimulation.cpp#L82), [src/Game.cpp#L317](../src/Game.cpp#L317)

## i) Item criativo

- O que mostrar rodando: modo dia/noite (`N`), HUD TV-style, torcida dinamica.
- Evidencia no codigo: [src/GameInput.cpp#L140](../src/GameInput.cpp#L140), [src/GameRender.cpp#L498](../src/GameRender.cpp#L498), [src/GameRender.cpp#L87](../src/GameRender.cpp#L87)

## j) Realismo e qualidade

- O que mostrar rodando: atrito, rebote na trave, sombra/spotlight, tempo de partida.
- Evidencia no codigo: [src/GameSimulation.cpp#L10](../src/GameSimulation.cpp#L10), [src/GameSimulation.cpp#L53](../src/GameSimulation.cpp#L53), [src/GameRender.cpp#L706](../src/GameRender.cpp#L706), [src/Game.cpp#L297](../src/Game.cpp#L297)

---

## 5. Perguntas provaveis da banca (com respostas objetivas + onde abrir no codigo)

## IA, movimentacao e estado

**Pergunta:** Como a IA decide para onde cada jogador vai?

Resposta curta: cada jogador tem papel/zoneamento; com posse, abre para apoiar; sem posse, compacta e marca ameaca.

Abrir: [src/GameSimulation.cpp#L82](../src/GameSimulation.cpp#L82)

**Pergunta:** Como a posse e detectada?

Resposta curta: pela menor distancia jogador-bola dentro de um raio de controle.

Abrir: [src/Game.cpp#L317](../src/Game.cpp#L317)

**Pergunta:** O que muda entre facil, medio e dificil?

Resposta curta: multiplicador de velocidade da IA e parametros de reacao/cobertura do goleiro.

Abrir: [src/Game.cpp#L224](../src/Game.cpp#L224), [src/GameSimulation.cpp#L82](../src/GameSimulation.cpp#L82)

## Fisica e colisao

**Pergunta:** Como a bola se move fisicamente?

Resposta curta: integracao temporal com `posicao += velocidade * dt` e amortecimento por atrito.

Abrir: [src/GameSimulation.cpp#L10](../src/GameSimulation.cpp#L10)

**Pergunta:** Como funciona o rebote na trave?

Resposta curta: trave modelada como circulo; usa vetor normal e reflexao da velocidade com perda de energia.

Abrir: [src/GameSimulation.cpp#L53](../src/GameSimulation.cpp#L53)

**Pergunta:** Existe colisao entre jogadores?

Resposta curta: sim, ha separacao de hitboxes por empurrao simetrico para evitar sobreposicao.

Abrir: [src/GameSimulation.cpp#L269](../src/GameSimulation.cpp#L269)

## Rotacao, translacao e transformacoes

**Pergunta:** Voces usam `glTranslatef` e `glRotatef`?

Resposta curta: nao. As transformacoes sao calculadas manualmente por vetores e trigonometria nas coordenadas dos vertices/objetos.

Abrir: [src/GameSimulation.cpp#L10](../src/GameSimulation.cpp#L10), [src/GameRender.cpp#L401](../src/GameRender.cpp#L401), [src/GameRender.cpp#L87](../src/GameRender.cpp#L87)

**Pergunta:** Onde aparece rotacao no projeto?

Resposta curta: no padrao da bola (patches girando com o tempo) e na construcao parametricas de arcos/circulos por seno/cosseno.

Abrir: [src/GameRender.cpp#L401](../src/GameRender.cpp#L401), [src/GameRender.cpp#L782](../src/GameRender.cpp#L782), [src/GameRender.cpp#L772](../src/GameRender.cpp#L772)

**Pergunta:** Como explicam translacao na pratica?

Resposta curta: e a mudanca de posicao dos objetos ao longo do tempo (bola e jogadores), atualizada por vetores de velocidade/direcao.

Abrir: [src/GameSimulation.cpp#L10](../src/GameSimulation.cpp#L10), [src/GameSimulation.cpp#L82](../src/GameSimulation.cpp#L82)

## Renderizacao e pipeline

**Pergunta:** Como evitam distorcao da cena ao redimensionar janela?

Resposta curta: recalculo da projecao ortografica com compensacao de aspect ratio.

Abrir: [src/GameRender.cpp#L61](../src/GameRender.cpp#L61)

**Pergunta:** Como simulam profundidade sem 3D real?

Resposta curta: ordem de desenho (camadas), sombras fake, gradientes e blending.

Abrir: [src/GameRender.cpp#L15](../src/GameRender.cpp#L15), [src/GameRender.cpp#L706](../src/GameRender.cpp#L706), [src/GameRender.cpp#L87](../src/GameRender.cpp#L87)

## Audio

**Pergunta:** O audio depende de arquivos externos?

Resposta curta: nao obrigatoriamente; ha geracao procedural. Se houver arquivo em `sounds/`, ele pode ser carregado/convertido.

Abrir: [src/AudioSystem.cpp#L239](../src/AudioSystem.cpp#L239), [src/AudioSystem.cpp#L555](../src/AudioSystem.cpp#L555), [src/AudioSystem.cpp#L169](../src/AudioSystem.cpp#L169)

---

## 6. Ordem de arquivos para abrir na apresentacao (cola rapida)

1. [docs/criterios_avaliacao.txt](criterios_avaliacao.txt)
2. [src/GameRender.cpp#L15](../src/GameRender.cpp#L15)
3. [src/GameRender.cpp#L265](../src/GameRender.cpp#L265)
4. [src/GameInput.cpp#L14](../src/GameInput.cpp#L14)
5. [src/GameSimulation.cpp#L10](../src/GameSimulation.cpp#L10)
6. [src/GameSimulation.cpp#L82](../src/GameSimulation.cpp#L82)
7. [src/Game.cpp#L269](../src/Game.cpp#L269)
8. [src/AudioSystem.cpp#L437](../src/AudioSystem.cpp#L437)

---

## 7. Plano B (se algo falhar ao vivo)

- Se audio nao funcionar no PC da apresentacao: seguir demo focando em render, fisica e IA, abrindo [src/AudioSystem.cpp#L375](../src/AudioSystem.cpp#L375) para provar implementacao.
- Se faltar tempo: priorizar secoes 3.2, 3.3, 3.4 e a parte de gol no roteiro de demo.
- Se perguntarem de transformacoes OpenGL por matriz: responder com honestidade que o projeto priorizou transformacao por algebra vetorial direta e explicar onde isso aparece no codigo.
