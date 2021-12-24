#ifndef USER_H
#define USER_H

typedef struct range_t {
	uid_t	r_from;		/* low uid */
	uid_t	r_to;		/* high uid */
} range_t;

#define COMMENTLEN_MAX 64
#define PATHLEN_MAX 64
#define PASSLEN_MAX 64
#define GROUPLEN_MAX 64

typedef struct user_t {
	int		u_flags;		/* see below */
	uid_t		u_uid;			/* uid of user */
	char	       u_password[PASSLEN_MAX];		/* encrypted password */
	char	       u_comment[COMMENTLEN_MAX];		/* comment field */
	char	       u_home[PATHLEN_MAX];		/* home directory */
	char	       u_primgrp[GROUPLEN_MAX];		/* primary group */
	int		u_groupc;		/* # of secondary groups */
	const char     *u_groupv[NGROUPS_MAX];	/* secondary groups */
	char	       u_shell[PATHLEN_MAX];		/* user's shell */
	char	       *u_basedir;		/* base directory for home */
	char	       *u_expire;		/* when account will expire */
	char	       *u_inactive;		/* when password will expire */
	char	       *u_skeldir;		/* directory for startup files */
	char	       *u_class;		/* login class */
	unsigned int	u_rsize;		/* size of range array */
	unsigned int	u_rc;			/* # of ranges */
	range_t	       *u_rv;			/* the ranges */
	unsigned int	u_defrc;		/* # of ranges in defaults */
	int		u_preserve;		/* preserve uids on deletion */
} user_t;

#endif