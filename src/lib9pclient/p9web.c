/*
 * p9web: lib9pclient on the kernel's mount, not a 9P client in the program: a CFsys is a
 * service in /srv (post9pservice's) mounted with its spec on /mnt/name, where Plan 9 mounts it
 * (Acme's command sees /mnt/acme), in this process's namespace; a CFid is an open file under it. The kernel's mount driver
 * shares one 9P session among all the mounts of a /srv entry, as Plan 9's devmnt does, so
 * there is no 9pserve. A CFid's reads and writes go by an ioproc (libthread's), so they wait
 * in their thread only, as plan9port's do: Acme's plumb thread reads in its main proc. Only the
 * calls Acme and libplumb make are here.
 */
#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <9pclient.h>
#include <thread.h>

int chatty9pclient;	/* plumb -d sets it; the kernel's mount driver has the 9P, so it prints nothing */

struct CFsys
{
	char *dir;	/* where it is mounted */
};

struct CFid
{
	int fd;
	Ioproc *io;	/* made at the first read or write */
};

static Ioproc*
fidio(CFid *fid)
{
	if(fid->io == nil)
		fid->io = ioproc();
	return fid->io;
}

CFsys*
nsmount(char *name, char *aname)
{
	char srv[128], *dir;
	CFsys *fs;
	int fd;

	snprint(srv, sizeof srv, "/srv/%s", name);
	if((fd = open(srv, ORDWR)) < 0)
		return nil;
	if((dir = smprint("/mnt/%s", name)) == nil){
		close(fd);
		return nil;
	}
	if(mount(fd, -1, dir, MREPL, aname ? aname : "") < 0){
		close(fd);
		free(dir);
		return nil;
	}
	close(fd);
	if((fs = mallocz(sizeof *fs, 1)) == nil){
		unmount(nil, dir);
		free(dir);
		return nil;
	}
	fs->dir = dir;
	return fs;
}

/*
 * fsunmount lets go of fs; the mount stays in the namespace, as a Plan 9 mount does: the files
 * opened under it stay open, and Acme's command sees /mnt/acme after Acme's runproc lets go.
 */
void
fsunmount(CFsys *fs)
{
	if(fs == nil)
		return;
	free(fs->dir);
	free(fs);
}

int
fsopenfd(CFsys *fs, char *name, int mode)
{
	char *path;
	int fd;

	if((path = smprint("%s/%s", fs->dir, name)) == nil)
		return -1;
	fd = open(path, mode);
	free(path);
	return fd;
}

CFid*
fsopen(CFsys *fs, char *name, int mode)
{
	CFid *fid;
	int fd;

	if((fd = fsopenfd(fs, name, mode)) < 0)
		return nil;
	if((fid = mallocz(sizeof *fid, 1)) == nil){
		close(fd);
		return nil;
	}
	fid->fd = fd;
	return fid;
}

long
fsread(CFid *fid, void *buf, long n)
{
	return ioread(fidio(fid), fid->fd, buf, n);
}

long
fsreadn(CFid *fid, void *buf, long n)
{
	return ioreadn(fidio(fid), fid->fd, buf, n);
}

long
fswrite(CFid *fid, void *buf, long n)
{
	return iowrite(fidio(fid), fid->fd, buf, n);
}

void
fsclose(CFid *fid)
{
	if(fid == nil)
		return;
	if(fid->io)
		closeioproc(fid->io);
	close(fid->fd);
	free(fid);
}
