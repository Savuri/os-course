#ifndef JOS_INC_UCRED_H
#define JOS_INC_UCRED_H

#include <inc/types.h>

#define NGROUPS_MAX 32

struct Cred {
    //	u_int	ref;			/* reference count */

    uid_t uid;                 /* user id. */
    uid_t euid;                /* effective user id */
    uid_t svuid;               /* Saved effective user id. */
    gid_t gid;                 /* group id. */
    gid_t egid;                /* effective group id */
    gid_t svgid;               /* Saved effective group id. */
    short ngroups;             /* number of groups */
    gid_t groups[NGROUPS_MAX]; /* groups */
};

#endif /* !JOS_INC_UCRED_H */
