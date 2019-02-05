	// Exercise yield
	{
        sys_os_yield();
    }

    // Exercise sleep
	{
        sys_os_usleep(10);
    }

    // Exercise pseudo random number generation
	{
        int r1 = rand();
        int r2 = -1;
        sys_os_srand(1);
        assert(r1 == rand());
        sys_os_srand(20);
        r2 = rand();
        assert(r1 != r2);
        sys_os_srand(20);
        assert(r2 == rand());
    }
