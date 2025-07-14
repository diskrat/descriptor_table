# Tabela de Descritores

## Objetivo
Implementar uma tabela de descritores de arquivos em C, semelhante às usadas em sistemas Unix/Linux, incluindo:

- Alocação do índice disponível mais baixo (conforme exigido pelo POSIX).
- Expansão dinâmica com um limite máximo.
- Duplicação e redirecionamento de descritores (`dup`, `dup2`).
- Contagem de referências para recursos.
- Suporte para leitura, escrita e fechamento permanente de recursos.
- Concorrência segura entre múltiplas threads.

## Descrição Geral
Em sistemas Unix, descritores de arquivos são inteiros que referenciam recursos (arquivos, pipes, sockets, etc.). Esses recursos possuem contadores de referência: quando não há descritores apontando para um recurso, ele é destruído.

Neste projeto, o objetivo é implementar uma estrutura de tabela de descritores de arquivos com as seguintes funcionalidades:

## Requisitos Funcionais

1. **Tabela de Descritores**:
   - Uma tabela de descritores com um tamanho inicial e um tamanho máximo.
   - Cresce dinamicamente até o limite máximo permitido.

2. **Alocação do Índice Livre Mais Baixo**:
   - Sempre alocar o índice disponível mais baixo.

3. **Fechamento de Descritor**:
   - Fechar o descritor e decrementar o contador de referência do recurso.
   - Destruir o recurso quando o contador de referência atingir zero.

4. **Contagem de Referências**:
   - Manter um contador de referência para os recursos.
   - Garantir que um descritor aponte para um recurso válido.
   - Destruir recursos quando nenhum descritor os referenciar.
   - Quando a contagem atingir um valor máximo, tornar o descritor permanente.

5. **Concorrência**:
   - Garantir que a implementação funcione corretamente em execução paralela.

## Status da Implementação

### Funcionalidades Implementadas
- **Tabela de Descritores**:
  - Uma tabela com expansão dinâmica e um tamanho máximo.
  - Gerenciamento de tamanho inicial e capacidade implementados.

- **Alocação do Índice Livre Mais Baixo**:
  - Funcionalidade para encontrar e alocar o índice disponível mais baixo.

- **Fechamento de Descritor**:
  - Função `user_close` implementada para decrementar o contador de referência e liberar recursos.

- **Contagem de Referências**:
  - Gerenciamento de contagem de referência implementado em `user_open` e `user_close`.

- **Concorrência**:
  - Mutex (`pthread_mutex_t`) usado para garantir operações seguras entre threads(ineficiente).

### Funcionalidades Faltantes
- **Duplicação e Redirecionamento**:
  - Funções `dup` e `dup2` não estão implementadas.

- **Contagem de referência**:
  - Saturação de contagem de referência não está implementada.

## Execução
1. Compile o programa:
   ```bash
   gcc main.c -o main.out -pthread
   ```
2. Execute o programa:
   ```bash
   ./main.out
   ```