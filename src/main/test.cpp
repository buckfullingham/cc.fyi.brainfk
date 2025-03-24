// clang -O3 -S -emit-llvm test2.c -o test.ll
//
//
// extern void putc(unsigned char);


//typedef void (*program_t)(unsigned char* ptr, void(*putc)(unsigned char), unsigned char(*getc)());

extern "C" void
program(unsigned char* ptr) {
	//for (auto& i : memory)
		//i = 0;

	++*ptr;
	/*
	++*ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	if (!*ptr)
		goto label2;
label1:
	++ptr;
	++*ptr;
	++ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	++ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	++ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	--ptr;
	--ptr;
	--ptr;
	--ptr;
	--*ptr;
label2:
	if (*ptr)
		goto label1;
	++ptr;
	++ptr;
	++ptr;
	++*ptr;
	++*ptr;
	putc(*ptr);
	++ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	++*ptr;
	putc(*ptr);
	--ptr;
	--ptr;
	--ptr;
	putc(*ptr);
	*/
}

// extern "C" void execute(program_t program) {
  // static unsigned char memory[30'000]{};
// 
  // program(memory, [](unsigned char c) {}, []() -> unsigned char {return {};});
// 
// }

