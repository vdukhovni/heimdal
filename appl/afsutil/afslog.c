/*
 * Copyright (c) 1997 Kungliga Tekniska H�gskolan
 * (Royal Institute of Technology, Stockholm, Sweden). 
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 *
 * 3. All advertising materials mentioning features or use of this software 
 *    must display the following acknowledgement: 
 *      This product includes software developed by Kungliga Tekniska 
 *      H�gskolan and its contributors. 
 *
 * 4. Neither the name of the Institute nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
RCSID("$Id$");
#endif
#include <krb5.h>
#include <kafs.h>
#include <roken.h>
#include <getarg.h>


static int help_flag;
static int version_flag;
static int crete_user;
static getarg_strings cells;
static char *realm;
static getarg_strings files;
static int unlog_flag;
static int verbose;

struct getargs args[] = {
    { "help", 'h', arg_flag, &help_flag, NULL, NULL },
    { "version", 0, arg_flag, &version_flag, NULL, NULL },
    { "verbose", 'v', arg_flag, &verbose, NULL, NULL },
#if 0
    { "create-user", 0, arg_flag, &crete_user, 
      "create user if not found", NULL },
#endif
    { "cell", 'c', arg_strings, &cells, "cell to get tokens for", "cell" },
    { "realm", 'k', arg_string, &realm, "realm for afs cell", "realm" },
    { "file", 'p', arg_strings, &files, "file to get tokens for", "path" },
    { "unlog", 'u', arg_flag, &unlog_flag, "remove tokens", NULL },
};

static int num_args = sizeof(args) / sizeof(args[0]);

static const char *
expand_cell_name(const char *cell)
{
    FILE *F;
    static char buf[128];
    char *p;

    F = fopen(_PATH_CELLSERVDB, "r");
    if(F == NULL)
	return cell;
    do{
	fgets(buf, 128, F);
	if(buf[0] == '>'){
	    for(p=buf; *p && *p != ' ' && *p != '\t'; p++);
	    *p=0;
	    if(strstr(buf, cell)){
		fclose(F);
		return buf + 1;
	    }
	}
	buf[0] = 0;
    }while(!feof(F));
    fclose(F);
    return cell;
}

#if 0
static int
createuser (char *cell)
{
    char cellbuf[64];
    char name[ANAME_SZ];
    char instance[INST_SZ];
    char realm[REALM_SZ];
    char cmd[1024];

    if (cell == NULL) {
	FILE *f;
	int len;

	f = fopen (_PATH_THISCELL, "r");
	if (f == NULL)
	    err (1, "open(%s)", _PATH_THISCELL);
	if (fgets (cellbuf, sizeof(cellbuf), f) == NULL)
	    err (1, "read cellname from %s", _PATH_THISCELL);
	len = strlen(cellbuf);
	if (cellbuf[len-1] == '\n')
	    cellbuf[len-1] = '\0';
	cell = cellbuf;
    }

    if(krb_get_default_principal(name, instance, realm))
	errx (1, "Could not even figure out who you are");

    snprintf (cmd, sizeof(cmd),
	      "pts createuser %s%s%s@%s -cell %s",
	      name, *instance ? "." : "", instance, strlwr(realm),
	      cell);
    DEBUG("Executing %s", cmd);
    return system(cmd);
}
#endif

void
usage(int ecode)
{
    arg_printusage(args, num_args, "[cell]... [path]...");
    exit(ecode);
}

static int
afslog_cell(krb5_context context, krb5_ccache id,
	    const char *cell, int expand)
{
    const char *c = cell;
    if(expand){
	c = expand_cell_name(cell);
	if(c == NULL){
	    krb5_warnx(context, "No cell matching \"%s\" found.", cell);
	    return -1;
	}
	if(verbose)
	    krb5_warnx(context, "Cell \"%s\" expanded to \"%s\"", cell, c);
    }
    return krb5_afslog(context, id, c, realm);
}

static int
afslog_file(krb5_context context, krb5_ccache id,
	    const char *path)
{
    char cell[64];
    if(k_afs_cell_of_file(path, cell, sizeof(cell))){
	krb5_warnx(context, "No cell found for file \"%s\".", path);
	return -1;
    }
    if(verbose)
	krb5_warnx(context, "File \"%s\" lives in cell \"%s\"", path, cell);
    return afslog_cell(context, id, cell, 0);
}

int main(int argc, char **argv)
{
    int optind = 0;
    krb5_context context;
    krb5_ccache id;
    char cellbuf[64];
    int i;
    int num;
    
    set_progname(argv[0]);

    krb5_init_context(&context);
    if(!k_hasafs())
	krb5_errx(context, 1, 
		  "AFS doesn't seem to be present on this machine");
    if(getarg(args, num_args, argc, argv, &optind))
	usage(1);
    if(help_flag)
	usage(0);
    if(version_flag)
	krb5_errx(context, 0, "%s", heimdal_version);
    if(unlog_flag){
	k_unlog();
	exit(0);
    }
    krb5_cc_default(context, &id);
    num = 0;
    for(i = 0; i < files.num_strings; i++){
	afslog_file(context, id, files.strings[i]);
	num++;
    }
    for(i = 0; i < cells.num_strings; i++){
	afslog_cell(context, id, cells.strings[i], 1);
	num++;
    }
    for(i = optind; i < argc; i++){
	num++;
	if(strcmp(argv[i], ".") == 0 ||
	   strcmp(argv[i], "..") == 0 ||
	   strchr(argv[i], '/') ||
	   access(argv[i], F_OK) == 0)
	    afslog_file(context, id, argv[i]);
	else
	    afslog_cell(context, id, argv[i], 1);
    }    
    if(num == 0)
	krb5_afslog(context, id, NULL, NULL);
}
