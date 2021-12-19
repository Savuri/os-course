#ifndef JOS_INC_UCRED_H
#define JOS_INC_UCRED_H

#include <inc/types.h>

#define NGROUPS_MAX 32

struct ucred {
//	u_int	cr_ref;			/* reference count */

	uid_t	cr_uid;			/* effective user id */
	uid_t	cr_ruid;		/* Real user id. */
	uid_t	cr_svuid;		/* Saved effective user id. */
	gid_t	cr_gid;			/* effective group id */
	gid_t	cr_rgid;		/* Real group id. */
	gid_t	cr_svgid;		/* Saved effective group id. */
	short	cr_ngroups;		/* number of groups */
	gid_t	cr_groups[NGROUPS_MAX];	/* groups */
};

#endif /* !JOS_INC_UCRED_H */

