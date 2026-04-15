# StudyMe: Análise Técnica do Projeto "Copa do Mundo 2026"

Este documento serve como um guia técnico para programadores que desejam entender a arquitetura e as tecnologias utilizadas no projeto, com foco especial nas implementações de Computação Gráfica com OpenGL 2.0.

## 1. Conceitos de Computação Gráfica Aplicados

Esta seção aborda os conceitos teóricos de CG que formam a base do projeto, independentemente da API gráfica.

### Pipeline de Renderização 2D

-   **O que é:** Uma sequência de etapas para converter uma descrição de cena 2D (posições, formas, cores) em uma imagem na tela. Envolve transformar coordenadas do "mundo" do jogo em coordenadas de tela e, em seguida, "pintar" os pixels correspondentes (rasterização).
-   **Como foi usado:** O jogo utiliza uma projeção ortográfica configurada dinamicamente (`glOrtho`) para mapear o mundo 2D mantendo a proporção (aspect ratio) do campo independente do tamanho da janela. A função `Game::render()` orquestra o desenho seguindo uma ordem estrita (estádio -> campo -> spotlight da bola -> sombras -> gols -> jogadores -> bola -> HUD -> overlays) para simular profundidade e sobreposição (fake depth layering).

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
    -   `drawBallSpotlight()`: Cria um efeito de "holofote" suave e translúcido (amarelado) ao redor da bola, mais visível no modo noturno.
    -   `drawGameOverOverlay()`: Renderiza uma caixa escura semi-transparente no final do jogo.
    -   Também é utilizado no render principal para o "flash" brilhante na tela no momento de uma comemoração de gol (`goalFlashTimer_`).
    -   Para isso, a cor é definida com `glColor4f`, onde o quarto parâmetro (alpha) controla o nível de opacidade.

### Renderização de Texto (GLUT)

-   **O que é:** O OpenGL legado não possui uma forma nativa simples de renderizar texto. A biblioteca GLUT oferece uma utilidade para desenhar fontes de bitmap. `glRasterPos2f` posiciona o "cursor" de texto, e `glutBitmapCharacter` desenha um caractere por vez.
-   **Como foi usado:** A função `Game::drawText` encapsula essa lógica. Ela é usada massivamente em `drawHud()` e `drawGameOverOverlay()` para exibir o placar, cronômetro regressivo formatado, nível de dificuldade, instruções e mensagens de fim de jogo.

---

## 3. Critérios de Avaliação Cumpridos

Análise de como o projeto atendeu aos requisitos funcionais.

-   **a) Desenhe um Campo de Futebol:**
    -   **Como foi cumprido:** A função `drawField()` em `src/GameRender.cpp` desenha um campo completo, com todas as marcações (áreas, círculo central, arcos) usando primitivas `GL_LINE_LOOP`, `GL_QUADS` e funções customizadas para arcos e círculos.

-   **b) Crie uma bola - a bola deverá ser movimentada usando o teclado:**
    -   **Como foi cumprido:** O critério foi implementado de forma direta. A bola é movimentada diretamente pelo teclado (WASD/setas), e o jogador pode carregar e executar chutes. A física da bola, com momento e atrito, é gerenciada em `updateBallPhysics`.
    -   **Detalhes adicionais:** O carregamento do chute possui uma barra visual no HUD e o domínio inicial de bola aplica uma redução de velocidade orgânica.

-   **c) Crie um placar:**
    -   **Como foi cumprido:** A função `checkGoalDetection()` detecta a entrada da bola no gol (respeitando a linha demarcatória), atualiza as variáveis de placar e aciona a comemoração visual na tela. O placar é renderizado por `drawHud()`, que agora também exibe o cronômetro, posse de bola, dificuldade e histórico de vitórias da sessão atual (A / B / Empates).

-   **e) e f) Crie os jogadores e a torcida:**
    -   **Como foi cumprido:** Jogadores desenhados através de código procedural (`drawPlayers()`), com uniformes detalhados apresentando listras texturizadas no uniforme brasileiro. A torcida é composta por arquibancadas elípticas contendo assentos codificados por cores usando algoritmos de *hash* em `drawBackgroundAndStands()`. A intensidade luminosa/cor da torcida pulsa mediante gols (`goalFlashTimer_`) e eventos perigosos.

-   **g) Incluir som:**
    -   **Como foi cumprido:** O `AudioSystem` usa OpenAL para gerar ambiência procedural (cantos, assobios e vibração natural contínua) que reage ativamente à movimentação do jogo. Além disso, se existir algum arquivo externo (`.mp3`, `.wav`) na pasta `sounds/`, o motor tenta convertê-lo via `ffmpeg` em tempo de execução e o carrega em loop constante.

-   **h) Crie uma mecânica de movimentação dos jogadores atrás da bola:**
    -   **Como foi cumprido:** A IA (em `updatePlayersAI`) é sistêmica e atua sobre todos os NPCs em campo. Eles assumem papéis definidos: o goleiro monitora ângulos de trave sem ser invencível; a zaga aguarda posicionada e o ataque busca desmarques. Os jogadores interagem fisicamente empurrando uns aos outros na disputa pela bola, e a velocidade geral da IA é regida pela variável de Dificuldade selecionada (1, 2, ou 3).

-   **i) Seja criativo e crie um item:**
    -   **Como foi cumprido:** Vários itens foram criados:
        1.  **Modo Dia/Noite:** Uma tecla (`N`) alterna todo o esquema de cores do jogo.
        2.  **Chute Carregável:** Segurar a barra de espaço carrega a força do chute, adicionando uma camada de habilidade.
        3.  **Áudio Procedural:** Uma solução técnica altamente criativa para o som.
        4.  **Sistema de Dificuldades:** Níveis 1 (Easy), 2 (Medium) e 3 (Hard) em tempo real que afetam IA e comportamento do goleiro.
        5.  **Histórico de Partidas Contínuo:** Capacidade de reiniciar o jogo (`R`) com registro persistente de vitorias no canto da tela.

-   **j) Realismo e Qualidade:**
    -   **Como foi cumprido:** Mantida taxa cravada (aprox 60 FPS viabilizado por `glutTimerFunc`), proporções baseadas nas medidas oficiais da FIFA, colisão precisa em rebotes nas traves de gol, atrito mecânico da bola e uma UI clara sem sobrecarregar a área limpa da partida.

---

## 4. Demais Aspectos do Projeto

Breve visão geral de outros sistemas importantes.

### Arquitetura de Software

-   **Como foi implementado:** O projeto é bem estruturado, dividindo responsabilidades em múltiplos arquivos:
    -   `Game.cpp`: Gerencia o estado geral e o loop principal.
    -   `GameInput.cpp`: Processa a entrada do usuário para controlar diretamente a bola e gerenciar o carregamento e execução de chutes.
    -   `GameSimulation.cpp`: Lida com a física da bola e a IA dos jogadores.
    -   `GameRender.cpp`: Responsável por todo o código de desenho com OpenGL.
    -   `AudioSystem.cpp`: Encapsula toda a lógica de som.
    -   Essa separação de interesses torna o código mais fácil de manter e expandir.

### Sistema de Áudio (OpenAL e Procedural)

-   **Como foi implementado:** O `AudioSystem` é um módulo robusto. Ele verifica a disponibilidade do OpenAL e, se presente, gera em tempo real os sons da torcida, cantos e efeitos usando funções matemáticas (ondas senoidais, ruído). Isso torna o executável leve e autossuficiente. Ele também possui um fallback para um simples "beep" de terminal caso o OpenAL não esteja disponível.
    - Para flexibilização adicional, ele utiliza subprocessos do SO (`popen` com `ffmpeg`) para decodificar e ingerir arquivos `.mp3` dinamicamente se injetados pelo usuário na pasta apropriada.

### Física e Simulação

-   **Como foi implementado:** A função `updateBallPhysics` em `src/GameSimulation.cpp` baseia-se na integração de Euler para o movimento, aplica um fator de amortecimento para simular fricção natural no gramado e implementa um limitador restritivo nas bordas do campo para impedir a saída de bola (exceto na boca do gol). As traves possuem pequenos *colliders* circulares que causam rebatidas orgânicas de acordo com o vetor normal do impacto.

### Sistema de Build e Configuração

-   **Como foi implementado:** O projeto possui build facilitado tanto por um `Makefile` padrão quanto por `CMakeLists.txt`. Outro ponto vital arquitetural é o uso de Data-Driven Design no qual o arquivo `config/game.cfg` injeta configurações de calibração finas (duração, atrito da bola, limiares de chute) antes da inicialização.

---

## 5. Conceitos Adicionais Empregados (Não Listados em `Conceitos.md`)

O arquivo `Conceitos.md` cobre a base teórica de OpenGL, mas o projeto emprega várias técnicas e conceitos adicionais que são cruciais para seu funcionamento e qualidade.

### Animação Baseada em Tempo (`glutTimerFunc`)

-   **O que é:** Enquanto `Conceitos.md` cita `glutIdleFunc` (que executa uma função continuamente, o mais rápido possível), o projeto utiliza `glutTimerFunc`. Esta função de callback da GLUT agenda a execução de uma função após um número específico de milissegundos.
-   **Como foi usado:** Em `main.cpp`, `glutTimerFunc` é configurado para chamar a função `update` a cada `1000 / 60` milissegundos, visando uma taxa de atualização de 60 quadros por segundo (FPS). Isso é fundamental para:
    1.  **Consistência:** A velocidade do jogo (física, IA) se torna independente da velocidade do processador do usuário.
    2.  **Eficiência:** Evita que o loop do jogo consuma 100% da CPU, como aconteceria com `glutIdleFunc`.
    3.  **Animação Suave:** Garante uma cadência regular para as atualizações de tela.

### Geração Procedural de Geometria e Cor

-   **O que é:** Uma abordagem algorítmica onde a geometria e os atributos visuais (como cores) não são carregados de arquivos externos, mas sim gerados em tempo de execução através de código.
-   **Como foi usado:** Este é um pilar do projeto para manter o executável leve e autossuficiente.
    -   **Torcida (`drawBackgroundAndStands`):** As arquibancadas e os "assentos" coloridos são desenhados usando laços e funções matemáticas (elipses, hashes para gerar cores variadas). Isso permite que a torcida reaja dinamicamente a eventos do jogo (como gols) simplesmente alterando os parâmetros de cor no algoritmo.
    -   **Uniformes (`drawPlayers`):** As listras nas camisas dos jogadores são desenhadas proceduralmente sobre um círculo base, evitando a necessidade de texturas.

### Input Handling Completo (Teclas "Up" e Especiais)

-   **O que é:** A GLUT oferece callbacks não apenas para quando uma tecla é pressionada (`glutKeyboardFunc`), mas também para quando ela é solta (`glutKeyboardUpFunc`) e para teclas que não são ASCII, como as setas (`glutSpecialFunc` e `glutSpecialUpFunc`).
-   **Como foi usado:** Essencial para a mecânica de chute. O jogo detecta quando a barra de espaço é pressionada (`glutKeyboardFunc`) para começar a carregar a força, e quando ela é solta (`glutKeyboardUpFunc`) para executar o chute com a força acumulada. As setas são lidas via `glutSpecialFunc`.

### Design Orientado a Dados (Data-Driven Design)

-   **O que é:** Um princípio de arquitetura de software onde a lógica e o comportamento do programa são ditados por dados lidos de fontes externas, em vez de estarem fixos no código-fonte.
-   **Como foi usado:** O arquivo `config/game.cfg` é um exemplo claro. Parâmetros críticos de gameplay como a duração da partida, o atrito da bola e a velocidade dos chutes podem ser ajustados por qualquer pessoa, sem a necessidade de acessar o código C++ e recompilar o projeto. Isso acelera drasticamente o processo de balanceamento e personalização do jogo.