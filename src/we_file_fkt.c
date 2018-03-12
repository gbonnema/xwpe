/** \file we_file_fkt.c                                      */
/* Copyright (C) 1993 Fred Kruse                          */
/* This is free software; you can redistribute it and/or  */
/* modify it under the terms of the                       */
/* GNU General Public License, see the file COPYING.      */

#include "config.h"
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include "keys.h"
#include "messages.h"
#include "model.h"
#include "edit.h"
#include "we_edit.h"
#include "WeString.h"
#include "we_file_fkt.h"
#include "utils.h"
#ifdef UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef HAVE_LIBZ
#include <zlib.h>
#endif
#ifdef DEFPGC
#define putc(c, fp) fputc(c, fp)
#endif

int e_read_help ();
int e_read_info ();

char *info_file = NULL;
char *e_tmp_dir = NULL;

/**
 * \brief create complete file name.
 *
 * \param dr char pointer to the directory (validate?).
 * \param fn char pointer to the filename.
 *
 * \returns char pointer to the complete filename, including path.
 *
 *
 * */
char *
e_mkfilename (char *dr, char *fn)
{
    char *fl;

    if (!fn) {
        return (NULL);
    }
    if (!dr) {
        fl = malloc (strlen (fn) + 1);
        strcpy (fl, fn);
        return (fl);
    }
    fl = malloc (strlen (dr) + strlen (fn) + 2);
    strcpy (fl, dr);
    if (dr[0] != '\0' && dr[strlen (dr) - 1] != DIRC) {
        strcat (fl, DIRS);
    }
    strcat (fl, fn);
    return (fl);
}

/**
 *	\brief read file routine.
 *
 *	\param i int
 *	\param j int
 *	\param fp FILE pointer
 *	\param buffer we_buffer_t pointer
 *	\param type char pointer
 *
 *	\returns we_point_t struct containing
 *
 **/
we_point_t
e_readin (int i, int j, FILE * fp, we_buffer_t * buffer, char *type)
{
    we_point_t pkt;
    int ii, k, n = 1, hb = 0;
    signed char cc, c = 0;

    WpeMouseChangeShape (WpeWorkingShape);
    if (i == 0) {
        e_new_line (j, buffer);
    }
    while (n == 1) {
        for (; i < buffer->mx.x; i++) {
            if (hb != 2) {
                n = fread (&c, sizeof (char), 1, fp);
                if (n == 0 && ferror(fp)) {
                    char *msg = "Error during read of stream.";
                    e_error(msg, SERIOUS_ERROR_MSG, buffer->colorset); // does not return
                }
            } else {
                c = cc;
                hb = 0;
            }
            if (n == 1 && c == WPE_CR) {
                if (DTMD_NORMAL == *type) {
                    *type = DTMD_MSDOS;
                }
                i--;
                continue;
            }
            if ((DTMD_HELP == *type) && (c == CtrlH)) {
                if (!hb) {
                    *(buffer->buflines[j].s + i - 1) = HBB;
                    hb = 1;
                } else {
                    i--;
                }
                n = fread (&c, sizeof (char), 1, fp);
                if (n == 0 && ferror(fp)) {
                    char *msg = "Error during read of stream.";
                    e_error(msg, SERIOUS_ERROR_MSG, buffer->colorset); // does not return
                }
            } else if ((DTMD_HELP == *type) && (hb == 1)) {
                n = fread (&cc, sizeof (char), 1, fp);
                if (n == 0 && ferror(fp)) {
                    char *msg = "Error during read of stream.";
                    e_error(msg, SERIOUS_ERROR_MSG, buffer->colorset); // does not return
                }
                if (cc != HBS) {
                    *(buffer->buflines[j].s + i) = HED;
                    i++;
                    hb = 2;
                } else {
                    n = fread (&c, sizeof (char), 1, fp);
                    if (n == 0 && ferror(fp)) {
                        char *msg = "Error during read of stream.";
                        e_error(msg, SERIOUS_ERROR_MSG, buffer->colorset); // does not return
                    }
                }
            }
            if (n != 1) {
                *(buffer->buflines[j].s + i) = '\0';
                break;
            }
            *(buffer->buflines[j].s + i) = c;
            if (c == WPE_WR) {
                break;
            }
        }
        if (n != 1) {
            if (i == 0 && j > 0) {
                free (buffer->buflines[j].s);
                for (k = j; k < buffer->mxlines; k++) {
                    buffer->buflines[k] = buffer->buflines[k + 1];
                }
                (buffer->mxlines)--;
            } else {
                *(buffer->buflines[j].s + i) = WPE_WR;
                *(buffer->buflines[j].s + i + 1) = '\0';
                j++;
            }
        } else if (i >= buffer->mx.x) {
            for (ii = i - 1;
                    *(buffer->buflines[j].s + ii) != ' ' && *(buffer->buflines[j].s + ii) != '-'
                    && ii > 0; ii--);
            if (ii <= 1) {
                ii = i - 1;
            }
            e_new_line (j + 1, buffer);
            for (k = ii + 1; k <= i; k++) {
                if (*(buffer->buflines[j].s + k) != '\0') {
                    *(buffer->buflines[j + 1].s + k - ii - 1) = *(buffer->buflines[j].s + k);
                } else {
                    *(buffer->buflines[j + 1].s + k - ii - 1) = ' ';
                }
            }
            i = i - ii - 1;
            *(buffer->buflines[j].s + ii + 1) = '\0';
            j++;
            if (i < 0) {
                i = 0;
            }
        } else if (c == WPE_WR) {
            *(buffer->buflines[j].s + i) = WPE_WR;
            *(buffer->buflines[j].s + i + 1) = '\0';
            e_new_line (++j, buffer);
            i = 0;
        } else {
            *(buffer->buflines[j].s + i) = ' ';
            i++;
            *(buffer->buflines[j].s + i) = '\0';
        }
        buffer->buflines[j - 1].len = e_str_len (buffer->buflines[j - 1].s);
        buffer->buflines[j - 1].nrc = strlen ((const char *) buffer->buflines[j - 1].s);
    }
    pkt.x = i;
    pkt.y = j;
    WpeMouseRestoreShape ();
    return (pkt);
}

/*   Open new edit window   */
int
e_new (we_window_t * window)
{
    e_edit (window->edit_control, "");
    return (0);
}

int
e_m_save (we_window_t * window)
{
    int ret = e_save (window);

    if (ret == WPE_ESC) {
        e_message (0, e_msg[ERR_MSG_SAVE], window);
    }
    return (ret);
}

int
e_save (we_window_t * window)
{
    we_buffer_t *buffer;
    int ret;

    if (!DTMD_ISTEXT (window->dtmd)) {
        return (0);
    }
    for (ret = window->edit_control->mxedt; ret > 0 && window->edit_control->window[ret] != window; ret--)
        ;
    if (!ret) {
        return (0);
    }
    WpeMouseChangeShape (WpeWorkingShape);
    buffer = window->edit_control->window[ret]->buffer;
    ret = e_write (0, 0, buffer->mx.x, buffer->mxlines - 1, window, WPE_BACKUP);
    if (ret != WPE_ESC) {
        window->save = 0;
    }
    WpeMouseRestoreShape ();
    return (ret);
}

/*	save all windows */
int
e_saveall (we_window_t * window)
{
    int i, ret = 0;
    we_control_t *control = window->edit_control;

    for (i = control->mxedt; i > 0; i--) {
        if (control->window[i]->save != 0 && control->window[i]->ins != 8)
            if (e_save (control->window[i]) == WPE_ESC) {
                ret = WPE_ESC;
            }
    }
    if (ret == WPE_ESC) {
        e_message (0, e_msg[ERR_MSG_SAVEALL], window);
    }
    return (ret);
}

/*	terminate edit session */
int
e_quit (we_window_t * window)
{
    int i;
    char tmp[128];
#if  MOUSE
    int g[4];
#endif
    we_control_t *control = window->edit_control;
#ifdef DEBUGGER
    e_d_quit_basic (window);
#endif
    for (i = control->mxedt; i > 0; i--) {
        if (e_close_window (control->window[control->mxedt]) == WPE_ESC) {
            return (WPE_ESC);
        }
    }
    if (control->autosv & 1) {
        e_save_opt (control->window[0]);
    }
#ifdef UNIX
    if (WpeQuitWastebasket (control->window[0])) {
        return (0);
    }
    sprintf (tmp, "rm -rf %s&", e_tmp_dir);
    int ret = system (tmp);
    if (WIFSIGNALED (ret)
            && (WTERMSIG (ret) == SIGINT || WTERMSIG (ret) == SIGQUIT)) {
        printf ("System call command %s resulted in an interrupt.\n", tmp);
    } else if (ret == 127) {
        printf ("System call command %s failed with code 127\n%s\n%s\n%s\n",
                tmp,
                "This could mean one of two things:",
                "1. No shell was available (should never happen unless using chroot)",
                "2. The command returned 127.\n");
    } else if (ret != 0) {
        printf ("System call command %s failed. Return code = %i.\n", tmp, ret);
    }

#endif
#if  MOUSE
    g[0] = 2;
    fk_u_mouse (g);
#endif
    e_cls (control->colorset->ws, ' ');
    fk_u_locate (0, 0);
    fk_u_cursor (1);
    e_u_refresh ();
    e_exit (0);
    return (0);
}

/*	write file to disk */
int
e_write (int xa, int ya, int xe, int ye, we_window_t * window, int backup)
{
    we_buffer_t *buffer;
    int i = xa, j;
    char *tmp, *ptmp;
    FILE *fp;

    for (j = window->edit_control->mxedt; j > 0 && window->edit_control->window[j] != window; j--)
        ;
    buffer = window->edit_control->window[j]->buffer;
    if (window->ins == 8) {
        return (WPE_ESC);
    }
    ptmp = e_mkfilename (window->dirct, window->datnam);
// F_OK means: test for existance and has the value zero.
// Use R_OK, W_OK and X_OK for Read, Write or execute permissions
// FIXME: man page of access: determines access based on real user, not effective user
    if ((backup == WPE_BACKUP) && (access (ptmp, F_OK) == 0)) {
        tmp = e_bakfilename (ptmp);
        if (access (tmp, F_OK) == 0) { // real user ID has access to this file and it exists
            remove (tmp);    // remove on basis of effective user ID
        }
        WpeRenameCopy (ptmp, tmp, window, 1);
        free (tmp);
    }
    if ((fp = fopen (ptmp, "wb")) == NULL) {
        e_error (e_msg[ERR_FOPEN], ERROR_MSG, window->colorset);
        free (ptmp);
        return (WPE_ESC);
    }
    if (window->filemode >= 0) {
        chmod (ptmp, window->filemode);
    }
    for (j = ya; j < ye && j < buffer->mxlines; j++) {
        if (buffer->buflines[j].s != NULL)
            for (; i <= buffer->buflines[j].len && i < buffer->mx.x && *(buffer->buflines[j].s + i) != '\0';
                    i++) {
                if (*(buffer->buflines[j].s + i) == WPE_WR) {
                    if (DTMD_MSDOS == window->dtmd) {
                        putc (WPE_CR, fp);
                    }
                    putc (WPE_WR, fp);
                    break;
                } else {
                    putc (*(buffer->buflines[j].s + i), fp);
                }
            }
        i = 0;
    }
    if (buffer->buflines[j].s != NULL)
        for (i = 0;
                i < xe && *(buffer->buflines[j].s + i) != '\0' && i <= buffer->buflines[j].len
                && i < buffer->mx.x; i++) {
            if (*(buffer->buflines[j].s + i) == WPE_WR) {
                if (DTMD_MSDOS == window->dtmd) {
                    putc (WPE_CR, fp);
                }
                putc (WPE_WR, fp);
                break;
            } else {
                putc (*(buffer->buflines[j].s + i), fp);
            }
        }
    free (ptmp);
    fclose (fp);
    return (0);
}

/*	append new qualifier (suffix) to file name */
char *
e_new_qual (char *s, char *ns, char *sb)
{
    int i, j;

    for (i = strlen (s); i >= 0 && s[i] != '.' && s[i] != DIRC; i--)
        ;
    if (i < 0 || s[i] == DIRC) {
        strcpy (sb, s);
    } else if (i == 0 || s[i - 1] == DIRC) {
        strcpy (sb, s);
    } else {
        for (j = 0; j < i; j++) {
            sb[j] = s[j];
        }
        sb[j] = '\0';
    }
    strcat (sb, ".");
    strcat (sb, ns);
    return (sb);
}

/*     make up the bak-file name */
char *
e_bakfilename (char *s)
{
    /* "" is the special code for TurboC replace-extension style */
#ifndef SIMPLE_BACKUP_SUFFIX
#define SIMPLE_BACKUP_SUFFIX ""
#endif

    static char *bak = NULL;
    char *result;
    if (!bak) {
        bak = getenv ("SIMPLE_BACKUP_SUFFIX");
        if (!bak) {
            bak = SIMPLE_BACKUP_SUFFIX;
        }
    }

    result = malloc (strlen (s) + strlen (bak) + 5);
    if (!*bak) {
        return e_new_qual (s, "bak", result);    /* TurboC style */
    } else {
        strcpy (result, s);
        strcat (result, bak);
        return result;
    }
}

/*    Clear file-/directory-structure   */
int
freedf (struct dirfile *df)
{
    if (df == NULL) {
        return (-1);
    }
    if (df->name) {
        for (; df->nr_files > 0; (df->nr_files)--) {
            if (*(df->name + df->nr_files - 1)) {
                free (*(df->name + df->nr_files - 1));
            }
        }
        free (df->name);
    }
    free (df);
    df = NULL;
    return (0);
}



int
e_file_window (int sw, FLWND * fw, int ft, int fz)
{
    int i, c, nsu = 0;
    int len = 0;
    char sustr[18];
    if (fw->df->nr_files > 0) {
        if (fw->nf >= fw->df->nr_files) {
            fw->nf = fw->df->nr_files - 1;
        }
        len = strlen (*(fw->df->name + fw->nf));
    }
    e_mouse_bar (fw->xe, fw->ya, fw->ye - fw->ya, 0, fw->window->colorset->em.fg_bg_color);
    e_mouse_bar (fw->xa, fw->ye, fw->xe - fw->xa, 1, fw->window->colorset->em.fg_bg_color);
    while (1) {
        e_pr_file_window (fw, 1, 1, ft, fz, 0);
#if  MOUSE
        if ((c = e_u_getch ()) < 0) {
            c = fl_wnd_mouse (sw, c, fw);
        }
#else
        UNUSED(sw);
        c = e_u_getch ();
#endif
        if (fw->df->nr_files <= 0) {
            return (WPE_ESC);
        }
        if (c == CUP || c == CtrlP) {
            if (fw->nf <= 0) {
                return (c);
            } else {
                fw->nf--;
            }
            if (fw->srcha < 0) {
                nsu = 0;
            }
        } else if ((c == CDO || c == CtrlN) && fw->nf < fw->df->nr_files - 1) {
            fw->nf++;
            if (fw->srcha < 0) {
                nsu = 0;
            }
        } else if (c == CLE || c == CtrlB) {
            if (fw->ja <= 0) {
                return (c);
            } else {
                fw->ja--;
            }
        } else if (c == CRI || c == CtrlF) {
            if (fw->ja > len - 2) {
                return (c);
            } else {
                fw->ja++;
            }
        } else if (c == BUP) {
            if (fw->nf <= 0) {
                return (c);
            }
            if ((fw->nf -= (fw->ye - fw->ya - 1)) < 0) {
                fw->nf = 0;
            }
            if (fw->srcha < 0) {
                nsu = 0;
            }
        } else if (c == BDO) {
            if ((fw->nf += (fw->ye - fw->ya - 1)) > fw->df->nr_files - 1) {
                fw->nf = fw->df->nr_files - 1;
            }
            if (fw->srcha < 0) {
                nsu = 0;
            }
        } else if (c == POS1 || c == CtrlA || c == CBUP) {
            fw->nf = 0;
            nsu = 0;
            fw->ja = fw->srcha > 0 ? fw->srcha : 0;
        } else if (c == ENDE || c == CtrlE || c == CBDO) {
            fw->nf = fw->df->nr_files - 1;
        } else if (c >= 32 && c < 127) {
            sustr[nsu] = c;
            sustr[++nsu] = '\0';
            if (fw->srcha < 0) {
                for (i = nsu > 1 ? fw->nf : fw->nf + 1; i < fw->df->nr_files; i++) {
                    for (c = 0; *(fw->df->name[i] + c)
                            && (*(fw->df->name[i] + c) <= 32
                                || *(fw->df->name[i] + c) >= 127); c++);
#ifdef UNIX
                    if (!WpeIsXwin () && *(fw->df->name[i] + c - 1) == 32) {
                        c += 3;
                    }
#endif
                    if (strncmp (fw->df->name[i] + c, sustr, nsu) >= 0) {
                        break;
                    }
                }
            } else {
                for (i = 0; i < fw->df->nr_files
                        && strncmp (fw->df->name[i] + fw->srcha, sustr, nsu) < 0;
                        i++);
            }
            fw->nf = i < fw->df->nr_files ? i : fw->df->nr_files - 1;
        } else if (c == CCLE) {
            return (c);
        } else if (c == CCRI) {
            return (c);
        } else if (c) {
            nsu = 0;
            return (c);
        } else if (fw->srcha < 0) {
            nsu = 0;
        }
        if (fw->nf > fw->df->nr_files - 1) {
            fw->nf = fw->df->nr_files - 1;
        }
        len = strlen (*(fw->df->name + fw->nf));
        if (fw->ja >= len) {
            fw->ja = !len ? len : len - 1;
        }
        if (fw->nf - fw->ia >= fw->ye - fw->ya) {
            fw->ia = fw->nf + fw->ya - fw->ye + 1;
        } else if (fw->nf - fw->ia < 0) {
            fw->ia = fw->nf;
        }
    }
    return (0);
}

int
e_pr_file_window (FLWND * fw, int c, int sw, int ft, int fz, int fs)
{
#ifdef NEWSTYLE
    int xrt = 0;
#endif
    int i = fw->ia, len;

    if (fw->df != NULL) {
        for (; i < fw->df->nr_files && i - fw->ia < fw->ye - fw->ya; i++) {
            if ((len = strlen (*(fw->df->name + i))) < 0) {
                len = 0;
            }
            if (i == fw->nf && c && len >= fw->ja) {
                e_pr_nstr (fw->xa + 1, fw->ya + i - fw->ia, fw->xe - fw->xa,
                           *(fw->df->name + i) + fw->ja, fz, fz);
#ifdef NEWSTYLE
                xrt = 1;
#endif
            } else if (i == fw->nf && len >= fw->ja)
                e_pr_nstr (fw->xa + 1, fw->ya + i - fw->ia, fw->xe - fw->xa,
                           *(fw->df->name + i) + fw->ja, fs, fs);
            else if (len >= fw->ja)
                e_pr_nstr (fw->xa + 1, fw->ya + i - fw->ia, fw->xe - fw->xa,
                           *(fw->df->name + i) + fw->ja, ft, ft);
            else if (len < fw->ja) {
                e_blk (fw->xe - fw->xa - 1, fw->xa + 1, fw->ya + i - fw->ia, ft);
            }
            /*
               if ((len -= fw->ja) < 0) len = 0;
               e_blk(fw->xe-fw->xa-len-1, fw->xa+len+1, fw->ya+i-fw->ia, ft);
            */
            if (sw) {
                fw->nyfo = e_lst_zeichen (fw->xe, fw->ya, fw->ye - fw->ya, 0,
                                          fw->window->colorset->em.fg_bg_color, fw->df->nr_files,
                                          fw->nyfo, fw->nf);
                fw->nxfo =
                    e_lst_zeichen (fw->xa, fw->ye, fw->xe - fw->xa, 1,
                                   fw->window->colorset->em.fg_bg_color, len, fw->nxfo, fw->ja);
            }
        }
    }
    for (; i - fw->ia < fw->ye - fw->ya; i++) {
        e_blk (fw->xe - fw->xa, fw->xa, fw->ya + i - fw->ia, ft);
    }
#ifdef NEWSTYLE
    e_make_xrect (fw->xa, fw->ya, fw->xe - 1, fw->ye - 1, 1);
    if (xrt)
        e_make_xrect_abs (fw->xa, fw->ya + fw->nf - fw->ia, fw->xe - 1,
                          fw->ya + fw->nf - fw->ia, 0);
#endif
    return (0);
}


struct help_ud {
    struct help_ud *next;
    char *str, *nstr, *pstr, *file;
    int x, y, sw;
} *ud_help;

#ifdef HAVE_LIBZ
typedef gzFile IFILE;

#define e_i_fgets(s, n, p) gzgets(p, s, n)
#define e_i_fclose(p) gzclose(p)
#else
typedef struct {
    FILE *fp;
    int sw;
} ifile_s;

typedef ifile_s *IFILE;

#define e_i_fgets(s, n, p) fgets(s, n, p->fp)
#endif

int
e_mkdir_path (char *path)
{
    int i, len;
    char *tmp = malloc (((len = strlen (path)) + 1) * sizeof (char));

    if (!tmp) {
        return (-1);
    }
    strcpy (tmp, path);
    for (i = len; i > 0 && tmp[i] != DIRC; i--)
        ;
    if (i > 0) {
        tmp[i] = '\0';
        if (access (tmp, F_OK)) {
            e_mkdir_path (tmp);
            mkdir (tmp, 0700);
        }
    }
    free (tmp);
    return (0);
}

IFILE
e_i_fopen (char *path, char *stat)
{
    char *tmp2;
#ifdef HAVE_LIBZ
    IFILE fp;

    if (!path) {
        return (NULL);
    }
    if ((fp = gzopen (path, stat))) {
        return (fp);
    }
    if (!(tmp2 = malloc ((strlen (path) + 11) * sizeof (char)))) {
        free (fp);
        return (NULL);
    }
    strcpy (tmp2, path);
    strcat (tmp2, ".info");
    if ((fp = gzopen (tmp2, stat))) {
        free (tmp2);
        return (fp);
    }
    strcpy (tmp2, path);
    strcat (tmp2, ".gz");
    if ((fp = gzopen (tmp2, stat))) {
        free (tmp2);
        return (fp);
    }
    strcpy (tmp2, path);
    strcat (tmp2, ".Z");
    if ((fp = gzopen (tmp2, stat))) {
        free (tmp2);
        return (fp);
    }
    strcpy (tmp2, path);
    strcat (tmp2, ".info.gz");
    if ((fp = gzopen (tmp2, stat))) {
        free (tmp2);
        return (fp);
    }
    strcpy (tmp2, path);
    strcat (tmp2, ".info.Z");
    if ((fp = gzopen (tmp2, stat))) {
        free (tmp2);
        return (fp);
    }
    free (fp);
    free (tmp2);
    return (NULL);
#else
    extern char *e_tmp_dir;
    char *tmp, *command;
    int len;
    IFILE fp = malloc (sizeof (IFILE));

    if (!fp) {
        return (NULL);
    }
    if (!path) {
        free (fp);
        return (NULL);
    }
    if ((fp->fp = fopen (path, stat))) {
        fp->sw = 0;
        return (fp);
    }
    if (!(tmp2 = malloc ((strlen (path) + 11) * sizeof (char)))) {
        free (fp);
        return (NULL);
    }
    strcpy (tmp2, path);
    strcat (tmp2, ".info");
    if ((fp->fp = fopen (tmp2, stat))) {
        free (tmp2);
        fp->sw = 0;
        return (fp);
    }
    strcpy (tmp2, path);
    strcat (tmp2, ".gz");
    if (access (tmp2, F_OK)) {
        strcpy (tmp2, path);
        strcat (tmp2, ".Z");
        if (access (tmp2, F_OK)) {
            strcpy (tmp2, path);
            strcat (tmp2, ".info.gz");
            if (access (tmp2, F_OK)) {
                strcpy (tmp2, path);
                strcat (tmp2, ".info.Z");
                if (access (tmp2, F_OK)) {
                    free (fp);
                    free (tmp2);
                    return (NULL);
                }
            }
        }
    }
    if (!
            (tmp =
                 malloc ((strlen (path) + (len = strlen (e_tmp_dir)) +
                          2) * sizeof (char)))) {
        free (fp);
        free (tmp2);
        return (NULL);
    }
    strcpy (tmp, e_tmp_dir);
    if (path[0] != DIRC) {
        tmp[len] = DIRC;
        tmp[len + 1] = '\0';
    }
    strcat (tmp, path);
    if ((fp->fp = fopen (tmp, stat))) {
        fp->sw = 1;
        free (tmp);
        free (tmp2);
        return (fp);
    }
    command = malloc ((strlen (tmp) + strlen (tmp2) + 14) * sizeof (char));
    if (!command) {
        free (fp);
        free (tmp);
        free (tmp2);
        return (NULL);
    }
    e_mkdir_path (tmp);
    sprintf (command, "gunzip < %s > %s", tmp2, tmp);
    free (tmp2);
    int ret = system (command);
    if (WIFSIGNALED (ret)
            && (WTERMSIG (ret) == SIGINT || WTERMSIG (ret) == SIGQUIT)) {
        printf ("System call command %s resulted in an interrupt.\n%s\n",
                command, "Program will quit executing commands.\n");
    } else if (ret == 127) {
        printf ("System call command %s failed with code 127\n%s\n%s\n%s\n",
                command,
                "This could mean one of two things:",
                "1. No shell was available (should never happen unless using chroot)",
                "2. The command returned 127.\n");
    } else if (ret != 0) {
        printf ("System call command %s failed. Return code = %i.\n", command,
                ret);
    }
    free (command);
    fp->fp = fopen (tmp, stat);
    free (tmp);
    if (!fp->fp) {
        free (fp);
        return (NULL);
    }
    fp->sw = 1;
    return (fp);
#endif
}

#ifndef HAVE_LIBZ
int
e_i_fclose (IFILE fp)
{
    int ret = fclose (fp->fp);

    free (fp);
    return (ret);
}
#endif

int
e_read_help (char *str, we_window_t * window, int sw)
{
    IFILE fp;
    char *ptmp, tstr[256];
    int i;

    ptmp = e_mkfilename (LIBRARY_DIR, HELP_FILE);
    fp = e_i_fopen (ptmp, "rb");
    free (ptmp);
    if (!fp) {
        return (1);
    }
    e_close_buffer (window->buffer);
    if ((window->buffer = (we_buffer_t *) malloc (sizeof (we_buffer_t))) == NULL) {
        e_error (e_msg[ERR_LOWMEM], SERIOUS_ERROR_MSG, window->colorset);
    }
    if ((window->buffer->buflines = (STRING *) malloc (MAXLINES * sizeof (STRING))) == NULL) {
        e_error (e_msg[ERR_LOWMEM], SERIOUS_ERROR_MSG, window->colorset);
    }
    window->buffer->window = window;
    window->buffer->cursor = e_set_pnt (0, 0);
    window->buffer->mx = e_set_pnt (window->edit_control->maxcol, MAXLINES);
    window->buffer->mxlines = 0;
    window->buffer->colorset = window->colorset;
    window->buffer->control = window->edit_control;
    window->buffer->undo = NULL;
    e_new_line (0, window->buffer);
    if (str && str[0]) {
        while ((ptmp = e_i_fgets (tstr, 256, fp)) && !WpeStrcstr (tstr, str))
            ;
        if (ptmp && !sw) {
            strcpy ((char *) window->buffer->buflines[window->buffer->mxlines - 1].s, tstr);
            window->buffer->buflines[window->buffer->mxlines - 1].len =
                e_str_len (window->buffer->buflines[window->buffer->mxlines - 1].s);
            window->buffer->buflines[window->buffer->mxlines - 1].nrc =
                strlen ((const char *) window->buffer->buflines[window->buffer->mxlines - 1].s);
        }
    }
    if (sw) {
        char *tp = NULL, tmp[128], ts[2];

        ts[0] = HHD;
        ts[1] = '\0';
        while ((ptmp = e_i_fgets (tstr, 256, fp)) && !(tp = strstr (tstr, ts)))
            ;
        if (!str || !str[0])
            while ((ptmp = e_i_fgets (tstr, 256, fp))
                    && !(tp = strstr (tstr, ts)))
                ;
        if (!ptmp) {
            e_i_fclose (fp);
            return (e_error ("No Next Page", ERROR_MSG, window->colorset));
        } else {
            for (i = 0; (tmp[i] = tp[i]) && tp[i] != HED; i++)
                ;
            tmp[i] = HED;
            tmp[i + 1] = '\0';
            if ((ud_help->str =
                        malloc ((strlen (tmp) + 1) * sizeof (char))) != NULL) {
                strcpy (ud_help->str, tmp);
            }
            strcpy ((char *) window->buffer->buflines[window->buffer->mxlines - 1].s, tstr);
            window->buffer->buflines[window->buffer->mxlines - 1].len =
                e_str_len (window->buffer->buflines[window->buffer->mxlines - 1].s);
            window->buffer->buflines[window->buffer->mxlines - 1].nrc =
                strlen ((const char *) window->buffer->buflines[window->buffer->mxlines - 1].s);
        }
    }
    while (e_i_fgets (tstr, 256, fp)) {
        for (i = 0; tstr[i]; i++)
            if (tstr[i] == HFE) {
                e_i_fclose (fp);
                return (0);
            }
        e_new_line (window->buffer->mxlines, window->buffer);
        strcpy ((char *) window->buffer->buflines[window->buffer->mxlines - 1].s, tstr);
        window->buffer->buflines[window->buffer->mxlines - 1].len =
            e_str_len (window->buffer->buflines[window->buffer->mxlines - 1].s);
        window->buffer->buflines[window->buffer->mxlines - 1].nrc =
            strlen ((const char *) window->buffer->buflines[window->buffer->mxlines - 1].s);
    }
    e_i_fclose (fp);
    return (2);
}

int
e_help_ret (we_window_t * window)
{
    we_buffer_t *buffer = window->buffer;
    int i, j;
    unsigned char str[126];
    struct help_ud *next;

    for (i = buffer->cursor.x; i >= 0 && buffer->buflines[buffer->cursor.y].s[i] != HED; i--) {
        if (buffer->buflines[buffer->cursor.y].s[i] == HBG) {
            str[0] = HHD;
            for (j = i + 1; j < buffer->buflines[buffer->cursor.y].len
                    && (str[j - i] = buffer->buflines[buffer->cursor.y].s[j]) != HED; j++);
            str[j - i + 1] = '\0';
            if ((next = malloc (sizeof (struct help_ud))) != NULL) {
                next->str =
                    malloc ((strlen ((const char *) str) + 1) * sizeof (char));
                if (next->str) {
                    strcpy (next->str, (const char *) str);
                }
                if (ud_help && ud_help->file) {
                    next->file =
                        malloc ((strlen (ud_help->file) + 1) * sizeof (char));
                    if (next->file) {
                        strcpy (next->file, ud_help->file);
                    }
                } else {
                    next->file = NULL;
                }
                next->nstr = next->pstr = NULL;
                next->x = buffer->cursor.x;
                next->y = buffer->cursor.y;
                next->sw = ud_help ? ud_help->sw : 0;
                next->next = ud_help;
                ud_help = next;
            }
            if (ud_help->sw) {
                e_read_info (str, window, ud_help->file);
            } else {
                e_read_help ((char *) str, window, 0);
            }
            buffer = window->buffer;
            buffer->cursor.x = buffer->cursor.y = 0;
            e_cursor (window, 1);
            e_write_screen (window, 1);
            return (0);
        } else if (buffer->buflines[buffer->cursor.y].s[i] == HNF) {
            for (i++; i < buffer->buflines[buffer->cursor.y].len && buffer->buflines[buffer->cursor.y].s[i] != HED; i++) {
                ;
            }
            for (i++, j = 0; j + i < buffer->buflines[buffer->cursor.y].len
                    && (str[j] = buffer->buflines[buffer->cursor.y].s[j + i]) != HED; j++);
            str[j] = '\0';
            if ((next = malloc (sizeof (struct help_ud))) != NULL) {
                next->str = malloc (4 * sizeof (char));
                if (next->str) {
                    strcpy (next->str, "Top");
                }
                next->file =
                    malloc ((strlen ((const char *) str) + 1) * sizeof (char));
                if (next->file) {
                    strcpy (next->file, (const char *) str);
                }
                next->sw = 1;
                next->next = ud_help;
                next->x = buffer->cursor.x;
                next->y = buffer->cursor.y;
                next->nstr = next->pstr = NULL;
                ud_help = next;
            }
            e_read_info ("Top", window, ud_help->file);
            buffer = window->buffer;
            buffer->cursor.x = buffer->cursor.y = 0;
            e_cursor (window, 1);
            e_write_screen (window, 1);
            return (0);
        } else if (buffer->buflines[buffer->cursor.y].s[i] == HFB) {
            for (j = i + 1; j < buffer->buflines[buffer->cursor.y].len
                    && (str[j - i - 1] = buffer->buflines[buffer->cursor.y].s[j]) != HED; j++) {
                ;
            }
            str[j - i - 1] = '\0';
            return (e_ed_man (str, window));
        } else if (buffer->buflines[buffer->cursor.y].s[i] == HHD) {
            return (e_help_last (window));
        }
    }
    return (1);
}

int
e_help_last (we_window_t * window)
{
    struct help_ud *last = ud_help;

    if (!last) {
        return (1);
    }
    if (last->sw) {
        if (last->next == NULL) {
            e_read_info (NULL, window, NULL);
        } else {
            e_read_info (last->next->str, window, last->next->file);
        }
    } else {
        if (last->next == NULL) {
            e_read_help (NULL, window, 0);
        } else {
            e_read_help (last->next->str, window, 0);
        }
    }
    window->buffer->cursor.x = last->x;
    window->buffer->cursor.y = last->y;
    e_cursor (window, 1);
    e_write_screen (window, 1);
    ud_help = last->next;
    free (last->str);
    free (last);
    return (0);
}

int
e_help_next (we_window_t * window, int sw)
{
    struct help_ud *last = ud_help;
    if (last && last->sw) {
        if (sw && last->nstr) {
            if (last->str) {
                free (last->str);
            }
            if (last->pstr) {
                free (last->pstr);
            }
            last->str = last->nstr;
            last->nstr = NULL;
        } else if (!sw && last->pstr) {
            if (last->str) {
                free (last->str);
            }
            if (last->nstr) {
                free (last->nstr);
            }
            last->str = last->pstr;
            last->pstr = NULL;
        } else {
            return (e_error (sw ? "No Next Page" : "No Previous Page", ERROR_MSG, window->colorset));
        }
        e_read_info (last->str, window, last->file);
        window->buffer->cursor.x = window->buffer->cursor.y = 0;
        e_cursor (window, 1);
        e_write_screen (window, 1);
        return (0);
    } else {
        if (sw) {
            if ((last = malloc (sizeof (struct help_ud))) != NULL) {
                last->str = NULL;
                last->file = NULL;
                last->x = last->y = 0;
                last->nstr = last->pstr = NULL;
                last->sw = 0;
                last->next = ud_help;
                ud_help = last;
                e_read_help (ud_help->next ? ud_help->next->str : NULL, window, 1);
                window->buffer->cursor.x = window->buffer->cursor.y = 0;
                e_cursor (window, 1);
                e_write_screen (window, 1);
                return (0);
            }
        } else {
            return (e_help_last (window));
        }
    }
    return (e_error (sw ? "No Next Page" : "No Previous Page", ERROR_MSG, window->colorset));
}

int
e_help_free (we_window_t * window)
{
    UNUSED (window);
    struct help_ud *next, *last = ud_help;

    while (last) {
        next = last->next;
        if (last->str) {
            free (last->str);
        }
        if (last->pstr) {
            free (last->pstr);
        }
        if (last->nstr) {
            free (last->nstr);
        }
        if (last->file) {
            free (last->file);
        }
        free (last);
        last = next;
    }
    ud_help = NULL;
    return (0);
}

int
e_help_comp (we_window_t * window)
{
    we_buffer_t *buffer = window->buffer;
    int i, j, k, hn = 0, sn = 0;
    char **swtch = malloc (sizeof (char *));
    char **hdrs = malloc (sizeof (char *));
    char str[256];

    if (window->dtmd != DTMD_HELP) {
        return (1);
    }
    for (j = 0; j < buffer->mxlines; j++) {
        for (i = 0; i < buffer->buflines[j].len; i++) {
            if (buffer->buflines[j].s[i] == HBG) {
                for (k = i + 1;
                        k < buffer->buflines[j].len && (str[k - i - 1] = buffer->buflines[j].s[k]) != HED
                        && str[k - i - 1] != HBG && str[k - i - 1] != HHD
                        && str[k - i - 1] != HFE; k++)
                    ;
                if (str[k - i - 1] != HED) {
                    e_error ("Error in Switch!", ERROR_MSG, window->colorset);
                    buffer->cursor.x = k;
                    buffer->cursor.y = j;
                    e_cursor (window, 1);
                    return (2);
                }
                str[k - i - 1] = '\0';
                sn++;
                swtch = realloc (swtch, sn * sizeof (char *));
                swtch[sn - 1] = malloc ((strlen (str) + 1) + sizeof (char));
                strcpy (swtch[sn - 1], str);
            }
        }
    }
    for (j = 0; j < buffer->mxlines; j++) {
        for (i = 0; i < buffer->buflines[j].len; i++) {
            if (buffer->buflines[j].s[i] == HHD) {
                for (k = i + 1;
                        k < buffer->buflines[j].len && (str[k - i - 1] = buffer->buflines[j].s[k]) != HED
                        && str[k - i - 1] != HBG && str[k - i - 1] != HHD
                        && str[k - i - 1] != HFE; k++)
                    ;
                if (str[k - i - 1] != HED) {
                    e_error ("Error in Header!", ERROR_MSG, window->colorset);
                    buffer->cursor.x = k;
                    buffer->cursor.y = j;
                    e_cursor (window, 1);
                    return (2);
                }
                str[k - i - 1] = '\0';
                hn++;
                hdrs = realloc (hdrs, hn * sizeof (char *));
                hdrs[hn - 1] = malloc ((strlen (str) + 1) + sizeof (char));
                strcpy (hdrs[hn - 1], str);
            }
        }
    }
    for (j = 0; j < sn; j++) {
        for (i = 0; i < hn; i++)
            if (!WpeStrccmp (swtch[j], hdrs[i])) {
                break;
            }
        if (i >= hn) {
            sprintf (str, "Switch \'%s\' has no Header!", swtch[j]);
            e_error (str, ERROR_MSG, window->colorset);
            goto ende;
        }
    }
    for (j = 0; j < hn; j++) {
        for (i = 0; i < sn; i++)
            if (!WpeStrccmp (swtch[i], hdrs[j])) {
                break;
            }
        if (i >= sn) {
            sprintf (str, "No Jump to Header \'%s\'!", hdrs[j]);
            e_error (str, ERROR_MSG, window->colorset);
            goto ende;
        }
    }
ende:
    for (i = 0; i < sn; i++) {
        free (swtch[i]);
    }
    for (i = 0; i < hn; i++) {
        free (hdrs[i]);
    }
    free (swtch);
    free (hdrs);
    return (0);
}

/*          Help window    */
int
e_help (we_window_t * window)
{
    extern char *e_hlp;

    e_hlp = NULL;
    return (e_help_loc (window, 0));
}

int
e_info (we_window_t * window)
{
    extern char *e_hlp;

    e_hlp = NULL;
    return (e_help_loc (window, 1));
}

#define IFE 31

/**
 * Make an Info button.
 *
 * \TODO This function assumes an format unknown to current maintainer.
 *
 * Remark. Refactored name 'bg' into begin. See HBG for reference.
 * Refactored 'nd' to next data. Also suspicion.
 *
 * \param str char pointer to the info on the button.
 * \returns 1 if the string is empty, zero if the string was ok, -1 on error.
 *
 */
int
e_mk_info_button (char *str)
{
    int i, begin, next, len = strlen (str);

    /* Return 1 for empty string */
    if (str[0] == '\n' || str[0] == '\0') {
        return (1);
    }
    /* Search for beginning of string */
    for (begin = 0; str[begin] && isspace (str[begin]); begin++)
        ;
    /* Seems to look for next selection of: EOF or ": " or ":[.]*(" or ":[.]*:\." */
    /* \TODO Find out the purpose of these formats. */
    for (next = begin; str[next] &&
            (str[next] != ':' ||
             (!isspace (str[next + 1]) && (str[next + 1] != '(')
              && (str[next + 1] != ':'
                  || (!isspace (str[next + 2])
                      && str[next + 2] != '.')
                 )
             )
            ); next++)
        ;
    /* postcondition: str contains EOF or ": " or ":(" or "::."
     * the space could be any character that isspace() recognizes as space.
     */
    /* if we get an empty string after the previous search, we have an error */
    if (!str[next]) {
        return (-1);
    }
    /* in case of ":" process the text further */
    if (str[next + 1] != ':') {
        /* Find the first non space character */
        for (next++; str[next] && isspace (str[next]); next++);
        /* process text between parentheses, removing the parentheses. */
        if (str[next] == '(') {
            for (i = len; i >= begin; i--) {
                str[i + 1] = str[i];
            }
            str[begin] = HNF;
            str[next + 1] = HED;
            /* Find ending parentheses */
            for (next++; str[next] && str[next] && str[next] != ')'; next++);
            /* Replace ending parentheses */
            if (str[next]) {
                str[next] = HED;
            }
            /* return zero to indicate string found */
            return (0);
        }

        for (; str[next] && (str[next] != '.' || isalnum1 (str[next + 1])); next++);
        if (!str[next]) {
            return (-1);
        }
        for (begin = next; str[begin] != ':' || !isspace (str[begin + 1]); begin--);
        for (begin++; isspace (str[begin]); begin++);
    }
    for (i = len; i >= next; i--) {
        str[i + 2] = str[i];
    }
    str[next + 1] = HED;
    for (; i >= begin; i--) {
        str[i + 1] = str[i];
    }
    str[begin] = HBG;
    return (0);
}

int
e_mk_info_mrk (char *str)
{
    int bg, nd = -1;

    do {
        for (bg = nd + 1; str[bg] && str[bg] != '`'; bg++)
            ;
        for (nd = bg; str[nd] && str[nd] != '\''; nd++)
            ;
        if (str[nd]) {
            str[bg] = HBB;
            str[nd] = HED;
        }
    } while (str[nd]);
    return (0);
}

IFILE
e_info_jump (char *str, char **path, IFILE fp)
{
    IFILE fpn;
    int i, j, n, anz = 0;
    char *ptmp, *fstr, tstr[256], nfl[128];
    struct FL_INFO {
        char *name;
        int line;
    } **files = malloc (1);

    while (e_i_fgets (tstr, 256, fp) && tstr[0] != IFE) {
        for (i = 0; (nfl[i] = tstr[i]) && tstr[i] != ':'; i++)
            ;
        if (!tstr[i]) {
            continue;
        }
        nfl[i] = '\0';
        anz++;
        files = realloc (files, anz * sizeof (struct FL_INFO *));
        files[anz - 1] = malloc (sizeof (struct FL_INFO));
        files[anz - 1]->name = malloc ((strlen (nfl) + 1) * sizeof (char));
        strcpy (files[anz - 1]->name, nfl);
        files[anz - 1]->line = atoi (tstr + i + 1);
    }
    i = (strlen (str) + 6);
    fstr = malloc ((i + 1) * sizeof (char));
    strcat (strcpy (fstr, "Node: "), str);
    if (fstr[n = strlen (fstr) - 1] == HED) {
        fstr[n] = '\0';
        i--;
    }
    while ((ptmp = e_i_fgets (tstr, 256, fp)) && strncmp (tstr, fstr, i))
        ;
    if (ptmp) {
        n = atoi (tstr + i + 1);
        for (i = 1; i < anz && n >= files[i]->line; i++)
            ;
        if (files[i - 1]->name[0] == DIRC) {
            ptmp = malloc ((strlen (files[i - 1]->name) + 1) * sizeof (char));
            strcpy (ptmp, files[i - 1]->name);
        } else {
            for (n = strlen (*path) - 1; n >= 0 && *(*path + n) != DIRC; n--)
                ;
            n++;
            ptmp =
                malloc ((strlen (files[i - 1]->name) + n + 1) * sizeof (char));
            for (j = 0; j < n; j++) {
                ptmp[j] = *(*path + j);
            }
            ptmp[j] = '\0';
            strcat (ptmp, files[i - 1]->name);
        }
        if ((fpn = e_i_fopen (ptmp, "rb")) != NULL) {
            e_i_fclose (fp);
            fp = fpn;
            free (*path);
            *path = ptmp;
        } else {
            free (ptmp);
        }
    }
    free (fstr);
    for (i = 0; i < anz; i++) {
        free (files[i]->name);
        free (files[i]);
    }
    free (files);
    return (fp);
}

char *
e_mk_info_pt (char *str, char *node)
{
    int i;
    char *ptmp, tmp[128];

    do {
        if (!(ptmp = strstr (str, node))) {
            return (NULL);
        }
        while (*ptmp && *ptmp != ',' && *ptmp != ':') {
            ptmp++;
        }
        if (!*ptmp) {
            return (NULL);
        }
    } while (*ptmp == ',');
    for (ptmp++; isspace (*ptmp); ptmp++)
        ;
    for (i = 0; (tmp[i] = ptmp[i]) && ptmp[i] != ',' && ptmp[i] != '\n'; i++)
        ;
    tmp[i] = '\0';
    if (i == 0) {
        return (NULL);
    }
    ptmp = malloc ((strlen (tmp) + 1) * sizeof (char));
    strcpy (ptmp, tmp);
    return (ptmp);
}

char *
e_mk_info_path (char *path, char *file)
{
    int n;
    char *tp;

    if (!info_file || !*info_file) {
        return (NULL);
    }
    if (file[0] == DIRC) {
        if (path && !strcmp (path, file)) {
            free (path);
            return (NULL);
        } else if (path) {
            free (path);
        }
        if (!(path = malloc ((strlen (file) + 1) * sizeof (char)))) {
            return (NULL);
        }
        return (strcpy (path, file));
    }
    tp = info_file;
    if (path) {
        n = strlen (path) - strlen (file);
        if (n > 1) {
            path[n - 1] = PTHD;
        } else if (n == 1) {
            path[n++] = PTHD;
        }
        while (strncmp (tp, path, n) && (tp = strchr (tp, PTHD)) != NULL
                && *++tp);
        if (tp == NULL || !*tp || !*(tp += n)) {
            return (NULL);
        }
        free (path);
    }
    for (n = 0; tp[n] && tp[n] != PTHD; n++);
    if (!(path = malloc ((strlen (file) + n + 2) * sizeof (char)))) {
        return (NULL);
    }
    strncpy (path, tp, n);
    if (n > 1) {
        path[n++] = DIRC;
    }
    strcpy (path + n, file);
    return (path);
}

int
e_read_info (char *str, we_window_t * window, char *file)
{
    IFILE fp = NULL;
    char *path = NULL, *ptmp, tstr[256], fstr[128];
    int i, len, sw = 0, bsw = 0;
    if (!str) {
        str = "Top";
    }
    if (str[0] == HHD) {
        str++;
    }
    strcat (strcpy (fstr, "Node: "), str);
    len = strlen (fstr);
    if (fstr[len - 1] == HED) {
        fstr[len - 1] = '\0';
    }
    if (!file || !file[0]) {
        file = "dir";
    }
    do {
        path = e_mk_info_path (path, file);
        fp = e_i_fopen (path, "rb");
    } while (!fp && path);
    if (!fp) {
        return (1);
    }
    e_close_buffer (window->buffer);
    if ((window->buffer = (we_buffer_t *) malloc (sizeof (we_buffer_t))) == NULL) {
        e_error (e_msg[ERR_LOWMEM], SERIOUS_ERROR_MSG, window->colorset);
    }
    if ((window->buffer->buflines = (STRING *) malloc (MAXLINES * sizeof (STRING))) == NULL) {
        e_error (e_msg[ERR_LOWMEM], SERIOUS_ERROR_MSG, window->colorset);
    }
    window->buffer->window = window;
    window->buffer->cursor = e_set_pnt (0, 0);
    window->buffer->mx = e_set_pnt (window->edit_control->maxcol, MAXLINES);
    window->buffer->mxlines = 0;
    window->buffer->colorset = window->colorset;
    window->buffer->control = window->edit_control;
    window->buffer->undo = NULL;
    e_new_line (0, window->buffer);
    while ((ptmp = e_i_fgets (tstr, 256, fp)) != NULL) {
        if (!strncmp (tstr, "Indirect:", 9)) {
            fp = e_info_jump (str, &path, fp);
        } else if (!strncmp (tstr, "File:", 5) && strstr (tstr, fstr)) {
            break;
        }
    }
    free (path);
    if (ptmp) {
        ud_help->nstr = e_mk_info_pt (tstr, "Next");
        ud_help->pstr = e_mk_info_pt (tstr, "Prev");
    }
    if (!strcmp (file, "dir"))
        while ((ptmp = e_i_fgets (tstr, 256, fp))
                && WpeStrnccmp (tstr, "* Menu:", 7));
    else
        while ((ptmp = e_i_fgets (tstr, 256, fp)) && tstr[0] == '\n');
    if (ptmp) {
        if (!sw && !WpeStrnccmp (tstr, "* Menu:", 7)) {
            sw = 1;
        }
        for (i = len = strlen (tstr); i >= 0; i--) {
            tstr[i + 1] = tstr[i];
        }
        tstr[0] = HHD;
        tstr[len + 1] = HED;
        tstr[len + 1] = '\0';
        strcpy ((char *) window->buffer->buflines[window->buffer->mxlines - 1].s, tstr);
        window->buffer->buflines[window->buffer->mxlines - 1].len =
            e_str_len (window->buffer->buflines[window->buffer->mxlines - 1].s);
        window->buffer->buflines[window->buffer->mxlines - 1].nrc =
            strlen ((const char *) window->buffer->buflines[window->buffer->mxlines - 1].s);
    }
    while (e_i_fgets (tstr, 256, fp)) {
        for (i = 0; tstr[i]; i++)
            if (tstr[i] == IFE) {
                e_i_fclose (fp);
                return (0);
            }
        if (bsw == 1) {
            bsw = e_mk_info_button (tstr);
            for (ptmp = tstr; (ptmp = WpeStrcstr (ptmp, "*note")); ptmp += 5) {
                bsw = e_mk_info_button (ptmp + 5);
            }
        } else if (!sw && !WpeStrnccmp (tstr, "* Menu:", 7)) {
            sw = 1;
        } else if ((ptmp = WpeStrcstr (tstr, "*note"))) {
            bsw = e_mk_info_button (ptmp + 5);
            for (ptmp += 5; (ptmp = WpeStrcstr (ptmp, "*note")); ptmp += 5) {
                bsw = e_mk_info_button (ptmp + 5);
            }
        } else if (sw && tstr[0] == '*') {
            bsw = e_mk_info_button (tstr + 1);
            for (ptmp = tstr + 1; (ptmp = WpeStrcstr (ptmp, "*note"));
                    ptmp += 5) {
                bsw = e_mk_info_button (ptmp + 5);
            }
        }
        e_mk_info_mrk (tstr);
        e_new_line (window->buffer->mxlines, window->buffer);
        strcpy ((char *) window->buffer->buflines[window->buffer->mxlines - 1].s, tstr);
        window->buffer->buflines[window->buffer->mxlines - 1].len =
            e_str_len (window->buffer->buflines[window->buffer->mxlines - 1].s);
        window->buffer->buflines[window->buffer->mxlines - 1].nrc =
            strlen ((const char *) window->buffer->buflines[window->buffer->mxlines - 1].s);
    }
    e_i_fclose (fp);
    return (2);
}

int
e_help_loc (we_window_t * window, int sw)
{
    extern char *e_hlp;
    int i;
    char *tmp = NULL;
    struct help_ud *next;

    if (!sw) {
        tmp = e_hlp;
    }
    for (i = window->edit_control->mxedt; i >= 0; i--) {
        if (!strcmp (window->edit_control->window[i]->datnam, "Help")) {
            e_switch_window (window->edit_control->edt[i], window);
            if (ud_help && sw != ud_help->sw) {
                e_close_window (window->edit_control->window[window->edit_control->mxedt]);
                i = -1;
            }
            break;
        }
    }
    if (i < 0) {
        e_edit (window->edit_control, "Help");
    }
    if ((tmp || sw) && (next = malloc (sizeof (struct help_ud)))) {
        if (tmp) {
            next->str = malloc ((strlen (tmp) + 1) * sizeof (char));
            if (next->str) {
                strcpy (next->str, tmp);
            }
        } else {
            next->str = NULL;
        }
        next->file = NULL;
        next->x = next->y = 0;
        next->nstr = next->pstr = NULL;
        next->sw = sw;
        next->next = ud_help;
        ud_help = next;
    }
    if (sw) {
        e_read_info (NULL, window->edit_control->window[window->edit_control->mxedt], NULL);
    } else {
        e_read_help (tmp, window->edit_control->window[window->edit_control->mxedt], 0);
    }
    e_write_screen (window->edit_control->window[window->edit_control->mxedt], 1);
    return (0);
}

int
e_help_options (we_window_t * window)
{
    char str[128];

    if (!info_file) {
        info_file = malloc (1);
        info_file[0] = '\0';
    }
    strcpy (str, info_file);
    if (e_add_arguments (str, "Info-Path", window, 0, AltI, NULL)) {
        info_file = realloc (info_file, strlen (str) + 1);
        strcpy (info_file, str);
    }
    return (0);
}

int
e_hp_next (we_window_t * window)
{
    int i;

    for (i = window->edit_control->mxedt; i >= 0; i--) {
        if (!strcmp (window->edit_control->window[i]->datnam, "Help")) {
            e_switch_window (window->edit_control->edt[i], window);
            window = window->edit_control->window[window->edit_control->mxedt];
            break;
        }
    }
    if (i < 0) {
        return (e_help_loc (window, 0));
    } else {
        return (e_help_next (window, 1));
    }
}

int
e_hp_prev (we_window_t * window)
{
    int i;

    for (i = window->edit_control->mxedt; i >= 0; i--) {
        if (!strcmp (window->edit_control->window[i]->datnam, "Help")) {
            e_switch_window (window->edit_control->edt[i], window);
            window = window->edit_control->window[window->edit_control->mxedt];
            break;
        }
    }
    if (i < 0) {
        return (e_help_loc (window, 0));
    } else {
        return (e_help_next (window, 0));
    }
}

int
e_hp_back (we_window_t * window)
{
    int i;

    for (i = window->edit_control->mxedt; i >= 0; i--) {
        if (!strcmp (window->edit_control->window[i]->datnam, "Help")) {
            e_switch_window (window->edit_control->edt[i], window);
            window = window->edit_control->window[window->edit_control->mxedt];
            break;
        }
    }
    if (i < 0) {
        return (e_help_loc (window, 0));
    } else {
        return (e_help_last (window));
    }
}

int
e_hp_ret (we_window_t * window)
{
    int i;

    for (i = window->edit_control->mxedt; i >= 0; i--) {
        if (!strcmp (window->edit_control->window[i]->datnam, "Help")) {
            e_switch_window (window->edit_control->edt[i], window);
            window = window->edit_control->window[window->edit_control->mxedt];
            break;
        }
    }
    if (i < 0) {
        return (e_help_loc (window, 0));
    } else {
        return (e_help_ret (window));
    }
}

/* Give a context-sensitive help for the identifier under cursor */
int
e_topic_search (we_window_t * window)
{
    int x, y;
    unsigned char *s;
    we_buffer_t *buffer;
    unsigned char item[100], *ptr = item;

    if (!DTMD_ISTEXT (window->edit_control->window[window->edit_control->mxedt]->dtmd)) {
        return (0);
    }
    buffer = window->edit_control->window[window->edit_control->mxedt]->buffer;
    y = buffer->cursor.y;
    x = buffer->cursor.x;
    s = buffer->buflines[y].s;
    if (!isalnum (s[x]) && s[x] != '_') {
        return (0);
    }
    for (; x >= 0 && (isalnum (s[x]) || s[x] == '_'); x--);
    if (x < 0 && !isalnum (s[0]) && s[0] != '_') {
        return (0);
    }
    x++;
    for (; x <= buffer->buflines[y].len && (isalnum (s[x]) || s[x] == '_'); x++, ptr++) {
        *ptr = s[x];
    }
    *ptr = 0;
    e_ed_man (item, window);
    return (0);
}
