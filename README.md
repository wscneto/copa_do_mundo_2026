# Copa do Mundo 2026 - Jogo de Futebol em OpenGL 2.0

Projeto completo em C++ com OpenGL (freeglut/GLUT), com tema da Copa do Mundo FIFA 2026.

## Recursos Implementados

1. Campo completo e proporcional (escala FIFA)
- Gramado com listras
- Linhas laterais/fundo, linha central e circulo central
- Grandes areas, pequenas areas, marcas de penalti e arcos de penalti
- Arcos de escanteio
- Dois gols com estrutura e rede sugerida por linhas

2. Jogador controlavel + bola fisica
- Controle de um jogador do Team A com WASD ou setas
- Troca de jogador com TAB apenas sem posse, sempre para o jogador mais proximo da bola
- Passe inteligente com E (alvo baseado na direcao) e possibilidade de interceptacao
- Chute carregavel (segurar SPACE, soltar para chutar)
- Dominio e conducao de bola com primeira dominada mais natural
- Bola com momentum e atrito
- Bola respeita limites do campo, com excecao da boca do gol
- Bola desenhada com padrao estilizado de pentagonos

3. Placar e deteccao de gol
- HUD com placar TEAM A x TEAM B
- Gol detectado quando a bola cruza a linha dentro da boca do gol
- Atualizacao de placar e reset para o centro
- Flash visual e texto de comemoracao (GOOOOOL)

4. Jogadores (duas equipes)
- Dois times com cores distintas
- Goleiro, defensores e atacantes em cada lado
- Posicoes iniciais em metades opostas

5. Torcida / arquibancada
- Torcida estilizada em volta de todo o campo
- Reacao visual da torcida apos gols (pulso de cor)

6. Som (OpenAL)
- Ambiencia de estadio em loop + camada de cantos de torcida (procedural)
- Som de gol com comemoracao reforcada da torcida
- Som de toque/chute na bola
- Reacao de torcida para chances perigosas
- Se existir arquivo em `sounds/` (ex.: `football_crowd.mp3`), ele e carregado automaticamente e fica em loop durante o jogo
- Sem necessidade de assets de audio externos (gerado em tempo real)

7. IA tatica basica + goleiro ajustado
- Jogadores se movem em direcao a (bola - jogador) com offsets
- Defensores, meio-campo e atacante com comportamentos distintos
- Time em posse se reposiciona para opcao de passe
- Goleiros usam cobertura por angulo e foram enfraquecidos para nao defender tudo
- Colisao com bola aplica empurrao orientado para frente do time

8. Dificuldade
- Troca em tempo real: 1 (Easy), 2 (Medium), 3 (Hard)
- Ajusta velocidade da IA e eficiencia do goleiro

9. Extra criativo: modo Dia/Noite
- Tecla N alterna entre visual diurno e noturno
- Impacta ceu/estadio/gramado e HUD de modo

10. Modo partida
- Partida com cronometro regressivo
- Fim de jogo automatico
- Resultado final e reinicio com tecla R
- Registro de vitorias da sessao (A/B/empates)

11. Qualidade visual e animacao
- Animacao suave por `glutTimerFunc` (~60 FPS)
- HUD com placar, cronometro, posse, dificuldade e barra de chute
- Sombras fake de jogadores/bola e spotlight perto da bola
- Colisao com traves (rebote em poste)
- Compatibilidade com OpenGL 2.0 (pipeline fixo)

## Estrutura

- `include/Vec2.h`: utilitario 2D
- `include/AudioSystem.h`: interface de audio
- `include/Game.h`: estado do jogo e API
- `src/Game.cpp`: nucleo do jogo (ciclo, estado da partida, posse e callbacks)
- `src/GameInput.cpp`: input do jogador e acoes de passe/chute/troca
- `src/GameSimulation.cpp`: fisica da bola e IA dos jogadores
- `src/GameRender.cpp`: renderizacao do estadio, HUD e overlays
- `src/GameInternal.h`: utilitarios internos compartilhados entre os modulos do jogo
- `src/AudioSystem.cpp`: OpenAL + sons procedurais
- `src/main.cpp`: ponto de entrada
- `config/game.cfg`: ajuste externo de parametros de gameplay
- `Makefile`: build principal
- `CMakeLists.txt`: alternativa de build via CMake

## Dependencias (Linux)

Pacotes tipicos (Debian/Ubuntu):

```bash
sudo apt update
sudo apt install -y g++ make freeglut3-dev libopenal-dev pkg-config
```

## Compilacao

### Opcao 1 (recomendada): Makefile

```bash
make
```

### Opcao 2: CMake

```bash
cmake -S . -B build
cmake --build build -j
```

## Execucao

```bash
./worldcup2026
```

## Controles

- `W A S D` ou setas: movimentar jogador selecionado
- `TAB`: trocar jogador controlado (Team A)
- `E`: passe (direcional, com risco de interceptacao)
- `SPACE`: segurar para carregar, soltar para chutar
- `1`, `2`, `3`: dificuldade (easy, medium, hard)
- `N`: alternar Dia/Noite
- `R`: reiniciar partida
- `ESC`: sair

## Arquivo de Configuracao

O jogo pode ser ajustado via `config/game.cfg` (sem recompilar), por exemplo:
- `matchDurationSeconds`
- `ballFriction`
- `dribbleFollow`
- `dribbleCarrySpeed`
- `passSpeed`
- `shotMinSpeed`
- `shotMaxSpeed`
- `dangerousShotSpeed`
- `difficulty` (`easy`, `medium`, `hard`)

## Observacoes Tecnicas

- O codigo foi implementado para OpenGL 2.0 (compatibility profile, immediate mode).
- O audio usa OpenAL quando disponivel (`HAS_OPENAL`) com mix dinamico por nivel de emocao da partida.
- Se OpenAL nao estiver presente no build, o projeto ainda compila e usa aviso sonoro simples de fallback.
- Para audio externo em formatos como `.mp3`, `.ogg`, `.flac` e `.m4a`, o jogo tenta converter via `ffmpeg` em tempo de execucao.
- Para evitar dependencia de `ffmpeg`, use `.wav` PCM 16-bit na pasta `sounds/`.
