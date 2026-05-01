Notes pt 2 — basics first, in the order I'd review them
========================================================

These are the "what should ALWAYS happen" notes. They live next to the
existing `Code Notes` and `Lecture 17 code explained` folders, but they
are tighter and rule-focused, not full programs. Each .c file compiles
and runs on its own.

  00_threads_basics.c              create / join / passing args
  01_mutex_basics.c                init / lock / unlock / destroy
  02_condvar_basics.c              cond_wait, the GOLDEN RULES, signal vs broadcast
  03_init_destroy_cheatsheet.c     every sync object's init/destroy form, in one file
  04_rwlock_and_barrier_basics.c   readers/writers + barrier, like the winner.c problem
  05_always_rules.txt              one-page rules cheat sheet (no code)

Compile any of them with:

  gcc -Wall <file>.c -o <file> -lpthread

Read in order if you're starting from scratch. If you only have 5 minutes,
read 05_always_rules.txt and 02_condvar_basics.c.
