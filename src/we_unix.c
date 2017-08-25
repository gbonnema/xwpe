/** \file we_unix.c                                        */
/* Copyright (C) 1993 Fred Kruse                          */
/* This is free software; you can redistribute it and/or  */
/* modify it under the terms of the                       */
/* GNU General Public License, see the file COPYING.      */

#include "config.h"
#include <string.h>
#include "keys.h"
#include "model.h"		/* exchange for D.S.  */
#include "we_control.h"
#include "options.h"
#include "we_term.h"
#include <signal.h>

#ifdef UNIX
#include <unistd.h>
#endif

// \todo TODO: checkout when and if we need XWPE_DLL
#ifdef XWPE_DLL
#include <dlfcn.h>
#else
int WpeXtermInit (int *argc, char **argv);
int WpeTermInit (int *argc, char **argv);
#endif

#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <signal.h>
#include <locale.h>
#if defined HAVE_LIBNCURSES || defined HAVE_LIBCURSES
#include<curses.h>
#endif

#include "edit.h"		/*   exchange for D.S.  */
#include "we_unix.h"
#include "attrb.h"
#include "we_progn.h"

#ifndef HAVE_SYMLINK
#define lstat(x,y)  stat(x,y)
#undef S_ISLNK
#define S_ISLNK(x)  0
#endif

char e_we_sw = 0;

void WpeSignalUnknown (int sig);
void WpeSignalChild (int sig);

void (*WpeMouseChangeShape) (WpeMouseShape new_shape);
void (*WpeMouseRestoreShape) (void);
void (*WpeDisplayEnd) (void);
int (*fk_u_locate) (int x, int y);
int (*fk_u_cursor) (int x);
int (*fk_u_putchar) (int c);
int (*u_bioskey) (void);
int (*e_frb_u_menue) (int sw, int xa, int ya, we_window_t * window, int md);
we_color_t (*e_s_u_clr) (int fg_color, int bg_color);
we_color_t (*e_n_u_clr) (int fg_bg_color);
void (*e_pr_u_col_kasten) (int xa, int ya, int x, int y, we_window_t * window, int sw);
int (*fk_mouse) (int g[]);
/**
 * refresh the screen.
 *
 * filled with e_t_refresh for non-X terminal or e_x_refresh for X terminal.
 */
int (*e_u_refresh) (void);
/**
 * get character.
 *
 * filled with e_t_getch for non-X terminal or e_x_getch for X terminal.
 */
int (*e_u_getch) (void);
/**
 * Initialize terminal.
 *
 * filled with e_t_sys_ini for non-X terminal or e_x_sys_ini (= zero function) for X terminal.
 */
int (*e_u_sys_ini) (void);
/**
 * Ending
 *
 * filled with e_t_sys_end for non-X terminal or e_x_sys_end (= zero function) for X terminal.
 */
int (*e_u_sys_end) (void);
/**
 * Function to execute a system command.
 *
 * filled with system for non-X terminal or e_x_system for X terminal.
 * Remark that e_x_system is wrapped around system and includes some specific X features
 * like -geometry.
 */
int (*e_u_system) (const char *exe);
/**
 * This function does some sort of screen switch: \todo: what does it do exactly?
 *
 * filled with e_t_d_switch_out for non-X terminal or WpeZeroFunction for X terminal.
 */
int (*e_u_d_switch_out) (int sw);
/**
 * This function does some sort of screen switch: \todo: what does it exactly do?
 *
 * filled with e_t_switch_screen for non-X terminal or WpeZeroFunction for X terminal.
 */
int (*e_u_switch_screen) (int sw);
/**
 * debug output function
 *
 * filled with e_t_deb_out for non-X terminal.
 */
int (*e_u_deb_out) (we_window_t * window);
/**
 * Copies
 */
int (*e_u_cp_X_to_buffer) (we_window_t * window);
int (*e_u_copy_X_buffer) (we_window_t * window);
int (*e_u_paste_X_buffer) (we_window_t * window);
int (*e_u_kbhit) (void);
int (*e_u_change) (we_view_t * view);
int (*e_get_pic_urect) (int xa, int ya, int xe, int ye,
                        struct view_struct * view);
int (*e_u_s_sys_end) (void);
int (*e_u_s_sys_ini) (void);
void (*e_u_setlastpic) (we_view_t * view);

we_colorset_t *u_fb, *x_fb;

char MCI, MCA, RD1, RD2, RD3, RD4, RD5, RD6, WBT;
char RE1, RE2, RE3, RE4, RE5, RE6;
int e_mn_men = 3;

struct termios otermio, ntermio, ttermio;
void *libxwpe;

we_view_t *e_X_l_pic = NULL;

void
WpeNullFunction (void)
{
}

int
WpeZeroFunction (void)
{
    return (0);
}

int
e_ini_unix (int *argc, char **argv)
{
    extern OPT opt[];
    int i, debug;
    struct sigaction act;
#ifdef XWPE_DLL
    int initfunc (int *argc, char **argv);
#endif

    setlocale (LC_ALL, "");
    u_fb = NULL;
    x_fb = NULL;
    debug = 0;
    for (i = 1; i < *argc; i++)
    {
        if (strcmp ("--debug", argv[i]) == 0)
        {
            debug = 1;
        }
    }
    for (i = strlen (argv[0]) - 1; i >= 0 && *(argv[0] + i) != DIRC; i--)
        ;
#ifndef NO_XWINDOWS
    if (*(argv[0] + i + 1) == 'x')
        e_we_sw = 1;
    else
        e_we_sw = 0;
#endif
#ifdef PROG
    if (!strncmp ("wpe", (argv[0] + i + e_we_sw + 1), 3))
        e_we_sw |= 2;
#endif
// \todo TODO: Checkout whether we need XWPE_DLL
// \todo FIXME: adjusted code to read only libxwpe (common library), needs testing
#ifdef XWPE_DLL
    libxwpe = dlopen (LIBRARY_DIR "/libxwpe.so", RTLD_NOW);
    if (!libxwpe)
    {
        printf ("%s\n", dlerror ());
        exit (0);
    }
    initfunc = dlsym (libxwpe, "WpeDllInit");
    if (initfunc)
    {
        initfunc (argc, argv);
    }
    else
    {
        printf ("%s\n", dlerror ());
        exit (0);
    }
#else
#ifndef NO_XWINDOWS
    if (WpeIsXwin ())
    {
        WpeXtermInit (argc, argv);
    }
    else
#endif
    {
        WpeTermInit (argc, argv);
    }
#endif
    if (WpeIsProg ())
    {
#ifndef DEBUGGER
        opt[0].x = 2, opt[1].x = 7, opt[2].x = 14;
        opt[3].x = 21;
        opt[4].x = 30;
        opt[9].t = opt[7].t;
        opt[9].x = 74;
        opt[9].s = opt[7].s;
        opt[9].as = opt[7].as;
        opt[8].t = opt[6].t;
        opt[8].x = 65;
        opt[8].s = opt[6].s;
        opt[8].as = opt[6].as;
        opt[7].t = opt[5].t;
        opt[7].x = 55;
        opt[7].s = opt[5].s;
        opt[7].as = opt[5].as;
        opt[6].t = "Project";
        opt[6].x = 45;
        opt[6].s = 'P';
        opt[6].as = AltP;
        opt[5].t = "Run";
        opt[5].x = 38;
        opt[5].s = 'R';
        opt[5].as = AltR;
        MENOPT = 10;
        e_mn_men = 2;
#else
        opt[0].x = 2, opt[1].x = 6, opt[2].x = 12;
        opt[3].x = 18;
        opt[4].x = 26;
        opt[10].t = opt[7].t;
        opt[10].x = 74;
        opt[10].s = opt[7].s;
        opt[10].as = opt[7].as;
        opt[9].t = opt[6].t;
        opt[9].x = 65;
        opt[9].s = opt[6].s;
        opt[9].as = opt[6].as;
        opt[8].t = opt[5].t;
        opt[8].x = 56;
        opt[8].s = opt[5].s;
        opt[8].as = opt[5].as;
        opt[7].t = "Project";
        opt[7].x = 47;
        opt[7].s = 'P';
        opt[7].as = AltP;
        opt[6].t = "Debug";
        opt[6].x = 40;
        opt[6].s = 'D';
        opt[6].as = AltD;
        opt[5].t = "Run";
        opt[5].x = 34;
        opt[5].s = 'R';
        opt[5].as = AltR;
        MENOPT = 11;
        e_mn_men = 1;
#endif
    }

    /* Unknown error signal handling */
    act.sa_handler = WpeSignalUnknown;
    sigfillset (&act.sa_mask);	/* Mask all signals while running */
    act.sa_flags = 0;
    if (!debug)
    {
        sigaction (SIGQUIT, &act, NULL);
        sigaction (SIGILL, &act, NULL);
        sigaction (SIGABRT, &act, NULL);
        sigaction (SIGFPE, &act, NULL);
        sigaction (SIGSEGV, &act, NULL);
#ifdef SIGTRAP
        sigaction (SIGTRAP, &act, NULL);
#endif
#ifdef SIGIOT
        sigaction (SIGIOT, &act, NULL);
#endif
#ifdef SIGBUS
        sigaction (SIGBUS, &act, NULL);
#endif
    }
    /* Ignore SIGINT */
    act.sa_handler = SIG_IGN;
    sigaction (SIGINT, &act, NULL);
    /* Catch SIGCHLD */
    act.sa_handler = WpeSignalChild;
    act.sa_flags = SA_NOCLDSTOP;
    sigaction (SIGCHLD, &act, NULL);
    return (*argc);
}

void
e_refresh_area (int x, int y, int width, int height)
{
    char *curloc;
    int i, j;

    if (width + x > MAXSCOL)
    {
        width = MAXSCOL - x;
    }
    if (height + y > MAXSLNS)
    {
        height = MAXSLNS - y;
    }
    curloc = global_alt_screen + ((x + (y * MAXSCOL)) * 2);
    for (j = 0; j < height; j++, curloc += MAXSCOL * 2)
    {
        for (i = 0; i < width; i++)
        {
            curloc[i * 2] = 0;
            curloc[i * 2 + 1] = 0;
        }
    }
}

int
e_tast_sim (int c)
{
    if (c >= 'A' && c <= 'Z')
        return (c + 1024 - 'A');
    switch (c)
    {
    case 'a':
        return (AltA);
    case 'b':
        return (AltB);
    case 'c':
        return (AltC);
    case 'd':
        return (AltD);
    case 'e':
        return (AltE);
    case 'f':
        return (AltF);
    case 'g':
        return (AltG);
    case 'h':
        return (AltH);
    case 'i':
        return (AltI);
    case 'j':
        return (AltJ);
    case 'k':
        return (AltK);
    case 'l':
        return (AltL);
    case 'm':
        return (AltM);
    case 'n':
        return (AltN);
    case 'o':
        return (AltO);
    case 'p':
        return (AltP);
    case 'q':
        return (AltQ);
    case 'r':
        return (AltR);
    case 's':
        return (AltS);
    case 't':
        return (AltT);
    case 'u':
        return (AltU);
    case 'v':
        return (AltV);
    case 'w':
        return (AltW);
    case 'x':
        return (AltX);
    case 'y':
        return (AltY);
    case 'z':
        return (AltZ);
    case '1':
        return (Alt1);
    case '2':
        return (Alt2);
    case '3':
        return (Alt3);
    case '4':
        return (Alt4);
    case '5':
        return (Alt5);
    case '6':
        return (Alt6);
    case '7':
        return (Alt7);
    case '8':
        return (Alt8);
    case '9':
        return (Alt9);
    case '0':
        return (Alt0);
    case ' ':
        return (AltBl);
    case '#':
        return (AltSYS);
    case CtrlA:
        return (CBUP);
    case CtrlE:
        return (CBDO);
    case CtrlB:
        return (CCLE);
    case CtrlF:
        return (CCRI);
    case CtrlP:
        return (CPS1);
    case CtrlN:
        return (CEND);
    case CtrlH:
        return (AltBS);
    default:
        return (0);
    }
}

void
WpeSignalUnknown (int sig)
{
    /*    psignal(sig, "Xwpe");   */
    printf ("Xwpe: unexpected signal %d, exiting ...\n", sig);
    e_exit (1);
}

void
WpeSignalChild (int sig)
{
    UNUSED (sig);
    int statloc;

    wait (&statloc);
}

static int e_bool_exit = 0;

void
e_err_save ()
{
    we_control_t *control = global_editor_control;
    int i;
    unsigned long maxname;
    we_window_t *window;
    we_buffer_t *buffer;

    /* Quick fix to multiple emergency save problems */
    if (e_bool_exit)
        return;
    e_bool_exit = 1;
    for (i = 0; i <= control->mxedt; i++)
    {
        if (DTMD_ISTEXT (control->window[i]->dtmd))
        {
            window = control->window[i];
            buffer = control->window[i]->buffer;
            if (buffer->mxlines > 1 || buffer->buflines[0].len > 0)
            {
                /* Check if file system could have an autosave or emergency save file
                   >12 check is to eliminate dos file systems */
                if (((maxname =
                            pathconf (window->dirct,
                                      _PC_NAME_MAX)) >= strlen (window->datnam) + 4)
                        && (maxname > 12))
                {
                    strcat (window->datnam, ".ESV");
                    printf ("Try to save %s!\n", window->datnam);
                    if (!e_save (window))
                        printf ("File %s saved!\n", window->datnam);
                }
            }
        }
    }
}

void
e_exit (int n)
{
#ifdef DEBUGGER
    extern int e_d_pid;

    if (e_d_pid)
        kill (e_d_pid, 7);
#endif
    WpeDisplayEnd ();
    e_u_switch_screen (0);
    if (n != 0)
    {
        printf ("\nError-Exit!   Code: %d!\n", n);
        e_err_save ();
    }
    exit (n);
}

char *
e_mkfilepath (char *dr, char *fn, char *fl)
{
    strcpy (fl, dr);
    if (dr[strlen (dr) - 1] != DIRC)
    {
        strcat (fl, DIRS);
    }
    strcat (fl, fn);
    return (fl);
}

int
e_compstr (char *a, char *b)
{
    int n, k;
    char *ctmp, *cp;

    if (a[0] == '*' && !a[1])
        return (0);
    if (!a[0] || !b[0])
        return (a[0] - b[0]);
    if (a[0] == '*' && a[1] == '*')
        return (e_compstr (++a, b));
    for (n = a[0] == '*' ? 2 : 1;
            a[n] != '*' && a[n] != '?' && a[n] != '[' && a[n]; n++)
        ;
    if (a[0] == '*')
    {
        n--;
        a++;
        if (a[0] == '?')
        {
            cp = malloc ((strlen (a) + 1) * sizeof (char));
            strcpy (cp, a);
            cp[0] = '*';
            n = e_compstr (cp, ++b);
            free (cp);
            return (n);
        }
        else if (a[0] == '[')
        {
            while (*b && (n = e_compstr (a, b)))
                b++;
            return (n);
        }
        ctmp = malloc (n + 1);
        for (k = 0; k < n; k++)
            ctmp[k] = a[k];
        ctmp[n] = '\0';
        cp = strstr (b, ctmp);
        free (ctmp);
        if (cp == NULL)
            return ((a[0] - b[0]) ? a[0] - b[0] : -1);
        if (!a[n] && !cp[n])
            return (0);
        if (!a[n])
            return (e_compstr (a - 1, cp + 1));
        if (!(k = e_compstr (a + n, cp + n)))
            return (0);
        return (e_compstr (a - 1, cp + 1));
    }
    else if (a[0] == '?')
    {
        n--;
        a++;
        b++;
    }
    else if (a[0] == '[')
    {
        if (a[1] == '!')
        {
            for (k = 2; a[k] && (a[k] != ']' || k == 2) && a[k] != b[0]; k++)
                if (a[k + 1] == '-' && b[0] >= a[k] && b[0] <= a[k + 2])
                    return (-b[0]);
            if (a[k] != ']')
                return (-b[0]);
            n -= (k + 1);
            a += (k + 1);
            b++;
        }
        else
        {
            for (k = 1; a[k] && (a[k] != ']' || k == 1) && a[k] != b[0]; k++)
                if (a[k + 1] == '-' && b[0] >= a[k] && b[0] <= a[k + 2])
                    break;
            if (a[k] == ']' || a[k] == '\0')
                return (-b[0]);
            for (; a[k] && (a[k] != ']'); k++)
                ;
            n -= (k + 1);
            a += (k + 1);
            b++;
        }
    }
    if (n <= 0)
        return (e_compstr (a, b));
    if ((k = strncmp (a, b, n)) != 0)
        return (k);
    return (e_compstr (a + n, b + n));
}

struct dirfile *
e_find_files (char *sufile, int sw)
{
    char *stmp, *tmpst, *sfile, *sdir;
    struct dirfile *df = malloc (sizeof (struct dirfile));
    DIR *dirp;
    struct dirent *dp;
    struct stat buf;
    struct stat lbuf;
    int i, n, cexist, sizeSdir;
    unsigned int sizeStmp;

    df->name = NULL;
    df->nr_files = 0;
    for (n = strlen (sufile); n >= 0 && sufile[n] != DIRC; n--);
    sfile = sufile + 1 + n;
    if (n <= 0)
    {
        sizeSdir = 2;
        sdir = (char *) malloc (2 * sizeof (char));
        sdir[0] = n ? '.' : DIRC;
        sdir[1] = '\0';
    }
    else
    {
        sizeSdir = n + 1;
        sdir = (char *) malloc ((n + 1) * sizeof (char));
        for (i = 0; i < n; i++)
            sdir[i] = sufile[i];
        sdir[n] = '\0';
    }
    if (!(dirp = opendir (sdir)))
    {
        free (sdir);
        return (df);
    }
    sizeStmp = 256;
    stmp = (char *) malloc (sizeStmp);
    while ((dp = readdir (dirp)) != NULL)
    {
        if (!(sw & 1) && dp->d_name[0] == '.' && sfile[0] != '.')
            continue;
        if (!e_compstr (sfile, dp->d_name))
        {
            if (sizeSdir + strlen (dp->d_name) + 10 > sizeStmp)
            {
                while (sizeSdir + strlen (dp->d_name) + 10 > sizeStmp)
                    sizeStmp <<= 1;
                stmp = (char *) realloc (stmp, sizeStmp);
            }

            e_mkfilepath (sdir, dp->d_name, stmp);
            lstat (stmp, &lbuf);
            stat (stmp, &buf);

            /* check existence of the file */
            cexist = access (stmp, F_OK);

            /* we accept it as a file if
               - a regular file
               - link and it does not point to anything
               - link and it points to a non-directory */
            if ((S_ISREG (buf.st_mode) ||
                    (S_ISLNK (lbuf.st_mode) &&
                     (cexist || (cexist == 0 && !S_ISDIR (buf.st_mode))))) &&
                    (!(sw & 2) || (buf.st_mode & 0111)))
            {
                if (df->nr_files == 0)
                    df->name = malloc ((df->nr_files + 1) * sizeof (char *));
                else
                    df->name =
                        realloc (df->name, (df->nr_files + 1) * sizeof (char *));
                if (df->name == NULL
                        || !(tmpst = malloc (strlen (dp->d_name) + 1)))
                {
                    df->nr_files = 0;
                    closedir (dirp);
                    free (stmp);
                    free (sdir);
                    return (df);
                }
                strcpy (tmpst, dp->d_name);
                for (n = df->nr_files;
                        n > 0 && strcmp (*(df->name + n - 1), tmpst) > 0; n--)
                    *(df->name + n) = *(df->name + n - 1);
                *(df->name + n) = tmpst;
                (df->nr_files)++;
            }
        }
    }
    closedir (dirp);
    free (stmp);
    free (sdir);
    return (df);
}

struct dirfile *
e_find_dir (char *sufile, int sw)
{
    char *stmp, *tmpst, *sfile, *sdir;
    struct dirfile *df = malloc (sizeof (struct dirfile));
    DIR *dirp;
    struct dirent *dp;
    struct stat buf;
    int i, n, sizeSdir;
    unsigned int sizeStmp;

    df->name = NULL;
    df->nr_files = 0;
    for (n = strlen (sufile); n >= 0 && sufile[n] != DIRC; n--);
    sfile = sufile + 1;
    sfile = sfile + n;
    if (n <= 0)
    {
        sizeSdir = 2;
        sdir = malloc (2 * sizeof (char));
        sdir[0] = n ? '.' : DIRC;
        sdir[1] = '\0';
    }
    else
    {
        sizeSdir = n + 1;
        sdir = malloc ((n + 1) * sizeof (char));
        for (i = 0; i < n; i++)
            sdir[i] = sufile[i];
        sdir[n] = '\0';
    }
    if (!(dirp = opendir (sdir)))
    {
        free (sdir);
        return (df);
    }
    sizeStmp = 256;
    stmp = (char *) malloc (sizeStmp);
    while ((dp = readdir (dirp)) != NULL)
    {
        if (!sw && dp->d_name[0] == '.' && sfile[0] != '.')
            continue;
        if (!e_compstr (sfile, dp->d_name) && strcmp (dp->d_name, ".") &&
                strcmp (dp->d_name, ".."))
        {
            if (sizeSdir + strlen (dp->d_name) + 10 > sizeStmp)
            {
                while (sizeSdir + strlen (dp->d_name) + 10 > sizeStmp)
                    sizeStmp <<= 1;
                stmp = (char *) realloc (stmp, sizeStmp);
            }
            stat (e_mkfilepath (sdir, dp->d_name, stmp), &buf);

            /* we accept _only_ real, existing directories */
            if (S_ISDIR (buf.st_mode))
            {

                if (df->nr_files == 0)
                    df->name = malloc ((df->nr_files + 1) * sizeof (char *));
                else
                    df->name =
                        realloc (df->name, (df->nr_files + 1) * sizeof (char *));
                if (df->name == NULL
                        || !(tmpst = malloc (strlen (dp->d_name) + 1)))
                {
                    df->nr_files = 0;
                    closedir (dirp);
                    free (sdir);
                    free (stmp);
                    return (df);
                }
                strcpy (tmpst, dp->d_name);
                for (n = df->nr_files;
                        n > 0 && strcmp (*(df->name + n - 1), tmpst) > 0; n--)
                    *(df->name + n) = *(df->name + n - 1);
                *(df->name + n) = tmpst;
                (df->nr_files)++;
            }
        }
    }
    closedir (dirp);
    free (sdir);
    free (stmp);
    return (df);
}

#include <time.h>

char *
e_file_info (char *filen, char *str, int *num, int sw)
{
    struct tm *ttm;
    struct stat buf[1];

    stat (filen, buf);
    ttm = localtime (&(buf->st_mtime));
    sprintf (str,
             "%c%c%c%c%c%c%c%c%c%c  %-13s  %6ld  %2.2u.%2.2u.%4.4u  %2.2u.%2.2u",
             buf->st_mode & 040000 ? 'd' : '-', buf->st_mode & 0400 ? 'r' : '-',
             buf->st_mode & 0200 ? 'w' : '-', buf->st_mode & 0100 ? 'x' : '-',
             buf->st_mode & 040 ? 'r' : '-', buf->st_mode & 020 ? 'w' : '-',
             buf->st_mode & 010 ? 'x' : '-', buf->st_mode & 04 ? 'r' : '-',
             buf->st_mode & 02 ? 'w' : '-', buf->st_mode & 01 ? 'x' : '-',
             filen, buf->st_size, ttm->tm_mday, ttm->tm_mon + 1,
             ttm->tm_year + 1900, ttm->tm_hour, ttm->tm_min);
    if (sw & 1)
        *num = buf->st_mtime;
    else if (sw & 2)
        *num = buf->st_size;
    return (str);
}

void
ini_repaint (we_control_t * control)
{
    e_cls (control->colorset->df.fg_bg_color, control->colorset->dc);
    e_ini_desk (control);
}

void
end_repaint ()
{
    e_u_refresh ();
}

int
e_recover (we_control_t * control)
{
    struct dirfile *files;
    we_window_t *window = NULL;
    we_buffer_t *buffer;
    we_screen_t *s;
    int i;

    files = e_find_files ("*.ESV", 1);
    for (i = 0; i < files->nr_files; i++)
    {
        e_edit (control, files->name[i]);
        window = control->window[control->mxedt];
        window->datnam[strlen (window->datnam) - 4] = '\0';
        if (!strcmp (window->datnam, BUFFER_NAME))
        {
            s = control->window[control->mxedt]->screen;
            buffer = control->window[control->mxedt]->buffer;
            s->mark_end.y = buffer->mxlines - 1;
            s->mark_end.x = buffer->buflines[buffer->mxlines - 1].len;
            e_edt_copy (window);
            e_close_window (window);
        }
        else
            window->save = 1;
#ifdef PROG
        if (WpeIsProg ())
            e_add_synt_tl (window->datnam, window);
#endif
        if ((window->edit_control->edopt & ED_ALWAYS_AUTO_INDENT) ||
                ((window->edit_control->edopt & ED_SOURCE_AUTO_INDENT) && window->c_st))
            window->flg = 1;
    }
    freedf (files);
    return (0);
}

int
e_frb_t_menue (int sw, int xa, int ya, we_window_t * window, int md)
{
    we_color_t *frb = &(window->colorset->er);
    int i, j, y, c = 1, fb, fsv;

    if (md == 1)
        sw += 11;
    else if (md == 2)
        sw += 16;
    else if (md == 3)
        sw += 32;
    fsv = fb = frb[sw].fg_bg_color;
    if (fb == 0)
        y = 0;
    else
        for (y = 1, j = fb; j > 1; y++)
            j /= 2;
    do
    {
        if (c == CDO)
            y = y < 6 ? y + 1 : 0;
        else if (c == CUP)
            y = y > 0 ? y - 1 : 6;
        if (y == 0)
            fb = 0;
        else
            for (i = 1, fb = 1; i < y; i++)
                fb *= 2;
        frb[sw] = e_n_u_clr (fb);
        e_pr_t_col_kasten (xa, ya, fb, fb, window, 1);
        e_pr_ed_beispiel (1, 2, window, sw, md);
#if  MOUSE
        if ((c = e_u_getch ()) == -1)
            c = e_opt_ck_mouse (xa, ya, md);
#else
        c = e_u_getch ();
#endif
    }
    while (c != WPE_ESC && c != WPE_CR && c > -2);
    if (c == WPE_ESC || c < -1)
        frb[sw] = e_n_u_clr (fsv);
    return (frb[sw].fg_bg_color);
}

/*   draw colors box  */
void
e_pr_t_col_kasten (int xa, int ya, int x, int y, we_window_t * window, int sw)
{
    int rfrb, xe = xa + 14, ye = ya + 8;

    if (x == 0)
        y = 0;
    else
        for (rfrb = x, y = 1; rfrb > 1; y++)
            rfrb /= 2;
    rfrb = sw == 0 ? window->colorset->nt.fg_bg_color : window->colorset->fs.fg_bg_color;
    e_std_window (xa, ya, xe, ye, "Colors", 0, rfrb, 0);
    /*     e_pr_str((xa+xe-8)/2, ya, "Colors", rfrb, 0, 1,
                                            window->colorset->ms.fg_color+16*(rfrb/16), 0);
    */
    e_pr_nstr (xa + 2, ya + 1, xe - xa - 1, "A_NORMAL   ", 0, 0);
    e_pr_nstr (xa + 2, ya + 2, xe - xa - 1, "A_STANDOUT ", A_STANDOUT,
               A_STANDOUT);
    e_pr_nstr (xa + 2, ya + 3, xe - xa - 1, "A_UNDERLINE", A_UNDERLINE,
               A_UNDERLINE);
    e_pr_nstr (xa + 2, ya + 4, xe - xa - 1, "A_REVERSE  ", A_REVERSE,
               A_REVERSE);
    e_pr_nstr (xa + 2, ya + 5, xe - xa - 1, "A_BLINK    ", A_BLINK, A_BLINK);
    e_pr_nstr (xa + 2, ya + 6, xe - xa - 1, "A_DIM      ", A_DIM, A_DIM);
    e_pr_nstr (xa + 2, ya + 7, xe - xa - 1, "A_BOLD     ", A_BOLD, A_BOLD);

    fk_u_locate (xa + 4, ya + y + 1);
}
