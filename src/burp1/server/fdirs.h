#ifndef _FDIRS_H
#define _FDIRS_H

struct fdirs // Finishing directories.
{
	char *manifest;
	char *deletionsfile;
	char *datadir;
	char *datadirtmp;
	char *currentdup;
	char *currentduptmp;
	char *currentdupdata;
	char *timestamp;
	char *fullrealcurrent;
	char *logpath;
	char *hlinkedpath;
};

extern void fdirs_free(struct fdirs *fdirs);
extern struct fdirs *fdirs_alloc(void);
extern int fdirs_init(struct fdirs *fdirs,
	struct sdirs *sdirs, const char *realcurrent);

#endif
