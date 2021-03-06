/* #############################################################################
 * code for general security like handling the memory protection
 * #############################################################################
 * Copyright (C) 2005-2009 Harry Brueckner
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contact: Harry Brueckner <harry_b@mm.st>
 *          Muenchener Strasse 12a
 *          85253 Kleinberghofen
 *          Germany
 * #############################################################################
 */

/* #############################################################################
 * includes
 */
#include "cpm.h"
#include <regex.h>
#include "configuration.h"
#include "general.h"
#include "memory.h"
#include "security.h"
#include "string.h"


/* #############################################################################
 * internal functions
 */
int checkEnvValue(int showerror, int type, char* value);
int clearEnvironment(void);


/* #############################################################################
 * global variables
 */
#define STAT_GREEN      "[1;32m"
#define STAT_YELLOW     "[1;33m"
#define STAT_RED        "[1;31m"
#define STAT_OFF        "[0m"

#define STRTYPE_NOCHECK         0
#define STRTYPE_GENERAL         1
#define STRTYPE_ALPHANUMERIC    2
#define STRTYPE_NUMERIC         3
#define STRTYPE_FILENAME        4
#define STRTYPE_PATH            5


/* #############################################################################
 *
 * Description    check a given environ variable for the given type
 * Author         Harry Brueckner
 * Date           2005-05-30
 * Arguments      int type      - expected type of the value
 *                char* value   - string to verify
 * Return         int 0 if ok, 1 on error
 */
int checkEnvValue(int showerror, int type, char* value)
  {
    regex_t             regex;
    int                 result;
    char*               errormsg;
    char*               pattern;

    TRACE(99, "checkEnvValue()", NULL);

    if (!value)
      {
        if (showerror)
          { fprintf(stderr, _("can not check null environment variable.\n")); }
        return 1;
      }

    switch (type)
      {
        case STRTYPE_NOCHECK:
            pattern = "^.*$";
            break;
        case STRTYPE_GENERAL:
            pattern = "^[^=]*$";
            break;
        case STRTYPE_ALPHANUMERIC:
            pattern = "^[[:alnum:][:blank:]@_\\.-]*$";
            break;
        case STRTYPE_NUMERIC:
            pattern = "^[[:digit:]]+$";
            break;
        case STRTYPE_FILENAME:
            pattern = "^[[:alnum:][:blank:]_\\.\\:/-]*$";
            break;
        case STRTYPE_PATH:
            pattern = "^[[:alnum:][:blank:]_\\./:]*$";

            /* for paths we do some other checks as well */
            result = strlen(value);
            if (result > 0)
              {
                if (value[0] == ':' ||
                    value[result - 1] == ':')
                  { return 1; }
              }
            break;
        default:
            return 1;
      }

    result = regcomp(&regex, pattern, REG_EXTENDED | REG_NOSUB);
    if (showerror && result)
      {
        errormsg = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
        regerror(result, &regex, errormsg, STDBUFFERLENGTH);
        fprintf(stderr, "regerror: %s\n", errormsg);
        memFree(__FILE__, __LINE__, errormsg, STDBUFFERLENGTH);

        return 1;
      }

    result = regexec(&regex, value, 0, NULL, 0);

    if (showerror && result)
      {
        errormsg = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
        regerror(result, &regex, errormsg, STDBUFFERLENGTH);
        fprintf(stderr, "regerror: %s\n", errormsg);
        memFree(__FILE__, __LINE__, errormsg, STDBUFFERLENGTH);
      }

    regfree(&regex);

    return result;
  }


/* #############################################################################
 *
 * Description    check and display the currently used protections
 * Author         Harry Brueckner
 * Date           2005-05-30
 * Arguments      int silent  - 1 for no message display; 0 for messages
 * Return         level of security (yes answers)
 */
int checkSecurity(int silent)
  {
    struct rlimit       rl;
    int                 level = 0;
    /* Flawfinder: ignore */
    char                memlimit[24];

    TRACE(99, "checkSecurity()", NULL);

    if (runtime -> memlock_limit == -2)
      { snprintf(memlimit, 24, "%s", "---"); }
    else if (runtime -> memlock_limit == -1)
      { snprintf(memlimit, 24, "%s", _("unlimited")); }
    else
      {
        snprintf(memlimit, 24, "%ld kB", (long)runtime -> memlock_limit / 1024);
      }

    if (!silent)
      { printf("%-50s", _("Running without root privileges:")); }
    if (geteuid() && getegid() && getuid() && getgid())
      {
        if (!silent)
          { printf("%s%s%s\n", STAT_GREEN, _("yes"), STAT_OFF); }
        level++;
      }
    else
      {
        if (!silent)
          { printf("%s%s%s\n", STAT_RED, _("no"), STAT_OFF); }
      }

    if (getrlimit(RLIMIT_CORE, &rl) == -1)
      {
        fprintf(stderr, "%s (%s, %d)\n",
            _("Can not read RLIMIT_CORE resources."),
            strerror(errno),
            errno
            );
        return 1;
      }

    if (!silent)
      { printf("%-50s", _("Memory protection from core dumps:")); }
    if (!rl.rlim_cur && !rl.rlim_max)
      {
        if (!silent)
          { printf("%s%s%s\n", STAT_GREEN, _("yes"), STAT_OFF); }
        level++;
      }
    else
      {
        if (!silent)
          { printf("%s%s%s\n", STAT_RED, _("no"), STAT_OFF); }
      }

#ifndef NO_MEMLOCK
#ifdef HAVE_MLOCKALL
    if (!silent)
      { printf("%-50s", _("Memory protection from swap writings:")); }
    if (runtime -> memory_safe)
      {
        if (!silent)
          { printf("%s%s%s\n", STAT_GREEN, _("yes"), STAT_OFF); }
        level++;
      }
    else
      {
        if (!silent)
          { printf("%s%s%s\n", STAT_RED, _("no"), STAT_OFF); }
      }

#ifdef __linux__
    if (!silent)
      { printf("%-50s", _("Max. memory lock ok:")); }
    if (runtime -> max_mem_lock)
      {
        if (!silent)
          { printf("%s%s%s (%s)\n", STAT_GREEN, _("yes"), STAT_OFF, memlimit); }
        level++;
      }
    else
      {
        if (!silent)
          { printf("%s%s%s (%s)\n", STAT_RED, _("no"), STAT_OFF, memlimit); }
      }
#endif
#endif
#endif

    if (!silent)
      { printf("%-50s", _("Memory protection from ptrace spying:")); }
    if (runtime -> ptrace_safe)
      {
        if (!silent)
          { printf("%s%s%s\n", STAT_GREEN, _("yes"), STAT_OFF); }
        level++;
      }
    else
      {
        if (!silent)
          { printf("%s%s%s\n", STAT_RED, _("no"), STAT_OFF); }
      }

    if (!silent)
      {
        printf("%-50s", _("Validation of environment variables:"));
        printf("%s%s%s\n", STAT_GREEN, _("yes"), STAT_OFF);
      }
    level++;

#if defined(HAVE_CRACKLIB) && defined(CRACKLIB_DICTPATH)
    struct stat buf;

    if (stat(CRACKLIB_DICTPATH ".pwd", &buf) == -1) {
        config->cracklibstatus = CRACKLIB_OFF;
    } else {
        level++;
    }
    
    if (!silent) {
        printf("%-50s", _("Cracklib dictionary (" CRACKLIB_DICTPATH"):"));
      if (config->cracklibstatus == CRACKLIB_ON) {
          printf("%s%s%s\n", STAT_GREEN, _("yes"), STAT_OFF);
      } else {
          printf("%s%s%s\n", STAT_RED, _("no"), STAT_OFF);
      }
    }
#else
    if (!silent) {
        printf("%-50s", _("Cracklib dictionary:"));
        printf("%s%s%s\n", STAT_YELLOW, _("not_"), STAT_OFF);
    }
    level++;
#endif

    return level;
  }


/* #############################################################################
 *
 * Description    clear the program environment and check all variables
 * Author         Harry Brueckner
 * Date           2005-05-30
 * Arguments      void
 * Return         1 on error, 0 if all is ok
 */
#define PutEnv(eptr, ptr, ctype, name, value) \
    if (value) \
      { \
        if (checkEnvValue(0, ctype, value)) \
          { \
            fprintf(stderr, _("illegal environment variable found (%s%s).\n"), name, value); \
            return 1; \
          } \
        ptr = strNewcat(name, value); \
        /* we must use malloc here because we can't free this string again */ \
        eptr = malloc(strlen(ptr) + 1); \
        if (eptr) \
          { \
            /* Flawfinder: ignore */ \
            strcpy(eptr, ptr); \
            if (putenv(eptr)) \
              { \
                fprintf(stderr, "%s (%s, %s, %d)\n", \
                    _("Unable to set the environment"), \
                    eptr, \
                    strerror(errno), \
                    errno \
                    ); \
                memFreeString(__FILE__, __LINE__, ptr); \
                return 1; \
              } \
          } \
        else \
          { \
            memFreeString(__FILE__, __LINE__, ptr); \
            return 1; \
          } \
        memFreeString(__FILE__, __LINE__, ptr); \
      }

int clearEnvironment(void)
  {
    extern char**       environ;
    char*               columns;
    char*               gnupghome;
    char*               gpg_agent_info;
    char*               home;
    char*               lang;
    char*               lines;
    char*               no_utf8_acs;
    char*               term;
    char*               termcap;

    char*               eptr;
    char*               ptr;

    TRACE(99, "clearEnvironment()", NULL);

    /* Flawfinder: ignore */
    columns = getenv("COLUMNS");
    /* Flawfinder: ignore */
    gnupghome = getenv("GNUPGHOME");
    /* Flawfinder: ignore */
    gpg_agent_info = getenv("GPG_AGENT_INFO");
    /* Flawfinder: ignore */
    home = getenv("HOME");
    /* Flawfinder: ignore */
    lang = getenv("LANG");
    /* Flawfinder: ignore */
    lines = getenv("LINES");
    /* Flawfinder: ignore */
    no_utf8_acs = getenv("NCURSES_NO_UTF8_ACS");
    /* Flawfinder: ignore */
    termcap = getenv("TERMCAP");
    /* Flawfinder: ignore */
    term = getenv("TERM");

    /* we clear the environment */
#ifdef HAVE_CLEARENV
    if (clearenv())
      {
        fprintf(stderr, _("clearenv() reported an error.\n"));
        return 1;
      }
#elif HAVE_UNSETENV
    while (environ && environ[0])
      {
        int                 i;
        ptr = environ[0];

        i = 0;
        /* count how many chars the variable name has */
        while (ptr[i] && ptr[i] != '=')
          { i++; }

        if (ptr[i] == '=')
          {   /* we really found a variable */
            eptr = memAlloc(__FILE__, __LINE__, i + 1);
            strStrncpy(eptr, ptr, i + 1);

            /* unset the variable */
            if (unsetenv(eptr))
              {
                fprintf(stderr, "%s (%s, %s, %d)\n",
                    _("Unable to unset the environment"),
                    eptr,
                    strerror(errno),
                    errno
                    );
              }

            memFree(__FILE__, __LINE__, eptr, i + 1);
          }
      }
#endif

    /* general */
    PutEnv(eptr, ptr, STRTYPE_ALPHANUMERIC, "LANG=", lang);
    PutEnv(eptr, ptr, STRTYPE_FILENAME, "HOME=", home);

    /* terminal */
    PutEnv(eptr, ptr, STRTYPE_NUMERIC, "COLUMNS=", columns);
    PutEnv(eptr, ptr, STRTYPE_NUMERIC, "LINES=", lines);
    PutEnv(eptr, ptr, STRTYPE_NUMERIC, "NCURSES_NO_UTF8_ACS=", no_utf8_acs);
    PutEnv(eptr, ptr, STRTYPE_ALPHANUMERIC, "TERM=", term);
    PutEnv(eptr, ptr, STRTYPE_NOCHECK, "TERMCAP=", termcap);

    /* GPG */
    PutEnv(eptr, ptr, STRTYPE_FILENAME, "GNUPGHOME=", gnupghome);
    PutEnv(eptr, ptr, STRTYPE_FILENAME, "GPG_AGENT_INFO=", gpg_agent_info);

    return 0;
  }

#undef PutEnv


/* #############################################################################
 *
 * Description    enable the protection mechanism and drop all privileges
 * Author         Harry Brueckner
 * Date           2005-05-30
 * Arguments      int* max_mem_lock       - the max memory lock flag
 *                int* memory_safe        - the memory status flag
 *                int* ptrace_safe        - the ptrace status flag
 *                rlim_t* memlock_limit   - the current memory limit
 * Return         1 on error, 0 if all is ok
 */
int initSecurity(int* max_mem_lock, int* memory_safe, int* ptrace_safe,
    rlim_t* memlock_limit)
  {
    struct rlimit       rl;
#ifdef __linux__
    rlim_t              rl_old;
#endif
    int                 canary;
#ifndef NO_MEMLOCK
#ifdef HAVE_MLOCKALL
    int                 result;
#endif
#endif

    TRACE(99, "initSecurity()", NULL);

    /* we initialize our flags */
    *memlock_limit = -2;
    *max_mem_lock = 0;
    *memory_safe = 0;
    *ptrace_safe = 0;

    /* drop eventual group root privileges; this must be done twice to
     * counter "saved IDs" see Secure Programming HowTo
     */
    setgid(getgid());
    setgid(getgid());
#ifdef HAVE_SYS_FSUID_H
    setfsgid(getgid());
    setfsgid(getgid());
#endif

  if (!geteuid())
    {   /* the process is (suid) root. mlockall() can be used to
         * avoid swapping
         */
#ifndef NO_MEMLOCK
#ifdef HAVE_MLOCKALL
#ifdef __linux__
      /* check if our max memory lock size is acceptable */
      if (getrlimit(RLIMIT_MEMLOCK, &rl) == -1)
        {
          fprintf(stderr, "%s (%s, %d)\n",
              _("Can not read RLIMIT_MEMLOCK resources."),
              strerror(errno),
              errno
              );
          return 1;
        }

      /* we try to raise the current limit to the maximum, in case it */
      /* is not there yet. */
      /* This patch was provided by Kurt Neufeld */
      /* <kneufeld at burgundywall.com> */
      if(rl.rlim_cur < rl.rlim_max)
        {
          fprintf(stderr, _("Max. memory lock, curr: %d kB, max: %d kB\n"),
              (int)rl.rlim_cur / 1024, (int)rl.rlim_max / 1024);
          fprintf(stderr, _("Attempting to set limit to: %d kB"),
              (int)rl.rlim_max / 1024);

          rl_old = rl.rlim_cur;
          rl.rlim_cur = rl.rlim_max;
          if (setrlimit(RLIMIT_MEMLOCK, &rl) < 0)
            {
              rl.rlim_cur = rl_old;
              fprintf(stderr, _(" %sfailed%s\n"), STAT_RED, STAT_OFF);
              fprintf(stderr, _("Unable to increase the limit to: %d kB"),
                  (int)rl.rlim_max / 1024);
            }
          else
            {
              fprintf(stderr, _(" %ssuccess%s\n"), STAT_GREEN, STAT_OFF);
            }
        }

      *memlock_limit = rl.rlim_cur;
      if (rl.rlim_cur == -1 || rl.rlim_cur >= MEMLOCK_LIMIT * 1024)
        {   /* we only lock memory if the limit is high enough */
          *max_mem_lock = 1;

          /* lock the memory */
          result = mlockall(MCL_CURRENT | MCL_FUTURE);
          if (result)
            {
              fprintf(stderr, "%s\n",
                  _("The process is suid root, but memory paging can't be locked.")
                  );
              return 1;
            }
          else
            { *memory_safe = 1; }
        }
#elif __sun__
      /* we dont have any limit, so locking should always work */
      *max_mem_lock = 1;

      /* lock the memory */
      result = mlockall(MCL_CURRENT | MCL_FUTURE);
      if (result)
        {
          fprintf(stderr, "%s\n",
              _("The process is suid root, but memory paging can't be locked.")
              );
          return 1;
        }
      else
        { *memory_safe = 1; }
#endif
#endif
#endif

      /* drop root privileges */
      setuid(getuid());
      setuid(getuid());
#ifdef HAVE_SYS_FSUID_H
      setfsuid(getuid());
      setfsuid(getuid());
#endif

#ifdef HAVE_SYS_FSUID_H
      if (getuid() && (!setuid(0) || !setfsuid(0)))
#else
      if (getuid() && !setuid(0))
#endif
        {
          fprintf(stderr, "%s (%d)\n", _("Cannot drop root privileges."),
              __LINE__);
          return 1;
        }

      *ptrace_safe = 1;
    }

#ifdef HAVE_SYS_FSUID_H
    if (getuid() && (!setgid(0) || !setfsgid(0)))
#else
    if (getuid() && !setgid(0))
#endif
      {
        fprintf(stderr, "%s (%d)\n", _("Cannot drop root privileges."),
            __LINE__);
        return 1;
      }

    /* set core dump generation */
    rl.rlim_cur = rl.rlim_max = 0;
    setrlimit(RLIMIT_CORE, &rl);

    /* validate core dump generation */
    if (getrlimit(RLIMIT_CORE, &rl) == -1)
        {
          fprintf(stderr, "%s (%s, %d)\n",
              _("Can not read RLIMIT_CORE resources."),
              strerror(errno),
              errno
              );
          return 1;
        }
    if (rl.rlim_cur || rl.rlim_max)
      {
        fprintf(stderr, "%s\n",
            _("Cannot disable core file generation.")
            );
        return 1;
      }

    /* to have a default for the filemask, we call umask - even if it's not
     * used anywhere
     */
    umask(0022);

    /* check that stderr, stdin and stdout are opened */
    /* NOTE: no file must be opened before this test! */
    canary = dup(0);
    close(canary);
    if (canary != 3)
      {
        fprintf(stderr, "%s\n",
            _("stdin, stdout and/or stderr are invalid."));
        return 1;
      }

    return clearEnvironment();
  }


/* #############################################################################
 *
 * Description    display the current environment, displayed after cleanup
 * Author         Harry Brueckner
 * Date           2008-09-28
 * Arguments      void
 * Return         void
 */
void listEnvironment(void)
  {
    int                 i;
    extern char**       environ;

    TRACE(99, "listEnvironment()", NULL);

    printf("Environment:\n");

    if (!environ)
      {
        printf("No environment found.\n");
        return;
      }

    for (i = 0; environ[i] && environ[i]; i++)
      { printf("%s\n", environ[i]); }
  }


/* #############################################################################
 *
 * Description    run several tests against the pattern matcher
 * Author         Harry Brueckner
 * Date           2005-05-30
 * Arguments      void
 * Return         void
 */
#define runTestOk(name, id, type, value) \
    result = checkEnvValue(1, type, value); \
    fprintf(stderr, "%s #%d: %s (%d)\n", \
    name, id, result ? "[1;31merror[0m" : "[1;32mok[0m", result);
#define runTestError(name, id, type, value) \
    result = checkEnvValue(1, type, value); \
    fprintf(stderr, "%s #%d: %s (%d)\n", \
    name, id, result ? "[1;32mok[0m" : "[1;31merror[0m", result);

void testEnvironment(void)
  {
    int                 id,
                        result;

    TRACE(99, "testEnvironment()", NULL);

    /* a general variable might contain everything except the '=' sign */
    id = 0;
    runTestOk("general", ++id, STRTYPE_GENERAL, "test this");
    runTestError("general", ++id, STRTYPE_GENERAL, "test=this");

    /* a variable might containing alphanumeric values */
    id = 0;
    runTestOk("alpha", ++id, STRTYPE_ALPHANUMERIC, "abcABC123_. ");
    runTestOk("alpha", ++id, STRTYPE_ALPHANUMERIC, "abcABC-123_. ");
    runTestError("alpha", ++id, STRTYPE_ALPHANUMERIC, "illegal value <>");

    /* a variable must be an integer */
    id = 0;
    runTestOk("numeric", ++id, STRTYPE_NUMERIC, "1234");
    runTestError("numeric", ++id, STRTYPE_NUMERIC, "abc123def");
    runTestError("numeric", ++id, STRTYPE_NUMERIC, "123def");
    runTestError("numeric", ++id, STRTYPE_NUMERIC, "abc123");
    runTestError("numeric", ++id, STRTYPE_NUMERIC, "123.123");
    runTestError("numeric", ++id, STRTYPE_NUMERIC, "123-123");

    /* a variable must be a filename or directory */
    id = 0;
    runTestOk("filename", ++id, STRTYPE_FILENAME, "/home/harry");
    runTestOk("filename", ++id, STRTYPE_FILENAME, "/home/harry/12345.jpg");
    runTestOk("filename", ++id, STRTYPE_FILENAME, "/home/harry/im-age.jpg");
    runTestOk("filename", ++id, STRTYPE_FILENAME, "/home/harry/image.jpg");
    runTestOk("filename", ++id, STRTYPE_FILENAME, "/home/harry/IMAGE.jpg");
    runTestOk("filename", ++id, STRTYPE_FILENAME, "/home/harry/another image.jpg");
    runTestOk("filename", ++id, STRTYPE_FILENAME, "/home/harry/../harry/");
    runTestOk("filename", ++id, STRTYPE_FILENAME, "/tmp/gpg-abcdef/S.gpg-agent:12345:1");

    /* a variable must be a path */
    id = 0;
    runTestOk("path", ++id, STRTYPE_PATH, "/dir1");
    runTestOk("path", ++id, STRTYPE_PATH, "/dir1:/dir2:/dir3");
    runTestOk("path", ++id, STRTYPE_PATH, "/dir1:/dumb dir");
    runTestError("path", ++id, STRTYPE_PATH, ":/dir1");
    runTestError("path", ++id, STRTYPE_PATH, "/dir1:");
    runTestError("path", ++id, STRTYPE_PATH, ":/dir1:");
  }


/* #############################################################################
 */

