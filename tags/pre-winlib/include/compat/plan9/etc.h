struct Ureg {
 int smthng;
};

typedef struct Ureg Ureg;

typedef int Rendez; // use address only?

struct Dirtab
{
    const char *name;
    Qid qid;
    int length;
    int perm;
};

typedef struct Dirtab Dirtab;


struct Chan
{
    int dev;
    int offset;
    Qid qid;
};

typedef struct Chan Chan;


void tsleep( Rendez *, int (*ready)(void*), void *arg, int b_possible_timeout);
void wakeup( Rendez * );

int waserror(void);
void nexterror(void);
void poperror(void);


struct Walkqid
{
};

typedef struct Walkqid Walkqid;

Walkqid * devwalk(Chan *c, Chan *nc, char **name, int nname, Dirtab *dir, int ndir, int (*gen)(Chan *c, char*, Dirtab *tab, int ntab, int i, Dir *dp) );
int devstat(Chan *c, uchar *dp, int n, Dirtab *dir, int ndir, int (*gen)(Chan *c, char*, Dirtab *tab, int ntab, int i, Dir *dp) );
Chan* devopen(Chan *c, int omode, Dirtab *dir, int ndir, int (*gen)(Chan *c, char*, Dirtab *tab, int ntab, int i, Dir *dp) );

long devdirread(Chan *c, void *a, long n, Dirtab *dir, int ndir, int (*gen)(Chan *c, char*, Dirtab *tab, int ntab, int i, Dir *dp) );


Chan * devattach( char , char *spec );


void error(const char *e);




int     devreset( Chan *c );
int     devinit( Chan *c );
int    devshutdown( Chan *c );
int    devcreate( Chan *c );
int    devbread( Chan *c );
int    devbwrite( Chan *c );
int    devremove( Chan *c );
int    devwstat( Chan *c );


void devdir( Chan *c, Qid *qid, char *path, int _x1, it _whatTheFuckIs_eve, int perm, Dir *dp);



struct Dev
{
	char c;
	char *name;

void *    dev_reset;
void *    dev_init;
void *    dev_shutdown;
void *    dev_attach;
void *    dev_walk;
void *    dev_stat;
void *    dev_open;
void *    dev_create;
void *    dev_close;
void *    dev_read;
void *    dev_bread;
void *    dev_write;
void *    dev_bwrite;
void *    dev_remove;
void *    dev_wstat;

};


typedef struct Dev Dev;



void intrenable(int irq_no, void (*intr)(Ureg*, void*), int _x, int _bus, const char *name );
// errno_t
int ioalloc( int iobase, int nio, int exclusive, const char *name );







