# Compilador e flags
CC = gcc
CFLAGS = -Wall -O2

# Nome do executável
EXEC = ecosystem

# Ficheiros fonte
SRC = ecosystem.c

# Encontrar todos os ficheiros de input
INPUT_FILES = $(wildcard input*)

# Gerar nomes dos ficheiros de output esperados
EXPECTED_OUTPUTS = $(patsubst input%,output%,$(INPUT_FILES))

# Cores para output
RED = \033[0;31m
GREEN = \033[0;32m
YELLOW = \033[1;33m
NC = \033[0m # No Color

# Timeout para evitar bloqueio (em segundos)
TIMEOUT = 5

# Alvo principal: compilar e executar todos os testes
all: $(EXEC) run-tests

# Compilar o programa
$(EXEC): $(SRC)
	@echo "$(YELLOW)Compilando $(SRC)...$(NC)"
	$(CC) $(CFLAGS) -o $(EXEC) $(SRC)
	@echo "$(GREEN)Compilação concluída!$(NC)"

# Executar todos os testes
run-tests: $(EXEC)
	@echo "\n$(YELLOW)======================================$(NC)"
	@echo "$(YELLOW)Executando testes...$(NC)"
	@echo "$(YELLOW)======================================$(NC)\n"
	@passed=0; failed=0; \
	for input in $(INPUT_FILES); do \
		suffix=$${input#input}; \
		expected="output$$suffix"; \
		if [ ! -f "$$expected" ]; then \
			echo "$(RED)✗ AVISO: Ficheiro $$expected não encontrado para $$input$(NC)"; \
			continue; \
		fi; \
		echo "$(YELLOW)Testando $$input...$(NC)"; \
		timeout $(TIMEOUT)s ./$(EXEC) < $$input > temp_output.txt 2>&1; \
		if [ $$? -eq 124 ]; then \
			echo "$(RED)✗ FALHOU: $$input (timeout)$(NC)"; \
			failed=$$((failed + 1)); \
			continue; \
		fi; \
		if diff -q temp_output.txt $$expected > /dev/null 2>&1; then \
			echo "$(GREEN)✓ PASSOU: $$input -> $$expected$(NC)"; \
			passed=$$((passed + 1)); \
		else \
			echo "$(RED)✗ FALHOU: $$input$(NC)"; \
			echo "$(RED)  Diferenças encontradas:$(NC)"; \
			diff temp_output.txt $$expected | head -20; \
			failed=$$((failed + 1)); \
		fi; \
		echo ""; \
	done; \
	rm -f temp_output.txt; \
	echo "$(YELLOW)Resumo: $(GREEN)$$passed passou(m)$(NC) $(YELLOW)|$(NC) $(RED)$$failed falhou(aram)$(NC)"; \
	if [ $$failed -gt 0 ]; then exit 1; fi

# Testar um ficheiro específico
# Uso: make test-one INPUT=input10x10
test-one: $(EXEC)
	@if [ -z "$(INPUT)" ]; then \
		echo "$(RED)Erro: especifique INPUT=<nome_ficheiro>$(NC)"; \
		echo "Exemplo: make test-one INPUT=input10x10"; \
		exit 1; \
	fi
	@suffix=$${INPUT#input}; \
	expected="output$$suffix"; \
	if [ ! -f "$(INPUT)" ]; then \
		echo "$(RED)Erro: Ficheiro $(INPUT) não encontrado$(NC)"; \
		exit 1; \
	fi; \
	if [ ! -f "$$expected" ]; then \
		echo "$(RED)Erro: Ficheiro $$expected não encontrado$(NC)"; \
		exit 1; \
	fi; \
	echo "$(YELLOW)Testando $(INPUT)...$(NC)"; \
	timeout $(TIMEOUT)s ./$(EXEC) < $(INPUT) > temp_output.txt 2>&1; \
	if [ $$? -eq 124 ]; then \
		echo "$(RED)✗ FALHOU: $(INPUT) (timeout)$(NC)"; \
	else \
		if diff -q temp_output.txt $$expected > /dev/null 2>&1; then \
			echo "$(GREEN)✓ PASSOU: $(INPUT) -> $$expected$(NC)"; \
		else \
			echo "$(RED)✗ FALHOU: $(INPUT)$(NC)"; \
			echo "$(RED)Diferenças:$(NC)"; \
			diff temp_output.txt $$expected; \
		fi; \
	fi; \
	rm -f temp_output.txt

# Gerar output para um input específico (sem comparação)
# Uso: make run INPUT=input10x10
run: $(EXEC)
	@if [ -z "$(INPUT)" ]; then \
		echo "$(RED)Erro: especifique INPUT=<nome_ficheiro>$(NC)"; \
		echo "Exemplo: make run INPUT=input10x10"; \
		exit 1; \
	fi
	@if [ ! -f "$(INPUT)" ]; then \
		echo "$(RED)Erro: Ficheiro $(INPUT) não encontrado$(NC)"; \
		exit 1; \
	fi
	@echo "$(YELLOW)Executando $(INPUT)...$(NC)"
	@timeout $(TIMEOUT)s ./$(EXEC) < $(INPUT)

# Mostrar diferenças detalhadas para um teste específico
# Uso: make diff INPUT=input10x10
diff: $(EXEC)
	@if [ -z "$(INPUT)" ]; then \
		echo "$(RED)Erro: especifique INPUT=<nome_ficheiro>$(NC)"; \
		echo "Exemplo: make diff INPUT=input10x10"; \
		exit 1; \
	fi
	@suffix=$${INPUT#input}; \
	expected="output$$suffix"; \
	if [ ! -f "$(INPUT)" ]; then \
		echo "$(RED)Erro: Ficheiro $(INPUT) não encontrado$(NC)"; \
		exit 1; \
	fi; \
	if [ ! -f "$$expected" ]; then \
		echo "$(RED)Erro: Ficheiro $$expected não encontrado$(NC)"; \
		exit 1; \
	fi; \
	timeout $(TIMEOUT)s ./$(EXEC) < $(INPUT) > temp_output.txt 2>&1; \
	echo "$(YELLOW)Diferenças entre output gerado e esperado:$(NC)"; \
	diff -u $$expected temp_output.txt || true; \
	rm -f temp_output.txt

# Listar todos os testes disponíveis
list:
	@echo "$(YELLOW)Ficheiros de teste disponíveis:$(NC)"
	@for input in $(INPUT_FILES); do \
		suffix=$${input#input}; \
		expected="output$$suffix"; \
		if [ -f "$$expected" ]; then \
			echo "  $(GREEN)✓$(NC) $$input -> $$expected"; \
		else \
			echo "  $(RED)✗$(NC) $$input ($$expected não encontrado)"; \
		fi; \
	done

# Limpar ficheiros gerados
clean:
	@echo "$(YELLOW)Limpando ficheiros gerados...$(NC)"
	rm -f $(EXEC) temp_output.txt
	@echo "$(GREEN)Limpeza concluída!$(NC)"

# Limpar tudo (incluindo outputs temporários)
distclean: clean
	rm -f *.o *~

# Ajuda
help:
	@echo "$(YELLOW)Makefile para Simulação de Ecossistema$(NC)"
	@echo ""
	@echo "$(GREEN)Alvos disponíveis:$(NC)"
	@echo "  make              - Compila e executa todos os testes"
	@echo "  make all          - Mesmo que 'make'"
	@echo "  make run-tests    - Executa todos os testes"
	@echo "  make test-one INPUT=<ficheiro> - Testa um ficheiro específico"
	@echo "  make run INPUT=<ficheiro>      - Executa sem comparação"
	@echo "  make diff INPUT=<ficheiro>     - Mostra diferenças detalhadas"
	@echo "  make list         - Lista todos os testes disponíveis"
	@echo "  make clean        - Remove ficheiros gerados"
	@echo "  make help         - Mostra esta ajuda"
	@echo ""
	@echo "$(GREEN)Exemplos:$(NC)"
	@echo "  make test-one INPUT=input10x10"
	@echo "  make run INPUT=input10x10"
	@echo "  make diff INPUT=input10x10"

.PHONY: all run-tests test-one run diff list clean distclean help
