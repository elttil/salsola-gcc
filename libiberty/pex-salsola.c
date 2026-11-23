/* Salsola support for pexecute.

   A child is a process capability from fork_handle (), which is what
   waitod () and process_kill () take, so that od stands in for a pid
   between exec_child and wait.  There are no plain pipes to hand a
   child, so PEX_USE_PIPES is refused and callers fall back to
   temporary files.  */

#include "config.h"
#include "libiberty.h"
#include "pex-common.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

static int pex_salsola_open_read (struct pex_obj *, const char *, int);
static int pex_salsola_open_write (struct pex_obj *, const char *, int, int);
static pid_t pex_salsola_exec_child (struct pex_obj *, int, const char *,
				     char * const *, char * const *,
				     int, int, int, int,
				     const char **, int *);
static int pex_salsola_close (struct pex_obj *, int);
static pid_t pex_salsola_wait (struct pex_obj *, pid_t, int *,
			       struct pex_time *, int, const char **, int *);
static int pex_salsola_pipe (struct pex_obj *, int *, int);
static FILE *pex_salsola_fdopenr (struct pex_obj *, int, int);
static FILE *pex_salsola_fdopenw (struct pex_obj *, int, int);

const struct pex_funcs funcs =
{
  pex_salsola_open_read,
  pex_salsola_open_write,
  pex_salsola_exec_child,
  pex_salsola_close,
  pex_salsola_wait,
  pex_salsola_pipe,
  pex_salsola_fdopenr,
  pex_salsola_fdopenw,
  NULL
};

struct pex_obj *
pex_init (int flags, const char *pname, const char *tempbase)
{
  return pex_init_common (flags, pname, tempbase, &funcs);
}

static int
pex_salsola_open_read (struct pex_obj *obj ATTRIBUTE_UNUSED,
		       const char *name, int binary ATTRIBUTE_UNUSED)
{
  return open (name, O_RDONLY);
}

static int
pex_salsola_open_write (struct pex_obj *obj ATTRIBUTE_UNUSED,
			const char *name, int binary ATTRIBUTE_UNUSED,
			int append)
{
  return open (name, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC),
	       0666);
}

static int
pex_salsola_close (struct pex_obj *obj ATTRIBUTE_UNUSED, int fd)
{
  return close (fd);
}

static void
child_fail (const char *what, const char *executable)
{
  int saved = errno;
  fprintf (stderr, "%s: %s: %s\n", what, executable, xstrerror (saved));
  _exit (-1);
}

static pid_t
pex_salsola_exec_child (struct pex_obj *obj ATTRIBUTE_UNUSED, int flags,
			const char *executable, char * const * argv,
			char * const * env, int in, int out, int errdes,
			int toclose, const char **errmsg, int *err)
{
  int child;
  pid_t pid = fork_handle (&child);

  if (pid < 0)
    {
      *err = errno;
      *errmsg = "fork";
      return (pid_t) -1;
    }

  if (pid == 0)
    {
      if (in != STDIN_FILE_NO)
	{
	  if (dup2 (in, STDIN_FILE_NO) < 0)
	    child_fail ("dup2", executable);
	  close (in);
	}
      if (out != STDOUT_FILE_NO)
	{
	  if (dup2 (out, STDOUT_FILE_NO) < 0)
	    child_fail ("dup2", executable);
	  close (out);
	}
      if (errdes != STDERR_FILE_NO)
	{
	  if (dup2 (errdes, STDERR_FILE_NO) < 0)
	    child_fail ("dup2", executable);
	  close (errdes);
	}
      if ((flags & PEX_STDERR_TO_STDOUT) != 0)
	{
	  if (dup2 (STDOUT_FILE_NO, STDERR_FILE_NO) < 0)
	    child_fail ("dup2", executable);
	}
      if (toclose >= 0)
	close (toclose);

      if (env != NULL)
	environ = (char **) env;

      if ((flags & PEX_SEARCH) != 0)
	execvp (executable, argv);
      else
	execv (executable, argv);
      child_fail ("exec", executable);
    }

  if (in != STDIN_FILE_NO)
    close (in);
  if (out != STDOUT_FILE_NO)
    close (out);
  if (errdes != STDERR_FILE_NO)
    close (errdes);

  return (pid_t) child;
}

static pid_t
pex_salsola_wait (struct pex_obj *obj ATTRIBUTE_UNUSED, pid_t pid,
		  int *status, struct pex_time *time, int done,
		  const char **errmsg, int *err)
{
  int child = (int) pid;
  uint8_t code;
  err_t e;

  if (done)
    process_kill (child);

  if (time != NULL)
    memset (time, 0, sizeof (struct pex_time));

  e = waitod (child, &code, NULL);
  close (child);
  if (e != ERROR_SUCCESS)
    {
      *err = error_to_errno (e);
      *errmsg = "waitod";
      return (pid_t) -1;
    }

  /* Callers decode this with the WIFEXITED/WEXITSTATUS fallbacks in
     libiberty and gcc's system.h, which expect the code above bit 8.  */
  *status = (int) code << 8;
  return pid;
}

static int
pex_salsola_pipe (struct pex_obj *obj ATTRIBUTE_UNUSED, int *p ATTRIBUTE_UNUSED,
		  int binary ATTRIBUTE_UNUSED)
{
  errno = ENOSYS;
  return -1;
}

static FILE *
pex_salsola_fdopenr (struct pex_obj *obj ATTRIBUTE_UNUSED, int fd,
		     int binary ATTRIBUTE_UNUSED)
{
  return fdopen (fd, "r");
}

static FILE *
pex_salsola_fdopenw (struct pex_obj *obj ATTRIBUTE_UNUSED, int fd,
		     int binary ATTRIBUTE_UNUSED)
{
  return fdopen (fd, "w");
}
