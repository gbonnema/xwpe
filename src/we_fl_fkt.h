#ifndef WE_FL_FKT_H
#define WE_FL_FKT_H

#include "config.h"
#include <stdlib.h>
#include "we_edit.h"
#include "we_e_aus.h"
#include "we_fl_unix.h"
#include "we_mouse.h"
#include "we_opt.h"
#include "we_debug.h"

extern char *info_file;
extern char *e_tmp_dir;

/*   we_fl_fkt.c   */
char *e_mkfilename (char *dr, char *fn);
POINT e_readin (int i, int j, FILE * fp, BUFFER * b, char *sw);
int e_new (We_window * f);
int e_m_save (We_window * f);
int e_save (We_window * f);
int e_saveall (We_window * f);
int e_quit (We_window * f);
int e_write (int xa, int ya, int xe, int ye, We_window * f, int backup);
char *e_new_qual (char *s, char *ns, char *sb);
char *e_bakfilename (char *s);
int freedf (struct dirfile *df);
int e_file_window (int sw, FLWND * fw, int ft, int fz);
int e_pr_file_window (FLWND * fw, int c, int sw, int ft, int fz, int fs);
int e_help_last (We_window * f);
int e_help_comp (We_window * f);
int e_help (We_window * f);
int e_help_loc (We_window * f, int sw);
int e_help_free (We_window * f);
int e_help_ret (We_window * f);
int e_topic_search (We_window * f);

#endif
