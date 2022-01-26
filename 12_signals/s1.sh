for zm in {1..3}; do
	for S in SIGUSR{1,2} SIGRTMIN; do
		kill "-$S" 11060
		sleep 0.2
	done
	sleep 0.8
done
