# StudyMe: Análise Técnica do Projeto "Copa do Mundo 2026"

Este documento serve como um guia técnico para programadores que desejam entender a arquitetura e as tecnologias utilizadas no projeto, com foco especial nas implementações de Computação Gráfica com OpenGL 2.0.

## 1. Conceitos de Computação Gráfica Aplicados

Esta seção aborda os conceitos teóricos de CG que formam a base do projeto, independentemente da API gráfica.

### Pipeline de Renderização 2D

-   **O que é:** Uma sequência de etapas para converter uma descrição de cena 2D (posições, formas, cores) em uma imagem na tela. Envolve transformar coordenadas do "mundo" do jogo em coordenadas de tela e, em seguida, "pintar" os pixels correspondentes (rasterização).
-   **Como foi usado:** O jogo utiliza uma projeção ortográfica (`glOrtho`) para mapear o mundo 2D do jogo diretamente para a janela. A função `Game::render()` em `src/GameRender.cpp` orquestra o desenho, seguindo uma ordem estrita de trás para a frente (estádio -> campo -> sombras -> jogadores -> bola) para simular profundidade e garantir que os elementos se sobreponham corretamente.

### Álgebra Vetorial

-   **O que é:** Um sistema matemático para manipular quantidades que possuem magnitude e direção (vetores). É a base para calcular movimento, distâncias, direções e colisões em qualquer ambiente 2D ou 3D.
-   **Como foi usado:** A classe `Vec2` é a espinha dorsal da lógica de simulação.
    -   **Física:** Em `src/GameSimulation.cpp`, vetores são usados para atualizar a posição da bola com base em sua velocidade (`ball_.position += ball_.velocity * dt;`).
    -   **Colisões:** A resposta a uma colisão (ex: bola na trave) é calculada usando operações vetoriais como o produto escalar para determinar o vetor de reflexão.
    -   **IA:** A inteligência artificial usa vetores para tomar decisões, como calcular a direção que um jogador deve seguir para interceptar a bola ou se posicionar taticamente.

### Detecção e Resposta a Colisão

-   **O que é:** Algoritmos para determinar se dois ou mais objetos estão se sobrepondo. A "resposta" dita o que acontece após a colisão (ex: quicar, parar).
-   **Como foi usado:** O projeto usa uma abordagem simplificada e eficiente:
    -   **Jogador-Bola:** Uma simples verificação de distância entre os centros dos círculos do jogador e da bola.
    -   **Bola-Paredes:** Uma verificação de AABB (Caixa Delimitadora Alinhada aos Eixos) contra os limites do campo.
    -   **Bola-Traves:** Em `updateBallPhysics`, as traves são tratadas como pequenos círculos. Quando a bola colide, um vetor normal é calculado e usado para simular um rebote realista, tornando a física mais crível.

---

## 2. Conceitos de OpenGL 2.0 Aplicados

Esta seção detalha as funcionalidades específicas da API OpenGL (em seu modo de compatibilidade/legado) que foram utilizadas para a renderização.

### Modo Imediato (`glBegin`/`glEnd`)

-   **O que é:** O método clássico do OpenGL legado para enviar geometria à GPU. O programador inicia um bloco com `glBegin(PRIMITIVA)` e envia os vértices um a um com `glVertex*`, finalizando com `glEnd()`. É simples de entender, mas menos performático que abordagens modernas (Vertex Buffers).
-   **Como foi usado:** Todo o desenho no projeto (`src/GameRender.cpp`) utiliza este modo para máxima compatibilidade com o pipeline fixo do OpenGL 2.0. Por exemplo, `drawField()` usa `glBegin(GL_LINE_LOOP)` para as linhas do campo e `glBegin(GL_QUADS)` para as faixas do gramado.

### Gerenciamento de Matrizes (`glMatrixMode`, `glOrtho`)

-   **O que é:** O OpenGL 2.0 gerencia transformações através de uma máquina de estados com pilhas de matrizes (`GL_PROJECTION` para a câmera, `GL_MODELVIEW` para os objetos). `glMatrixMode` seleciona a pilha ativa, e `glOrtho` define uma projeção 2D sem perspectiva.
-   **Como foi usado:** A função `Game::configureProjection()` é o ponto central. Ela ativa a matriz `GL_PROJECTION`, a reseta com `glLoadIdentity()`, e então configura a visão 2D com `glOrtho`. A lógica garante que a proporção do campo seja mantida independentemente do tamanho da janela, evitando distorções. A matriz `GL_MODELVIEW` é simplesmente resetada a cada quadro, pois todas as posições já estão em coordenadas do mundo.

### Atributos de Vértice (`glColor`, `glLineWidth`)

-   **O que é:** Propriedades que alteram a aparência das primitivas. `glColor*` define a cor atual, que será aplicada a todos os vértices subsequentes. `glLineWidth` define a espessura de primitivas de linha.
-   **Como foi usado:** Essencial para a identidade visual do jogo. `glColor3f` é usado para colorir tudo: gramado, uniformes dos times, bola, HUD, etc. `glLineWidth` é usado para garantir que as linhas do campo e os indicadores de jogador sejam claramente visíveis.

### Alpha Blending (Transparência)

-   **O que é:** Uma técnica que mistura a cor de um objeto sendo desenhado com a cor dos pixels que já estão na tela. É ativada com `glEnable(GL_BLEND)` e configurada com `glBlendFunc`.
-   **Como foi usado:** Crucial para o polimento visual do jogo.
    -   `drawShadows()`: Desenha sombras semi-transparentes sob os jogadores e a bola.
    -   `drawBallSpotlight()`: Cria um efeito de "holofote" suave e translúcido ao redor da bola.
    -   `drawGameOverOverlay()`: Renderiza uma caixa escura semi-transparente no final do jogo.
    -   Para isso, a cor é definida com `glColor4f`, onde o quarto parâmetro (alpha) controla o nível de opacidade.

### Renderização de Texto (GLUT)

-   **O que é:** O OpenGL legado não possui uma forma nativa simples de renderizar texto. A biblioteca GLUT oferece uma utilidade para desenhar fontes de bitmap. `glRasterPos2f` posiciona o "cursor" de texto, e `glutBitmapCharacter` desenha um caractere por vez.
-   **Como foi usado:** A função `Game::drawText` encapsula essa lógica. Ela é usada massivamente em `drawHud()` para exibir o placar, cronômetro, instruções de controle e outras informações textuais.

---

## 3. Critérios de Avaliação Cumpridos

Análise de como o projeto atendeu aos requisitos funcionais.

-   **a) Desenhe um Campo de Futebol:**
    -   **Como foi cumprido:** A função `drawField()` em `src/GameRender.cpp` desenha um campo completo, com todas as marcações (áreas, círculo central, arcos) usando primitivas `GL_LINE_LOOP`, `GL_QUADS` e funções customizadas para arcos e círculos.

-   **b) Crie uma bola - a bola deverá ser movimentada usando o teclado:**
    -   **Como foi cumprido:** O critério foi superado. O jogador é movido pelo teclado (`src/GameInput.cpp`), e ele, por sua vez, interage com a bola através de uma mecânica de drible. A bola possui uma simulação de física própria em `updateBallPhysics`, com momento e atrito.

-   **c) Crie um placar:**
    -   **Como foi cumprido:** A função `checkGoalDetection()` detecta a entrada da bola no gol, atualiza as variáveis de placar (`scoreTeamA_`, `scoreTeamB_`) e chama `resetAfterGoal()` para reposicionar os jogadores e a bola. O placar é renderizado no HUD por `drawHud()`.

-   **e) e f) Crie os jogadores e a torcida:**
    -   **Como foi cumprido:** Os jogadores são inicializados em `initializePlayers()` e desenhados em `drawPlayers()`. A torcida é desenhada de forma procedural em `drawBackgroundAndStands()` como uma série de pequenos quads coloridos. A cor da torcida pulsa dinamicamente com base na "emoção" da partida (`crowdExcitement_`), tornando-a reativa.

-   **g) Incluir som:**
    -   **Como foi cumprido:** O `AudioSystem` é um dos pontos altos. Ele usa OpenAL para áudio espacializado e, de forma criativa, gera os sons da torcida e cantos de forma procedural, eliminando a necessidade de arquivos de áudio externos. Ele reage a gols e chances perigosas.

-   **h) Crie uma mecânica de movimentação dos jogadores atrás da bola:**
    -   **Como foi cumprido:** A IA em `updatePlayersAI` é tática. Os jogadores não apenas seguem a bola; eles se posicionam em zonas, alternam entre comportamento de ataque e defesa, e têm papéis específicos (goleiro, defensor, atacante), criando uma simulação de time.

-   **i) Seja criativo e crie um item:**
    -   **Como foi cumprido:** Vários itens foram criados:
        1.  **Modo Dia/Noite:** Uma tecla (`N`) alterna todo o esquema de cores do jogo.
        2.  **Chute Carregável:** Segurar a barra de espaço carrega a força do chute, adicionando uma camada de habilidade.
        3.  **Áudio Procedural:** Uma solução técnica altamente criativa para o som.

-   **j) Realismo e Qualidade:**
    -   **Como foi cumprido:** A qualidade se manifesta na arquitetura de software modular e limpa, na física crível da bola (atrito, quiques nas traves), na IA tática, no sistema de áudio dinâmico e no polimento visual (transparência, HUD completo).

---

## 4. Demais Aspectos do Projeto

Breve visão geral de outros sistemas importantes.

### Arquitetura de Software

-   **Como foi implementado:** O projeto é bem estruturado, dividindo responsabilidades em múltiplos arquivos:
    -   `Game.cpp`: Gerencia o estado geral e o loop principal.
    -   `GameInput.cpp`: Processa toda a entrada do usuário.
    -   `GameSimulation.cpp`: Lida com a física da bola e a IA dos jogadores.
    -   `GameRender.cpp`: Responsável por todo o código de desenho com OpenGL.
    -   `AudioSystem.cpp`: Encapsula toda a lógica de som.
    -   Essa separação de interesses torna o código mais fácil de manter e expandir.

### Sistema de Áudio (OpenAL e Procedural)

-   **Como foi implementado:** O `AudioSystem` é um módulo robusto. Ele verifica a disponibilidade do OpenAL e, se presente, gera em tempo real os sons da torcida, cantos e efeitos usando funções matemáticas (ondas senoidais, ruído). Isso torna o executável leve e autossuficiente. Ele também possui um fallback para um simples "beep" de terminal caso o OpenAL não esteja disponível.

### Física e Simulação

-   **Como foi implementado:** A função `updateBallPhysics` em `src/GameSimulation.cpp` é o coração da física. Ela usa integração de Euler para o movimento, aplica um fator de amortecimento (`damping`) para simular o atrito com o gramado e implementa colisões com as bordas do campo e as traves.

### Sistema de Build e Configuração

-   **Como foi implementado:** O projeto utiliza um `Makefile` inteligente que detecta automaticamente os arquivos-fonte e as dependências (via `pkg-config`), com lógica de fallback. Além disso, o arquivo `config/game.cfg` permite que parâmetros de gameplay (duração da partida, física da bola, dificuldade) sejam ajustados sem a necessidade de recompilar o código, uma prática excelente para desenvolvimento e personalização.