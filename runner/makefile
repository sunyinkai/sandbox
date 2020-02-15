runner_FSM: runner.c syscall_monitor.c resource.c compiler.c fsm.c  bootstrap.c syscall_monitor.h resource.h fsm.h compiler.h runner.h  include/cJSON.c include/cJSON.h  checker.c checker.h
	gcc runner.c syscall_monitor.c resource.c compiler.c fsm.c bootstrap.c syscall_monitor.h fsm.h resource.h compiler.h runner.h include/cJSON.c include/cJSON.h checker.c checker.h -static -D DEBUG -o runner_FSM.out -lseccomp
#runner: runner.c syscall_monitor.c syscall_monitor.h
#	gcc runner.c syscall_monitor.c syscall_monitor.h -o runner -lseccomp
clean:
	/bin/rm -rf runner
