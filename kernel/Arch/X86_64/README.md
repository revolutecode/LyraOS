# x86-64 architecture layer

Architecture-specific CPU setup belongs here: early entry, GDT/IDT, interrupt entry stubs, paging helpers, and CPU feature detection.

Keep generic kernel policy outside this directory. Do not assume that x86-64 register state or interrupt semantics apply to other architectures.
