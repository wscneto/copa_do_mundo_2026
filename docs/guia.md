# 🏆 Guia de Estudo e Apresentação: Copa do Mundo 2026

Bem-vindo ao guia oficial do projeto **Copa do Mundo 2026**. Este documento tem um propósito duplo: servir como um roteiro de apresentação das capacidades do jogo e atuar como um guia de estudos imersivo para alunos e entusiastas de Computação Gráfica e OpenGL C++.

---

## 1. Visão Geral do Projeto (Apresentação)

O projeto é uma simulação de futebol 2D top-down desenvolvida integralmente em **C++ e OpenGL 2.0 (Legado/Modo Imediato)**. 

Diferente de jogos que dependem de motores gráficos pesados (como Unity ou Unreal), este jogo foi construído do zero usando a biblioteca **GLUT/FreeGLUT**. Ele demonstra a aplicação prática de matemática para jogos, física vetorial e manipulação direta da pipeline gráfica.

### 🌟 Principais Destaques:
- **Controle Físico da Bola:** O jogador controla a bola diretamente (WASD/Setas), com física de atrito, momento contínuo e colisões realistas com as traves do gol (rebotes calculados via vetores normais).
- **Inteligência Artificial Sistêmica:** Todos os 10 jogadores de linha e os 2 goleiros possuem IA. Eles operam em zonas de campo, oferecem apoio no ataque, marcam pressão na defesa e realizam desarmes físicos.
- **Áudio 100% Procedural:** Utilizando OpenAL, o jogo sintetiza sons de torcida, cantos, assobios e apitos matematicamente em tempo real, dispensando dezenas de megabytes de arquivos `.mp3`/`.wav`.
- **Renderização Dinâmica:** Torcida pulsante reagindo a gols, sistema de iluminação global fake (Modo Dia/Noite) e interface rica (HUD).
- **Data-Driven:** Configurações como dificuldade e fricção da bola podem ser manipuladas via arquivo de texto (`game.cfg`) sem recompilar o jogo.

---

## 2. Mapa de Estudos de Computação Gráfica

Se você está estudando a disciplina de CG, este repositório é um excelente laboratório. Aqui está onde você deve olhar no código (`src/`) para ver a teoria em ação:

### A. Pipeline de Renderização e Máquina de Estados
* **Onde olhar:** `GameRender.cpp` -> `Game::render()`
* **O que estudar:** Veja como o código limpa os buffers de cor (`glClear`), reseta a matriz (`glLoadIdentity`) e desenha os objetos de trás para frente (Estádio -> Campo -> Jogadores -> Bola -> UI). Isso é conhecido como **Painter's Algorithm** (fake depth).

### B. Sistemas de Coordenadas e Projeção Ortográfica
* **Onde olhar:** `GameRender.cpp` -> `Game::configureProjection()`
* **O que estudar:** Como manter o campo com a mesma proporção (medidas oficiais da FIFA) independente de o usuário esticar ou achatar a janela do sistema operacional. O uso inteligente de `glOrtho` resolve o "aspect ratio".

### C. Primitivas Gráficas e Geração Procedural
* **Onde olhar:** `GameRender.cpp` -> `drawField()`, `drawPlayers()`, `drawBackgroundAndStands()`
* **O que estudar:** Como `GL_QUADS`, `GL_LINE_LOOP` e algoritmos como triângulos em leque (`GL_TRIANGLE_FAN`) são usados para desenhar círculos perfeitos, arcos de escanteio, uniformes listrados e milhares de assentos nas arquibancadas usando geração procedimental.

### D. Transparência e Alpha Blending
* **Onde olhar:** `GameRender.cpp` -> `drawShadows()` e `drawBallSpotlight()`
* **O que estudar:** Como ativar o `GL_BLEND` e usar `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)` para criar camadas translúcidas (sombras e holofotes) usando a variável Alpha do `glColor4f`.

### E. Álgebra Linear Aplicada a Jogos (Física e Colisões)
* **Onde olhar:** `GameSimulation.cpp` -> `updateBallPhysics()` e `include/Vec2.h`
* **O que estudar:** 
  1. **Euler Integration:** Como a velocidade altera a posição a cada *frame*.
  2. **Reflexão Vetorial:** Como a bola quica nas traves do gol encontrando o vetor normal da colisão e usando o Produto Escalar (Dot Product) para rebater.

### F. Animação e Independência de Framerate
* **Onde olhar:** `main.cpp` -> Configuração do `glutTimerFunc`
* **O que estudar:** Como garantir que o jogo rode de forma suave e previsível travando as atualizações do loop principal com precisão de milissegundos (~60 FPS), ao invés de deixá-lo correr livre consumindo toda a CPU.

---

## 3. Guia Rápido de Instalação e Execução

Para apresentar ou testar as funcionalidades:

1. **Pré-requisitos (Linux/Ubuntu):**
   ```bash
   sudo apt install -y g++ make freeglut3-dev libopenal-dev pkg-config
   ```
2. **Compilação:**
   Navegue até o diretório do projeto e rode o comando `make`.
3. **Execução:**
   `./worldcup2026`

**Controles de Apresentação:**
Mostre a interatividade alterando o jogo em tempo real! Aperte **`N`** para demonstrar a troca de paleta de cores entre Dia/Noite, pressione **`1`, `2`, ou `3`** para alterar a inteligência artificial ao vivo, e carregue a força do chute segurando **`SPACE`**.

---

## 4. Desafios para Estudantes (Faça Você Mesmo)

Quer testar suas habilidades modificando este código?
1. **Adicionar Texturas:** Tente carregar uma imagem `.bmp` ou `.png` (usando stb_image) e mapeá-la no centro do gramado usando `glTexCoord2f`.
2. **Câmera Móvel:** Modifique a matriz `GL_MODELVIEW` usando `glTranslatef` para fazer a câmera seguir a bola (como no jogo FIFA clássico) em vez de mostrar o campo todo.
3. **Partículas:** Crie um pequeno sistema de emissão de partículas (pontos usando `GL_POINTS`) toda vez que a bola bater com força na trave.