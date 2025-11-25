# Sequential and Parallel execution with timing
INPUTS = input5x5 input10x10 input100x100 input100x100_unbal01 input100x100_unbal02 input200x200
SEQUENTIAL_TIMING_FILE = seq_execution_times.txt
PARALLEL_TIMING_FILE = parallel_execution_times.txt
THREADS = 1 2 4 8 16

sequential: sequential.c
	gcc -fopenmp sequential.c -o sequential

parallel: parallel.c
	gcc -fopenmp parallel.c -o parallel

run_sequential: sequential
	@mkdir -p sequential_outputs
	@> sequential_outputs/$(SEQUENTIAL_TIMING_FILE)
	@echo "Starting sequential execution..."
	@for input in $(INPUTS); do \
		echo "Running with $$input..." ; \
		echo "Input file: $$input" >> sequential_outputs/$(SEQUENTIAL_TIMING_FILE) ; \
		output=$$(./sequential < examples/$$input 2>&1) ; \
		exec_time=$$(echo "$$output" | head -1) ; \
		echo "  ✓ Execution time: $$exec_time ms" ; \
		echo "Execution time: $$exec_time ms" >> sequential_outputs/$(SEQUENTIAL_TIMING_FILE) ; \
		echo "$$output" | tail -n +2 > sequential_outputs/output_$$input ; \
		echo "" >> sequential_outputs/$(SEQUENTIAL_TIMING_FILE) ; \
	done
	@echo "✓ Execution times saved to sequential_outputs/$(SEQUENTIAL_TIMING_FILE)"

clean_sequential:
	rm -rf sequential_outputs sequential $(SEQUENTIAL_TIMING_FILE)

run_parallel: parallel
	@mkdir -p parallel_outputs
	@> parallel_outputs/$(PARALLEL_TIMING_FILE)
	@echo "Starting parallel execution with threads: $(THREADS)..."
	@for threads in $(THREADS); do \
		echo ""; \
		echo "=== Running with $$threads threads ===" ; \
		echo "Running with $$threads threads..." >> parallel_outputs/$(PARALLEL_TIMING_FILE) ; \
		echo "" >> parallel_outputs/$(PARALLEL_TIMING_FILE) ; \
		for input in $(INPUTS); do \
			echo "  Processing $$input..." ; \
			echo "Input file: $$input" >> parallel_outputs/$(PARALLEL_TIMING_FILE) ; \
			output=$$(./parallel $$threads < examples/$$input 2>&1) ; \
			exec_time=$$(echo "$$output" | head -1) ; \
			echo "    ✓ Time: $$exec_time ms" ; \
			echo "Execution time: $$exec_time ms" >> parallel_outputs/$(PARALLEL_TIMING_FILE) ; \
			echo "$$output" | tail -n +2 > parallel_outputs/output_$$threads\_$$input ; \
			echo "" >> parallel_outputs/$(PARALLEL_TIMING_FILE) ; \
		done ; \
		echo "-----------------------------------" >> parallel_outputs/$(PARALLEL_TIMING_FILE) ; \
		echo "" >> parallel_outputs/$(PARALLEL_TIMING_FILE) ; \
	done
	@echo ""
	@echo "✓ Parallel execution times saved to parallel_outputs/$(PARALLEL_TIMING_FILE)"

clean_parallel:
	rm -rf parallel_outputs parallel $(PARALLEL_TIMING_FILE)

clean_sequential:
	rm -rf sequential_outputs sequential $(SEQUENTIAL_TIMING_FILE)

.PHONY: sequential run_sequential clean_sequential parallel run_parallel clean_parallel