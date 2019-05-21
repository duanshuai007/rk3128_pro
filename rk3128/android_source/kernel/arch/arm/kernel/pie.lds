/*
 *
 * Automatically generated file; DO NOT EDIT.
 * Linux/arm 3.10.0 Kernel Configuration
 *
 */
/*
 * Helper macros to use CONFIG_ options in C/CPP expressions. Note that
 * these only work with boolean and tristate options.
 */
/*
 * Getting something that works in C and CPP for an arg that may or may
 * not be defined is tricky.  Here, if we have "#define CONFIG_BOOGER 1"
 * we match on the placeholder define, insert the "0," for arg1 and generate
 * the triplet (0, 1, 0).  Then the last step cherry picks the 2nd arg (a one).
 * When CONFIG_BOOGER is not defined, we generate a (... 1, 0) pair, and when
 * the last step cherry picks the 2nd arg, we get a zero.
 */






/*
 * IS_ENABLED(CONFIG_FOO) evaluates to 1 if CONFIG_FOO is set to 'y' or 'm',
 * 0 otherwise.
 *
 */



/*
 * IS_BUILTIN(CONFIG_FOO) evaluates to 1 if CONFIG_FOO is set to 'y', 0
 * otherwise. For boolean options, this is equivalent to
 * IS_ENABLED(CONFIG_FOO).
 */


/*
 * IS_MODULE(CONFIG_FOO) evaluates to 1 if CONFIG_FOO is set to 'm', 0
 * otherwise.
 */
/*
 * ld script to make ARM PIEs
 * taken from the ARM vmlinux.lds.S version by Russ Dill <russ.dill@ti.com.
 */
/*
 * Helper macros to support writing architecture specific
 * pie linker scripts.
 *
 * A minimal linker scripts has following content:
 * [This is a sample, architectures may have special requiriements]
 *
 * OUTPUT_FORMAT(...)
 * OUTPUT_ARCH(...)
 * SECTIONS
 * {
 *	. = 0x0;
 *
 *	PIE_COMMON_START
 *	.text {
 *		PIE_TEXT_TEXT
 *	}
 *	PIE_COMMON_END
 *
 *	PIE_OVERLAY_START
 *	OVERLAY : NOCROSSREFS {
 *		PIE_OVERLAY_SECTION(am33xx)
 *		PIE_OVERLAY_SECTION(am347x)
 *		[...]
 *	}
 *	PIE_OVERLAY_END
 *
 *	PIE_DISCARDS		// must be the last
 * }
 */
/*
 * Helper macros to support writing architecture specific
 * linker scripts.
 *
 * A minimal linker scripts has following content:
 * [This is a sample, architectures may have special requiriements]
 *
 * OUTPUT_FORMAT(...)
 * OUTPUT_ARCH(...)
 * ENTRY(...)
 * SECTIONS
 * {
 *	. = START;
 *	__init_begin = .;
 *	HEAD_TEXT_SECTION
 *	INIT_TEXT_SECTION(PAGE_SIZE)
 *	INIT_DATA_SECTION(...)
 *	PERCPU_SECTION(CACHELINE_SIZE)
 *	__init_end = .;
 *
 *	_stext = .;
 *	TEXT_SECTION = 0
 *	_etext = .;
 *
 *      _sdata = .;
 *	RO_DATA_SECTION(PAGE_SIZE)
 *	RW_DATA_SECTION(...)
 *	_edata = .;
 *
 *	EXCEPTION_TABLE(...)
 *	NOTES
 *
 *	BSS_SECTION(0, 0, 0)
 *	_end = .;
 *
 *	STABS_DEBUG
 *	DWARF_DEBUG
 *
 *	DISCARDS		// must be the last
 * }
 *
 * [__init_begin, __init_end] is the init section that may be freed after init
 * [_stext, _etext] is the text section
 * [_sdata, _edata] is the data section
 *
 * Some of the included output section have their own set of constants.
 * Examples are: [__initramfs_start, __initramfs_end] for initramfs and
 *               [__nosave_begin, __nosave_end] for the nosave data
 */
/*
 * Export symbols from the kernel to modules.  Forked from module.h
 * to reduce the amount of pointless cruft we feed to gcc when only
 * exporting a simple symbol or two.
 *
 * Try not to add #includes here.  It slows compilation and makes kernel
 * hackers place grumpy comments in header files.
 */
/* Some toolchains use a `_' prefix for all user symbols. */
/* Indirect, so macros are expanded before pasting. */
/* Align . to a 8 byte boundary equals to maximum function alignment. */
/*
 * Align to a 32 byte boundary equal to the
 * alignment gcc 4.5 uses for a struct
 */
/* The actual configuration determine if the init/exit sections
 * are handled as text/data or they can be discarded (which
 * often happens at runtime)
 */
/* .data section */
/*
 * Data section helpers
 */
/*
 * Read only Data
 */
/* RODATA & RO_DATA provided for backward compatibility.
 * All archs are supposed to use RO_DATA() */
/* .text section. Map to function alignment to avoid address changes
 * during second ld run in second ld pass when generating System.map */
/* sched.text is aling to function alignment to secure we have same
 * address even at second ld pass when generating System.map */
/* spinlock.text is aling to function alignment to secure we have same
 * address even at second ld pass when generating System.map */
/* Section used for early init (in .S files) */
/*
 * Exception table
 */
/*
 * Init task
 */
/* init and exit section handling */
/*
 * bss (Block Started by Symbol) - uninitialized data
 * zeroed during startup
 */
/*
 * Allow archectures to redefine BSS_FIRST_SECTIONS to add extra
 * sections to the front of bss.
 */
/*
 * DWARF debug sections.
 * Symbols in the DWARF debugging sections are relative to
 * the beginning of the section so we begin them at 0.
 */
  /* Stabs debugging sections.  */
/*
 * Default discarded sections.
 *
 * Some archs want to discard exit text/data at runtime rather than
 * link time due to cross-section references such as alt instructions,
 * bug table, eh_frame, etc.  DISCARDS must be the last of output
 * section definitions so that such archs put those in earlier section
 * definitions.
 */
/**
 * PERCPU_INPUT - the percpu input sections
 * @cacheline: cacheline size
 *
 * The core percpu section names and core symbols which do not rely
 * directly upon load addresses.
 *
 * @cacheline is used to align subsections to avoid false cacheline
 * sharing between subsections for different purposes.
 */
/**
 * PERCPU_VADDR - define output section for percpu area
 * @cacheline: cacheline size
 * @vaddr: explicit base address (optional)
 * @phdr: destination PHDR (optional)
 *
 * Macro which expands to output section for percpu area.
 *
 * @cacheline is used to align subsections to avoid false cacheline
 * sharing between subsections for different purposes.
 *
 * If @vaddr is not blank, it specifies explicit base address and all
 * percpu symbols will be offset from the given address.  If blank,
 * @vaddr always equals @laddr + LOAD_OFFSET.
 *
 * @phdr defines the output PHDR to use if not blank.  Be warned that
 * output PHDR is sticky.  If @phdr is specified, the next output
 * section in the linker script will go there too.  @phdr should have
 * a leading colon.
 *
 * Note that this macros defines __per_cpu_load as an absolute symbol.
 * If there is no need to put the percpu section at a predetermined
 * address, use PERCPU_SECTION.
 */
/**
 * PERCPU_SECTION - define output section for percpu area, simple version
 * @cacheline: cacheline size
 *
 * Align to PAGE_SIZE and outputs output section for percpu area.  This
 * macro doesn't manipulate @vaddr or @phdr and __per_cpu_load and
 * __per_cpu_start will be identical.
 *
 * This macro is equivalent to ALIGN(PAGE_SIZE); PERCPU_VADDR(@cacheline,,)
 * except that __per_cpu_load is defined as a relative symbol against
 * .data..percpu which is required for relocatable x86_32 configuration.
 */
/*
 * Definition of the high level *_SECTION macros
 * They will fit only a subset of the architectures
 */
/*
 * Writeable data.
 * All sections are combined in a single .data section.
 * The sections following CONSTRUCTORS are arranged so their
 * typical alignment matches.
 * A cacheline is typical/always less than a PAGE_SIZE so
 * the sections that has this restriction (or similar)
 * is located before the ones requiring PAGE_SIZE alignment.
 * NOSAVE_DATA starts and ends with a PAGE_SIZE alignment which
 * matches the requirement of PAGE_ALIGNED_DATA.
 *
 * use 0 as page_align if page_aligned data is not used */
OUTPUT_ARCH(arm)
SECTIONS
{
 . = 0x0;
 __pie_common_start : { __pie_common_start = .; }
 .got.plt : {
  *(.got)
  *(.got.plt)
 }
 .text : {
  KEEP(*(.pie.text))
 }
 __pie_common_end : { __pie_common_end = .; }
 __pie_rel_dyn_start : {
  __pie_rel_dyn_start = .;
 }
 .rel.dyn : {
  KEEP(*(.rel.pie.text))
 }
 __pie_rel_dyn_end : {
  __pie_rel_dyn_end = .;
 }
 __pie_overlay_start : { __pie_overlay_start = .; }
 OVERLAY : NOCROSSREFS {
  .pie.overlay { KEEP(*(.pie.overlay.*)) __pie_overlay_start = LOADADDR(.pie.overlay); __pie_overlay_end = LOADADDR(.pie.overlay) + SIZEOF(.pie.overlay); } .rel.overlay { KEEP(*(.rel.pie.overlay.*)) __pie_rel_overlay_start = LOADADDR(.rel.overlay); __pie_rel_overlay_end = LOADADDR(.rel.overlay) + SIZEOF(.rel.overlay); }
  .pie.rk3288 { KEEP(*(.pie.rk3288.*)) __pie_rk3288_start = LOADADDR(.pie.rk3288); __pie_rk3288_end = LOADADDR(.pie.rk3288) + SIZEOF(.pie.rk3288); } .rel.rk3288 { KEEP(*(.rel.pie.rk3288.*)) __pie_rel_rk3288_start = LOADADDR(.rel.rk3288); __pie_rel_rk3288_end = LOADADDR(.rel.rk3288) + SIZEOF(.rel.rk3288); }
  .pie.rk3188 { KEEP(*(.pie.rk3188.*)) __pie_rk3188_start = LOADADDR(.pie.rk3188); __pie_rk3188_end = LOADADDR(.pie.rk3188) + SIZEOF(.pie.rk3188); } .rel.rk3188 { KEEP(*(.rel.pie.rk3188.*)) __pie_rel_rk3188_start = LOADADDR(.rel.rk3188); __pie_rel_rk3188_end = LOADADDR(.rel.rk3188) + SIZEOF(.rel.rk3188); }
  .pie.rk3036 { KEEP(*(.pie.rk3036.*)) __pie_rk3036_start = LOADADDR(.pie.rk3036); __pie_rk3036_end = LOADADDR(.pie.rk3036) + SIZEOF(.pie.rk3036); } .rel.rk3036 { KEEP(*(.rel.pie.rk3036.*)) __pie_rel_rk3036_start = LOADADDR(.rel.rk3036); __pie_rel_rk3036_end = LOADADDR(.rel.rk3036) + SIZEOF(.rel.rk3036); }
  .pie.rk312x { KEEP(*(.pie.rk312x.*)) __pie_rk312x_start = LOADADDR(.pie.rk312x); __pie_rk312x_end = LOADADDR(.pie.rk312x) + SIZEOF(.pie.rk312x); } .rel.rk312x { KEEP(*(.rel.pie.rk312x.*)) __pie_rel_rk312x_start = LOADADDR(.rel.rk312x); __pie_rel_rk312x_end = LOADADDR(.rel.rk312x) + SIZEOF(.rel.rk312x); }
  .pie.rk3126b { KEEP(*(.pie.rk3126b.*)) __pie_rk3126b_start = LOADADDR(.pie.rk3126b); __pie_rk3126b_end = LOADADDR(.pie.rk3126b) + SIZEOF(.pie.rk3126b); } .rel.rk3126b { KEEP(*(.rel.pie.rk3126b.*)) __pie_rel_rk3126b_start = LOADADDR(.rel.rk3126b); __pie_rel_rk3126b_end = LOADADDR(.rel.rk3126b) + SIZEOF(.rel.rk3126b); }
 }
 __pie_overlay_end : { __pie_overlay_end = .; }
 /DISCARD/ : { *(.dynsym) *(.dynstr*) *(.dynamic*) *(.plt*) *(.interp*) *(.gnu*) *(.hash) *(.comment) *(.bss*) *(.data) *(.discard) *(.discard.*) }
}
