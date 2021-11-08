compile:
		@gcc -g  -Wall -Wextra cache_sim.c -o cache_sim -lm
sim1:
		@./cache_sim 128 dm uc | grep "Hit Rate"
		@echo should be 0.6425
		@echo -----------------
sim2:
		@./cache_sim 128 dm sc | grep "Hit Rate"
		@echo should be 0.8152
		@echo -----------------
sim3:
		@./cache_sim 128 fa uc | grep "Hit Rate"
		@echo should be 0.7611
		@echo -----------------
sim4:
		@./cache_sim 128 fa sc | grep "Hit Rate"
		@echo should be 0.8152
		@echo -----------------
sim5:
		@./cache_sim 4096 dm uc | grep "Hit Rate"
		@echo should be 0.9288
		@echo -----------------
sim6:
		@./cache_sim 4096 dm sc | grep "Hit Rate"
		@echo should be 0.9335
		@echo -----------------
sim7:
		@./cache_sim 4096 fa uc | grep "Hit Rate"
		@echo should be 0.9309
		@echo -----------------
sim8:
		@./cache_sim 4096 fa sc | grep "Hit Rate"
		@echo should be 0.9309
		@echo -----------------

all: compile sim1 sim2 sim3 sim4 sim5 sim6 sim7 sim8