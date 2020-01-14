runner_FSM: runner_FSM.c syscall_monitor.c syscall_monitor.h
	gcc runner_FSM.c syscall_monitor.c syscall_monitor.h -o runner_FSM -lseccomp
runner: runner.c syscall_monitor.c syscall_monitor.h
	gcc runner.c syscall_monitor.c syscall_monitor.h -o runner -lseccomp
clean:
	/bin/rm -rf runner
