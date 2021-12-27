#ifndef USER_H
#define USER_H

#define COMMENTLEN_MAX    32
#define PATHLEN_MAX       64
#define PASSLEN_MAX       64
#define UID_MAX           1024
#define NBUFSIZ           1024
#define NUSERSINGROUP_MAX 30

static gid_t readgid();
static uid_t readuid();
int isuserexist(uid_t uid);
int isgroupexist(gid_t gid);

typedef struct user_t {
    int u_flags;                       /* see below */
    uid_t u_uid;                       /* uid of user */
    char u_password[PASSLEN_MAX];      /* encrypted password */
    char u_comment[COMMENTLEN_MAX];    /* comment field */
    char u_home[PATHLEN_MAX];          /* home directory */
    gid_t u_primgrp;                   /* primary group */
    int u_groupc;                      /* # of secondary groups */
    const char *u_groupv[NGROUPS_MAX]; /* secondary groups */
    char u_shell[PATHLEN_MAX];         /* user's shell */
    char *u_basedir;                   /* base directory for home */
    char *u_expire;                    /* when account will expire */
    char *u_inactive;                  /* when password will expire */
    char *u_skeldir;                   /* directory for startup files */
    char *u_class;                     /* login class */
    unsigned int u_rsize;              /* size of range array */
    unsigned int u_rc;                 /* # of ranges */
    unsigned int u_defrc;              /* # of ranges in defaults */
    int u_preserve;                    /* preserve uids on deletion */
} user_t;

#endif
