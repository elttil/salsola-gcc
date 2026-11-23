#undef TARGET_SALSOLA
#define TARGET_SALSOLA 1

#undef LIB_SPEC
#define LIB_SPEC "-lc"

/* Files that are linked before user code.
   The %s tells GCC to look for these files in the library directory. */
#undef STARTFILE_SPEC
#define STARTFILE_SPEC "crt0.o%s crti.o%s %{static-pie:crtbeginS.o%s;:crtbegin.o%s}"

/* Files that are linked after user code. */
#undef ENDFILE_SPEC
#define ENDFILE_SPEC "%{static-pie:crtendS.o%s;:crtend.o%s} crtn.o%s"

#undef LINK_SPEC
#define LINK_SPEC "%{static-pie:-static -pie --no-dynamic-linker -z text}"

/* Additional predefined macros. */
#undef TARGET_OS_CPP_BUILTINS
#define TARGET_OS_CPP_BUILTINS()      \
  do {                                \
    builtin_define ("__salsola__");      \
    builtin_define ("__unix__");      \
    builtin_assert ("system=salsola");   \
    builtin_assert ("system=unix");   \
    builtin_assert ("system=posix");   \
  } while(0);

