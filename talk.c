#define FUSE_USE_VERSION 31

#include<fuse.h>
#include<stdio.h>
#include<string.h>
#include<errno.h>
#include<fcntl.h>
#include<stddef.h>
#include<assert.h>
#include<stdlib.h>

static const int MaxFileLen = 10;

char *debug;

static struct Node {
	int type; // 1: file 2: dir
	char *filename;
	char *contents;
	struct Node *nxt;
	struct Node *pre;
	struct Node *son;
	struct Node *par;
};
static struct Node *root;
static struct Node *log;

static struct Node *NewNode(int _type, char *_filename, char *_contents, struct Node *_nxt, struct Node *_pre, struct Node *_son, struct Node *_par) {
	struct Node *ptr = malloc(sizeof(struct Node));
	memset(ptr, 0, sizeof(struct Node));

	ptr -> type = _type;
	ptr -> filename = strdup(_filename);
	ptr -> contents = strdup(_contents);
	ptr -> nxt = _nxt;
	ptr -> pre = _pre;
	ptr -> son = _son;
	ptr -> par = _par;

	return ptr;
}

static void Add(struct Node *f, struct Node *ptr) {
	ptr -> par = f;
	struct Node *p = f;
	if(p -> son == NULL)
		p -> son = ptr;
	else {
		p = p -> son;
		while(p -> nxt != NULL)
			p = p -> nxt;
		p -> nxt = ptr;
		ptr -> pre = p;
	}
}

static void Del(struct Node *ptr) {
	if(ptr -> pre != NULL && ptr -> nxt != NULL)
		ptr -> pre -> nxt = ptr -> nxt, ptr -> nxt -> pre = ptr -> pre;
	else if(ptr -> pre != NULL)
		ptr -> pre -> nxt = NULL;
	else if(ptr -> nxt != NULL)
		ptr -> par -> son = ptr -> nxt, ptr -> nxt -> pre = NULL;
	else
		ptr -> par -> son = NULL;
}

static int Find(const char *path, struct Node **nd) {
	struct Node *p = root;
	for(int pos = 1; path[pos] != '\0';) {
		char name[MaxFileLen];
		int len = 0;
		while(path[pos] != '\0' && path[pos] != '/')
			name[len ++] = path[pos ++];
		name[len] = '\0';
		int type = path[pos] == '/' ? ++ pos, 2 : 0;
		if(p -> son == NULL)
			return -ENOENT;
		p = p -> son;
		int f = 0;
		while(p != NULL) {
			if(strcmp(p -> filename, name) == 0) {
				if(type && p -> type != type)
					return -ENOENT;
				f = 1;
				break;
			}
			p = p -> nxt;
		}
		if(!f)
			return -ENOENT;
	}
	(*nd) = p;
	return 0;
}

static void *InitChat(struct fuse_conn_info *con, struct fuse_config *cfg) {
	(void) con;
	cfg -> kernel_cache = 0;
	return NULL;
}

static int ChatGetattr(const char *path, struct stat *buf, struct fuse_file_info *info) {
	(void) info;
	memset(buf, 0, sizeof(struct stat));
	struct Node *nd;
	if(Find(path, &nd) != 0)
		return -ENOENT;
	if(nd -> type == 2)
		buf -> st_mode = S_IFDIR | 0755, buf -> st_nlink = 2;
	else
		buf -> st_mode = S_IFREG | 0770, buf -> st_nlink = 1, buf -> st_size = strlen(nd -> contents);
	return 0;
}

static int ChatReaddir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *info, enum fuse_readdir_flags flags) {
	(void) offset, (void) info, (void) flags;
	struct Node *nd;
	if(Find(path, &nd) != 0)
		return -ENOENT;
	filler(buf, ".", NULL, 0, 0), filler(buf, "..", NULL, 0, 0);
	nd = nd -> son;
	while(nd != NULL)
		filler(buf, nd -> filename, NULL, 0, 0), nd = nd -> nxt;
	return 0;
}

static int ChatOpen(const char *path, struct fuse_file_info *info) {
	struct Node *nd;
	if(Find(path, &nd) != 0)
		return -ENOENT;
	if(nd -> type != 1)
		return -ENOENT;
	return 0;
}

static int ChatRead(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *info) {
	(void) info;
	struct Node *nd;
	if(Find(path, &nd) != 0)
		return -ENOENT;
	if(nd -> type != 1)
		return -ENOENT;
	size_t len = strlen(nd -> contents);
	if(offset < len) {
		if(offset + size > len)
			size = len - offset;
		memcpy(buf, nd -> contents + offset, size);
	}
	else
		size = 0;
	return size;
}

static int ChatAccess(const char *path, int mask) {
	struct Node *nd;
	if(Find(path, &nd) != 0)
		return -ENOENT;
	if(nd -> type != 2)
		return -ENOENT;
	return 0;
}

/*
static int ChatWrite(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *info) {
	(void) info;
	int len = strlen(path), user1len = 0, user2len = 0, envilen = 0, pos = len;
	char user1[MaxFileLen], user2[MaxFileLen];
	char *envi = (char *)malloc(len + 1);

	if(path[len - 1] == '/')
		return -ENOENT;

	for(; pos && path[pos - 1] != '/'; -- pos);
	if(!pos)
		return -ENOENT;
	for(; pos + user1len < len; ++ user1len)
		user1[user1len] = path[pos + user1len];
	user1[user1len] = '\0';

	for(-- pos; pos && path[pos - 1] != '/'; -- pos);
	if(!pos)
		return -ENOENT;
	for(; pos + user2len < len - user1len - 1; ++ user2len)
		user2[user2len] = path[pos + user2len];
	user2[user2len] = '\0';

	for(; envilen < pos; ++ envilen)
		envi[envilen] = path[envilen];
	
	char *path1 = (char *)malloc(len + 1);
	char *path2 = (char *)malloc(len + 1);
	memcpy(path1, path, len);
	path1[len] = '\0';
	for(int i = 0; i < envilen; ++ i)
		path2[i] = envi[i];
	for(int i = 0; i < user1len; ++ i)
		path2[envilen + i] = user1[i];
	path2[envilen + user1len] = '/';
	for(int i = 0; i < user2len; ++ i)
		path2[envilen + user1len + i + 1] = user2[i];
	path2[len] = '\0';

	struct Node *us1, *us2;
	if(Find(path1, &us1) != 0 || Find(path2, &us2) != 0)
		return -ENOENT;
	if(us1 -> type != 1 || us2 -> type != 1)
		return -ENOENT;
	
	char *old = us1 -> contents;
	int tlen = user2len + 3;
	char *title = (char *)malloc(tlen + 1);
	title[0] = '[';
	for(int i = 0; i < user2len; ++ i)
		title[i + 1] = user2[i];
	title[tlen - 2] = ']', title[tlen - 1] = '\n', title[tlen] = '\0';
	
	strcat(debug, strdup(title));
	strcat(debug, "\n");
	
	int ns = 0;
	if(offset + tlen + size > strlen(old))
		ns = offset + tlen + size;
	else
		ns = strlen(old);
	
	char *newcontents = (char *)malloc(ns + 1);
	memcpy(newcontents, old, strlen(old));
	memcpy(newcontents + offset, title, tlen);
	memcpy(newcontents + offset + tlen, buf, size);
	newcontents[ns] = '\0';

	us1 -> contents = newcontents;
	us2 -> contents = strdup(newcontents);

	return size;
}
*/

static int ChatWrite(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *info) {
	(void) info;
	int len = strlen(path), user1len = 0, user2len = 0, envilen = 0, pos1 = len, pos2 = 1;
	char user1[MaxFileLen], user2[MaxFileLen];
	char *envi = (char *)malloc(len + 1);

	if(path[len - 1] == '/')
		return -ENOENT;

	for(; pos1 && path[pos1 - 1] != '/'; -- pos1);
	if(!pos1)
		return -ENOENT;
	for(; pos1 + user1len < len; ++ user1len)
		user1[user1len] = path[pos1 + user1len];
	user1[user1len] = '\0';

	for(; pos2 < len && path[pos2] != '/'; ++ pos2);
	if(pos2 >= len)
		return -ENOENT;
	for(; user2len + 1 < pos2; ++ user2len)
		user2[user2len] = path[user2len + 1];
	user2[user2len] = '\0';

	for(int i = pos2 + 1; i < pos1; ++ envilen, ++ i)
		envi[envilen] = path[i];
	
	char *path1 = (char *)malloc(len + 1);
	char *path2 = (char *)malloc(len + 1);
	memcpy(path1, path, len);
	path1[len] = '\0';
	path2[0] = '/';
	for(int i = 0; i < user1len; ++ i)
		path2[i + 1] = user1[i];
	path2[user1len + 1] = '/';
	for(int i = 0; i < envilen; ++ i)
		path2[user1len + i + 2] = envi[i];
	for(int i = 0; i < user2len; ++ i)
		path2[envilen + user1len + i + 2] = user2[i];
	path2[len] = '\0';

	// strcat(debug, strdup(path));
	// strcat(debug, "\n");
	// strcat(debug, strdup(user1));
	// strcat(debug, "\n");
	// strcat(debug, strdup(user2));
	// strcat(debug, "\n");
	// strcat(debug, strdup(path1));
	// strcat(debug, "\n");
	// strcat(debug, strdup(path2));
	// strcat(debug, "\n");

	struct Node *us1, *us2;
	if(Find(path1, &us1) != 0 || Find(path2, &us2) != 0)
		return -ENOENT;
	if(us1 -> type != 1 || us2 -> type != 1)
		return -ENOENT;
	
	char *old = us1 -> contents;
	int tlen = user2len + 3;
	char *title = (char *)malloc(tlen + 1);
	title[0] = '[';
	for(int i = 0; i < user2len; ++ i)
		title[i + 1] = user2[i];
	title[tlen - 2] = ']', title[tlen - 1] = '\n', title[tlen] = '\0';
	
	// strcat(debug, strdup(title));
	// strcat(debug, "\n");
	
	int ns = 0;
	if(offset + tlen + size > strlen(old))
		ns = offset + tlen + size;
	else
		ns = strlen(old);
	
	char *newcontents = (char *)malloc(ns + 1);
	memcpy(newcontents, old, strlen(old));
	memcpy(newcontents + offset, title, tlen);
	memcpy(newcontents + offset + tlen, buf, size);
	newcontents[ns] = '\0';

	us1 -> contents = newcontents;
	us2 -> contents = strdup(newcontents);

	return size;
}

static int ChatRelease(const char *path, struct fuse_file_info *info) {
	struct Node *nd;
	if(Find(path, &nd) != 0)
		return -ENOENT;
	if(nd -> type != 1)
		return -ENOENT;
	return 0;
}

static int ChatMknod(const char *path, mode_t mode, dev_t rdev) {
	(void) rdev;
	(void) mode;
	int len = strlen(path), namelen = 0, pos = len;
	char name[MaxFileLen];
	for(; pos && path[pos - 1] != '/'; -- pos);
	if(pos <= 0)
		return -ENOENT;
	for(; pos + namelen < len; ++ namelen)
		name[namelen] = path[pos + namelen];
	name[namelen] = '\0';

	int plen = pos;
	char *parpath = (char *)malloc(plen + 1);
	for(int i = 0; i < plen; ++ i)
		parpath[i] = path[i];
	parpath[plen] = '\0';
	
	struct Node *p;
	if(Find(parpath, &p) != 0)
		return -ENOENT;
	if(p -> type != 2)
		return -ENOENT;
	struct Node *now = NewNode(1, name, "", NULL, NULL, NULL, NULL);
	Add(p, now);
	return 0;
}

static int ChatUnlink(const char *path) {
	struct Node *nd;
	if(Find(path, &nd) != 0)
		return -ENOENT;
	if(nd -> type != 1)
		return -ENOENT;
	Del(nd);
	return 0;
}

static int ChatMkdir(const char *path, mode_t mode) {
	(void) mode;
	int len = strlen(path), namelen = 0, pos = len;
	char name[MaxFileLen];
	for(; pos && path[pos - 1] != '/'; -- pos);
	if(pos <= 0)
		return -ENOENT;
	for(; pos + namelen < len; ++ namelen)
		name[namelen] = path[pos + namelen];
	name[namelen] = '\0';
	
	int plen = pos;
	char *parpath = (char *)malloc(plen + 1);
	for(int i = 0; i < plen; ++ i)
		parpath[i] = path[i];
	parpath[plen] = '\0';

	struct Node *p;
	if(Find(parpath, &p) != 0)
		return -ENOENT;
	if(p -> type != 2)
		return -ENOENT;
	struct Node *now = NewNode(2, name, "", NULL, NULL, NULL, NULL);
	Add(p, now);
	return 0;
}

static int ChatRmdir(const char *path) {
	struct Node *nd;
	if(Find(path, &nd) != 0)
		return -ENOENT;
	if(nd -> type != 2)
		return -ENOENT;
	Del(nd);
	return 0;
}

static int ChatUtimens(const char *path, const struct timespec tv[2], struct fuse_file_info *info) {
	return 0;
}

static const struct fuse_operations chat_oper = {
	.init = InitChat,
	.getattr = ChatGetattr,
	.readdir = ChatReaddir,
	.open = ChatOpen,
	.read = ChatRead,
	.access = ChatAccess,
	.write = ChatWrite,
	.release = ChatRelease,
	.mknod = ChatMknod,
	.unlink = ChatUnlink,
	.mkdir = ChatMkdir,
	.rmdir = ChatRmdir,
	.utimens = ChatUtimens,
};

int main(int argc, char *argv[]) {
	int ret;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	root = NewNode(2, "/", "", NULL, NULL, NULL, NULL);
	log = NewNode(1, "log", "", NULL, NULL, NULL, NULL);
	debug = (char *)malloc(1500);
	Add(root, log);
	log -> contents = debug;
	ret = fuse_main(args.argc, args.argv, &chat_oper, NULL);

	fuse_opt_free_args(&args);
	return ret;
}