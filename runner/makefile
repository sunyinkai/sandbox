runner: runner.o compiler.o checker.o json.o syscall_monitor.o resource.o fsm.o  bootstrap.o  
	gcc -g checker.o compiler.o runner.o json.o syscall_monitor.o resource.o  fsm.o bootstrap.o  -static -o runner_FSM_docker.out -lseccomp
	rm *.o

runner.o: runner.c  runner.h 
	gcc -g  -c runner.c  -o runner.o

compiler.o:compiler.c compiler.h 
	gcc -g -c compiler.c  -o compiler.o

checker.o: checker.c  checker.h 
	gcc -g -Wno-format-security -c checker.c  -o checker.o

json.o :include/cJSON.c include/cJSON.h
	gcc -g -c include/cJSON.c -o json.o

syscall_monitor.o: syscall_monitor.c syscall_monitor.h
	gcc -g -Wno-pointer-to-int-cast -c syscall_monitor.c -o syscall_monitor.o

resource.o :resource.c resource.h
	gcc -g -Wno-int-conversion -c resource.c -o resource.o

fsm.o:fsm.c fsm.h
	gcc -g -c fsm.c -o fsm.o

bootstrap.o:bootstrap.c
	gcc -g -DDEBUG -c  bootstrap.c  -o bootstrap.o

clean:
	/bin/rm -rf runner
