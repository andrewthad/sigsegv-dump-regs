# Change Registers in SIGSEGV Handler

This simple library is a proof of concept for two things:

1. Printing out values in the x86-64 registers inside of a signal handler
   for SIGSEGV. This print the values at the time that the SIGSEGV was
   generated.
2. Modify these values before starting the application back. The faulting
   instruction is retried, but since we have reassigned the crucial registers
   used to carry the index a `for` loop, the second time it attempts the
   `movb` instruction, it succeeds.

In the application, it is known that `REG_RDX` and `REG_RAX` need to be
modified to continue running the application. This was learned by dumping
the assembly and examining which registers were used in the loop. The
technique used here is not portable, and it isn't even stable in the
presence of optimizations. In fact, it's not something that you can generally
do in C since register allocation cannot be predicted. The real use for
this powerful technique would be a copying garbage collector. This would
require a compiler that could generate both assembly with some serious
invariants on register use (e.g. registers never hold derived pointers).
And the compiler would have to generate, for each instruction that
initialized a bump-allocated pointer, the set of all registers holding
managed pointers.

Such an undertaking would be grueling. I'm not aware of any project that
exploits virtual memory in this way. (The JVM does use SIGSEGV, but I do
not believe that it messes with the general-purpose registers.) The
advantages of this VM-driven approach to garbage collection over traditional
safepoint methods are:

1. No need to compare the heap pointer against a minimum/maximum bound.
2. No need to spill registers with freshly allocated data to the stack
   and reload them after the garbage collector runs.

Together, these would considerably reduce the size of the generated code
around all allocation sites. Would this lead to tangible performance gains?
No idea. Someone has to try it, but it'll be a bear to implement.
